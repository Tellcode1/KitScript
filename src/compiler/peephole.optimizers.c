#include "peephole.optimizers.h"

#include "codegraph/codegraph.h"

int
remove_jmp_where_it_would_fallthrough(kit_compiler* cc)
{
  u32* label_map = kit_arnalloc(cc->arena, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32)); // UINT32_MAX = not found

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == KIT_IR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  /* find a jump instruction */
  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];

    if (ins->opcode != KIT_IR_OPCODE_JMP) continue;

    u32 target = label_map[ins->jmp.target];
    if (target == UINT32_MAX || i > target) continue; /* skip backward jumps */

    /* check if there is nothing but noops in between the jump and the target */
    bool useless = true;
    for (u32 j = i + 1; j < target; j++) {
      kit_ins* k = &cc->instructions[j];

      // fprintf(stderr, "%i\n", k->opcode);

      if (!is_instruction_noop(k->opcode)) {
        useless = false;
        break;
      }
    }

    /* jmp is useless. noop it. */
    if (useless) { ins->opcode = KIT_IR_OPCODE_NOP; }
  }

  // kit_arnfree(cc->arena, label_map);

  return 0;
}

int
strip_noops(kit_compiler* cc)
{
  /* just remove them. label pass will handle the indices later. */
  kit_ins* copy = kit_arnalloc(cc->arena, sizeof(kit_ins) * cc->ninstructions);
  memcpy(copy, cc->instructions, sizeof(kit_ins) * cc->ninstructions);

  u32 ctr = 0;
  for (u32 i = 0; i < cc->ninstructions; i++) {
    if (copy[i].opcode != KIT_IR_OPCODE_NOP) { cc->instructions[ctr++] = copy[i]; }
  }

  cc->ninstructions = ctr;

  // kit_arnfree(cc->arena, copy);

  return 0;
}