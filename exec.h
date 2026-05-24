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

#ifndef E_VM_H
#define E_VM_H

#include "bfunc.h"
#include "bvar.h"
#include "cc.h"
#include "stack.h"
#include "stdafx.h"
#include "var.h"

#include <stddef.h>

typedef struct e_exec_info {
  const u8*                     code;
  const e_var*                  args;      // nullptr if nargs == 0, shallow copied.
  const u32*                    arg_slots; // The IDs which the arguments take
  const e_var*                  literals;  // must outlive the exec function.
  const u32*                    literals_hashes;
  const ecc_function*           funcs;
  const e_builtin_func*         extern_funcs;
  const e_builtin_var*          extern_vars;
  const ecc_struct_information* structs;

  const char** names;
  const u32*   names_hashes;

  /**
   * Must not be NULL.
   * A temporary scratchpad for the VM.
   */
  e_stack* stack;

  u32 code_size; // must not be equal to 0
  u32 nargs;     // can be equal to 0 (no arguments passed :<)
  u32 nliterals;
  u32 nfuncs;
  u32 nextern_funcs;
  u32 nextern_vars;
  u32 nnames;
  u32 nstructs;
} e_exec_info;

/**
 * Execute a bytecode stream.
 * You can hook external (C) functions to the program
 * using the extern_funcs member of info.
 *
 * Arguments can also be passed using the args member
 * of info.
 */
e_ecode e_exec(const e_exec_info* info, e_var* ret);

#endif // E_VM_H