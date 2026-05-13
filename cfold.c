#include "ast.h"
#include "operate.h"
#include "opt.h"
#include "stdafx.h"
#include "var.h"

static inline e_opcode
e_binary_operator_to_opcode(e_operator op)
{
  switch (op) {
    case E_OPERATOR_ADD: return E_OPCODE_ADD;
    case E_OPERATOR_SUB: return E_OPCODE_SUB;
    case E_OPERATOR_MUL: return E_OPCODE_MUL;
    case E_OPERATOR_DIV: return E_OPCODE_DIV;
    case E_OPERATOR_MOD: return E_OPCODE_MOD;
    case E_OPERATOR_EXP: return E_OPCODE_EXP;
    case E_OPERATOR_AND: return E_OPCODE_AND;
    case E_OPERATOR_OR: return E_OPCODE_OR;
    case E_OPERATOR_BAND: return E_OPCODE_BAND;
    case E_OPERATOR_BOR: return E_OPCODE_BOR;
    case E_OPERATOR_XOR: return E_OPCODE_XOR;
    case E_OPERATOR_ISEQL: return E_OPCODE_EQL;
    case E_OPERATOR_ISNEQ: return E_OPCODE_NEQ;
    case E_OPERATOR_LT: return E_OPCODE_LT;
    case E_OPERATOR_LTE: return E_OPCODE_LTE;
    case E_OPERATOR_GT: return E_OPCODE_GT;
    case E_OPERATOR_GTE: return E_OPCODE_GTE;
    case E_OPERATOR_NOT: return E_OPCODE_NOT;
    case E_OPERATOR_BNOT: return E_OPCODE_BNOT;
    case E_OPERATOR_DEC: return E_OPCODE_DEC;
    case E_OPERATOR_INC: return E_OPCODE_INC;
  }
  return -1;
}

static inline RETURNS_ERRCODE int
lower_node_to_literal(const e_ast* p, int node, e_var* o)
{
  switch (E_GET_NODE(p, node)->type) {
    case E_AST_NODE_INT: *o = (e_var){ .type = E_VARTYPE_INT, .val.i = E_GET_NODE(p, node)->i.i }; return 0;
    case E_AST_NODE_FLOAT: *o = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = E_GET_NODE(p, node)->f.f }; return 0;
    case E_AST_NODE_CHAR: *o = (e_var){ .type = E_VARTYPE_CHAR, .val.c = E_GET_NODE(p, node)->c.c }; return 0;
    case E_AST_NODE_BOOL: *o = (e_var){ .type = E_VARTYPE_BOOL, .val.b = E_GET_NODE(p, node)->b.b }; return 0;

    default: return 1;
  }
  return 1;
}

static inline e_ast_node_type
var_type_to_node_type(e_vartype var_type)
{
  switch (var_type) {
    case E_VARTYPE_INT: return E_AST_NODE_INT;
    case E_VARTYPE_BOOL: return E_AST_NODE_BOOL;
    case E_VARTYPE_CHAR: return E_AST_NODE_CHAR;
    case E_VARTYPE_FLOAT: return E_AST_NODE_FLOAT;
    case E_VARTYPE_STRING: return E_AST_NODE_STRING;
    default: return E_AST_NODE_NOP;
  }
}

static inline RETURNS_ERRCODE int
convert_literal_to_node(e_ast* p, int override_node, const e_var* lit)
{
  E_GET_NODE(p, override_node)->type = var_type_to_node_type(lit->type);
  switch (lit->type) {
    case E_VARTYPE_INT: E_GET_NODE(p, override_node)->i.i = lit->val.i; break;
    case E_VARTYPE_BOOL: E_GET_NODE(p, override_node)->b.b = lit->val.b; break;
    case E_VARTYPE_CHAR: E_GET_NODE(p, override_node)->c.c = lit->val.c; break;
    case E_VARTYPE_FLOAT: E_GET_NODE(p, override_node)->f.f = lit->val.f; break;
    case E_VARTYPE_STRING: E_GET_NODE(p, override_node)->i.i = lit->val.i; break;
    default: return -1;
  }
  return 0;
}

static inline bool
is_literal_value(const e_ast* ast, int node)
{
  e_ast_node_type type = E_GET_NODE(ast, node)->type;
  return type == E_AST_NODE_INT || type == E_AST_NODE_CHAR || type == E_AST_NODE_BOOL || type == E_AST_NODE_STRING || type == E_AST_NODE_FLOAT;
}

