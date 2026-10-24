#include "codegraph.h"

#include "../../../inc/kit.cast.h"
#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.operate.h"
#include "../../../inc/kit.reg.h"
#include "../../../inc/kit.var.h"
#include "../tables.h"

static inline bool
is_instruction_unary_operation(kit_ir_opcode op)
{
  switch (op) {
    case KIT_IR_OPCODE_NOT:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_DEC:
    case KIT_IR_OPCODE_INC: return true;
    default: return false;
  }
  return -1;
}

static inline bool
is_instruction_binary_operation(kit_ir_opcode op)
{
  switch (op) {
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
    case KIT_IR_OPCODE_GTE: return true;
    default: return false;
  }
  return -1;
}

static inline bool
instruction_produces_constant_value(kit_ir_opcode op)
{
  switch (op) {
    case KIT_IR_OPCODE_MOVI:
    case KIT_IR_OPCODE_MOVF:
    case KIT_IR_OPCODE_LOADK: return true;
    default: return false;
  }
}

static inline int
get_instruction_constant_result(const kit_compiler* cc, const kit_ins* i, kit_var* result)
{
  switch (i->opcode) {
    case KIT_IR_OPCODE_MOVI: {
      *result = kit_var_from_int(i->movi.value);
      return 0;
    }
    case KIT_IR_OPCODE_MOVF: {
      *result = kit_var_from_float(i->movf.value);
      return 0;
    }
    case KIT_IR_OPCODE_LOADK: {
      u32 id = i->loadk.id;
      for (u32 j = 0; j < cc->lit_table->literals_count; j++) {
        kit_var* lit = &cc->lit_table->literals[j];
        if ((cc->lit_table->literal_hashes[j] != id)) continue;

        kit_var_shallow_cpy(lit, result);

        return 0;
      }
      return -1;
    }
    default: break;
  }
  return -1;
}

static inline bool
is_instruction_commutative(kit_ir_opcode op)
{
  switch (op) {
    case KIT_IR_OPCODE_ADD:
    case KIT_IR_OPCODE_MUL:
    case KIT_IR_OPCODE_AND:
    case KIT_IR_OPCODE_OR:
    case KIT_IR_OPCODE_BAND:
    case KIT_IR_OPCODE_BOR:
    case KIT_IR_OPCODE_XOR:
    case KIT_IR_OPCODE_EQL:
    case KIT_IR_OPCODE_NEQ: return true;

    default: return false;
  }
}

