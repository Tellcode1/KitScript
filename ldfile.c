#include "bc.h"
#include "cc.h"
#include "rwhelp.h"
#include "stdafx.h"

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
  if (*root_allocation == nullptr) return E_FILE_READ_ERR_ROOT_ALLOCATION_FAILED;

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
        lit->val.s = (e_refdobj*)(alloc);
        alloc += sizeof(e_refdobj);

        /* Initialize ref counter to 1. Not used for literals but VM expects it */
        lit->val.s->refc = 1;

        E_VAR_AS_STRING(lit)->s = (char*)(alloc);
        alloc += len + 1;

        if (fread(E_VAR_AS_STRING(lit)->s, sizeof(char), len, f) != len) goto ERR;

        E_VAR_AS_STRING(lit)->s[len] = 0;
        break;
      }

        // clang-format off
      case E_VARTYPE_NULL:
      case E_VARTYPE_VOID:
      case E_VARTYPE_STRUCT: *lit = (e_var){ .type = E_VARTYPE_NULL }; break;
      case E_VARTYPE_INT: if (fread(&lit->val.i, sizeof(lit->val.i), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_BOOL: if (fread(&lit->val.b, sizeof(lit->val.b), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_CHAR: if (fread(&lit->val.c, sizeof(lit->val.c), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_FLOAT: if (fread(&lit->val.f, sizeof(lit->val.f), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC2: if (fread(&lit->val.vec2, sizeof(lit->val.vec2), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC3: if (fread(&lit->val.vec3, sizeof(lit->val.vec3), 1, f) != 1) goto ERR; break;
      case E_VARTYPE_VEC4: if (fread(&lit->val.vec4, sizeof(lit->val.vec4), 1, f) != 1) goto ERR; break;
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
    if (fread(&r->structs[i].name_hash, sizeof(u32), 1, f) != 1) goto ERR;
    if (fread(&r->structs[i].fields_count, sizeof(u32), 1, f) != 1) goto ERR;

    alloc                      = e_align_ptr(alloc, 4);
    r->structs[i].field_hashes = (u32*)alloc;
    alloc += sizeof(u32) * r->structs[i].fields_count;

    if (fread(r->structs[i].field_hashes, sizeof(u32), r->structs[i].fields_count, f) != r->structs[i].fields_count) goto ERR;
  }

  if (fread(&r->functions_count, sizeof(r->functions_count), 1, f) != 1) goto ERR;

  alloc        = e_align_ptr(alloc, 8);
  r->functions = (e_function*)(alloc);
  alloc += sizeof(e_function) * (r->functions_count);

  for (u32 i = 0; i < r->functions_count; i++) {
    e_function func = { 0 };
    if (fread(&func.code_size, sizeof(func.code_size), 1, f) != 1) goto ERR;
    if (fread(&func.nargs, sizeof(func.nargs), 1, f) != 1) goto ERR;
    if (fread(&func.name_hash, sizeof(func.name_hash), 1, f) != 1) goto ERR;

    alloc = e_align_ptr(alloc, 8);

    func.arg_slots = (u32*)alloc;
    alloc += sizeof(u32) * func.nargs;
    if (fread(func.arg_slots, sizeof(*func.arg_slots), func.nargs, f) != func.nargs) goto ERR;

    alloc = e_align_ptr(alloc, 8);

    func.code = (u8*)alloc;
    alloc += func.code_size;
    if (func.code == nullptr) return -1;

    if (fread(func.code, 1, func.code_size, f) != func.code_size) goto ERR;
    r->functions[i] = func;
  }

  if (fread(&r->instructions_count, sizeof(r->instructions_count), 1, f) != 1) goto ERR;

  alloc           = e_align_ptr(alloc, 8);
  r->instructions = (u8*)alloc;
  if (fread(r->instructions, sizeof(u8), r->instructions_count, f) != r->instructions_count) goto ERR;

  alloc += r->instructions_count;
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
    size = e_align_size(size, 4);
    size += r->structs[i].fields_count * sizeof(u32);
  }

  // functions array
  size = e_align_size(size, 8);
  size += sizeof(e_function) * r->functions_count;
  for (u32 i = 0; i < r->functions_count; i++) {
    const e_function* fn = &r->functions[i];

    if (fn->nargs > 0) {
      size = e_align_size(size, 8);
      size += sizeof(u32) * fn->nargs; // arg_slots
    }

    size = e_align_size(size, 8);
    size += fn->code_size; // code
  }

  size = e_align_size(size, 8);
  size += r->instructions_count;

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
e_file_write(const e_compilation_result* r, FILE* f)
{
  u32 magic = E_FILE_MAGIC;
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
    fwrite(&r->structs[i].name_hash, sizeof(u32), 1, f);
    fwrite(&r->structs[i].fields_count, sizeof(u32), 1, f);
    fwrite(r->structs[i].field_hashes, sizeof(u32), r->structs[i].fields_count, f);
  }

  fwrite(&r->functions_count, sizeof(r->functions_count), 1, f);
  for (u32 i = 0; i < r->functions_count; i++) {
    const e_function* fn = &r->functions[i];
    fwrite(&fn->code_size, sizeof(fn->code_size), 1, f);
    fwrite(&fn->nargs, sizeof(fn->nargs), 1, f);
    fwrite(&fn->name_hash, sizeof(fn->name_hash), 1, f);
    if (fn->arg_slots && fn->nargs > 0) fwrite(fn->arg_slots, sizeof(*fn->arg_slots), fn->nargs, f);
    fwrite(fn->code, 1, fn->code_size, f);
  }

  fwrite(&r->instructions_count, sizeof(r->instructions_count), 1, f);
  fwrite(r->instructions, sizeof(u8), r->instructions_count, f);

  fwrite(&r->names_count, sizeof(r->names_count), 1, f);
  for (u32 i = 0; i < r->names_count; i++) {
    u32 len = strlen(r->names[i]);
    fwrite(&len, sizeof(len), 1, f);
    fwrite(r->names[i], 1, len, f);
  }
}

e_ins
e_read_ins(const u8** ip)
{
  e_ins i  = { 0 };
  i.opcode = e_read_u8(ip);
  switch ((e_opcode_bck)i.opcode) {
    case E_OPCODE_NOOP:
    case E_OPCODE_ADD:
    case E_OPCODE_SUB:
    case E_OPCODE_MUL:
    case E_OPCODE_DIV:
    case E_OPCODE_MOD:
    case E_OPCODE_EXP:
    case E_OPCODE_AND:
    case E_OPCODE_OR:
    case E_OPCODE_NOT:
    case E_OPCODE_BAND:
    case E_OPCODE_BOR:
    case E_OPCODE_XOR:
    case E_OPCODE_BNOT:
    case E_OPCODE_EQL:
    case E_OPCODE_NEQ:
    case E_OPCODE_LT:
    case E_OPCODE_LTE:
    case E_OPCODE_GT:
    case E_OPCODE_GTE:
    case E_OPCODE_NEG:
    case E_OPCODE_INC:
    case E_OPCODE_POP:
    case E_OPCODE_DUP:
    case E_OPCODE_INDEX_ASSIGN:
    case E_OPCODE_INDEX_PEEK:
    case E_OPCODE_PUSH_FRAME:
    case E_OPCODE_POP_FRAME:
    case E_OPCODE_INDEX:
    case E_OPCODE_DEC: break;

    case E_OPCODE_CALL:
      i.v.call.hash  = e_read_u32(ip);
      i.v.call.nargs = e_read_u16(ip);
      break;

    case E_OPCODE_RETURN: i.v.has_return_value = e_read_u8(ip); break;

    case E_OPCODE_LITERAL: i.v.literal = e_read_u32(ip); break;
    case E_OPCODE_ASSERT: i.v.assertion = e_read_u32(ip); break;
    case E_OPCODE_LOAD: i.v.load = e_read_u32(ip); break;
    case E_OPCODE_ASSIGN: i.v.assign = e_read_u32(ip); break;
    case E_OPCODE_INIT: i.v.init = e_read_u32(ip); break;

    case E_OPCODE_MK_LIST: i.v.mk_list = e_read_u32(ip); break;
    case E_OPCODE_MK_MAP: i.v.mk_map = e_read_u32(ip); break;
    case E_OPCODE_LABEL: i.v.label = e_read_u32(ip); break;

    case E_OPCODE_JMP:
    case E_OPCODE_JE:
    case E_OPCODE_JNE:
    case E_OPCODE_JZ:
    case E_OPCODE_JNZ: i.v.jmp = e_read_u32(ip); break;

    case E_OPCODE_MK_STRUCT: i.v.mk_struct = e_read_u32(ip); break;

    case E_OPCODE_MEMBER_ACCESS:
    case E_OPCODE_MEMBER_ASSIGN: i.v.member = e_read_u32(ip); break;
    case E_OPCODE_HALT:
    case E_OPCODE_COUNT: break;
  }
  return i;
}
