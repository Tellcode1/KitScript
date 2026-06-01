#include "../inc/cc.h"
#include "../inc/ir.h"
#include "../inc/rwhelp.h"
#include "../inc/stdafx.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline u8
read_u8(FILE* f)
{
  u8 w = 0;
  fread(&w, sizeof(w), 1, f);
  return w;
}

static inline u32
read_u32(FILE* f)
{
  u32 w = 0;
  fread(&w, sizeof(w), 1, f);
  return w;
}

static inline u64
read_u64(FILE* f)
{
  u64 w = 0;
  fread(&w, sizeof(w), 1, f);
  return w;
}

e_ins
read_ins(FILE* f)
{
  e_ins i  = { 0 };
  i.opcode = read_u8(f);

  switch (i.opcode) {
    case EIR_OPCODE_LOADK: {
      i.loadk.dst = read_u32(f);
      i.loadk.id  = read_u32(f);
      break;
    }
    case EIR_OPCODE_MOVG:
    case EIR_OPCODE_GETG:
    case EIR_OPCODE_SETG:
    case EIR_OPCODE_MOV: {
      i.mov.dst = read_u32(f);
      i.mov.src = read_u32(f);
      break;
    }

    case EIR_OPCODE_MOVI: {
      i.movi.dst   = read_u32(f);
      i.movi.value = (int)read_u32(f);
      break;
    }

    case EIR_OPCODE_MOVF: {
      i.movf.dst = read_u32(f);
      u64 read   = read_u64(f);
      memcpy(&i.movf.value, &read, sizeof(double));
      break;
    }

    case EIR_OPCODE_ADD:
    case EIR_OPCODE_SUB:
    case EIR_OPCODE_MUL:
    case EIR_OPCODE_DIV:
    case EIR_OPCODE_MOD:
    case EIR_OPCODE_EXP:
    case EIR_OPCODE_AND:
    case EIR_OPCODE_OR:
    case EIR_OPCODE_BAND:
    case EIR_OPCODE_BOR:
    case EIR_OPCODE_XOR:
    case EIR_OPCODE_EQL:
    case EIR_OPCODE_NEQ:
    case EIR_OPCODE_LT:
    case EIR_OPCODE_LTE:
    case EIR_OPCODE_GT:
    case EIR_OPCODE_GTE: {
      i.binop.dst = read_u32(f);
      i.binop.a   = read_u32(f);
      i.binop.b   = read_u32(f);
      break;
    }

    case EIR_OPCODE_INC:
    case EIR_OPCODE_DEC:
    case EIR_OPCODE_BNOT:
    case EIR_OPCODE_NEG:
    case EIR_OPCODE_NOT: {
      i.unop.dst = read_u32(f);
      i.unop.a   = read_u32(f);
      break;
    }

    case EIR_OPCODE_RET: {
      i.ret.return_value = read_u32(f);
      break;
    }
    case EIR_OPCODE_NOP: break;

    case EIR_OPCODE_MK_LIST: {
      i.mk_list.dst    = read_u32(f);
      i.mk_list.nelems = read_u32(f);
      break;
    }
    case EIR_OPCODE_MK_MAP: {
      i.mk_map.dst    = read_u32(f);
      i.mk_map.npairs = read_u32(f);
      break;
    }
    case EIR_OPCODE_INDEX: {
      i.index.dst   = read_u32(f);
      i.index.base  = read_u32(f);
      i.index.index = read_u32(f);
      break;
    }
    case EIR_OPCODE_INDEX_ASSIGN: {
      i.index_assign.value = read_u32(f);
      i.index_assign.base  = read_u32(f);
      i.index_assign.index = read_u32(f);
      break;
    }
    case EIR_OPCODE_CALL: {
      i.call.dst         = read_u32(f);
      i.call.function_id = read_u32(f);
      i.call.nargs       = read_u32(f);
      break;
    }
    case EIR_OPCODE_JZ:
    case EIR_OPCODE_JNZ: {
      i.cj.target    = read_u32(f);
      i.cj.condition = read_u32(f);
      break;
    }
    case EIR_OPCODE_JMP: {
      i.jmp.target = read_u32(f);
      break;
    }

    case EIR_OPCODE_LABEL: {
      i.label.id = read_u32(f);
      break;
    }

    case EIR_OPCODE_MEMBER_ACCESS: {
      i.member_access.dst       = read_u32(f);
      i.member_access.base      = read_u32(f);
      i.member_access.member_id = read_u32(f);
      break;
    }
    case EIR_OPCODE_MEMBER_ASSIGN: {
      i.member_assign.value     = read_u32(f);
      i.member_assign.base      = read_u32(f);
      i.member_assign.member_id = read_u32(f);
      break;
    }
    case EIR_OPCODE_MK_STRUCT: {
      i.mk_struct.dst       = read_u32(f);
      i.mk_struct.struct_id = read_u32(f);
      break;
    }
    case EIR_OPCODE_PUSH: {
      i.push.reg = read_u32(f);
      break;
    }
    case EIR_OPCODE_POP: {
      i.pop.reg = read_u32(f);
      break;
    }
  }
  return i;
}

