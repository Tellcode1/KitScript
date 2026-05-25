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

#ifndef E_FUNCTION_H
#define E_FUNCTION_H

#include "stdafx.h"

struct e_compiler;

typedef struct e_function {
  u32  name_hash;
  u32  nargs;
  u32  code_size;
  u8*  code;
  u32* arg_slots; /* The ID of the arguments, in order. Arena allocated. */
} e_function;

/* Defined in cc.c */

/**
 * Modify the compiler struct to write to a newly allocated stream!
 * Otherwise this will just overwrite your root stream or append to it.
 * Returns 0 on success.
 */
int e_compile_function(struct e_compiler* cc, int node);

#endif // E_FUNCTION_H
