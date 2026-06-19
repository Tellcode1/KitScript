/**
 * MIT License
 *
 * Copyright (c) 2026 Tellcode1
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "../inc/kit.exec.h"

#include "../inc/kit.bfunc.h"
#include "../inc/kit.cast.h"
#include "../inc/kit.cc.h"
#include "../inc/kit.list.h"
#include "../inc/kit.operate.h"
#include "../inc/kit.perr.h"
#include "../inc/kit.pool.h"
#include "../inc/kit.reg.h"
#include "../inc/kit.script.h"
#include "../inc/kit.stdafx.h"
#include "../inc/kit.struct.h"
#include "../inc/kit.sysexpose.h"
#include "../inc/kit.var.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define remove(var)                                                                                                                                  \
  do {                                                                                                                                               \
    kit_var_release(vm->pool, var);                                                                                                                  \
    *(var) = KIT_NULLVAR;                                                                                                                            \
  } while (0)

#define print_err(...)                                                                                                                               \
  do {                                                                                                                                               \
    fputs("[KitExec::vm] [error] ", kit_log_file ? kit_log_file : stderr);                                                                           \
    fprintf(kit_log_file ? kit_log_file : stderr, __VA_ARGS__);                                                                                      \
  } while (0)

static inline const kit_builtin_func*
get_builtin_func_hashed(u32 hash)
{
  for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
    if (kit_hash(kit_builtins_funcs[i].name, strlen(kit_builtins_funcs[i].name)) == hash) return &kit_builtins_funcs[i];
  }
  return NULL;
}

static kit_ecode
call(kit_vm* vm, const kit_exec_info* info, u32 hash, kit_var* args, u32 nargs, kit_var* ret)
{
  int e = 0;

  // Special handling
  // builtin functions can not return errors...
  if (hash == kit_hash(KIT_PANIC_FUNCTION_NAME, strlen(KIT_PANIC_FUNCTION_NAME))) {
    print_err("---> PANIC <---\n");
    return KIT_EPANIC;
  }

  // builtins
  const kit_builtin_func* builtin = get_builtin_func_hashed(hash);
  if (builtin != NULL) {
    e = builtin->func(vm, args, nargs, ret);
    if (e) { print_err("builtin %s returned %s\n", builtin->name, kit_ecode_str(e)); }
    goto pop_and_ret;
  }

  // extern
  for (u32 i = 0; i < info->nextern_funcs; i++) {
    const char* name = info->extern_funcs[i].name;
    if (kit_hash(name, strlen(name)) != hash) continue;
    if (info->extern_funcs[i].func) { e = info->extern_funcs[i].func(vm, args, nargs, ret); }
    goto pop_and_ret;
  }

  // user defined
  for (u32 f = 0; f < info->nfuncs; f++) {
    if (info->funcs[f].name_hash != hash) continue;

    kit_vm fork_vm = { 0 };
    if (kit_vm_fork(hash, vm, &fork_vm) < 0) {
      e = KIT_EMALLOC;
      goto pop_and_ret;
    }

    kit_exec_info fi = {
      .gvars           = info->gvars,
      .code            = info->funcs[f].code,
      .args            = args,
      .literals        = info->literals,
      .literals_hashes = info->literals_hashes,
      .funcs           = info->funcs,
      .extern_funcs    = info->extern_funcs,
      .code_count      = info->funcs[f].code_count,
      .nargs           = info->funcs[f].nargs,
      .nliterals       = info->nliterals,
      .nfuncs          = info->nfuncs,
      .nextern_funcs   = info->nextern_funcs,
      .extern_vars     = info->extern_vars,
      .nextern_vars    = info->nextern_vars,
      .structs         = info->structs,
      .nstructs        = info->nstructs,
    };

    e = kit_exec(&fork_vm, &fi, ret);
    kit_vm_free(&fork_vm);

    if (e) goto pop_and_ret;

    goto pop_and_ret;
  }

  const char* lookup = NULL;
  for (u32 i = 0; i < info->nnames; i++) {
    if (hash == info->names_hashes[i]) {
      lookup = info->names[i];
      break;
    }
  }

  if (lookup) {
    print_err("Function %s not defined\n", lookup);
  } else {
    print_err("Function %u not defined\n", hash);
  }

  *ret = KIT_NULLVAR;
  return KIT_EUNDEFINED;

pop_and_ret:
  /* nothing to pop */
  return e;
}