static void
write8(u8 x, FILE* f)
{ fwrite(&x, sizeof(x), 1, f); }

static void
write32(u32 x, FILE* f)
{ fwrite(&x, sizeof(x), 1, f); }

static void
writef(double flo, FILE* f)
{ fwrite(&flo, sizeof(flo), 1, f); }

static void
write_ins(e_ins i, FILE* f)
{
  write8(i.opcode, f);

  switch (i.opcode) {
    case EIR_OPCODE_LOADK: {
      write32(i.loadk.dst, f);
      write32(i.loadk.id, f);
      break;
    }
    case EIR_OPCODE_MOVI: {
      write32(i.movi.dst, f);
      write32(i.movi.value, f);
      break;
    }
    case EIR_OPCODE_MOVF: {
      write32(i.movf.dst, f);
      writef(i.movf.value, f);
      break;
    }
    case EIR_OPCODE_MOVG:
    case EIR_OPCODE_GETG:
    case EIR_OPCODE_SETG:
    case EIR_OPCODE_MOV: {
      write32(i.mov.dst, f);
      write32(i.mov.src, f);
      break;
    }

    case EIR_OPCODE_ADD:
    case EIR_OPCODE_SUB:
    case EIR_OPCODE_MUL:
    case EIR_OPCODE_DIV:
    case EIR_OPCODE_MOD:
    case EIR_OPCODE_EXP:
    case EIR_OPCODE_AND:
    case EIR_OPCODE_OR:
    case EIR_OPCODE_BAND:
    case EIR_OPCODE_BOR:
    case EIR_OPCODE_XOR:
    case EIR_OPCODE_EQL:
    case EIR_OPCODE_NEQ:
    case EIR_OPCODE_LT:
    case EIR_OPCODE_LTE:
    case EIR_OPCODE_GT:
    case EIR_OPCODE_GTE: {
      write32(i.binop.dst, f);
      write32(i.binop.a, f);
      write32(i.binop.b, f);
      break;
    }

    case EIR_OPCODE_INC:
    case EIR_OPCODE_DEC:
    case EIR_OPCODE_BNOT:
    case EIR_OPCODE_NEG:
    case EIR_OPCODE_NOT: {
      write32(i.unop.dst, f);
      write32(i.unop.a, f);
      break;
    }

    case EIR_OPCODE_RET: {
      write32(i.ret.return_value, f);
      break;
    }
    case EIR_OPCODE_NOP: break;

    case EIR_OPCODE_MK_LIST: {
      write32(i.mk_list.dst, f);
      write32(i.mk_list.nelems, f);
      break;
    }
    case EIR_OPCODE_MK_MAP: {
      write32(i.mk_map.dst, f);
      write32(i.mk_map.npairs, f);
      break;
    }
    case EIR_OPCODE_INDEX: {
      write32(i.index.dst, f);
      write32(i.index.base, f);
      write32(i.index.index, f);
      break;
    }
    case EIR_OPCODE_INDEX_ASSIGN: {
      write32(i.index_assign.value, f);
      write32(i.index_assign.base, f);
      write32(i.index_assign.index, f);
      break;
    }
    case EIR_OPCODE_CALL: {
      write32(i.call.dst, f);
      write32(i.call.function_id, f);
      write32(i.call.nargs, f);
      break;
    }
    case EIR_OPCODE_JZ:
    case EIR_OPCODE_JNZ: {
      write32(i.cj.target, f);
      write32(i.cj.condition, f);
      break;
    }
    case EIR_OPCODE_JMP: {
      write32(i.jmp.target, f);
      break;
    }

    case EIR_OPCODE_LABEL: {
      write32(i.label.id, f);
      break;
    }

    case EIR_OPCODE_MEMBER_ACCESS: {
      write32(i.member_access.dst, f);
      write32(i.member_access.base, f);
      write32(i.member_access.member_id, f);
      break;
    }
    case EIR_OPCODE_MEMBER_ASSIGN: {
      write32(i.member_assign.value, f);
      write32(i.member_assign.base, f);
      write32(i.member_assign.member_id, f);
      break;
    }
    case EIR_OPCODE_MK_STRUCT: {
      write32(i.mk_struct.dst, f);
      write32(i.mk_struct.struct_id, f);
      break;
    }
    case EIR_OPCODE_PUSH: {
      write32(i.push.reg, f);
      break;
    }
    case EIR_OPCODE_POP: {
      write32(i.pop.reg, f);
      break;
    }
  }
}

