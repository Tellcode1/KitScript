#include "../inc/kit.ast.h"
#include "../inc/kit.var.h"

kit_var_type
kit_infer_node_type(const kit_ast* ast, int node)
{
  switch (KIT_GET_NODE(ast, node)->type) {
    case KIT_AST_NODE_NOP:
    case KIT_AST_NODE_ROOT:
    case KIT_AST_NODE_FOR:
    case KIT_AST_NODE_WHILE:
    case KIT_AST_NODE_DEFER: return KIT_VARTYPE_NULL;

    case KIT_AST_NODE_FUNCTION_DEFINITION:

    case KIT_AST_NODE_BINARYOP:
    case KIT_AST_NODE_UNARYOP:
    case KIT_AST_NODE_STATEMENT_LIST:
    case KIT_AST_NODE_BREAK:
    case KIT_AST_NODE_CONTINUE:
    case KIT_AST_NODE_RETURN:
    case KIT_AST_NODE_IF:
    case KIT_AST_NODE_VARIABLE:
    case KIT_AST_NODE_VARIABLE_DECL:
    case KIT_AST_NODE_ASSIGN:
    case KIT_AST_NODE_INDEX:
    case KIT_AST_NODE_INDEX_ASSIGN:
    case KIT_AST_NODE_INDEX_COMPOUND_OP:
    case KIT_AST_NODE_MEMBER_ACCESS:
    case KIT_AST_NODE_MEMBER_ASSIGN:
    case KIT_AST_NODE_NAMESPACE_DECL:
    case KIT_AST_NODE_STRUCT_DECL:
    case KIT_AST_NODE_CALL:
    case KIT_AST_NODE_INT:
    case KIT_AST_NODE_CHAR:
    case KIT_AST_NODE_BOOL:
    case KIT_AST_NODE_STRING:
    case KIT_AST_NODE_FLOAT:
    case KIT_AST_NODE_LIST:
    case KIT_AST_NODE_MAP: break;
  }
}