kit_var*
read_args_vector(kit_refdobj_pool* pool, kit_var* regs, kit_var* stack, u32 sp, u32 nelems)
{
  kit_var* tmp = kit_aligned_malloc(nelems * sizeof(kit_var), 32);
  if (!tmp) return NULL;

  u32 i = 0;

  /* copy first 16 arguments from registers */
  for (; i < MIN(nelems, KIT_REG_ARG_COUNT); i++) {
    memcpy(&tmp[i], &regs[KIT_REG_ARG0 + i], sizeof(kit_var));
    kit_var_acquire(pool, &tmp[i]);
  }

  /* copy rest of the arguments from the stack */
  for (; i < nelems; i++) {
    memcpy(&tmp[i], &stack[sp - nelems + i], sizeof(kit_var));
    kit_var_acquire(pool, &tmp[i]);
  }

  return tmp;
}

kit_ecode
kit_exec(kit_vm* vm, const kit_exec_info* const info, kit_var* ret)
{
  /* Initial state check. */
  if ((info->nargs > 0 && info->args == NULL) || (info->code == NULL && info->code_count != 0)
      || (info->extern_funcs == NULL && info->nextern_funcs > 0) || (info->extern_vars == NULL && info->nextern_vars > 0)
      || (info->funcs == NULL && info->nfuncs > 0) || (info->literals == NULL && info->nliterals > 0)) {
    print_err("Corrupted info during execution (broken fork?)\n");
    return KIT_EBADARG;
  }

  /* error code */
  kit_ecode e = KIT_OK;

  kit_var regs[KIT_REG_COUNT];
  for (u32 i = 0; i < KIT_REG_COUNT; i++) { regs[i] = KIT_NULLVAR; }

  size_t   stack_capacity = 256;
  kit_var* stack          = kit_xalloc(stack_capacity, sizeof(kit_var));
  kit_var* sp             = &regs[KIT_REG_SP];
  kit_var* ip             = &regs[KIT_REG_IP];

  regs[KIT_REG_NIL] = KIT_NULLVAR;

  *sp = kit_var_from_int(0);
  *ip = kit_var_from_int(0);

  for (u32 i = 0; i < info->nargs; i++) {
    u32 dst = KIT_REG_ARG0 + i;
    if (dst < KIT_REG_ARG_COUNT) {
      kit_var_shallow_cpy(&info->args[i], &regs[dst]);
      kit_var_acquire(vm->pool, &regs[dst]);
    } else {
      /* Push to stack */
      stack[sp->val.i] = info->args[i];
      kit_var_acquire(vm->pool, &stack[sp->val.i]);
      sp->val.i++;
    }
  }

  for (ip->val.i = 0; ip->val.i < info->code_count; ip->val.i++) {
    kit_ins ins = info->code[ip->val.i];

    /* set nilreg to null before every instruction, just for safety */
    regs[KIT_REG_NIL] = KIT_NULLVAR;

    switch ((eir_opcode_bits)ins.opcode) {
      case EIR_OPCODE_LOADK: {
        u32 dst = ins.loadk.dst;
        u32 id  = ins.loadk.id;

        kit_var v = KIT_NULLVAR;

        bool found = false;
        for (u32 i = 0; i < info->nliterals; i++) {
          if (id == info->literals_hashes[i]) {
            v     = info->literals[i];
            found = true;
            break;
          }
        }

        if (!found) {
          print_err("LOADK: hash %u not found in literal table\n", id);
          e = KIT_EBADARG;
          goto RET;
        }

        remove(&regs[dst]);
        kit_var_deep_cpy(vm->pool, &v, &regs[dst]);
        break;
      }
      case EIR_OPCODE_RET: {
        u32 ret_val = ins.ret.return_value;

        /* store return value */
        kit_var_shallow_cpy(&regs[ret_val], ret);
        kit_var_acquire(vm->pool, ret);

        /* return from this procedure */
        e = KIT_OK;
        goto RET;
      }

      case EIR_OPCODE_ASSERT: {
        u32 cond    = ins.assertion.cond;
        u32 line_id = ins.assertion.line_id;

        if (kit_cast_to_bool(&regs[cond]) == false) {
          char* line = NULL;
          for (u32 i = 0; i < info->nliterals; i++) {
            if (info->literals_hashes[i] == line_id) {
              line = KIT_VAR_AS_STRING(&info->literals[i])->s;
              break;
            }
          }

          print_err("ASSERTION FAILED: %s\n", line ? line : "[line debug symbol not found]");

          /* propogate assertion failure to top, it will decide what to do */
          e = KIT_EASSERT;
          goto RET;
        }

        break;
      }

      case EIR_OPCODE_MOV: {
        u32 src = ins.mov.src;
        u32 dst = ins.mov.dst;
        if (src == dst) break;

        remove(&regs[dst]);
        kit_var_shallow_cpy(&regs[src], &regs[dst]);
        kit_var_acquire(vm->pool, &regs[dst]);
        break;
      }

      case EIR_OPCODE_MOVI: {
        int value = ins.movi.value;
        u32 dst   = ins.movi.dst;

        kit_var src = kit_var_from_int(value);

        remove(&regs[dst]);
        kit_var_shallow_cpy(&src, &regs[dst]);
        // kit_var_acquire(vm->pool,&regs[dst]);
        break;
      }

      case EIR_OPCODE_MOVF: {
        double value = ins.movf.value;
        u32    dst   = ins.movf.dst;

        kit_var src = kit_var_from_float(value);

        remove(&regs[dst]);
        kit_var_shallow_cpy(&src, &regs[dst]);
        // kit_var_acquire(vm->pool,&regs[dst]);
        break;
      }

      case EIR_OPCODE_ADD:
      case EIR_OPCODE_SUB:
      case EIR_OPCODE_MUL:
      case EIR_OPCODE_DIV:
      case EIR_OPCODE_MOD:
      case EIR_OPCODE_EXP:
      case EIR_OPCODE_AND:
      case EIR_OPCODE_OR:
      case EIR_OPCODE_BAND:
      case EIR_OPCODE_BOR:
      case EIR_OPCODE_XOR:
      case EIR_OPCODE_EQL:
      case EIR_OPCODE_NEQ:
      case EIR_OPCODE_LT:
      case EIR_OPCODE_LTE:
      case EIR_OPCODE_GT:
      case EIR_OPCODE_GTE: {
        u32 dst = ins.binop.dst;
        u32 a   = ins.binop.a;
        u32 b   = ins.binop.b;

        kit_var l = regs[a];
        kit_var r = regs[b];

        remove(&regs[dst]);
        kit_var result = operate(l, r, ins.opcode);
        // kit_var_acquire(vm->pool,&result);

        /* removing elements from the stack, free them */
        if (dst == KIT_REG_SP && (result.val.i < sp->val.i)) {
          while (sp->val.i > result.val.i) { /* while old value is greater than new value */
            remove(&stack[--sp->val.i]);
          }
        }

        regs[dst] = result;
        break;
      }
      case EIR_OPCODE_BNOT:
      case EIR_OPCODE_NEG:
      case EIR_OPCODE_NOT: {
        u32 dst = ins.unop.dst;
        u32 a   = ins.unop.a;

        kit_var l = KIT_NULLVAR;
        kit_var r = regs[a];

        remove(&regs[dst]);
        regs[dst] = operate(l, r, ins.opcode);
        kit_var_acquire(vm->pool, &regs[dst]);
        break;
      }
      case EIR_OPCODE_INC:
      case EIR_OPCODE_DEC: {
        u32 dst = ins.unop.dst;
        u32 rhs = ins.unop.a;

        /* There are cases (especially after register allocation && optimization) where this is not correct */
        // if (dst != rhs) { print_err("operand != dst for inc/dec? Possibly corrupted bytecode stream. Continuing.\n"); }

        kit_var* d = &regs[dst];
        kit_var* r = &regs[rhs];

        kit_var_type type  = r->type;
        int          ival  = kit_cast_to_int(r);
        double       fval  = kit_cast_to_float(r);
        int          delta = (ins.opcode == EIR_OPCODE_INC) ? 1 : -1;

        remove(d);

        if (type == KIT_VARTYPE_INT || type == KIT_VARTYPE_CHAR || type == KIT_VARTYPE_BOOL) {
          d->type  = KIT_VARTYPE_INT;
          d->val.i = ival + delta;
        } else if (type == KIT_VARTYPE_FLOAT) {
          d->type  = KIT_VARTYPE_FLOAT;
          d->val.f = fval + (double)delta;
        } else {
          d->type  = KIT_VARTYPE_INT;
          d->val.i = delta; // null + 1 = 1
        }
        break;
      }

      case EIR_OPCODE_MK_MAP: {
        u32 dst    = ins.mk_map.dst;
        u32 npairs = ins.mk_map.npairs;

        /* read every variable */
        kit_var* tmp = read_args_vector(vm->pool, regs, stack, sp->val.i, npairs * 2);

        kit_var* dstp = &regs[dst];
        remove(dstp); // overwriting it

        dstp->type    = KIT_VARTYPE_MAP;
        dstp->val.map = kit_refdobj_pool_acquire(vm->pool);

        /* Constructing it directly in our register. No need for refcounting here. */
        if (kit_map_init(vm->pool, tmp, npairs, KIT_VAR_AS_MAP(dstp)) != 0) {
          e = KIT_EMALLOC;

          for (u32 i = 0; i < npairs * 2; i++) { remove(&tmp[i]); }
          kit_aligned_free(tmp);
          goto RET;
        }

        for (u32 i = 0; i < npairs * 2; i++) { remove(&tmp[i]); }
        kit_aligned_free(tmp);
        break;
      }
        // default: printf("%i\n", ins.opcode); assert(0);

      case EIR_OPCODE_LABEL:
      case EIR_OPCODE_NOP: break;

      case EIR_OPCODE_MK_LIST: {
        u32 dst    = ins.mk_list.dst;
        u32 nelems = ins.mk_list.nelems;

        kit_var* tmp = read_args_vector(vm->pool, regs, stack, sp->val.i, nelems);

        kit_var* dstp = &regs[dst];
        remove(dstp); // overwriting it

        dstp->type     = KIT_VARTYPE_LIST;
        dstp->val.list = kit_refdobj_pool_acquire(vm->pool);

        /* Constructing it directly in our register. No need for refcounting here. */
        if (kit_list_init(vm->pool, tmp, nelems, KIT_VAR_AS_LIST(dstp)) != 0) {
          for (u32 i = 0; i < nelems; i++) { remove(&tmp[i]); }
          kit_aligned_free(tmp);

          e = KIT_EMALLOC;
          goto RET;
        }

        for (u32 i = 0; i < nelems; i++) { remove(&tmp[i]); }
        kit_aligned_free(tmp);

        break;
      }
      case EIR_OPCODE_INDEX: {
        u32 dst   = ins.index.dst;
        u32 base  = ins.index.base;
        u32 index = ins.index.index;

        kit_var tmp = KIT_NULLVAR;

        if (kit_var_index(vm->pool, &regs[base], &regs[index], &tmp)) {
          tmp = KIT_NULLVAR;
          break;
        }

        remove(&regs[dst]);
        kit_var_shallow_cpy(&tmp, &regs[dst]);
        kit_var_acquire(vm->pool, &regs[dst]);
        break;
      }

      case EIR_OPCODE_INDEX_ASSIGN: {
        u32 value = ins.index_assign.value;
        u32 base  = ins.index_assign.base;
        u32 index = ins.index_assign.index;

        if (regs[base].type == KIT_VARTYPE_STRING) {
          print_err("Can not index assign strings\n");
          e = KIT_EMALFORM;
          goto RET;
        }

        if (kit_var_index_assign(vm->pool, &regs[base], &regs[index], &regs[value])) {
          print_err("kit_var_index_assign (%i): error\n", ip->val.i);
          e = KIT_EMALFORM;
          goto RET;
        }
        break;
      }

      case EIR_OPCODE_CALL: {
        u32 dst         = ins.call.dst;
        u32 function_id = ins.call.function_id;
        u32 nargs       = ins.call.nargs;

        kit_var* args = read_args_vector(vm->pool, regs, stack, sp->val.i, nargs);
        if (!args) {
          e = KIT_EMALLOC;
          goto RET;
        }

        /* overwriting it */
        remove(&regs[dst]);

        e = call(vm, info, function_id, args, nargs, &regs[dst]);
        /* No need to acquire regs[dst], it's deep copied / preacquired. */

        for (u32 i = 0; i < nargs; i++) { remove(&args[i]); }
        kit_aligned_free(args);

        if (e) {
          print_err("call() returned error: %s\n", kit_ecode_str(e));
          goto RET;
        }

        break;
      }

      case EIR_OPCODE_JMP: {
        u32 target = ins.jmp.target;

        ip->val.i = (int)(target - 1);
        break;
      }
      case EIR_OPCODE_JZ: {
        u32 target = ins.cj.target;
        u32 cond   = ins.cj.condition;

        if (!kit_cast_to_bool(&regs[cond])) { ip->val.i = (int)(target - 1); }
        break;
      }
      case EIR_OPCODE_JNZ: {
        u32 target = ins.cj.target;
        u32 cond   = ins.cj.condition;

        if (kit_cast_to_bool(&regs[cond])) { ip->val.i = (int)(target - 1); }
        break;
      }

      /* MEMBER_ACCESS [dst] [base] [member ID : u32] */
      case EIR_OPCODE_MEMBER_ACCESS: {
        u32 dst       = ins.member_access.dst;
        u32 base      = ins.member_access.base;
        u32 member_id = ins.member_access.member_id;

        if (regs[base].type == KIT_VARTYPE_VEC2 || regs[base].type == KIT_VARTYPE_VEC3 || regs[base].type == KIT_VARTYPE_VEC4) {
          const u32 x = kit_hash("x", strlen("x"));
          const u32 y = kit_hash("y", strlen("y"));
          const u32 z = kit_hash("z", strlen("z"));
          const u32 w = kit_hash("w", strlen("w"));

          kit_vec4* ptr = &regs[base].val.vec4;

          double component = 0.0;
          if (member_id == x) {
            component = (*ptr)[0];
          } else if (member_id == y) {
            component = (*ptr)[1];
          } else if (member_id == z) {
            component = (*ptr)[2];
          } else if (member_id == w) {
            component = (*ptr)[3];
          } else {
            print_err("Member access %u invalid for vector\n", member_id);
            e = KIT_EMALFORM;
            goto RET;
          }

          remove(&regs[dst]);
          regs[dst] = kit_var_from_float(component);
          // kit_var_acquire(vm->pool,&regs[dst]); // useless, vectors are not reference based
          break;
        }

        if (regs[base].type != KIT_VARTYPE_STRUCT) {
          print_err("Can not access a member within non struct/vector type: %s\n", kit_var_type_to_string(regs[base].type));
          e = KIT_EMALFORM;
          goto RET;
        }

        kit_var push = KIT_NULLVAR;

        kit_struct* st = KIT_VAR_AS_STRUCT(&regs[base]);
        for (u32 i = 0; i < st->member_count; i++) {
          if (st->member_hashes[i] == member_id) {
            kit_var_shallow_cpy(&st->members[i], &push);
            break;
          }
        }

        remove(&regs[dst]);
        kit_var_shallow_cpy(&push, &regs[dst]);
        kit_var_acquire(vm->pool, &regs[dst]);
        break;
      }
      /* MEMBER_ASSIGN [value] [base] [member ID : u32] */
      case EIR_OPCODE_MEMBER_ASSIGN: {
        u32 value     = ins.member_assign.value;
        u32 base      = ins.member_assign.base;
        u32 member_id = ins.member_assign.member_id;

        if (regs[base].type == KIT_VARTYPE_VEC2 || regs[base].type == KIT_VARTYPE_VEC3 || regs[base].type == KIT_VARTYPE_VEC4) {
          const u32 x = kit_hash("x", strlen("x"));
          const u32 y = kit_hash("y", strlen("y"));
          const u32 z = kit_hash("z", strlen("z"));
          const u32 w = kit_hash("w", strlen("w"));

          kit_vec4* ptr = &regs[base].val.vec4;
          double    val = kit_cast_to_float(&regs[value]);

          if (member_id == x) {
            (*ptr)[0] = val;
          } else if (member_id == y) {
            (*ptr)[1] = val;
          } else if (member_id == z) {
            (*ptr)[2] = val;
          } else if (member_id == w) {
            (*ptr)[3] = val;
          } else {
            print_err("Member access %u invalid for vector\n", member_id);
            e = KIT_EMALFORM;
            goto RET;
          }
          break;
        }

        kit_struct* st = KIT_VAR_AS_STRUCT(&regs[base]);
        for (u32 i = 0; i < st->member_count; i++) {
          if (st->member_hashes[i] == member_id) {
            kit_var* member = &st->members[i];
            remove(member);
            kit_var_shallow_cpy(&regs[value], member);
            kit_var_acquire(vm->pool, member);
            break;
          }
        }
        break;
      }

      /**
       * MK_STRUCT [dst] [ID] [baseArgRegs]       */
      case EIR_OPCODE_MK_STRUCT: {
        u32 dst = ins.mk_struct.dst;
        u32 id  = ins.mk_struct.struct_id;

        const kitc_struct_information* st = NULL;
        for (u32 i = 0; i < info->nstructs; i++) {
          if (info->structs[i].name_hash == id) {
            st = &info->structs[i];
            break;
          }
        }

        if (!st) {
          print_err("Structure %u not found in table\n", id);
          e = KIT_EUNDEFINED;
          goto RET;
        }

        kit_var* dstp = &regs[dst];
        remove(dstp);

        dstp->type      = KIT_VARTYPE_STRUCT;
        dstp->val.struc = kit_refdobj_pool_acquire(vm->pool);

        if (kit_struct_init(st->name, st->fields_count, (const char**)st->field_names, KIT_VAR_AS_STRUCT(dstp)) != 0) {
          e = KIT_EMALLOC;
          goto RET;
        }

        kit_var* args = read_args_vector(vm->pool, regs, stack, sp->val.i, st->fields_count);
        for (u32 i = 0; i < st->fields_count; i++) {
          kit_var* member = &KIT_VAR_AS_STRUCT(dstp)->members[i];
          remove(member);
          kit_var_shallow_cpy(&args[i], member);
          kit_var_acquire(vm->pool, member);

          remove(&args[i]);
        }
        kit_aligned_free(args);
        break;
      }
      case EIR_OPCODE_GETG: {
        u32 dst = ins.getg.dst;
        u32 src = ins.getg.src;
        remove(&regs[dst]);
        kit_var_shallow_cpy(&info->gvars[src], &regs[dst]);
        kit_var_acquire(vm->pool, &regs[dst]);
        break;
      }

      case EIR_OPCODE_SETG: {
        u32 dst = ins.setg.dst;
        u32 src = ins.setg.src;
        remove(&info->gvars[dst]);
        kit_var_shallow_cpy(&regs[src], &info->gvars[dst]);
        kit_var_acquire(vm->pool, &info->gvars[dst]);
        break;
      }

      case EIR_OPCODE_MOVG: {
        u32 dst = ins.movg.dst;
        u32 src = ins.movg.src;
        remove(&info->gvars[dst]);
        kit_var_shallow_cpy(&info->gvars[src], &info->gvars[dst]);
        kit_var_acquire(vm->pool, &info->gvars[dst]);
        break;
      }

      case EIR_OPCODE_PUSH: {
        u32 reg = ins.push.reg;

        if (sp->val.i >= stack_capacity) {
          size_t   new_capacity = stack_capacity * 2;
          kit_var* new_stack    = realloc(stack, sizeof(kit_var) * new_capacity);
          if (!new_stack) {
            e = KIT_EMALLOC;
            goto RET;
          }

          stack          = new_stack;
          stack_capacity = new_capacity;
        }

        remove(&stack[sp->val.i]); /* overwriting it. */
        stack[sp->val.i] = regs[reg];
        kit_var_acquire(vm->pool, &stack[sp->val.i]); /* temporary hold */

        sp->val.i++;
        break;
      }
      case EIR_OPCODE_POP: {
        u32 reg = ins.push.reg;
        remove(&regs[reg]); /* overwriting it. */

        sp->val.i--;
        regs[reg] = stack[sp->val.i];
        remove(&stack[sp->val.i]); /* release temporary hold */
        break;
      }
    }
  }

RET:
  false;

  const u32 sp_save = sp->val.i;
  for (u32 i = 0; i < KIT_REG_COUNT; i++) { remove(&regs[i]); }
  for (i32 i = 0; i < sp_save; i++) { remove(&stack[i]); }
  free(stack);
  return e;
}

