#ifndef KIT_COMPILER_LVALUE_H
#define KIT_COMPILER_LVALUE_H

#include "../../inc/kit.ast.h"
#include "../../inc/kit.ir.h"
#include "../../inc/kit.stdafx.h"

typedef enum kit_lval_type {
  KIT_LVAL_VAR,
  KIT_LVAL_GVAR,    // global variable
  KIT_LVAL_MEMBER,  // struct member
  KIT_LVAL_INDEX,   // indexed array
  KIT_LVAL_UNKNOWN, // Error!
} lval_type;

typedef union kit_lval_value {
  struct {
    u32   id;
    char* name; // allocated
  } var, gvar;
  struct {
    int         base;
    u32         member_hash;
    const char* member;
  } member;
  struct {
    int left_node;  // Compile to get LHS of index. For vec[16], it will push vec to stack.
    int index_node; // Compile to get index. For vec[16], it will push 16 to stack.

    // ereg_t cache_base;
    // ereg_t cache_index;
  } index;
} lval_value;

typedef struct val_t {
  kit_filespan* span;
  lval_type     type;
  lval_value    val;
} val_t;

static inline bool
can_make_value(const kit_ast* ast, int node)
{
  if (ast == NULL || node < 0) return false;
  return KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_VARIABLE || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_INDEX
      || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_INDEX_ASSIGN || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_MEMBER_ACCESS
      || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_MEMBER_ASSIGN || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_VARIABLE_DECL;
}

RETURNS_ERRCODE int        value_init(struct kit_compiler* cc, int node, struct val_t* d);
void                       value_free(struct val_t* lv);
RETURNS_ERRCODE kit_vreg_t emit_lvalue_load(struct kit_compiler* cc, struct val_t* lv);
RETURNS_ERRCODE kit_vreg_t emit_lvalue_assign(struct kit_compiler* cc, kit_vreg_t value, struct val_t* lv);

#endif // KIT_COMPILER_LVALUE_H