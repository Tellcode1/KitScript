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

#ifndef E_STACK_H
#define E_STACK_H

#include <stdio.h>
#include <string.h>
#define DEBUG_PRINT_STACK 1

#include "perr.h"
#include "stdafx.h"
#include "var.h"

#include <stddef.h>
#include <stdlib.h>

/**
 * Stack frame.
 * Stack will automatically release
 * the variables on frame pop.
 */
typedef struct e_stack_frame {
  u32 variable_offset;
  u32 stack_size;
} e_stack_frame;

/**
 * The position of a variable in the stack.
 *
 * Initializing a variable pushes an entry
 * in to the variables array of the stack.
 *
 * There can be multiple entries with the same
 * ID! This is how we allow overshadowing of variables.
 */
typedef struct e_var_offset {
  u32 id; /* Hashed name */
  u32 offset_index;
  u32 depth;
} e_var_entry;

/**
 * Stack.
 */
typedef struct e_stack {
  u32    size;
  u32    capacity;
  e_var* stack;

  u32            depth; // nframes
  u32            frame_capacity;
  e_stack_frame* frames;

  u32          nvariables;
  u32          variable_capacity;
  e_var_entry* variables;

  u32 max_usage;
} e_stack;

int  e_stack_init(u32 capacity, u32 frame_capacity, u32 variable_capacity, e_stack* stack) ATTR_NODISCARD;
void e_stack_free(e_stack* stack);

int  e_stack_push_frame(e_stack* stack) ATTR_NODISCARD;
void e_stack_pop_frame(e_stack* stack);

e_var* e_stack_push_variable(u32 id, e_stack* stack);
int    e_stack_push(e_stack* stack, const e_var* v) ATTR_NODISCARD;
void   e_stack_pop(e_stack* stack);

e_var* e_stack_top(e_stack* stack);

e_var* e_stack_find(const e_stack* stack, u32 hash);
e_var* e_stack_find_in_current_scope(const e_stack* stack, u32 hash);

#endif // E_STACK_H