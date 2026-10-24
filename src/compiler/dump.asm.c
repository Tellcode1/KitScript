#include "dump.asm.h"

#include "../../inc/kit.cc.h"
#include "../../inc/kit.ir.h"
#include "../../inc/kit.reg.h"
#include "../../inc/kit.stdafx.h"

static inline const char*
lookup(const kit_compilation_result* r, u32 name)
{
  for (u32 i = 0; i < r->names_count; i++) {
    if (r->names_hashes[i] == name) { return r->names[i]; }
  }
  return "[symbol not found]";
}

static inline const char*
get_register_name(u32 reg_id, char buff[32])
{
  if (reg_id >= KIT_REG_ARG0 && reg_id < KIT_REG_ARG_COUNT) {
    snprintf(buff, 32, "arg%i", reg_id - KIT_REG_ARG0);
  } else if (reg_id >= KIT_REG_GENERAL_BEGIN && reg_id <= KIT_REG_GENERAL_END) {
    snprintf(buff, 32, "r%i", reg_id - KIT_REG_GENERAL_BEGIN);
  } else if (reg_id == KIT_REG_SP) {
    snprintf(buff, 32, "rsp");
  } else if (reg_id == KIT_REG_IP) {
    snprintf(buff, 32, "rip");
  } else if (reg_id == KIT_REG_NIL) {
    snprintf(buff, 32, "rnil");
  } else {
    snprintf(buff, 32, "??%i", reg_id);
  }
  return buff;
}

