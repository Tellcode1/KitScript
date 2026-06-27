/**
 * MIT License
 *
 * Copyright (c) 2026 Tellcode1
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef KIT_AST_H
#define KIT_AST_H

#include "kit.cerr.h"
#include "kit.lex.h"
#include "kit.strint.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum kit_ast_nodetype {
  KIT_AST_NODE_NOP,

  KIT_AST_NODE_ROOT,
  KIT_AST_NODE_BINARYOP,
  KIT_AST_NODE_UNARYOP,
  KIT_AST_NODE_STATEMENT_LIST,

  KIT_AST_NODE_ASSERT,
  KIT_AST_NODE_DEFER,
  KIT_AST_NODE_FOR,
  KIT_AST_NODE_RANGED_FOR, /* for [let] x in 0..2 */
  KIT_AST_NODE_WHILE,
  KIT_AST_NODE_BREAK,
  KIT_AST_NODE_CONTINUE,
  KIT_AST_NODE_RETURN,
  KIT_AST_NODE_IF,
  KIT_AST_NODE_FUNCTION_DEFINITION,

  KIT_AST_NODE_VARIABLE,

  // let <name> [ : type] [ = initializer];
  KIT_AST_NODE_VARIABLE_DECL,
  KIT_AST_NODE_ASSIGN,
  KIT_AST_NODE_INDEX,
  KIT_AST_NODE_INDEX_ASSIGN, // Assign to member

  // x.y
  KIT_AST_NODE_MEMBER_ACCESS,
  KIT_AST_NODE_MEMBER_ASSIGN,

  KIT_AST_NODE_NAMESPACE_DECL,
  KIT_AST_NODE_STRUCT_DECL,

  KIT_AST_NODE_CALL,
  KIT_AST_NODE_INT,
  KIT_AST_NODE_CHAR,
  KIT_AST_NODE_BOOL,
  KIT_AST_NODE_STRING,
  KIT_AST_NODE_FLOAT,
  KIT_AST_NODE_LIST,
  KIT_AST_NODE_MAP,
} kit_ast_node_type;

typedef enum kit_operator {
  KIT_OPERATOR_ADD,
  KIT_OPERATOR_SUB,
  KIT_OPERATOR_MUL,
  KIT_OPERATOR_DIV,
  KIT_OPERATOR_MOD, // Integral remainder
  KIT_OPERATOR_EXP, // Exponent
  KIT_OPERATOR_AND,
  KIT_OPERATOR_NOT,
  KIT_OPERATOR_OR,
  KIT_OPERATOR_BAND,
  KIT_OPERATOR_BOR,
  KIT_OPERATOR_BNOT,
  KIT_OPERATOR_XOR,
  KIT_OPERATOR_ISEQL, // Equal?
  KIT_OPERATOR_ISNEQ, // Not equal?

  KIT_OPERATOR_LT,
  KIT_OPERATOR_LTE,
  KIT_OPERATOR_GT,
  KIT_OPERATOR_GTE,

  /**
   * Contrary to C, the VM doesn't push any value for these. Only increments the variable.
   * Treated as compound operators always!
   */
  KIT_OPERATOR_DEC,
  KIT_OPERATOR_INC,
} kit_operator;

typedef enum kit_if_level {
  KIT_IF_LEVEL_TOP,
  KIT_IF_LEVEL_ELSE_IF,
  KIT_IF_LEVEL_ELSE,
} kit_if_level;

typedef struct kit_if_stmt {
  kit_ast_node_type type;
  kit_filespan      span;

  int*                body;
  int*                else_body; // NULL for no else statement
  struct kit_if_stmt* else_ifs;  // Allocated, free after compilation.

  int condition;
  u32 nstmts;
  u32 nelse_stmts;
  u32 nelse_ifs;
} kit_if_stmt;

/**
 * A tagged union similar to SDL_Event.
 * Structured this way to lessen the node->as.<typedata> to
 * node-><typedata>.
 * By placing the common fields in the same location in every union,
 * We guarantee they'll be in the same memory address everytime.
 * It's standard.
 */
