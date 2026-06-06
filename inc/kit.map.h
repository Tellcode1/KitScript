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

#ifndef KIT_MAP_H
#define KIT_MAP_H

#include "kit.stdafx.h"

struct kit_var;

typedef struct kit_map {
  struct kit_var* keys;
  struct kit_var* vals;
  u32*            hashes; // hashes of keys
  u32             size;
  u32             capacity;
} kit_map;

int  kit_map_init(struct kit_var* vars, u32 npairs, kit_map* map);
void kit_map_free(kit_map* map);

struct kit_var* kit_map_find(kit_map* map, const struct kit_var* key);
struct kit_var* kit_map_find_or_insert(kit_map* map, const struct kit_var* key);

/**
 * Get all the keys as a list
 */
struct kit_var kit_map_keys(const kit_map* map);

/**
 * Get all the values as a list
 */
struct kit_var kit_map_values(const kit_map* map);

#endif // KIT_MAP_H