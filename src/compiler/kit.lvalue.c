#include "../../inc/kit.bfunc.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.rwhelp.h"
#include "compile_routines.h"
#include "lvalue.h"
#include "scope.h"

int
value_init(kit_compiler* cc, int node, val_t* d)
{
  val_t l = { 0 };

  switch (KIT_GET_NODE(cc->ast, node)->type) {
    case KIT_AST_NODE_VARIABLE: {
      kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
      char*        name = qualify_name(cc, KIT_GET_NODE(cc->ast, node)->ident.ident);

      u32 id = kit_hash(name, strlen(name));

      for (u32 i = 0; i < cc->builtin_var_table->builtin_vars_count; i++) {
        // const kit_builtin_var* builtin_var      = &cc->builtin_var_table->builtin_vars[i];
        u32 builtin_var_hash = cc->builtin_var_table->builtin_var_hashes[i];

        if (builtin_var_hash == id) {
          l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
          l.type         = KIT_LVAL_GVAR;
          l.val.var.id   = id;
          l.val.var.name = name;

          *d = l;
          return 0;
        }
      }

      for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
        const kit_builtin_func* func = &kit_builtins_funcs[i];

        if (strcmp(func->name, name) == 0) {
          l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
          l.type         = KIT_LVAL_GVAR;
          l.val.var.id   = id;
          l.val.var.name = name;

          *d = l;
          return 0;
        }
      }

      for (u32 i = 0; i < cc->struct_table->structs_count; i++) {
        const kitc_struct_information* struct_info = &cc->struct_table->structs[i];

        if (struct_info->name_hash == id) {
          l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
          l.type         = KIT_LVAL_GVAR;
          l.val.var.id   = id;
          l.val.var.name = name;

          *d = l;
          return 0;
        }
      }

      for (u32 i = 0; i < cc->function_table->functions_count; i++) {
        const kitc_function* func = &cc->function_table->functions[i];

        if (func->name_hash == id) {
          l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
          l.type         = KIT_LVAL_GVAR;
          l.val.var.id   = id;
          l.val.var.name = name;

          *d = l;
          return 0;
        }
      }

      kitc_var* v = scope_lookup_info(cc, id);
      if (!v) {
        cerror(span, "Undeclared variable %s\n", name);
        return -1;
      }

      l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type         = v->is_global ? KIT_LVAL_GVAR : KIT_LVAL_VAR;
      l.val.var.id   = id;
      l.val.var.name = name;

      *d = l;
      return 0;
    }

    case KIT_AST_NODE_VARIABLE_DECL: {
      char* name = qualify_name(cc, KIT_GET_NODE(cc->ast, node)->let.name);

      u32       id = kit_hash(name, strlen(name));
      kitc_var* v  = scope_lookup_info(cc, id);
      if (!v) return -1;

      l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type         = v->is_global ? KIT_LVAL_GVAR : KIT_LVAL_VAR;
      l.val.var.id   = id;
      l.val.var.name = name;
      *d             = l;
      return 0;
    }

    case KIT_AST_NODE_INDEX: {
      l.span                 = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type                 = KIT_LVAL_INDEX;
      l.val.index.left_node  = KIT_GET_NODE(cc->ast, node)->index.base;
      l.val.index.index_node = KIT_GET_NODE(cc->ast, node)->index.index;
      *d                     = l;
      return 0;
    }

    case KIT_AST_NODE_INDEX_ASSIGN: {
      l.span                 = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type                 = KIT_LVAL_INDEX;
      l.val.index.left_node  = KIT_GET_NODE(cc->ast, node)->index_assign.base;
      l.val.index.index_node = KIT_GET_NODE(cc->ast, node)->index_assign.index;
      *d                     = l;
      return 0;
    }

    case KIT_AST_NODE_MEMBER_ACCESS: {
      l.span                   = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type                   = KIT_LVAL_MEMBER;
      l.val.member.base        = KIT_GET_NODE(cc->ast, node)->member_access.left;
      l.val.member.member      = KIT_GET_NODE(cc->ast, node)->member_access.right;
      l.val.member.member_hash = kit_hash(l.val.member.member, strlen(l.val.member.member));
      *d                       = l;
      return 0;
    }

    case KIT_AST_NODE_MEMBER_ASSIGN: {
      l.type                   = KIT_LVAL_MEMBER;
      l.span                   = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.val.member.base        = KIT_GET_NODE(cc->ast, node)->member_assign.left;
      l.val.member.member      = KIT_GET_NODE(cc->ast, node)->member_assign.right;
      l.val.member.member_hash = kit_hash(l.val.member.member, strlen(l.val.member.member));
      *d                       = l;
      return 0;
    }

    default:
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "%i can not be represented as a value (it is %u)\n", node, KIT_GET_NODE(cc->ast, node)->type);
      return -1;
  }

  return -1;
}

