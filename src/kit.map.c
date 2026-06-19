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

#include "../inc/kit.map.h"

#include "../inc/kit.list.h"
#include "../inc/kit.pool.h"
#include "../inc/kit.var.h"

#include <stdlib.h>

int
kit_map_init(kit_refdobj_pool* object_pool, struct kit_var* vars, u32 npairs, kit_map* map)
{
  if (!map) return -1;
  memset(map, 0, sizeof(*map));

  u32 capacity = MAX(8, npairs);

  map->size     = npairs;
  map->capacity = capacity;
  map->keys     = (kit_var*)kit_xalloc(capacity, sizeof(kit_var));
  map->vals     = (kit_var*)kit_xalloc(capacity, sizeof(kit_var));
  map->hashes   = (u32*)kit_xalloc(capacity, sizeof(u32));
  if (!map->keys || !map->vals || !map->hashes) goto err;

  for (u32 i = 0, j = 0; i < (npairs * 2); i += 2, j++) {
    if (kit_var_shallow_cpy(&vars[i], &map->keys[j])) goto err;
    if (kit_var_shallow_cpy(&vars[i + 1], &map->vals[j])) goto err;

    kit_var_acquire(object_pool, &map->keys[j]);
    kit_var_acquire(object_pool, &map->vals[j]);

    map->hashes[j] = kit_var_hash(&vars[i]);

    // kit_builtins_print(&map->keys[j], 1);
    // printf(" : ");
    // kit_builtins_println(&map->vals[j], 1);
  }
  return 0;

err:
  if (map->keys) free(map->keys);
  if (map->vals) free(map->vals);
  if (map->hashes) free(map->hashes);
  return -1;
}

void
kit_map_free(kit_refdobj_pool* object_pool, kit_map* map)
{
  for (u32 i = 0; i < map->size; i++) {
    kit_var_release(object_pool, &map->keys[i]);
    kit_var_release(object_pool, &map->vals[i]);
  }
  free(map->keys);
  free(map->vals);
  free(map->hashes);
}

kit_var*
kit_map_find(kit_map* map, const kit_var* key)
{
  u32 hash = kit_var_hash(key);

  for (u32 i = 0; i < map->size; i++) {
    if (hash == map->hashes[i] && kit_var_equal(key, &map->keys[i])) { return &map->vals[i]; }
  }
  return NULL;
}

kit_var*
kit_map_find_or_insert(kit_refdobj_pool* object_pool, kit_map* map, const struct kit_var* key)
{
  u32 hash = kit_var_hash(key);

  for (u32 i = 0; i < map->size; i++) {
    if (hash == map->hashes[i] && kit_var_equal(key, &map->keys[i])) { return &map->vals[i]; }
  }

  if (map->size >= map->capacity) {
    u32      new_capacity = map->capacity * 2;
    kit_var* new_keys     = (kit_var*)realloc(map->keys, sizeof(kit_var) * new_capacity);
    kit_var* new_vals     = (kit_var*)realloc(map->vals, sizeof(kit_var) * new_capacity);
    u32*     new_hashes   = (u32*)realloc(map->hashes, sizeof(u32) * new_capacity);

    map->keys   = new_keys;
    map->vals   = new_vals;
    map->hashes = new_hashes;
  }

  u32     j = map->size;
  kit_var v = { .type = KIT_VARTYPE_NULL };

  kit_var* ptr = &map->vals[j];

  kit_var_shallow_cpy(key, &map->keys[j]);
  kit_var_shallow_cpy(&v, &map->vals[j]);
  kit_var_acquire(object_pool, &map->keys[j]);
  kit_var_acquire(object_pool, &map->vals[j]);

  map->hashes[j] = kit_var_hash(key);

  map->size++;

  return ptr;
}

struct kit_var
kit_map_keys(kit_refdobj_pool* object_pool, const kit_map* map)
{
  kit_list l;
  kit_list_init(object_pool, NULL, 0, &l);

  for (u32 i = 0; i < map->size; i++) {
    kit_var_acquire(object_pool, &map->keys[i]);
    kit_list_append(object_pool, &map->keys[i], &l);
  }

  kit_var v = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(object_pool),
  };
  memcpy(v.val.list, &l, sizeof(kit_list));

  return v;
}

struct kit_var
kit_map_values(kit_refdobj_pool* object_pool, const kit_map* map)
{
  kit_list l;
  kit_list_init(object_pool, NULL, 0, &l);

  for (u32 i = 0; i < map->size; i++) {
    kit_var_acquire(object_pool, &map->vals[i]);
    kit_list_append(object_pool, &map->vals[i], &l);
  }

  kit_var v = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(object_pool),
  };
  memcpy(v.val.list, &l, sizeof(kit_list));

  return v;
}
