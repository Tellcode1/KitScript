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

#ifndef E_AST_H
#define E_AST_H

#include "cerr.h"
#include "lex.h"
#include "strint.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum e_ast_nodetype {
  E_AST_NODE_NOP,

  E_AST_NODE_ROOT,
  E_AST_NODE_BINARYOP,
  E_AST_NODE_UNARYOP,
  E_AST_NODE_STATEMENT_LIST,
  // E_AST_NODE_INITIALIZER_LIST,

  E_AST_NODE_ASSERT,
  E_AST_NODE_DEFER,
  E_AST_NODE_FOR,
  E_AST_NODE_WHILE,
  E_AST_NODE_BREAK,
  E_AST_NODE_CONTINUE,
  E_AST_NODE_RETURN,
  E_AST_NODE_IF,
  E_AST_NODE_FUNCTION_DEFINITION,

  E_AST_NODE_VARIABLE,

  // let <name> [ : type] [ = initializer];
  E_AST_NODE_VARIABLE_DECL,
  E_AST_NODE_ASSIGN,
  E_AST_NODE_INDEX,
  E_AST_NODE_INDEX_ASSIGN,      // Assign to member
  E_AST_NODE_INDEX_COMPOUND_OP, // Assign to member, using a compound operator (unary/binary)

  // x.y
  E_AST_NODE_MEMBER_ACCESS,
  E_AST_NODE_MEMBER_ASSIGN,

  E_AST_NODE_NAMESPACE_DECL,
  E_AST_NODE_STRUCT_DECL,

  E_AST_NODE_CALL,
  E_AST_NODE_INT,
  E_AST_NODE_CHAR,
  E_AST_NODE_BOOL,
  E_AST_NODE_STRING,
  E_AST_NODE_FLOAT,
  E_AST_NODE_LIST,
  E_AST_NODE_MAP,
} e_ast_node_type;

typedef enum e_operator {
  E_OPERATOR_ADD,
  E_OPERATOR_SUB,
  E_OPERATOR_MUL,
  E_OPERATOR_DIV,
  E_OPERATOR_MOD, // Integral remainder
  E_OPERATOR_EXP, // Exponent
  E_OPERATOR_AND,
  E_OPERATOR_NOT,
  E_OPERATOR_OR,
  E_OPERATOR_BAND,
  E_OPERATOR_BOR,
  E_OPERATOR_BNOT,
  E_OPERATOR_XOR,
  E_OPERATOR_ISEQL, // Equal?
  E_OPERATOR_ISNEQ, // Not equal?

  E_OPERATOR_LT,
  E_OPERATOR_LTE,
  E_OPERATOR_GT,
  E_OPERATOR_GTE,

  /**
   * Contrary to C, the VM doesn't push any value for these. Only increments the variable.
   * Treated as compound operators always!
   */
  E_OPERATOR_DEC,
  E_OPERATOR_INC,
} e_operator;

typedef enum e_if_level {
  E_IF_LEVEL_TOP,
  E_IF_LEVEL_ELSE_IF,
  E_IF_LEVEL_ELSE,
} e_if_level;

typedef struct e_if_stmt {
  e_ast_node_type type;
  e_filespan      span;

  int*              body;
  int*              else_body; // NULL for no else statement
  struct e_if_stmt* else_ifs;  // Allocated, free after compilation.

  int condition;
  u32 nstmts;
  u32 nelse_stmts;
  u32 nelse_ifs;
} e_if_stmt;

/**
 * A tagged union similar to SDL_Event.
 * Structured this way to lessen the node->as.<typedata> to
 * node-><typedata>.
 * By placing the common fields in the same location in every union,
 * We guarantee they'll be in the same memory address everytime.
 * It's standard.
 */