kit_ecode
kit_script_call(kit_script* s, const char* func_name, kit_var* args, u32 nargs, kit_var* ret)
{
  u32 hash = kit_hash(func_name, strlen(func_name));

  for (u32 i = 0; i < s->compiled.functions_count; i++) {
    if (hash == s->compiled.functions[i].name_hash) {
      kit_exec_info info = {
        .code            = s->compiled.functions[i].code,
        .args            = args,
        .literals        = s->compiled.literals,
        .literals_hashes = s->compiled.literals_hashes,
        .funcs           = s->compiled.functions,
        .extern_funcs    = s->extern_funcs,
        .code_count      = s->compiled.functions[i].code_count,
        .nargs           = nargs,
        .nliterals       = s->compiled.literals_count,
        .nfuncs          = s->compiled.functions_count,
        .nextern_funcs   = s->nxtern_funcs,
        .extern_vars     = s->extern_vars,
        .nextern_vars    = s->nextern_vars,
        .structs         = s->compiled.structs,
        .nstructs        = s->compiled.structs_count,
      };

      if (s->compiled.functions[i].nargs != nargs) {
        print_err("Function expects %u arguments (%u were provided)\n", s->compiled.functions[i].nargs, nargs);
        return KIT_EBADARG;
      }

      return kit_exec(s->vm, &info, ret);
    }
  }

  return KIT_EUNDEFINED;
}
