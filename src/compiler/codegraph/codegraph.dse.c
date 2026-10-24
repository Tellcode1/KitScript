#include "codegraph.h"

#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.reg.h"

bool
codegraph_dead_store_elimination(kit_compiler* cc, codegraph* cfg)
{
  codegraph_build_successor_list(cc, cfg);

  bool* live = kit_arnalloc(cfg->arena, cfg->nvregs * sizeof(bool));

  bool changed = false;

  for (u32 pass = 0; pass < 4; pass++) {
    /* reparse liveliness information */
    codegraph_block_level_liveliness_analysis(cc, cfg);

    for (u32 i = 0; i < cfg->nblocks; i++) {
      codeblock* blk = &cfg->blocks[i];

      memcpy(live, blk->live_out, cfg->nvregs * sizeof(bool));

      for (i64 ip = blk->end; ip >= blk->start; ip--) {
        kit_ins* ins = &cc->instructions[ip];

        if (ins->opcode == KIT_IR_OPCODE_NOP || ins->opcode == KIT_IR_OPCODE_LABEL) continue;

        u32 dst     = UINT32_MAX;
        u32 src[32] = { 0 };
        u32 nsrc    = 0;

        dst  = get_destination_reg(ins);
        nsrc = get_source_registers(ins, src);

        /* need to mark sources for impure instructions */
        if (!is_instruction_impure(ins->opcode) && (dst != UINT32_MAX && !live[dst])) {
          /* dead store */
          ins->opcode = KIT_IR_OPCODE_NOP;
          changed     = true;
          continue;
        }

        /* mark all sources as live */
        for (u32 s = 0; s < nsrc; s++) { live[src[s]] = true; }
        if (dst != UINT32_MAX) live[dst] = false;
      }
    }
  }

  return changed;
}

bool
codegraph_redundant_move_elimination(kit_compiler* cc, codegraph* cfg)
{
  /**
   * Find the pattern:
   * mov a, x
   * mov b, a
   *
   * or loadk a
   * mov b, a -> loadk b
   *
   * Then determine whether any register relies on a
   * (and can we switch a to b?). If no one does,
   * replace the pattern with a single mov b, x
   */
  bool changed = false;

  for (u32 pass = 0; pass < 4; pass++) {
    for (u32 i = 0; i < cc->ninstructions; i++) {
      if (i + 1 >= cc->ninstructions) break;

      u32 a_idx = i;
      u32 b_idx = i + 1;

      if (a_idx == UINT32_MAX || b_idx == UINT32_MAX) break;

      kit_ins* a = &cc->instructions[a_idx];
      kit_ins* b = &cc->instructions[b_idx];

      if (b->opcode != KIT_IR_OPCODE_MOV) continue;

      u32 a_dst = get_destination_reg(a);

      if (a_dst == UINT32_MAX) continue; /* no destination */
      if (a_dst != b->mov.src) continue; /* isn't chained. */

      /* check if the first operation to a is a read (bad) or a write (good). */
      bool is_a_read_later = cfg->ins_live_out[a_dst];

      if (!is_a_read_later) {
        for (u32 j = b_idx + 1; j < cc->ninstructions; j++) {
          kit_ins* k   = &cc->instructions[j];
          u32      dst = get_destination_reg(k);

          u32 srcs[32] = { 0 };
          u32 nsrcs    = get_source_registers(k, srcs);

          /* a is read later. can't optimize :( */
          for (u32 s = 0; s < nsrcs; s++) {
            if (srcs[s] == a_dst) {
              is_a_read_later = true;

              break;
            }
          }

          if (is_a_read_later) break;

          /**
           * First operation to the register is a write
           * This means we can safely overwrite it.
           */
          if (dst == a_dst) break;
        }
      }

      if (!is_a_read_later) {
        switch (a->opcode) {
          case KIT_IR_OPCODE_MOV:
            // mov a,x  ;  mov b,a  →  mov b,x
            b->mov.src = a->mov.src;
            a->opcode  = KIT_IR_OPCODE_NOP;
            changed    = true;
            break;

          case KIT_IR_OPCODE_LOADK:
            // loadk a, id;
            // mov b, a  → loadk b, id b->opcode = KIT_IR_OPCODE_LOADK;
            b->loadk.dst = b->mov.dst; // reuse the dst slot
            b->loadk.id  = a->loadk.id;
            a->opcode    = KIT_IR_OPCODE_NOP;
            changed      = true;
            break;

          case KIT_IR_OPCODE_MOVI:
            // movi a,val ; mov b,a → movi b,val
            b->opcode     = KIT_IR_OPCODE_MOVI;
            b->movi.dst   = b->mov.dst;
            b->movi.value = a->movi.value;
            a->opcode     = KIT_IR_OPCODE_NOP;
            changed       = true;
            break;

          case KIT_IR_OPCODE_MOVF:
            // movf a,val ; mov b,a → movf b,val
            b->opcode     = KIT_IR_OPCODE_MOVF;
            b->movf.dst   = b->mov.dst;
            b->movf.value = a->movf.value;
            a->opcode     = KIT_IR_OPCODE_NOP;
            changed       = true;
            break;

          case KIT_IR_OPCODE_GETG:
            // getg a,gid ; mov b,a → getg b,gid
            b->opcode   = KIT_IR_OPCODE_GETG;
            b->getg.dst = b->mov.dst;
            b->getg.src = a->getg.src; // global id
            a->opcode   = KIT_IR_OPCODE_NOP;
            changed     = true;
            break;

            // binop dst=x a,b ; move x, a
          case KIT_IR_OPCODE_ADD:
          case KIT_IR_OPCODE_SUB:
          case KIT_IR_OPCODE_MUL:
          case KIT_IR_OPCODE_DIV:
          case KIT_IR_OPCODE_MOD:
          case KIT_IR_OPCODE_EXP:
          case KIT_IR_OPCODE_AND:
          case KIT_IR_OPCODE_OR:
          case KIT_IR_OPCODE_BAND:
          case KIT_IR_OPCODE_BOR:
          case KIT_IR_OPCODE_XOR:
          case KIT_IR_OPCODE_EQL:
          case KIT_IR_OPCODE_NEQ:
          case KIT_IR_OPCODE_LT:
          case KIT_IR_OPCODE_LTE:
          case KIT_IR_OPCODE_GT:
          case KIT_IR_OPCODE_GTE: {
            u32 dst = b->mov.dst;

            if (dst == a->binop.a || dst == a->binop.b) { continue; }

            b->opcode    = a->opcode;
            b->binop.a   = a->binop.a;
            b->binop.b   = a->binop.b;
            b->binop.dst = dst;

            a->opcode = KIT_IR_OPCODE_NOP;

            changed |= true;
            break;
          }

          default: break;
        }
      }

      /* There pairs are often produced from this pass, clean them up right here. */
      if (a->opcode == KIT_IR_OPCODE_MOV && a->mov.src == a->mov.dst) { a->opcode = KIT_IR_OPCODE_NOP; }
    }
  }

  return changed;
}

