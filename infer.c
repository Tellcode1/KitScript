#include "ast.h"
#include "opt.h"
#include "var.h"

e_vartype
e_infer_node_type(const e_ast* ast, int node)
{
  switch (E_GET_NODE(ast, node)->type) {
    case E_AST_NODE_NOP:
    case E_AST_NODE_ROOT:
    case E_AST_NODE_FOR:
    case E_AST_NODE_WHILE:
    case E_AST_NODE_DEFER: return E_VARTYPE_NULL;

    case E_AST_NODE_FUNCTION_DEFINITION:

    case E_AST_NODE_BINARYOP:
    case E_AST_NODE_UNARYOP:
    case E_AST_NODE_STATEMENT_LIST:
    case E_AST_NODE_BREAK:
    case E_AST_NODE_CONTINUE:
    case E_AST_NODE_RETURN:
    case E_AST_NODE_IF:
    case E_AST_NODE_VARIABLE:
    case E_AST_NODE_VARIABLE_DECL:
    case E_AST_NODE_ASSIGN:
    case E_AST_NODE_INDEX:
    case E_AST_NODE_INDEX_ASSIGN:
    case E_AST_NODE_INDEX_COMPOUND_OP:
    case E_AST_NODE_MEMBER_ACCESS:
    case E_AST_NODE_MEMBER_ASSIGN:
    case E_AST_NODE_NAMESPACE_DECL:
    case E_AST_NODE_STRUCT_DECL:
    case E_AST_NODE_CALL:
    case E_AST_NODE_INT:
    case E_AST_NODE_CHAR:
    case E_AST_NODE_BOOL:
    case E_AST_NODE_STRING:
    case E_AST_NODE_FLOAT:
    case E_AST_NODE_LIST:
    case E_AST_NODE_MAP: break;
  }
}