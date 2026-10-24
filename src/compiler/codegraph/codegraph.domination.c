#include "codegraph.h"

#include "../../../inc/kit.arena.h"
#include "../../../inc/kit.cc.h"

int
codegraph_domination_analysis(kit_compiler* cc, codegraph* cfg)
{
  for (u32 b = 0; b < cfg->nblocks; b++) {
    codeblock* blk = &cfg->blocks[b];

    blk->dominators = kit_arnalloc(cfg->arena, cfg->nblocks * sizeof(bool));

    if (b == cfg->entry_block) {
      memset(blk->dominators, 0, cfg->nblocks * sizeof(bool));

      /* entry block dominates itself */
      blk->dominators[b] = true;
    } else {
      /* (start with) everything else dominating everything else */
      memset(blk->dominators, 1, cfg->nblocks * sizeof(bool));
    }
  }

  bool changed = true;

  while (changed) {
    changed = false;

    for (u32 b = 0; b < cfg->nblocks; b++) {
      if (b == cfg->entry_block) continue;

      codeblock* blk = &cfg->blocks[b];

      bool* new_dom = kit_arnalloc(cfg->arena, cfg->nblocks * sizeof(bool));

      /* start as all true */
      memset(new_dom, 1, cfg->nblocks * sizeof(bool));

      for (u32 p = 0; p < blk->npredecessors; p++) {
        codeblock* pred = &cfg->blocks[blk->predecessors[p]];

        for (u32 i = 0; i < cfg->nblocks; i++) { new_dom[i] = new_dom[i] && pred->dominators[i]; }
      }

      /* add self */
      new_dom[b] = true;

      if (memcmp(new_dom, blk->dominators, cfg->nblocks * sizeof(bool)) != 0) {
        changed |= true;

        memcpy(blk->dominators, new_dom, cfg->nblocks * sizeof(bool));
      }

      kit_arnfree(cfg->arena, new_dom);
    }
  }

  for (u32 b = 0; b < cfg->nblocks; b++) {
    if (b == cfg->entry_block) {
      cfg->blocks[b].idom = UINT32_MAX;
      continue;
    }

    u32 idom = UINT32_MAX;

    for (u32 d = 0; d < cfg->nblocks; d++) {
      if (d == b) continue;

      if (!cfg->blocks[b].dominators[d]) continue;

      bool closest = true;

      for (u32 e = 0; e < cfg->nblocks; e++) {
        if (e == b || e == d) continue;

        if (!cfg->blocks[b].dominators[e]) continue;

        /*
         * If e dominates d,
         * then d is not closest.
         */

        /* fuck you */
        if (cfg->blocks[e].dominators[d]) {
          closest = false;
          break;
        }
      }

      if (closest) {
        idom = d;
        break;
      }
    }

    cfg->blocks[b].idom = idom;
  }

  return 0;
}