typedef union kit_ast_node_val {
  kit_ast_node_type type;
  struct {
    kit_ast_node_type type;
    kit_filespan      span;
  } common;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               i;
  } i;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    bool              b;
  } b;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    char              c;
  } c;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    double            f;
  } f;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    char*             s; // Allocated ; free after use
  } s;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    const char*       ident; /* interned string ; DO NOT FREE */
  } ident;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    const char*       name;        /* interned string ; DO NOT FREE */
    int               initializer; // -1 if not given.
    bool              is_const;
  } let;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    // Node ID of the expression
    int expr_id;
    // If false, expr_id is < 0 and function must return void.
    bool has_return_value;
  } ret;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int*              elems;
    u32               nelems;
  } list;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int*              keys;
    int*              values;
    u32               npairs;
  } map;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               left; // index of left
    int               right;
    kit_operator      op;
    bool              is_compound;
  } binaryop, assign;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               base;
    int               index;
  } index;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               base;  // list/structure
    int               index; // index: integer
    int               value; // any value.
  } index_assign;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               base;  // list/structure
    int               index; // index: integer
    int               value; // any value.
    kit_operator      op;
  } index_compound;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               right; // index of right
    kit_operator      op;
    bool              is_compound;
  } unaryop;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               stmt;
    char*             assertion_line; // The line in string form (like assert have_feature();)
    // sorry for the bad comments im sleepy
  } assertion;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int*              stmts;
    u32               nstmts;
    u32               capacity; // root needs this.
  } stmts, root, defer;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    const char*       struct_name;
    const char**      member_names;
    int*              stmts;
    u32               nstmts;
  } initializer_list;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;

    /* interned string ; DO NOT FREE */
    const char* name;  // Namespace name
    int*        stmts; // All statements in namespace
    u32         nstmts;
  } namespace_decl;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;

    /* interned string ; DO NOT FREE */
    const char* name;  // structure name
    int*        stmts; // All statements in structure decl
    u32         nstmts;
  } struct_decl;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               left;
    const char*       right; /* interned string ; DO NOT FREE */
    int               value;
  } member_assign;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               left;
    const char*       right; /* interned string ; DO NOT FREE */
  } member_access;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               func; /* The node ID containing the function, an identifier or a closure. */
    int*              args;
    u32               nargs;
  } call;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    const char*       name;  /* interned string ; DO NOT FREE */
    const char**      args;  // Each string is interned. DO NOT FREE INDIVIDUALS.
    int*              stmts; // Function body
    u32               nargs;
    u32               nstmts;
  } func;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               condition;
    int*              stmts;
    u32               nstmts;
  } while_stmt;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int               initializers; // EXPRESSION_LIST, for (let x = 0, y = 16;
    int               condition;    // x >= 0;
    int*              iterators;    // x++,y--)
    u32               niterators;
    u32               nstmts;
    int*              stmts;
  } for_stmt;

  struct {
    kit_ast_node_type type;
    kit_filespan      span;
    int*              stmts;
    char*             iterator_name; /* for iterator_name in 0..5, malloc'd */
    int               start;
    int               stop;
    u32               nstmts;
  } for_range_stmt;

  kit_if_stmt if_stmt;
} kit_ast_node_val;

typedef kit_ast_node_val kit_ast_node;

typedef struct kit_ast {
  kit_str_interner* interner;

  kit_ast_node* nodes;
  u32           nnodes;
  u32           capacity;

  /**
   * MUST BE EXPLICITLY SET TO -1
   * If not, on error free's will try to go the usual way,
   * But the node is NULL, so they will fail.
   * And give you a sigsegv.
   */
  int root;
} kit_ast;

typedef struct kit_parser {
  const struct kit_token* toks;
  u32                     ntoks;
  u32                     head;

  /* Target AST */
  struct kit_ast* ast;
} kit_parser;

int  kit_ast_init(kit_str_interner* interner, kit_ast* ast) ATTR_NODISCARD;
void kit_ast_free(kit_ast* ast);

int  kit_parser_init(const kit_token* tokens, u32 ntokens, kit_ast* ast, kit_parser* parser) ATTR_NODISCARD;
void kit_parser_free(kit_parser* parser);

/**
 * Recursively free a node in the tree.
 */
void kit_ast_node_free(kit_ast* p, int id);

static inline const kit_token*
kit_parser_next(kit_parser* prsr)
{
  if (prsr->head > prsr->ntoks) return NULL;
  return &prsr->toks[prsr->head++];
}
static inline const kit_token*
kit_parser_peek(const kit_parser* prsr)
{
  if (prsr->head >= prsr->ntoks) return NULL;
  return &prsr->toks[prsr->head];
}
static inline const kit_token*
kit_parser_prev(const kit_parser* prsr)
{
  if (prsr->head > prsr->ntoks || prsr->head == 0) return NULL;
  return &prsr->toks[prsr->head - 1];
}

/**
 * Get binding power of a token.
 */
bool kit_getbp(kit_token_type type, int* left, int* right);

#define KIT_GET_NODE kit_ast_get_node

static inline kit_ast_node*
kit_ast_get_node(const kit_ast* p, int idx)
{ return idx >= 0 ? &p->nodes[idx] : NULL; }

static inline ATTR_NODISCARD int
kit_ast_make_node(kit_ast* p)
{
  if (p->nnodes >= p->capacity) {
    u32           newcap   = MAX(p->capacity * 2, p->nnodes + 1);
    kit_ast_node* newnodes = (kit_ast_node*)realloc(p->nodes, newcap * sizeof(kit_ast_node));
    if (newnodes == NULL) {
      perror("Allocation failed");
      return -1;
    }

    p->nodes    = newnodes;
    p->capacity = newcap;
  }

  u32 idx = p->nnodes;
  memset(&p->nodes[idx], 0, sizeof(kit_ast_node));
  p->nnodes++;

  return (int)idx;
}

static inline bool
kit_ast_is_limiter_exempt(kit_ast_node_type t)
{
  return (
      t == KIT_AST_NODE_IF || t == KIT_AST_NODE_WHILE || t == KIT_AST_NODE_FOR || t == KIT_AST_NODE_RANGED_FOR
      || t == KIT_AST_NODE_FUNCTION_DEFINITION || t == KIT_AST_NODE_DEFER || t == KIT_AST_NODE_NOP || t == KIT_AST_NODE_DEFER);
}

int kit_parse(kit_parser* p) ATTR_NODISCARD;
int kit_ast_nud(kit_parser* p, const kit_token* tk) ATTR_NODISCARD;
int kit_ast_expr(kit_parser* p, int rbp) ATTR_NODISCARD;
int kit_ast_led(kit_parser* p, const kit_token* tk, int leftidx, int rbp) ATTR_NODISCARD;

/**
 * Expect that peek() is pointing to a token
 * with type. Returns 0 on match.
 */
int kit_ast_expect(kit_parser* p, kit_token_type type) ATTR_NODISCARD;

#endif // KIT_AST_H
