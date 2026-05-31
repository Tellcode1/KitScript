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

#include "../inc/cc.h"
#include "../inc/reg.h"
#include "../inc/rwhelp.h"
#include "../inc/stdafx.h"
#include "../inc/var.h"

#include <stdio.h>
#include <string.h>

static inline const char*
lookup(const e_compilation_result* r, u32 name)
{
  for (u32 i = 0; i < r->names_count; i++) {
    if (r->names_hashes[i] == name) { return r->names[i]; }
  }
  return "[symbol not found]";
}

static inline const char*
get_register_name(u32 reg_id, char buff[32])
{
  if (reg_id >= E_REG_ARG0 && reg_id <= E_REG_ARG15) {
    snprintf(buff, 32, "arg%i", reg_id - E_REG_ARG0);
  } else if (reg_id >= E_REG_GENERAL_BEGIN && reg_id <= E_REG_GENERAL_END) {
    snprintf(buff, 32, "r%i", reg_id - E_REG_GENERAL_BEGIN);
  } else if (reg_id == E_REG_SP) {
    snprintf(buff, 32, "rsp");
  } else if (reg_id == E_REG_IP) {
    snprintf(buff, 32, "rip");
  } else {
    snprintf(buff, 32, "ru%i", reg_id);
  }
  return buff;
}

