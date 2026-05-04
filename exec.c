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

#include "exec.h"

#include "bc.h"
#include "bfunc.h"
#include "cast.h"
#include "fn.h"
#include "list.h"
#include "map.h"
#include "mathstrucs.h"
#include "operate.h"
#include "perr.h"
#include "pool.h"
#include "rwhelp.h"
#include "script.h"
#include "stack.h"
#include "stdafx.h"
#include "string.h"
#include "var.h"

#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PASTE(x, y) x##y

// clang-format off
#define TRY(expr) do { int PASTE(__e__, __LINE__) = 0; if ((PASTE(__e__, __LINE__) = (expr))) { return PASTE(__e__, __LINE__); } } while (0)
#define TRY_V(expr) do {   int PASTE(__e__, __LINE__) = (expr);   if (PASTE(__e__, __LINE__)) { return (e_var){ .type = E_VARTYPE_ERROR, .val.errcode = PASTE(__e__, __LINE__) }; } } while (0)
// clang-format on

// static inline e_var*
// stack_find_rev(const struct stack* s, u16 id)
// {
//   for (int i = 0; i < s->size; i++)
//   {
//     // if (id == s->stack[i].) {TRY_V( e_stack_push(info->stack, info->stack.stack[variables[i].offset]); )}
//   }
// }

static inline const e_builtin_func*
get_builtin_func_hashed(u32 hash)
{
  for (u32 i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (e_hash(eb_funcs[i].name, strlen(eb_funcs[i].name)) == hash) return &eb_funcs[i];
  }
  return nullptr;
}

static e_var
call(const e_exec_info* info, u32 hash, u32 nargs)
{
  e_var return_value = { .type = E_VARTYPE_NULL };

  /**
   * Copy argumenst into a temporary array and remove them from the stack.
   */
  e_var* args_copy = nargs > 0 ? calloc(nargs, sizeof(e_var)) : NULL;
  if (nargs > 0) memcpy(args_copy, &info->stack->stack[info->stack->size - nargs], nargs * sizeof(e_var));

  for (u32 i = 0; i < nargs; i++) e_var_acquire(&args_copy[i]);
  for (u32 i = 0; i < nargs; i++) e_stack_pop(info->stack);

  /**
   * The depth that we'll need to
   * restore to when the function returns.
   */
  u32 depth_restore = info->stack->depth;

  // builtins
  const e_builtin_func* builtin = get_builtin_func_hashed(hash);
  if (builtin != nullptr) {
    return_value = builtin->func(args_copy, nargs);
    goto pop_and_ret;
  }

  // extern
  for (u32 i = 0; i < info->nextern_funcs; i++) {
    const char* name = info->extern_funcs[i].name;
    if (e_hash(name, strlen(name)) != hash) continue;
    if (info->extern_funcs[i].func) { return_value = info->extern_funcs[i].func(args_copy, nargs); }
    goto pop_and_ret;
  }

  // user defined
  for (u32 f = 0; f < info->nfuncs; f++) {
    if (info->funcs[f].name_hash != hash) continue;

    e_stack stack = { 0 };
    if (e_stack_init(32, 4, 8, &stack)) goto pop_and_ret;

    e_exec_info fi = {
      .code            = info->funcs[f].code,
      .args            = args_copy,
      .arg_slots       = info->funcs[f].arg_slots,
      .literals        = info->literals,
      .literals_hashes = info->literals_hashes,
      .funcs           = info->funcs,
      .extern_funcs    = info->extern_funcs,
      .code_size       = info->funcs[f].code_size,
      .nargs           = info->funcs[f].nargs,
      .nliterals       = info->nliterals,
      .nfuncs          = info->nfuncs,
      .nextern_funcs   = info->nextern_funcs,
      .extern_vars     = info->extern_vars,
      .nextern_vars    = info->nextern_vars,
      .stack           = &stack,
    };
    return_value = e_exec(&fi);

    e_stack_free(&stack);

    goto pop_and_ret;
  }

  fprintf(stderr, "Function %u not defined\n", hash);

  e_var null_var = { .type = E_VARTYPE_NULL };
  return null_var;

pop_and_ret:
  // #ifdef DEBUG_PRINT_STACK
  // printf("Function %u returned: ", hash);
  // eb_println(&return_value, 1);
  // #endif

  if (args_copy) {
    for (u32 i = 0; i < nargs; i++) e_var_release(&args_copy[i]);
    free(args_copy);
  }

  /**
   * Restore to the depth that we
   * were in
   */
  while (info->stack->depth > depth_restore) e_stack_pop_frame(info->stack);

  return return_value;
}