typedef union e_ast_node_val {
  e_ast_node_type type;
  struct {
    e_ast_node_type type;
    e_filespan      span;
  } common;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             i;
  } i;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    bool            b;
  } b;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    char            c;
  } c;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    double          f;
  } f;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    char*           s; // Allocated ; free after use
  } s;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    const char*     ident; /* interned string ; DO NOT FREE */
  } ident;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    const char*     name;        /* interned string ; DO NOT FREE */
    int             initializer; // -1 if not given.
    bool            is_const;
  } let;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    // Node ID of the expression
    int expr_id;
    // If false, expr_id is < 0 and function must return void.
    bool has_return_value;
  } ret;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int*            elems;
    u32             nelems;
  } list;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int*            keys;
    int*            values;
    u32             npairs;
  } map;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             left; // index of left
    int             right;
    e_operator      op;
    bool            is_compound;
  } binaryop, assign;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             base;
    int             index;
  } index;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             base;  // list/structure
    int             index; // index: integer
    int             value; // any value.
  } index_assign;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             base;  // list/structure
    int             index; // index: integer
    int             value; // any value.
    e_operator      op;
  } index_compound;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             right; // index of right
    e_operator      op;
    bool            is_compound;
  } unaryop;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             stmt;
    char*           assertion_line; // The line in string form (like assert have_feature();)
    // sorry for the bad comments im sleepy
  } assertion;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int*            stmts;
    u32             nstmts;
  } stmts, root, defer;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    const char*     struct_name;
    const char**    member_names;
    int*            stmts;
    u32             nstmts;
  } initializer_list;

  struct {
    e_ast_node_type type;
    e_filespan      span;

    /* interned string ; DO NOT FREE */
    const char* name;  // Namespace name
    int*        stmts; // All statements in namespace
    u32         nstmts;
  } namespace_decl;

  struct {
    e_ast_node_type type;
    e_filespan      span;

    /* interned string ; DO NOT FREE */
    const char* name;  // structure name
    int*        stmts; // All statements in structure decl
    u32         nstmts;
  } struct_decl;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             left;
    const char*     right; /* interned string ; DO NOT FREE */
    int             value;
  } member_assign;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             left;
    const char*     right; /* interned string ; DO NOT FREE */
  } member_access;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    const char*     function_name; /* interned string ; DO NOT FREE */
    int*            args;
    u32             nargs;
  } call;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    const char*     name;  /* interned string ; DO NOT FREE */
    const char**    args;  // Each string is interned. DO NOT FREE INDIVIDUALS.
    int*            stmts; // Function body
    u32             nargs;
    u32             nstmts;
  } func;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             condition;
    int*            stmts;
    u32             nstmts;
  } while_stmt;

  struct {
    e_ast_node_type type;
    e_filespan      span;
    int             initializers; // EXPRESSION_LIST, for (let x = 0, y = 16;
    int             condition;    // x >= 0;
    int*            iterators;    // x++,y--)
    u32             niterators;
    u32             nstmts;
    int*            stmts;
  } for_stmt;

  e_if_stmt if_stmt;
} e_ast_node_val;

typedef e_ast_node_val e_ast_node;

typedef struct e_ast {
  e_str_interner* interner;

  e_ast_node* nodes;
  u32         nnodes;
  u32         capacity;

  e_token* toks;
  u32      ntoks;
  u32      head;

  /**
   * MUST BE EXPLICITLY SET TO -1
   * If not, on error free's will try to go the usual way,
   * But the node is NULL, so they will fail.
   * And give you a sigsegv.
   */
  int root;
} e_ast;

int  e_ast_init(e_token* toks, u32 ntoks, e_str_interner* interner, e_ast* prsr) ATTR_NODISCARD;
void e_ast_free(e_ast* prsr);

/**
 * Recursively free a node in the tree.
 */
void e_ast_node_free(e_ast* p, int id);

static inline e_token*
e_ast_next(struct e_ast* prsr)
{
  if (prsr->head > prsr->ntoks) return NULL;
  return &prsr->toks[prsr->head++];
}
static inline e_token*
e_ast_peek(const struct e_ast* prsr)
{
  if (prsr->head >= prsr->ntoks) return NULL;
  return &prsr->toks[prsr->head];
}
static inline e_token*
e_ast_prev(const struct e_ast* prsr)
{
  if (prsr->head > prsr->ntoks || prsr->head == 0) return NULL;
  return &prsr->toks[prsr->head - 1];
}

/**
 * Get binding power of a token.
 */
bool e_getbp(e_token_type type, int* left, int* right);

#define E_GET_NODE e_ast_get_node

static inline e_ast_node*
e_ast_get_node(const e_ast* p, int idx)
{ return idx >= 0 ? &p->nodes[idx] : NULL; }

static inline ATTR_NODISCARD int
e_ast_make_node(e_ast* p)
{
  if (p->nnodes >= p->capacity) {
    u32         newcap   = MAX(p->capacity * 2, p->nnodes + 1);
    e_ast_node* newnodes = (e_ast_node*)realloc(p->nodes, newcap * sizeof(e_ast_node));
    if (newnodes == NULL) {
      perror("Allocation failed");
      return -1;
    }

    p->nodes    = newnodes;
    p->capacity = newcap;
  }

  u32 idx = p->nnodes;
  memset(&p->nodes[idx], 0, sizeof(e_ast_node));
  p->nnodes++;

  return (int)idx;
}

static inline bool
e_ast_is_limiter_exempt(e_ast_node_type t)
{
  return (
      t == E_AST_NODE_IF || t == E_AST_NODE_WHILE || t == E_AST_NODE_FOR || t == E_AST_NODE_FUNCTION_DEFINITION || t == E_AST_NODE_DEFER
      || t == E_AST_NODE_NOP);
}

int e_ast_parse(e_ast* p, int* root) ATTR_NODISCARD;
int e_ast_nud(e_ast* p, e_token* tk) ATTR_NODISCARD;
int e_ast_expr(e_ast* p, int rbp) ATTR_NODISCARD;
int e_ast_led(e_ast* p, e_token* tk, int leftidx, int rbp) ATTR_NODISCARD;

/**
 * Expect that peek() is pointing to a token
 * with type. Returns 0 on match.
 */
int e_ast_expect(e_ast* p, e_token_type type) ATTR_NODISCARD;

#endif // E_AST_H
