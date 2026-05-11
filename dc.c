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

#include "cc.h"
#include "fn.h"
#include "rwhelp.h"
#include "stdafx.h"
#include "var.h"

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

static inline void
e_print_instruction(e_ins i, const e_compilation_result* r)
{
  switch ((e_opcode_bck)i.opcode) {
    case E_OPCODE_NOOP: printf("noop\n"); break;

    case E_OPCODE_ADD: printf("add\n"); break;
    case E_OPCODE_SUB: printf("sub\n"); break;
    case E_OPCODE_MUL: printf("mul\n"); break;
    case E_OPCODE_DIV: printf("div\n"); break;
    case E_OPCODE_MOD: printf("mod\n"); break;
    case E_OPCODE_EXP: printf("exp\n"); break;
    case E_OPCODE_AND: printf("and\n"); break;
    case E_OPCODE_OR: printf("or\n"); break;
    case E_OPCODE_XOR: printf("xor\n"); break;
    case E_OPCODE_BAND: printf("band\n"); break;
    case E_OPCODE_BOR: printf("bor\n"); break;
    case E_OPCODE_EQL: printf("eql\n"); break;
    case E_OPCODE_NEQ: printf("neq\n"); break;
    case E_OPCODE_LT: printf("lt\n"); break;
    case E_OPCODE_LTE: printf("lte\n"); break;
    case E_OPCODE_GT: printf("gt\n"); break;
    case E_OPCODE_GTE: printf("gte\n"); break;

    case E_OPCODE_NOT: printf("not\n"); break;
    case E_OPCODE_BNOT: printf("bnot\n"); break;
    case E_OPCODE_NEG: printf("neg\n"); break;

    case E_OPCODE_INC: {
      printf("inc\n");
      break;
    }
    case E_OPCODE_DEC: {
      printf("dec\n");
      break;
    }
    case E_OPCODE_CALL: {
      printf("call [%s|%u] [%u]\n", lookup(r, i.v.call.hash), i.v.call.hash, i.v.call.nargs);
      break;
    }
    case E_OPCODE_RETURN: {
      printf("ret [%s]\n", i.v.has_return_value ? "true" : "false");
      break;
    }
    case E_OPCODE_LITERAL: printf("literal [%u]\n", i.v.literal); break;
    case E_OPCODE_LOAD: printf("load %u\n", i.v.load); break;
    // case E_OPCODE_LOAD_REFERENCE: printf("load_reference\n"); break;
    case E_OPCODE_ASSIGN: printf("assign [%s|%u]\n", lookup(r, i.v.assign), i.v.assign); break;
    case E_OPCODE_INIT: printf("init [%s|%u]\n", lookup(r, i.v.init), i.v.init); break;
    case E_OPCODE_LABEL: printf("label [%u]\n", i.v.label); break;
    case E_OPCODE_JMP: printf("jmp [%u]\n", i.v.jmp); break;
    case E_OPCODE_JE: printf("je [%u]\n", i.v.jmp); break;
    case E_OPCODE_JNE: printf("jne [%u]\n", i.v.jmp); break;
    case E_OPCODE_JZ: printf("jz [%u]\n", i.v.jmp); break;
    case E_OPCODE_JNZ: printf("jnz [%u]\n", i.v.jmp); break;
    case E_OPCODE_PUSH_FRAME: printf("save vars\n"); break;
    case E_OPCODE_POP_FRAME: printf("restore vars\n"); break;
    case E_OPCODE_HALT: printf("halt [%u]\n", i.v.halt); break;
    case E_OPCODE_MK_LIST: {
      printf("mklist [%u]\n", i.v.mk_list);
      break;
    }
    case E_OPCODE_MK_MAP: {
      printf("mkmap [%u]\n", i.v.mk_map);
      break;
    }
    case E_OPCODE_INDEX: printf("index\n"); break;
    case E_OPCODE_POP: printf("pop\n"); break;
    case E_OPCODE_INDEX_ASSIGN: printf("idx_assign\n"); break;
    case E_OPCODE_MEMBER_ACCESS: printf("member_access [%u]\n", i.v.member); break;
    case E_OPCODE_MK_STRUCT: {
      printf("mk_struct [%s|%u]\n", lookup(r, i.v.mk_struct), i.v.mk_struct);
      break;
    }
    case E_OPCODE_MEMBER_ASSIGN: {
      printf("member_assign [%u]\n", i.v.member);
      break;
    }
    case E_OPCODE_DUP: printf("dup\n"); break;
    case E_OPCODE_INDEX_PEEK: printf("idx_peek\n"); break;

    case E_OPCODE_COUNT:
    default: printf("unknown\n");
  }
}

static inline void
e_print_instruction_stream(const e_compilation_result* r, const u8* stm, u32 stm_size, int indent)
{
  const u8* ip  = stm;
  const u8* end = stm + stm_size;
  while (ip < end) {
    u32 instruction_offset = ip - stm;

    e_ins ins = e_read_ins(&ip);

    for (int j = 0; j < indent; j++) fputc(' ', stdout);

    printf("%-4u: ", instruction_offset); // Print offset of instruction
    e_print_instruction(ins, r);
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

  void*                root_allocation = nullptr;
  e_compilation_result r               = { 0 };

  int e = e_file_load(&r, &root_allocation, f);
  if (e) {
    fprintf(stderr, "eexec: Failed to parse input file: %i\n", e);
    return -1;
  }

  e_print_instruction_stream(&r, (const u8*)r.instructions, r.instructions_count, 0);
  for (u32 i = 0; i < r.functions_count; i++) {
    const e_function* f = &r.functions[i];
    printf("[%s|%u](%u):\n", lookup(&r, f->name_hash), f->name_hash, f->nargs);
    e_print_instruction_stream(&r, (const u8*)f->code, f->code_size, 4);
  }

  printf("structures:\n");
  for (u32 i = 0; i < r.structs_count; i++) {
    const ecc_struct_information* info = &r.structs[i];
    printf("[%s/%u] = {", lookup(&r, info->name_hash), info->name_hash);
    for (u32 j = 0; j < info->fields_count; j++) {
      printf("%s/%u", lookup(&r, info->field_hashes[j]), info->field_hashes[j]);
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