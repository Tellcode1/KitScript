#ifndef E_REGISTER_ALLOCATION_H
#define E_REGISTER_ALLOCATION_H

#include "cc.h"
#include "reg.h"
#include "stdafx.h"

#define ERA_NUM_PHYS E_REG_COUNT
#define ERA_SPILL_FLAG 0x80000000u
#define ERA_SPILL_SCRATCH (E_REG_GENERAL_END - 1)

typedef struct era_range {
  u32 vreg;
  u32 start; // instruction index of definition
  u32 end;   // instruction index of last use
  u32 phys;  // assigned physical register or spill slot
  // ORd with RA_SPILL flag if spilled.
  u32 spill_offset;
} era_range;

typedef struct era_state {
  era_range* ranges;
  u32*       vreg_to_phys;
  u32        nranges;
} era_state;

/* rewrite cc's instruction stream to use physical register indices instead of vregisters. */
int era_simple_register_allocation_pass(struct e_compiler* cc);

#endif // E_REGISTER_ALLOCATION_H