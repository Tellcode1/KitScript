#include "../../inc/kit.ast.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.rwhelp.h"
#include "compile_routines.h"
#include "scope.h"

kit_vreg_t
compile_variable_decleration(kit_compiler* cc, int node)
{
  const char* name = KIT_GET_NODE(cc->ast, node)->let.name;
  char*       full = qualify_name(cc, name);
  if (!full) return -1;

  u32 hash        = kit_hash(full, strlen(full));
  int initializer = KIT_GET_NODE(cc->ast, node)->let.initializer;

  kitc_var* v = cc->scope->vars;
  while (v) {
    if (v->name_hash == hash) {
      cerror(v->span, "Variable redeclared in same scope\n");
      return -1;
    }
    v = v->next;
  }

  /* Add variable entry to stack */

  const bool initializer_provided = initializer >= 0;

  kit_filespan span     = KIT_GET_NODE(cc->ast, node)->let.span;
  bool         is_const = KIT_GET_NODE(cc->ast, node)->let.is_const;

  kit_vreg_t init_reg = -1;
  if (initializer_provided) {
    /* cant use the lval system because it doesn't handle const global variables (this is a variable decleration, and if it is const,
     * emit_lvalue_assign will always error out) */

    init_reg = compile(cc, initializer);
    if (init_reg < 0) return init_reg;
  } else {
    kit_var nil = KIT_NULLVAR;
    init_reg    = compile_and_push_literal_variable(cc, &nil);
    if (init_reg < 0) return init_reg;
  }

  /* Define the variable */
  kit_vreg_t new_var = scope_define(cc, span, full, is_const);
  kitc_var*  info    = scope_lookup_info(cc, hash);

  /* no initializer specified. initialize it to null. */
  if (info->is_global) {
    kit_emit_ins(cc, (kit_ins){ .setg = { .opcode = KIT_IR_OPCODE_SETG, .dst = new_var, .src = init_reg } });
  } else {
    kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = new_var, .src = init_reg } });
  }

  return new_var;
}
