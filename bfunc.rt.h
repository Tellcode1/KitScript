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

#ifndef E_RUNTIME_BUILTIN_FUNCTIONS_H
#define E_RUNTIME_BUILTIN_FUNCTIONS_H

#include "var.h"

/**
 * Compile and execute the source code (code must be given as a string).
 * This involves the usual compilation and then execution.
 * Execution does not inherit the stack frame of the caller.
 * However, arguments can be passed to the entry point.
 */
e_var eb_rt_compile_and_exec(e_var* args, u32 nargs); // rt::exec(code, entry_point, args...)

/**
 * Perform lexical analysis on the text using the builtin E lexicalizer.
 * Returned is a list of tokens (See rt::token structure in bstructs.h)
 */
e_var e_rt_lex(e_var* args, u32 nargs);

/**
 * Parse and compile the given list of tokens (from rt::lex).
 * Returned is a list containing the bytecode.
 */
e_var e_rt_compile(e_var* args, u32 nargs);

#endif // E_STR_BUILTIN_FUNCTIONS_H