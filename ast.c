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

#include "ast.h"

#include "cerr.h"
#include "lex.h"
#include "stdafx.h"
#include "strint.h"

#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define prev(parser) e_parser_prev(parser)
#define peek(parser) e_parser_peek(parser)
#define next(parser) e_parser_next(parser)

// Initial pool size for all nodes
static const u32 init_node_capacity        = 128;
static const u32 init_braced_node_capacity = 32;

#if defined(__has_attribute) && __has_attribute(format)
#  define FORMAT_(...) __attribute__((format(__VA_ARGS__)))
#endif

static inline FORMAT_(printf, 4, 5) bool internal_asterror(const char* file, size_t line, e_filespan span, const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);

  fprintf(stderr, "[%s:%zu] [%s:%i:%i] ast error: ", file, line, span.file, span.line, span.col);
  vfprintf(stderr, msg, ap);

  va_end(ap);

  return true;
}

#define asterror(span, ...) internal_asterror(__FILE__, __LINE__, span, __VA_ARGS__)

static inline e_filespan
clonespan(e_ast* a, e_filespan s)
{
  e_filespan dup = {
    .file = (char*)e_str_intern(s.file, a->interner),
    .line = s.line,
    .col  = s.col,
  };
  // printf("[%s:%i:%i]\n", dup.file, dup.line, dup.col);
  return dup;
}

/* Returns -1 on invalid !!!!! */
static inline e_operator
conv_token_type_to_operator(e_token_type t)
{
  switch (t) {
    case E_TOKEN_TYPE_PLUS: return E_OPERATOR_ADD;
    case E_TOKEN_TYPE_MINUS: return E_OPERATOR_SUB;
    case E_TOKEN_TYPE_MULTIPLY: return E_OPERATOR_MUL;
    case E_TOKEN_TYPE_DIVIDE: return E_OPERATOR_DIV;
    case E_TOKEN_TYPE_MOD: return E_OPERATOR_MOD;
    case E_TOKEN_TYPE_EXPONENT: return E_OPERATOR_EXP;
    case E_TOKEN_TYPE_AND: return E_OPERATOR_AND;
    case E_TOKEN_TYPE_NOT: return E_OPERATOR_NOT;
    case E_TOKEN_TYPE_OR: return E_OPERATOR_OR;
    case E_TOKEN_TYPE_BAND: return E_OPERATOR_BAND;
    case E_TOKEN_TYPE_BOR: return E_OPERATOR_BOR;
    case E_TOKEN_TYPE_BNOT: return E_OPERATOR_BNOT;
    case E_TOKEN_TYPE_XOR: return E_OPERATOR_XOR;
    case E_TOKEN_TYPE_DOUBLEEQUAL: return E_OPERATOR_ISEQL;
    case E_TOKEN_TYPE_NOTEQUAL: return E_OPERATOR_ISNEQ;
    case E_TOKEN_TYPE_LT: return E_OPERATOR_LT;
    case E_TOKEN_TYPE_LTE: return E_OPERATOR_LTE;
    case E_TOKEN_TYPE_GT: return E_OPERATOR_GT;
    case E_TOKEN_TYPE_GTE: return E_OPERATOR_GTE;
    default: return -1;
  }
}

bool
e_getbp(e_token_type type, int* left, int* right)
{
  switch (type) {
    case E_TOKEN_TYPE_EQUAL:
      *left  = 10;
      *right = 9;
      break;

    case E_TOKEN_TYPE_AND:
      *left  = 30;
      *right = 29;
      break;

    case E_TOKEN_TYPE_OR:
      *left  = 20;
      *right = 19;
      break;
    case E_TOKEN_TYPE_NOT:
      *left  = 0;
      *right = 65;
      break;

    // bitwise
    case E_TOKEN_TYPE_BOR: // |
      *left  = 32;
      *right = 33;
      break;
    case E_TOKEN_TYPE_XOR: // ^
      *left  = 34;
      *right = 35;
      break;
    case E_TOKEN_TYPE_BAND: // &
      *left  = 36;
      *right = 37;
      break;

    // compares
    case E_TOKEN_TYPE_GT:
    case E_TOKEN_TYPE_LT:
    case E_TOKEN_TYPE_GTE:
    case E_TOKEN_TYPE_LTE:
    case E_TOKEN_TYPE_DOUBLEEQUAL:
    case E_TOKEN_TYPE_NOTEQUAL:
      *left  = 40;
      *right = 41;
      break;

    case E_TOKEN_TYPE_PLUS:
    case E_TOKEN_TYPE_MINUS:
      *left  = 50;
      *right = 51;
      break;
    case E_TOKEN_TYPE_MULTIPLY:
    case E_TOKEN_TYPE_DIVIDE:
    case E_TOKEN_TYPE_MOD:
      *left  = 60;
      *right = 61;
      break;

    case E_TOKEN_TYPE_BNOT: // ~
      *left  = 0;
      *right = 65;
      break;

    // member access
    case E_TOKEN_TYPE_DOT:
      *left  = 70;
      *right = 69;
      break;

    case E_TOKEN_TYPE_DOUBLE_COLON:
      *left  = 75;
      *right = 74;
      break;

    case E_TOKEN_TYPE_OPENPAREN:
      *left  = 90;
      *right = 100;
      break;
    case E_TOKEN_TYPE_CLOSEPAREN:
      *left  = 0;
      *right = 100;
      break;

    case E_TOKEN_TYPE_OPENBRACE:
    case E_TOKEN_TYPE_OPENBRACKET:
      *left  = 100;
      *right = 99;
      break;

    case E_TOKEN_TYPE_PLUSPLUS:
    case E_TOKEN_TYPE_MINUSMINUS:
      *left  = 80; // postfix only!!
      *right = 0;
      break;

    default: *left = *right = 0; return false;
  }
  return true;
}

/**
 * Single line if statements like if (x < 0) x = -x;
 * are handled by this because the expression parses consumes the semi colon itself,
 * Keeping the parser from getting polluted
 *
 * Returns 0 on succcess.
 */
static inline RETURNS_ERRCODE int
parse_braces(e_parser* p, int** outstmts, u32* outnstmts)
{
  u32  capacity = init_braced_node_capacity;
  u32  nstmts   = 0;
  int* stmts    = e_xalloc(capacity, sizeof(int));
  if (!stmts) goto err;

  while (peek(p) != nullptr && peek(p)->type != E_TOKEN_TYPE_CLOSEBRACE) {
    if (nstmts + 1 >= capacity) {
      u32  newcap   = MAX(capacity * 2, 1);
      int* newstmts = realloc(stmts, sizeof(int) * newcap);
      if (!newstmts) { goto err; }

      stmts    = newstmts;
      capacity = newcap;
    }

    e_filespan span = peek(p)->span;

    // We need the stmt to figure out if we need to chek for a limiter(;) at the end or not.
    int stmt = e_ast_expr(p, 0);
    if (stmt < 0) {
      asterror(span, "Failed to parse braced statement [braced statement list]\n");
      goto err;
    }

    stmts[nstmts++] = stmt;

    // isn't exempt and doesn't have a semicolon
    if (!e_ast_is_limiter_exempt(E_GET_NODE(p->ast, stmt)->type) && e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
      /* we need to be expecting a semi colon a character after the last */
      span.col++;
      asterror(
          prev(p)->span,
          "Expected semi colon ';', got '%s' [braced statement list]\n",
          e_token_type_to_string(prev(p)->type)); // use older span, newer one is token after where semicolon should be
      goto err;
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACE)) {
    asterror(prev(p)->span, "Expected closing brace '}' [braced statement list]\n");
    goto err;
  }

  if (outstmts) *outstmts = stmts;
  if (outnstmts) *outnstmts = nstmts;

  return 0;

err:
  if (outstmts) *outstmts = nullptr;
  if (outnstmts) *outnstmts = 0;
  for (u32 i = 0; i < nstmts; i++) e_ast_node_free(p->ast, stmts[i]);
  free(stmts);
  return -1;
}

