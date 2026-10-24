#include "codegraph.h"

#include "../../../inc/kit.arena.h"
#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.ir.h"
#include "../../../inc/kit.reg.h"

ATTR_NODISCARD int
codegraph_init(struct kit_arena* a, struct kit_compiler* cc, codegraph* dst)
{
  const kit_ins* code      = cc->instructions;
  u32            code_size = cc->ninstructions;

  const u32 nvregs = cc->next_vreg;

  u32* label_map = kit_arnalloc(a, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32));

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == KIT_IR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  /* Find all block headers */
  bool* is_blk_header = kit_arnalloc(a, code_size * sizeof(bool));
  memset(is_blk_header, 0, code_size * sizeof(bool));

  /* first instruction */
  is_blk_header[0] = true;

  for (u32 i = 0; i < code_size; i++) {
    const kit_ins* ins = &code[i];

    if (ins->opcode == KIT_IR_OPCODE_JMP) {
      u32 target = label_map[ins->jmp.target];
      if (target < cc->ninstructions) { is_blk_header[target] = true; }
      if (i + 1 < code_size) is_blk_header[i + 1] = true;
    } else if (ins->opcode == KIT_IR_OPCODE_JZ || ins->opcode == KIT_IR_OPCODE_JNZ) {
      u32 target = label_map[ins->cj.target];

      if (target < code_size) is_blk_header[target] = true;
      if (i + 1 < code_size) is_blk_header[i + 1] = true; // fallthrough
    } else if (ins->opcode == KIT_IR_OPCODE_RET && i + 1 < code_size) {
      is_blk_header[i + 1] = true;
    }
  }

  // kit_arnfree(a, label_map);

  /* Divide the code into blocks using our block headers */
  codeblock* blocks  = kit_arnalloc(a, sizeof(codeblock) * code_size);
  size_t     nblocks = 0;

  u32 block_start = 0;
  for (u32 i = 0; i < code_size; i++) {
    if (is_blk_header[i] && i != 0) {
      /* Block starts from 0, it's the entry point */
      if (block_start == 0) { dst->entry_block = nblocks; }

      blocks[nblocks].start = block_start;
      blocks[nblocks].end   = i - 1;
      nblocks++;
      block_start = i;
    }
  }

  // last block
  blocks[nblocks].start = block_start;
  blocks[nblocks].end   = code_size - 1;
  nblocks++;

  dst->blocks  = blocks;
  dst->nblocks = nblocks;
  dst->nvregs  = nvregs;
  dst->arena   = a;

  return 0;
}

void
codegraph_free(kit_compiler* cc, codegraph* graph)
{
}

int
codegraph_rebuild(kit_compiler* cc, codegraph* cfg)
{
  kit_arena* arena = cfg->arena;
  if (kit_arena_reset(arena) < 0) return -1;
  memset(cfg, 0, sizeof(*cfg));
  cfg->arena = arena;

  int e = codegraph_init(arena, cc, cfg);
  if (e < 0) return e;

  e = codegraph_build_successor_list(cc, cfg);
  if (e < 0) return e;

  e = codegraph_block_level_liveliness_analysis(cc, cfg);
  if (e < 0) return e;

  e = codegraph_instruction_level_liveliness_analysis(cc, cfg);
  if (e < 0) return e;

  e = codegraph_domination_analysis(cc, cfg);
  if (e < 0) return e;

  e = codegraph_loop_analysis(cc, cfg);
  if (e < 0) return e;

  return 0;
}

