#include "codegraph.h"

#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.reg.h"

static inline bool
is_argument_register(u32 reg)
{ return reg >= KIT_REG_ARG0 && reg <= KIT_REG_ARG_END; }

bool
codegraph_argument_vector_move_coalescing(kit_compiler* cc, codegraph* cfg)
{
  bool changed_ever = false;

  for (u32 ip = 0; ip < cc->ninstructions; ip++) {
    kit_ins* mov = &cc->instructions[ip];

    if (mov->opcode != KIT_IR_OPCODE_MOV) continue;

    u32 dst = mov->mov.dst;
    u32 src = mov->mov.src;

    if (!is_argument_register(dst)) continue;

    if (src >= cfg->nvregs) continue;

    /* source still needed later */
    if (cfg->ins_live_out[ip][src]) continue;

    i64 def_ip = -1;

    for (i64 j = (i64)ip - 1; j >= 0; j--) {
      kit_ins* def = &cc->instructions[j];

      if (def->opcode == KIT_IR_OPCODE_LABEL) break;
      if (is_instruction_noop(def->opcode)) continue;
      if (is_instruction_impure(def->opcode)) break;

      u32 d = get_destination_reg(def);

      if (d == src) {
        def_ip = j;
        break;
      }

      /* another definition of dst means stop */
      if (d == dst) break;

      u32  srcs[32];
      u32  nsrcs   = get_source_registers(def, srcs);
      bool touches = (d == dst);
      for (u32 s = 0; s < nsrcs; s++) {
        if (srcs[s] == src || srcs[s] == dst) touches = true;
      }
      if (touches) break;
    }

    if (def_ip < 0) continue;

    kit_ins* def = &cc->instructions[def_ip];

    /*
     * Rewrite: rax = x
     *          arg0 = rax
     *
     * into: arg0 = x
     *       nop
     */
    set_destination_reg(def, dst);

    mov->opcode = KIT_IR_OPCODE_NOP;

    changed_ever = true;
  }

  return changed_ever;
}