static inline e_var*
get_variable_from_id(e_stack* stack, u32 hash)
{
  for (u32 i = 0; i < stack->nvariables; i++) {
    u32 idx = stack->nvariables - i - 1;
    if (hash == stack->variables[idx].id) return &stack->stack[stack->variables[idx].offset_index];
  }
  return NULL;
}

static inline int
vector_elements(const e_var* vec)
{
  switch (vec->type) {
    case E_VARTYPE_VEC2: return 2;
    case E_VARTYPE_VEC3: return 3;
    case E_VARTYPE_VEC4: return 4;
    default: break;
  }
  return 0;
}

static inline e_var
vector_index(const e_var* vec, int index)
{
  e_var push = { .type = E_VARTYPE_NULL };

  e_vec4 vector;
  evector_zero_extend(vec, vector);
  if (index < vector_elements(vec)) { push = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = vector[index] }; }

  return push;
}

e_var
e_exec(const e_exec_info* info)
{
  /* Initial state check. */
  if (!info->stack || (info->nargs > 0 && (info->args == nullptr || info->arg_slots == nullptr)) || (info->code == nullptr && info->code_size != 0)
      || (info->extern_funcs == nullptr && info->nextern_funcs > 0) || (info->extern_vars == nullptr && info->nextern_vars > 0)
      || (info->funcs == nullptr && info->nfuncs > 0) || (info->literals == nullptr && info->nliterals > 0)) {
    fputs("Corrupted info during execution (broken fork?)\n", stderr);
    return E_NULLVAR;
  }

  for (u32 i = 0; i < info->nargs; i++) {
    e_var* slot = e_stack_push_variable(info->arg_slots[i], info->stack);
    if (!slot) return (e_var){ .type = E_VARTYPE_ERROR, .val.errcode = E_EUNKNOWN };

    // Nothing should be on the stack but still...
    e_var_release(slot);

    e_var_shallow_cpy(&info->args[i], slot);
    e_var_acquire(slot);
  }

  e_var retval = { .type = E_VARTYPE_NULL };

  const u8* ip  = info->code;
  const u8* end = info->code + info->code_size;

  e_ins ins = { 0 };
  while (ip < end) {
    ins = e_read_ins(&ip);

    // fputs("STACK[ ", stdout);
    // for (u32 i = 0; i < info->stack->size; i++) {
    //   eb_print(&info->stack->stack[i], 1);
    //   fputs(", ", stdout);
    // }
    // fputs(" ]\n\n", stdout);

    switch ((e_opcode_bck)ins.opcode) {
      /* NOOPs */
      case E_OPCODE_LABEL:
      case E_OPCODE_COUNT:
      case E_OPCODE_NOOP: break;

      case E_OPCODE_MOV: {
        const u32 dst = ins.v.mov.dst;
        const u32 src = ins.v.mov.src;
        if (dst >= info->stack->capacity || src >= info->stack->capacity) {
          fprintf(stderr, "OOB mov [%u, %u], capacity is [0, %u]\n", dst, src, info->stack->capacity);
          return E_NULLVAR;
        }

        e_var* stack = info->stack->stack;

        e_var_release(&stack[dst]);
        stack[dst] = stack[src];
        e_var_acquire(&stack[dst]);

        break;
      }

      case E_OPCODE_POP: e_stack_pop(info->stack); break;
      case E_OPCODE_DUP: {
        e_var v;
        // e_var_acquire(e_stack_top(info->stack)); // Don't acquire! Stack_push already does that
        TRY_V(e_var_shallow_cpy(e_stack_top(info->stack), &v));
        TRY_V(e_stack_push(info->stack, &v));
        break;
      }

      case E_OPCODE_CALL: {
        const u32 func_nargs = ins.v.call.nargs;
        const u32 hash       = ins.v.call.hash;

        if (info->stack->size < func_nargs) {
          fputs("*** stack corruption ***\n", stderr);
          return E_NULLVAR;
        }

        e_var r = call(info, hash, func_nargs);
        if (r.type == E_VARTYPE_ERROR) { return r; }

        TRY_V(e_stack_push(info->stack, &r));
        e_var_release(&r);
        break;
      }

      case E_OPCODE_LITERAL: {
        const u32 id = ins.v.literal;

        e_var v = E_NULLVAR;
        for (u32 i = 0; i < info->nliterals; i++) {
          if (id == info->literals_hashes[i]) {
            int e = e_var_deep_cpy(&info->literals[i], &v); // Deep copy the literal.
            if (e) return (e_var){ .type = E_VARTYPE_NULL };
            break;
          }
        }

        TRY_V(e_stack_push(info->stack, &v));
        e_var_release(&v); // Hand over v to the stack. Noop for E_NULLVAR.
        break;
      }

      case E_OPCODE_MK_LIST: {
        const u32 nelems = ins.v.mk_list;

        e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);
        if (!obj) return E_NULLVAR;

        // Convert the object
        e_list* list = E_OBJ_AS_LIST(obj);

        e_var new_list = {
          .type = E_VARTYPE_LIST,
          // e_list_init initializes refc too
          .val.list = obj,
        };

        e_var* stack      = info->stack->stack;
        size_t stack_size = info->stack->size;

        if (info->stack->size < nelems) {
          fputs("*** stack corruption ***\n", stderr);
          return E_NULLVAR;
        }

        e_var* elems = &stack[stack_size - nelems];
        TRY_V(e_list_init(elems, nelems, list)); // acquires the elements.

        // Release variables from the stack.
        for (u32 i = 0; i < nelems; i++) { e_stack_pop(info->stack); }

        TRY_V(e_stack_push(info->stack, &new_list));
        e_var_release(&new_list);

        break;
      }

      case E_OPCODE_MK_MAP: {
        const u32 npairs = ins.v.mk_map;

        e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);
        if (!obj) return E_NULLVAR;

        // Convert the object
        e_map* map = E_OBJ_AS_MAP(obj);

        e_var new_map = {
          .type    = E_VARTYPE_MAP,
          .val.map = obj,
        };

        e_var* stack      = info->stack->stack;
        u32    stack_size = info->stack->size;

        if (info->stack->size < (npairs * 2)) {
          fputs("*** stack corruption ***\n", stderr);
          return E_NULLVAR;
        }

        e_var* elems = &stack[stack_size - (npairs * 2)];
        TRY_V(e_map_init(elems, npairs, map)); // acquires the elements.

        // Release variables from the stack.
        for (u32 i = 0; i < npairs * 2; i++) { e_stack_pop(info->stack); }

        TRY_V(e_stack_push(info->stack, &new_map));
        e_var_release(&new_map);

        break;
      }

      case E_OPCODE_MK_STRUCT: {
        const u32 member_count = ins.v.mk_struct.nmembers;

        u32* fields = calloc(member_count, sizeof(u32));
        for (u32 i = 0; i < member_count; i++) { fields[i] = ins.v.mk_struct.members[i]; }

        e_var st = {
          .type      = E_VARTYPE_STRUCT,
          .val.struc = e_refdobj_pool_acquire(&ge_pool),
        };
        if (!st.val.struc) return E_NULLVAR;

        E_VAR_AS_STRUCT(&st)->member_hashes = fields;
        E_VAR_AS_STRUCT(&st)->members       = (e_var*)calloc(member_count, sizeof(e_var));
        E_VAR_AS_STRUCT(&st)->member_count  = member_count;

        for (u32 i = 0; i < member_count; i++) { E_VAR_AS_STRUCT(&st)->members[i] = (e_var){ .type = E_VARTYPE_NULL }; }

        TRY_V(e_stack_push(info->stack, &st));
        e_var_release(&st); // Stack owns it now

        break;
      }

      case E_OPCODE_MEMBER_ACCESS: {
        if (!info->stack) return E_NULLVAR;

        const u32 member = ins.v.member;

        e_var push = { .type = E_VARTYPE_NULL };

        e_vartype type = e_stack_top(info->stack)->type;
        if (type == E_VARTYPE_VEC2 || type == E_VARTYPE_VEC3 || type == E_VARTYPE_VEC4) {
          e_vec4 v;
          evector_zero_extend(e_stack_top(info->stack), v);

          double d = DBL_MAX;
          if (member == e_hash("x", 1)) { d = v[0]; }
          if (member == e_hash("y", 1)) { d = v[1]; }
          if (member == e_hash("z", 1)) { d = v[2]; }
          if (member == e_hash("w", 1)) { d = v[3]; }

          // Vectors are not ref counted. This is safe.
          push = (e_var){
            .type  = d == DBL_MAX ? E_VARTYPE_NULL : E_VARTYPE_FLOAT, // if member not in vec2, return a null var
            .val.f = d,
          };
        } else if (type == E_VARTYPE_STRUCT) {
          e_struct* st = E_VAR_AS_STRUCT(e_stack_top(info->stack));
          for (u32 i = 0; i < st->member_count; i++) {
            if (st->member_hashes[i] == member) {
              TRY_V(e_var_shallow_cpy(&st->members[i], &push));
              e_var_acquire(&push); // acquire tmp
              break;
            }
          }
        } else {
          push = E_NULLVAR;
        }

        // pop off struct
        e_stack_pop(info->stack);

        // Push member
        TRY_V(e_stack_push(info->stack, &push));
        e_var_release(&push); // release tmp hold
        break;
      }

      case E_OPCODE_MEMBER_ASSIGN: {
        const u32 member = ins.v.member;

        if (info->stack->size < 2) {
          fputs("*** stack corruption ***\n", stderr);
          return E_NULLVAR;
        }

        e_var* value = e_stack_top(info->stack);
        e_var* struc = e_stack_top(info->stack) - 1;

        e_vartype type = struc->type;

        if (type == E_VARTYPE_VEC2 || type == E_VARTYPE_VEC3 || type == E_VARTYPE_VEC4) {
          const u32 x = e_hash("x", 1);
          const u32 y = e_hash("y", 1);
          const u32 z = e_hash("z", 1);
          const u32 w = e_hash("w", 1);

          if (member == x) {
            struc->val.vec4[0] = value->val.f;
          } else if (member == y) {
            struc->val.vec4[1] = value->val.f;
          } else if (type != E_VARTYPE_VEC2 && member == z) {
            struc->val.vec4[2] = value->val.f;
          } else if (type == E_VARTYPE_VEC4 && member == w) {
            struc->val.vec4[3] = value->val.f;
          }
        } else if (struc->type == E_VARTYPE_STRUCT) {
          e_struct* st = E_VAR_AS_STRUCT(struc);
          for (u32 i = 0; i < st->member_count; i++) {
            if (st->member_hashes[i] == member) {
              e_var_release(&st->members[i]);
              TRY_V(e_var_shallow_cpy(value, &st->members[i]));
              e_var_acquire(&st->members[i]);
              break;
            }
          }
        }

        // e_var value_copy;
        // e_var_shallow_cpy(value, &value_copy);
        // e_var_acquire(&value_copy);

        /* remove value, we have a copy of it.  */
        e_stack_pop(info->stack);

        // /* release old struct object */
        // e_var_release(struc);

        // /* replace struct slot with value copy */
        // *struc = value_copy;

        break;
      }

      case E_OPCODE_DIV:
        if (e_cast_to_int(&info->stack->stack[info->stack->size - 1]) == 0) {
          fputs("*** Divide by zero ***\n", stderr);
          return (e_var){ .type = E_VARTYPE_NULL };
        }
      case E_OPCODE_ADD:
      case E_OPCODE_SUB:
      case E_OPCODE_MUL:
      case E_OPCODE_MOD:
      case E_OPCODE_EXP:
      case E_OPCODE_AND:
      case E_OPCODE_OR:
      case E_OPCODE_BAND:
      case E_OPCODE_BOR:
      case E_OPCODE_XOR:
      case E_OPCODE_EQL:
      case E_OPCODE_NEQ:
      case E_OPCODE_LT:
      case E_OPCODE_LTE:
      case E_OPCODE_GT:
      case E_OPCODE_GTE: {
        if (info->stack->size < 2) {
          fputs("*** stack corruption ***\n", stderr);
          return E_NULLVAR;
        }
        // Since we compile left first, right next
        // right will be at the top of the stack and left will be below it
        e_var r = operate(info->stack->stack[info->stack->size - 2], info->stack->stack[info->stack->size - 1], ins.opcode);

        e_stack_pop(info->stack); // remove L & R
        e_stack_pop(info->stack);

        /* No need to acquire r. */
        TRY_V(e_stack_push(info->stack, &r));
        break;
      }

      case E_OPCODE_INC:
      case E_OPCODE_DEC: {
        e_var* top = e_stack_top(info->stack);

        if (top->type == E_VARTYPE_INT) {
          top->val.i += (ins.opcode == E_OPCODE_INC) ? 1 : -1;
        } else {
          top->val.f += (ins.opcode == E_OPCODE_INC) ? 1.F : -1.F;
        }

        break;
      }

      case E_OPCODE_NEG:
      case E_OPCODE_NOT:
      case E_OPCODE_BNOT: {
        // Provide an empty variable for the LHS
        e_var r = operate((e_var){ 0 }, *e_stack_top(info->stack), ins.opcode);

        e_stack_pop(info->stack); // remove RHS
        TRY_V(e_stack_push(info->stack, &r));

        break;
      }

      case E_OPCODE_JZ: {
        const u32 target = ins.v.jmp;
        if (target >= info->code_size) {
          fputs("JZ OOB\n", stderr);
          return E_NULLVAR;
        }

        const e_var* cnd  = e_stack_top(info->stack);
        bool         eval = evar_to_bool(*cnd);
        if (!eval) ip = info->code + target;

        e_stack_pop(info->stack); // remove condition
        break;
      }

      case E_OPCODE_JNZ: {
        const u32 target = ins.v.jmp;
        if (target >= info->code_size) {
          fputs("JNZ OOB\n", stderr);
          return E_NULLVAR;
        }

        const e_var* cnd  = e_stack_top(info->stack);
        bool         eval = evar_to_bool(*cnd);
        if (eval) ip = info->code + target;

        e_stack_pop(info->stack); // remove condition
        break;
      }

      case E_OPCODE_JE: {
        const u32 target = ins.v.jmp;
        e_var*    top    = e_stack_top(info->stack);
        if (e_var_equal(top, top - 1)) ip = info->code + target;

        e_stack_pop(info->stack); // remove conditions
        e_stack_pop(info->stack);
        break;
      }

      case E_OPCODE_JNE: {
        const u32 target = ins.v.jmp;
        e_var*    top    = e_stack_top(info->stack);
        if (!e_var_equal(top, top - 1)) ip = info->code + target;

        e_stack_pop(info->stack); // remove conditions
        e_stack_pop(info->stack);
        break;
      }

      case E_OPCODE_JMP: {
        const u32 target = ins.v.jmp;
        if (target >= info->code_size) {
          fputs("JMP OOB\n", stderr);
          return E_NULLVAR;
        }
        ip = info->code + target;
      } break;

      case E_OPCODE_INIT: {
        const u32 hash = ins.v.init;
        e_stack_push_variable(hash, info->stack);
        break;
      }

      case E_OPCODE_LOAD: {
        const u32 id = ins.v.load;

        bool was_external = false;
        for (u32 i = 0; i < info->nextern_vars; i++) {
          const char* name = info->extern_vars[i].name;
          if (e_hash(name, strlen(name)) == id) {
            e_var v = {
              .type = info->extern_vars[i].type,
              .val  = info->extern_vars[i].value,
            };

            TRY_V(e_stack_push(info->stack, &v));

            was_external = true;
            break;
          }
        }

        if (was_external) break;

        e_var* slot = get_variable_from_id(info->stack, id);
        if (!slot) {
          fprintf(stderr, "*** undeclared variable (id=%u) LOAD'd ***\n", id);
          return E_NULLVAR;
        }

        e_var v;
        TRY_V(e_var_shallow_cpy(slot, &v));

        TRY_V(e_stack_push(info->stack, &v));
        break;
      }

      case E_OPCODE_INDEX: {
        e_var push = { .type = E_VARTYPE_NULL };

        e_var* stack      = info->stack->stack;
        size_t stack_size = info->stack->size;
        if (stack_size < 2) {
          fputs("*** stack corruption ***\n", stderr);
          return E_NULLVAR;
        }

        e_var* left = &stack[stack_size - 2];

        if (left->type == E_VARTYPE_LIST) {
          e_list* list = left->type == E_VARTYPE_LIST ? E_VAR_AS_LIST(left) : NULL;
          if (!list) { return (e_var){ .type = E_VARTYPE_NULL }; }

          int idx = e_cast_to_int(&stack[stack_size - 1]);

          if (list && idx >= 0 && (u64)idx < list->size) {
            push = list->vars[idx];
            e_var_acquire(&push);
          }
        } else if (left->type == E_VARTYPE_MAP) {
          e_map* map = E_VAR_AS_MAP(left);
          e_var  key = stack[stack_size - 1];

          if (!map) { return (e_var){ .type = E_VARTYPE_NULL }; }

          e_var* find = e_map_find(map, &key);
          if (find) {
            push = *find;
            e_var_acquire(&push);
          } // else push is null var
        } else if (is_vector(left)) {
          int idx = e_cast_to_int(&stack[stack_size - 1]);
          push    = vector_index(left, idx);
        }

        e_stack_pop(info->stack); // pop index
        e_stack_pop(info->stack); // pop base

        TRY_V(e_stack_push(info->stack, &push));

        e_var_release(&push);
        break;
      }

      case E_OPCODE_INDEX_ASSIGN: {
        e_var* stack      = info->stack->stack;
        u32    stack_size = info->stack->size;
        if (stack_size < 3) {
          fputs("*** stack corruption ***\n", stderr);
          return E_NULLVAR;
        }

        e_var* base  = &stack[stack_size - 3];
        e_var* index = &stack[stack_size - 2];
        e_var* value = &stack[stack_size - 1];

        if (base->type == E_VARTYPE_LIST) {
          e_list* list = E_VAR_AS_LIST(base);
          int     idx  = e_cast_to_int(&*index);

          e_var* slot = &list->vars[idx];
          e_var_release(slot);
          TRY_V(e_var_shallow_cpy(value, slot));
          e_var_acquire(slot);
        } else if (base->type == E_VARTYPE_MAP) {
          e_map* map  = E_VAR_AS_MAP(base);
          e_var* slot = e_map_find_or_insert(map, index);

          if (!map || !slot) { return (e_var){ .type = E_VARTYPE_NULL }; }

          e_var_release(slot);
          TRY_V(e_var_shallow_cpy(value, slot));
          e_var_acquire(slot);
        } else if (base->type == E_VARTYPE_VEC2 || base->type == E_VARTYPE_VEC3 || base->type == E_VARTYPE_VEC4) {
          int    idx = e_cast_to_int(&*index);
          double f   = e_cast_to_float(value);

          if (idx == 0) {
            base->val.vec4[0] = f;
          } else if (idx == 1) {
            base->val.vec4[1] = f;
          } else if (idx == 2) {
            base->val.vec4[2] = f;
          } else if (idx == 3) {
            base->val.vec4[3] = f;
          }
        }

        e_stack_pop(info->stack);
        // fputs("top=", stdout);
        // eb_println(e_stack_top(info->stack), 1);
        e_stack_pop(info->stack);
        /* base remains on stack */

        break;
      }

      case E_OPCODE_ASSIGN: {
        const u32 id = ins.v.assign;

        e_var* slot = get_variable_from_id(info->stack, id);

        if (!slot) { return (e_var){ .type = E_VARTYPE_NULL }; }

        e_var_release(slot); // free old value in slot

        /**
         * Copy it. We need the variable on the stack to support.
         * chained assignments
         * let x = 16;
         * let y = 32;
         * let z = 64;
         * x = y = z = 68;
         *             ^ value needs to be on the stack!
         */
        TRY_V(e_var_shallow_cpy(e_stack_top(info->stack), slot)); // CPY from stack to slot

        e_var_acquire(slot);

        break;
      }

      case E_OPCODE_RETURN: {
        const bool has_return_value = ins.v.has_return_value;
        if (has_return_value) {
          TRY_V(e_var_shallow_cpy(e_stack_top(info->stack), &retval));
          e_var_acquire(&retval);
        }

        goto _RETURN;
      }

      case E_OPCODE_PUSH_FRAME: TRY_V(e_stack_push_frame(info->stack)); break;
      case E_OPCODE_POP_FRAME: e_stack_pop_frame(info->stack); break;

      // Non fatal return
      case E_OPCODE_HALT: goto _RETURN;

      default: fprintf(stderr, "[%zu] Illegal instruction\n", (size_t)(ip - end)); break;
    }
  }

