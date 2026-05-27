#ifndef E_REGISTER_ALLOCATION_H
#define E_REGISTER_ALLOCATION_H

#include "cc.h"
#include "stdafx.h"

#define ERA_MAX_VREGS 4096
#define ERA_NUM_PHYS 256
#define ERA_SPILL_FLAG 0x80000000u

typedef struct era_range {
  u32 vreg;
  u32 start; // instruction index of definition
  u32 end;   // instruction index of last use
  u32 phys;  // assigned physical register or spill slot
  // ORd with RA_SPILL flag if spilled.
} era_range;

typedef struct era_state {
  era_range ranges[ERA_MAX_VREGS];
  u32       vreg_to_phys[ERA_MAX_VREGS];
  u32       nranges;
} era_state;

/* rewrite cc's instruction stream to use physical register indices instead of vregisters. */
void era_register_allocation_pass(struct e_compiler* cc);

#endif // E_REGISTER_ALLOCATION_H