void
e_file_write(const e_compilation_result* r, FILE* f)
{
  const u32 magic = E_FILE_MAGIC;
  fwrite(&magic, sizeof(magic), 1, f);

  u32 bytes_req = e_file_bytes_required(r);
  fwrite(&bytes_req, sizeof(bytes_req), 1, f);

  fwrite(&r->literals_count, sizeof(r->literals_count), 1, f);
  for (u32 i = 0; i < r->literals_count; i++) {
    const e_var* lit = &r->literals[i];
    fwrite(&lit->type, sizeof(lit->type), 1, f);

    if (lit->type == E_VARTYPE_STRING) {
      u32 len = strlen(E_VAR_AS_STRING(lit)->s);
      fwrite(&len, sizeof(len), 1, f);
      fwrite(E_VAR_AS_STRING(lit)->s, sizeof(char), len, f);
    } else {
      switch (lit->type) {
        case E_VARTYPE_INT: fwrite(&lit->val.i, sizeof(lit->val.i), 1, f); break;
        case E_VARTYPE_BOOL: fwrite(&lit->val.b, sizeof(lit->val.b), 1, f); break;
        case E_VARTYPE_CHAR: fwrite(&lit->val.c, sizeof(lit->val.c), 1, f); break;
        case E_VARTYPE_FLOAT: fwrite(&lit->val.f, sizeof(lit->val.f), 1, f); break;
        case E_VARTYPE_VEC2: fwrite(&lit->val.vec2, sizeof(lit->val.vec2), 1, f); break;
        case E_VARTYPE_VEC3: fwrite(&lit->val.vec3, sizeof(lit->val.vec3), 1, f); break;
        case E_VARTYPE_VEC4: fwrite(&lit->val.vec4, sizeof(lit->val.vec4), 1, f); break;
        default: break;
      }
    }
  }

  fwrite(&r->structs_count, sizeof(u32), 1, f);
  for (u32 i = 0; i < r->structs_count; i++) {
    ecc_struct_information* st = &r->structs[i];

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
    const ecc_function* fn = &r->functions[i];
    fwrite(&fn->code_count, sizeof(fn->code_count), 1, f);
    fwrite(&fn->nargs, sizeof(fn->nargs), 1, f);
    fwrite(&fn->name_hash, sizeof(fn->name_hash), 1, f);
    // fwrite(fn->code, sizeof(e_ins), fn->code_count, f);
    for (u32 j = 0; j < fn->code_count; j++) { write_ins(fn->code[j], f); }
  }

  fwrite(&r->instructions_count, sizeof(r->instructions_count), 1, f);
  for (u32 j = 0; j < r->instructions_count; j++) { write_ins(r->instructions[j], f); }

  // fwrite(r->instructions, sizeof(e_ins), r->instructions_count, f);

  fwrite(&r->names_count, sizeof(r->names_count), 1, f);
  for (u32 i = 0; i < r->names_count; i++) {
    const char* sym = r->names[i];
    u32         len = strlen(sym);
    fwrite(&len, sizeof(len), 1, f);
    fwrite(sym, len, 1, f);
  }
}