static inline int
fold(e_ast* ast, int node)
{
  int e = 0;

  e_ast_node* nodep = e_ast_get_node(ast, node);
  switch (nodep->type) {
    case E_AST_NODE_BINARYOP: {
      int l = nodep->binaryop.left;
      int r = nodep->binaryop.right;

      e = fold(ast, l);
      if (e) return e;

      e = fold(ast, r);
      if (e) return e;

      if (!is_literal_value(ast, l) || !is_literal_value(ast, r)) return 0;

      e_var lv;
      e_var rv;

      e = lower_node_to_literal(ast, l, &lv);
      if (e) return e;

      e = lower_node_to_literal(ast, r, &rv);
      if (e) return e;

      e_var result = operate(lv, rv, e_binary_operator_to_opcode(E_GET_NODE(ast, node)->binaryop.op));

      e = convert_literal_to_node(ast, node, &result);
      if (e) return e;
      break;
    }

    case E_AST_NODE_ROOT: {
      for (u32 i = 0; i < nodep->root.nstmts; i++) {
        e = fold(ast, nodep->root.stmts[i]);
        if (e) return e;
      }
      return 0;
    }
    case E_AST_NODE_STATEMENT_LIST: {
      for (u32 i = 0; i < nodep->stmts.nstmts; i++) {
        e = fold(ast, nodep->stmts.stmts[i]);
        if (e) return e;
      }
      return 0;
    }
    case E_AST_NODE_DEFER: {
      for (u32 i = 0; i < nodep->defer.nstmts; i++) {
        e = fold(ast, nodep->defer.stmts[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_FUNCTION_DEFINITION: {
      for (u32 i = 0; i < nodep->func.nstmts; i++) {
        e = fold(ast, nodep->func.stmts[i]);
        if (e) return e;
      }
      /* Can not optimize arguments in function definition. CALL should do that. */
      return 0;
    }

    case E_AST_NODE_NAMESPACE_DECL: {
      for (u32 i = 0; i < nodep->namespace_decl.nstmts; i++) {
        e = fold(ast, nodep->namespace_decl.stmts[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_CALL: {
      for (u32 i = 0; i < nodep->call.nargs; i++) {
        e = fold(ast, nodep->call.args[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      if (nodep->let.initializer >= 0) {
        e = fold(ast, nodep->let.initializer);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_ASSIGN: {
      e = fold(ast, nodep->assign.left);
      if (e) return e;
      e = fold(ast, nodep->assign.right);
      return e;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      e = fold(ast, nodep->member_assign.left);
      if (e) return e;
      e = fold(ast, nodep->member_assign.value);
      return e;
    }

    case E_AST_NODE_INDEX: {
      e = fold(ast, nodep->index.index);
      return 0;
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      e = fold(ast, nodep->index_assign.index);
      e = fold(ast, nodep->index_assign.value);
      return 0;
    }

    case E_AST_NODE_INDEX_COMPOUND_OP: {
      e = fold(ast, nodep->index_compound.index);
      e = fold(ast, nodep->index_compound.value);
      return 0;
    }

    case E_AST_NODE_FOR: {
      e = fold(ast, nodep->for_stmt.condition);
      if (e) return e;

      e = fold(ast, nodep->for_stmt.initializers);
      if (e) return e;

      for (u32 i = 0; i < nodep->for_stmt.nstmts; i++) {
        e = fold(ast, nodep->for_stmt.stmts[i]);
        if (e) return e;
      }

      for (u32 i = 0; i < nodep->for_stmt.niterators; i++) {
        e = fold(ast, nodep->for_stmt.iterators[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_WHILE: {
      for (u32 i = 0; i < nodep->while_stmt.nstmts; i++) {
        e = fold(ast, nodep->while_stmt.stmts[i]);
        if (e) return e;
      }

      e = fold(ast, nodep->while_stmt.condition);
      if (e) return e;

      return 0;
    }

    case E_AST_NODE_IF: {
      e = fold(ast, nodep->if_stmt.condition);
      if (e) return e;

      for (u32 i = 0; i < nodep->if_stmt.nstmts; i++) {
        e = fold(ast, nodep->if_stmt.body[i]);
        if (e) return e;
      }

      u32 nelse_ifs = nodep->if_stmt.nelse_ifs;
      for (u32 i = 0; i < nelse_ifs; i++) {
        struct e_if_stmt* else_if = &nodep->if_stmt.else_ifs[i];

        e = fold(ast, else_if->condition);
        if (e) return e;

        for (u32 j = 0; j < else_if->nstmts; j++) {
          e = fold(ast, else_if->body[j]);
          if (e) return e;
        }
      }

      for (u32 i = 0; i < nodep->if_stmt.nelse_stmts; i++) {
        e = fold(ast, nodep->if_stmt.else_body[i]);
        if (e) return e;
      }

      return 0;
    }

    default: return 0;
  }

  return 0;
}

int
eopt_constant_fold(eopt_stage stage, eopt_data* data)
{
  int root = data->ast->root;
  return fold(data->ast, root);
}