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

#ifndef KIT_SCRIPT_H
#define KIT_SCRIPT_H

#include "kit.bfunc.h"
#include "kit.cc.h"
#include "kit.perr.h"
#include "kit.stdafx.h"
#include "kit.var.h"

#include <stddef.h>

/**
 * Script entity.
 */
typedef struct kit_script {
  kit_vm*                 vm;
  kit_compilation_result  compiled;
  const kit_builtin_func* extern_funcs;
  const kit_builtin_var*  extern_vars;
  u32                     nxtern_funcs;
  u32                     nextern_vars;
} kit_script;

/**
 * (You need to have executed the root stream first!)
 * Ignoring the root stream, find the function within
 * the bytecode, and try to execute it (if it exists.)
 */
kit_ecode kit_script_call(kit_script* s, const char* func_name, kit_var* args, u32 nargs, kit_var* ret);

static inline int
kit_script_init(
    kit_vm*                       vm,
    const kit_compilation_result* r,
    const kit_builtin_func*       extern_funcs,
    u32                           nextern_funcs,
    const kit_builtin_var*        extern_vars,
    u32                           nextern_vars,
    kit_script*                   s)
{
  s->extern_funcs = extern_funcs;
  s->nxtern_funcs = nextern_funcs;
  s->extern_vars  = extern_vars;
  s->nextern_vars = nextern_vars;
  s->compiled     = *r;
  s->vm           = vm;
  return 0;
}

static inline void
kit_script_free(kit_script* s)
{
}

/**
 * Find the variable with the hash specified
 * and return a RW pointer to it.
 * You can safely modify the value only when
 * the program is not running (after/before).
 */
// static inline kit_var*
// kit_script_get_variable(u32 name_hash, const kit_script* s)
// { return kit_stack_find(&s->stack, name_hash); }

static inline bool
kit_script_has_function(kit_script* s, const char* func_name)
{
  u32 hash = kit_hash(func_name, strlen(func_name));
  for (u32 i = 0; i < s->compiled.functions_count; i++) {
    if (hash == s->compiled.functions[i].name_hash) return true;
  }
  return false;
}

#endif // KIT_SCRIPT_H