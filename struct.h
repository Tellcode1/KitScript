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

#ifndef E_STRUCT_H
#define E_STRUCT_H

#include "stdafx.h"

struct e_var;
struct e_struct_member_pair; // var.h, need e_var defined

typedef struct e_struct {
  struct e_var* members;
  u32*          member_hashes; // allocated free
  const char**  member_names;  // array allocated, free (individuals from literal table)
  u32           member_count;
} e_struct;

int  e_struct_init_from(const e_struct* from, e_struct* s);
int  e_struct_init_paired(u32 nmembers, const char** member_names, const struct e_struct_member_pair* pairs, e_struct* s);
int  e_struct_init(u32 nmembers, const char** member_names, e_struct* s); // All variables initialized to NULL
void e_struct_free(e_struct* s);

struct e_var* e_struct_get_member(u32 hash, const e_struct* s);
int           e_struct_set_member_by_name(const e_struct* s, const char* name, const struct e_var* value);
int           e_struct_set_member_by_index(const e_struct* s, u32 index, const struct e_var* value);
int           e_struct_set_member_pairs(const e_struct* s, u32 npairs, const struct e_struct_member_pair* pairs);

#endif // E_STRUCT_H