#include "codegraph.h"

#include "../../../inc/kit.cc.h"
#include "../../../inc/kit.reg.h"

static u32
remap_reg(u32 r, u32 vreg_off)
{
  if (r == UINT32_MAX || r < KIT_REG_GENERAL_BEGIN) return r;
  return r + vreg_off;
}

static void
remap_ins(kit_ins* ins, u32 vo, u32 lo)
{
  switch ((kit_ir_opcode_bits)ins->opcode) {
    case KIT_IR_OPCODE_MOV:
      ins->mov.dst = remap_reg(ins->mov.dst, vo);
      ins->mov.src = remap_reg(ins->mov.src, vo);
      break;
    case KIT_IR_OPCODE_MOVI: ins->movi.dst = remap_reg(ins->movi.dst, vo); break;
    case KIT_IR_OPCODE_MOVF: ins->movf.dst = remap_reg(ins->movf.dst, vo); break;
    case KIT_IR_OPCODE_LOADK: ins->loadk.dst = remap_reg(ins->loadk.dst, vo); break;
    case KIT_IR_OPCODE_LOADFN: ins->loadfn.dst = remap_reg(ins->loadfn.dst, vo); break;
    case KIT_IR_OPCODE_GETG: ins->getg.dst = remap_reg(ins->getg.dst, vo); break;
    case KIT_IR_OPCODE_SETG: ins->setg.src = remap_reg(ins->setg.src, vo); break;
    case KIT_IR_OPCODE_MOVG: break;

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
      ins->binop.dst = remap_reg(ins->binop.dst, vo);
      ins->binop.a   = remap_reg(ins->binop.a, vo);
      ins->binop.b   = remap_reg(ins->binop.b, vo);
      break;

    case KIT_IR_OPCODE_NOT:
    case KIT_IR_OPCODE_NEG:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_INC:
    case KIT_IR_OPCODE_DEC:
      ins->unop.dst = remap_reg(ins->unop.dst, vo);
      ins->unop.a   = remap_reg(ins->unop.a, vo);
      break;

    case KIT_IR_OPCODE_INDEX:
      ins->index.dst   = remap_reg(ins->index.dst, vo);
      ins->index.base  = remap_reg(ins->index.base, vo);
      ins->index.index = remap_reg(ins->index.index, vo);
      break;
    case KIT_IR_OPCODE_INDEX_ASSIGN:
      ins->index_assign.value = remap_reg(ins->index_assign.value, vo);
      ins->index_assign.base  = remap_reg(ins->index_assign.base, vo);
      ins->index_assign.index = remap_reg(ins->index_assign.index, vo);
      break;

    case KIT_IR_OPCODE_MEMBER_ACCESS:
      ins->member_access.dst  = remap_reg(ins->member_access.dst, vo);
      ins->member_access.base = remap_reg(ins->member_access.base, vo);
      break;
    case KIT_IR_OPCODE_MEMBER_ASSIGN:
      ins->member_assign.value = remap_reg(ins->member_assign.value, vo);
      ins->member_assign.base  = remap_reg(ins->member_assign.base, vo);
      break;

    case KIT_IR_OPCODE_MK_LIST: ins->mk_list.dst = remap_reg(ins->mk_list.dst, vo); break;
    case KIT_IR_OPCODE_MK_MAP: ins->mk_map.dst = remap_reg(ins->mk_map.dst, vo); break;
    case KIT_IR_OPCODE_MK_STRUCT: ins->mk_struct.dst = remap_reg(ins->mk_struct.dst, vo); break;

    case KIT_IR_OPCODE_CALL:
      ins->call.dst = remap_reg(ins->call.dst, vo);
      ins->call.reg = remap_reg(ins->call.reg, vo);
      break;

    case KIT_IR_OPCODE_PUSH: ins->push.reg = remap_reg(ins->push.reg, vo); break;
    case KIT_IR_OPCODE_POP: ins->pop.reg = remap_reg(ins->pop.reg, vo); break;

    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ:
      ins->cj.condition = remap_reg(ins->cj.condition, vo);
      ins->cj.target += lo;
      break;
    case KIT_IR_OPCODE_JMP: ins->jmp.target += lo; break;

    case KIT_IR_OPCODE_RET: ins->ret.return_value = remap_reg(ins->ret.return_value, vo); break;
    case KIT_IR_OPCODE_ASSERT: ins->assertion.cond = remap_reg(ins->assertion.cond, vo); break;
    case KIT_IR_OPCODE_LABEL: ins->label.id += lo; break;

    case KIT_IR_OPCODE_NOP:
    default: break;
  }
}

bool
codegraph_inline_one_call(kit_compiler* cc, u32 call_ip, const kitc_function* callee)
{
  kit_ins* call      = &cc->instructions[call_ip];
  u32      dst       = call->call.dst;
  u32      nargs     = call->call.nargs;
  u32      vreg_off  = cc->next_vreg;
  u32      label_off = cc->next_label;
  u32      end_label = label_off + callee->labels_used;

  u32 body_len = callee->code_count;
  u32 new_len  = (cc->ninstructions * 2) - 1 + body_len + 1; /* -call + body + label */

  kit_ins* out = malloc(sizeof(kit_ins) * new_len);
  u32      k   = 0;

  for (u32 i = 0; i < call_ip; i++) out[k++] = cc->instructions[i];

  for (u32 i = 0; i < body_len; i++) {
    kit_ins ins = callee->code[i];
    remap_ins(&ins, vreg_off, label_off);

    if (ins.opcode == KIT_IR_OPCODE_RET) {
      out[k++] = (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = dst, .src = ins.ret.return_value } };
      out[k++] = (kit_ins){ .jmp = { .opcode = KIT_IR_OPCODE_JMP, .target = end_label } };
    } else {
      out[k++] = ins;
    }
  }

  out[k++] = (kit_ins){ .label = { .opcode = KIT_IR_OPCODE_LABEL, .id = end_label } };

  for (u32 i = call_ip + 1; i < cc->ninstructions; i++) { out[k++] = cc->instructions[i]; }

  free(cc->instructions);
  cc->instructions  = out;
  cc->ninstructions = k;
  cc->cinstructions = new_len;
  cc->next_vreg += (i32)callee->vregs_used;
  cc->next_label += callee->labels_used + 1;
  return true;
}