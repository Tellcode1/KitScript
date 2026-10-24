#include "codegraph.h"

#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.reg.h"
#include "../../../inc/kit.regalloc.h"

int
era_compute_ranges(kit_compiler* cc, era_state* ra)
{
  memset(ra, 0, sizeof(*ra));

  const u32 vreg_count = cc->next_vreg;

  ra->ranges = kit_xalloc(vreg_count, sizeof(era_range));
  if (!ra->ranges) return -1;

  ra->vreg_to_phys = kit_xalloc(vreg_count, sizeof(u32));
  if (!ra->vreg_to_phys) {
    free(ra->ranges);
    ra->ranges = NULL;
    return -1;
  }

  for (u32 i = 0; i < vreg_count; i++) {
    ra->ranges[i].start = UINT32_MAX;
    ra->ranges[i].end   = 0;
    ra->ranges[i].vreg  = i;
  }

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins ins = cc->instructions[i];

    /* mark vreg is defined at this instruction */
#define WRITES_TO(r)                                                                                                                                 \
  do {                                                                                                                                               \
    u32 _r = (r);                                                                                                                                    \
    if (ra->ranges[_r].start == UINT32_MAX) ra->ranges[_r].start = i;                                                                                \
    if (i > ra->ranges[_r].end) ra->ranges[_r].end = i;                                                                                              \
  } while (0)

    /* mark vreg is used at this instruction */
#define READS_FROM(r)                                                                                                                                \
  do {                                                                                                                                               \
    u32 _r = (r);                                                                                                                                    \
    if (i > ra->ranges[_r].end) ra->ranges[_r].end = i;                                                                                              \
  } while (0)

    switch ((kit_ir_opcode_bits)ins.opcode) {
      /* getg, setg and movg  */
      case KIT_IR_OPCODE_MOV:
        WRITES_TO(ins.mov.dst);
        READS_FROM(ins.mov.src);
        break;

      case KIT_IR_OPCODE_MOVI: WRITES_TO(ins.movi.dst); break; /* values are not registers */
      case KIT_IR_OPCODE_MOVF: WRITES_TO(ins.movf.dst); break; /* values are not registers */

      case KIT_IR_OPCODE_ASSERT: READS_FROM(ins.assertion.cond); break; /* values are not registers */

      case KIT_IR_OPCODE_LOADFN: WRITES_TO(ins.loadfn.dst); break;

      case KIT_IR_OPCODE_GETG: WRITES_TO(ins.mov.dst); break;
      case KIT_IR_OPCODE_SETG: READS_FROM(ins.mov.src); break;
      case KIT_IR_OPCODE_MOVG: {
        break;
      }
      case KIT_IR_OPCODE_LOADK:
        WRITES_TO(ins.loadk.dst);
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
      case KIT_IR_OPCODE_GTE:
        WRITES_TO(ins.binop.dst);
        READS_FROM(ins.binop.a);
        READS_FROM(ins.binop.b);
        break;
      case KIT_IR_OPCODE_NOT:
      case KIT_IR_OPCODE_NEG:
      case KIT_IR_OPCODE_BNOT:
      case KIT_IR_OPCODE_DEC:
      case KIT_IR_OPCODE_INC:
        WRITES_TO(ins.unop.dst);
        READS_FROM(ins.unop.a);
        break;
      case KIT_IR_OPCODE_CALL:
        for (u32 j = KIT_REG_ARG0; j < MIN(ins.call.nargs, KIT_REG_ARG_COUNT); j++) { READS_FROM(j); }
        WRITES_TO(ins.call.dst);
        READS_FROM(ins.call.reg);
        break;
      case KIT_IR_OPCODE_INDEX:
        WRITES_TO(ins.index.dst);
        READS_FROM(ins.index.base);
        READS_FROM(ins.index.index);
        break;
      case KIT_IR_OPCODE_INDEX_ASSIGN:
        READS_FROM(ins.index_assign.value);
        READS_FROM(ins.index_assign.index);
        READS_FROM(ins.index_assign.base);
        break;
      case KIT_IR_OPCODE_MK_LIST:
        for (u32 j = KIT_REG_ARG0; j < MIN(ins.mk_list.nelems, KIT_REG_ARG_COUNT); j++) { READS_FROM(j); }
        WRITES_TO(ins.mk_list.dst);
        break;
      case KIT_IR_OPCODE_MK_MAP:
        for (u32 j = KIT_REG_ARG0; j < MIN(ins.mk_map.npairs * 2, KIT_REG_ARG_COUNT); j++) { READS_FROM(j); }
        WRITES_TO(ins.mk_map.dst);
        break;
      case KIT_IR_OPCODE_MK_STRUCT:
        /* we don't know how many members this instruction will initialize. clobber the entire argument vector. */
        for (u32 j = KIT_REG_ARG0; j < KIT_REG_ARG_COUNT; j++) { READS_FROM(j); }
        WRITES_TO(ins.mk_struct.dst);
        break;
      case KIT_IR_OPCODE_MEMBER_ACCESS:
        WRITES_TO(ins.member_access.dst);
        READS_FROM(ins.member_access.base);
        break;
      case KIT_IR_OPCODE_MEMBER_ASSIGN:
        READS_FROM(ins.member_assign.value);
        READS_FROM(ins.member_assign.base);
        break;
      case KIT_IR_OPCODE_RET: READS_FROM(ins.ret.return_value); break;
      case KIT_IR_OPCODE_JZ:
      case KIT_IR_OPCODE_JNZ: READS_FROM(ins.cj.condition); break;
      case KIT_IR_OPCODE_PUSH: {
        READS_FROM(ins.push.reg);
        break;
      }
      case KIT_IR_OPCODE_POP: {
        WRITES_TO(ins.pop.reg);
        break;
      }

        // default: break;
      case KIT_IR_OPCODE_NOP:
      case KIT_IR_OPCODE_LABEL:
      case KIT_IR_OPCODE_JMP: {
        break;
      }
    }
#undef WRITES_TO
#undef READS_FROM
  }

  // collect only ranges that were actually used
  ra->nranges = 0;
  for (u32 i = 0; i < vreg_count; i++) {
    if (ra->ranges[i].start != UINT32_MAX) { ra->ranges[ra->nranges++] = ra->ranges[i]; }
  }

  return 0;
}

