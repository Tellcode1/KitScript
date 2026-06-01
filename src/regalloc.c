#include "../inc/regalloc.h"

#include "../inc/ir.h"
#include "../inc/reg.h"

#include <stdlib.h>
#include <string.h>

static int
era_compute_ranges(e_compiler* cc, era_state* ra)
{
  memset(ra, 0, sizeof(*ra));

  const u32 vreg_count = cc->next_vreg;

  ra->ranges = e_xalloc(vreg_count, sizeof(era_range));
  if (!ra->ranges) return -1;

  ra->vreg_to_phys = e_xalloc(vreg_count, sizeof(u32));
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
    e_ins ins = cc->instructions[i];

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

    switch (ins.opcode) {
      /* getg, setg and movg  */
      case EIR_OPCODE_MOV:
        WRITES_TO(ins.mov.dst);
        READS_FROM(ins.mov.src);
        break;

      case EIR_OPCODE_MOVI: WRITES_TO(ins.movi.dst); break; /* values are not registers */
      case EIR_OPCODE_MOVF: WRITES_TO(ins.movf.dst); break; /* values are not registers */

      case EIR_OPCODE_GETG: WRITES_TO(ins.mov.dst); break;
      case EIR_OPCODE_SETG: READS_FROM(ins.mov.src); break;
      case EIR_OPCODE_MOVG: {
        break;
      }
      case EIR_OPCODE_LOADK:
        WRITES_TO(ins.loadk.dst);
        break; /* id is not a register */

      // case EIR_OPCODE_LOADK: break;
      case EIR_OPCODE_ADD:
      case EIR_OPCODE_SUB:
      case EIR_OPCODE_MUL:
      case EIR_OPCODE_DIV:
      case EIR_OPCODE_MOD:
      case EIR_OPCODE_EXP:
      case EIR_OPCODE_AND:
      case EIR_OPCODE_OR:
      case EIR_OPCODE_BAND:
      case EIR_OPCODE_BOR:
      case EIR_OPCODE_XOR:
      case EIR_OPCODE_EQL:
      case EIR_OPCODE_NEQ:
      case EIR_OPCODE_LT:
      case EIR_OPCODE_LTE:
      case EIR_OPCODE_GT:
      case EIR_OPCODE_GTE:
        WRITES_TO(ins.binop.dst);
        READS_FROM(ins.binop.a);
        READS_FROM(ins.binop.b);
        break;
      case EIR_OPCODE_NOT:
      case EIR_OPCODE_NEG:
      case EIR_OPCODE_BNOT:
      case EIR_OPCODE_DEC:
      case EIR_OPCODE_INC:
        WRITES_TO(ins.unop.dst);
        READS_FROM(ins.unop.a);
        break;
      case EIR_OPCODE_CALL:
        for (u32 i = E_REG_ARG0; i < E_REG_ARG_COUNT; i++) { READS_FROM(i); }
        WRITES_TO(ins.call.dst);
        break;
      case EIR_OPCODE_INDEX:
        WRITES_TO(ins.index.dst);
        READS_FROM(ins.index.base);
        READS_FROM(ins.index.index);
        break;
      case EIR_OPCODE_INDEX_ASSIGN:
        READS_FROM(ins.index_assign.value);
        READS_FROM(ins.index_assign.index);
        READS_FROM(ins.index_assign.base);
        break;
      case EIR_OPCODE_MK_LIST:
        for (u32 i = E_REG_ARG0; i < E_REG_ARG_COUNT; i++) { READS_FROM(i); }
        WRITES_TO(ins.mk_list.dst);
        break;
      case EIR_OPCODE_MK_MAP:
        for (u32 i = E_REG_ARG0; i < E_REG_ARG_COUNT; i++) { READS_FROM(i); }
        WRITES_TO(ins.mk_map.dst);
        break;
      case EIR_OPCODE_MK_STRUCT:
        /* we don't know how many members this instruction will initialize. clobber the entire argument vector. */
        for (u32 i = E_REG_ARG0; i < E_REG_ARG_COUNT; i++) { READS_FROM(i); }
        WRITES_TO(ins.mk_struct.dst);
        break;
      case EIR_OPCODE_MEMBER_ACCESS:
        WRITES_TO(ins.member_access.dst);
        READS_FROM(ins.member_access.base);
        break;
      case EIR_OPCODE_MEMBER_ASSIGN:
        READS_FROM(ins.member_assign.value);
        READS_FROM(ins.member_assign.base);
        break;
      case EIR_OPCODE_RET: READS_FROM(ins.ret.return_value); break;
      case EIR_OPCODE_JZ:
      case EIR_OPCODE_JNZ: READS_FROM(ins.cj.condition); break;
      case EIR_OPCODE_PUSH: {
        READS_FROM(ins.push.reg);
        break;
      }
      case EIR_OPCODE_POP: {
        WRITES_TO(ins.pop.reg);
        break;
      }

        // default: break;
      case EIR_OPCODE_NOP:
      case EIR_OPCODE_LABEL:
      case EIR_OPCODE_JMP: {
        break;
      }
    }
#undef WRITES_TO
#undef READS_FROM
  }

  /**
   * Extend the life of any register inside a backward jump
   * so that loop conditions (and the like) aren't overwritten.
   */
  for (u32 i = 0; i < cc->ninstructions; i++) {
    e_ins ins    = cc->instructions[i];
    u32   target = UINT32_MAX;
    if (ins.opcode == EIR_OPCODE_JMP) {
      target = ins.jmp.target;
    } else if (ins.opcode == EIR_OPCODE_JZ || ins.opcode == EIR_OPCODE_JNZ) {
      target = ins.cj.target;
    }
    if (target != UINT32_MAX && target < i) { // backward jump
      for (u32 r = 0; r < vreg_count; r++) {
        if (ra->ranges[r].start != UINT32_MAX && ra->ranges[r].start <= target && ra->ranges[r].end >= target) {
          if (i > ra->ranges[r].end) ra->ranges[r].end = i;
        }
      }
    }
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
// static int
// era_cmp_end(const void* a, const void* b)
// { return (int)((const era_range*)a)->end - (int)((const era_range*)b)->end; }

static int
era_allocate(int opt_level, era_state* ra)
{
  qsort(ra->ranges, ra->nranges, sizeof(era_range), era_cmp_start);

  bool phys_free[ERA_NUM_PHYS];
  memset(phys_free, 1, sizeof(phys_free));

  /* mark non general registers as always in use */
  if (opt_level >= 1) {
    /* OPTIMIZATION: Allow usage of non general registers (they're marked free). */
  } else {
    for (u32 i = 0; i < E_REG_GENERAL_BEGIN; i++) { phys_free[i] = false; }
  }

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
    for (u32 p = E_REG_GENERAL_BEGIN; p < ERA_NUM_PHYS; p++) {
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
        r->phys           = spill->phys;
        spill->phys       = ERA_SPILL_FLAG | next_spill++;
        active[spill_idx] = r; // replace
      } else {
        // r itself gets spilled
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

static int
era_rewrite(e_compiler* cc, const u32* vreg_to_phys)
{
#define MAP(r)                                                                                                                                       \
  do {                                                                                                                                               \
    u32 _r = (r);                                                                                                                                    \
    if (_r < E_REG_GENERAL_BEGIN) break;                                                                                                             \
    if (!(_r & ERA_SPILL_FLAG)) (r) = vreg_to_phys[_r];                                                                                              \
  } while (0)

  for (u32 i = 0; i < cc->ninstructions; i++) {
    e_ins ins = cc->instructions[i];

    switch (ins.opcode) {
      case EIR_OPCODE_MOV:
        MAP(ins.mov.dst);
        MAP(ins.mov.src);
        break;

      case EIR_OPCODE_MOVI: MAP(ins.movi.dst); break;
      case EIR_OPCODE_MOVF: MAP(ins.movf.dst); break;

      case EIR_OPCODE_PUSH:
      case EIR_OPCODE_POP: {
        MAP(ins.push.reg);
        break;
      }
      case EIR_OPCODE_LOADK: MAP(ins.loadk.dst); break;
      // case EIR_OPCODE_LOADK: break;
      case EIR_OPCODE_ADD:
      case EIR_OPCODE_SUB:
      case EIR_OPCODE_MUL:
      case EIR_OPCODE_DIV:
      case EIR_OPCODE_MOD:
      case EIR_OPCODE_EXP:
      case EIR_OPCODE_AND:
      case EIR_OPCODE_OR:
      case EIR_OPCODE_BAND:
      case EIR_OPCODE_BOR:
      case EIR_OPCODE_XOR:
      case EIR_OPCODE_EQL:
      case EIR_OPCODE_NEQ:
      case EIR_OPCODE_LT:
      case EIR_OPCODE_LTE:
      case EIR_OPCODE_GT:
      case EIR_OPCODE_GTE:
        MAP(ins.binop.dst);
        MAP(ins.binop.a);
        MAP(ins.binop.b);
        break;
      case EIR_OPCODE_NOT:
      case EIR_OPCODE_NEG:
      case EIR_OPCODE_BNOT:
      case EIR_OPCODE_INC:
      case EIR_OPCODE_DEC:
        MAP(ins.unop.dst);
        MAP(ins.unop.a);
        break;
      case EIR_OPCODE_CALL: MAP(ins.call.dst); break;
      case EIR_OPCODE_INDEX:
        MAP(ins.index.dst);
        MAP(ins.index.base);
        MAP(ins.index.index);
        break;
      case EIR_OPCODE_INDEX_ASSIGN:
        MAP(ins.index_assign.value);
        MAP(ins.index_assign.base);
        MAP(ins.index_assign.index);
        break;
      case EIR_OPCODE_MEMBER_ACCESS:
        MAP(ins.member_access.dst);
        MAP(ins.member_access.base);
        break;
      case EIR_OPCODE_MEMBER_ASSIGN:
        MAP(ins.member_assign.value);
        MAP(ins.member_assign.base);
        break;
      case EIR_OPCODE_RET: MAP(ins.ret.return_value); break;
      case EIR_OPCODE_JZ:
      case EIR_OPCODE_JNZ: MAP(ins.cj.condition); break;
      case EIR_OPCODE_GETG: MAP(ins.getg.dst); break;
      case EIR_OPCODE_SETG: MAP(ins.setg.src); break;

      case EIR_OPCODE_MK_LIST: MAP(ins.mk_list.dst); break;
      case EIR_OPCODE_MK_MAP: MAP(ins.mk_map.dst); break;
      case EIR_OPCODE_MK_STRUCT:
        MAP(ins.mk_struct.dst);
        break;

        // default: break;

      case EIR_OPCODE_NOP:
      case EIR_OPCODE_MOVG:
      case EIR_OPCODE_LABEL:
      case EIR_OPCODE_JMP: break;
    }

    // write the patched instruction back to the same location
    cc->instructions[i] = ins;
  }
#undef MAP

  return 0;
}

int
era_simple_register_allocation_pass(struct e_compiler* cc)
{
  era_state state;

  int e = era_compute_ranges(cc, &state);
  if (e < 0) return e;

  e = era_allocate(cc->info->opt_level, &state);
  if (e < 0) return e;

  e = era_rewrite(cc, state.vreg_to_phys);
  if (e < 0) return e;

  free(state.ranges);
  free(state.vreg_to_phys);

  return 0;
}