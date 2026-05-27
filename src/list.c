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

#include "../inc/list.h"

#include "../inc/var.h"

#include <string.h>

int
e_list_init(e_var* vars, u32 nvars, struct e_list* list)
{
  if (!list) return -1;

  memset(list, 0, sizeof(*list));

  u32 capacity = MAX(8, nvars);

  list->size     = nvars;
  list->capacity = capacity;
  list->vars     = (e_var*)e_xalloc(capacity, sizeof(e_var));
  if (!list->vars) return -1;

  for (u32 i = 0; i < nvars; i++) {
    e_var_shallow_cpy(&vars[i], &list->vars[i]);
    e_var_acquire(&list->vars[i]); /* tell the big man we want the variables to outlive the list */
  }

  return 0;
}

int
e_list_append(const e_var* v, struct e_list* list)
{
  if (!list || !v) return -1;

  if (list->size >= list->capacity) {
    int e = e_list_reserve(MAX(list->capacity * 2, 1), list);
    if (e) return -1;
  }

  e_var_shallow_cpy(v, &list->vars[list->size]);
  e_var_acquire(&list->vars[list->size]);
  list->size++;

  return 0;
}

e_var*
e_list_index(const struct e_list* list, u32 index)
{
  if (index >= list->size) return nullptr;
  return &list->vars[index];
}

void
e_list_remove(u32 index, struct e_list* list)
{
  if (index >= list->size) return;

  e_var_release(&list->vars[index]);

  // shift elements down. could use memmove but whatever.
  for (u32 i = index; i < list->size - 1; i++) { list->vars[i] = list->vars[i + 1]; }

  list->size--;
}

void
e_list_free(e_list* list)
{
  for (u32 i = 0; i < list->size; i++) { e_var_release(&list->vars[i]); }
  free(list->vars);
}

void
e_list_pop(struct e_list* list)
{
  if (list->size != 0) {
    e_var_release(&list->vars[list->size - 1]);
    list->size--;
  }
}

int
e_list_insert(u32 index, const e_var* v, struct e_list* l)
{
  if (!l || !v || index > l->size) return -1;

  if (l->size >= l->capacity) {
    int e = e_list_reserve(MAX(l->capacity * 2, 1), l);
    if (e) return -1;
  }

  // shr
  for (u32 i = l->size; i > index; i--) { e_var_shallow_cpy(&l->vars[i - 1], &l->vars[i]); }
  // I've started to optimize my language... Interesting.

  e_var_shallow_cpy(v, &l->vars[index]);
  e_var_acquire(&l->vars[index]);

  l->size++;

  return 0;
}

int
e_list_reserve(u32 new_capacity, struct e_list* list)
{
  e_var* new_vars = (e_var*)realloc(list->vars, sizeof(e_var) * new_capacity);
  if (!new_vars) return -1;

  for (u32 i = list->size; i < new_capacity; i++) { new_vars[i] = E_NULLVAR; }

  list->vars     = new_vars;
  list->capacity = new_capacity;

  return 0;
}

int
e_list_resize(u32 new_size, struct e_list* list)
{
  if (new_size > list->capacity) {
    u32    new_capacity = MAX(new_size, list->capacity * 2);
    e_var* new_vars     = (e_var*)realloc(list->vars, sizeof(e_var) * new_capacity);
    if (!new_vars) return -1;

    // Set new elements to NULL.
    for (u32 i = list->size; i < new_capacity; i++) { new_vars[i] = E_NULLVAR; }

    list->vars     = new_vars;
    list->capacity = new_capacity;
  } else if (new_size < list->size) {
    /* Truncating, free old elements */
    for (u32 i = list->size; i > new_size; i--) { e_var_release(&list->vars[i - 1]); }
  }

  list->size = new_size;

  return 0;
}