static int
era_cmp_start(const void* a, const void* b)
{ return (int)((const era_range*)a)->start - (int)((const era_range*)b)->start; }
// int
// era_cmp_end(const void* a, const void* b)
// { return (int)((const era_range*)a)->end - (int)((const era_range*)b)->end; }

int
era_allocate(int opt_level, era_state* ra)
{
  qsort(ra->ranges, ra->nranges, sizeof(era_range), era_cmp_start);

  bool phys_free[ERA_NUM_PHYS];
  memset(phys_free, 1, sizeof(phys_free));

  /* mark non general registers as always in use */
  if (opt_level >= 1) {
    /* OPTIMIZATION: Allow usage of non general registers (they're marked free). */
  } else {
    for (u32 i = 0; i < KIT_REG_GENERAL_BEGIN; i++) { phys_free[i] = false; }
  }

  phys_free[ERA_SPILL_SCRATCH] = false;

  era_range* active[ERA_NUM_PHYS] = { 0 };
  u32        nactive              = 0;
  u32        next_spill           = 0;

  for (u32 i = 0; i < ra->nranges; i++) {
    era_range* r = &ra->ranges[i];

    // expire intervals that ended before this one starts
    u32 new_nactive = 0;
    for (u32 j = 0; j < nactive; j++) {
      if (active[j]->end < r->start) {
        phys_free[active[j]->phys] = true;
      } else {
        active[new_nactive++] = active[j];
      }
    }
    nactive = new_nactive;

    // find a free physical register
    u32 phys = UINT32_MAX;

    /* skip non general registers */
    for (u32 p = KIT_REG_GENERAL_BEGIN; p < ERA_NUM_PHYS; p++) {
      if (phys_free[p]) {
        phys = p;
        break;
      }
    }

    // need spill?
    if (phys == UINT32_MAX) {
      // find active interval with furthest end
      u32 spill_idx = 0;
      for (u32 j = 1; j < nactive; j++) {
        if (active[j]->end > active[spill_idx]->end) spill_idx = j;
      }
      era_range* spill = active[spill_idx];

      if (spill->end > r->end) {
        r->phys             = spill->phys;
        spill->spill_offset = next_spill * 8;
        spill->phys         = ERA_SPILL_FLAG | next_spill++;
        active[spill_idx]   = r; // replace
      } else {
        // r itself gets spilled
        r->spill_offset           = next_spill * 8;
        r->phys                   = ERA_SPILL_FLAG | next_spill++;
        ra->vreg_to_phys[r->vreg] = r->phys;
        continue;
      }
    } else {
      phys_free[phys] = false;
      r->phys         = phys;
      // insert into active sorted by end
      u32 ins_pos = nactive++;
      while (ins_pos > 0 && active[ins_pos - 1]->end > r->end) {
        active[ins_pos] = active[ins_pos - 1];
        ins_pos--;
      }
      active[ins_pos] = r;
    }

    ra->vreg_to_phys[r->vreg] = r->phys;
  }

  return 0;
}