_RETURN: {
  /**
   * Everything is externally managed. Don't need to free anything.
   */
  return retval;
}
}

e_var
e_script_call(e_script* s, const char* func_name, e_var* args, u32 nargs)
{
  u32 hash = e_hash(func_name, strlen(func_name));

  for (u32 i = 0; i < s->compiled.nfunctions; i++) {
    if (hash == s->compiled.functions[i].name_hash) {
      e_exec_info info = {
        .code            = s->compiled.functions[i].code,
        .args            = args,
        .arg_slots       = s->compiled.functions[i].arg_slots,
        .literals        = s->compiled.literals,
        .literals_hashes = s->compiled.literals_hashes,
        .funcs           = s->compiled.functions,
        .extern_funcs    = s->extern_funcs,
        .stack           = &s->stack,
        .code_size       = s->compiled.functions[i].code_size,
        .nargs           = nargs,
        .nliterals       = s->compiled.nliterals,
        .nfuncs          = s->compiled.nfunctions,
        .nextern_funcs   = s->nxtern_funcs,
        .extern_vars     = s->extern_vars,
        .nextern_vars    = s->nextern_vars,
      };

      if (s->compiled.functions[i].nargs != nargs) {
        fprintf(stderr, "Function expects %u arguments (%u were provided)\n", s->compiled.functions[i].nargs, nargs);
        return (e_var){ .type = E_VARTYPE_NULL };
      }

      if ((e_stack_push_frame(info.stack))) { return (e_var){ .type = E_VARTYPE_NULL }; }
      e_var r = e_exec(&info);
      e_stack_pop_frame(info.stack);

      return r;
    }
  }

  return (e_var){ .type = E_VARTYPE_NULL };
}
