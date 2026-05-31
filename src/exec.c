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

#include "../inc/exec.h"

#include "../inc/bfunc.h"
#include "../inc/cast.h"
#include "../inc/cc.h"
#include "../inc/list.h"
#include "../inc/operate.h"
#include "../inc/perr.h"
#include "../inc/pool.h"
#include "../inc/reg.h"
#include "../inc/script.h"
#include "../inc/stdafx.h"
#include "../inc/struct.h"
#include "../inc/var.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASTE(x, y) x##y

// clang-format off
#define TRY(expr) do { int PASTE(__e__, __LINE__) = 0; if ((PASTE(__e__, __LINE__) = (expr))) { return PASTE(__e__, __LINE__); } } while (0)
#define TRY_V(expr) do {   int PASTE(__e__, __LINE__) = (expr);   if (PASTE(__e__, __LINE__)) { return -1; } } while (0)
// clang-format on

// static inline e_var*
// stack_find_rev(const struct stack* s, u16 id)
// {
//   for (int i = 0; i < s->size; i++)
//   {
//     // if (id == s->stack[i].) {TRY_V( e_stack_push(info->stack, info->stack.stack[variables[i].offset]); )}
//   }
// }

#define print_err(...)                                                                                                                               \
  do {                                                                                                                                               \
    fputs("[eexec::vm] ", stderr);                                                                                                                   \
    fprintf(stderr, __VA_ARGS__);                                                                                                                    \
  } while (0)

static inline const e_builtin_func*
get_builtin_func_hashed(u32 hash)
{
  for (u32 i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (e_hash(eb_funcs[i].name, strlen(eb_funcs[i].name)) == hash) return &eb_funcs[i];
  }
  return NULL;
}

static inline const char*
lookup(const char** names, const u32* names_hashes, u32 nnames, u32 name)
{
  for (u32 i = 0; i < nnames; i++) {
    if (names_hashes[i] == name) { return names[i]; }
  }
  return "[symbol not found]";
}

