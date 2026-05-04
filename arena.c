#include "arena.h"

#include "stdafx.h"

#include <stdlib.h>
#include <string.h>

static inline uintptr_t
align_up(uintptr_t data, size_t alignment)
{
  if (data % alignment == 0) return data;
  return ((data + alignment - 1) & ~(alignment - 1));
}

static inline int
add_free_page(size_t size, e_arena* arena)
{
  // Round to page size
  size = ((size + E_PAGE_SIZE - 1) / E_PAGE_SIZE) * E_PAGE_SIZE;

  e_arena_page* page = (e_arena_page*)malloc(size);
  if (page == nullptr) return -1;

  page->size = size - sizeof(e_arena_page); // size - metadata_size
  page->head = 0;
  page->next = arena->free_pages;

  arena->free_pages = page;

  return 0;
}

static inline e_arena_page*
get_and_unmark_free_page(e_arena* arena)
{
  if (!arena->free_pages) {
    int e = add_free_page(E_PAGE_SIZE, arena);
    if (e) return NULL;
  }

  e_arena_page* next = arena->free_pages->next;
  e_arena_page* page = arena->free_pages;

  arena->free_pages = next;

  e_arena_page* curr = arena->current;
  arena->current     = page;
  page->head         = 0;
  page->next         = curr;

  return page;
}

int
e_arena_init(u32 npages, e_arena* arena)
{
  npages = MAX(npages, 1);
  memset(arena, 0, sizeof(*arena));

  int e = 0;
  for (u32 i = 0; i < npages; i++) {
    e = add_free_page(E_PAGE_SIZE, arena);
    if (e < 0) return e;
  }

  return e;
}

void
e_arena_resize(e_arena* a)
{ add_free_page(E_PAGE_SIZE, a); }

void*
e_arnalloc(e_arena* a, size_t size)
{
  size_t total = size;
  total        = align_up(total, E_MEMALIGN);

  /* can't fit in regular page. */
  if (total > (E_PAGE_SIZE - sizeof(e_arena_page))) {
    e_arena_page* page = (e_arena_page*)malloc(sizeof(e_arena_page) + total);
    if (!page) return NULL;

    page->size = total;
    page->head = total;
    page->next = a->current;
    a->current = page;

    uchar* data = (uchar*)page + sizeof(*page);
    void*  p    = e_align_ptr(data, E_MEMALIGN);
    if ((uintptr_t)p & (E_MEMALIGN - 1)) abort();
    return p;
  }

  /**
   * Page with enough capacity.
   */
  e_arena_page* fits = a->current;

  // If current pages doesn't meet our requirements,
  // Create an link a new one

  if (fits == nullptr || (fits->size - fits->head) < total) {
    fits = get_and_unmark_free_page(a);
    if (!fits) return NULL; // FAILURE!
  }

  uchar* data = ((uchar*)fits + sizeof(*fits)) + fits->head;

  // And return the pointer after it.
  void* ptr = e_align_ptr(data, E_MEMALIGN);
  fits->head += total;

  /**
   * If we detect the branch is close to being full,
   * we can place an order to the OS for more memory,
   * before we actually need it.
   * This generally improves performance by amortizing
   * malloc cost.
   *
   * Default threshold is 80%
   */
  if ((10 * fits->head) >= (8 * fits->size)) { add_free_page(E_PAGE_SIZE, a); }

  return ptr;
}

char*
e_arnstrdup(e_arena* arena, const char* s)
{
  char* new_s = nullptr;
  if (s != nullptr) {
    size_t l = strlen(s);
    new_s    = (char*)e_arnalloc(arena, l + 1);
    memcpy(new_s, s, l);
    new_s[l] = 0;
  }
  return new_s;
}

void
e_arena_free(e_arena* arena)
{
  e_arena_page* next = arena->current;
  while (next != nullptr) {
    e_arena_page* new_next = next->next;
    free(next);
    next = new_next;
  }
  next = arena->free_pages;
  while (next != nullptr) {
    e_arena_page* new_next = next->next;
    free(next);
    next = new_next;
  }
  memset(arena, 0, sizeof *arena);
}