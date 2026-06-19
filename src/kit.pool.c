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

#include "../inc/kit.pool.h"

#include "../inc/kit.stdafx.h"

#include <stdint.h>
#include <string.h>

static int
branch_init(kit_refdobj_pool* pool, kit_refdobj_branch* b)
{
  memset(b, 0, sizeof(*b));
  b->pool = pool;
  return 0;
}

int
kit_refdobj_pool_init(u32 nbranches, kit_refdobj_pool* pool)
{
  memset(pool, 0, sizeof(*pool));

  pool->nbranches = nbranches;

  // array of pointers to branches!
  pool->branches = (kit_refdobj_branch**)kit_aligned_malloc(nbranches * sizeof(kit_refdobj_branch*), 16);
  if (!pool->branches) return -1;

  pool->in_use_masks = (u32*)kit_xalloc(nbranches, sizeof(u32));
  if (!pool->in_use_masks) return -1;

  memset((void*)pool->branches, 0x0, sizeof(kit_refdobj_branch*) * nbranches);
  memset(pool->in_use_masks, 0x0, sizeof(u32) * nbranches);

  for (u32 i = 0; i < nbranches; i++) {
    pool->branches[i] = (kit_refdobj_branch*)kit_aligned_malloc(sizeof(kit_refdobj_branch), 16);
    if (!pool->branches[i]) return -1;

    if (branch_init(pool, pool->branches[i])) return -1;
  }

  return 0;
}

void
kit_refdobj_pool_free(kit_refdobj_pool* pool)
{
  for (u32 i = 0; i < pool->nbranches; i++) kit_aligned_free(pool->branches[i]);
  KIT_ARR_FREE(pool->in_use_masks);
  kit_aligned_free((void*)pool->branches);
}

static inline int
first_unset_bit_in_array(const u32* x, u32 n, u32* word_idx, u32* bit_idx)
{
#if defined(__has_builtin) && __has_builtin(__builtin_ctzl)
  for (u32 i = 0; i < n; i++) {
    if (x[i] == 0xFFFFFFFF) continue;
    if (word_idx) *word_idx = i;
    if (bit_idx) *bit_idx = __builtin_ctzl(~x[i]);
    return 0;
  }
#else
  for (u32 j = 0; j < n; j++) {
    for (u32 i = 0; i < sizeof(u32) * 8; i++) {
      if (!(x[j] & (1 << i))) {
        if (byte_idx) *byte_idx = j;
        if (bit_idx) *bit_idx = i;
        return 0;
      }
    }
  }
#endif
  return -1;
}

kit_refdobj*
kit_refdobj_pool_acquire(kit_refdobj_pool* pool)
{
  /* Find a free leaf on a branch, and return it */

  u32 branch_idx = 0;
  u32 leaf_idx   = 0;

  if (first_unset_bit_in_array(pool->in_use_masks, pool->nbranches, &branch_idx, &leaf_idx)) {
    /* No free leaves. Add more. */

    /* Double number of branches and recurse */
    u32 new_branch_count = pool->nbranches * 2;
    u32 old_branch_count = pool->nbranches;
    // KIT_ARR_REALLOC(pool->branches, kit_refdobj_branch*, new_branch_count);
    kit_refdobj_branch** new_branches = (kit_refdobj_branch**)kit_aligned_realloc(
        (void*)pool->branches, old_branch_count * sizeof(kit_refdobj_branch*), new_branch_count * sizeof(kit_refdobj_branch*), 16);
    if (!new_branches) return NULL;

    u32* new_free_lists = KIT_ARR_REALLOC(pool->in_use_masks, u32, new_branch_count);
    if (!new_free_lists) return NULL;

    pool->branches     = new_branches;
    pool->in_use_masks = new_free_lists;
    pool->nbranches    = new_branch_count;

    for (u32 i = pool->nbranches; i < new_branch_count; i++) { pool->in_use_masks[i] = 0; }

    /* Setup all new branches. Running from old branch count to new branch count. */
    for (u32 i = old_branch_count; i < pool->nbranches; i++) {
      pool->branches[i] = (kit_refdobj_branch*)kit_aligned_malloc(sizeof(kit_refdobj_branch), 16);
      if (!pool->branches[i]) return NULL;

      branch_init(pool, pool->branches[i]);
    }

    /* Recurse */
    return kit_refdobj_pool_acquire(pool);
  }

  kit_refdobj_branch* br = pool->branches[branch_idx];
  if (!br) return NULL;

  pool->in_use_masks[branch_idx] |= 1ULL << leaf_idx;
  br->pool = pool;

  kit_refdobj* leaf = &br->leaves[leaf_idx];
  memset(leaf, 0, sizeof(*leaf));

  /* Reset reference counter */
  leaf->refc = 1;

  /* Fill in location */
  leaf->branch_idx = branch_idx;
  leaf->leaf_idx   = leaf_idx;
  return leaf;
}

void
kit_refdobj_pool_return(kit_refdobj_pool* pool, kit_refdobj* obj)
{
  u32 branch_idx = obj->branch_idx;
  u32 leaf_idx   = obj->leaf_idx;

  // OOB
  if (branch_idx >= pool->nbranches) return;

  kit_refdobj_branch* br = pool->branches[branch_idx];

  /* Didn't find the object in the pool. Possible double free. */
  if (!br) return;

  /* Explicit double free check. */
  if (!(pool->in_use_masks[branch_idx] & (1ULL << leaf_idx))) return;

  pool->in_use_masks[branch_idx] &= ~(1ULL << leaf_idx); // Unset in_use bit
}

void
kit_refdobj_pool_trim(kit_refdobj_pool* pool)
{
  for (u32 i = 0; i < pool->nbranches; i++) {
    if (!pool->branches[i]) continue;

    kit_refdobj_branch* br = pool->branches[i];

    // Every leaf is free
    if (pool->in_use_masks[i] == 0) {
      kit_aligned_free(br);
      pool->branches[i] = NULL;
    }
  }
}