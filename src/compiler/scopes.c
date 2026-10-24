#include "../../inc/kit.bfunc.h"
#include "../../inc/kit.rwhelp.h"
#include "compile_routines.h"
#include "scope.h"

int
resolve_name(kit_compiler* cc, u32 hash, const char* full, kit_name_resolution* out)
{
  for (u32 i = 0; i < cc->builtin_var_table->builtin_vars_count; i++) {
    if (cc->builtin_var_table->builtin_var_hashes[i] != hash) continue;
    const kit_builtin_var* bv = &cc->builtin_var_table->builtin_vars[i];
    *out                      = (kit_name_resolution){ .kind = KIT_NAME_BUILTIN_VAR, .as.builtin_val = { .type = bv->type, .val = bv->value } };
    return 0;
  }

  for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
    if (strcmp(kit_builtins_funcs[i].name, full) != 0) continue;
    *out = (kit_name_resolution){ .kind = KIT_NAME_CALLABLE, .as.fn_hash = hash };
    return 0;
  }

  for (u32 i = 0; i < cc->struct_table->structs_count; i++) {
    if (cc->struct_table->structs[i].name_hash != hash) continue;
    *out = (kit_name_resolution){ .kind = KIT_NAME_CALLABLE, .as.fn_hash = hash };
    return 0;
  }

  for (u32 i = 0; i < cc->function_table->functions_count; i++) {
    if (cc->function_table->functions[i].name_hash != hash) continue;
    *out = (kit_name_resolution){ .kind = KIT_NAME_CALLABLE, .as.fn_hash = hash };
    return 0;
  }

  kitc_var* v = scope_lookup_info(cc, hash);
  if (!v) return -1;

  if (v->is_global) {
    *out = (kit_name_resolution){ .kind = KIT_NAME_GLOBAL, .as.global_id = v->slot.global_id };
  } else {
    *out = (kit_name_resolution){ .kind = KIT_NAME_LOCAL, .as.reg = v->slot.reg };
  }
  return 0;
}

kit_vreg_t
emit_name_load(kit_compiler* cc, const kit_name_resolution* r)
{
  switch (r->kind) {
    case KIT_NAME_BUILTIN_VAR: return compile_and_push_literal_variable(cc, &r->as.builtin_val);

    case KIT_NAME_CALLABLE: {
      kit_vreg_t dst = vreg_alloc(cc);
      kit_emit_ins(cc, (kit_ins){ .loadfn = { .opcode = KIT_IR_OPCODE_LOADFN, .dst = dst, .id = r->as.fn_hash } });
      return dst;
    }

    case KIT_NAME_GLOBAL: {
      kit_vreg_t dst = vreg_alloc(cc);
      kit_emit_ins(cc, (kit_ins){ .getg = { .opcode = KIT_IR_OPCODE_GETG, .dst = dst, .src = r->as.global_id } });
      return dst;
    }

    case KIT_NAME_LOCAL: return r->as.reg;
  }
  return -1;
}