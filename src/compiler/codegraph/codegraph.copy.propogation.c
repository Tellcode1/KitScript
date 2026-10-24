#include "codegraph.h"

#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.reg.h"

static void
invalidate_copies(u32* copy_map, u32 nregs, u32 reg)
{
  copy_map[reg] = reg;
  for (u32 r = 0; r < nregs; r++) {
    if (copy_map[r] == reg) copy_map[r] = r;
  }
}

bool
codegraph_local_copy_propagation(kit_compiler* cc, codegraph* cfg)
{
  bool changed = false;

  u32* copy_map = kit_arnalloc(cfg->arena, cc->next_vreg * sizeof(u32));
  for (u32 b = 0; b < cfg->nblocks; b++) {
    codeblock* blk = &cfg->blocks[b];

    for (u32 r = 0; r < cc->next_vreg; r++) copy_map[r] = r;

    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      kit_ins* ins = &cc->instructions[ip];
      if (ins->opcode == KIT_IR_OPCODE_NOP || ins->opcode == KIT_IR_OPCODE_LABEL) continue;

      switch ((kit_ir_opcode_bits)ins->opcode) {
          /* getg, setg and movg  */
        case KIT_IR_OPCODE_MOV:
          ins->mov.src = copy_map[ins->mov.src];
          changed      = true;
          break;

        case KIT_IR_OPCODE_ASSERT:
          ins->assertion.cond = copy_map[ins->assertion.cond];
          changed             = true;
          break;

        case KIT_IR_OPCODE_LOADFN:
        case KIT_IR_OPCODE_MOVI:
        case KIT_IR_OPCODE_MOVF: /* values are not registers */
        case KIT_IR_OPCODE_GETG: break;

        case KIT_IR_OPCODE_SETG:
          ins->mov.src = copy_map[ins->mov.src];
          changed      = true;
          break;

        case KIT_IR_OPCODE_MOVG:
        case KIT_IR_OPCODE_LOADK: /* id is not a register */ break;

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
        case KIT_IR_OPCODE_GTE:
          if (copy_map[ins->binop.a] != ins->binop.a || copy_map[ins->binop.b] != ins->binop.b) { changed = true; }
          ins->binop.a = copy_map[ins->binop.a];
          ins->binop.b = copy_map[ins->binop.b];
          break;

        case KIT_IR_OPCODE_NOT:
        case KIT_IR_OPCODE_NEG:
        case KIT_IR_OPCODE_BNOT:
        case KIT_IR_OPCODE_DEC:
        case KIT_IR_OPCODE_INC:
          if (copy_map[ins->unop.a] != ins->binop.a) { changed = true; }
          ins->unop.a = copy_map[ins->unop.a];
          changed     = true;
          break;

        /* can't rewrite argument vector */
        case KIT_IR_OPCODE_MK_LIST:
        case KIT_IR_OPCODE_MK_MAP:
        case KIT_IR_OPCODE_MK_STRUCT:
        case KIT_IR_OPCODE_CALL: break;

        case KIT_IR_OPCODE_INDEX:
          if (ins->index.index != copy_map[ins->index.index] || ins->index.base != copy_map[ins->index.base]) { changed = true; }

          ins->index.base  = copy_map[ins->index.base];
          ins->index.index = copy_map[ins->index.index];
          break;
        case KIT_IR_OPCODE_INDEX_ASSIGN:
          if (ins->index_assign.value != copy_map[ins->index_assign.value] || ins->index_assign.index != copy_map[ins->index_assign.index]
              || ins->index_assign.base != copy_map[ins->index_assign.base]) {
            changed |= true;
          }

          ins->index_assign.value = copy_map[ins->index_assign.value];
          ins->index_assign.index = copy_map[ins->index_assign.index];
          ins->index_assign.base  = copy_map[ins->index_assign.base];
          break;

        case KIT_IR_OPCODE_MEMBER_ACCESS:
          ins->member_access.base = copy_map[ins->member_access.base];
          changed                 = true;
          break;
        case KIT_IR_OPCODE_MEMBER_ASSIGN:
          ins->member_assign.value = copy_map[ins->member_assign.value];
          ins->member_assign.base  = copy_map[ins->member_assign.base];
          changed                  = true;
          break;
        case KIT_IR_OPCODE_RET:
          ins->ret.return_value = copy_map[ins->ret.return_value];
          changed               = true;
          break;
        case KIT_IR_OPCODE_JZ:
        case KIT_IR_OPCODE_JNZ:
          ins->cj.condition = copy_map[ins->cj.condition];
          changed           = true;
          break;
        case KIT_IR_OPCODE_PUSH: {
          ins->push.reg = copy_map[ins->push.reg];
          changed       = true;
          break;
        }

        case KIT_IR_OPCODE_POP:
        case KIT_IR_OPCODE_NOP:
        case KIT_IR_OPCODE_LABEL:
        case KIT_IR_OPCODE_JMP: {
          break;
        }
      }

      u32 dst = get_destination_reg(ins);
      if (dst != UINT32_MAX) {
        if (ins->opcode == KIT_IR_OPCODE_MOV) {
          u32 src = ins->mov.src;

          if (dst == src) {
            ins->opcode = KIT_IR_OPCODE_NOP;
            changed     = true;
          } else {
            invalidate_copies(copy_map, cc->next_vreg, dst);
            if (src >= KIT_REG_GENERAL_BEGIN) copy_map[dst] = src;
            changed = true;
          }
        } else {
          /* kill the old value */
          invalidate_copies(copy_map, cc->next_vreg, dst);
        }
      }
    }
  }

  // kit_arnfree(cfg->arena, copy_map);
  return changed;
}