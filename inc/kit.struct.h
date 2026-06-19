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

#ifndef KIT_STRUCT_H
#define KIT_STRUCT_H

#include "kit.pool.h"
#include "kit.stdafx.h"

struct kit_var;
struct kit_struct_member_pair; // var.h, need kit_var defined

typedef struct kit_struct {
  const char*     name; // owned by file
  struct kit_var* members;
  u32*            member_hashes; // allocated free
  const char**    member_names;  // array allocated, free (individuals from literal table)
  u32             member_count;
} kit_struct;

int  kit_struct_init_from(kit_refdobj_pool* object_pool, const kit_struct* from, kit_struct* s);
int  kit_struct_init_paired(const char* name, u32 nmembers, const char** member_names, const struct kit_struct_member_pair* pairs, kit_struct* s);
int  kit_struct_init(const char* name, u32 nmembers, const char** member_names, kit_struct* s); // All variables initialized to NULL
void kit_struct_free(kit_struct* s);

struct kit_var* kit_struct_get_member(u32 hash, const kit_struct* s);
int             kit_struct_set_member_by_name(const kit_struct* s, const char* name, const struct kit_var* value);
int             kit_struct_set_member_by_index(const kit_struct* s, u32 index, const struct kit_var* value);
int             kit_struct_set_member_pairs(const kit_struct* s, u32 npairs, const struct kit_struct_member_pair* pairs);

#endif // KIT_STRUCT_H