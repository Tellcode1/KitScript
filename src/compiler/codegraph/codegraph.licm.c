#include "codegraph.h"

#include "../../../inc/kit.cc.h"

static void
shift_and_insert_instruction(kit_compiler* cc, u32 idx, const kit_ins* ins)
{
  if (cc->ninstructions + 1 > cc->cinstructions) {
    u32      new_cap = cc->cinstructions * 2;
    kit_ins* new_ins = realloc(cc->instructions, sizeof(kit_ins) * new_cap);

    cc->instructions  = new_ins;
    cc->cinstructions = new_cap;
  }

  /* Shift right from idx */
  memmove(&cc->instructions[idx + 1], &cc->instructions[idx], (cc->ninstructions - idx) * sizeof(kit_ins));
  cc->instructions[idx] = *ins;
  cc->ninstructions++;
}

bool
codegraph_loop_invariant_code_motion(kit_compiler* cc, codegraph* cfg)
{
  return false;
  bool changed_ever = false;

  while (true) {
    bool changed = false;

    const u32 nvregs        = cc->next_vreg;
    const u32 ninstructions = cc->ninstructions;

    u32* def_count = kit_arnalloc(cfg->arena, sizeof(u32) * nvregs);
    u32* def_ip    = kit_arnalloc(cfg->arena, sizeof(u32) * nvregs);

    memset(def_count, 0, sizeof(u32) * nvregs);
    for (u32 i = 0; i < nvregs; i++) def_ip[i] = UINT32_MAX;

    for (u32 ip = 0; ip < ninstructions; ip++) {
      u32 dst = get_destination_reg(&cc->instructions[ip]);
      if (dst == UINT32_MAX || dst >= nvregs) continue;
      def_count[dst]++;

      if (def_count[dst] == 1) def_ip[dst] = ip;
      else def_ip[dst] = UINT32_MAX;
    }

    for (u32 l = 0; l < cfg->nloops; l++) {
      blockloop* loop = &cfg->loops[l];

      u32 header    = loop->header;
      u32 preheader = cfg->blocks[header].idom;

      if (preheader == UINT32_MAX) continue;

      /* idom is itself inside the loop :( */
      if (loop->is_block_member[preheader]) continue;

      u32  exit_block_count = 0;
      u32* exit_blocks      = kit_arnalloc(cfg->arena, sizeof(u32) * cfg->nblocks);

      for (u32 b = 0; b < cfg->nblocks; b++) {
        if (!loop->is_block_member[b]) continue;
        codeblock* blk = &cfg->blocks[b];
        for (u32 s = 0; s < blk->nsuccessors; s++) {
          if (!loop->is_block_member[blk->successors[s]]) {
            exit_blocks[exit_block_count++] = b;
            break;
          }
        }
      }

      bool* is_invariant = kit_arnalloc(cfg->arena, sizeof(bool) * ninstructions);
      memset(is_invariant, 0, sizeof(bool) * ninstructions);

      bool local_changed;
      do {
        local_changed = false;

        for (u32 ip = 0; ip < ninstructions; ip++) {
          if (!loop->is_ins_member[ip]) continue;
          if (is_invariant[ip]) continue;

          kit_ins* ins = &cc->instructions[ip];

          /* don't hoist if dst is defined more than once */
          u32 dst = get_destination_reg(ins);
          if (dst != UINT32_MAX && dst < nvregs && def_count[dst] > 1) continue;
          // if (dst != UINT32_MAX && dst < nvregs && cfg->blocks[header].live_in[dst]) continue;

          /* Skip anything that must not be hoisted. */
          if (is_instruction_impure(ins->opcode)) continue;
          if (is_instruction_noop(ins->opcode)) continue; /* me after hoisting all my noops to outside the loop :> */

          switch (ins->opcode) {
            case KIT_IR_OPCODE_LABEL:
            case KIT_IR_OPCODE_LOADFN:
            case KIT_IR_OPCODE_JMP:
            case KIT_IR_OPCODE_JZ:
            case KIT_IR_OPCODE_JNZ:
            case KIT_IR_OPCODE_RET: continue;
            default: break;
          }

          u32 ins_block = UINT32_MAX;
          for (u32 b = 0; b < cfg->nblocks; b++) {
            if (ip >= cfg->blocks[b].start && ip <= cfg->blocks[b].end) {
              ins_block = b;
              break;
            }
          }
          if (ins_block == UINT32_MAX) continue;

          u32  srcs[32];
          u32  nsrcs = get_source_registers(ins, srcs);
          bool ok    = true;

          for (u32 s = 0; s < nsrcs; s++) {
            u32 r = srcs[s];
            if (r >= nvregs) continue;

            u32 dip = def_ip[r];

            if (dip == UINT32_MAX) {
              if (def_count[r] > 1) {
                ok = false;
                break;
              }
              continue;
            }

            /* Defined outside the loop so it is invariant. */
            if (!loop->is_ins_member[dip]) continue;

            /* Defined inside the loop by an invariant. */
            if (is_invariant[dip]) continue;

            /* Defined inside the loop by a variant, continue to search to prove its invariant. */
            ok = false;
            break;
          }

          if (!ok) continue;

          is_invariant[ip] = true;
          local_changed    = true;
        }
      } while (local_changed);

      u32* hoist_ips = kit_arnalloc(cfg->arena, sizeof(u32) * ninstructions);
      u32  nhoist    = 0;

      for (u32 ip = 0; ip < ninstructions; ip++) {
        if (is_invariant[ip]) hoist_ips[nhoist++] = ip;
      }

      if (nhoist == 0) continue;

      /* hoist the instructions */

      /* we know which instructions we're going to hoist, copy them over to an internal array and then actually hoist them */
      kit_ins* saved = kit_arnalloc(cfg->arena, sizeof(kit_ins) * nhoist);
      for (u32 h = 0; h < nhoist; h++) {
        saved[h]                              = cc->instructions[hoist_ips[h]];
        cc->instructions[hoist_ips[h]].opcode = KIT_IR_OPCODE_NOP;
      }

      codeblock* pre       = &cfg->blocks[preheader];
      u32        insert_ip = pre->end;

      /* find the terminator. */
      while (insert_ip > pre->start && cc->instructions[insert_ip].opcode == KIT_IR_OPCODE_NOP) insert_ip--;

      for (u32 h = 0; h < nhoist; h++) {
        shift_and_insert_instruction(cc, insert_ip, &saved[h]);
        insert_ip++;
      }

      changed = true;

      /* modified the IR */
      codegraph_rebuild(cc, cfg);

      break;
    }

    if (!changed) break;
    changed_ever = true;
  }

  return changed_ever;
}