u32
get_destination_reg(const kit_ins* i)
{
  switch ((kit_ir_opcode_bits)i->opcode) {
      /* getg, setg and movg  */
    case KIT_IR_OPCODE_MOV: return i->mov.dst; break;

    case KIT_IR_OPCODE_MOVI: return i->movi.dst; break; /* values are not registers */
    case KIT_IR_OPCODE_MOVF: return i->movf.dst; break; /* values are not registers */

    case KIT_IR_OPCODE_GETG: return i->getg.dst; break;

    case KIT_IR_OPCODE_LOADFN: return i->loadfn.dst; break;

    case KIT_IR_OPCODE_ASSERT:
    case KIT_IR_OPCODE_SETG:
    case KIT_IR_OPCODE_MOVG: {
      break;
    }
    case KIT_IR_OPCODE_LOADK:
      return i->loadk.dst;
      break; /* id is not a register */

    // case KIT_IR_OPCODE_LOADK: break;
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
    case KIT_IR_OPCODE_GTE: return i->binop.dst; break;
    case KIT_IR_OPCODE_NOT:
    case KIT_IR_OPCODE_NEG:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_DEC:
    case KIT_IR_OPCODE_INC: return i->unop.dst; break;
    case KIT_IR_OPCODE_CALL: return i->call.dst; break;
    case KIT_IR_OPCODE_INDEX: return i->index.dst; break;
    case KIT_IR_OPCODE_INDEX_ASSIGN: break;
    case KIT_IR_OPCODE_MK_LIST: return i->mk_list.dst; break;
    case KIT_IR_OPCODE_MK_MAP: return i->mk_map.dst; break;
    case KIT_IR_OPCODE_MK_STRUCT: return i->mk_struct.dst; break;
    case KIT_IR_OPCODE_MEMBER_ACCESS: return i->member_access.dst; break;
    case KIT_IR_OPCODE_MEMBER_ASSIGN: break;
    case KIT_IR_OPCODE_RET:
    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ:
    case KIT_IR_OPCODE_PUSH: {
      break;
    }
    case KIT_IR_OPCODE_POP: {
      return i->pop.reg;
    }

      // default: break;
    case KIT_IR_OPCODE_NOP:
    case KIT_IR_OPCODE_LABEL:
    case KIT_IR_OPCODE_JMP: {
      break;
    }
  }

  return UINT32_MAX;
}

void
set_destination_reg(kit_ins* i, u32 value)
{
  switch ((kit_ir_opcode_bits)i->opcode) {
      /* getg, setg and movg  */
    case KIT_IR_OPCODE_MOV: i->mov.dst = value; break;

    case KIT_IR_OPCODE_MOVI: i->movi.dst = value; break; /* values are not registers */
    case KIT_IR_OPCODE_MOVF: i->movf.dst = value; break; /* values are not registers */

    case KIT_IR_OPCODE_GETG: i->getg.dst = value; break;

    case KIT_IR_OPCODE_LOADFN: i->loadfn.dst = value; break;

    case KIT_IR_OPCODE_ASSERT:
    case KIT_IR_OPCODE_SETG:
    case KIT_IR_OPCODE_MOVG: {
      break;
    }
    case KIT_IR_OPCODE_LOADK:
      i->loadk.dst = value;
      break; /* id is not a register */

    // case KIT_IR_OPCODE_LOADK: break;
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
    case KIT_IR_OPCODE_GTE: i->binop.dst = value; break;
    case KIT_IR_OPCODE_NOT:
    case KIT_IR_OPCODE_NEG:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_DEC:
    case KIT_IR_OPCODE_INC: i->unop.dst = value; break;
    case KIT_IR_OPCODE_CALL: i->call.dst = value; break;
    case KIT_IR_OPCODE_INDEX: i->index.dst = value; break;
    case KIT_IR_OPCODE_INDEX_ASSIGN: break;
    case KIT_IR_OPCODE_MK_LIST: i->mk_list.dst = value; break;
    case KIT_IR_OPCODE_MK_MAP: i->mk_map.dst = value; break;
    case KIT_IR_OPCODE_MK_STRUCT: i->mk_struct.dst = value; break;
    case KIT_IR_OPCODE_MEMBER_ACCESS: i->member_access.dst = value; break;
    case KIT_IR_OPCODE_MEMBER_ASSIGN: break;
    case KIT_IR_OPCODE_RET:
    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ:
    case KIT_IR_OPCODE_PUSH: {
      break;
    }
    case KIT_IR_OPCODE_POP: {
      i->pop.reg = value;
      break;
    }

      // default: break;
    case KIT_IR_OPCODE_NOP:
    case KIT_IR_OPCODE_LABEL:
    case KIT_IR_OPCODE_JMP: {
      break;
    }
  }
}