static inline void
e_print_instruction(e_ins i, const e_compilation_result* r)
{
  char buf0[32];
  char buf1[32];
  char buf2[32];
  switch (i.opcode) {
    case EIR_OPCODE_LOADK: {
      printf("loadk dst=%s, id=%u\n", get_register_name(i.loadk.dst, buf0), i.loadk.id);
      break;
    }
    case EIR_OPCODE_MOV: {
      printf("mov dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1));
      break;
    }

    case EIR_OPCODE_MOVI: {
      printf("movi dst=%s, value=%i\n", get_register_name(i.movi.dst, buf0), i.movi.value);
      break;
    }
    case EIR_OPCODE_MOVF: {
      printf("movf dst=%s, value=%f\n", get_register_name(i.movf.dst, buf0), i.movf.value);
      break;
    }

    case EIR_OPCODE_ADD:
      printf(
          "add dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_SUB:
      printf(
          "sub dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MUL:
      printf(
          "mul dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_DIV:
      printf(
          "div dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MOD:
      printf(
          "mod dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EXP:
      printf(
          "exp dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_AND:
      printf(
          "and dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_OR:
      printf("or dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BAND:
      printf(
          "band dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BOR:
      printf(
          "bor dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_XOR:
      printf(
          "xor dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EQL:
      printf(
          "eql dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_NEQ:
      printf(
          "neq dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LT:
      printf("lt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LTE:
      printf(
          "lte dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GT:
      printf("gt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GTE:
      printf(
          "gte dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;

    case EIR_OPCODE_INC: printf("inc dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_DEC: printf("dec dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_BNOT: printf("bnot dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NEG: printf("neg dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NOT: printf("not dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;

    case EIR_OPCODE_RET: {
      printf("ret val=%s\n", get_register_name(i.ret.return_value, buf0));
      break;
    }
    case EIR_OPCODE_NOP: printf("nop\n"); break;
    case EIR_OPCODE_MK_LIST: printf("mk_list dst=%s, nelems=%u\n", get_register_name(i.mk_list.dst, buf0), i.mk_list.nelems); break;
    case EIR_OPCODE_MK_MAP: printf("mk_map dst=%s, npairs=%u\n", get_register_name(i.mk_map.dst, buf0), i.mk_map.npairs); break;
    case EIR_OPCODE_INDEX:
      printf(
          "index dst=%s, base=%s, index=%s\n",
          get_register_name(i.index.dst, buf0),
          get_register_name(i.index.base, buf1),
          get_register_name(i.index.index, buf2));
      break;
    case EIR_OPCODE_CALL:
      printf(
          "call dst=%s, id=%u|name=%s, nargs=%u\n",
          get_register_name(i.call.dst, buf0),
          i.call.function_id,
          lookup(r, i.call.function_id),
          i.call.nargs);
      break;
    case EIR_OPCODE_INDEX_ASSIGN:
      printf(
          "index_assign value=%s, base=%s, index=%s\n",
          get_register_name(i.index_assign.value, buf0),
          get_register_name(i.index_assign.base, buf1),
          get_register_name(i.index_assign.index, buf2));
      break;

    case EIR_OPCODE_LABEL: printf("label id=%u\n", i.label.id); break;
    case EIR_OPCODE_JMP: printf("jmp target=%u\n", i.jmp.target); break;
    case EIR_OPCODE_JZ: printf("jz target=%u, condition=%s\n", i.jz.target, get_register_name(i.jz.condition, buf0)); break;
    case EIR_OPCODE_JNZ: printf("jnz target=%u, condition=%s\n", i.jnz.target, get_register_name(i.jnz.condition, buf0)); break;
    case EIR_OPCODE_MEMBER_ACCESS:
      printf(
          "member_access dst=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_access.dst, buf0),
          get_register_name(i.member_access.base, buf1),
          i.member_access.member_id);
      break;
    case EIR_OPCODE_MEMBER_ASSIGN:
      printf(
          "member_assign value=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_assign.value, buf0),
          get_register_name(i.member_assign.base, buf1),
          i.member_assign.member_id);
      break;
    case EIR_OPCODE_MK_STRUCT: printf("mk_struct dst=%s, id=%u\n", get_register_name(i.mk_struct.dst, buf0), i.mk_struct.struct_id); break;
    case EIR_OPCODE_GETG: printf("getg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_SETG: printf("setg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_MOVG: printf("movg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_PUSH: printf("push %s\n", get_register_name(i.push.reg, buf0)); break;
    case EIR_OPCODE_POP: printf("pop %s\n", get_register_name(i.pop.reg, buf0)); break;
  }
}

static inline void
e_print_instruction_stream(const e_compilation_result* r, const e_ins* ins, u32 nins, int indent)
{
  for (u32 ip = 0; ip < nins; ip++) {
    u32 instruction_offset = ip;

    for (int j = 0; j < indent; j++) fputc(' ', stdout);

    printf("%-4u: ", instruction_offset); // Print offset of instruction
    e_print_instruction(ins[ip], r);
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

  void*                root_allocation = NULL;
  e_compilation_result r               = { 0 };

  int e = e_file_load(&r, &root_allocation, f);
  if (e) {
    fprintf(stderr, "eexec: Failed to parse input file: %i\n", e);
    return -1;
  }

  e_print_instruction_stream(&r, r.instructions, r.instructions_count, 0);
  for (u32 i = 0; i < r.functions_count; i++) {
    const ecc_function* func = &r.functions[i];
    printf("[%s|%u](%u):\n", lookup(&r, func->name_hash), func->name_hash, func->nargs);
    e_print_instruction_stream(&r, func->code, func->code_count, 4);
  }

  printf("\nstructures:\n");
  for (u32 i = 0; i < r.structs_count; i++) {
    const ecc_struct_information* info = &r.structs[i];
    printf("[%s] = {", info->name);
    for (u32 j = 0; j < info->fields_count; j++) {
      printf("%s/%u", info->field_names[j], info->field_hashes[j]);
      if (j != info->fields_count - 1) { printf(", "); }
    }
    printf("},\n");
  }
  printf("\n");

  printf("literals:\n");
  for (u32 i = 0; i < r.literals_count; i++) {
    printf("[%u | %u] = ", i, e_var_hash(&r.literals[i]));
    e_var_print(&r.literals[i], stdout);
    fputc('\n', stdout);
  }

  if (strcmp(bin_file, "-") != 0) fclose(f);

  free(root_allocation);
  return 0;
}