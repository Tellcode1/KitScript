#include "arena.h"

#include "stdafx.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline uintptr_t
align_up(uintptr_t data, size_t alignment)
{
  if (data % alignment == 0) return data;
  return ((data + alignment - 1) & ~(alignment - 1));
}

static inline ATTR_NODISCARD int
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
get_and_unmark_free_page(e_arena* arena, size_t minimum_size)
{
  if (!arena->free_pages) {
    int e = add_free_page(MAX(E_PAGE_SIZE, minimum_size), arena);
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

int
e_arena_add_free_page(e_arena* a, size_t size)
{ return add_free_page(size, a); }

void*
e_arnalloc(e_arena* a, size_t size)
{
  size_t total = size;
  total        = align_up(total, E_ARENA_MINIMUM_ALIGNMENT);

  /* can't fit in regular page. */
  if (total > (E_PAGE_SIZE - sizeof(e_arena_page))) {
    e_arena_page* page = (e_arena_page*)malloc(sizeof(e_arena_page) + total);
    if (!page) return NULL;

    page->size = total;
    page->head = total;
    page->next = a->current;
    a->current = page;

    uchar* data = (uchar*)page + sizeof(*page);
    void*  p    = e_align_ptr(data, E_ARENA_MINIMUM_ALIGNMENT);
    if ((uintptr_t)p & (E_ARENA_MINIMUM_ALIGNMENT - 1)) abort();
    return p;
  }

  /**
   * Page with enough capacity.
   */
  e_arena_page* fits = a->current;

  // If current pages doesn't meet our requirements,
  // Create an link a new one

  if (fits == nullptr || (fits->size - fits->head) < total) {
    fits = get_and_unmark_free_page(a, total);
    if (!fits) return NULL; // FAILURE!
  }

  uchar* data = ((uchar*)fits + sizeof(*fits)) + fits->head;

  // And return the pointer after it.
  void* ptr = e_align_ptr(data, E_ARENA_MINIMUM_ALIGNMENT);

  uintptr_t start   = (uintptr_t)data;
  uintptr_t aligned = (uintptr_t)ptr;

  fits->head += (aligned - start) + total;

  a->top      = ptr;
  a->top_size = total; // How much we actually moved from ptr

  /**
   * If we detect the branch is close to being full,
   * we can place an order to the OS for more memory,
   * before we actually need it.
   * This generally improves performance by amortizing
   * malloc cost.
   */
  if ((E_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_DENOMINATOR * fits->head) >= (E_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_NUMERATOR * fits->size)) {
    int e = add_free_page(E_PAGE_SIZE, a);
    /* what to do with error? */
    (void)e;
  }

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

void*
e_arnrealloc(e_arena* a, void* top, size_t old_size, size_t new_size)
{
  size_t old_total = align_up(old_size, E_ARENA_MINIMUM_ALIGNMENT);
  size_t new_total = align_up(new_size, E_ARENA_MINIMUM_ALIGNMENT);

  if (top == a->top) {
    if (new_total >= old_total) {
      size_t extra = new_total - old_total;

      size_t left = a->current->size - a->current->head;

      if (left >= extra) {
        a->current->head += extra;
        a->top_size = new_total;
        return top;
      }

    } else {
      size_t shrink = old_total - new_total;

      a->current->head -= shrink;
      a->top_size = new_total;
      return top;
    }
  }

  void* p = e_arnalloc(a, new_size);
  if (!p) return NULL;

  memcpy(p, top, MIN(old_size, new_size));

  return p;
}
