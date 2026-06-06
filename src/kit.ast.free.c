#include "../inc/kit.ast.h"

#include <string.h>

static void
free_if_stmt(kit_ast* ast, kit_if_stmt* ifs)
{
  if (!ifs) return;

  if (ifs->condition >= 0) kit_ast_node_free(ast, ifs->condition);

  for (u32 i = 0; i < ifs->nstmts; i++) kit_ast_node_free(ast, ifs->body[i]);
  free(ifs->body);

  if (ifs->else_body) {
    for (u32 i = 0; i < ifs->nelse_stmts; i++) kit_ast_node_free(ast, ifs->else_body[i]);
    free(ifs->else_body);
  }

  if (ifs->else_ifs) {
    for (u32 i = 0; i < ifs->nelse_ifs; i++) free_if_stmt(ast, &ifs->else_ifs[i]);
    free(ifs->else_ifs);
  }
}

void
kit_ast_node_free(kit_ast* p, int id)
{
  if (!p || id < 0) return;

  kit_ast_node_val* node = &p->nodes[id];

  // if (node->type == KIT_AST_NODE_SENTINEL) return;
  // node->type = KIT_AST_NODE_SENTINEL;

  switch (node->type) {
    case KIT_AST_NODE_NOP:
    case KIT_AST_NODE_INT:
    case KIT_AST_NODE_CHAR:
    case KIT_AST_NODE_BOOL:
    case KIT_AST_NODE_FLOAT: break;

    case KIT_AST_NODE_STRING: free(node->s.s); break;

    case KIT_AST_NODE_VARIABLE: break;

    case KIT_AST_NODE_VARIABLE_DECL:
      if (node->let.initializer >= 0) kit_ast_node_free(p, node->let.initializer);
      break;

    case KIT_AST_NODE_ASSIGN:
      kit_ast_node_free(p, node->assign.left);
      kit_ast_node_free(p, node->assign.right);
      break;

    case KIT_AST_NODE_BINARYOP:
      kit_ast_node_free(p, node->binaryop.left);
      kit_ast_node_free(p, node->binaryop.right);
      break;

    case KIT_AST_NODE_UNARYOP: kit_ast_node_free(p, node->unaryop.right); break;

    case KIT_AST_NODE_INDEX:
      kit_ast_node_free(p, node->index.base);
      kit_ast_node_free(p, node->index.index);
      break;

    case KIT_AST_NODE_INDEX_ASSIGN:
      kit_ast_node_free(p, node->index_assign.base);
      kit_ast_node_free(p, node->index_assign.index);
      kit_ast_node_free(p, node->index_assign.value);
      break;

    case KIT_AST_NODE_MEMBER_ASSIGN:
      kit_ast_node_free(p, node->member_assign.left);
      kit_ast_node_free(p, node->member_assign.value);
      break;

    case KIT_AST_NODE_MEMBER_ACCESS: kit_ast_node_free(p, node->member_access.left); break;

    case KIT_AST_NODE_CALL:
      for (u32 i = 0; i < node->call.nargs; i++) kit_ast_node_free(p, node->call.args[i]);
      free(node->call.args);
      break;

    case KIT_AST_NODE_ASSERT:
      free(node->assertion.assertion_line);
      kit_ast_node_free(p, node->assertion.stmt);
      break;

    case KIT_AST_NODE_FUNCTION_DEFINITION:
      // Strings are interned. Don't free.
      // for (u32 i = 0; i < node->func.nargs; i++) free(node->func.args[i]);
      free((void*)node->func.args);

      for (u32 i = 0; i < node->func.nstmts; i++) kit_ast_node_free(p, node->func.stmts[i]);
      free(node->func.stmts);
      break;

    case KIT_AST_NODE_DEFER:
      for (u32 i = 0; i < node->defer.nstmts; i++) kit_ast_node_free(p, node->defer.stmts[i]);
      free(node->defer.stmts);
      break;

    case KIT_AST_NODE_STATEMENT_LIST:
      for (u32 i = 0; i < node->stmts.nstmts; i++) kit_ast_node_free(p, node->stmts.stmts[i]);
      free(node->stmts.stmts);
      break;

    case KIT_AST_NODE_ROOT:
      for (u32 i = 0; i < node->root.nstmts; i++) kit_ast_node_free(p, node->root.stmts[i]);
      free(node->root.stmts);
      break;

    case KIT_AST_NODE_NAMESPACE_DECL:
      for (u32 i = 0; i < node->namespace_decl.nstmts; i++) kit_ast_node_free(p, node->namespace_decl.stmts[i]);
      free(node->namespace_decl.stmts);
      break;

    case KIT_AST_NODE_STRUCT_DECL:
      for (u32 i = 0; i < node->struct_decl.nstmts; i++) kit_ast_node_free(p, node->struct_decl.stmts[i]);
      free(node->struct_decl.stmts);
      break;

    case KIT_AST_NODE_WHILE:
      kit_ast_node_free(p, node->while_stmt.condition);
      for (u32 i = 0; i < node->while_stmt.nstmts; i++) kit_ast_node_free(p, node->while_stmt.stmts[i]);
      free(node->while_stmt.stmts);
      break;

    case KIT_AST_NODE_FOR:
      kit_ast_node_free(p, node->for_stmt.initializers);
      kit_ast_node_free(p, node->for_stmt.condition);

      for (u32 i = 0; i < node->for_stmt.niterators; i++) kit_ast_node_free(p, node->for_stmt.iterators[i]);
      free(node->for_stmt.iterators);

      for (u32 i = 0; i < node->for_stmt.nstmts; i++) kit_ast_node_free(p, node->for_stmt.stmts[i]);
      free(node->for_stmt.stmts);
      break;

    case KIT_AST_NODE_RANGED_FOR: {
      free(node->for_range_stmt.iterator_name);
      for (u32 i = 0; i < node->for_range_stmt.nstmts; i++) kit_ast_node_free(p, node->for_range_stmt.stmts[i]);
      free(node->for_range_stmt.stmts);
      break;
    }

    case KIT_AST_NODE_IF: free_if_stmt(p, &node->if_stmt); break;

    case KIT_AST_NODE_RETURN:
      if (node->ret.has_return_value) kit_ast_node_free(p, node->ret.expr_id);
      break;

    case KIT_AST_NODE_LIST:
      for (u32 i = 0; i < node->list.nelems; i++) kit_ast_node_free(p, node->list.elems[i]);
      free(node->list.elems);
      break;

    case KIT_AST_NODE_MAP:
      for (u32 i = 0; i < node->map.npairs; i++) {
        kit_ast_node_free(p, node->map.keys[i]);
        kit_ast_node_free(p, node->map.values[i]);
      }
      free(node->map.keys);
      free(node->map.values);
      break;

    case KIT_AST_NODE_BREAK:
    case KIT_AST_NODE_CONTINUE: break;
  }

  memset(node, 0, sizeof *node);
  node->type = KIT_AST_NODE_NOP;
}