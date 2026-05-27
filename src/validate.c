#include "../inc/validate.h"

#include "../inc/bc.h"
#include "../inc/bvar.h"
#include "../inc/exec.h"
#include "../inc/rwhelp.h"
#include "../inc/stackemu.h"

static inline const char*
find_name(u32 id, const e_exec_info* info)
{
  for (u32 i = 0; i < info->nnames; i++) {
    if (info->names_hashes[i] == id) { return info->names[i]; }
  }
  return "(Unknown)";
}

static int
validate_stream(const u8* code, u32 code_size, const u32* arg_slots, u32 nargs, e_stackemu* emu, const e_exec_info* info, FILE* f)
{
  int e = 0;

  const u8* ip  = code;
  const u8* end = code + code_size;

  for (u32 i = 0; i < nargs; i++) {
    ecc_variable_information var = { .name_hash = arg_slots[i] };

    e = e_stackemu_push_var(emu, &var);
    if (e) return e;
  }

  // u32 stack_top = 0;

  while (ip < end) {
    e_ins ins = e_read_ins(&ip);

    if (ins.opcode == E_OPCODE_INIT) {
      ecc_variable_information var = { .name_hash = ins.v.init };

      e = e_stackemu_push_var(emu, &var);
      if (e) return e;
    }

    else if (ins.opcode == E_OPCODE_LOAD) {
      const u32 id = ins.v.load;

      bool found = false;
      for (u32 i = 0; i < info->nextern_vars; i++) {
        const char* name = info->extern_vars[i].name;
        if (e_hash(name, strlen(name)) == id) {
          found = true;
          break;
        }
      }

      for (u32 i = 0; i < E_ARRLEN(eb_vars); i++) {
        const char* name = eb_vars[i].name;
        if (e_hash(name, strlen(name)) == id) {
          found = true;
          break;
        }
      }

      if (e_stackemu_find_var(emu, id) != NULL) { found = true; }

      if (!found) {
        fprintf(f, "LOAD: %s undeclared\n", find_name(id, info));
        e = -1;
      }

      // stack_top++;
    }

    else if (ins.opcode == E_OPCODE_LITERAL) {
      const u32 id = ins.v.literal;

      bool found = false;
      for (u32 i = 0; i < info->nliterals; i++) {
        if (info->literals_hashes[i] == id) {
          found = true;
          break;
        }
      }

      if (!found) {
        fprintf(f, "LITERAL: index out of bound (%u)\n", id);
        e = -1;
      }
    }

    else if (ins.opcode == E_OPCODE_ASSIGN) {
      const u32 id = ins.v.assign;

      bool found = false;
      if (e_stackemu_find_var(emu, id) != NULL) { found = true; }

      if (!found) {
        fprintf(f, "ASSIGN [%s]: target undeclared\n", find_name(id, info));
        e = -1;
      }

      // stack_top--;
    }

    else if (ins.opcode == E_OPCODE_CALL) {
      const u32 id         = ins.v.call.hash;
      const u32 func_nargs = ins.v.call.nargs;

      bool found             = false;
      bool invalid_arg_count = false;
      for (u32 i = 0; i < info->nfuncs; i++) {
        if (info->funcs[i].name_hash == id) {
          found = true;
          if (info->funcs[i].nargs != func_nargs) { invalid_arg_count = true; }
          break;
        }
      }

      for (u32 i = 0; i < E_ARRLEN(eb_funcs); i++) {
        if (e_hash(eb_funcs[i].name, strlen(eb_funcs[i].name)) == id) {
          found = true;
          if (func_nargs < eb_funcs[i].min_args || func_nargs > eb_funcs[i].max_args) { invalid_arg_count = true; }
          break;
        }
      }

      for (u32 i = 0; i < info->nextern_vars; i++) {
        if (e_hash(info->extern_funcs[i].name, strlen(info->extern_funcs[i].name)) == id) {
          found = true;
          if (func_nargs < info->extern_funcs[i].min_args || func_nargs > info->extern_funcs[i].max_args) { invalid_arg_count = true; }
          break;
        }
      }

      if (!found) { fprintf(f, "CALL[%s,%u]: undefined function\n", find_name(id, info), func_nargs); }
      if (invalid_arg_count) { fprintf(f, "CALL[%s,%u]: invalid argument count\n", find_name(id, info), func_nargs); }

      if (!found || invalid_arg_count) return -1;
    }

    else if (ins.opcode == E_OPCODE_PUSH_FRAME) {
      e = e_stackemu_push_frame(emu);
      if (e) return e;
    } else if (ins.opcode == E_OPCODE_POP_FRAME) {
      e_stackemu_pop_frame(emu);
    }
  }

  return e;
}

int
e_validate(const struct e_exec_info* info, FILE* f)
{
  e_stackemu emu = { 0 };

  int e = e_stackemu_init(&emu);
  if (e) return e;

  e = validate_stream(info->code, info->code_size, NULL, 0, &emu, info, f);
  if (e) goto RET;

  e = e_stackemu_push_frame(&emu);
  if (e) goto RET;

  for (u32 i = 0; i < info->nfuncs; i++) {
    const ecc_function* fn = &info->funcs[i];

    e = validate_stream(fn->code, fn->code_size, fn->arg_slots, fn->nargs, &emu, info, f);
    if (e) { goto RET; }
  }

  e_stackemu_pop_frame(&emu);

RET:
  e_stackemu_free(&emu);
  return e;
}