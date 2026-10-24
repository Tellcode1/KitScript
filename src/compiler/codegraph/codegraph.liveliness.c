#include "codegraph.h"

#include "../../../inc/kit.arena.h"
#include "../../../inc/kit.cc.h"

int
codegraph_build_successor_list(kit_compiler* cc, codegraph* dst)
{
  /* label ID -> Label instruction index */
  u32* label_map = kit_arnalloc(dst->arena, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32));

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == KIT_IR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  const u32  nblocks = dst->nblocks;
  codeblock* blocks  = dst->blocks;

  const kit_ins* code      = cc->instructions;
  u32            code_size = cc->ninstructions;

  u32* ip_to_block = kit_arnalloc(dst->arena, code_size * sizeof(u32));
  for (u32 b = 0; b < nblocks; b++) {
    for (u32 ip = blocks[b].start; ip <= blocks[b].end; ip++) { ip_to_block[ip] = b; }
  }

  for (u32 i = 0; i < nblocks; i++) {
    codeblock* blk   = &blocks[i];
    blk->nsuccessors = 0; /* filling it in this loop */

    blk->uses     = NULL; /* define for later passes */
    blk->defines  = NULL;
    blk->live_in  = NULL;
    blk->live_out = NULL;

    u32            last_ins_ip = blk->end;
    const kit_ins* last_ins    = &code[last_ins_ip];

    /* jump to another code block */
    if (last_ins->opcode == KIT_IR_OPCODE_JMP) {
      u32 target_ip = label_map[last_ins->jmp.target];
      if (target_ip < code_size) {
        u32 target_block                    = ip_to_block[target_ip];
        blk->successors[blk->nsuccessors++] = target_block;
      }
    }

    else if (last_ins->opcode == KIT_IR_OPCODE_JZ || last_ins->opcode == KIT_IR_OPCODE_JNZ) {
      // fallthrough
      if (i + 1 < nblocks) blk->successors[blk->nsuccessors++] = i + 1;

      // the branch taken (if condition were to be true)
      u32 target_ip = label_map[last_ins->cj.target];
      if (target_ip < code_size) {
        u32 target_block                    = ip_to_block[target_ip];
        blk->successors[blk->nsuccessors++] = target_block;
      }
    }

    else if (last_ins->opcode == KIT_IR_OPCODE_RET) {
      // no successors ever.
    }

    /* normal instruction. fallthrough only. */
    else if ((i + 1) < nblocks) {
      blk->successors[blk->nsuccessors++] = i + 1;
    }
  }

  u32* pred_count = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  memset(pred_count, 0, nblocks * sizeof(u32));

  for (u32 b = 0; b < nblocks; b++) {
    for (u32 s = 0; s < blocks[b].nsuccessors; s++) {
      u32 succ = blocks[b].successors[s];
      if (succ < nblocks) pred_count[succ]++;
    }
  }

  for (u32 b = 0; b < nblocks; b++) {
    blocks[b].npredecessors = pred_count[b];
    if (pred_count[b] > 0) {
      blocks[b].predecessors = kit_arnalloc(cc->arena, pred_count[b] * sizeof(u32));
    } else {
      blocks[b].predecessors = NULL;
    }
  }

  u32* fill_idx = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  memset(fill_idx, 0, nblocks * sizeof(u32));

  /* fill predecessors */
  for (u32 b = 0; b < nblocks; b++) {
    for (u32 s = 0; s < blocks[b].nsuccessors; s++) {
      u32 succ = blocks[b].successors[s];
      if (succ < nblocks) { blocks[succ].predecessors[fill_idx[succ]++] = b; }
    }
  }

  return 0;
}

