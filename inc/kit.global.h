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

#ifndef KIT_GLOBAL_NAMESPACE_H
#define KIT_GLOBAL_NAMESPACE_H

#include "kit.stdafx.h"

struct kitc_function;
struct kit_var;

typedef struct kit_global_namespace {
  /* array owned by struct */
  struct kitc_function* funcs;
  u32                   funcs_count;
  u32                   funcs_capacity;

  /* array owned by struct */
  struct kit_var* vars;
  u32*            var_hashes; /* the hash for the variable at the same index */
  u32             vars_count;
  u32             vars_capacity;
} kit_global_namespace;

int  kit_global_namespace_init(const struct kitc_function* funcs, u32 nfuncs, const struct kit_var* vars, u32 nvars, kit_global_namespace* dst);
void kit_global_namespace_free(kit_global_namespace* space);

int kit_global_namespace_add_function(struct kitc_function* func, kit_global_namespace* space);
int kit_global_namespace_add_variable(u32 variable_name_hash, struct kit_var* var, kit_global_namespace* space);

struct kitc_function* kit_global_namespace_find_function(u32 id, kit_global_namespace* space);
struct kit_var*       kit_global_namespace_find_variable(u32 id, kit_global_namespace* space);

#endif // KIT_GLOBAL_NAMESPACE_H