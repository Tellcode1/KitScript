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

#include "pool.h"

#include "stdafx.h"

#include <stdint.h>

e_refdobj_pool ge_pool = { 0 };

static int
branch_init(e_refdobj_pool* pool, e_refdobj_branch* b)
{
  memset(b, 0, sizeof(*b));
  b->free_leaves = E_REFLEAVE_COUNT; // All leaves are free
  b->pool        = pool;
  return 0;
}

int
e_refdobj_pool_init(u32 nbranches, e_refdobj_pool* pool)
{
  memset(pool, 0, sizeof(*pool));

  pool->nbranches = nbranches;

  // array of pointers to branches!
  pool->branches = (e_refdobj_branch**)e_xalloc(nbranches, sizeof(e_refdobj_branch*));

  if (!pool->branches) return -1;

  for (u32 i = 0; i < nbranches; i++) {
    pool->branches[i] = (e_refdobj_branch*)e_aligned_malloc(sizeof(e_refdobj_branch), 16);
    if (!pool->branches[i]) return -1;

    if (branch_init(pool, pool->branches[i])) return -1;
  }

  return 0;
}

void
e_refdobj_pool_free(e_refdobj_pool* pool)
{
  for (u32 i = 0; i < pool->nbranches; i++) e_aligned_free(pool->branches[i]);
  E_ARR_FREE(pool->branches);
}

e_refdobj*
e_refdobj_pool_acquire(e_refdobj_pool* pool)
{ /* Find a free leaf on a branch, and return it */
  e_refdobj_branch* br = nullptr;
  for (u32 i = 0; i < pool->nbranches; i++) {
    if (!pool->branches[i] || pool->branches[i]->free_leaves == 0) continue;
    br = pool->branches[i];
    break;
  }

  /*  Didn't find a free branch. */
  if (!br) {
    /* Double number of branches and recurse */
    u32 new_branch_count = pool->nbranches * 2;
    u32 old_branch_count = pool->nbranches;

    e_refdobj_branch** new_branches = E_ARR_REALLOC(pool->branches, e_refdobj_branch*, new_branch_count);

    if (!new_branches) return nullptr;

    pool->branches  = new_branches;
    pool->nbranches = new_branch_count;

    /* Setup all new branches. Running from old branch count to new branch count. */
    for (u32 i = old_branch_count; i < pool->nbranches; i++) {
      pool->branches[i] = (e_refdobj_branch*)e_aligned_malloc(sizeof(e_refdobj_branch), 16);
      if (!pool->branches[i]) return nullptr;

      branch_init(pool, pool->branches[i]);
    }

    /* Recurse */
    return e_refdobj_pool_acquire(&ge_pool);
  }

  /* br is free */
  for (u32 i = 0; i < E_REFLEAVE_COUNT; i++) {
    if (!(br->leaves_in_use & (1ULL << i))) {
      br->leaves_in_use |= 1ULL << i;
      br->free_leaves--;
      br->pool = pool;

      /* Reset reference counter */
      br->leaves[i].refc = 1;
      return &br->leaves[i];
    }
  }

  /* Should never reach here. br->free_nodes should be accurate. */
  return nullptr;
}

void
e_refdobj_pool_return(e_refdobj_pool* pool, e_refdobj* obj)
{
  u32 leaf_index = UINT32_MAX;

  e_refdobj_branch* br = nullptr;
  for (u32 i = 0; i < pool->nbranches; i++) {
    br = pool->branches[i];
    if (!br || br->free_leaves == E_REFLEAVE_COUNT) continue;

    /* Object isn't in this branches range? */
    if (obj < br->leaves || obj >= (br->leaves + E_REFLEAVE_COUNT)) continue;

    /* Object is in this branch! Find its index */
    for (u32 j = 0; j < E_REFLEAVE_COUNT; j++) {
      if (&br->leaves[j] == obj) {
        leaf_index = j;
        break;
      }
    }

    // Found the branch
    if (leaf_index != UINT32_MAX) break;
  }

  /* Didn't find the object in the pool. Possible double free. */
  if (!br || leaf_index == UINT32_MAX) return;

  /* Explicit double free check. */
  if (!(br->leaves_in_use & (1ULL << leaf_index))) return;

  br->free_leaves++;
  br->leaves_in_use &= ~(1ULL << leaf_index); // Unset in_use bit
}

void
e_refdobj_pool_trim(e_refdobj_pool* pool)
{
  for (u32 i = 0; i < pool->nbranches; i++) {
    if (!pool->branches[i]) continue;

    e_refdobj_branch* br = pool->branches[i];

    // Every leaf is free
    if (br->free_leaves == E_REFLEAVE_COUNT) {
      e_aligned_free(br);
      pool->branches[i] = nullptr;
    }
  }
}