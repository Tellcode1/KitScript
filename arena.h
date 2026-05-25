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
#ifndef E_PAGE_SIZE
#  define E_PAGE_SIZE (8ULL * 1024ULL)
#endif

/**
 * Minimum bytes that memory allocated is aligned to.
 * Applies both to malloc and realloc.
 */
#ifndef E_ARENA_MINIMUM_ALIGNMENT
#  define E_ARENA_MINIMUM_ALIGNMENT (16)
#endif

/**
 * If set to false, (or unset),
 * When allocations reach the threshold specified below (default is 80%),
 * a new page is preallocated through e_add_free_page
 * before we actually need it.
 * This generally improves performance by amortizing
 * malloc cost.
 *
 * Can be disabled on memory tight systems to reduce overallocation.
 *
 * Default threshold is 80% (on 80% page full, a new page is added)
 */
#ifndef E_ARENA_DISABLE_ALLOCATION_AMORTIZATION
#  define E_ARENA_DISABLE_ALLOCATION_AMORTIZATION (false)
#endif

#ifndef E_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_NUMERATOR
#  define E_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_NUMERATOR (8)
#endif
#ifndef E_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_DENOMINATION
#  define E_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_DENOMINATION (10)
#endif

/* Data is (uchar*)&page + sizeof(e_arena_page) */
typedef struct e_arena_page {
  struct e_arena_page* next; /* Next page.*/

  /* Size of the page. Need not be equal to E_PAGE_SIZE. */
  size_t size;

  /* Bumper */
  size_t head;

  /* Data */
} e_arena_page;

typedef struct e_arena {
  /**
   * Head of the linked list of pages in use.
   * Newest first.
   * When the arena is exhausted, a free page is searched for
   * in the free list of this structure. If it is found, it is removed
   * from there and current is set to that page.
   * If no free page exists, a new one is allocated and current is set to it.
   *
   * (ExhaustedPage, current) -> (ExhaustedPage, current->next) -> (ExhaustedPage, current->next->next)
   * Allocation needed! We do not have a page in our list with enough size required...
   *
   * Lets search the free list
   * (FreePage) -> (AnotherFreePage) ->(AnotherAnotherFreePage)
   *
   * Let's take out the first FreePage, and set current to it, linking the old current.
   *
   * So our new current linked list is:
   * (FreePage, current) -> (ExhaustedPage, current->next) -> (ExhaustedPage, current->next->next) -> (ExhaustedPage, current->next->next->next)
   *
   * And our free list will be:
   * (AnotherFreePage) -> (AnotherAnotherFreePage)
   */
  struct e_arena_page* current;

  /**
   * Free list. Free pages can be added to the freelist (for example, for preallocation)
   * by calling e_arena_add_free_page.
   */
  struct e_arena_page* free_pages;

  void*  top;      // Last allocation pointer
  size_t top_size; // How much we allocated
} e_arena;

/**
 * @brief Allocate a new arena with 'npages' of pages with size equal to E_PAGESIZE.
 *        npages must atleast be 1.
 */
int e_arena_init(u32 npages, e_arena* arena) ATTR_NODISCARD;

/**
 * @brief Allocate a free page and link it to the freelist of the arena.
 * @param size The size of the page. Need not be equal to E_PAGESIZE.
 */
int e_arena_add_free_page(e_arena* a, size_t size) ATTR_NODISCARD;

/**
 * @brief Allocate a block of memory from the arena, aligned to E_ARENA_MINIMUM_ALIGNMENT.
 * @param size The size of the block of memory to allocate.
 */
void* e_arnalloc(e_arena* a, size_t size) ATTR_NODISCARD;

/**
 * @brief Allocate an aligned block of memory from the arena.
 * @param size The size of memory to allocate, need not be aligned up.
 * @param align The alignment with which to allocate, no less than E_ARENA_MINIMUM_ALIGNMENT (rounded up), must be a power of 2
 */
void* e_arnalloc_aligned(e_arena* a, size_t size, size_t align) ATTR_NODISCARD;

/**
 * @brief Rellocate a block of memory from the arena, aligned to E_ARENA_MINIMUM_ALIGNMENT.
 * @param top The pointer to reallocate. Should be the stack top. Otherwise, this function switches to e_arnalloc.
 * @param size The new size of the memory block
 */
void* e_arnrealloc(e_arena* a, void* top, size_t old_size, size_t new_size) ATTR_NODISCARD;

/**
 * @brief Duplicate a NULL terminated string, allocating the new one from the arena
 * @param s The string to duplicate, NULL terminated.
 */
char* e_arnstrdup(e_arena* arena, const char* s) ATTR_NODISCARD;

/**
 * @brief Free the arena.
 */
void e_arena_free(e_arena* arena);

#endif // E_ARENA_H