int
codegraph_block_level_liveliness_analysis(kit_compiler* cc, codegraph* dst)
{
  const kit_ins* code      = cc->instructions;
  u32            code_size = cc->ninstructions;

  const u32 nvregs = cc->next_vreg;
  if (nvregs == 0) return 0; /* nothing to do. */

  for (u32 i = 0; i < dst->nblocks; i++) {
    codeblock* blk = &dst->blocks[i];

    blk->uses     = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    blk->defines  = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    blk->live_in  = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    blk->live_out = kit_arnalloc(dst->arena, nvregs * sizeof(bool));

    memset(blk->defines, 0, nvregs * sizeof(bool));
    memset(blk->uses, 0, nvregs * sizeof(bool));
    memset(blk->live_in, 0, nvregs * sizeof(bool));
    memset(blk->live_out, 0, nvregs * sizeof(bool));
  }

  for (u32 i = 0; i < dst->nblocks; i++) {
    codeblock* blk = &dst->blocks[i];

    bool* defined_so_far = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    memset(defined_so_far, 0, nvregs * sizeof(bool));

    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      kit_ins ins = cc->instructions[ip];

      u32 dst_reg = get_destination_reg(&ins);

      u32 srcs[32] = { 0 };
      u32 nsrcs    = get_source_registers(&ins, srcs);

      for (u32 j = 0; j < nsrcs; j++) {
        u32 r = srcs[j];
        if (r < nvregs && !defined_so_far[r]) blk->uses[r] = true;
      }

      if (dst_reg != UINT32_MAX && dst_reg < nvregs) {
        blk->defines[dst_reg]   = true;
        defined_so_far[dst_reg] = true;
      }
    }
  }

  /* Find all registers used for returning values */
  bool* return_live = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
  for (u32 i = 0; i < code_size; i++) {
    if (code[i].opcode == KIT_IR_OPCODE_RET) return_live[code[i].ret.return_value] = true;
  }

  /* initialize live_out sets for exit blocks */
  for (u32 i = 0; i < dst->nblocks; i++) {
    codeblock* blk = &dst->blocks[i];
    if (blk->nsuccessors != 0) continue;
    for (u32 r = 0; r < nvregs; r++) {
      if (return_live[r]) { blk->live_out[r] = true; }
    }
  }

  /* compute reverse post order for this graph */
  u32   nblocks = dst->nblocks;
  bool* visited = kit_arnalloc(dst->arena, ((size_t)nblocks * 2 * +1) * sizeof(bool));

  memset(visited, 0, nblocks * sizeof(bool));

  u32* stack_trav = kit_arnalloc(dst->arena, nblocks * sizeof(u32));
  u32* stack_post = kit_arnalloc(dst->arena, nblocks * sizeof(u32));

  u32 top_trav           = 0;
  u32 top_post           = 0;
  stack_trav[top_trav++] = dst->entry_block;

  memset(visited, 0, nblocks * sizeof(bool));
  top_trav               = 0;
  top_post               = 0;
  stack_trav[top_trav++] = dst->entry_block;

  while (top_trav > 0) {
    u32 b = stack_trav[--top_trav];
    if (visited[b]) continue;
    visited[b]             = true;
    stack_post[top_post++] = b;

    codeblock* blk = &dst->blocks[b];
    for (u32 s = 0; s < blk->nsuccessors; s++) {
      u32 succ = blk->successors[s];
      if (!visited[succ]) { stack_trav[top_trav++] = succ; }
    }
  }

  u32* rpo = kit_arnalloc(dst->arena, nblocks * sizeof(u32));

  /* reverse */
  for (u32 i = 0; i < top_post; i++) { rpo[i] = stack_post[top_post - 1 - i]; }
  const u32 nrpo = top_post;

  kit_arnfree(dst->arena, visited);
  kit_arnfree(dst->arena, stack_trav);
  kit_arnfree(dst->arena, stack_post);

  bool* new_live_out = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
  bool* new_live_in  = kit_arnalloc(dst->arena, nvregs * sizeof(bool));

  bool changed = false;
  do {
    changed = false;

    for (u32 idx = 0; idx < nrpo; idx++) {
      u32        b   = rpo[idx];
      codeblock* blk = &dst->blocks[b];

      memset(new_live_out, 0, nvregs * sizeof(bool));
      memset(new_live_in, 0, nvregs * sizeof(bool));

      /* new_live_out = U live_in[successor] */
      for (u32 s = 0; s < blk->nsuccessors; s++) {
        codeblock* successor = &dst->blocks[blk->successors[s]];

        for (u32 reg = 0; reg < nvregs; reg++) {
          if (successor->live_in[reg]) { new_live_out[reg] = true; }
        }
      }

      /* new_live_in = uses U (new_live_out - definitions) */
      for (u32 reg = 0; reg < nvregs; reg++) {
        if ((blk->uses[reg]) || (new_live_out[reg] && !blk->defines[reg])) { new_live_in[reg] = true; }
      }

      if (memcmp(new_live_in, blk->live_in, nvregs * sizeof(bool)) != 0) { /* changed? */
        changed = true;
      }
      if (memcmp(new_live_out, blk->live_out, nvregs * sizeof(bool)) != 0) { /* changed? */
        changed = true;
      }

      memcpy(blk->live_in, new_live_in, nvregs * sizeof(bool));
      memcpy(blk->live_out, new_live_out, nvregs * sizeof(bool));
    }
  } while (changed);

  return 0;
}

int
codegraph_instruction_level_liveliness_analysis(kit_compiler* cc, codegraph* dst)
{
  const u32 nvregs = dst->nvregs;

  dst->ins_live_in  = (bool**)kit_arnalloc(dst->arena, cc->ninstructions * sizeof(bool*));
  dst->ins_live_out = (bool**)kit_arnalloc(dst->arena, cc->ninstructions * sizeof(bool*));

  for (u32 i = 0; i < cc->ninstructions; i++) {
    dst->ins_live_in[i]  = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    dst->ins_live_out[i] = kit_arnalloc(dst->arena, nvregs * sizeof(bool));

    memset(dst->ins_live_in[i], 0, nvregs * sizeof(bool));
    memset(dst->ins_live_out[i], 0, nvregs * sizeof(bool));
  }

  for (u32 b = 0; b < dst->nblocks; b++) {
    codeblock* blk = &dst->blocks[b];

    bool* live = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    memcpy(live, blk->live_out, nvregs * sizeof(bool));

    for (i64 ip = blk->end; ip >= (i64)blk->start; ip--) {
      /* blk->end is a valid index. */
      kit_ins* ins = &cc->instructions[ip];

      /* instruction's live_after = currently live */
      memcpy(dst->ins_live_out[ip], live, nvregs * sizeof(bool));

      /* start destination as dead */
      u32 dst_reg = get_destination_reg(ins);
      if (dst_reg != UINT32_MAX && dst_reg < nvregs) { live[dst_reg] = false; }

      u32 srcs[32];
      u32 nsrcs = get_source_registers(ins, srcs);

      /* set live to each register that is used (as source) */
      for (u32 s = 0; s < nsrcs; s++) {
        if (srcs[s] < nvregs) live[srcs[s]] = true;
      }

      /* live_before = live */
      memcpy(dst->ins_live_in[ip], live, nvregs * sizeof(bool));
    }

    kit_arnfree(dst->arena, live);
  }

  return 0;
}