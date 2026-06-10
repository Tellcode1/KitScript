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

#ifndef KIT_LIST_H
#define KIT_LIST_H

#include "kit.stdafx.h"

struct kit_var;

typedef struct kit_list {
  struct kit_var* vars;
  u32             size;
  u32             capacity;
} kit_list;

int  kit_list_init(struct kit_var* vars, u32 nvars, struct kit_list* list);
void kit_list_free(struct kit_list* list);

int kit_list_init_from_int_array(const int* arr, u32 nelems, kit_list* list);
int kit_list_init_from_float_array(const double* arr, u32 nelems, kit_list* list);
int kit_list_init_from_bool_array(const bool* arr, u32 nelems, kit_list* list);
int kit_list_init_from_char_array(const char* arr, u32 nelems, kit_list* list);

struct kit_var* kit_list_index(const struct kit_list* list, u32 index);
int             kit_list_append(const struct kit_var* v, kit_list* list);
void            kit_list_pop(kit_list* list);
int             kit_list_insert(u32 index, const struct kit_var* v, kit_list* list);
void            kit_list_remove(u32 index, kit_list* list);

int kit_list_reserve(u32 new_capacity, kit_list* list);
int kit_list_resize(u32 new_size, kit_list* list);

/* Perform tim sort in place on an array */
int kit_tim_sort(struct kit_var* elems, u32 nelems);

#endif // KIT_LIST_H