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

#include "../inc/map.h"

#include "../inc/list.h"
#include "../inc/pool.h"
#include "../inc/var.h"

#include <stdlib.h>

int
e_map_init(struct e_var* vars, u32 npairs, e_map* map)
{
  if (!map) return -1;
  memset(map, 0, sizeof(*map));

  u32 capacity = MAX(8, npairs);

  map->size     = npairs;
  map->capacity = capacity;
  map->keys     = (e_var*)e_xalloc(capacity, sizeof(e_var));
  map->vals     = (e_var*)e_xalloc(capacity, sizeof(e_var));
  map->hashes   = (u32*)e_xalloc(capacity, sizeof(u32));
  if (!map->keys || !map->vals || !map->hashes) goto err;

  for (u32 i = 0, j = 0; i < (npairs * 2); i += 2, j++) {
    if (e_var_shallow_cpy(&vars[i], &map->keys[j])) goto err;
    if (e_var_shallow_cpy(&vars[i + 1], &map->vals[j])) goto err;

    e_var_acquire(&map->keys[j]);
    e_var_acquire(&map->vals[j]);

    map->hashes[j] = e_var_hash(&vars[i]);

    // eb_print(&map->keys[j], 1);
    // printf(" : ");
    // eb_println(&map->vals[j], 1);
  }
  return 0;

err:
  if (map->keys) free(map->keys);
  if (map->vals) free(map->vals);
  if (map->hashes) free(map->hashes);
  return -1;
}

void
e_map_free(e_map* map)
{
  for (u32 i = 0; i < map->size; i++) {
    e_var_release(&map->keys[i]);
    e_var_release(&map->vals[i]);
  }
  free(map->keys);
  free(map->vals);
  free(map->hashes);
}

e_var*
e_map_find(e_map* map, const e_var* key)
{
  u32 hash = e_var_hash(key);

  for (u32 i = 0; i < map->size; i++) {
    if (hash == map->hashes[i] && e_var_equal(key, &map->keys[i])) { return &map->vals[i]; }
  }
  return NULL;
}

e_var*
e_map_find_or_insert(e_map* map, struct e_var* key)
{
  u32 hash = e_var_hash(key);

  for (u32 i = 0; i < map->size; i++) {
    if (hash == map->hashes[i] && e_var_equal(key, &map->keys[i])) { return &map->vals[i]; }
  }

  if (map->size >= map->capacity) {
    u32    new_capacity = map->capacity * 2;
    e_var* new_keys     = (e_var*)realloc(map->keys, sizeof(e_var) * new_capacity);
    e_var* new_vals     = (e_var*)realloc(map->vals, sizeof(e_var) * new_capacity);
    u32*   new_hashes   = (u32*)realloc(map->hashes, sizeof(u32) * new_capacity);

    map->keys   = new_keys;
    map->vals   = new_vals;
    map->hashes = new_hashes;
  }

  u32   j = map->size;
  e_var v = { .type = E_VARTYPE_NULL };

  e_var* ptr = &map->vals[j];

  e_var_shallow_cpy(key, &map->keys[j]);
  e_var_shallow_cpy(&v, &map->vals[j]);
  e_var_acquire(&map->keys[j]);
  e_var_acquire(&map->vals[j]);

  map->hashes[j] = e_var_hash(key);

  map->size++;

  return ptr;
}

struct e_var
e_map_keys(const e_map* map)
{
  e_list l;
  e_list_init(NULL, 0, &l);

  for (u32 i = 0; i < map->size; i++) {
    e_var_acquire(&map->keys[i]);
    e_list_append(&map->keys[i], &l);
  }

  e_var v = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };
  memcpy(v.val.list, &l, sizeof(e_list));

  return v;
}

struct e_var
e_map_values(const e_map* map)
{
  e_list l;
  e_list_init(NULL, 0, &l);

  for (u32 i = 0; i < map->size; i++) {
    e_var_acquire(&map->vals[i]);
    e_list_append(&map->vals[i], &l);
  }

  e_var v = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };
  memcpy(v.val.list, &l, sizeof(e_list));

  return v;
}
