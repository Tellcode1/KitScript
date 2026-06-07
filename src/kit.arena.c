#include "../inc/kit.arena.h"

#include "../inc/kit.stdafx.h"

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
add_free_page(size_t size, kit_arena* arena)
{
  // Round to page size
  size = ((size + KIT_PAGE_SIZE - 1) / KIT_PAGE_SIZE) * KIT_PAGE_SIZE;

  kit_arena_page* page = (kit_arena_page*)kit_xalloc(1, sizeof(kit_arena_page) + (KIT_ARENA_MINIMUM_ALIGNMENT - 1) + size);
  if (page == NULL) return -1;

  page->size = size - sizeof(kit_arena_page); // size - metadata_size
  page->head = 0;
  page->next = arena->free_pages;

  arena->free_pages = page;

  return 0;
}

static inline kit_arena_page*
get_and_unmark_free_page(kit_arena* arena, size_t minimum_size)
{
  if (!arena->free_pages) {
    int e = add_free_page(MAX(KIT_PAGE_SIZE, minimum_size), arena);
    if (e) return NULL;
  }

  kit_arena_page* next = arena->free_pages->next;
  kit_arena_page* page = arena->free_pages;

  arena->free_pages = next;

  kit_arena_page* curr = arena->current;
  arena->current       = page;
  page->head           = 0;
  page->next           = curr;

  return page;
}

int
kit_arena_init(u32 npages, kit_arena* arena)
{
  npages = MAX(npages, 1);
  memset(arena, 0, sizeof(*arena));

  int e = 0;
  for (u32 i = 0; i < npages; i++) {
    e = add_free_page(KIT_PAGE_SIZE, arena);
    if (e < 0) return e;
  }

  return e;
}

int
kit_arena_add_free_page(kit_arena* a, size_t size)
{ return add_free_page(size, a); }

void*
kit_arnalloc(kit_arena* a, size_t size)
{
  size_t total = size;
  total        = align_up(total, KIT_ARENA_MINIMUM_ALIGNMENT);

  /* can't fit in regular page. */
  if (total + sizeof(kit_arena_page) + (KIT_ARENA_MINIMUM_ALIGNMENT - 1) > KIT_PAGE_SIZE) {
    kit_arena_page* page = (kit_arena_page*)kit_xalloc(1, sizeof(kit_arena_page) + (KIT_ARENA_MINIMUM_ALIGNMENT - 1) + total);
    if (!page) return NULL;

    page->size = total;
    page->head = total;
    page->next = a->current;
    a->current = page;

    uchar* data = (uchar*)page + sizeof(*page);
    void*  p    = kit_align_ptr(data, KIT_ARENA_MINIMUM_ALIGNMENT);

    return p;
  }

  /**
   * Page with enough capacity.
   */
  kit_arena_page* fits = a->current;

  // If current pages doesn't meet our requirements,
  // Create and link a new one

  if (fits == NULL || (fits->size - fits->head) < total + (KIT_ARENA_MINIMUM_ALIGNMENT - 1)) {
    fits = get_and_unmark_free_page(a, total);
    if (!fits) return NULL; // FAILURE!
  }

  uchar* data = ((uchar*)fits + sizeof(*fits)) + fits->head;

  // And return the pointer after it.
  void* ptr = kit_align_ptr(data, KIT_ARENA_MINIMUM_ALIGNMENT);

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
  if ((KIT_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_DENOMINATOR * fits->head)
      >= (KIT_ARENA_ALLOCATION_AMORTIZATION_THRESHOLD_NUMERATOR * fits->size)) {
    int e = add_free_page(KIT_PAGE_SIZE, a);
    /* what to do with error? */
    (void)e;
  }

  memset(ptr, 0, size);

  return ptr;
}

char*
kit_arnstrdup(kit_arena* arena, const char* s)
{
  char* new_s = NULL;
  if (s != NULL) {
    size_t l = strlen(s);
    new_s    = (char*)kit_arnalloc(arena, l + 1);
    memcpy(new_s, s, l);
    new_s[l] = 0;
  }
  return new_s;
}

int
kit_arena_reset(kit_arena* arena)
{
  kit_arena_page* used = arena->current;
  arena->current       = NULL;

  if (used != NULL) {
    kit_arena_page* last = used;
    while (last->next != NULL) {
      last->head = 0; // mark page as free
      last       = last->next;
    }
    last->head = 0;

    last->next        = arena->free_pages;
    arena->free_pages = used;
  }

  return 0;
}

void
kit_arena_free(kit_arena* arena)
{
  kit_arena_page* next = arena->current;
  while (next != NULL) {
    kit_arena_page* new_next = next->next;
    free(next);
    next = new_next;
  }
  next = arena->free_pages;
  while (next != NULL) {
    kit_arena_page* new_next = next->next;
    free(next);
    next = new_next;
  }
  memset(arena, 0, sizeof *arena);
}

void*
kit_arnrealloc(kit_arena* a, void* top, size_t old_size, size_t new_size)
{
  size_t old_total = align_up(old_size, KIT_ARENA_MINIMUM_ALIGNMENT);
  size_t new_total = align_up(new_size, KIT_ARENA_MINIMUM_ALIGNMENT);

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

  void* p = kit_arnalloc(a, new_size);
  if (!p) return NULL;

  memcpy(p, top, MIN(old_size, new_size));

  return p;
}

void
kit_arnfree(kit_arena* a, void* ptr)
{
  if (ptr == a->top) {
    if (a->top_size >= a->current->head) {
      /* mark page as free and move it to the free list */
      a->current->head = 0;

      /* set current page to next linked page */
      kit_arena_page* curr = a->current;
      a->current           = curr->next;

      /* link our free page */
      curr->next    = a->free_pages;
      a->free_pages = curr;

    } else a->current->head -= a->top_size;
  }
}
