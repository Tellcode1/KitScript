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

#ifndef E_MAP_H
#define E_MAP_H

#include "stdafx.h"

struct e_var;

typedef struct e_map {
  struct e_var* keys;
  struct e_var* vals;
  u32*          hashes; // hashes of keys
  u32           size;
  u32           capacity;
} e_map;

int  e_map_init(struct e_var* vars, u32 npairs, e_map* map);
void e_map_free(e_map* map);

struct e_var* e_map_find(e_map* map, const struct e_var* key);
struct e_var* e_map_find_or_insert(e_map* map, struct e_var* key);

/**
 * Get all the keys as a list
 */
struct e_var e_map_keys(const e_map* map);

/**
 * Get all the values as a list
 */
struct e_var e_map_values(const e_map* map);

#endif // E_MAP_H