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

#ifndef KIT_RUNTIME_BUILTIN_FUNCTIONS_H
#define KIT_RUNTIME_BUILTIN_FUNCTIONS_H

#include "kit.perr.h"
#include "kit.var.h"
#include "kit.vm.h"

/**
 * Compile and execute the source code (code must be given as a string).
 * This involves the usual compilation and then execution.
 * Execution does not inherit the stack frame of the caller.
 * However, arguments can be passed to the entry point.
 */
kit_ecode kit_builtins_rt_compile_and_exec(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

/**
 * Perform lexical analysis on the text using the builtin E lexicalizer.
 * Returned is a list of tokens (See kit::token structure in bstructs.h)
 */
kit_ecode kit_builtins_rt_tokenize(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

kit_ecode kit_builtins_rt_ast_init(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rt_ast_free(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

/**
 * Parse a token stream into an AST.
 */
kit_ecode kit_builtins_rt_parse(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

/**
 * Parse and compile the given list of tokens (from kit::lex).
 * Returned is a list containing the bytecode.
 */
kit_ecode kit_builtins_rt_compile(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

kit_ecode kit_builtins_rt_eval(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

#endif // KIT_STR_BUILTIN_FUNCTIONS_H