bool
codegraph_constant_folding(kit_compiler* cc, codegraph* cfg)
{
  bool changed_anything = false;

  bool changed = true;
  while (changed) {
    changed = false;
    /**
     * Read in 3 instructions at a time to find the following pattern:
     * movi a, 20
     * movf b, 30.0
     * neq dst=r0 a=a b=b
     *
     * and replace them with a single
     * loadk [true]
     *
     * We first do binary operations and then unary operations (slightly faster and cleaner).
     */
    u32 i = 0;
    while ((i = next_real_ins(cc, i)) != UINT32_MAX) {
      if (i + 2 >= cc->ninstructions) break;

      u32 one_idx   = i;
      u32 two_idx   = next_real_ins(cc, one_idx + 1);
      u32 three_idx = next_real_ins(cc, two_idx + 1);

      if (one_idx == UINT32_MAX || two_idx == UINT32_MAX || three_idx == UINT32_MAX) break;

      i = three_idx + 1;

      kit_ins* one   = &cc->instructions[one_idx];
      kit_ins* two   = &cc->instructions[two_idx];
      kit_ins* three = &cc->instructions[three_idx];
      // printf("CF: %u %u %u\n", one_idx, two_idx, three_idx);
      // printf("    opcodes: %d %d %d\n", one->opcode, two->opcode, three->opcode);

      if (!instruction_produces_constant_value(one->opcode) || !instruction_produces_constant_value(two->opcode)) continue;
      if (!is_instruction_binary_operation(three->opcode)) continue;

      /* check if three depends on one and (or two) */
      u32 three_sources[32];
      get_source_registers(three, three_sources);

      u32 one_dst = get_destination_reg(one);
      u32 two_dst = get_destination_reg(two);

      bool found_one = three_sources[0] == one_dst;
      bool found_two = three_sources[1] == two_dst;

      /* only swap out if the operation is associative */
      if ((three_sources[0] == two_dst && three_sources[1] == one_dst)) {
        if (!is_instruction_commutative(three->opcode)) continue;

        kit_ins* tmp = one;
        one          = two;
        two          = tmp;

        u32 tmp_dst = one_dst;
        one_dst     = two_dst;
        two_dst     = tmp_dst;

        found_one = true;
        found_two = true;
      }

      if (!found_one || !found_two) continue;

      kit_var one_result = KIT_NULLVAR;
      kit_var two_result = KIT_NULLVAR;

      if (get_instruction_constant_result(cc, one, &one_result) < 0) continue;
      if (get_instruction_constant_result(cc, two, &two_result) < 0) continue;

      kit_var result = operate(one_result, two_result, three->opcode);

      /* replace the three instructions with a single loadk */

      one->opcode = KIT_IR_OPCODE_NOP;
      two->opcode = KIT_IR_OPCODE_NOP;

      u32 three_dst = get_destination_reg(three);

      if (result.type == KIT_VARTYPE_INT) {
        three->opcode     = KIT_IR_OPCODE_MOVI;
        three->movi.dst   = three_dst;
        three->movi.value = result.val.i;
      } else if (result.type == KIT_VARTYPE_FLOAT) {
        three->opcode     = KIT_IR_OPCODE_MOVF;
        three->movf.dst   = three_dst;
        three->movf.value = result.val.f;
      } else if (result.type == KIT_VARTYPE_NULL) {
        three->opcode  = KIT_IR_OPCODE_MOV;
        three->mov.dst = three_dst;
        three->mov.src = KIT_REG_NIL;
      } else {
        if (add_literal_to_track(cc, &result) < 0) continue;
        three->opcode    = KIT_IR_OPCODE_LOADK;
        three->loadk.id  = kit_var_hash(&result);
        three->loadk.dst = three_dst;
      }

      changed          = true;
      changed_anything = true;
    }
  }

  /* now do unary operations */
  for (u32 i = 0; i < cc->ninstructions; i++) {
    if (i + 2 >= cc->ninstructions) break;

    kit_ins* one = &cc->instructions[i];
    kit_ins* two = &cc->instructions[i + 1];

    if (!instruction_produces_constant_value(one->opcode)) continue;

    u32 one_dst = get_destination_reg(one);

    /* special case, conditional jumps */
    if ((two->opcode == KIT_IR_OPCODE_JZ || two->opcode == KIT_IR_OPCODE_JNZ) && two->cj.condition == one_dst) {
      /* load the constant */
      kit_var one_result = KIT_NULLVAR;
      if (get_instruction_constant_result(cc, one, &one_result) < 0) continue;

      bool b = kit_cast_to_bool(&one_result);

      if (two->opcode == KIT_IR_OPCODE_JZ) {
        if (b) { /* condition always false, fallthrough */
          two->opcode = KIT_IR_OPCODE_NOP;
        } else { /* condition always true, just jump to it */
          two->opcode     = KIT_IR_OPCODE_JMP;
          two->jmp.target = two->cj.target;
        }
      }
      if (two->opcode == KIT_IR_OPCODE_JNZ) {
        if (!b) { /* condition always false, fallthrough */
          two->opcode = KIT_IR_OPCODE_NOP;
        } else { /* condition always true, just jump to it */
          two->opcode     = KIT_IR_OPCODE_JMP;
          two->jmp.target = two->cj.target;
        }
      }
      continue;
    }

    // if (!is_instruction_binary_operation(three->opcode) && !is_instruction_unary_operation(three->opcode)) continue;
    if (!is_instruction_unary_operation(two->opcode)) continue;

    /* check if two depends on one */
    u32 two_sources[32];
    get_source_registers(two, two_sources);

    bool found_one = two_sources[0] == one_dst;

    if (!found_one) continue;

    kit_var one_result = KIT_NULLVAR;
    if (get_instruction_constant_result(cc, one, &one_result) < 0) continue;

    kit_var result = operate(KIT_NULLVAR, one_result, two->opcode);

    /* replace the three instructions with a single loadk */

    if (add_literal_to_track(cc, &result) < 0) continue;

    one->opcode = KIT_IR_OPCODE_NOP;

    u32 two_dst = get_destination_reg(two);

    two->opcode    = KIT_IR_OPCODE_LOADK;
    two->loadk.id  = kit_var_hash(&result);
    two->loadk.dst = two_dst;
  }

  return changed_anything;
}