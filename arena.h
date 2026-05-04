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

#ifndef E_ARENA_H
#define E_ARENA_H

#include "stdafx.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Minimum size of memory block that is allocated.
 */
#define E_PAGE_SIZE (64ULL * 1024ULL)

/**
 * Minimum bytes that memory allocated is aligned to.
 * Applies both to malloc and realloc.
 */
#define E_MEMALIGN 8

/* Data is (uchar*)&page + sizeof(size_t) */
typedef struct e_arena_page {
  struct e_arena_page* next;
  size_t               size;
  size_t               head;
} e_arena_page;

typedef struct e_arena {
  struct e_arena_page* current;
  struct e_arena_page* free_pages;
} e_arena;

int e_arena_init(u32 npages, e_arena* arena) ATTR_NODISCARD;

void  e_arena_resize(e_arena* a);
void* e_arnalloc(e_arena* a, size_t size);
char* e_arnstrdup(e_arena* arena, const char* s);
void  e_arena_free(e_arena* arena);

#endif // E_ARENA_H