#include "../inc/kit.cc.h"
#include "../inc/kit.ir.h"
#include "../inc/kit.reg.h"
#include "../inc/kit.rwhelp.h"
#include "../inc/kit.stdafx.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define write(ptr, f) fwrite(ptr, sizeof(*(ptr)), 1, f)
#define write_reg(ptr, f) fwrite(ptr, KIT_REG_DISK_SIZE, 1, f)
#define read(ptr, f) fread(ptr, sizeof(*(ptr)), 1, f)
#define read_reg(ptr, f) fread(ptr, KIT_REG_DISK_SIZE, 1, f)

kit_ins
read_ins(FILE* f)
{
  kit_ins i = { 0 };
  read(&i.opcode, f);
  switch ((kit_ir_opcode_bits)i.opcode) {
    case KIT_IR_OPCODE_LOADK: {
      read_reg(&i.loadk.dst, f);
      read(&i.loadk.id, f);
      break;
    }
    case KIT_IR_OPCODE_MOVG:
    case KIT_IR_OPCODE_GETG:
    case KIT_IR_OPCODE_SETG:
    case KIT_IR_OPCODE_MOV: {
      read_reg(&i.mov.dst, f);
      read_reg(&i.mov.src, f);
      break;
    }

    case KIT_IR_OPCODE_ASSERT: {
      read_reg(&i.assertion.cond, f);
      read(&i.assertion.line_id, f);
      break;
    }

    case KIT_IR_OPCODE_MOVI: {
      read_reg(&i.movi.dst, f);
      read(&i.movi.value, f);
      break;
    }

    case KIT_IR_OPCODE_MOVF: {
      read_reg(&i.movf.dst, f);
      read(&i.movf.value, f);
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
    case KIT_IR_OPCODE_EQL:
    case KIT_IR_OPCODE_NEQ:
    case KIT_IR_OPCODE_LT:
    case KIT_IR_OPCODE_LTE:
    case KIT_IR_OPCODE_GT:
    case KIT_IR_OPCODE_GTE: {
      read_reg(&i.binop.dst, f);
      read_reg(&i.binop.a, f);
      read_reg(&i.binop.b, f);
      break;
    }

    case KIT_IR_OPCODE_INC:
    case KIT_IR_OPCODE_DEC:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_NEG:
    case KIT_IR_OPCODE_NOT: {
      read_reg(&i.unop.dst, f);
      read_reg(&i.unop.a, f);
      break;
    }

    case KIT_IR_OPCODE_RET: {
      read_reg(&i.ret.return_value, f);
      break;
    }
    case KIT_IR_OPCODE_NOP: break;

    case KIT_IR_OPCODE_MK_LIST: {
      read_reg(&i.mk_list.dst, f);
      read(&i.mk_list.nelems, f);
      break;
    }
    case KIT_IR_OPCODE_MK_MAP: {
      read_reg(&i.mk_map.dst, f);
      read(&i.mk_map.npairs, f);
      break;
    }
    case KIT_IR_OPCODE_INDEX: {
      read_reg(&i.index.dst, f);
      read_reg(&i.index.base, f);
      read_reg(&i.index.index, f);
      break;
    }
    case KIT_IR_OPCODE_INDEX_ASSIGN: {
      read_reg(&i.index_assign.value, f);
      read_reg(&i.index_assign.base, f);
      read_reg(&i.index_assign.index, f);
      break;
    }
    case KIT_IR_OPCODE_CALL: {
      read_reg(&i.call.dst, f);
      read(&i.call.function_id, f);
      read(&i.call.nargs, f);
      break;
    }
    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ: {
      read(&i.cj.target, f);
      read_reg(&i.cj.condition, f);
      break;
    }
    case KIT_IR_OPCODE_JMP: {
      read(&i.jmp.target, f);
      break;
    }

    case KIT_IR_OPCODE_LABEL: {
      read(&i.label.id, f);
      break;
    }

    case KIT_IR_OPCODE_MEMBER_ACCESS: {
      read_reg(&i.member_access.dst, f);
      read_reg(&i.member_access.base, f);
      read(&i.member_access.member_id, f);
      break;
    }
    case KIT_IR_OPCODE_MEMBER_ASSIGN: {
      read_reg(&i.member_assign.value, f);
      read_reg(&i.member_assign.base, f);
      read(&i.member_assign.member_id, f);
      break;
    }
    case KIT_IR_OPCODE_MK_STRUCT: {
      read_reg(&i.mk_struct.dst, f);
      read(&i.mk_struct.struct_id, f);
      break;
    }
    case KIT_IR_OPCODE_PUSH: {
      read_reg(&i.push.reg, f);
      break;
    }
    case KIT_IR_OPCODE_POP: {
      read_reg(&i.pop.reg, f);
      break;
    }
  }
  return i;
}

static void
write_ins(kit_ins i, FILE* f)
{
  write(&i.opcode, f);
  switch ((kit_ir_opcode_bits)i.opcode) {
    case KIT_IR_OPCODE_LOADK: {
      write_reg(&i.loadk.dst, f);
      write(&i.loadk.id, f);
      break;
    }
    case KIT_IR_OPCODE_MOVG:
    case KIT_IR_OPCODE_GETG:
    case KIT_IR_OPCODE_SETG:
    case KIT_IR_OPCODE_MOV: {
      write_reg(&i.mov.dst, f);
      write_reg(&i.mov.src, f);
      break;
    }

    case KIT_IR_OPCODE_ASSERT: {
      write_reg(&i.assertion.cond, f);
      write(&i.assertion.line_id, f);
      break;
    }

    case KIT_IR_OPCODE_MOVI: {
      write_reg(&i.movi.dst, f);
      write(&i.movi.value, f);
      break;
    }

    case KIT_IR_OPCODE_MOVF: {
      write_reg(&i.movf.dst, f);
      write(&i.movf.value, f);
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
    case KIT_IR_OPCODE_EQL:
    case KIT_IR_OPCODE_NEQ:
    case KIT_IR_OPCODE_LT:
    case KIT_IR_OPCODE_LTE:
    case KIT_IR_OPCODE_GT:
    case KIT_IR_OPCODE_GTE: {
      write_reg(&i.binop.dst, f);
      write_reg(&i.binop.a, f);
      write_reg(&i.binop.b, f);
      break;
    }

    case KIT_IR_OPCODE_INC:
    case KIT_IR_OPCODE_DEC:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_NEG:
    case KIT_IR_OPCODE_NOT: {
      write_reg(&i.unop.dst, f);
      write_reg(&i.unop.a, f);
      break;
    }

    case KIT_IR_OPCODE_RET: {
      write_reg(&i.ret.return_value, f);
      break;
    }
    case KIT_IR_OPCODE_NOP: break;

    case KIT_IR_OPCODE_MK_LIST: {
      write_reg(&i.mk_list.dst, f);
      write(&i.mk_list.nelems, f);
      break;
    }
    case KIT_IR_OPCODE_MK_MAP: {
      write_reg(&i.mk_map.dst, f);
      write(&i.mk_map.npairs, f);
      break;
    }
    case KIT_IR_OPCODE_INDEX: {
      write_reg(&i.index.dst, f);
      write_reg(&i.index.base, f);
      write_reg(&i.index.index, f);
      break;
    }
    case KIT_IR_OPCODE_INDEX_ASSIGN: {
      write_reg(&i.index_assign.value, f);
      write_reg(&i.index_assign.base, f);
      write_reg(&i.index_assign.index, f);
      break;
    }
    case KIT_IR_OPCODE_CALL: {
      write_reg(&i.call.dst, f);
      write(&i.call.function_id, f);
      write(&i.call.nargs, f);
      break;
    }
    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ: {
      write(&i.cj.target, f);
      write_reg(&i.cj.condition, f);
      break;
    }
    case KIT_IR_OPCODE_JMP: {
      write(&i.jmp.target, f);
      break;
    }

    case KIT_IR_OPCODE_LABEL: {
      write(&i.label.id, f);
      break;
    }

    case KIT_IR_OPCODE_MEMBER_ACCESS: {
      write_reg(&i.member_access.dst, f);
      write_reg(&i.member_access.base, f);
      write(&i.member_access.member_id, f);
      break;
    }
    case KIT_IR_OPCODE_MEMBER_ASSIGN: {
      write_reg(&i.member_assign.value, f);
      write_reg(&i.member_assign.base, f);
      write(&i.member_assign.member_id, f);
      break;
    }
    case KIT_IR_OPCODE_MK_STRUCT: {
      write_reg(&i.mk_struct.dst, f);
      write(&i.mk_struct.struct_id, f);
      break;
    }
    case KIT_IR_OPCODE_PUSH: {
      write_reg(&i.push.reg, f);
      break;
    }
    case KIT_IR_OPCODE_POP: {
      write_reg(&i.pop.reg, f);
      break;
    }
  }
}

void
kit_file_write(const kit_compilation_result* r, FILE* f)
{
  const u32 magic = KIT_FILE_MAGIC;
  fwrite(&magic, sizeof(magic), 1, f);

  u32 bytes_req = kit_file_bytes_required(r);
  fwrite(&bytes_req, sizeof(bytes_req), 1, f);

  fwrite(&r->literals_count, sizeof(r->literals_count), 1, f);
  for (u32 i = 0; i < r->literals_count; i++) {
    const kit_var* lit = &r->literals[i];
    fwrite(&lit->type, sizeof(lit->type), 1, f);

    if (lit->type == KIT_VARTYPE_STRING) {
      u32 len = strlen(KIT_VAR_AS_STRING(lit)->s);
      fwrite(&len, sizeof(len), 1, f);
      fwrite(KIT_VAR_AS_STRING(lit)->s, sizeof(char), len, f);
    } else {
      switch (lit->type) {
        case KIT_VARTYPE_INT: fwrite(&lit->val.i, sizeof(lit->val.i), 1, f); break;
        case KIT_VARTYPE_BOOL: fwrite(&lit->val.b, sizeof(lit->val.b), 1, f); break;
        case KIT_VARTYPE_CHAR: fwrite(&lit->val.c, sizeof(lit->val.c), 1, f); break;
        case KIT_VARTYPE_FLOAT: fwrite(&lit->val.f, sizeof(lit->val.f), 1, f); break;
        case KIT_VARTYPE_VEC2: fwrite(&lit->val.vec2, sizeof(lit->val.vec2), 1, f); break;
        case KIT_VARTYPE_VEC3: fwrite(&lit->val.vec3, sizeof(lit->val.vec3), 1, f); break;
        case KIT_VARTYPE_VEC4: fwrite(&lit->val.vec4, sizeof(lit->val.vec4), 1, f); break;
        default: break;
      }
    }
  }

  fwrite(&r->structs_count, sizeof(u32), 1, f);
  for (u32 i = 0; i < r->structs_count; i++) {
    kitc_struct_information* st = &r->structs[i];

    u32 name_len = strlen(st->name);
    fwrite(&name_len, sizeof(u32), 1, f);
    fwrite(st->name, name_len, 1, f);

    fwrite(&st->fields_count, sizeof(u32), 1, f);
    fwrite(st->field_hashes, sizeof(u32), st->fields_count, f);
    for (u32 j = 0; j < st->fields_count; j++) {
      u32 len = (u32)strlen(st->field_names[j]);
      fwrite(&len, sizeof(u32), 1, f);
      fwrite(st->field_names[j], 1, len, f);
    }
  }

  fwrite(&r->functions_count, sizeof(r->functions_count), 1, f);
  for (u32 i = 0; i < r->functions_count; i++) {
    const kitc_function* fn = &r->functions[i];
    fwrite(&fn->code_count, sizeof(fn->code_count), 1, f);
    fwrite(&fn->nargs, sizeof(fn->nargs), 1, f);
    fwrite(&fn->name_hash, sizeof(fn->name_hash), 1, f);
    // fwrite(fn->code, sizeof(kit_ins), fn->code_count, f);
    for (u32 j = 0; j < fn->code_count; j++) { write_ins(fn->code[j], f); }
  }

  fwrite(&r->instructions_count, sizeof(r->instructions_count), 1, f);
  for (u32 j = 0; j < r->instructions_count; j++) { write_ins(r->instructions[j], f); }

  // fwrite(r->instructions, sizeof(kit_ins), r->instructions_count, f);

  fwrite(&r->names_count, sizeof(r->names_count), 1, f);
  for (u32 i = 0; i < r->names_count; i++) {
    const char* sym = r->names[i];
    u32         len = strlen(sym);
    fwrite(&len, sizeof(len), 1, f);
    fwrite(sym, len, 1, f);
  }
}

kit_file_read_error
kit_file_load(kit_compilation_result* r, void** root_allocation, FILE* f)
{
  *root_allocation = NULL;

  memset(r, 0, sizeof(kit_compilation_result));

  u32 magic = 0;
  if (fread(&magic, sizeof(magic), 1, f) != 1) goto ERR;

  if (magic != KIT_FILE_MAGIC) return KIT_FILE_READ_ERR_INVALID_MAGIC;

  u32 bytes_req = 0;
  if (fread(&bytes_req, sizeof(bytes_req), 1, f) != 1) goto ERR;

  if (bytes_req > INT32_MAX) {
    fprintf(stderr, "[%s:%i] Warning: Loading a file with requested size > INT32_MAX. Possible file corruption. Continuing.\n", __FILE__, __LINE__);
  }

  *root_allocation = kit_xalloc(bytes_req, 1);
  if (*root_allocation == NULL) return KIT_FILE_READ_ERR_ROOT_ALLOCATION_FAILED;

  uchar* alloc = (uchar*)*root_allocation;

  if (fread(&r->literals_count, sizeof(r->literals_count), 1, f) != 1) goto ERR;

  alloc       = kit_align_ptr(alloc, 8);
  r->literals = (kit_var*)alloc;
  alloc += sizeof(kit_var) * r->literals_count;

  alloc              = kit_align_ptr(alloc, 8);
  r->literals_hashes = (u32*)alloc;
  alloc += sizeof(u32) * r->literals_count;

  for (u32 i = 0; i < r->literals_count; i++) {
    if (fread(&r->literals[i].type, sizeof(kit_var_type), 1, f) != 1) goto ERR;

    kit_var* lit = &r->literals[i];
    switch (lit->type) {
      case KIT_VARTYPE_STRING: {
        u32 len = 0;
        if (fread(&len, sizeof(len), 1, f) != 1) goto ERR;

        alloc      = kit_align_ptr(alloc, 16);
        lit->val.s = (kit_refdobj*)alloc;
        alloc += sizeof(kit_refdobj);

        /* Initialize ref counter to 1. They are immortal for the length of the program. */
        lit->val.s->refc = 1;

        KIT_VAR_AS_STRING(lit)->s = (char*)alloc;
        alloc += len + 1;

        if (fread(KIT_VAR_AS_STRING(lit)->s, sizeof(char), len, f) != len) goto ERR;

        KIT_VAR_AS_STRING(lit)->s[len] = 0;
        break;
      }

        // clang-format off
      case KIT_VARTYPE_NULL:
      case KIT_VARTYPE_STRUCT: *lit = (kit_var){ .type = KIT_VARTYPE_NULL }; break;
      case KIT_VARTYPE_INT: if (fread(&lit->val.i, sizeof(lit->val.i), 1, f) != 1) goto ERR; break;
      case KIT_VARTYPE_BOOL: if (fread(&lit->val.b, sizeof(lit->val.b), 1, f) != 1) goto ERR; break;
      case KIT_VARTYPE_CHAR: if (fread(&lit->val.c, sizeof(lit->val.c), 1, f) != 1) goto ERR; break;
      case KIT_VARTYPE_FLOAT: if (fread(&lit->val.f, sizeof(lit->val.f), 1, f) != 1) goto ERR; break;
      case KIT_VARTYPE_VEC2: if (fread(lit->val.vec2, sizeof(lit->val.vec2), 1, f) != 1) goto ERR; break;
      case KIT_VARTYPE_VEC3: if (fread(lit->val.vec3, sizeof(lit->val.vec3), 1, f) != 1) goto ERR; break;
      case KIT_VARTYPE_VEC4: if (fread(lit->val.vec4, sizeof(lit->val.vec4), 1, f) != 1) goto ERR; break;
      default: break;
        // clang-format on
    }
    r->literals_hashes[i] = kit_var_hash(lit);
  }

  if (fread(&r->structs_count, sizeof(u32), 1, f) != 1) goto ERR;

  alloc      = kit_align_ptr(alloc, 8);
  r->structs = (kitc_struct_information*)alloc;
  alloc += sizeof(kitc_struct_information) * r->structs_count;

  for (u32 i = 0; i < r->structs_count; i++) {
    kitc_struct_information* st = &r->structs[i];

    st->name = (char*)alloc;

    u32 name_len = 0;
    if (fread(&name_len, sizeof(u32), 1, f) != 1) goto ERR;
    alloc += name_len + 1;

    if (fread(st->name, name_len, 1, f) != 1) goto ERR;
    st->name[name_len] = 0;

    st->name_hash = kit_hash(st->name, strlen(st->name));

    if (fread(&st->fields_count, sizeof(u32), 1, f) != 1) goto ERR;

    alloc            = kit_align_ptr(alloc, 4);
    st->field_hashes = (u32*)alloc;
    alloc += sizeof(u32) * st->fields_count;

    if (fread(r->structs[i].field_hashes, sizeof(u32), st->fields_count, f) != st->fields_count) goto ERR;

    alloc           = kit_align_ptr(alloc, 8);
    st->field_names = (char**)alloc;
    alloc += sizeof(const char*) * st->fields_count;

    for (u32 j = 0; j < st->fields_count; j++) {
      u32 len = 0;
      if (fread(&len, sizeof(u32), 1, f) != 1) goto ERR;

      alloc              = kit_align_ptr(alloc, 8);
      st->field_names[j] = (char*)alloc;
      alloc += len + 1;

      if (fread(st->field_names[j], len, 1, f) != 1) goto ERR;
      st->field_names[j][len] = 0;
    }
  }

  if (fread(&r->functions_count, sizeof(r->functions_count), 1, f) != 1) goto ERR;

  alloc        = kit_align_ptr(alloc, 8);
  r->functions = (kitc_function*)alloc;
  alloc += sizeof(kitc_function) * (r->functions_count);

  for (u32 i = 0; i < r->functions_count; i++) {
    kitc_function func = { 0 };
    if (fread(&func.code_count, sizeof(func.code_count), 1, f) != 1) goto ERR;
    if (fread(&func.nargs, sizeof(func.nargs), 1, f) != 1) goto ERR;
    if (fread(&func.name_hash, sizeof(func.name_hash), 1, f) != 1) goto ERR;

    alloc = kit_align_ptr(alloc, 16);

    func.code = (kit_ins*)alloc;
    alloc += func.code_count * sizeof(kit_ins);
    if (func.code == NULL) return -1;

    for (u32 j = 0; j < func.code_count; j++) { func.code[j] = read_ins(f); }

    r->functions[i] = func;
  }

  if (fread(&r->instructions_count, sizeof(r->instructions_count), 1, f) != 1) goto ERR;

  alloc           = kit_align_ptr(alloc, 16);
  r->instructions = (kit_ins*)alloc;
  // if (fread(r->instructions, sizeof(kit_ins), r->instructions_count, f) != r->instructions_count) goto ERR;
  for (u32 j = 0; j < r->instructions_count; j++) { r->instructions[j] = read_ins(f); }

  alloc += r->instructions_count * sizeof(kit_ins);
  alloc = kit_align_ptr(alloc, 4);

  if (fread(&r->names_count, sizeof(r->names_count), 1, f) != 1) goto ERR;

  alloc    = kit_align_ptr(alloc, 8);
  r->names = (char**)alloc;
  alloc += sizeof(char*) * r->names_count;

  alloc           = kit_align_ptr(alloc, 8);
  r->names_hashes = (u32*)alloc;
  alloc += sizeof(u32) * r->names_count;

  for (u32 i = 0; i < r->names_count; i++) {
    u32 len = 0;

    if (fread(&len, sizeof(len), 1, f) != 1) goto ERR;

    alloc = kit_align_ptr(alloc, 8);

    char* name = (char*)alloc;
    alloc += len + 1;

    if (fread(name, sizeof(char), len, f) != len) goto ERR;
    name[len] = 0;

    r->names[i]        = name;
    r->names_hashes[i] = kit_hash(name, len);
  }

  return KIT_FILE_READ_SUCCESS;

ERR:
  free(*root_allocation);
  *root_allocation = NULL;
  return KIT_FILE_READ_ERR_INVALID_FILE;
}

u32
kit_file_bytes_required(const kit_compilation_result* r)
{
  u32 size = 0;

  size = kit_align_size(size, 8);
  // literals array
  size += sizeof(kit_var) * r->literals_count;

  size = kit_align_size(size, 8);
  // literal hashes array (we don't write this tho)
  size += sizeof(u32) * r->literals_count;

  for (u32 i = 0; i < r->literals_count; i++) {
    const kit_var* lit = &r->literals[i];

    if (lit->type == KIT_VARTYPE_STRING) {
      u32 len = strlen(KIT_VAR_AS_STRING(lit)->s);

      size = kit_align_size(size, 16);
      size += sizeof(kit_refdobj);
      size += len + 1; // null terminator
    }
  }

  // structures array
  size = kit_align_size(size, 8);
  size += sizeof(kitc_struct_information) * r->structs_count;
  for (u32 i = 0; i < r->structs_count; i++) {
    u32 fields_count = r->structs[i].fields_count;
    size += strlen(r->structs[i].name) + 1; // NULL term
    size = kit_align_size(size, 4);
    size += fields_count * sizeof(u32);
    size = kit_align_size(size, 8);
    size += fields_count * sizeof(char*);
    for (u32 j = 0; j < fields_count; j++) {
      size = kit_align_size(size, 8);
      size += strlen(r->structs[i].field_names[j]) + 1;
    }
  }

  // functions array
  size = kit_align_size(size, 8);
  size += sizeof(kitc_function) * r->functions_count;
  for (u32 i = 0; i < r->functions_count; i++) {
    const kitc_function* fn = &r->functions[i];

    size = kit_align_size(size, 16);
    size += fn->code_count * sizeof(kit_ins); // code
  }

  size = kit_align_size(size, 16);
  size += r->instructions_count * sizeof(kit_ins);

  size = kit_align_size(size, 4);
  size += sizeof(r->names_count);

  size = kit_align_size(size, 8);
  size += sizeof(char*) * r->names_count;

  size = kit_align_size(size, 8);
  size += sizeof(u32) * r->names_count;

  for (u32 i = 0; i < r->names_count; i++) {
    size = kit_align_size(size, 8);
    size += strlen(r->names[i]) + 1;
  }

  return size;
}

void
kit_emit_ins(kit_compiler* cc, kit_ins ins)
{
  if (cc->ninstructions >= cc->cinstructions) kitc_stream_resize(cc, cc->cinstructions * 2U);
  memcpy(&cc->instructions[cc->ninstructions++], &ins, sizeof(kit_ins));
}

#define read_reg_ip(ptr, ip)                                                                                                                         \
  do {                                                                                                                                               \
    memcpy((void*)(ptr), (void*)(ip), KIT_REG_DISK_SIZE);                                                                                            \
    *(ip) += sizeof(*(ptr));                                                                                                                         \
  } while (0)

#define read_ip(ptr, ip)                                                                                                                             \
  do {                                                                                                                                               \
    memcpy((void*)(ptr), (void*)(ip), sizeof(*(ptr)));                                                                                               \
    *(ip) += sizeof(*(ptr));                                                                                                                         \
  } while (0)

kit_ins
kit_read_ins(const u8** ip)
{
  kit_ins i = { 0 };
  read_ip(&i.opcode, ip);
  switch ((kit_ir_opcode_bits)i.opcode) {
    case KIT_IR_OPCODE_LOADK: {
      read_reg_ip(&i.loadk.dst, ip);
      read_ip(&i.loadk.id, ip);
      break;
    }
    case KIT_IR_OPCODE_MOVG:
    case KIT_IR_OPCODE_GETG:
    case KIT_IR_OPCODE_SETG:
    case KIT_IR_OPCODE_MOV: {
      read_reg_ip(&i.mov.dst, ip);
      read_reg_ip(&i.mov.src, ip);
      break;
    }

    case KIT_IR_OPCODE_ASSERT: {
      read_reg_ip(&i.assertion.cond, ip);
      read_ip(&i.assertion.line_id, ip);
      break;
    }

    case KIT_IR_OPCODE_MOVI: {
      read_reg_ip(&i.movi.dst, ip);
      read_ip(&i.movi.value, ip);
      break;
    }

    case KIT_IR_OPCODE_MOVF: {
      read_reg_ip(&i.movf.dst, ip);
      read_ip(&i.movf.value, ip);
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
    case KIT_IR_OPCODE_EQL:
    case KIT_IR_OPCODE_NEQ:
    case KIT_IR_OPCODE_LT:
    case KIT_IR_OPCODE_LTE:
    case KIT_IR_OPCODE_GT:
    case KIT_IR_OPCODE_GTE: {
      read_reg_ip(&i.binop.dst, ip);
      read_reg_ip(&i.binop.a, ip);
      read_reg_ip(&i.binop.b, ip);
      break;
    }

    case KIT_IR_OPCODE_INC:
    case KIT_IR_OPCODE_DEC:
    case KIT_IR_OPCODE_BNOT:
    case KIT_IR_OPCODE_NEG:
    case KIT_IR_OPCODE_NOT: {
      read_reg_ip(&i.unop.dst, ip);
      read_reg_ip(&i.unop.a, ip);
      break;
    }

    case KIT_IR_OPCODE_RET: {
      read_reg_ip(&i.ret.return_value, ip);
      break;
    }
    case KIT_IR_OPCODE_NOP: break;

    case KIT_IR_OPCODE_MK_LIST: {
      read_reg_ip(&i.mk_list.dst, ip);
      read_ip(&i.mk_list.nelems, ip);
      break;
    }
    case KIT_IR_OPCODE_MK_MAP: {
      read_reg_ip(&i.mk_map.dst, ip);
      read_ip(&i.mk_map.npairs, ip);
      break;
    }
    case KIT_IR_OPCODE_INDEX: {
      read_reg_ip(&i.index.dst, ip);
      read_reg_ip(&i.index.base, ip);
      read_reg_ip(&i.index.index, ip);
      break;
    }
    case KIT_IR_OPCODE_INDEX_ASSIGN: {
      read_reg_ip(&i.index_assign.value, ip);
      read_reg_ip(&i.index_assign.base, ip);
      read_reg_ip(&i.index_assign.index, ip);
      break;
    }
    case KIT_IR_OPCODE_CALL: {
      read_reg_ip(&i.call.dst, ip);
      read_ip(&i.call.function_id, ip);
      read_ip(&i.call.nargs, ip);
      break;
    }
    case KIT_IR_OPCODE_JZ:
    case KIT_IR_OPCODE_JNZ: {
      read_ip(&i.cj.target, ip);
      read_reg_ip(&i.cj.condition, ip);
      break;
    }
    case KIT_IR_OPCODE_JMP: {
      read_ip(&i.jmp.target, ip);
      break;
    }

    case KIT_IR_OPCODE_LABEL: {
      read_ip(&i.label.id, ip);
      break;
    }

    case KIT_IR_OPCODE_MEMBER_ACCESS: {
      read_reg_ip(&i.member_access.dst, ip);
      read_reg_ip(&i.member_access.base, ip);
      read_ip(&i.member_access.member_id, ip);
      break;
    }
    case KIT_IR_OPCODE_MEMBER_ASSIGN: {
      read_reg_ip(&i.member_assign.value, ip);
      read_reg_ip(&i.member_assign.base, ip);
      read_ip(&i.member_assign.member_id, ip);
      break;
    }
    case KIT_IR_OPCODE_MK_STRUCT: {
      read_reg_ip(&i.mk_struct.dst, ip);
      read_ip(&i.mk_struct.struct_id, ip);
      break;
    }
    case KIT_IR_OPCODE_PUSH: {
      read_reg_ip(&i.push.reg, ip);
      break;
    }
    case KIT_IR_OPCODE_POP: {
      read_reg_ip(&i.pop.reg, ip);
      break;
    }
  }
  return i;
}
