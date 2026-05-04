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

#ifndef E_STACK_EMULATOR_H
#define E_STACK_EMULATOR_H

#include "stdafx.h"

struct ecc_variable_information;
struct ecc_struct_information;

/**
 * Stack emulator for the compiler to track stack usage
 * Very tightly knit with the compiler.
 */

typedef struct e_stackemu {
  struct ecc_variable_information* vars;
  u32                              vars_count;
  u32                              vars_capacity;

  struct ecc_struct_information* strucs;
  u32                            strucs_count;
  u32                            strucs_capacity;

  u32* frame_var_counts; // Index into vars array which specifies where they start.
  u32  frame_top;
  u32  frame_capacity;
} e_stackemu;

int  e_stackemu_init(e_stackemu* emu) RETURNS_ERRCODE;
void e_stackemu_free(e_stackemu* emu);

int  e_stackemu_push_frame(e_stackemu* emu) RETURNS_ERRCODE;
void e_stackemu_pop_frame(e_stackemu* emu);

int                              e_stackemu_push_var(e_stackemu* emu, const struct ecc_variable_information* info) RETURNS_ERRCODE;
struct ecc_variable_information* e_stackemu_find_var(const e_stackemu* emu, u32 id);
struct ecc_variable_information* e_stackemu_find_var_in_curr_scope(const e_stackemu* emu, u32 id);

#endif // E_STACK_EMULATOR_H