u32
get_source_registers(const kit_ins* i, u32 sources[32])
{
  u32 n = 0;
  switch ((kit_ir_opcode_bits)i->opcode) {
      /* getg, setg and movg  */
    case KIT_IR_OPCODE_MOV: sources[n++] = i->mov.src; break;

    case KIT_IR_OPCODE_ASSERT: sources[n++] = i->assertion.cond; break;

    case KIT_IR_OPCODE_LOADFN:
    case KIT_IR_OPCODE_MOVI:
    case KIT_IR_OPCODE_MOVF: /* values are not registers */
    case KIT_IR_OPCODE_GETG: break;

    case KIT_IR_OPCODE_SETG: sources[n++] = i->mov.src; break;

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
      sources[n++] = i->binop.a;
      sources[n++] = i->binop.b;
      break;

    case KIT_IR_OPCODE_NOT:
    case KIT_IR_OPCODE_NEG:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_DEC:
    case KIT_IR_OPCODE_INC: sources[n++] = i->unop.a; break;

    case KIT_IR_OPCODE_MK_LIST:
      for (u32 j = KIT_REG_ARG0; j < MIN(KIT_REG_ARG_COUNT, i->mk_list.nelems); j++) { sources[n++] = j; }
      break;

    case KIT_IR_OPCODE_MK_MAP:
      for (u32 j = KIT_REG_ARG0; j < MIN(KIT_REG_ARG_COUNT, i->mk_map.npairs * 2); j++) { sources[n++] = j; }
      break;

    case KIT_IR_OPCODE_MK_STRUCT:
      /* finding out which argument registers to clear will take some time. just kill the entire vector. */
      for (u32 j = KIT_REG_ARG0; j < KIT_REG_ARG_COUNT; j++) { sources[n++] = j; }
      break;
    case KIT_IR_OPCODE_CALL:
      /* record only the argument registers that were used */
      for (u32 j = KIT_REG_ARG0; j < MIN(KIT_REG_ARG_COUNT, i->call.nargs); j++) { sources[n++] = j; }
      sources[n++] = i->call.reg;
      break;

    case KIT_IR_OPCODE_INDEX:
      sources[n++] = i->index.base;
      sources[n++] = i->index.index;
      break;
    case KIT_IR_OPCODE_INDEX_ASSIGN:
      sources[n++] = i->index_assign.value;
      sources[n++] = i->index_assign.index;
      sources[n++] = i->index_assign.base;
      break;

    case KIT_IR_OPCODE_MEMBER_ACCESS: sources[n++] = i->member_access.base; break;
    case KIT_IR_OPCODE_MEMBER_ASSIGN:
      sources[n++] = i->member_assign.value;
      sources[n++] = i->member_assign.base;
      break;
    case KIT_IR_OPCODE_RET: sources[n++] = i->ret.return_value; break;
    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ: sources[n++] = i->cj.condition; break;
    case KIT_IR_OPCODE_PUSH: {
      sources[n++] = i->push.reg;
      break;
    }
    // default: break;
    case KIT_IR_OPCODE_POP:
    case KIT_IR_OPCODE_NOP:
    case KIT_IR_OPCODE_LABEL:
    case KIT_IR_OPCODE_JMP: {
      break;
    }
  }

  return n;
}

u32
next_real_ins(kit_compiler* cc, u32 start)
{
  for (u32 i = start; i < cc->ninstructions; i++) {
    kit_ir_opcode op = cc->instructions[i].opcode;
    if (op != KIT_IR_OPCODE_NOP && op != KIT_IR_OPCODE_LABEL) return i;
  }
  return UINT32_MAX;
}

bool
is_instruction_impure(kit_ir_opcode op)
{
  switch (op) {
    case KIT_IR_OPCODE_CALL:
    case KIT_IR_OPCODE_PUSH:
    case KIT_IR_OPCODE_POP:
    case KIT_IR_OPCODE_RET:
    case KIT_IR_OPCODE_JMP:
    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ:
    case KIT_IR_OPCODE_LABEL:  // keep labels for control flow
    case KIT_IR_OPCODE_SETG:   // writes global state
    case KIT_IR_OPCODE_ASSERT: /* can crash the program */
    case KIT_IR_OPCODE_MOVG:
    case KIT_IR_OPCODE_INDEX_ASSIGN:
    case KIT_IR_OPCODE_MEMBER_ASSIGN:
    case KIT_IR_OPCODE_MK_LIST:
    case KIT_IR_OPCODE_MK_MAP:
    case KIT_IR_OPCODE_MK_STRUCT: return true;
    default: return false;
  }
}

bool
is_instruction_noop(kit_ir_opcode opcode)
{
  switch (opcode) {
    case KIT_IR_OPCODE_NOP:
    case KIT_IR_OPCODE_LABEL: return true;
    default: return false;
  }
}