int
era_rewrite(kit_compiler* cc, const u32* vreg_to_phys)
{
#define MAP(r)                                                                                                                                       \
  do {                                                                                                                                               \
    u32 _r = (r);                                                                                                                                    \
    /*                                                                                                                                               \
    if (_r & ERA_SPILL_FLAG) {                                                                                                                       \
      fprintf(stderr, "Spilled register %u not rewritten\n", r);                                                                                     \
      (r) = KIT_REG_NIL;                                                                                                                             \
    }                                                                                                                                                \
      */                                                                                                                                             \
    if (_r < KIT_REG_GENERAL_BEGIN) break;                                                                                                           \
    if (!(_r & ERA_SPILL_FLAG)) (r) = vreg_to_phys[_r];                                                                                              \
  } while (0)

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins ins = cc->instructions[i];

    switch ((kit_ir_opcode_bits)ins.opcode) {
      case KIT_IR_OPCODE_MOV:
        MAP(ins.mov.dst);
        MAP(ins.mov.src);
        break;

      case KIT_IR_OPCODE_MOVI: MAP(ins.movi.dst); break;
      case KIT_IR_OPCODE_MOVF: MAP(ins.movf.dst); break;

      case KIT_IR_OPCODE_LOADFN: MAP(ins.loadfn.dst); break;

      case KIT_IR_OPCODE_PUSH:
      case KIT_IR_OPCODE_POP: {
        MAP(ins.push.reg);
        break;
      }

      case KIT_IR_OPCODE_ASSERT: {
        MAP(ins.assertion.cond);
        break;
      }

      case KIT_IR_OPCODE_LOADK: MAP(ins.loadk.dst); break;

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
        MAP(ins.binop.dst);
        MAP(ins.binop.a);
        MAP(ins.binop.b);
        break;
      case KIT_IR_OPCODE_NOT:
      case KIT_IR_OPCODE_NEG:
      case KIT_IR_OPCODE_BNOT:
      case KIT_IR_OPCODE_INC:
      case KIT_IR_OPCODE_DEC:
        MAP(ins.unop.dst);
        MAP(ins.unop.a);
        break;
      case KIT_IR_OPCODE_CALL:
        MAP(ins.call.dst);
        MAP(ins.call.reg);
        break;
      case KIT_IR_OPCODE_INDEX:
        MAP(ins.index.dst);
        MAP(ins.index.base);
        MAP(ins.index.index);
        break;
      case KIT_IR_OPCODE_INDEX_ASSIGN:
        MAP(ins.index_assign.value);
        MAP(ins.index_assign.base);
        MAP(ins.index_assign.index);
        break;
      case KIT_IR_OPCODE_MEMBER_ACCESS:
        MAP(ins.member_access.dst);
        MAP(ins.member_access.base);
        break;
      case KIT_IR_OPCODE_MEMBER_ASSIGN:
        MAP(ins.member_assign.value);
        MAP(ins.member_assign.base);
        break;
      case KIT_IR_OPCODE_RET: MAP(ins.ret.return_value); break;
      case KIT_IR_OPCODE_JZ:

      case KIT_IR_OPCODE_JNZ: MAP(ins.cj.condition); break;
      case KIT_IR_OPCODE_GETG: MAP(ins.getg.dst); break;
      case KIT_IR_OPCODE_SETG: MAP(ins.setg.src); break;

      case KIT_IR_OPCODE_MK_LIST: MAP(ins.mk_list.dst); break;
      case KIT_IR_OPCODE_MK_MAP: MAP(ins.mk_map.dst); break;
      case KIT_IR_OPCODE_MK_STRUCT:
        MAP(ins.mk_struct.dst);
        break;

        // default: break;

      case KIT_IR_OPCODE_NOP:
      case KIT_IR_OPCODE_MOVG:
      case KIT_IR_OPCODE_LABEL:
      case KIT_IR_OPCODE_JMP: break;
    }

    // write the patched instruction back to the same location
    cc->instructions[i] = ins;
  }