/**
 * Parse a body of a statement.
 * This parses the tail end of the following statements:
 * if (true) { let x = 0; let y = 10; println(x+y); }
 *
 * while(true) i++; or for(;;i++); if you're cool.
 *
 * Returns 0 on succcess.
 */
static inline RETURNS_ERRCODE int
parse_body(e_parser* p, int** outstmts, u32* outnstmts)
{
  int  nstmts = 0;
  int* stmts  = nullptr;
  int  stmt   = 0;

  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_OPENBRACE) {
    next(p); // consume {
    return parse_braces(p, outstmts, outnstmts);
  }

  // Explicity handle zero expression lists.
  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_SEMICOLON) {
    next(p);
    if (outstmts) *outstmts = nullptr;
    if (outnstmts) *outnstmts = 0;
  }
  // Single line expressions ending with a semi colon.
  else {
    stmts = e_xalloc(1, sizeof(int));
    if (!stmts) goto err;

    e_filespan prev_span = peek(p)->span;

    stmt = e_ast_expr(p, 0);
    if (stmt < 0) {
      asterror(prev_span, "Failed to parse single line statement\n");
      goto err;
    }

    stmts[0] = stmt;
    nstmts   = 1;

    if (outstmts) *outstmts = stmts;
    if (outnstmts) *outnstmts = nstmts;

    e_ast_node_type type = E_GET_NODE(p->ast, stmt)->type;
    if (!e_ast_is_limiter_exempt(type) && e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
      asterror(
          prev(p)->span,
          "Expected semi colon after expression [recv=%s, expect=%s]\n",
          e_token_type_to_string(prev(p)->type),
          e_token_type_to_string(E_TOKEN_TYPE_SEMICOLON));
      goto err;
    }
  }

  return 0;

err:
  e_ast_node_free(p->ast, stmt);
  free(stmts);
  if (outstmts) *outstmts = NULL;
  if (outnstmts) *outnstmts = 0;
  return -1;
}

int
e_ast_expect(e_parser* p, e_token_type type)
{
  // printf("%s\n", e_token_type_to_string(peek(p)->type));
  if (!peek(p)) return 1;
  return next(p)->type == type ? 0 : 1;
}

int
e_ast_expr(e_parser* p, int rbp)
{
  const e_token* tk   = next(p);
  int            left = e_ast_nud(p, tk);
  if (left < 0) return left;

  E_GET_NODE(p->ast, left)->common.span = clonespan(p->ast, tk->span);

  while (e_parser_peek(p)) {
    const e_token* op = e_parser_peek(p);

    int left_bp  = 0;
    int right_bp = 0;
    e_getbp(op->type, &left_bp, &right_bp);

    if (left_bp <= rbp) break;

    left = e_ast_led(p, e_parser_next(p), left, right_bp);
    if (left < 0) break;
  }

  return left;
}

/**
 * Returns node on success, a negative integer on error.
 */
static RETURNS_ERRCODE int
parse_variable_decleration(e_parser* p, bool is_const, int node)
{
  if (node < 0) return node;

  u32         capacity = 16;
  u32         ndecls   = 0;
  int*        decls    = e_xalloc(capacity, sizeof(int));
  const char* name     = nullptr;
  if (!decls) goto err;

  while (peek(p)) {
    const e_token* name_tk = peek(p);
    if (!name_tk || name_tk->type != E_TOKEN_TYPE_IDENT) {
      asterror(name_tk->span, "Expected identifier after 'let', got '%s' [variable decleration]\n", e_token_type_to_string(name_tk->type));
      goto err;
    }

    next(p);

    int decl_node = e_ast_make_node(p->ast);
    if (decl_node < 0) goto err;

    name = e_str_intern(name_tk->val.s, p->ast->interner);
    if (!name) goto err;

    E_GET_NODE(p->ast, decl_node)->type         = E_AST_NODE_VARIABLE_DECL;
    E_GET_NODE(p->ast, decl_node)->let.span     = clonespan(p->ast, name_tk->span);
    E_GET_NODE(p->ast, decl_node)->let.name     = name;
    E_GET_NODE(p->ast, decl_node)->let.is_const = is_const;

    int initializer = -1;

    if (peek(p)->type == E_TOKEN_TYPE_EQUAL) {
      next(p); // consume =
      initializer = e_ast_expr(p, 0);
      if (initializer < 0) {
        asterror(peek(p)->span, "Error parsing initializer [variable decleration]\n");
        e_ast_node_free(p->ast, decl_node);
        goto err;
      }
    }

    E_GET_NODE(p->ast, decl_node)->let.initializer = initializer;

    if (ndecls >= capacity) {
      u32  newcap = capacity * 2;
      int* tmp    = realloc(decls, sizeof(int) * newcap);
      if (!tmp) {
        e_ast_node_free(p->ast, decl_node);
        goto err;
      }
      decls    = tmp;
      capacity = newcap;
    }

    decls[ndecls++] = decl_node;

    if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_COMMA) break;

    next(p); // consume ,
  }

  // optimizations ooo
  if (ndecls == 1) {
    // ooo optimizations ruined span collection yay
    e_ast_node* src = E_GET_NODE(p->ast, decls[0]);
    e_ast_node* dst = E_GET_NODE(p->ast, node);

    dst->type            = src->type;
    dst->let.name        = src->let.name;
    dst->let.span        = src->let.span;
    dst->let.initializer = src->let.initializer;
    dst->let.is_const    = src->let.is_const;

    // prevent double free
    src->let.name      = NULL;
    src->let.span.file = NULL;
    src->type          = E_AST_NODE_NOP;

    free(decls);

    return node;
  }

  E_GET_NODE(p->ast, node)->type         = E_AST_NODE_STATEMENT_LIST;
  E_GET_NODE(p->ast, node)->stmts.stmts  = decls;
  E_GET_NODE(p->ast, node)->stmts.nstmts = ndecls;

  return node;

err:
  for (u32 i = 0; i < ndecls; i++) e_ast_node_free(p->ast, decls[i]);
  free(decls);
  return -1;
}