bool
codegraph_preliminary_dead_store_elimination(kit_compiler* cc, const codegraph* cfg)
{
  bool changed = false;

  for (u32 b = 0; b < cfg->nblocks; b++) {
    codeblock* blk = &cfg->blocks[b];
    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      if (ip + 1 > blk->end) break;

      kit_ins* ins  = &cc->instructions[ip];
      kit_ins* next = &cc->instructions[ip + 1];

      u32 ins_dst = get_destination_reg(ins);
      u32 nxt_dst = get_destination_reg(next);

      if (ins_dst == nxt_dst && ins_dst != UINT32_MAX && nxt_dst != UINT32_MAX) {
        /* The first write is invalidated by the next. remove the first */
        ins->opcode = KIT_IR_OPCODE_NOP;
        changed     = true;
      }
    }
  }

  return changed;
}

bool
codegraph_local_dead_store_elimination(kit_compiler* cc, codegraph* cfg)
{
  bool changed = false;

  for (u32 b = 0; b < cfg->nblocks; b++) {
    codeblock* blk = &cfg->blocks[b];

    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      kit_ins* ins = &cc->instructions[ip];
      if (ins->opcode == KIT_IR_OPCODE_NOP || ins->opcode == KIT_IR_OPCODE_LABEL) continue;

      if (is_instruction_impure(ins->opcode)) continue; /* can't modify instructions with side effects */

      u32 dst = get_destination_reg(ins);
      if (dst < KIT_REG_GENERAL_BEGIN || dst == UINT32_MAX) continue; /* can't modify non general registers | can't remove instructions with no dst */

      /* if it is not live_out to the instruction, remove it */
      if (!cfg->ins_live_out[ip][dst]) {
        changed     = true;
        ins->opcode = KIT_IR_OPCODE_NOP;
      }
    }
  }

  return changed;
}