#undef MAP

  return 0;
}

int
era_compute_ranges_from_cfg(kit_compiler* cc, era_state* ra)
{
  kit_arena cfg_arena;
  if (kit_arena_init(1, &cfg_arena) < 0) return -1;

  codegraph cfg = { 0 };
  if (codegraph_init(&cfg_arena, cc, &cfg) < 0) goto die;
  if (codegraph_build_successor_list(cc, &cfg) < 0) goto die;
  if (codegraph_block_level_liveliness_analysis(cc, &cfg) < 0) goto die;

  const u32 nvregs = cc->next_vreg;

  for (u32 b = 0; b < cfg.nblocks; b++) {
    codeblock* blk          = &cfg.blocks[b];
    u32        block_end_ip = blk->end;
    for (u32 r = 0; r < nvregs; r++) {
      if (blk->live_out[r] && ra->ranges[r].end < block_end_ip) { ra->ranges[r].end = block_end_ip; }
    }
  }

  for (i64 b = cfg.nblocks - 1; b >= 0; b--) {
    codeblock* blk = &cfg.blocks[b];

    bool* live = kit_arnalloc(&cfg_arena, nvregs * sizeof(bool));
    if (!live) goto die;

    memcpy(live, blk->live_out, nvregs * sizeof(bool));

    for (i64 ip = blk->end; ip >= (i64)blk->start; ip--) {
      kit_ins* ins = &cc->instructions[ip];

      u32 srcs[32];
      u32 nsrcs = get_source_registers(ins, srcs);
      for (u32 s = 0; s < nsrcs; s++) {
        u32 r = srcs[s];
        if (r < nvregs && !live[r]) {
          live[r] = true;
          if (ra->ranges[r].start > (u32)ip) ra->ranges[r].start = (u32)ip;
        }
      }

      u32 dst = get_destination_reg(ins);
      if (dst < nvregs) live[dst] = false;
    }

    kit_arnfree(&cfg_arena, live);
  }

  u32* label_map = kit_arnalloc(cfg.arena, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32));

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == KIT_IR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  /* If theres a loop, extend all registers to the end of the jump */
  for (u32 i = 0; i < cc->ninstructions; i++) {
    const kit_ins* ins = &cc->instructions[i];
    if (ins->opcode != KIT_IR_OPCODE_JMP && ins->opcode != KIT_IR_OPCODE_JZ && ins->opcode != KIT_IR_OPCODE_JNZ) continue;

    u32 target = label_map[ins->opcode == KIT_IR_OPCODE_JMP ? ins->jmp.target : ins->cj.target];
    if (i > target) {
      for (u32 r = 0; r < cc->next_vreg; r++) {
        if (ra->ranges[r].start != UINT32_MAX && ra->ranges[r].end >= target // currently reaches into the loop
            && ra->ranges[r].end < i) {                                      // but doesn't reach the back-edge yet
          ra->ranges[r].end = i + 1;
        }
      }
    }
  }

  kit_arena_free(&cfg_arena);
  return 0;

die:
  kit_arena_free(&cfg_arena);
  return -1;
}

int
era_register_allocation_pass(struct kit_compiler* cc)
{
  era_state state;
  int       e = era_compute_ranges(cc, &state);
  if (e < 0) return e;

  e = era_compute_ranges_from_cfg(cc, &state);
  if (e < 0) {
    free(state.ranges);
    free(state.vreg_to_phys);
    return e;
  }

  e = era_allocate(cc->info->opt_level, &state);
  if (e < 0) {
    free(state.ranges);
    free(state.vreg_to_phys);
    return e;
  }

  e = era_rewrite(cc, state.vreg_to_phys);
  free(state.ranges);
  free(state.vreg_to_phys);
  return e;
}