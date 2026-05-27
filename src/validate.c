#include "../inc/validate.h"

#include "../inc/exec.h"
#include "../inc/rwhelp.h"
#include "../inc/stackemu.h"

// static inline const char*
// find_name(u32 id, const e_exec_info* info)
// {
//   for (u32 i = 0; i < info->nnames; i++) {
//     if (info->names_hashes[i] == id) { return info->names[i]; }
//   }
//   return "(Unknown)";
// }

// static int
// validate_stream(const u8* code, u32 code_size, const ereg_t* arg_slots, u32 nargs, e_stackemu* emu, const e_exec_info* info, FILE* f)
// {
//   int e = 0;

//   const u8* ip  = code;
//   const u8* end = code + code_size;

//   for (u32 i = 0; i < nargs; i++) {
//     ecc_variable_information var = { .name_hash = arg_slots[i] };

//     e = e_stackemu_push_var(emu, &var);
//     if (e) return e;
//   }

//   // u32 stack_top = 0;

//   while (ip < end) {
//     e_ins ins = e_read_ins(&ip);

//     if (ins.opcode == EIR_OPCODE_LOADK) {
//       const u32 id = ins.loadk.id;

//       bool found = false;
//       for (u32 i = 0; i < info->nliterals; i++) {
//         if (info->literals_hashes[i] == id) {
//           found = true;
//           break;
//         }
//       }

//       if (!found) {
//         fprintf(f, "LITERAL: index out of bound (%u)\n", id);
//         e = -1;
//       }
//     }

//     else if (ins.opcode == EIR_OPCODE_CALL) {
//       const u32 id         = ins.call.function_id;
//       const u32 func_nargs = ins.call.nargs;

//       bool found             = false;
//       bool invalid_arg_count = false;
//       for (u32 i = 0; i < info->nfuncs; i++) {
//         if (info->funcs[i].name_hash == id) {
//           found = true;
//           if (info->funcs[i].nargs != func_nargs) { invalid_arg_count = true; }
//           break;
//         }
//       }

//       for (u32 i = 0; i < E_ARRLEN(eb_funcs); i++) {
//         if (e_hash(eb_funcs[i].name, strlen(eb_funcs[i].name)) == id) {
//           found = true;
//           if (func_nargs < eb_funcs[i].min_args || func_nargs > eb_funcs[i].max_args) { invalid_arg_count = true; }
//           break;
//         }
//       }

//       for (u32 i = 0; i < info->nextern_vars; i++) {
//         if (e_hash(info->extern_funcs[i].name, strlen(info->extern_funcs[i].name)) == id) {
//           found = true;
//           if (func_nargs < info->extern_funcs[i].min_args || func_nargs > info->extern_funcs[i].max_args) { invalid_arg_count = true; }
//           break;
//         }
//       }

//       if (!found) { fprintf(f, "CALL[%s,%u]: undefined function\n", find_name(id, info), func_nargs); }
//       if (invalid_arg_count) { fprintf(f, "CALL[%s,%u]: invalid argument count\n", find_name(id, info), func_nargs); }

//       if (!found || invalid_arg_count) return -1;
//     }
//   }

//   return e;
// }

int
e_validate(const struct e_exec_info* info, FILE* f)
{
  //   e_stackemu emu = { 0 };

  //   int e = e_stackemu_init(&emu);
  //   if (e) return e;

  //   e = validate_stream(info->code, info->code_count, NULL, 0, &emu, info, f);
  //   if (e) goto RET;

  //   e = e_stackemu_push_frame(&emu);
  //   if (e) goto RET;

  //   for (u32 i = 0; i < info->nfuncs; i++) {
  //     const ecc_function* fn = &info->funcs[i];

  //     e = validate_stream(fn->code, fn->code_count, fn->arg_slots, fn->nargs, &emu, info, f);
  //     if (e) { goto RET; }
  //   }

  //   e_stackemu_pop_frame(&emu);

  // RET:
  //   e_stackemu_free(&emu);
  //   return e;
  return 0;
}