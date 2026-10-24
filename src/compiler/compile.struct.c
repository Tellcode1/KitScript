#include "../../inc/kit.ast.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.reg.h"
#include "../../inc/kit.rwhelp.h"
#include "compile_routines.h"
#include "compiler.tree.h"
#include "constants.h"
#include "defer.h"
#include "scope.h"
#include "tables.h"

int
collect_struct_declerations(kit_compiler* cc, int* stmts, u32 nstmts, kitc_struct_information* deposit)
{
  for (u32 i = 0; i < nstmts; i++) {
    kit_ast_node_type type = KIT_GET_NODE(cc->ast, stmts[i])->type;

    if (type == KIT_AST_NODE_STATEMENT_LIST) {
      int* list_stmts  = KIT_GET_NODE(cc->ast, stmts[i])->stmts.stmts;
      u32  list_nstmts = KIT_GET_NODE(cc->ast, stmts[i])->stmts.nstmts;
      // RECURSE!
      int e = collect_struct_declerations(cc, list_stmts, list_nstmts, deposit);
      if (e < 0) return e;
      continue;
    }

    if (type == KIT_AST_NODE_VARIABLE_DECL) {
      bool is_const = KIT_GET_NODE(cc->ast, stmts[i])->let.is_const;
      if (is_const) {
        cerror(KIT_GET_NODE(cc->ast, stmts[i])->let.span, "A member of a struct cannot be declared 'const' [struct decleration]\n");
        return -1;
      }

      int initializer = KIT_GET_NODE(cc->ast, stmts[i])->let.initializer;
      if (initializer >= 0) {
        cerror(KIT_GET_NODE(cc->ast, initializer)->common.span, "Initializer given in decleration of struct member [struct decleration]\n");
        return -1;
      }

      const char* name = KIT_GET_NODE(cc->ast, stmts[i])->let.name;
      int         e    = append_struct_decleration(cc->arena, name, deposit);
      if (e < 0) return e;
    } else {
      const char* member_name = KIT_GET_NODE(cc->ast, stmts[i])->let.name;
      cerror(
          KIT_GET_NODE(cc->ast, stmts[i])->let.span,
          "Member index %u, with name %s is not allowed in a struct [struct decleration]\n",
          i,
          member_name);
      return -1;
    }
  }

  return 0;
}

kit_vreg_t
compile_struct_constructor(kit_compiler* fork, kit_filespan span, const kitc_struct_information* struc)
{
  u32        struct_id = kit_hash(struc->name, strlen(struc->name));
  kit_vreg_t tmp       = vreg_alloc(fork);

  kit_str_intern(struc->name, fork->ast->interner);

  /* Since struct constructors are called as ordinary functions, we don't have to handle register spilling here */
  for (u32 i = 0; i < struc->fields_count; i++) {
    if (i < KIT_REG_ARG_COUNT) {
      kit_vreg_t reg = (kit_vreg_t)(KIT_REG_ARG0 + i);                        /* Define our variable in the argument register */
      scope_define_in_register(fork, reg, span, struc->field_names[i], true); // let the compiler say it's constant
    } else {
      scope_define(fork, span, struc->field_names[i], true);
      /* The rest of the arguments will already be on the stack (call's job). We don't need to do anything. */
    }
  }

  /* Make the structure. Our inputs are already in the argument register and the stack (calling convention says so.) */
  kit_emit_ins(fork, (kit_ins){ .mk_struct = { .opcode = KIT_IR_OPCODE_MK_STRUCT, .dst = tmp, .struct_id = struct_id } });

  /* Return from our temporary register */
  kit_emit_ins(fork, (kit_ins){ .ret = { .opcode = KIT_IR_OPCODE_RET, .return_value = tmp } });

  /* No need to clean up the stack, the CALL handler is responsible for that */

  kitc_function f = {
    .code        = fork->instructions,
    .code_count  = fork->ninstructions,
    .name_hash   = struct_id,
    .nargs       = struc->fields_count,
    .vregs_used  = fork->next_vreg,
    .labels_used = fork->next_label,
  };

  int e = append_function_entry(fork->arena, fork->function_table, &f);
  if (e < 0) return e;

  return 0;
}

kit_vreg_t
compile_struct_decleration(kit_compiler* cc, int node)
{
  int                     e           = 0;
  kit_compiler            fork        = { 0 };
  kitc_struct_information struct_data = { 0 };

  const char* struct_name        = KIT_GET_NODE(cc->ast, node)->struct_decl.name;
  int*        struct_decl_stmts  = KIT_GET_NODE(cc->ast, node)->struct_decl.stmts;
  u32         struct_decl_nstmts = KIT_GET_NODE(cc->ast, node)->struct_decl.nstmts;

  /* intern structure name for debug symbols. */
  kit_str_intern(struct_name, cc->ast->interner);

  /* Gather all information the user provided into one big structure. */
  struct_data = (kitc_struct_information){
    .name           = kit_arnstrdup(cc->arena, struct_name),
    .name_hash      = kit_hash(struct_name, strlen(struct_name)),
    .field_hashes   = (u32*)kit_xalloc(init_fields_capacity, sizeof(u32)),
    .field_names    = (char**)kit_xalloc(init_fields_capacity, sizeof(char**)),
    .field_capacity = init_fields_capacity,
    .fields_count   = 0,
  };
  if (!struct_data.field_hashes) goto ERR;

  e = collect_struct_declerations(cc, struct_decl_stmts, struct_decl_nstmts, &struct_data);
  if (e < 0) goto ERR;

  /* Generate the constructor function */
  e = compiler_make_fork(cc, &fork);
  if (e < 0) goto ERR;

  scope_push(&fork);

  e = defer_push_scope(&fork);
  if (e < 0) goto ERR;

  e = compile_struct_constructor(&fork, KIT_GET_NODE(cc->ast, node)->common.span, &struct_data);
  if (e < 0) goto ERR;

  e = append_struct_info(cc, &struct_data);
  if (e < 0) goto ERR;

  defer_pop_scope(&fork);
  scope_pop(&fork);

  compiler_join_fork(&fork, cc);

  return 0;

ERR:
  if (struct_data.field_hashes) free(struct_data.field_hashes);
  if (struct_data.field_names) free((void*)struct_data.field_names);
  compiler_free_fork_entirely(&fork);
  return e ? e : -1;
}