void
value_free(val_t* lv)
{
  if (lv->type == KIT_LVAL_VAR) { /* free(lv->val.var.name); */
  }
  memset(lv, 0, sizeof(*lv));
}

int
emit_lvalue_assign(kit_compiler* cc, kit_vreg_t value, val_t* lv)
{
  switch (lv->type) {
    case KIT_LVAL_VAR: {
      kitc_var* v = scope_lookup_info(cc, lv->val.var.id);
      if (!v) {
        cerror(*lv->span, "Undeclared variable %s\n", lv->val.var.name);
        return -1;
      }

      if (v->is_const) {
        cerror(*lv->span, "Can not assign to const variable %s\n", lv->val.var.name);
        return -1;
      }

      /* local to local operation. global to global operations need a temporary register. */
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = v->slot.reg, .src = value } });
      break;
    }
    case KIT_LVAL_GVAR: {
      kitc_var* v = scope_lookup_info(cc, lv->val.gvar);

      if (!v) {
        cerror(*lv->span, "Undeclared global variable %u\n", lv->val.gvar);
        return -1;
      }

      if (v->is_const) {
        cerror(*lv->span, "Can not assign to const global variable %u\n", lv->val.gvar);
        return -1;
      }

      /* writing to a global variable from a local register */
      kit_emit_ins(cc, (kit_ins){ .setg = { .opcode = KIT_IR_OPCODE_SETG, .dst = v->slot.global_id, .src = value } });
      break;
    }
    case KIT_LVAL_INDEX: {
      kit_vreg_t base  = -1;
      kit_vreg_t index = -1;

      // base = compile(cc, lv->val.index.left_node);
      // if (base < 0) return base;

      // index = compile(cc, lv->val.index.index_node);
      // if (index < 0) return index;

      // kit_emit_ins(cc, (kit_ins){ .index_assign = { .opcode = KIT_IR_OPCODE_INDEX_ASSIGN, .value = value, .base = base, .index = index } });

      val_t left = { 0 };
      if (value_init(cc, lv->val.index.left_node, &left) < 0) return -1;

      /* prep base */
      base = emit_lvalue_load(cc, &left);
      if (base < 0) return base;

      /* prep index */
      index = compile(cc, lv->val.index.index_node);
      if (index < 0) return index;

      /* index_assign modifies base. */
      kit_emit_ins(cc, (kit_ins){ .index_assign = { .opcode = KIT_IR_OPCODE_INDEX_ASSIGN, .value = value, .base = base, .index = index } });

      /* write base back to its slot */
      if (emit_lvalue_assign(cc, base, &left) < 0) return -1;

      value_free(&left);
      break;
    }
    case KIT_LVAL_MEMBER: {
      int left      = lv->val.member.base;
      u32 member_id = lv->val.member.member_hash;

      val_t base_lv = { 0 };
      if (value_init(cc, left, &base_lv) < 0) return -1;

      /* prep base */
      kit_vreg_t base = emit_lvalue_load(cc, &base_lv);
      if (base < 0) return base;

      kit_emit_ins(cc, (kit_ins){ .member_assign = { .opcode = KIT_IR_OPCODE_MEMBER_ASSIGN, .value = value, .base = base, .member_id = member_id } });

      /* write base back to its slot */
      if (emit_lvalue_assign(cc, base, &base_lv) < 0) return -1;

      value_free(&base_lv);
      break;
    }
    case KIT_LVAL_UNKNOWN: return -1;
  }

  /* Propogate assigned value */
  return value;
}

