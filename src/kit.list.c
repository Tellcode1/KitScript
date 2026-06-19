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

#include "../inc/kit.list.h"

#include "../inc/kit.var.h"

#include <string.h>

int
kit_list_init(kit_refdobj_pool* object_pool, kit_var* vars, u32 nvars, struct kit_list* list)
{
  if (!list) return -1;

  memset(list, 0, sizeof(*list));

  u32 capacity = MAX(8, nvars);

  list->size     = nvars;
  list->capacity = capacity;
  list->vars     = (kit_var*)kit_xalloc(capacity, sizeof(kit_var));
  if (!list->vars) return -1;

  for (u32 i = 0; i < nvars; i++) {
    kit_var_shallow_cpy(&vars[i], &list->vars[i]);
    kit_var_acquire(object_pool, &list->vars[i]); /* tell the big man we want the variables to outlive the list */
  }

  return 0;
}

int
kit_list_append(kit_refdobj_pool* object_pool, const kit_var* v, struct kit_list* list)
{
  if (!list || !v) return -1;

  if (list->size >= list->capacity) {
    int e = kit_list_reserve(MAX(list->capacity * 2, 1), list);
    if (e) return -1;
  }

  kit_var_shallow_cpy(v, &list->vars[list->size]);
  kit_var_acquire(object_pool, &list->vars[list->size]);
  list->size++;

  return 0;
}

kit_var*
kit_list_index(kit_refdobj_pool* object_pool, const struct kit_list* list, u32 index)
{
  if (index >= list->size) return NULL;
  return &list->vars[index];
}

void
kit_list_remove(kit_refdobj_pool* object_pool, u32 index, struct kit_list* list)
{
  if (index >= list->size) return;

  kit_var_release(object_pool, &list->vars[index]);

  // shift elements down. could use memmove but whatever.
  for (u32 i = index; i < list->size - 1; i++) { kit_var_shallow_cpy(&list->vars[i + 1], &list->vars[i]); }

  list->vars[list->size - 1] = KIT_NULLVAR;
  list->size--;
}

void
kit_list_free(kit_refdobj_pool* object_pool, kit_list* list)
{
  for (u32 i = 0; i < list->size; i++) { kit_var_release(object_pool, &list->vars[i]); }
  free(list->vars);
}

void
kit_list_pop(kit_refdobj_pool* object_pool, struct kit_list* list)
{
  if (list->size != 0) {
    kit_var_release(object_pool, &list->vars[list->size - 1]);
    list->size--;
  }
}

int
kit_list_insert(kit_refdobj_pool* object_pool, u32 index, const kit_var* v, struct kit_list* l)
{
  if (!l || !v || index > l->size) return -1;

  if (l->size >= l->capacity) {
    int e = kit_list_reserve(MAX(l->capacity * 2, 1), l);
    if (e) return -1;
  }

  // shr
  for (u32 i = l->size; i > index; i--) {
    kit_var_release(object_pool, &l->vars[i]);
    kit_var_shallow_cpy(&l->vars[i - 1], &l->vars[i]);
    kit_var_acquire(object_pool, &l->vars[i]); /* need acquire here. */
  }
  // I've started to optimize my language... Interesting.

  kit_var_shallow_cpy(v, &l->vars[index]);
  kit_var_acquire(object_pool, &l->vars[index]);

  l->size++;

  return 0;
}

int
kit_list_reserve(u32 new_capacity, struct kit_list* list)
{
  kit_var* new_vars = (kit_var*)realloc(list->vars, sizeof(kit_var) * new_capacity);
  if (!new_vars) return -1;

  for (u32 i = list->size; i < new_capacity; i++) { new_vars[i] = KIT_NULLVAR; }

  list->vars     = new_vars;
  list->capacity = new_capacity;

  return 0;
}

int
kit_list_resize(kit_refdobj_pool* object_pool, u32 new_size, struct kit_list* list)
{
  if (new_size > list->capacity) {
    u32      new_capacity = MAX(new_size, list->capacity * 2);
    kit_var* new_vars     = (kit_var*)realloc(list->vars, sizeof(kit_var) * new_capacity);
    if (!new_vars) return -1;

    // Set new elements to NULL.
    for (u32 i = list->size; i < new_capacity; i++) { new_vars[i] = KIT_NULLVAR; }

    list->vars     = new_vars;
    list->capacity = new_capacity;
  } else if (new_size < list->size) {
    /* Truncating, free old elements */
    for (u32 i = list->size; i > new_size; i--) { kit_var_release(object_pool, &list->vars[i - 1]); }
  }

  list->size = new_size;

  return 0;
}
