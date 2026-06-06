/**
 * MIT License
 *
 * Copyright (c) 2026 Tellcode1
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "../inc/kit.cc.h"
#include "../inc/kit.reg.h"
#include "../inc/kit.rwhelp.h"
#include "../inc/kit.stdafx.h"
#include "../inc/kit.var.h"

#include <stdio.h>
#include <string.h>

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

static inline void
kit_print_instruction(kit_ins i, const kit_compilation_result* r, FILE* f)
{
  char buf0[32];
  char buf1[32];
  char buf2[32];
  switch ((eir_opcode_bits)i.opcode) {
    case EIR_OPCODE_LOADK: {
      fprintf(f, "loadk dst=%s, id=%u\n", get_register_name(i.loadk.dst, buf0), i.loadk.id);
      break;
    }
    case EIR_OPCODE_ASSERT: {
      fprintf(f, "assert condition=%s, line_id=%u\n", get_register_name(i.assertion.cond, buf0), i.assertion.line_id);
      break;
    }
    case EIR_OPCODE_MOV: {
      fprintf(f, "mov dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1));
      break;
    }

    case EIR_OPCODE_MOVI: {
      fprintf(f, "movi dst=%s, value=%i\n", get_register_name(i.movi.dst, buf0), i.movi.value);
      break;
    }
    case EIR_OPCODE_MOVF: {
      fprintf(f, "movf dst=%s, value=%f\n", get_register_name(i.movf.dst, buf0), i.movf.value);
      break;
    }

    case EIR_OPCODE_ADD:
      fprintf(
          f,
          "add dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_SUB:
      fprintf(
          f,
          "sub dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MUL:
      fprintf(
          f,
          "mul dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_DIV:
      fprintf(
          f,
          "div dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MOD:
      fprintf(
          f,
          "mod dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EXP:
      fprintf(
          f,
          "exp dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_AND:
      fprintf(
          f,
          "and dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_OR:
      fprintf(
          f, "or dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BAND:
      fprintf(
          f,
          "band dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BOR:
      fprintf(
          f,
          "bor dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_XOR:
      fprintf(
          f,
          "xor dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EQL:
      fprintf(
          f,
          "eql dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_NEQ:
      fprintf(
          f,
          "neq dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LT:
      fprintf(
          f, "lt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LTE:
      fprintf(
          f,
          "lte dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GT:
      fprintf(
          f, "gt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GTE:
      fprintf(
          f,
          "gte dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;

    case EIR_OPCODE_INC: fprintf(f, "inc dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_DEC: fprintf(f, "dec dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_BNOT: fprintf(f, "bnot dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NEG: fprintf(f, "neg dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NOT: fprintf(f, "not dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;

    case EIR_OPCODE_RET: {
      fprintf(f, "ret val=%s\n", get_register_name(i.ret.return_value, buf0));
      break;
    }
    case EIR_OPCODE_NOP: fprintf(f, "nop\n"); break;
    case EIR_OPCODE_MK_LIST: fprintf(f, "mk_list dst=%s, nelems=%u\n", get_register_name(i.mk_list.dst, buf0), i.mk_list.nelems); break;
    case EIR_OPCODE_MK_MAP: fprintf(f, "mk_map dst=%s, npairs=%u\n", get_register_name(i.mk_map.dst, buf0), i.mk_map.npairs); break;
    case EIR_OPCODE_INDEX:
      fprintf(
          f,
          "index dst=%s, base=%s, index=%s\n",
          get_register_name(i.index.dst, buf0),
          get_register_name(i.index.base, buf1),
          get_register_name(i.index.index, buf2));
      break;
    case EIR_OPCODE_CALL:
      fprintf(
          f,
          "call dst=%s, id=%u|name=%s, nargs=%u\n",
          get_register_name(i.call.dst, buf0),
          i.call.function_id,
          lookup(r, i.call.function_id),
          i.call.nargs);
      break;
    case EIR_OPCODE_INDEX_ASSIGN:
      fprintf(
          f,
          "index_assign value=%s, base=%s, index=%s\n",
          get_register_name(i.index_assign.value, buf0),
          get_register_name(i.index_assign.base, buf1),
          get_register_name(i.index_assign.index, buf2));
      break;

    case EIR_OPCODE_LABEL: fprintf(f, "label id=%u\n", i.label.id); break;
    case EIR_OPCODE_JMP: fprintf(f, "jmp target=%u\n", i.jmp.target); break;
    case EIR_OPCODE_JZ: fprintf(f, "jz target=%u, condition=%s\n", i.jz.target, get_register_name(i.jz.condition, buf0)); break;
    case EIR_OPCODE_JNZ: fprintf(f, "jnz target=%u, condition=%s\n", i.jnz.target, get_register_name(i.jnz.condition, buf0)); break;
    case EIR_OPCODE_MEMBER_ACCESS:
      fprintf(
          f,
          "member_access dst=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_access.dst, buf0),
          get_register_name(i.member_access.base, buf1),
          i.member_access.member_id);
      break;
    case EIR_OPCODE_MEMBER_ASSIGN:
      fprintf(
          f,
          "member_assign value=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_assign.value, buf0),
          get_register_name(i.member_assign.base, buf1),
          i.member_assign.member_id);
      break;
    case EIR_OPCODE_MK_STRUCT: fprintf(f, "mk_struct dst=%s, id=%u\n", get_register_name(i.mk_struct.dst, buf0), i.mk_struct.struct_id); break;
    case EIR_OPCODE_GETG: fprintf(f, "getg dst=%s, src=gv%u\n", get_register_name(i.mov.dst, buf0), i.mov.src); break;
    case EIR_OPCODE_SETG: fprintf(f, "setg dst=gv%u, src=%s\n", i.mov.dst, get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_MOVG: fprintf(f, "movg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_PUSH: fprintf(f, "push %s\n", get_register_name(i.push.reg, buf0)); break;
    case EIR_OPCODE_POP: fprintf(f, "pop %s\n", get_register_name(i.pop.reg, buf0)); break;
  }
}

// for (u32 i = 0; i < cfg.nblocks; i++) {
//   codeblock* blk = &cfg.blocks[i];

//   fprintf(stderr, "Block %i:\n", i);
//   for (u32 j = blk->start; j <= blk->end; j++) {
//     fprintf(stderr, "%i: ", j);
//     kit_print_instruction(fork.instructions[j], stderr);
//   }

//   fprintf(stderr, "\nBlock points to: [");
//   for (u32 j = 0; j < blk->nsuccessors; j++) { fprintf(stderr, "%u, ", blk->successors[j]); }
//   fprintf(stderr, "]\n");

//   fprintf(stderr, "Block %i end\n\n", i);
// }

static inline void
kit_print_instruction_stream(const kit_compilation_result* r, const kit_ins* ins, u32 nins, int indent, FILE* f)
{
  for (u32 ip = 0; ip < nins; ip++) {
    u32 instruction_offset = ip;

    for (int j = 0; j < indent; j++) fputc(' ', stdout);

    fprintf(f, "%-4u: ", instruction_offset); // Print offset of instruction
    kit_print_instruction(ins[ip], r, f);
  }
}

int
main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "edc: [input_binary]\n");
    return -1;
  }

  FILE* f = NULL;

  const char* bin_file = argv[1];
  if (strcmp(bin_file, "-") == 0) {
    f = stdin;
  } else {
    f = fopen(bin_file, "r");
    if (!f) {
      perror("edc: Failed to open input file");
      return -1;
    }
  }

  void*                  root_allocation = NULL;
  kit_compilation_result r               = { 0 };

  int e = kit_file_load(&r, &root_allocation, f);
  if (e) {
    fprintf(stderr, "eexec: Failed to parse input file: %i\n", e);
    return -1;
  }

  kit_print_instruction_stream(&r, r.instructions, r.instructions_count, 0, stdout);
  for (u32 i = 0; i < r.functions_count; i++) {
    const ecc_function* func = &r.functions[i];
    fprintf(stdout, "[%s|%u](%u):\n", lookup(&r, func->name_hash), func->name_hash, func->nargs);
    kit_print_instruction_stream(&r, func->code, func->code_count, 4, stdout);
  }

  fprintf(stdout, "\nstructures:\n");
  for (u32 i = 0; i < r.structs_count; i++) {
    const ecc_struct_information* info = &r.structs[i];
    fprintf(stdout, "[%s] = {", info->name);
    for (u32 j = 0; j < info->fields_count; j++) {
      fprintf(stdout, "%s/%u", info->field_names[j], info->field_hashes[j]);
      if (j != info->fields_count - 1) { fprintf(stdout, ", "); }
    }
    fprintf(stdout, "},\n");
  }
  fprintf(stdout, "\n");

  fprintf(stdout, "literals:\n");
  for (u32 i = 0; i < r.literals_count; i++) {
    fprintf(stdout, "[%u | %u] = ", i, kit_var_hash(&r.literals[i]));
    kit_var_print(&r.literals[i], stdout);
    fputc('\n', stdout);
  }

  if (strcmp(bin_file, "-") != 0) fclose(f);

  free(root_allocation);
  return 0;
}