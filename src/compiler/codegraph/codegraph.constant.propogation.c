#include "codegraph.h"

#include "../../../inc/kit.cast.h"
#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.operate.h"
#include "../tables.h"

static inline int
replace_move_with_constant_load(kit_compiler* cc, const codegraph* cfg, kit_ins* ins, u32 dst_reg, const kit_var* value)
{
  if (value->type == KIT_VARTYPE_INT) {
    ins->opcode     = KIT_IR_OPCODE_MOVI;
    ins->movi.dst   = dst_reg;
    ins->movi.value = value->val.i;
  } else if (value->type == KIT_VARTYPE_FLOAT) {
    ins->opcode     = KIT_IR_OPCODE_MOVF;
    ins->movf.dst   = dst_reg;
    ins->movf.value = value->val.f;
  } else {
    if (add_literal_to_track(cc, value) < 0) return -1;

    ins->opcode    = KIT_IR_OPCODE_LOADK;
    ins->loadk.id  = kit_var_hash(value);
    ins->loadk.dst = dst_reg;
  }

  return 0;
}

bool
codegraph_local_constant_propagation(kit_compiler* cc, codegraph* cfg)
{
  kit_var* regs         = kit_arnalloc(cfg->arena, cfg->nvregs * sizeof(kit_var));
  bool*    values_known = kit_arnalloc(cfg->arena, cfg->nvregs * sizeof(bool));

  const u32 nvregs = cfg->nvregs;
  for (u32 i = 0; i < nvregs; i++) {
    regs[i]         = KIT_NULLVAR;
    values_known[i] = false;
  }

  bool changed = false;

  for (u32 block = 0; block < cfg->nblocks; block++) {
    codeblock* blk = &cfg->blocks[block];

    memset(values_known, 0, cfg->nvregs * sizeof(bool));

    for (u32 i = blk->start; i <= blk->end; i++) {
      kit_ins* ins = &cc->instructions[i];

      /* impure instruction. flush state and move on. */
      if (ins->opcode != KIT_IR_OPCODE_LABEL && ins->opcode != KIT_IR_OPCODE_INDEX_ASSIGN && ins->opcode != KIT_IR_OPCODE_MEMBER_ASSIGN
          && ins->opcode != KIT_IR_OPCODE_JZ && ins->opcode != KIT_IR_OPCODE_JNZ && ins->opcode != KIT_IR_OPCODE_CALL
          && is_instruction_impure(ins->opcode)) {
        /* this is the first optimization pass, assume jumps wreck all registers */
        memset(values_known, 0, nvregs * sizeof(bool));
        continue;
      }

      switch (ins->opcode) {
        case KIT_IR_OPCODE_NOP: break;

        case KIT_IR_OPCODE_LOADFN: {
          u32 dst = ins->loadfn.dst;
          u32 id  = ins->loadfn.id;

          values_known[dst] = true;
          regs[dst]         = (kit_var){ .type = KIT_VARTYPE_FUNCTION, .val.func = { .hash = id } };

          break;
        }

        case KIT_IR_OPCODE_MOV: {
          u32 dst = ins->mov.dst;
          u32 src = ins->mov.src;

          /* If the value is already in dst, turn this instruction into a NOOP */
          if (values_known[dst] && values_known[src] && kit_var_equal(&regs[dst], &regs[src])) {
            changed     = true;
            ins->opcode = KIT_IR_OPCODE_NOP;
            continue;
          }

          if (values_known[src]) {
            changed |= true;
            replace_move_with_constant_load(cc, cfg, ins, dst, &regs[src]);
            values_known[dst] = true;
          }

          regs[dst]         = regs[src];
          values_known[dst] = values_known[src]; /* if the value of src is known, then the value of dst is known */

          break;
        }
        case KIT_IR_OPCODE_MOVI: {
          regs[ins->movi.dst]         = kit_var_from_int(ins->movi.value);
          values_known[ins->movi.dst] = true;
          break;
        }
        case KIT_IR_OPCODE_MOVF: {
          regs[ins->movf.dst]         = kit_var_from_float(ins->movf.value);
          values_known[ins->movf.dst] = true;
          break;
        }

        case KIT_IR_OPCODE_GETG: {
          values_known[ins->getg.dst] = false;
          break;
        }

        case KIT_IR_OPCODE_SETG:
        case KIT_IR_OPCODE_MOVG: break;

        case KIT_IR_OPCODE_NEQ:
        case KIT_IR_OPCODE_EQL: {
          /* special case, if both point to the same register */
          u32 a   = ins->binop.a;
          u32 b   = ins->binop.b;
          u32 dst = ins->binop.dst;
          if (a == b) {
            values_known[dst] = true;
            changed           = true;
            /* register self comparison will always result in true (for EQL)*/
            regs[dst] = kit_var_from_bool(ins->opcode == KIT_IR_OPCODE_EQL ? true : false);

            replace_move_with_constant_load(cc, cfg, ins, dst, &regs[dst]);
            break;
          }

          bool values_are_equal = false;

          if (values_known[a] && values_known[b]) {
            values_are_equal = kit_var_equal(&regs[a], &regs[b]);
            regs[dst]        = kit_var_from_bool(ins->opcode == KIT_IR_OPCODE_EQL ? values_are_equal : !values_are_equal);
            replace_move_with_constant_load(cc, cfg, ins, dst, &regs[dst]);
            values_known[dst] = true;
            changed           = true;
            break;
          }
          values_known[dst] = false;
          break;
        }

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
        case KIT_IR_OPCODE_LT:
        case KIT_IR_OPCODE_LTE:
        case KIT_IR_OPCODE_GT:
        case KIT_IR_OPCODE_GTE:
          /* if value of both operands are known, compute it and store it */ {
            if (!values_known[ins->binop.a] || !values_known[ins->binop.b]) {
              values_known[ins->binop.dst] = false;
              break;
            }

            kit_var a      = regs[ins->binop.a];
            kit_var b      = regs[ins->binop.b];
            kit_var result = operate(a, b, ins->opcode);

            u32 dst = ins->binop.dst;
            replace_move_with_constant_load(cc, cfg, ins, dst, &result);

            changed |= true;

            regs[dst]         = result;
            values_known[dst] = true;
            break;
          }

        case KIT_IR_OPCODE_BNOT:
        case KIT_IR_OPCODE_NEG:
        case KIT_IR_OPCODE_NOT:
        case KIT_IR_OPCODE_INC:
        case KIT_IR_OPCODE_DEC: {
          if (!values_known[ins->unop.a]) {
            values_known[ins->unop.dst] = false;
            break;
          }

          kit_var a      = regs[ins->unop.a];
          kit_var result = operate(KIT_NULLVAR, a, ins->opcode);

          u32 dst = ins->unop.dst;
          replace_move_with_constant_load(cc, cfg, ins, dst, &result);

          changed |= true;

          regs[dst]         = result;
          values_known[dst] = true;
          break;
        }

        case KIT_IR_OPCODE_LOADK: {
          u32 id  = ins->loadk.id;
          u32 dst = ins->loadk.dst;

          values_known[dst] = false; // set when we find it.

          // for (u32 j = 0; j < cc->lit_table->literals_count; j++) {
          //   kit_var* lit = &cc->lit_table->literals[j];
          //   if (cc->lit_table->literal_hashes[j] != id) continue;

          //   kit_var tmp = *lit;
          //   replace_move_with_constant_load(cc, cfg, ins, ins->loadk.dst, lit);

          //   changed           = true;
          //   regs[dst]         = tmp;
          //   values_known[dst] = true;

          //   break;
          // }

          break;
        }

        case KIT_IR_OPCODE_JZ: {
          u32 target   = ins->jz.target;
          u32 cond_reg = ins->jz.condition;

          if (values_known[cond_reg] && kit_cast_to_bool(&regs[cond_reg])) {
            ins->opcode = KIT_IR_OPCODE_NOP;
            changed     = true;
          } else if (values_known[cond_reg] && !kit_cast_to_bool(&regs[cond_reg])) {
            ins->opcode     = KIT_IR_OPCODE_JMP;
            ins->jmp.target = target;
            changed         = true;
          }

          memset(values_known, 0, nvregs * sizeof(bool));
          break;
        }
        case KIT_IR_OPCODE_JNZ: {
          u32 target   = ins->jnz.target;
          u32 cond_reg = ins->jnz.condition;

          if (values_known[cond_reg] && kit_cast_to_bool(&regs[cond_reg])) {
            ins->opcode     = KIT_IR_OPCODE_JMP;
            ins->jmp.target = target;
            changed         = true;
          } else if (values_known[cond_reg] && !kit_cast_to_bool(&regs[cond_reg])) {
            ins->opcode = KIT_IR_OPCODE_NOP;
            changed     = true;
          }

          memset(values_known, 0, nvregs * sizeof(bool));
          break;
        }

        case KIT_IR_OPCODE_JMP: {
          memset(values_known, 0, nvregs * sizeof(bool));
          break;
        }

        case KIT_IR_OPCODE_ASSERT: {
          u32 cond = ins->assertion.cond;

          /* assertion is always true, noop it. */
          if (values_known[cond] && kit_var_to_bool(regs[cond])) { ins->opcode = KIT_IR_OPCODE_NOP; }

          break;
        }

        case KIT_IR_OPCODE_CALL: {
          u32 dst = ins->call.dst;
          u32 reg = ins->call.reg;

          /* know which function is being called */
          if (!values_known[reg]) {
            values_known[dst] = false;
            break;
          }

          u32            id = regs[reg].val.func.hash;
          kitc_function* fn = NULL;
          for (u32 i = 0; i < cc->function_table->functions_count; i++) {
            kitc_function* f = &cc->function_table->functions[i];
            if (f->name_hash == id) {
              fn = f;
              break;
            }
          }

          if (!fn) {
            values_known[dst] = false;
            break;
          }

          if (cc->info->opt_level >= 3) {
            codegraph_inline_one_call(cc, i, fn);

            /* Return :: we modified the codegraph */
            return true;
          }

          break;
        }

        default: {
          u32 d = get_destination_reg(ins);
          if (d != UINT32_MAX) values_known[d] = false;
          break;
        }
      }
    }
  }

  return changed;
}