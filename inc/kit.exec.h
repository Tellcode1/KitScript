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

#ifndef KIT_VM_H
#define KIT_VM_H

#include "kit.bfunc.h"
#include "kit.bvar.h"
#include "kit.cc.h"
#include "kit.ir.h"
#include "kit.perr.h"
#include "kit.stdafx.h"
#include "kit.var.h"

#include <stddef.h>

typedef struct kit_exec_info {
  kit_var*                       gvars;
  const kit_ins*                 code;
  const kit_var*                 args;     // NULL if nargs == 0, shallow copied.
  const kit_var*                 literals; // must outlive the exec function.
  const u32*                     literals_hashes;
  const kitc_function*           funcs;
  const kit_builtin_func*        extern_funcs;
  const kit_builtin_var*         extern_vars;
  const kitc_struct_information* structs;

  const char** names;
  const u32*   names_hashes;

  u32 code_count; // can be equal to 0.
  u32 nargs;      // can be equal to 0 (no arguments passed :<)
  u32 nliterals;
  u32 nfuncs;
  u32 nextern_funcs;
  u32 nextern_vars;
  u32 nnames;
  u32 nstructs;
} kit_exec_info;

/**
 * Execute a bytecode stream.
 * You can hook external (C) functions to the program
 * using the extern_funcs member of info.
 *
 * Arguments can also be passed using the args member
 * of info.
 */
kit_ecode kit_exec(const kit_exec_info* info, kit_var* ret);

#endif // KIT_VM_H