static RETURNS_ERRCODE int
parse_if(e_parser* p, int node)
{
  if (node < 0) return node;

  u32        cap_else_ifs = 4;
  u32        num_else_ifs = 0;
  e_if_stmt* else_ifs     = e_xalloc(cap_else_ifs, sizeof(e_if_stmt));

  int* body   = nullptr;
  u32  nstmts = 0;

  int* else_body   = nullptr;
  u32  nelse_stmts = 0;

  int  condition         = -1;
  int  else_if_condition = -1;
  int* else_if_body      = nullptr;
  u32  else_if_nstmts    = 0;

  if (else_ifs == nullptr) goto err;

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected '(' after 'if', got '%s' [if statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  const e_token* tk = peek(p);

  condition = e_ast_expr(p, 0);
  if (condition < 0) {
    asterror(tk->span, "Failed to parse condition [if statement]\n");
    goto err;
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')' after condition, but got '%s' [if statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // Take the span before so it starts where the body would start.
  e_filespan body_span = peek(p)->span;

  if (parse_body(p, &body, &nstmts)) {
    asterror(body_span, "Failed to parse body [if statement]\n");
    goto err;
  }

  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_ELSE) {
    while (true) {
      if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_ELSE) break; // no more else chains

      next(p); // consume else

      if (peek(p) && peek(p)->type == E_TOKEN_TYPE_IF) // else if
      {
        next(p);

        if (num_else_ifs >= cap_else_ifs) {
          u32        newcap           = MAX(cap_else_ifs * 2, 1);
          e_if_stmt* new_else_ifs     = realloc(else_ifs, sizeof(e_if_stmt) * newcap);
          u32        new_cap_else_ifs = newcap;
          if (!new_else_ifs) goto err;

          else_ifs     = new_else_ifs;
          cap_else_ifs = new_cap_else_ifs;
        }

        if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
          asterror(prev(p)->span, "Expected '(' after 'else if', but got '%s' [if statement :: else if]\n", e_token_type_to_string(prev(p)->type));
          goto errloop;
        }

        else_if_condition = e_ast_expr(p, 0);

        if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
          asterror(prev(p)->span, "Expected ')' after condition, but got '%s' [if statement :: else if]\n", e_token_type_to_string(prev(p)->type));
          goto errloop;
        }

        body_span = peek(p)->span;
        if (parse_body(p, &else_if_body, &else_if_nstmts)) {
          asterror(body_span, "Failed to parse body [if statement :: else if]\n");
          goto errloop;
        }

        else_ifs[num_else_ifs++] = (e_if_stmt){
          .condition = else_if_condition,
          .body      = else_if_body,
          .nstmts    = else_if_nstmts,
        };
        continue; //

      errloop:
        free(else_if_body);
        e_ast_node_free(p->ast, else_if_condition);
        goto err;
      }

      // else {}

      e_filespan else_span = peek(p)->span;

      if (parse_body(p, &else_body, &nelse_stmts)) {
        asterror(else_span, "Failed to parse body [if statement :: else]\n");
        goto err;
      }

      break; // no more deals
    }
  }

  // Write the collected infomation to the struct.
  E_GET_NODE(p->ast, node)->type                = E_AST_NODE_IF;
  E_GET_NODE(p->ast, node)->if_stmt.body        = body;
  E_GET_NODE(p->ast, node)->if_stmt.nstmts      = nstmts;
  E_GET_NODE(p->ast, node)->if_stmt.condition   = condition;
  E_GET_NODE(p->ast, node)->if_stmt.else_body   = else_body;
  E_GET_NODE(p->ast, node)->if_stmt.nelse_stmts = nelse_stmts;
  E_GET_NODE(p->ast, node)->if_stmt.else_ifs    = else_ifs;
  E_GET_NODE(p->ast, node)->if_stmt.nelse_ifs   = num_else_ifs;

  return node;

err:
  e_ast_node_free(p->ast, condition);
  for (u32 i = 0; i < nstmts; i++) e_ast_node_free(p->ast, body[i]);
  for (u32 i = 0; i < nelse_stmts; i++) e_ast_node_free(p->ast, else_body[i]);
  for (u32 i = 0; i < num_else_ifs; i++) {
    for (u32 j = 0; j < else_ifs[i].nstmts; j++) { e_ast_node_free(p->ast, else_ifs[i].body[j]); }
    free(else_ifs[i].body);
    e_ast_node_free(p->ast, else_ifs[i].condition);
  }
  free(else_ifs);
  free(else_body);
  free(body);
  return -1;
}

static RETURNS_ERRCODE int
parse_while(e_parser* p, int node)
{
  if (node < 0) return node;

  int  cnd    = -1;
  int* stmts  = nullptr;
  u32  nstmts = 0;

  // if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
  //   asterror(prev(p)->span, "Expected ( after 'while'\n");
  //   goto err;
  // }
  if (peek(p)->type == E_TOKEN_TYPE_OPENPAREN) next(p);

  e_filespan prev_span = peek(p)->span;

  cnd = e_ast_expr(p, 0);
  if (cnd < 0) {
    asterror(prev_span, "Expected expression\n");
    goto err;
  }

  if (peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) next(p);

  // if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
  //   asterror(prev(p)->span, "Expected ) after 'while'\n");
  //   goto err;
  // }

  prev_span = peek(p)->span;
  if (parse_body(p, &stmts, &nstmts)) {
    asterror(prev_span, "Failed to parse while statement body\n");
    goto err;
  }

  E_GET_NODE(p->ast, node)->type                 = E_AST_NODE_WHILE;
  E_GET_NODE(p->ast, node)->while_stmt.condition = cnd;
  E_GET_NODE(p->ast, node)->while_stmt.stmts     = stmts;
  E_GET_NODE(p->ast, node)->while_stmt.nstmts    = nstmts;

  return node;

err:
  e_ast_node_free(p->ast, cnd);
  for (u32 i = 0; i < nstmts; i++) e_ast_node_free(p->ast, stmts[i]);
  free(stmts);
  return -1;
}