int
emit_lvalue_load(kit_compiler* cc, val_t* lv)
{
  switch (lv->type) {
    case KIT_LVAL_VAR: {
      u32         hash = lv->val.var.id;
      const char* full = lv->val.var.name;

      for (u32 i = 0; i < cc->builtin_var_table->builtin_vars_count; i++) {
        const kit_builtin_var* builtin_var      = &cc->builtin_var_table->builtin_vars[i];
        u32                    builtin_var_hash = cc->builtin_var_table->builtin_var_hashes[i];

        if (builtin_var_hash == hash) {
          /* Instantiate a builtin variable only if it is used. */
          kit_var v = {
            .type = builtin_var->type,
            .val  = builtin_var->value,
          };

          return compile_and_push_literal_variable(cc, &v); // compile_literal_variable loads the value! Return.
        }
      }

      for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
        const kit_builtin_func* func = &kit_builtins_funcs[i];

        if (strcmp(func->name, full) == 0) {
          /* Builtin function, loadfn it and return */
          kit_vreg_t dst = vreg_alloc(cc);
          kit_emit_ins(cc, (kit_ins){ .loadfn = { .opcode = KIT_IR_OPCODE_LOADFN, .dst = dst, .id = hash } });
          return dst;
        }
      }

      for (u32 i = 0; i < cc->struct_table->structs_count; i++) {
        const kitc_struct_information* struct_info = &cc->struct_table->structs[i];

        if (struct_info->name_hash == hash) {
          /* Builtin function, loadfn it and return */
          kit_vreg_t dst = vreg_alloc(cc);
          kit_emit_ins(cc, (kit_ins){ .loadfn = { .opcode = KIT_IR_OPCODE_LOADFN, .dst = dst, .id = hash } });
          return dst;
        }
      }

      for (u32 i = 0; i < cc->function_table->functions_count; i++) {
        const kitc_function* func = &cc->function_table->functions[i];

        if (func->name_hash == hash) {
          /* Builtin function, loadfn it and return */
          kit_vreg_t dst = vreg_alloc(cc);
          kit_emit_ins(cc, (kit_ins){ .loadfn = { .opcode = KIT_IR_OPCODE_LOADFN, .dst = dst, .id = hash } });
          return dst;
        }
      }

      kit_vreg_t var_reg = scope_lookup_reg(cc, lv->val.var.id);
      if (var_reg < 0) return -1;

      return var_reg;
    }

    case KIT_LVAL_GVAR: {
      kitc_var* v = scope_lookup_info(cc, lv->val.var.id);
      if (!v) return -1;

      kit_vreg_t dst = vreg_alloc(cc);

      /* emit a GETG to fetch the global variable into our local register */
      kit_emit_ins(cc, (kit_ins){ .getg = { .opcode = KIT_IR_OPCODE_GETG, .dst = dst, .src = v->slot.global_id } });
      return dst;
    }

    case KIT_LVAL_INDEX: {
      kit_vreg_t dst = vreg_alloc(cc);

      kit_vreg_t base  = -1;
      kit_vreg_t index = -1;

      base = compile(cc, lv->val.index.left_node);
      if (base < 0) return base;

      index = compile(cc, lv->val.index.index_node);
      if (index < 0) return index;

      kit_emit_ins(cc, (kit_ins){ .index = { .opcode = KIT_IR_OPCODE_INDEX, .dst = dst, .base = base, .index = index } });

      return dst;
    }

    case KIT_LVAL_MEMBER: {
      int left      = lv->val.member.base;
      u32 member_id = lv->val.member.member_hash;

      /* pushes stack top */
      kit_vreg_t base = compile(cc, left);
      if (base < 0) return base;

      kit_vreg_t dst = vreg_alloc(cc);
      kit_emit_ins(cc, (kit_ins){ .member_access = { .opcode = KIT_IR_OPCODE_MEMBER_ACCESS, .dst = dst, .base = base, .member_id = member_id } });

      return dst;
    }

    default:
    case KIT_LVAL_UNKNOWN: return -1;
  }

  abort();
  return -1;
}