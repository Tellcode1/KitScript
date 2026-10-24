#include "../../inc/kit.ast.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.reg.h"
#include "../../inc/kit.rwhelp.h"
#include "ast.extract.info.h"
#include "compile_routines.h"
#include "lvalue.h"
#include "scope.h"
#include "tables.h"
#include "vreg.h"

kit_vreg_t
compile_member_access(kit_compiler* cc, int node)
{
  if (!can_make_value(cc->ast, node)) {
    kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
    cerror(span, "Failed to compile member access: Failed to lower to rvalue\n");
    return -1;
  }
  // Passthrough
  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  return emit_lvalue_load(cc, &lv);
}

kit_vreg_t
compile_index(kit_compiler* cc, int node)
{
  kit_vreg_t base = compile(cc, KIT_GET_NODE(cc->ast, node)->index.base);
  if (base < 0) return base;

  kit_vreg_t index = compile(cc, KIT_GET_NODE(cc->ast, node)->index.index);
  if (index < 0) return index;

  kit_vreg_t dst = vreg_alloc(cc);
  kit_emit_ins(cc, (kit_ins){ .index = { .opcode = KIT_IR_OPCODE_INDEX, .dst = dst, .base = base, .index = index } });

  return dst;
}

kit_vreg_t
compile_variable_load(kit_compiler* cc, int node)
{
  char* full = qualify_name(cc, KIT_GET_NODE(cc->ast, node)->ident.ident);
  u32   hash = kit_hash(full, strlen(full));

  kit_name_resolution res;
  if (resolve_name(cc, hash, full, &res) < 0) {
    cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Undeclared variable '%s'\n", full);
    return -1;
  }
  return emit_name_load(cc, &res);
}

RETURNS_ERRCODE int
compile_and_push_literal_variable(kit_compiler* cc, const kit_var* v)
{
  /* OPTIMIZATION: If the variable is an integer or a float, emit a MOVI or a MOVF  */
  if (cc->info->opt_level >= 1 && v->type == KIT_VARTYPE_INT) {
    kit_vreg_t dst = vreg_alloc(cc);
    kit_emit_ins(cc, (kit_ins){ .movi = { .opcode = KIT_IR_OPCODE_MOVI, .dst = dst, .value = v->val.i } });
    return dst;
  }

  if (cc->info->opt_level >= 1 && v->type == KIT_VARTYPE_FLOAT) {
    kit_vreg_t dst = vreg_alloc(cc);
    kit_emit_ins(cc, (kit_ins){ .movf = { .opcode = KIT_IR_OPCODE_MOVF, .dst = dst, .value = v->val.f } });
    return dst;
  }

  if (cc->info->opt_level >= 1 && v->type == KIT_VARTYPE_NULL) { return KIT_REG_NIL; }

  int e = add_literal_to_track(cc, v);
  if (e < 0) return e;

  kit_vreg_t dst = vreg_alloc(cc);
  kit_emit_ins(cc, (kit_ins){ .loadk = { .opcode = KIT_IR_OPCODE_LOADK, .dst = dst, .id = kit_var_hash(v) } });

  return dst;
}

RETURNS_ERRCODE kit_vreg_t
compile_literal(kit_compiler* cc, int node)
{
  // Convert the node to a variable and
  kit_var v = { .type = KIT_VARTYPE_NULL };
  if (convert_node_to_literal(cc, node, &v)) return -1;

  // Compile it
  return compile_and_push_literal_variable(cc, &v);
}