void
kit_print_instruction(kit_ins i, const kit_compilation_result* r, FILE* f)
{
  char buf0[32];
  char buf1[32];
  char buf2[32];
  switch ((kit_ir_opcode_bits)i.opcode) {
    case KIT_IR_OPCODE_LOADK: {
      fprintf(f, "loadk dst=%s, id=%u\n", get_register_name(i.loadk.dst, buf0), i.loadk.id);
      break;
    }
    case KIT_IR_OPCODE_ASSERT: {
      fprintf(f, "assert condition=%s, line_id=%u\n", get_register_name(i.assertion.cond, buf0), i.assertion.line_id);
      break;
    }
    case KIT_IR_OPCODE_MOV: {
      fprintf(f, "mov dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1));
      break;
    }

    case KIT_IR_OPCODE_MOVI: {
      fprintf(f, "movi dst=%s, value=%i\n", get_register_name(i.movi.dst, buf0), i.movi.value);
      break;
    }
    case KIT_IR_OPCODE_MOVF: {
      fprintf(f, "movf dst=%s, value=%f\n", get_register_name(i.movf.dst, buf0), i.movf.value);
      break;
    }

    case KIT_IR_OPCODE_LOADFN: {
      const char* name = lookup(r, i.loadfn.id);
      if (name) fprintf(f, "loadfn dst=%s, name=%s id=%u\n", get_register_name(i.loadfn.dst, buf0), name, i.loadfn.id);
      else fprintf(f, "loadfn dst=%s, id=%u\n", get_register_name(i.loadfn.dst, buf0), i.loadfn.id);
      break;
    }

    case KIT_IR_OPCODE_ADD:
      fprintf(
          f,
          "add dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_SUB:
      fprintf(
          f,
          "sub dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_MUL:
      fprintf(
          f,
          "mul dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_DIV:
      fprintf(
          f,
          "div dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_MOD:
      fprintf(
          f,
          "mod dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_EXP:
      fprintf(
          f,
          "exp dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_AND:
      fprintf(
          f,
          "and dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_OR:
      fprintf(
          f, "or dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_BAND:
      fprintf(
          f,
          "band dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_BOR:
      fprintf(
          f,
          "bor dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_XOR:
      fprintf(
          f,
          "xor dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_EQL:
      fprintf(
          f,
          "eql dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_NEQ:
      fprintf(
          f,
          "neq dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_LT:
      fprintf(
          f, "lt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_LTE:
      fprintf(
          f,
          "lte dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_GT:
      fprintf(
          f, "gt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case KIT_IR_OPCODE_GTE:
      fprintf(
          f,
          "gte dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;

    case KIT_IR_OPCODE_INC: fprintf(f, "inc dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case KIT_IR_OPCODE_DEC: fprintf(f, "dec dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case KIT_IR_OPCODE_BNOT: fprintf(f, "bnot dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case KIT_IR_OPCODE_NEG: fprintf(f, "neg dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case KIT_IR_OPCODE_NOT: fprintf(f, "not dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;

    case KIT_IR_OPCODE_RET: {
      fprintf(f, "ret val=%s\n", get_register_name(i.ret.return_value, buf0));
      break;
    }
    case KIT_IR_OPCODE_NOP: fprintf(f, "nop\n"); break;
    case KIT_IR_OPCODE_MK_LIST: fprintf(f, "mk_list dst=%s, nelems=%u\n", get_register_name(i.mk_list.dst, buf0), i.mk_list.nelems); break;
    case KIT_IR_OPCODE_MK_MAP: fprintf(f, "mk_map dst=%s, npairs=%u\n", get_register_name(i.mk_map.dst, buf0), i.mk_map.npairs); break;
    case KIT_IR_OPCODE_INDEX:
      fprintf(
          f,
          "index dst=%s, base=%s, index=%s\n",
          get_register_name(i.index.dst, buf0),
          get_register_name(i.index.base, buf1),
          get_register_name(i.index.index, buf2));
      break;
    case KIT_IR_OPCODE_CALL:
      fprintf(f, "call dst=%s, fn=%s, nargs=%u\n", get_register_name(i.call.dst, buf0), get_register_name(i.call.reg, buf1), i.call.nargs);
      break;
    case KIT_IR_OPCODE_INDEX_ASSIGN:
      fprintf(
          f,
          "index_assign value=%s, base=%s, index=%s\n",
          get_register_name(i.index_assign.value, buf0),
          get_register_name(i.index_assign.base, buf1),
          get_register_name(i.index_assign.index, buf2));
      break;

    case KIT_IR_OPCODE_LABEL: fprintf(f, "label id=%u\n", i.label.id); break;
    case KIT_IR_OPCODE_JMP: fprintf(f, "jmp target=%u\n", i.jmp.target); break;
    case KIT_IR_OPCODE_JZ: fprintf(f, "jz target=%u, condition=%s\n", i.jz.target, get_register_name(i.jz.condition, buf0)); break;
    case KIT_IR_OPCODE_JNZ: fprintf(f, "jnz target=%u, condition=%s\n", i.jnz.target, get_register_name(i.jnz.condition, buf0)); break;
    case KIT_IR_OPCODE_MEMBER_ACCESS:
      fprintf(
          f,
          "member_access dst=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_access.dst, buf0),
          get_register_name(i.member_access.base, buf1),
          i.member_access.member_id);
      break;
    case KIT_IR_OPCODE_MEMBER_ASSIGN:
      fprintf(
          f,
          "member_assign value=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_assign.value, buf0),
          get_register_name(i.member_assign.base, buf1),
          i.member_assign.member_id);
      break;
    case KIT_IR_OPCODE_MK_STRUCT: fprintf(f, "mk_struct dst=%s, id=%u\n", get_register_name(i.mk_struct.dst, buf0), i.mk_struct.struct_id); break;
    case KIT_IR_OPCODE_GETG: fprintf(f, "getg dst=%s, src=gv%u\n", get_register_name(i.mov.dst, buf0), i.mov.src); break;
    case KIT_IR_OPCODE_SETG: fprintf(f, "setg dst=gv%u, src=%s\n", i.mov.dst, get_register_name(i.mov.src, buf1)); break;
    case KIT_IR_OPCODE_MOVG: fprintf(f, "movg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case KIT_IR_OPCODE_PUSH: fprintf(f, "push %s\n", get_register_name(i.push.reg, buf0)); break;
    case KIT_IR_OPCODE_POP: fprintf(f, "pop %s\n", get_register_name(i.pop.reg, buf0)); break;
  }
}

void
kit_print_instruction_stream(const kit_compilation_result* r, const kit_ins* ins, u32 nins, int indent, FILE* f)
{
  for (u32 ip = 0; ip < nins; ip++) {
    u32 instruction_offset = ip;

    for (int j = 0; j < indent; j++) fputc(' ', stdout);

    fprintf(f, "%-4u: ", instruction_offset); // Print offset of instruction
    kit_print_instruction(ins[ip], r, f);
  }
}

void
kit_dump_asm(const kit_compilation_result* r)
{
  kit_print_instruction_stream(r, r->instructions, r->instructions_count, 0, stdout);
  for (u32 i = 0; i < r->functions_count; i++) {
    const kitc_function* func = &r->functions[i];
    fprintf(stdout, "[%s|%u](%u):\n", lookup(r, func->name_hash), func->name_hash, func->nargs);
    kit_print_instruction_stream(r, func->code, func->code_count, 4, stdout);
  }

  fprintf(stdout, "\nstructures:\n");
  for (u32 i = 0; i < r->structs_count; i++) {
    const kitc_struct_information* info = &r->structs[i];
    fprintf(stdout, "[%s] = {", info->name);
    for (u32 j = 0; j < info->fields_count; j++) {
      fprintf(stdout, "%s/%u", info->field_names[j], info->field_hashes[j]);
      if (j != info->fields_count - 1) { fprintf(stdout, ", "); }
    }
    fprintf(stdout, "},\n");
  }
  fprintf(stdout, "\n");

  fprintf(stdout, "literals:\n");
  for (u32 i = 0; i < r->literals_count; i++) {
    fprintf(stdout, "[%u | %u] = ", i, kit_var_hash(&r->literals[i]));
    kit_var_print(&r->literals[i], stdout);
    fputc('\n', stdout);
  }

  fprintf(stdout, "symtab:\n");
  for (u32 i = 0; i < r->names_count; i++) { fprintf(stdout, "%i: %u = %s\n", i, r->names_hashes[i], r->names[i]); }
}