static e_ecode
call(const e_exec_info* info, u32 hash, e_var* args, u32 nargs, e_var* ret)
{
  int e = 0;

  // Special handling
  // builtin functions can not return errors...
  if (hash == e_hash("PANIC", strlen("PANIC"))) {
    print_err("---> PANIC <---\n");
    return E_EPANIC;
  }

  // printf("call %s\n", lookup(info->names, info->names_hashes, info->nnames, hash));

  /**
   * Copy argumenst into a temporary array and remove them from the stack.
   */
  e_var* args_copy = nargs > 0 ? e_xalloc(nargs, sizeof(e_var)) : NULL;
  if (nargs > 0) memcpy(args_copy, args, nargs * sizeof(e_var));

  /* temporarily acquire all arguments */
  for (u32 i = 0; i < nargs; i++) e_var_acquire(&args_copy[i]);

  // builtins
  const e_builtin_func* builtin = get_builtin_func_hashed(hash);
  if (builtin != NULL) {
    *ret = builtin->func(args_copy, nargs);
    goto pop_and_ret;
  }

  // extern
  for (u32 i = 0; i < info->nextern_funcs; i++) {
    const char* name = info->extern_funcs[i].name;
    if (e_hash(name, strlen(name)) != hash) continue;
    if (info->extern_funcs[i].func) { *ret = info->extern_funcs[i].func(args_copy, nargs); }
    goto pop_and_ret;
  }

  // user defined
  for (u32 f = 0; f < info->nfuncs; f++) {
    if (info->funcs[f].name_hash != hash) continue;

    e_exec_info fi = {
      .gvars           = info->gvars,
      .code            = info->funcs[f].code,
      .args            = args_copy,
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

    e = e_exec(&fi, ret);
    if (e) goto pop_and_ret;

    goto pop_and_ret;
  }

  print_err("Function %u not defined\n", hash);

  *ret = E_NULLVAR;
  return -1;

pop_and_ret:
  // #ifdef DEBUG_PRINT_STACK
  // printf("Function %u returned: ", hash);
  // eb_println(&return_value, 1);
  // #endif

  /* release all temporary acquires. */
  if (args_copy) {
    for (u32 i = 0; i < nargs; i++) e_var_release(&args_copy[i]);
    free(args_copy);
  }

  return e;
}

e_var*
read_args_vector(e_var* regs, e_var* stack, u32 sp, u32 nelems)
{
  e_var* tmp = e_xalloc(nelems, sizeof(e_var));
  if (!tmp) return NULL;

  u32 i = 0;

  /* copy first 16 arguments from registers */
  for (; i < MIN(nelems, E_REG_ARG_COUNT); i++) {
    memcpy(&tmp[i], &regs[E_REG_ARG0 + i], sizeof(e_var));
    regs[E_REG_ARG0 + i] = E_NULLVAR; /* remove. */
  }

  /* copy rest of the arguments from the stack */
  for (; i < nelems; i++) { memcpy(&tmp[i], &stack[sp - nelems + i], sizeof(e_var)); }

  return tmp;
}

static inline const char*
get_register_name(u32 reg_id, char buff[32])
{
  if (reg_id >= E_REG_ARG0 && reg_id <= E_REG_ARG15) {
    snprintf(buff, 32, "arg%i", reg_id - E_REG_ARG0);
  } else if (reg_id >= E_REG_GENERAL_BEGIN && reg_id <= E_REG_GENERAL_END) {
    snprintf(buff, 32, "r%i", reg_id - E_REG_GENERAL_BEGIN);
  } else if (reg_id == E_REG_SP) {
    snprintf(buff, 32, "rsp");
  } else if (reg_id == E_REG_TMP1) {
    snprintf(buff, 32, "tmp1");
  } else if (reg_id == E_REG_TMP2) {
    snprintf(buff, 32, "tmp2");
  } else if (reg_id == E_REG_IP) {
    snprintf(buff, 32, "rip");
  } else {
    snprintf(buff, 32, "ru%i", reg_id);
  }
  return buff;
}

static inline void
e_print_instruction(e_ins i, const char** names, const u32* names_hashes, u32 nnames)
{
  char buf0[32] = { 0 };
  char buf1[32] = { 0 };
  char buf2[32] = { 0 };
  switch (i.opcode) {
    case EIR_OPCODE_LOADK: {
      printf("loadk dst=%s, id=%u\n", get_register_name(i.loadk.dst, buf0), i.loadk.id);
      break;
    }
    case EIR_OPCODE_MOV: {
      printf("mov dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1));
      break;
    }

    case EIR_OPCODE_MOVI: {
      printf("movi dst=%s, value=%i\n", get_register_name(i.movi.dst, buf0), i.movi.value);
      break;
    }
    case EIR_OPCODE_MOVF: {
      printf("movf dst=%s, value=%f\n", get_register_name(i.movf.dst, buf0), i.movf.value);
      break;
    }

    case EIR_OPCODE_ADD:
      printf(
          "add dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_SUB:
      printf(
          "sub dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MUL:
      printf(
          "mul dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_DIV:
      printf(
          "div dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MOD:
      printf(
          "mod dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EXP:
      printf(
          "exp dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_AND:
      printf(
          "and dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_OR:
      printf("or dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BAND:
      printf(
          "band dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BOR:
      printf(
          "bor dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_XOR:
      printf(
          "xor dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EQL:
      printf(
          "eql dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_NEQ:
      printf(
          "neq dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LT:
      printf("lt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LTE:
      printf(
          "lte dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GT:
      printf("gt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GTE:
      printf(
          "gte dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;

    case EIR_OPCODE_INC: printf("inc dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_DEC: printf("dec dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_BNOT: printf("bnot dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NEG: printf("neg dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NOT: printf("not dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;

    case EIR_OPCODE_RET: {
      printf("ret val=%s\n", get_register_name(i.ret.return_value, buf0));
      break;
    }
    case EIR_OPCODE_NOP: printf("nop\n"); break;
    case EIR_OPCODE_MK_LIST: printf("mk_list dst=%s, nelems=%u\n", get_register_name(i.mk_list.dst, buf0), i.mk_list.nelems); break;
    case EIR_OPCODE_MK_MAP: printf("mk_map dst=%s, npairs=%u\n", get_register_name(i.mk_map.dst, buf0), i.mk_map.npairs); break;
    case EIR_OPCODE_INDEX:
      printf(
          "index dst=%s, base=%s, index=%s\n",
          get_register_name(i.index.dst, buf0),
          get_register_name(i.index.base, buf1),
          get_register_name(i.index.index, buf2));
      break;
    case EIR_OPCODE_CALL:
      printf(
          "call dst=%s, id=%u|name=%s, nargs=%u\n",
          get_register_name(i.call.dst, buf0),
          i.call.function_id,
          lookup(names, names_hashes, nnames, i.call.function_id),
          i.call.nargs);
      break;
    case EIR_OPCODE_INDEX_ASSIGN:
      printf(
          "index_assign value=%s, base=%s, index=%s\n",
          get_register_name(i.index_assign.value, buf0),
          get_register_name(i.index_assign.base, buf1),
          get_register_name(i.index_assign.index, buf2));
      break;

    case EIR_OPCODE_LABEL: printf("label id=%u\n", i.label.id); break;
    case EIR_OPCODE_JMP: printf("jmp target=%u\n", i.jmp.target); break;
    case EIR_OPCODE_JZ: printf("jz target=%u, condition=%s\n", i.jz.target, get_register_name(i.jz.condition, buf0)); break;
    case EIR_OPCODE_JNZ: printf("jnz target=%u, condition=%s\n", i.jnz.target, get_register_name(i.jnz.condition, buf0)); break;
    case EIR_OPCODE_MEMBER_ACCESS:
      printf(
          "member_access dst=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_access.dst, buf0),
          get_register_name(i.member_access.base, buf1),
          i.member_access.member_id);
      break;
    case EIR_OPCODE_MEMBER_ASSIGN:
      printf(
          "member_assign value=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_assign.value, buf0),
          get_register_name(i.member_assign.base, buf1),
          i.member_assign.member_id);
      break;
    case EIR_OPCODE_MK_STRUCT: printf("mk_struct dst=%s, id=%u\n", get_register_name(i.mk_struct.dst, buf0), i.mk_struct.struct_id); break;
    case EIR_OPCODE_GETG: printf("getg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_SETG: printf("setg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_MOVG: printf("movg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_PUSH: printf("push %s\n", get_register_name(i.push.reg, buf0)); break;
    case EIR_OPCODE_POP: printf("pop %s\n", get_register_name(i.pop.reg, buf0)); break;
  }
}

e_ecode
e_exec(const e_exec_info* info, e_var* ret)
{
  /* Initial state check. */
  if ((info->nargs > 0 && info->args == NULL) || (info->code == NULL && info->code_count != 0)
      || (info->extern_funcs == NULL && info->nextern_funcs > 0) || (info->extern_vars == NULL && info->nextern_vars > 0)
      || (info->funcs == NULL && info->nfuncs > 0) || (info->literals == NULL && info->nliterals > 0)) {
    print_err("Corrupted info during execution (broken fork?)\n");
    return E_EBADARG;
  }

  /* error code */
  e_ecode e = E_OK;

  e_var* regs = malloc(sizeof(e_var) * 2048);
  for (u32 i = 0; i < 2048; i++) regs[i] = E_NULLVAR;

  size_t stack_capacity = 256;
  e_var* stack          = e_xalloc(stack_capacity, sizeof(e_var));
  e_var* sp             = &regs[E_REG_SP];
  e_var* ip             = &regs[E_REG_IP];

  *sp = e_var_from_int(0);
  *ip = e_var_from_int(0);

  for (u32 i = 0; i < info->nargs; i++) {
    u32 dst = E_REG_ARG0 + i;
    if (dst < E_REG_ARG_COUNT) {
      e_var_shallow_cpy(&info->args[i], &regs[dst]);
      e_var_acquire(&regs[dst]);
    } else {
      /* Push to stack */
      stack[sp->val.i] = info->args[i];
      e_var_acquire(&stack[sp->val.i]);
      sp->val.i++;
    }
  }

  for (ip->val.i = 0; ip->val.i < info->code_count; ip->val.i++) {
    e_ins ins = info->code[ip->val.i];
    // fprintf(stdout, "next instruction: ");
    // e_print_instruction(ins, info->names, info->names_hashes, info->nnames);

    switch (ins.opcode) {
      case EIR_OPCODE_LOADK: {
        u32 dst = ins.loadk.dst;
        u32 id  = ins.loadk.id;

        e_var v = E_NULLVAR;

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
          e = E_EBADARG;
          goto RET;
        }

        e_var_release(&regs[dst]);
        e_var_deep_cpy(&v, &regs[dst]);
        break;
      }
      case EIR_OPCODE_RET: {
        u32 ret_val = ins.ret.return_value;

        /* store return value */
        e_var_shallow_cpy(&regs[ret_val], ret);
        e_var_acquire(ret);

        /* return from this procedure */
        e = 0;
        goto RET;
      }

      case EIR_OPCODE_MOV: {
        u32 src = ins.mov.src;
        u32 dst = ins.mov.dst;
        if (src == dst) break;

        e_var_release(&regs[dst]);
        e_var_shallow_cpy(&regs[src], &regs[dst]);
        e_var_acquire(&regs[dst]);
        break;
      }

      case EIR_OPCODE_MOVI: {
        int value = ins.movi.value;
        u32 dst   = ins.movi.dst;

        e_var src = e_var_from_int(value);

        e_var_release(&regs[dst]);
        e_var_shallow_cpy(&src, &regs[dst]);
        e_var_acquire(&regs[dst]);
        break;
      }

      case EIR_OPCODE_MOVF: {
        double value = ins.movf.value;
        u32    dst   = ins.movf.dst;

        e_var src = e_var_from_float(value);

        e_var_release(&regs[dst]);
        e_var_shallow_cpy(&src, &regs[dst]);
        e_var_acquire(&regs[dst]);
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

        e_var l = regs[a];
        e_var r = regs[b];

        e_var_release(&regs[dst]);
        e_var result = operate(l, r, ins.opcode);
        e_var_acquire(&result);

        /* removing elements from the stack, free them */
        if (dst == E_REG_SP && (result.val.i < sp->val.i)) {
          while (sp->val.i > result.val.i) { /* while old value is greater than new value */
            e_var_release(&stack[--sp->val.i]);
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

        e_var l = E_NULLVAR;
        e_var r = regs[a];

        e_var_release(&regs[dst]);
        regs[dst] = operate(l, r, ins.opcode);
        e_var_acquire(&regs[dst]);
        break;
      }
      case EIR_OPCODE_INC:
      case EIR_OPCODE_DEC: {
        u32 dst = ins.unop.dst;
        u32 rhs = ins.unop.a;

        e_var* r = &regs[rhs];
        e_var* d = &regs[dst];

        e_var_release(d);
        d->type = r->type;
        if (r->type == E_VARTYPE_INT || r->type == E_VARTYPE_CHAR || r->type == E_VARTYPE_BOOL) {
          d->val.i = e_cast_to_int(r) + (ins.opcode == EIR_OPCODE_INC ? 1 : -1);
        } else if (r->type == E_VARTYPE_FLOAT) {
          d->val.f = e_cast_to_float(r) + (ins.opcode == EIR_OPCODE_INC ? 1 : -1);
        }

        break;
      }

      case EIR_OPCODE_MK_MAP: {
        u32 dst    = ins.mk_map.dst;
        u32 npairs = ins.mk_map.npairs;

        /* read every variable */
        e_var* tmp = read_args_vector(regs, stack, sp->val.i, npairs * 2);

        e_var* dstp = &regs[dst];
        e_var_release(dstp); // overwriting it

        dstp->type    = E_VARTYPE_MAP;
        dstp->val.map = e_refdobj_pool_acquire(&ge_pool);

        /* Constructing it directly in our register. No need for refcounting here. */
        if (e_map_init(tmp, npairs, E_VAR_AS_MAP(dstp)) != 0) {
          e = E_EMALLOC;
          free(tmp);
          goto RET;
        }

        free(tmp);
        break;
      }
        // default: printf("%i\n", ins.opcode); assert(0);

      case EIR_OPCODE_LABEL:
      case EIR_OPCODE_NOP: break;

      case EIR_OPCODE_MK_LIST: {
        u32 dst    = ins.mk_list.dst;
        u32 nelems = ins.mk_list.nelems;

        e_var* tmp = read_args_vector(regs, stack, sp->val.i, nelems);

        e_var* dstp = &regs[dst];
        e_var_release(dstp); // overwriting it

        dstp->type     = E_VARTYPE_LIST;
        dstp->val.list = e_refdobj_pool_acquire(&ge_pool);

        /* Constructing it directly in our register. No need for refcounting here. */
        if (e_list_init(tmp, nelems, E_VAR_AS_LIST(dstp)) != 0) {
          e = E_EMALLOC;
          free(tmp);
          goto RET;
        }

        free(tmp);
        break;
      }
      case EIR_OPCODE_INDEX: {
        u32 dst   = ins.index.dst;
        u32 base  = ins.index.base;
        u32 index = ins.index.index;

        e_var tmp = E_NULLVAR;

        if (e_var_index(&regs[base], &regs[index], &tmp)) {
          tmp = E_NULLVAR;
          break;
        }

        e_var_release(&regs[dst]);
        e_var_shallow_cpy(&tmp, &regs[dst]);
        e_var_acquire(&regs[dst]);
        break;
      }

      case EIR_OPCODE_INDEX_ASSIGN: {
        u32 value = ins.index_assign.value;
        u32 base  = ins.index_assign.base;
        u32 index = ins.index_assign.index;

        if (regs[base].type == E_VARTYPE_STRING) {
          print_err("Can not index assign strings\n");
          e = E_EMALFORM;
          goto RET;
        }

        if (e_var_index_assign(&regs[base], &regs[index], &regs[value])) {
          print_err("e_var_index_assign (%i): error\n", ip->val.i);
          e = E_EMALFORM;
          goto RET;
        }
        break;
      }

      case EIR_OPCODE_CALL: {
        u32 dst         = ins.call.dst;
        u32 function_id = ins.call.function_id;
        u32 nargs       = ins.call.nargs;

        e_var* args = read_args_vector(regs, stack, sp->val.i, nargs);
        if (!args) {
          e = E_EMALLOC;
          goto RET;
        }

        /* overwriting it */
        e_var_release(&regs[dst]);

        e = call(info, function_id, args, nargs, &regs[dst]);
        /* No need to acquire regs[dst], it's deep copied / preacquired. */
        e_var_acquire(&regs[dst]);

        free(args);

        if (e) {
          print_err("e_var_index_assign: error\n");
          goto RET;
        }

        break;
      }

      case EIR_OPCODE_JMP: {
        u32 target = ins.jmp.target;

        /* loop increments ip */
        ip->val.i = (int)(target - 1);
        break;
      }
      case EIR_OPCODE_JZ: {
        u32 target = ins.cj.target;
        u32 cond   = ins.cj.condition;

        /* loop increments ip */
        if (!e_cast_to_bool(&regs[cond])) { ip->val.i = (int)(target - 1); }
        break;
      }
      case EIR_OPCODE_JNZ: {
        u32 target = ins.cj.target;
        u32 cond   = ins.cj.condition;

        /* loop increments ip */
        if (e_cast_to_bool(&regs[cond])) { ip->val.i = (int)(target - 1); }
        break;
      }

      /* MEMBER_ACCESS [dst] [base] [member ID : u32] */
      case EIR_OPCODE_MEMBER_ACCESS: {
        u32 dst       = ins.member_access.dst;
        u32 base      = ins.member_access.base;
        u32 member_id = ins.member_access.member_id;

        if (regs[base].type == E_VARTYPE_VEC2 || regs[base].type == E_VARTYPE_VEC3 || regs[base].type == E_VARTYPE_VEC4) {
          const u32 x = e_hash("x", strlen("x"));
          const u32 y = e_hash("y", strlen("y"));
          const u32 z = e_hash("z", strlen("z"));
          const u32 w = e_hash("w", strlen("w"));

          e_vec4* ptr = &regs[base].val.vec4;

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
            e = E_EMALFORM;
            goto RET;
          }

          e_var_release(&regs[dst]);
          regs[dst] = e_var_from_float(component);
          e_var_acquire(&regs[dst]); // useless
          break;
        }

        if (regs[base].type != E_VARTYPE_STRUCT) {
          print_err("Can not access a member within non struct type: %s\n", e_var_type_to_string(regs[base].type));
          e = E_EMALFORM;
          goto RET;
        }

        e_var push = E_NULLVAR;

        e_struct* st = E_VAR_AS_STRUCT(&regs[base]);
        for (u32 i = 0; i < st->member_count; i++) {
          if (st->member_hashes[i] == member_id) {
            e_var_shallow_cpy(&st->members[i], &push);
            e_var_acquire(&push);
            break;
          }
        }

        e_var_release(&regs[dst]);
        e_var_shallow_cpy(&push, &regs[dst]);
        break;
      }
      /* MEMBER_ASSIGN [value] [base] [member ID : u32] */
      case EIR_OPCODE_MEMBER_ASSIGN: {
        u32 value     = ins.member_assign.value;
        u32 base      = ins.member_assign.base;
        u32 member_id = ins.member_assign.member_id;

        if (regs[base].type == E_VARTYPE_VEC2 || regs[base].type == E_VARTYPE_VEC3 || regs[base].type == E_VARTYPE_VEC4) {
          const u32 x = e_hash("x", strlen("x"));
          const u32 y = e_hash("y", strlen("y"));
          const u32 z = e_hash("z", strlen("z"));
          const u32 w = e_hash("w", strlen("w"));

          e_vec4* ptr = &regs[base].val.vec4;
          double  val = e_cast_to_float(&regs[value]);

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
            e = E_EMALFORM;
            goto RET;
          }
          break;
        }

        e_struct* st = E_VAR_AS_STRUCT(&regs[base]);
        for (u32 i = 0; i < st->member_count; i++) {
          if (st->member_hashes[i] == member_id) {
            e_var* member = &st->members[i];
            e_var_release(member);
            e_var_shallow_cpy(&regs[value], member);
            e_var_acquire(member);
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

        const ecc_struct_information* st = NULL;
        for (u32 i = 0; i < info->nstructs; i++) {
          if (info->structs[i].name_hash == id) {
            st = &info->structs[i];
            break;
          }
        }

        if (!st) {
          print_err("Structure %u not found in table\n", id);
          e = E_ENONEXISTENT;
          goto RET;
        }

        e_var* dstp = &regs[dst];
        e_var_release(dstp);

        dstp->type      = E_VARTYPE_STRUCT;
        dstp->val.struc = e_refdobj_pool_acquire(&ge_pool);

        if (e_struct_init(st->name, st->fields_count, (const char**)st->field_names, E_VAR_AS_STRUCT(dstp)) != 0) {
          e = E_EMALLOC;
          goto RET;
        }

        e_var* args = read_args_vector(regs, stack, sp->val.i, st->fields_count);
        for (u32 i = 0; i < st->fields_count; i++) {
          e_var* member = &E_VAR_AS_STRUCT(dstp)->members[i];
          e_var_release(member);
          e_var_shallow_cpy(&args[i], member);
          e_var_acquire(member);
        }
        free(args);
        break;
      }
      case EIR_OPCODE_GETG: {
        u32 dst = ins.getg.dst;
        u32 src = ins.getg.src;
        e_var_release(&regs[dst]);
        e_var_shallow_cpy(&info->gvars[src], &regs[dst]);
        break;
      }

      case EIR_OPCODE_SETG: {
        u32 dst = ins.setg.dst;
        u32 src = ins.setg.src;
        e_var_release(&info->gvars[dst]);
        e_var_shallow_cpy(&regs[src], &info->gvars[dst]);
        break;
      }

      case EIR_OPCODE_MOVG: {
        u32 dst = ins.movg.dst;
        u32 src = ins.movg.src;
        e_var_release(&info->gvars[dst]);
        e_var_shallow_cpy(&info->gvars[src], &info->gvars[dst]);
        break;
      }

      case EIR_OPCODE_PUSH: {
        u32 reg = ins.push.reg;

        if (sp->val.i >= stack_capacity) {
          size_t new_capacity = stack_capacity * 2;
          e_var* new_stack    = realloc(stack, sizeof(e_var) * new_capacity);
          if (!new_stack) {
            e = E_EMALLOC;
            goto RET;
          }

          stack          = new_stack;
          stack_capacity = new_capacity;
        }

        e_var_release(&stack[sp->val.i]); /* overwriting it. */
        stack[sp->val.i] = regs[reg];
        e_var_acquire(&stack[sp->val.i]); /* temporary hold */

        sp->val.i++;
        break;
      }
      case EIR_OPCODE_POP: {
        u32 reg = ins.push.reg;
        e_var_release(&regs[reg]); /* overwriting it. */

        sp->val.i--;
        regs[reg] = stack[sp->val.i];
        e_var_release(&stack[sp->val.i]); /* release temporary hold */
        break;
      }
    }
  }

RET:
  for (u32 i = 0; i < 2048; i++) { e_var_release(&regs[i]); }
  for (i32 i = 0; i < sp->val.i; i++) { e_var_release(&stack[i]); }
  free(regs);
  free(stack);
  return e;
}

e_ecode
e_script_call(e_script* s, const char* func_name, e_var* args, u32 nargs, e_var* ret)
{
  u32 hash = e_hash(func_name, strlen(func_name));

  for (u32 i = 0; i < s->compiled.functions_count; i++) {
    if (hash == s->compiled.functions[i].name_hash) {
      e_exec_info info = {
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
        return E_EBADARG;
      }

      return e_exec(&info, ret);
    }
  }

  return E_EUNDEFINED;
}