static RETURNS_ERRCODE int
parse_for(e_parser* p, int node)
{
  int init = -1;
  int cond = -1;

  int* stmts  = NULL;
  u32  nstmts = 0;

  u32  niterators = 0;
  u32  capacity   = 8;
  int* iterators  = e_xalloc(capacity, sizeof(int));
  if (!iterators) goto err;

  // (
  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected ( after 'for', got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // let i = 0
  if (peek(p) && peek(p)->type != E_TOKEN_TYPE_SEMICOLON) {
    e_filespan prev_span = peek(p)->span;

    init = e_ast_expr(p, 0);
    if (init < 0) {
      asterror(prev_span, "Expected expression (initializers), got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
    asterror(prev(p)->span, "Expected ';' after initializer list, got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // i < 10
  if (peek(p) && peek(p)->type != E_TOKEN_TYPE_SEMICOLON) {
    e_filespan prev_span = peek(p)->span;
    cond                 = e_ast_expr(p, 0);
    if (cond < 0) {
      asterror(prev_span, "Expected expression (condition), got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
    asterror(prev(p)->span, "Expected ';' after condition, got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  // i++,y++

  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_SEMICOLON) {
    next(p);
  } else {
    while (true) {
      if (niterators >= capacity) {
        u32  new_capacity  = capacity * 2;
        int* new_iterators = realloc(iterators, new_capacity * sizeof(int));

        if (!new_iterators) { goto err; }

        iterators = new_iterators;
        capacity  = new_capacity;
      }

      e_filespan prev_span = peek(p)->span;
      int        iter      = e_ast_expr(p, 0);
      if (iter < 0) {
        asterror(prev_span, "Expected expression (iterators), got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
        goto err;
      }

      iterators[niterators++] = iter;

      if (peek(p) && peek(p)->type != E_TOKEN_TYPE_COMMA) break;
      next(p); // consume ,
    }
  }

  // )
  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')' after header, got '%s' [for statement]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  e_filespan prev_span = peek(p)->span;

  if (parse_body(p, &stmts, &nstmts)) {
    asterror(prev_span, "Failed to parse body [for statement]\n");
    goto err;
  }

  E_GET_NODE(p->ast, node)->type                  = E_AST_NODE_FOR;
  E_GET_NODE(p->ast, node)->for_stmt.initializers = init;
  E_GET_NODE(p->ast, node)->for_stmt.condition    = cond;
  E_GET_NODE(p->ast, node)->for_stmt.niterators   = niterators;
  E_GET_NODE(p->ast, node)->for_stmt.iterators    = iterators;
  E_GET_NODE(p->ast, node)->for_stmt.stmts        = stmts;
  E_GET_NODE(p->ast, node)->for_stmt.nstmts       = nstmts;

  return node;

err:
  if (init >= 0) e_ast_node_free(p->ast, init);
  if (cond >= 0) e_ast_node_free(p->ast, cond);

  for (u32 i = 0; i < niterators; i++) e_ast_node_free(p->ast, iterators[i]);
  for (u32 i = 0; i < nstmts; i++) e_ast_node_free(p->ast, stmts[i]);

  free(iterators);
  free(stmts);
  return -1;
}

static RETURNS_ERRCODE int
parse_function(e_parser* p, bool external, int node)
{
  if (node < 0) return node;

  u32            names_capacity       = 0;
  u32            arg_names_size       = 0;
  const char**   arg_names            = nullptr;
  const char*    intern_function_name = nullptr;
  const char*    arg_name             = nullptr;
  const e_token* name_tk              = nullptr;
  int*           stmts                = nullptr;
  u32            nstmts               = 0;

  name_tk = peek(p);
  if (e_ast_expect(p, E_TOKEN_TYPE_IDENT)) {
    asterror(prev(p)->span, "Expected function name after 'fn'/'extern', got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected (, got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  intern_function_name = e_str_intern(name_tk->val.ident, p->ast->interner);

  names_capacity = 8; // NON ZERO
  arg_names_size = 0;
  arg_names      = (const char**)e_xalloc(names_capacity, sizeof(char*));

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEPAREN) {
    const e_token* tk = peek(p);
    if (tk->type != E_TOKEN_TYPE_IDENT) {
      asterror(peek(p)->span, "Expected parameter name, got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
    next(p);

    if (arg_names_size >= names_capacity) {
      u32          new_capacity = names_capacity * 2;
      const char** new_names    = (const char**)realloc((void*)arg_names, sizeof(char*) * new_capacity);

      if (!new_names) {
        asterror(peek(p)->span, "Allocation error! [function header]\n");
        goto err;
      }

      arg_names      = new_names;
      names_capacity = new_capacity;
    }

    arg_name = e_str_intern(tk->val.ident, p->ast->interner);
    if (!arg_name) {
      asterror(peek(p)->span, "Alloc error [function argument name]\n");
      goto err;
    }

    arg_names[arg_names_size] = arg_name;
    arg_names_size++;

    if (peek(p) && peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) { break; }

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',', got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')', got '%s' [function header]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  e_filespan parsing_span = peek(p)->span;

  if (external) {
    if (e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
      asterror(prev(p)->span, "Expected ')', got '%s' [external function decleration]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    if (parse_body(p, &stmts, &nstmts)) {
      asterror(parsing_span, "Failed to parse function body [function definition]\n");
      goto err;
    }
  }

  E_GET_NODE(p->ast, node)->type        = E_AST_NODE_FUNCTION_DEFINITION;
  E_GET_NODE(p->ast, node)->func.name   = intern_function_name;
  E_GET_NODE(p->ast, node)->func.args   = arg_names;
  E_GET_NODE(p->ast, node)->func.nargs  = arg_names_size;
  E_GET_NODE(p->ast, node)->func.stmts  = stmts;
  E_GET_NODE(p->ast, node)->func.nstmts = nstmts;

  return node;

err:
  free((void*)arg_names);
  for (u32 i = 0; i < nstmts; i++) { e_ast_node_free(p->ast, stmts[i]); }
  free(stmts);
  return -1;
}

static RETURNS_ERRCODE int
parse_function_call(e_parser* p, const e_token* tk, int node)
{
  u32  capacity = 16;
  u32  nargs    = 0;
  int* args     = e_xalloc(capacity, sizeof(int)); // argument nodes

  const char* func_name = e_str_intern(tk->val.ident, p->ast->interner);

  if (!func_name || !args) goto err;

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENPAREN)) // (
  {
    asterror(prev(p)->span, "Expected '(' after function name, got '%s'\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  E_GET_NODE(p->ast, node)->type               = E_AST_NODE_CALL;
  E_GET_NODE(p->ast, node)->call.function_name = func_name;
  // printf("AST: Detected function call: %s\n", tk->val.ident);

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEPAREN) {
    if (nargs >= capacity) {
      u32  newcap       = MAX(capacity * 2, 1);
      int* new_args     = realloc(args, newcap * sizeof(int));
      u32  new_capacity = newcap;

      if (new_args == nullptr) goto err;

      args     = new_args;
      capacity = new_capacity;
    }

    int expr = e_ast_expr(p, 0);
    if (expr < 0) goto err;

    /**
     * 0 binding power is to parse everything
     * until the closing parenthesis.
     */
    args[nargs++] = expr;

    if (peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) { break; }

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ')', got '%s'\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  }
  // consume )
  next(p);

  E_GET_NODE(p->ast, node)->call.args  = args;
  E_GET_NODE(p->ast, node)->call.nargs = nargs;

  return node;

err:
  for (u32 i = 0; i < nargs; i++) e_ast_node_free(p->ast, args[i]);
  free(args);
  return -1;
}

/**
 * On success, returns the node id.
 */
static inline RETURNS_ERRCODE int
make_literal_node(e_ast* p, int node, const e_token* tk)
{
  if (node < 0) return node;

  e_ast_node* nodep = e_ast_get_node(p, node);
  if (tk->type == E_TOKEN_TYPE_INT) {
    nodep->type = E_AST_NODE_INT;
    nodep->i.i  = tk->val.i;
  } else if (tk->type == E_TOKEN_TYPE_FLOAT) {
    nodep->type = E_AST_NODE_FLOAT;
    nodep->f.f  = tk->val.f;
  } else if (tk->type == E_TOKEN_TYPE_BOOL) {
    nodep->type = E_AST_NODE_BOOL;
    nodep->b.b  = tk->val.b;
  } else if (tk->type == E_TOKEN_TYPE_CHAR) {
    nodep->type = E_AST_NODE_CHAR;
    nodep->c.c  = tk->val.c;
  } else if (tk->type == E_TOKEN_TYPE_STRING) {
    nodep->type = E_AST_NODE_STRING;
    nodep->s.s  = e_strdup(tk->val.s);
    if (!nodep->s.s) return -1;
  }
  return node;
}

static RETURNS_ERRCODE int
parse_list(e_parser* p, int node)
{
  u32  capelems = 16;
  u32  nelems   = 0;
  int* elems    = e_xalloc(capelems, sizeof(int));

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEBRACKET) {
    int elem = e_ast_expr(p, 0);

    if (nelems >= capelems) {
      u32  new_cap   = MAX(capelems * 2, 1);
      int* new_elems = realloc(elems, new_cap * sizeof(int));
      if (!new_elems) { goto err; }

      elems    = new_elems;
      capelems = new_cap;
    }

    elems[nelems++] = elem;

    if (peek(p) && peek(p)->type == E_TOKEN_TYPE_CLOSEBRACKET) { break; }

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ']', got '%s' [list literal]\n", e_token_type_to_string(prev(p)->type));
      goto err;
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACKET)) {
    asterror(prev(p)->span, "Expected ']', got '%s' [list literal]\n", e_token_type_to_string(prev(p)->type));
    goto err;
  }

  E_GET_NODE(p->ast, node)->type        = E_AST_NODE_LIST;
  E_GET_NODE(p->ast, node)->list.elems  = elems;
  E_GET_NODE(p->ast, node)->list.nelems = nelems;

  return node;

err:
  for (u32 i = 0; i < nelems; i++) { e_ast_node_free(p->ast, elems[i]); }
  free(elems);
  return -1;
}

static RETURNS_ERRCODE int
parse_map(e_parser* p, int node)
{
  // if (e_ast_expect(p, E_TOKEN_TYPE_OPEN_BRACE)) {
  //   asterror(prev(p)->span, "Expected '{' after '#' [map literal]\n");
  //   return -1;
  // }
  int e = 0;

  u32 capacity = 8;
  u32 npairs   = 0;

  int* keys   = e_xalloc(capacity, sizeof(int));
  int* values = e_xalloc(capacity, sizeof(int));

  if (!keys || !values) goto ERR;

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEBRACE) {
    // key
    int key = e_ast_expr(p, 0);
    if (key < 0) goto ERR;

    if (e_ast_expect(p, E_TOKEN_TYPE_COLON)) {
      asterror(prev(p)->span, "Expected ':' [map literal]\n");
      goto ERR;
    }

    // value
    int value = e_ast_expr(p, 0);
    if (value < 0) goto ERR;

    if (npairs >= capacity) {
      u32 newcap = capacity * 2;

      int* new_keys = realloc(keys, sizeof(int) * newcap);
      if (!new_keys) { goto ERR; }

      int* new_values = realloc(values, sizeof(int) * newcap);
      if (!new_values) { goto ERR; }

      keys     = new_keys;
      values   = new_values;
      capacity = newcap;
    }

    keys[npairs]   = key;
    values[npairs] = value;
    npairs++;

    if (peek(p)->type == E_TOKEN_TYPE_CLOSEBRACE) break;

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or '}' [map literal]\n");
      goto ERR;
    }
  }

  if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACE)) {
    asterror(prev(p)->span, "Expected '}' [map literal]\n");
    goto ERR;
  }

  E_GET_NODE(p->ast, node)->type       = E_AST_NODE_MAP;
  E_GET_NODE(p->ast, node)->map.keys   = keys;
  E_GET_NODE(p->ast, node)->map.values = values;
  E_GET_NODE(p->ast, node)->map.npairs = npairs;

  return node;

ERR:
  for (u32 i = 0; i < npairs; i++) {
    e_ast_node_free(p->ast, keys[i]);
    e_ast_node_free(p->ast, values[i]);
  }
  free(keys);
  free(values);
  return e ? e : -1;
}

static RETURNS_ERRCODE int
parse_namespace_decleration(e_parser* p, int node)
{
  int  e      = 0;
  int* stmts  = NULL;
  u32  nstmts = 0;

  if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected namespace name, got '%s' [namespace decleration]\n", e_token_type_to_string(prev(p)->type));
    goto ERR;
  }

  const e_token* name_tk = next(p);

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENBRACE)) {
    asterror(prev(p)->span, "Expected '{' after namespace name, got '%s' [namespace decleration]\n", e_token_type_to_string(prev(p)->type));
    goto ERR;
  }

  e_filespan store = peek(p)->span;

  e = parse_braces(p, &stmts, &nstmts);
  if (e) {
    asterror(store, "Failed to parse namespace decleration [namespace decleration :: collection pass]\n");
    goto ERR;
  }

  for (u32 i = 0; i < nstmts; i++) {
    e_ast_node_type type = E_GET_NODE(p->ast, stmts[i])->common.type;
    e_filespan      span = E_GET_NODE(p->ast, stmts[i])->common.span;

    if (type == E_AST_NODE_STATEMENT_LIST) {
      u32  list_nstmts = E_GET_NODE(p->ast, stmts[i])->stmts.nstmts;
      int* list_stmts  = E_GET_NODE(p->ast, stmts[i])->stmts.stmts;
      for (u32 j = 0; j < list_nstmts; j++) {
        type = E_GET_NODE(p->ast, list_stmts[j])->common.type;
        if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_STRUCT_DECL && type != E_AST_NODE_NAMESPACE_DECL
            && type != E_AST_NODE_VARIABLE_DECL && type != E_AST_NODE_ASSERT) {
          asterror(span, "Expected only function definitions, variable declerations, namespace declerations or assertions in namespace scope\n");

          goto ERR;
        }
      }
    } else if (
        type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_STRUCT_DECL && type != E_AST_NODE_NAMESPACE_DECL
        && type != E_AST_NODE_VARIABLE_DECL && type != E_AST_NODE_ASSERT) {
      asterror(span, "Expected only function definitions, variable declerations, namespace declerations or assertions in namespace scope\n");

      goto ERR;
    }
  }

  const char* ident = e_str_intern(name_tk->val.ident, p->ast->interner);
  if (!ident) { goto ERR; }

  E_GET_NODE(p->ast, node)->type                  = E_AST_NODE_NAMESPACE_DECL;
  E_GET_NODE(p->ast, node)->namespace_decl.name   = ident;
  E_GET_NODE(p->ast, node)->namespace_decl.stmts  = stmts;
  E_GET_NODE(p->ast, node)->namespace_decl.nstmts = nstmts;

  return node;

ERR:
  for (u32 i = 0; i < nstmts; i++) e_ast_node_free(p->ast, stmts[i]);

  free(stmts);
  return e ? e : -1;
}

static RETURNS_ERRCODE int
parse_namespace_call(e_parser* p, const char* name, int node)
{
  u32  capacity = 16;
  u32  nargs    = 0;
  int* args     = e_xalloc(capacity, sizeof(int));
  if (!args) { goto ERR; }

  next(p); // '('

  while (peek(p) && peek(p)->type != E_TOKEN_TYPE_CLOSEPAREN) {
    if (nargs >= capacity) {
      u32  newcap   = MAX(capacity * 2, 1);
      int* new_args = realloc(args, sizeof(int) * newcap);
      if (!new_args) { goto ERR; }
      args     = new_args;
      capacity = newcap;
    }

    args[nargs++] = e_ast_expr(p, 0);

    if (peek(p)->type == E_TOKEN_TYPE_CLOSEPAREN) break;

    if (e_ast_expect(p, E_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ')' [qualified function call]\n");
      goto ERR;
    }
  }

  next(p); // ')'

  E_GET_NODE(p->ast, node)->type               = E_AST_NODE_CALL;
  E_GET_NODE(p->ast, node)->call.function_name = name;
  E_GET_NODE(p->ast, node)->call.args          = args;
  E_GET_NODE(p->ast, node)->call.nargs         = nargs;

  return node;

ERR:
  for (u32 i = 0; i < nargs; i++) e_ast_node_free(p->ast, args[i]);
  free(args);
  return -1;
}

static RETURNS_ERRCODE int
parse_namespace_access(e_parser* p, int leftidx, int node)
{
  if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected identifier after :: [namespace access]\n");

    return -1;
  }

  const e_token* ident = next(p);

  if (E_GET_NODE(p->ast, leftidx)->type != E_AST_NODE_VARIABLE) {
    asterror(peek(p)->span, "LHS of :: must be an identifier [namespace access]\n");
    return -1;
  }

  const char* left_name = E_GET_NODE(p->ast, leftidx)->ident.ident;

  size_t len      = strlen(left_name) + strlen(ident->val.ident) + 3; // +2 for ::, 1 for \0
  char*  fullname = e_xalloc(1, len);
  if (!fullname) { return -1; }

  snprintf(fullname, len, "%s::%s", left_name, ident->val.ident);

  const char* interned = e_str_intern(fullname, p->ast->interner);

  free(fullname);

  // If function call
  if (peek(p) && peek(p)->type == E_TOKEN_TYPE_OPENPAREN) {
    if (parse_namespace_call(p, interned, node) < 0) { return -1; }
    return node;
  }

  // just a namespaced variable.
  E_GET_NODE(p->ast, node)->type        = E_AST_NODE_VARIABLE;
  E_GET_NODE(p->ast, node)->ident.ident = interned;

  return node;
}

static RETURNS_ERRCODE int
parse_struct_decleration(e_parser* p, int node)
{
  int e = 0;

  int* stmts  = NULL;
  u32  nstmts = 0;

  if (!peek(p) || peek(p)->type != E_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected structure name, got '%s' [structure decleration]\n", e_token_type_to_string(peek(p)->type));
    goto ERR;
  }

  const e_token* name_tk = next(p);

  if (e_ast_expect(p, E_TOKEN_TYPE_OPENBRACE)) {
    asterror(prev(p)->span, "Expected '{' after structure name, got '%s' [structure decleration]\n", e_token_type_to_string(prev(p)->type));
    goto ERR;
  }

  e_filespan store = peek(p)->span;

  e = parse_braces(p, &stmts, &nstmts);
  if (e) {
    asterror(store, "Failed to parse structure decleration body [structure decleration :: collection pass]\n");
    goto ERR;
  }

  for (u32 i = 0; i < nstmts; i++) {
    e_ast_node_type type = E_GET_NODE(p->ast, stmts[i])->common.type;
    e_filespan      span = E_GET_NODE(p->ast, stmts[i])->common.span;

    const char* s = "Expected only function definitions or variable declerations in structure decleration scope\n";

    if (type == E_AST_NODE_STATEMENT_LIST) {
      u32  list_nstmts = E_GET_NODE(p->ast, stmts[i])->stmts.nstmts;
      int* list_stmts  = E_GET_NODE(p->ast, stmts[i])->stmts.stmts;
      for (u32 j = 0; j < list_nstmts; j++) {
        type = E_GET_NODE(p->ast, list_stmts[j])->common.type;
        if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_VARIABLE_DECL) {
          asterror(span, "%s", s);
          goto ERR;
        }
      }
    } else if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_VARIABLE_DECL) {
      asterror(span, "%s", s);
      goto ERR;
    }
  }

  const char* interned = e_str_intern(name_tk->val.ident, p->ast->interner);
  if (!interned) goto ERR;

  E_GET_NODE(p->ast, node)->type               = E_AST_NODE_STRUCT_DECL;
  E_GET_NODE(p->ast, node)->struct_decl.name   = interned;
  E_GET_NODE(p->ast, node)->struct_decl.stmts  = stmts;
  E_GET_NODE(p->ast, node)->struct_decl.nstmts = nstmts;

  return node;

ERR:
  for (u32 i = 0; i < nstmts; i++) e_ast_node_free(p->ast, stmts[i]);
  free(stmts);
  return e ? e : -1;
}

int
e_ast_nud(e_parser* p, const e_token* tk)
{
  if (!tk || !p) return -1;

  /**
   * !!!
   * Node is owned by this function, and led!!!
   * If any error occurs, only these function are allowed
   * to free the node.
   */
  int node = e_ast_make_node(p->ast);
  if (node < 0) return node; // ERROR!

  /* zero out the node for safety. */
  memset(E_GET_NODE(p->ast, node), 0, sizeof(e_ast_node));

  E_GET_NODE(p->ast, node)->common.span = clonespan(p->ast, tk->span);

  // if (tk->type == E_TOKEN_TYPE_EOF) return -1;

  switch (tk->type) {
    case E_TOKEN_TYPE_INT:
    case E_TOKEN_TYPE_CHAR:
    case E_TOKEN_TYPE_BOOL:
    case E_TOKEN_TYPE_FLOAT:
    case E_TOKEN_TYPE_STRING: {
      int e = make_literal_node(p->ast, node, tk);
      if (e < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_DEFER: {
      u32  nstmts = 0;
      int* stmts  = NULL;
      if (parse_body(p, &stmts, &nstmts) < 0) {
        asterror(tk->span, "Failed to parse statements [defer]\n");
        e_ast_node_free(p->ast, node);
        return -1;
      }

      E_GET_NODE(p->ast, node)->type         = E_AST_NODE_DEFER;
      E_GET_NODE(p->ast, node)->defer.nstmts = nstmts;
      E_GET_NODE(p->ast, node)->defer.stmts  = stmts;
      return node;
    }

    case E_TOKEN_TYPE_ASSERT: {
      int expr = e_ast_expr(p, 0);
      if (expr < 0) return -1;

      E_GET_NODE(p->ast, node)->type                     = E_AST_NODE_ASSERT;
      E_GET_NODE(p->ast, node)->assertion.stmt           = expr;
      E_GET_NODE(p->ast, node)->assertion.assertion_line = e_strdup(tk->val.assertion_line);
      return node;
    }

    /* List declerator ([elem1,elem2,]) */
    case E_TOKEN_TYPE_OPENBRACKET: {
      if (parse_list(p, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    /* Map declerator */
    case E_TOKEN_TYPE_HASH_OPENBRACE: {
      if (parse_map(p, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_IDENT: {
      // Function call
      if (peek(p)->type == E_TOKEN_TYPE_OPENPAREN) {
        if (parse_function_call(p, tk, node) < 0) { goto err1; }
        return node;
      }

      const char* interned = e_str_intern(tk->val.ident, p->ast->interner);
      if (!interned) goto err1;

      // Just a variable!
      E_GET_NODE(p->ast, node)->type        = E_AST_NODE_VARIABLE;
      E_GET_NODE(p->ast, node)->ident.ident = interned;

      return node;

    err1:
      e_ast_node_free(p->ast, node);
      return -1;
    }

    case E_TOKEN_TYPE_OPENPAREN: {
      int old_node = node;

      node = e_ast_expr(p, 0);
      if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEPAREN)) {
        e_filespan opening_paren_span = E_GET_NODE(p->ast, old_node)->common.span;
        // asterror(opening_paren_span, "Expected ')' here [%s:%i:%i] for this '('\n", peek(p)->span.file, peek(p)->span.line, peek(p)->span.col);
        asterror(
            prev(p)->span,
            "Expected ')' for this '(' [%s:%i:%i], got '%s'\n",
            opening_paren_span.file,
            opening_paren_span.line,
            opening_paren_span.col,
            e_token_type_to_string(prev(p)->type));

        e_ast_node_free(p->ast, node);
        return -1;
      }

      // Replaced it with expression
      e_ast_node_free(p->ast, old_node);
      return node;
    }

    /* { stmts;stmts;stmts; } */
    case E_TOKEN_TYPE_OPENBRACE: {
      int* stmts  = nullptr;
      u32  nstmts = 0;
      if (parse_braces(p, &stmts, &nstmts)) {
        e_ast_node_free(p->ast, node);
        asterror(tk->span, "Failed to parse braced statement\n");
        return -1;
      }

      E_GET_NODE(p->ast, node)->type         = E_AST_NODE_STATEMENT_LIST;
      E_GET_NODE(p->ast, node)->stmts.stmts  = stmts;
      E_GET_NODE(p->ast, node)->stmts.nstmts = nstmts;

      return node;
    }

    case E_TOKEN_TYPE_BREAK: {
      E_GET_NODE(p->ast, node)->type = E_AST_NODE_BREAK;
      return node;
    }

    case E_TOKEN_TYPE_CONTINUE: {
      E_GET_NODE(p->ast, node)->type = E_AST_NODE_CONTINUE;
      return node;
    }

    case E_TOKEN_TYPE_RETURN: {
      E_GET_NODE(p->ast, node)->type = E_AST_NODE_RETURN;
      if (peek(p) && peek(p)->type != E_TOKEN_TYPE_SEMICOLON) {
        // Expect an expression
        int expr = e_ast_expr(p, 0);
        if (expr < 0) {
          e_ast_node_free(p->ast, node);
          return -1; // Parser will eat semicolon
        }

        E_GET_NODE(p->ast, node)->ret.expr_id          = expr;
        E_GET_NODE(p->ast, node)->ret.has_return_value = true;
      } else {
        E_GET_NODE(p->ast, node)->ret.expr_id          = -1;
        E_GET_NODE(p->ast, node)->ret.has_return_value = false;
      }

      return node;
    }

    case E_TOKEN_TYPE_CONST:
    case E_TOKEN_TYPE_LET: {
      // const x = 69;
      // const let gay = true;
      bool is_const = (tk->type == E_TOKEN_TYPE_CONST);

      // skip the 2nd let tok if it exists
      if (tk->type == E_TOKEN_TYPE_CONST && peek(p) && peek(p)->type == E_TOKEN_TYPE_LET) { next(p); }

      // skip the 2nd const tok if it exists
      if (tk->type == E_TOKEN_TYPE_LET && peek(p) && peek(p)->type == E_TOKEN_TYPE_CONST) {
        is_const = true;
        next(p);
      }

      if (parse_variable_decleration(p, is_const, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_STRUCT: {
      if (parse_struct_decleration(p, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_IF: {
      if (parse_if(p, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_WHILE: {
      if (parse_while(p, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_FOR: {
      if (parse_for(p, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_FN: {
      if (parse_function(p, false, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_EXTERN: {
      if (parse_function(p, true, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_NAMESPACE: {
      if (parse_namespace_decleration(p, node) < 0) {
        e_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case E_TOKEN_TYPE_SEMICOLON: {
      E_GET_NODE(p->ast, node)->type = E_AST_NODE_NOP;
      return node;
    }

    case E_TOKEN_TYPE_MINUS:
    case E_TOKEN_TYPE_NOT:
    case E_TOKEN_TYPE_BNOT: {
      int left_bp  = 0;
      int right_bp = 0;
      if (!e_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type));

        e_ast_node_free(p->ast, node);
        return -1;
      }

      e_operator op = conv_token_type_to_operator(tk->type);
      if (op < 0) {
        asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type));

        e_ast_node_free(p->ast, node);
        return -1;
      }

      int right = e_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(tk->span, "Failed to evaluate RHS of unary operator\n");

        e_ast_node_free(p->ast, node);
        return -1;
      }

      E_GET_NODE(p->ast, node)->type                = E_AST_NODE_UNARYOP;
      E_GET_NODE(p->ast, node)->unaryop.op          = op;
      E_GET_NODE(p->ast, node)->unaryop.is_compound = tk->val.op.is_compound;
      E_GET_NODE(p->ast, node)->unaryop.right       = right;
      return node;
    }

    default: asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type)); return -1;
  }

  /* The handlers consume all the tokens that nud() is supposed to consume. */
  return -1;
}

int
e_ast_led(e_parser* p, const e_token* tk, int leftidx, int rbp)
{
  if (!tk || !p || leftidx < 0) return -1;

  int node = e_ast_make_node(p->ast);
  if (node < 0) return node; // ERROR!

  E_GET_NODE(p->ast, node)->common.span = clonespan(p->ast, tk->span);

  switch (tk->type) {
    case E_TOKEN_TYPE_EQUAL: {
      // printf("ASSIGN YAYY!!\n");

      // Assigning to a member of a (list/struct)
      if (e_ast_get_node(p->ast, leftidx)->type == E_AST_NODE_INDEX) {
        int rightidx = e_ast_expr(p, rbp);
        if (rightidx < 0) {
          asterror(peek(p)->span, "Failed to parse RHS [index assignment]\n");

          return -1;
        }

        E_GET_NODE(p->ast, node)->type               = E_AST_NODE_INDEX_ASSIGN;
        E_GET_NODE(p->ast, node)->index_assign.base  = E_GET_NODE(p->ast, leftidx)->index.base;
        E_GET_NODE(p->ast, node)->index_assign.index = E_GET_NODE(p->ast, leftidx)->index.index;
        E_GET_NODE(p->ast, node)->index_assign.value = rightidx;

        // we stole lefts' children, we don't want to recursively free it.
        E_GET_NODE(p->ast, leftidx)->common.span.file = NULL;
        E_GET_NODE(p->ast, leftidx)->type             = E_AST_NODE_NOP;

        return node;
      }
      if (e_ast_get_node(p->ast, leftidx)->type == E_AST_NODE_MEMBER_ACCESS) {
        int rightidx = e_ast_expr(p, rbp);
        if (rightidx < 0) {
          asterror(peek(p)->span, "Failed to parse RHS [member assignment]\n");

          return -1;
        }

        const char* right = E_GET_NODE(p->ast, leftidx)->member_access.right;

        E_GET_NODE(p->ast, node)->type                = E_AST_NODE_MEMBER_ASSIGN;
        E_GET_NODE(p->ast, node)->member_assign.left  = E_GET_NODE(p->ast, leftidx)->member_assign.left;
        E_GET_NODE(p->ast, node)->member_assign.right = right;
        E_GET_NODE(p->ast, node)->member_assign.value = rightidx;

        // we stole lefts' children, we don't want to recursively free it.
        E_GET_NODE(p->ast, leftidx)->common.span.file = NULL;
        E_GET_NODE(p->ast, leftidx)->type             = E_AST_NODE_NOP;

        return node;
      }

      int rightidx = e_ast_expr(p, rbp);
      if (rightidx < 0) {
        asterror(peek(p)->span, "Failed to parse RHS [assignment]\n");

        return -1;
      }

      E_GET_NODE(p->ast, node)->type         = E_AST_NODE_ASSIGN;
      E_GET_NODE(p->ast, node)->assign.left  = leftidx;
      E_GET_NODE(p->ast, node)->assign.right = rightidx;

      return node;
    }

    // Index: LEFT [ INDEX ]
    case E_TOKEN_TYPE_OPENBRACKET: {
      e_filespan span = peek(p)->span;

      int index = e_ast_expr(p, 0);
      if (index < 0) {
        cerror(span, "Failed to parse index statement\n");

        return -1;
      }

      span = peek(p)->span;
      if (e_ast_expect(p, E_TOKEN_TYPE_CLOSEBRACKET)) {
        cerror(span, "Expected ']' [Index expression]\n");
        return -1;
      }

      E_GET_NODE(p->ast, node)->type        = E_AST_NODE_INDEX;
      E_GET_NODE(p->ast, node)->index.base  = leftidx;
      E_GET_NODE(p->ast, node)->index.index = index;
      return node;
    }

    // Member access
    // x.y
    case E_TOKEN_TYPE_DOT: {
      char* s = peek(p)->val.ident;
      if (e_ast_expect(p, E_TOKEN_TYPE_IDENT)) {
        cerror(prev(p)->span, "Expected identifier [Member Access]\n");
        return -1;
      }

      E_GET_NODE(p->ast, node)->type                = E_AST_NODE_MEMBER_ACCESS;
      E_GET_NODE(p->ast, node)->member_access.left  = leftidx;
      E_GET_NODE(p->ast, node)->member_access.right = e_str_intern(s, p->ast->interner);

      return node;
    }

    // Namespace access
    // x::y
    case E_TOKEN_TYPE_DOUBLE_COLON: {
      if (parse_namespace_access(p, leftidx, node) < 0) { return -1; }
      return node;
    }

    case E_TOKEN_TYPE_PLUSPLUS: {
      E_GET_NODE(p->ast, node)->type                = E_AST_NODE_UNARYOP;
      E_GET_NODE(p->ast, node)->unaryop.op          = E_OPERATOR_INC;
      E_GET_NODE(p->ast, node)->unaryop.is_compound = true;
      E_GET_NODE(p->ast, node)->unaryop.right       = leftidx; // operand is left
      return node;
    }

    case E_TOKEN_TYPE_MINUSMINUS: {
      E_GET_NODE(p->ast, node)->type                = E_AST_NODE_UNARYOP;
      E_GET_NODE(p->ast, node)->unaryop.op          = E_OPERATOR_DEC;
      E_GET_NODE(p->ast, node)->unaryop.is_compound = true;
      E_GET_NODE(p->ast, node)->unaryop.right       = leftidx;
      return node;
    }

    case E_TOKEN_TYPE_DOUBLEEQUAL:
    case E_TOKEN_TYPE_NOTEQUAL:
    case E_TOKEN_TYPE_PLUS:
    case E_TOKEN_TYPE_MINUS:
    case E_TOKEN_TYPE_MULTIPLY:
    case E_TOKEN_TYPE_DIVIDE:
    case E_TOKEN_TYPE_EXPONENT:
    case E_TOKEN_TYPE_MOD:
    case E_TOKEN_TYPE_BAND:
    case E_TOKEN_TYPE_BOR:
    case E_TOKEN_TYPE_AND:
    case E_TOKEN_TYPE_OR:
    case E_TOKEN_TYPE_XOR:
    case E_TOKEN_TYPE_BNOT:
    case E_TOKEN_TYPE_LT:
    case E_TOKEN_TYPE_LTE:
    case E_TOKEN_TYPE_GT:
    case E_TOKEN_TYPE_GTE: {
      int left_bp  = 0;
      int right_bp = 0;
      if (!e_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(
            tk->span, "Operator %s doesn't have a binding power set in e_getbp! assuming 0 [binary operator]\n", e_token_type_to_string(tk->type));
        //
        // return -1;
      }

      e_operator op = conv_token_type_to_operator(tk->type);
      if (op == -1) {
        asterror(tk->span, "Unexpected token: '%s', Expected operator [binary operator]\n", e_token_type_to_string(tk->type));

        return -1;
      }

      e_filespan prev_span = peek(p)->span;

      int right = e_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(prev_span, "Failed to evaluate RHS [binary operator]\n");

        return -1;
      }

      if (E_GET_NODE(p->ast, leftidx)->type == E_AST_NODE_INDEX && tk->val.op.is_compound) {
        E_GET_NODE(p->ast, node)->type                 = E_AST_NODE_INDEX_COMPOUND_OP;
        E_GET_NODE(p->ast, node)->index_compound.op    = op;
        E_GET_NODE(p->ast, node)->index_compound.base  = E_GET_NODE(p->ast, leftidx)->index.base;
        E_GET_NODE(p->ast, node)->index_compound.index = E_GET_NODE(p->ast, leftidx)->index.index;
        E_GET_NODE(p->ast, node)->index_compound.value = right;

        E_GET_NODE(p->ast, leftidx)->type = E_AST_NODE_NOP;

        return node;
      }

      E_GET_NODE(p->ast, node)->type                 = E_AST_NODE_BINARYOP;
      E_GET_NODE(p->ast, node)->binaryop.op          = op;
      E_GET_NODE(p->ast, node)->binaryop.left        = leftidx;
      E_GET_NODE(p->ast, node)->binaryop.is_compound = tk->val.op.is_compound;
      E_GET_NODE(p->ast, node)->binaryop.right       = right;
      return node;
    }

    default: {
      asterror(tk->span, "Unexpected token: '%s'\n", e_token_type_to_string(tk->type));

      return -1;
    }
  }
}

int
e_parse(e_parser* p)
{
  if (!p) return -1;

  int node = -1;

  while (peek(p)) {
    e_filespan take_span = peek(p)->span;

    node = e_ast_expr(p, 0);
    if (node < 0) {
      asterror(take_span, "Error parsing expression [Root statement compilation failed, retreating]\n");
      goto ERR;
    }

    e_ast_node_type type = E_GET_NODE(p->ast, node)->type;
    if (type != E_AST_NODE_FUNCTION_DEFINITION && type != E_AST_NODE_NAMESPACE_DECL && type != E_AST_NODE_STRUCT_DECL
        && type != E_AST_NODE_VARIABLE_DECL && type != E_AST_NODE_ASSERT) {
      // asterror(take_span, "Expected function definition or variable declerations in global scope\n");
      asterror(
          take_span,
          "Expected only function definitions, variable declerations, namespace declerations, struct declerations or assertions in global scope "
          "(%i)\n",
          type);
      e_ast_node_free(p->ast, node);
      goto ERR;
    }

    if (!e_ast_is_limiter_exempt(e_ast_get_node(p->ast, node)->type) && e_ast_expect(p, E_TOKEN_TYPE_SEMICOLON)) {
      asterror(prev(p)->span, "Expected semicolon after expression\n");
      e_ast_node_free(p->ast, node);
      goto ERR;
    }

    int** pstmts  = &E_GET_NODE(p->ast, p->ast->root)->root.stmts;
    u32*  pnstmts = &E_GET_NODE(p->ast, p->ast->root)->root.nstmts;
    u32*  pcstmts = &E_GET_NODE(p->ast, p->ast->root)->root.capacity;

    if (*pnstmts >= *pcstmts) {
      u32  newcap   = MAX((*pcstmts) * 2, 1);
      int* newstmts = realloc(*pstmts, newcap * sizeof(int));
      if (!newstmts) {
        e_ast_node_free(p->ast, node);
        goto ERR;
      }

      *pstmts  = newstmts;
      *pcstmts = newcap;
    }

    // Add it to our list.
    (*pstmts)[*pnstmts] = node;
    (*pnstmts)++;
  }

  return 0;

ERR:
  return -1;
}

int
e_ast_init(e_str_interner* interner, e_ast* ast)
{
  if (ast) {
    *ast = (e_ast){
      .nodes    = e_xalloc(init_node_capacity, sizeof(e_ast_node)),
      .nnodes   = 0,
      .capacity = init_node_capacity,
      .interner = interner,
      .root     = -1,
    };
    if (!ast->nodes) return -1;

    // Initialize our single root node.
    ast->root = e_ast_make_node(ast);
    if (ast->root < 0) return -1;

    ast->nodes[ast->root].type = E_AST_NODE_ROOT;
  }
  return 0;
}

void
e_ast_free(e_ast* ast)
{
  if (ast->root >= 0) e_ast_node_free(ast, ast->root);
  free(ast->nodes);

  memset(ast, 0, sizeof *ast);
}

int
e_parser_init(const e_token* tokens, u32 ntokens, e_ast* ast, e_parser* parser)
{
  *parser = (e_parser){
    .toks  = tokens,
    .ntoks = ntokens,
    .head  = 0,
    .ast   = ast,
  };
  return 0;
}

void
e_parser_free(e_parser* parser)
{ /* Didn't allocate anything. */
}