e_file_read_error
e_file_load(e_compilation_result* r, void** root_allocation, FILE* f)
{
  *root_allocation = NULL;

  memset(r, 0, sizeof(e_compilation_result));

  u32 magic = 0;
  if (fread(&magic, sizeof(magic), 1, f) != 1) goto ERR;

  if (magic != E_FILE_MAGIC) return E_FILE_READ_ERR_INVALID_MAGIC;

  u32 bytes_req = 0;
  if (fread(&bytes_req, sizeof(bytes_req), 1, f) != 1) goto ERR;

  if (bytes_req > INT32_MAX) {
    fprintf(stderr, "[%s:%i] Warning: Loading a file with requested size > INT32_MAX. Possible file corruption. Continuing.\n", __FILE__, __LINE__);
  }

  *root_allocation = e_xalloc(bytes_req, 1);
  if (*root_allocation == NULL) return E_FILE_READ_ERR_ROOT_ALLOCATION_FAILED;

  uchar* alloc = (uchar*)*root_allocation;

  if (fread(&r->literals_count, sizeof(r->literals_count), 1, f) != 1) goto ERR;

  alloc       = e_align_ptr(alloc, 8);
  r->literals = (e_var*)alloc;
  alloc += sizeof(e_var) * r->literals_count;

  alloc              = e_align_ptr(alloc, 8);
  r->literals_hashes = (u32*)alloc;
  alloc += sizeof(u32) * r->literals_count;

  for (u32 i = 0; i < r->literals_count; i++) {
    if (fread(&r->literals[i].type, sizeof(e_vartype), 1, f) != 1) goto ERR;

    e_var* lit = &r->literals[i];
    switch (lit->type) {
      case E_VARTYPE_STRING: {
        u32 len = 0;
        if (fread(&len, sizeof(len), 1, f) != 1) goto ERR;

        alloc      = e_align_ptr(alloc, 16);
        lit->val.s = (e_refdobj*)alloc;
        alloc += sizeof(e_refdobj);

        /* Initialize ref counter to 1. They are immortal for the length of the program. */
        lit->val.s->refc = 1;

        E_VAR_AS_STRING(lit)->s = (char*)alloc;
        alloc += len + 1;

        if (fread(E_VAR_AS_STRING(lit)->s, sizeof(char), len, f) != len) goto ERR;

        E_VAR_AS_STRING(lit)->s[len] = 0;
        break;
      }

        // clang-format off
      case E_VARTYPE_NULL:
      case E_VARTYPE_STRUCT: *lit = (e_var){ .type = E_VARTYPE_NULL }; break;
      case E_VARTYPE_INT: if (fread(&lit->val.i, sizeof(lit->val.i), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_BOOL: if (fread(&lit->val.b, sizeof(lit->val.b), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_CHAR: if (fread(&lit->val.c, sizeof(lit->val.c), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_FLOAT: if (fread(&lit->val.f, sizeof(lit->val.f), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC2: if (fread(lit->val.vec2, sizeof(lit->val.vec2), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC3: if (fread(lit->val.vec3, sizeof(lit->val.vec3), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC4: if (fread(lit->val.vec4, sizeof(lit->val.vec4), 1, f) != 1) goto ERR; break;
      default: break;
        // clang-format on
    }
    r->literals_hashes[i] = e_var_hash(lit);
  }

  if (fread(&r->structs_count, sizeof(u32), 1, f) != 1) goto ERR;

  alloc      = e_align_ptr(alloc, 8);
  r->structs = (ecc_struct_information*)alloc;
  alloc += sizeof(ecc_struct_information) * r->structs_count;

  for (u32 i = 0; i < r->structs_count; i++) {
    ecc_struct_information* st = &r->structs[i];

    st->name = (char*)alloc;

    u32 name_len = 0;
    if (fread(&name_len, sizeof(u32), 1, f) != 1) goto ERR;
    alloc += name_len + 1;

    if (fread(st->name, name_len, 1, f) != 1) goto ERR;
    st->name[name_len] = 0;

    st->name_hash = e_hash(st->name, strlen(st->name));

    if (fread(&st->fields_count, sizeof(u32), 1, f) != 1) goto ERR;

    alloc            = e_align_ptr(alloc, 4);
    st->field_hashes = (u32*)alloc;
    alloc += sizeof(u32) * st->fields_count;

    if (fread(r->structs[i].field_hashes, sizeof(u32), st->fields_count, f) != st->fields_count) goto ERR;

    alloc           = e_align_ptr(alloc, 8);
    st->field_names = (char**)alloc;
    alloc += sizeof(const char*) * st->fields_count;

    for (u32 j = 0; j < st->fields_count; j++) {
      u32 len = 0;
      if (fread(&len, sizeof(u32), 1, f) != 1) goto ERR;

      alloc              = e_align_ptr(alloc, 8);
      st->field_names[j] = (char*)alloc;
      alloc += len + 1;

      if (fread(st->field_names[j], len, 1, f) != 1) goto ERR;
      st->field_names[j][len] = 0;
    }
  }

  if (fread(&r->functions_count, sizeof(r->functions_count), 1, f) != 1) goto ERR;

  alloc        = e_align_ptr(alloc, 8);
  r->functions = (ecc_function*)alloc;
  alloc += sizeof(ecc_function) * (r->functions_count);

  for (u32 i = 0; i < r->functions_count; i++) {
    ecc_function func = { 0 };
    if (fread(&func.code_count, sizeof(func.code_count), 1, f) != 1) goto ERR;
    if (fread(&func.nargs, sizeof(func.nargs), 1, f) != 1) goto ERR;
    if (fread(&func.name_hash, sizeof(func.name_hash), 1, f) != 1) goto ERR;

    alloc = e_align_ptr(alloc, 16);

    func.code = (e_ins*)alloc;
    alloc += func.code_count * sizeof(e_ins);
    if (func.code == NULL) return -1;

    for (u32 j = 0; j < func.code_count; j++) { func.code[j] = read_ins(f); }
    // if (fread(func.code, sizeof(e_ins), func.code_count, f) != func.code_count) goto ERR;
    r->functions[i] = func;
  }

  if (fread(&r->instructions_count, sizeof(r->instructions_count), 1, f) != 1) goto ERR;

  alloc           = e_align_ptr(alloc, 16);
  r->instructions = (e_ins*)alloc;
  // if (fread(r->instructions, sizeof(e_ins), r->instructions_count, f) != r->instructions_count) goto ERR;
  for (u32 j = 0; j < r->instructions_count; j++) { r->instructions[j] = read_ins(f); }

  alloc += r->instructions_count * sizeof(e_ins);
  alloc = e_align_ptr(alloc, 4);

  if (fread(&r->names_count, sizeof(r->names_count), 1, f) != 1) goto ERR;

  alloc    = e_align_ptr(alloc, 8);
  r->names = (char**)alloc;
  alloc += sizeof(char*) * r->names_count;

  alloc           = e_align_ptr(alloc, 8);
  r->names_hashes = (u32*)alloc;
  alloc += sizeof(u32) * r->names_count;

  for (u32 i = 0; i < r->names_count; i++) {
    u32 len = 0;

    if (fread(&len, sizeof(len), 1, f) != 1) goto ERR;

    alloc = e_align_ptr(alloc, 8);

    char* name = (char*)alloc;
    alloc += len + 1;

    if (fread(name, sizeof(char), len, f) != len) goto ERR;
    name[len] = 0;

    r->names[i]        = name;
    r->names_hashes[i] = e_hash(name, len);
  }

  return E_FILE_READ_SUCCESS;

ERR:
  free(*root_allocation);
  *root_allocation = NULL;
  return E_FILE_READ_ERR_INVALID_FILE;
}

u32
e_file_bytes_required(const e_compilation_result* r)
{
  u32 size = 0;

  size = e_align_size(size, 8);
  // literals array
  size += sizeof(e_var) * r->literals_count;

  size = e_align_size(size, 8);
  // literal hashes array (we don't write this tho)
  size += sizeof(u32) * r->literals_count;

  for (u32 i = 0; i < r->literals_count; i++) {
    const e_var* lit = &r->literals[i];

    if (lit->type == E_VARTYPE_STRING) {
      u32 len = strlen(E_VAR_AS_STRING(lit)->s);

      size = e_align_size(size, 16);
      size += sizeof(e_refdobj);
      size += len + 1; // null terminator
    }
  }

  // structures array
  size = e_align_size(size, 8);
  size += sizeof(ecc_struct_information) * r->structs_count;
  for (u32 i = 0; i < r->structs_count; i++) {
    u32 fields_count = r->structs[i].fields_count;
    size += strlen(r->structs[i].name) + 1; // NULL term
    size = e_align_size(size, 4);
    size += fields_count * sizeof(u32);
    size = e_align_size(size, 8);
    size += fields_count * sizeof(char*);
    for (u32 j = 0; j < fields_count; j++) {
      size = e_align_size(size, 8);
      size += strlen(r->structs[i].field_names[j]) + 1;
    }
  }

  // functions array
  size = e_align_size(size, 8);
  size += sizeof(ecc_function) * r->functions_count;
  for (u32 i = 0; i < r->functions_count; i++) {
    const ecc_function* fn = &r->functions[i];

    size = e_align_size(size, 16);
    size += fn->code_count * sizeof(e_ins); // code
  }

  size = e_align_size(size, 16);
  size += r->instructions_count * sizeof(e_ins);

  size = e_align_size(size, 4);
  size += sizeof(r->names_count);

  size = e_align_size(size, 8);
  size += sizeof(char*) * r->names_count;

  size = e_align_size(size, 8);
  size += sizeof(u32) * r->names_count;

  for (u32 i = 0; i < r->names_count; i++) {
    size = e_align_size(size, 8);
    size += strlen(r->names[i]) + 1;
  }

  return size;
}

void
e_emit_ins(e_compiler* cc, e_ins ins)
{
  if (cc->ninstructions >= cc->cinstructions) ecc_stream_resize(cc, cc->cinstructions * 2U);
  memcpy(&cc->instructions[cc->ninstructions++], &ins, sizeof(e_ins));
}

e_ins
e_read_ins(const u8** ip)
{
  e_ins i  = { 0 };
  i.opcode = e_read_u8(ip);

  switch (i.opcode) {
    case EIR_OPCODE_LOADK: {
      i.loadk.dst = e_read_u32(ip);
      i.loadk.id  = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_MOVG:
    case EIR_OPCODE_GETG:
    case EIR_OPCODE_SETG:
    case EIR_OPCODE_MOV: {
      i.mov.dst = e_read_u32(ip);
      i.mov.src = e_read_u32(ip);
      break;
    }

    case EIR_OPCODE_MOVI: {
      i.movi.dst   = e_read_u32(ip);
      i.movi.value = (int)e_read_u32(ip);
      break;
    }

    case EIR_OPCODE_MOVF: {
      i.movf.dst = e_read_u32(ip);
      u64 read   = e_read_u64(ip);
      memcpy(&i.movf.value, &read, sizeof(double));
      break;
    }

    case EIR_OPCODE_ADD:
    case EIR_OPCODE_SUB:
    case EIR_OPCODE_MUL:
    case EIR_OPCODE_DIV:
    case EIR_OPCODE_MOD:
    case EIR_OPCODE_EXP:
    case EIR_OPCODE_AND:
    case EIR_OPCODE_OR:
    case EIR_OPCODE_BAND:
    case EIR_OPCODE_BOR:
    case EIR_OPCODE_XOR:
    case EIR_OPCODE_EQL:
    case EIR_OPCODE_NEQ:
    case EIR_OPCODE_LT:
    case EIR_OPCODE_LTE:
    case EIR_OPCODE_GT:
    case EIR_OPCODE_GTE: {
      i.binop.dst = e_read_u32(ip);
      i.binop.a   = e_read_u32(ip);
      i.binop.b   = e_read_u32(ip);
      break;
    }

    case EIR_OPCODE_INC:
    case EIR_OPCODE_DEC:
    case EIR_OPCODE_BNOT:
    case EIR_OPCODE_NEG:
    case EIR_OPCODE_NOT: {
      i.unop.dst = e_read_u32(ip);
      i.unop.a   = e_read_u32(ip);
      break;
    }

    case EIR_OPCODE_RET: {
      i.ret.return_value = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_NOP: break;

    case EIR_OPCODE_MK_LIST: {
      i.mk_list.dst    = e_read_u32(ip);
      i.mk_list.nelems = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_MK_MAP: {
      i.mk_map.dst    = e_read_u32(ip);
      i.mk_map.npairs = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_INDEX: {
      i.index.dst   = e_read_u32(ip);
      i.index.base  = e_read_u32(ip);
      i.index.index = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_INDEX_ASSIGN: {
      i.index_assign.value = e_read_u32(ip);
      i.index_assign.base  = e_read_u32(ip);
      i.index_assign.index = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_CALL: {
      i.call.dst         = e_read_u32(ip);
      i.call.function_id = e_read_u32(ip);
      i.call.nargs       = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_JZ:
    case EIR_OPCODE_JNZ: {
      i.cj.target    = e_read_u32(ip);
      i.cj.condition = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_JMP: {
      i.jmp.target = e_read_u32(ip);
      break;
    }

    case EIR_OPCODE_LABEL: {
      i.label.id = e_read_u32(ip);
      break;
    }

    case EIR_OPCODE_MEMBER_ACCESS: {
      i.member_access.dst       = e_read_u32(ip);
      i.member_access.base      = e_read_u32(ip);
      i.member_access.member_id = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_MEMBER_ASSIGN: {
      i.member_assign.value     = e_read_u32(ip);
      i.member_assign.base      = e_read_u32(ip);
      i.member_assign.member_id = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_MK_STRUCT: {
      i.mk_struct.dst       = e_read_u32(ip);
      i.mk_struct.struct_id = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_PUSH: {
      i.push.reg = e_read_u32(ip);
      break;
    }
    case EIR_OPCODE_POP: {
      i.pop.reg = e_read_u32(ip);
      break;
    }
  }
  return i;
}
