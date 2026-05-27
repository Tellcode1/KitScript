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

#ifndef E_POOL_H
#define E_POOL_H

#include "mathstrucs.h"
#include "stdafx.h"

/**
 * I made this after looking at the linux slab allocator.
 * Do take a loot at it! It's really interesting.
 * Teaches you a lot about allocators, caching (both hw and sw).
 */

/**
 * Must be a power of 2 less than (or equal to) 32!
 */
#define E_REFLEAVE_COUNT 32

/**
 * Size of a leaf, excluding the reference counter.
 * 128 bytes, by default.
 */
#define E_REFLEAVE_SIZE sizeof(e_mat4)
struct e_refdobj;
struct e_refdobj_branch;
struct e_refdobj_pool;

/**
 * Common referenced object struct.
 * All referenced structures (currently lists,maps and strings)
 * must follow this layout EXCEPT, the reference counter they hold
 * is a pointer to the one in this struct.
 * Leaf.
 */
typedef struct e_refdobj {
  u8  data[E_REFLEAVE_SIZE] ALIGNAS(16);
  u32 branch_idx;
  u32 leaf_idx;
  int refc; /* Reference counter */
} e_refdobj;

/**
 * Branches.
 * The pool (heap) allocates a branch when it doesn't
 * doesn't see any free nodes.
 */
typedef struct e_refdobj_branch {
  e_refdobj leaves[E_REFLEAVE_COUNT] ALIGNAS(16);

  /* Parent pool */
  struct e_refdobj_pool* pool;
} e_refdobj_branch;

/**
 * Pool of referenced objects.
 * Root.
 */
typedef struct e_refdobj_pool {
  /**
   *  Heap allocated branches.
   */
  e_refdobj_branch** branches;
  u32*               in_use_masks;
  u32                nbranches;
} e_refdobj_pool;

/**
 * TODO: Eliminate the global variables.
 * This script is designed to be used in games!
 * Using global variables will just make it harder to
 * integrate this into long running programs, like say, games.
 * Or not?? Check it out.
 */
extern e_refdobj_pool ge_pool;

/* Returns 0 on success. */
int  e_refdobj_pool_init(u32 nbranches, e_refdobj_pool* pool);
void e_refdobj_pool_free(e_refdobj_pool* pool);

/* Acquire an object from the pool. */
e_refdobj* e_refdobj_pool_acquire(e_refdobj_pool* pool);

/* Return an object to the pool from the first free branch. */
void e_refdobj_pool_return(e_refdobj_pool* pool, e_refdobj* obj);

/* Free all branches not currently in use. */
void e_refdobj_pool_trim(e_refdobj_pool* pool);

#endif // E_POOL_H