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

#include "../inc/kit.ast.h"

#include "../inc/kit.cerr.h"
#include "../inc/kit.lex.h"
#include "../inc/kit.stdafx.h"
#include "../inc/kit.strint.h"

#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define prev(parser) kit_parser_prev(parser)
#define peek(parser) kit_parser_peek(parser)
#define next(parser) kit_parser_next(parser)

// Initial pool size for all nodes
static const u32 init_node_capacity        = 128;
static const u32 init_braced_node_capacity = 32;

#if defined(__has_attribute) && __has_attribute(format)
#  define FORMAT_(...) __attribute__((format(__VA_ARGS__)))
#endif

static inline FORMAT_(printf, 4, 5) bool internal_asterror(const char* file, size_t line, kit_filespan span, const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);

  fprintf(stderr, "[%s:%zu] [%s:%i:%i] ast error: ", file, line, span.file, span.line, span.col);
  vfprintf(stderr, msg, ap);

  va_end(ap);

  return true;
}

#define asterror(span, ...) internal_asterror(__FILE__, __LINE__, span, __VA_ARGS__)

static inline kit_filespan
clonespan(kit_ast* a, kit_filespan s)
{
  kit_filespan dup = {
    .file = (char*)kit_str_intern(s.file, a->interner),
    .line = s.line,
    .col  = s.col,
  };
  // printf("[%s:%i:%i]\n", dup.file, dup.line, dup.col);
  return dup;
}

/* Returns -1 on invalid !!!!! */
static inline kit_operator
conv_token_type_to_operator(kit_token_type t)
{
  switch (t) {
    case KIT_TOKEN_TYPE_PLUS: return KIT_OPERATOR_ADD;
    case KIT_TOKEN_TYPE_MINUS: return KIT_OPERATOR_SUB;
    case KIT_TOKEN_TYPE_MULTIPLY: return KIT_OPERATOR_MUL;
    case KIT_TOKEN_TYPE_DIVIDE: return KIT_OPERATOR_DIV;
    case KIT_TOKEN_TYPE_MOD: return KIT_OPERATOR_MOD;
    case KIT_TOKEN_TYPE_EXPONENT: return KIT_OPERATOR_EXP;
    case KIT_TOKEN_TYPE_AND: return KIT_OPERATOR_AND;
    case KIT_TOKEN_TYPE_NOT: return KIT_OPERATOR_NOT;
    case KIT_TOKEN_TYPE_OR: return KIT_OPERATOR_OR;
    case KIT_TOKEN_TYPE_BAND: return KIT_OPERATOR_BAND;
    case KIT_TOKEN_TYPE_BOR: return KIT_OPERATOR_BOR;
    case KIT_TOKEN_TYPE_BNOT: return KIT_OPERATOR_BNOT;
    case KIT_TOKEN_TYPE_XOR: return KIT_OPERATOR_XOR;
    case KIT_TOKEN_TYPE_DOUBLEEQUAL: return KIT_OPERATOR_ISEQL;
    case KIT_TOKEN_TYPE_NOTEQUAL: return KIT_OPERATOR_ISNEQ;
    case KIT_TOKEN_TYPE_LT: return KIT_OPERATOR_LT;
    case KIT_TOKEN_TYPE_LTE: return KIT_OPERATOR_LTE;
    case KIT_TOKEN_TYPE_GT: return KIT_OPERATOR_GT;
    case KIT_TOKEN_TYPE_GTE: return KIT_OPERATOR_GTE;
    default: return -1;
  }
}

bool
kit_getbp(kit_token_type type, int* left, int* right)
{
  switch (type) {
    case KIT_TOKEN_TYPE_EQUAL:
      *left  = 10;
      *right = 9;
      break;

    case KIT_TOKEN_TYPE_AND:
      *left  = 30;
      *right = 29;
      break;

    case KIT_TOKEN_TYPE_OR:
      *left  = 20;
      *right = 19;
      break;
    case KIT_TOKEN_TYPE_NOT:
      *left  = 0;
      *right = 65;
      break;

    // bitwise
    case KIT_TOKEN_TYPE_BOR: // |
      *left  = 32;
      *right = 33;
      break;
    case KIT_TOKEN_TYPE_XOR: // ^
      *left  = 34;
      *right = 35;
      break;
    case KIT_TOKEN_TYPE_BAND: // &
      *left  = 36;
      *right = 37;
      break;

    // compares
    case KIT_TOKEN_TYPE_GT:
    case KIT_TOKEN_TYPE_LT:
    case KIT_TOKEN_TYPE_GTE:
    case KIT_TOKEN_TYPE_LTE:
    case KIT_TOKEN_TYPE_DOUBLEEQUAL:
    case KIT_TOKEN_TYPE_NOTEQUAL:
      *left  = 40;
      *right = 41;
      break;

    case KIT_TOKEN_TYPE_PLUS:
    case KIT_TOKEN_TYPE_MINUS:
      *left  = 50;
      *right = 51;
      break;
    case KIT_TOKEN_TYPE_MULTIPLY:
    case KIT_TOKEN_TYPE_DIVIDE:
    case KIT_TOKEN_TYPE_MOD:
      *left  = 60;
      *right = 61;
      break;

    case KIT_TOKEN_TYPE_BNOT: // ~
      *left  = 0;
      *right = 65;
      break;

    // member access
    case KIT_TOKEN_TYPE_DOT:
      *left  = 70;
      *right = 69;
      break;

    case KIT_TOKEN_TYPE_DOUBLE_COLON:
      *left  = 75;
      *right = 74;
      break;

    case KIT_TOKEN_TYPE_OPENPAREN:
      *left  = 90;
      *right = 100;
      break;
    case KIT_TOKEN_TYPE_CLOSEPAREN:
      *left  = 0;
      *right = 100;
      break;

    case KIT_TOKEN_TYPE_OPENBRACE:
    case KIT_TOKEN_TYPE_OPENBRACKET:
      *left  = 100;
      *right = 99;
      break;

    case KIT_TOKEN_TYPE_PLUSPLUS:
    case KIT_TOKEN_TYPE_MINUSMINUS:
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
parse_braces(kit_parser* p, int** outstmts, u32* outnstmts)
{
  u32  capacity = init_braced_node_capacity;
  u32  nstmts   = 0;
  int* stmts    = kit_xalloc(capacity, sizeof(int));
  if (!stmts) goto err;

  while (peek(p) != NULL && peek(p)->type != KIT_TOKEN_TYPE_CLOSEBRACE) {
    if (nstmts + 1 >= capacity) {
      u32  newcap   = MAX(capacity * 2, 1);
      int* newstmts = realloc(stmts, sizeof(int) * newcap);
      if (!newstmts) { goto err; }

      stmts    = newstmts;
      capacity = newcap;
    }

    kit_filespan span = peek(p)->span;

    // We need the stmt to figure out if we need to chek for a limiter(;) at the end or not.
    int stmt = kit_ast_expr(p, 0);
    if (stmt < 0) {
      asterror(span, "Failed to parse braced statement [braced statement list]\n");
      goto err;
    }

    stmts[nstmts++] = stmt;

    // isn't exempt and doesn't have a semicolon
    if (!kit_ast_is_limiter_exempt(KIT_GET_NODE(p->ast, stmt)->type) && kit_ast_expect(p, KIT_TOKEN_TYPE_SEMICOLON)) {
      /* we need to be expecting a semi colon a character after the last */
      span.col++;
      asterror(
          prev(p)->span,
          "Expected semi colon ';', got '%s' [braced statement list]\n",
          kit_token_type_to_string(prev(p)->type)); // use older span, newer one is token after where semicolon should be
      goto err;
    }
  }

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEBRACE)) {
    asterror(prev(p)->span, "Expected closing brace '}' [braced statement list]\n");
    goto err;
  }

  if (outstmts) *outstmts = stmts;
  if (outnstmts) *outnstmts = nstmts;

  return 0;

err:
  if (outstmts) *outstmts = NULL;
  if (outnstmts) *outnstmts = 0;
  for (u32 i = 0; i < nstmts; i++) kit_ast_node_free(p->ast, stmts[i]);
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
parse_body(kit_parser* p, int** outstmts, u32* outnstmts)
{
  int  nstmts = 0;
  int* stmts  = NULL;
  int  stmt   = 0;

  if (peek(p) && peek(p)->type == KIT_TOKEN_TYPE_OPENBRACE) {
    next(p); // consume {
    return parse_braces(p, outstmts, outnstmts);
  }

  // Explicity handle zero expression lists.
  if (peek(p) && peek(p)->type == KIT_TOKEN_TYPE_SEMICOLON) {
    next(p);
    if (outstmts) *outstmts = NULL;
    if (outnstmts) *outnstmts = 0;
  }
  // Single line expressions ending with a semi colon.
  else {
    stmts = kit_xalloc(1, sizeof(int));
    if (!stmts) goto err;

    kit_filespan prev_span = peek(p)->span;

    stmt = kit_ast_expr(p, 0);
    if (stmt < 0) {
      asterror(prev_span, "Failed to parse single line statement\n");
      goto err;
    }

    stmts[0] = stmt;
    nstmts   = 1;

    if (outstmts) *outstmts = stmts;
    if (outnstmts) *outnstmts = nstmts;

    kit_ast_node_type type = KIT_GET_NODE(p->ast, stmt)->type;
    if (!kit_ast_is_limiter_exempt(type) && kit_ast_expect(p, KIT_TOKEN_TYPE_SEMICOLON)) {
      asterror(
          prev(p)->span,
          "Expected semi colon after expression [recv=%s, expect=%s]\n",
          kit_token_type_to_string(prev(p)->type),
          kit_token_type_to_string(KIT_TOKEN_TYPE_SEMICOLON));
      goto err;
    }
  }

  return 0;

err:
  kit_ast_node_free(p->ast, stmt);
  free(stmts);
  if (outstmts) *outstmts = NULL;
  if (outnstmts) *outnstmts = 0;
  return -1;
}

int
kit_ast_expect(kit_parser* p, kit_token_type type)
{
  // printf("%s\n", kit_token_type_to_string(peek(p)->type));
  if (!peek(p)) return 1;
  return next(p)->type == type ? 0 : 1;
}

int
kit_ast_expr(kit_parser* p, int rbp)
{
  const kit_token* tk   = next(p);
  int              left = kit_ast_nud(p, tk);
  if (left < 0) return left;

  KIT_GET_NODE(p->ast, left)->common.span = clonespan(p->ast, tk->span);

  while (kit_parser_peek(p)) {
    const kit_token* op = kit_parser_peek(p);

    int left_bp  = 0;
    int right_bp = 0;
    kit_getbp(op->type, &left_bp, &right_bp);

    if (left_bp <= rbp) break;

    left = kit_ast_led(p, kit_parser_next(p), left, right_bp);
    if (left < 0) break;
  }

  return left;
}

/**
 * Returns node on success, a negative integer on error.
 */
static RETURNS_ERRCODE int
parse_variable_decleration(kit_parser* p, bool is_const, int node)
{
  if (node < 0) return node;

  u32         capacity = 16;
  u32         ndecls   = 0;
  int*        decls    = kit_xalloc(capacity, sizeof(int));
  const char* name     = NULL;
  if (!decls) goto err;

  while (peek(p)) {
    const kit_token* name_tk = peek(p);
    if (!name_tk || name_tk->type != KIT_TOKEN_TYPE_IDENT) {
      asterror(name_tk->span, "Expected identifier after 'let', got '%s' [variable decleration]\n", kit_token_type_to_string(name_tk->type));
      goto err;
    }

    next(p);

    int decl_node = kit_ast_make_node(p->ast);
    if (decl_node < 0) goto err;

    name = kit_str_intern(name_tk->val.s, p->ast->interner);
    if (!name) goto err;

    KIT_GET_NODE(p->ast, decl_node)->type         = KIT_AST_NODE_VARIABLE_DECL;
    KIT_GET_NODE(p->ast, decl_node)->let.span     = clonespan(p->ast, name_tk->span);
    KIT_GET_NODE(p->ast, decl_node)->let.name     = name;
    KIT_GET_NODE(p->ast, decl_node)->let.is_const = is_const;

    int initializer = -1;

    if (peek(p)->type == KIT_TOKEN_TYPE_EQUAL) {
      next(p); // consume =
      initializer = kit_ast_expr(p, 0);
      if (initializer < 0) {
        asterror(peek(p)->span, "Failed to parse initializer [variable decleration]\n");
        kit_ast_node_free(p->ast, decl_node);
        goto err;
      }
    }

    // fprintf(stderr, "initializer provided to %s [%i]\n", name, initializer);

    KIT_GET_NODE(p->ast, decl_node)->let.initializer = initializer;

    if (ndecls >= capacity) {
      u32  newcap = capacity * 2;
      int* tmp    = realloc(decls, sizeof(int) * newcap);
      if (!tmp) {
        kit_ast_node_free(p->ast, decl_node);
        goto err;
      }
      decls    = tmp;
      capacity = newcap;
    }

    decls[ndecls++] = decl_node;

    if (!peek(p) || peek(p)->type != KIT_TOKEN_TYPE_COMMA) break;

    next(p); // consume ,
  }

  // optimizations ooo
  if (ndecls == 1) {
    // ooo optimizations ruined span collection yay
    kit_ast_node* src = KIT_GET_NODE(p->ast, decls[0]);
    kit_ast_node* dst = KIT_GET_NODE(p->ast, node);

    dst->type            = src->type;
    dst->let.name        = src->let.name;
    dst->let.span        = src->let.span;
    dst->let.initializer = src->let.initializer;
    dst->let.is_const    = src->let.is_const;

    // prevent double free
    src->let.name      = NULL;
    src->let.span.file = NULL;
    src->type          = KIT_AST_NODE_NOP;

    free(decls);

    return node;
  }

  KIT_GET_NODE(p->ast, node)->type         = KIT_AST_NODE_STATEMENT_LIST;
  KIT_GET_NODE(p->ast, node)->stmts.stmts  = decls;
  KIT_GET_NODE(p->ast, node)->stmts.nstmts = ndecls;

  return node;

err:
  for (u32 i = 0; i < ndecls; i++) kit_ast_node_free(p->ast, decls[i]);
  free(decls);
  return -1;
}

static RETURNS_ERRCODE int
parse_if(kit_parser* p, int node)
{
  if (node < 0) return node;

  u32          cap_else_ifs = 4;
  u32          num_else_ifs = 0;
  kit_if_stmt* else_ifs     = kit_xalloc(cap_else_ifs, sizeof(kit_if_stmt));

  int* body   = NULL;
  u32  nstmts = 0;

  int* else_body   = NULL;
  u32  nelse_stmts = 0;

  int  condition         = -1;
  int  else_if_condition = -1;
  int* else_if_body      = NULL;
  u32  else_if_nstmts    = 0;

  if (else_ifs == NULL) goto err;

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected '(' after 'if', got '%s' [if statement]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  const kit_token* tk = peek(p);

  condition = kit_ast_expr(p, 0);
  if (condition < 0) {
    asterror(tk->span, "Failed to parse condition [if statement]\n");
    goto err;
  }

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')' after condition, but got '%s' [if statement]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  // Take the span before so it starts where the body would start.
  kit_filespan body_span = peek(p)->span;

  if (parse_body(p, &body, &nstmts)) {
    asterror(body_span, "Failed to parse body [if statement]\n");
    goto err;
  }

  if (peek(p) && peek(p)->type == KIT_TOKEN_TYPE_ELSE) {
    while (true) {
      if (!peek(p) || peek(p)->type != KIT_TOKEN_TYPE_ELSE) break; // no more else chains

      next(p); // consume else

      if (peek(p) && peek(p)->type == KIT_TOKEN_TYPE_IF) // else if
      {
        next(p);

        if (num_else_ifs >= cap_else_ifs) {
          u32          newcap           = MAX(cap_else_ifs * 2, 1);
          kit_if_stmt* new_else_ifs     = realloc(else_ifs, sizeof(kit_if_stmt) * newcap);
          u32          new_cap_else_ifs = newcap;
          if (!new_else_ifs) goto err;

          else_ifs     = new_else_ifs;
          cap_else_ifs = new_cap_else_ifs;
        }

        if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPENPAREN)) {
          asterror(prev(p)->span, "Expected '(' after 'else if', but got '%s' [if statement :: else if]\n", kit_token_type_to_string(prev(p)->type));
          goto errloop;
        }

        else_if_condition = kit_ast_expr(p, 0);

        if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEPAREN)) {
          asterror(prev(p)->span, "Expected ')' after condition, but got '%s' [if statement :: else if]\n", kit_token_type_to_string(prev(p)->type));
          goto errloop;
        }

        body_span = peek(p)->span;
        if (parse_body(p, &else_if_body, &else_if_nstmts)) {
          asterror(body_span, "Failed to parse body [if statement :: else if]\n");
          goto errloop;
        }

        else_ifs[num_else_ifs++] = (kit_if_stmt){
          .condition = else_if_condition,
          .body      = else_if_body,
          .nstmts    = else_if_nstmts,
        };
        continue; //

      errloop:
        free(else_if_body);
        kit_ast_node_free(p->ast, else_if_condition);
        goto err;
      }

      // else {}

      kit_filespan else_span = peek(p)->span;

      if (parse_body(p, &else_body, &nelse_stmts)) {
        asterror(else_span, "Failed to parse body [if statement :: else]\n");
        goto err;
      }

      break; // no more deals
    }
  }

  // Write the collected infomation to the struct.
  KIT_GET_NODE(p->ast, node)->type                = KIT_AST_NODE_IF;
  KIT_GET_NODE(p->ast, node)->if_stmt.body        = body;
  KIT_GET_NODE(p->ast, node)->if_stmt.nstmts      = nstmts;
  KIT_GET_NODE(p->ast, node)->if_stmt.condition   = condition;
  KIT_GET_NODE(p->ast, node)->if_stmt.else_body   = else_body;
  KIT_GET_NODE(p->ast, node)->if_stmt.nelse_stmts = nelse_stmts;
  KIT_GET_NODE(p->ast, node)->if_stmt.else_ifs    = else_ifs;
  KIT_GET_NODE(p->ast, node)->if_stmt.nelse_ifs   = num_else_ifs;

  return node;

err:
  kit_ast_node_free(p->ast, condition);
  for (u32 i = 0; i < nstmts; i++) kit_ast_node_free(p->ast, body[i]);
  for (u32 i = 0; i < nelse_stmts; i++) kit_ast_node_free(p->ast, else_body[i]);
  for (u32 i = 0; i < num_else_ifs; i++) {
    for (u32 j = 0; j < else_ifs[i].nstmts; j++) { kit_ast_node_free(p->ast, else_ifs[i].body[j]); }
    free(else_ifs[i].body);
    kit_ast_node_free(p->ast, else_ifs[i].condition);
  }
  free(else_ifs);
  free(else_body);
  free(body);
  return -1;
}

static RETURNS_ERRCODE int
parse_while(kit_parser* p, int node)
{
  if (node < 0) return node;

  int  cnd    = -1;
  int* stmts  = NULL;
  u32  nstmts = 0;

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected ( after 'while'\n");
    goto err;
  }
  // if (peek(p)->type == KIT_TOKEN_TYPE_OPENPAREN) next(p);

  kit_filespan prev_span = peek(p)->span;

  cnd = kit_ast_expr(p, 0);
  if (cnd < 0) {
    asterror(prev_span, "Expected expression\n");
    goto err;
  }

  // if (peek(p)->type == KIT_TOKEN_TYPE_CLOSEPAREN) next(p);

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ) after 'while'\n");
    goto err;
  }

  prev_span = peek(p)->span;
  if (parse_body(p, &stmts, &nstmts)) {
    asterror(prev_span, "Failed to parse while statement body\n");
    goto err;
  }

  KIT_GET_NODE(p->ast, node)->type                 = KIT_AST_NODE_WHILE;
  KIT_GET_NODE(p->ast, node)->while_stmt.condition = cnd;
  KIT_GET_NODE(p->ast, node)->while_stmt.stmts     = stmts;
  KIT_GET_NODE(p->ast, node)->while_stmt.nstmts    = nstmts;

  return node;

err:
  kit_ast_node_free(p->ast, cnd);
  for (u32 i = 0; i < nstmts; i++) kit_ast_node_free(p->ast, stmts[i]);
  free(stmts);
  return -1;
}

static inline const kit_token*
peekn(const kit_parser* p, u32 n)
{
  if (p->head + n >= p->ntoks) return NULL;
  return &p->toks[p->head + n];
}

static bool
is_ranged_for_statement(const kit_parser* p)
{
  u32              n  = 0;
  const kit_token* t0 = peekn(p, n++);
  if (!t0) return false;

  if (t0->type == KIT_TOKEN_TYPE_OPENPAREN) {
    t0 = peekn(p, n++);
    if (!t0) return false;
  }

  if (t0->type == KIT_TOKEN_TYPE_LET) {
    const kit_token* t1 = peekn(p, n++); // identifier
    if (!t1 || t1->type != KIT_TOKEN_TYPE_IDENT) return false;
    const kit_token* t2 = peekn(p, n++); // 'in'
    return t2 && t2->type == KIT_TOKEN_TYPE_IN;
  }

  if (t0->type == KIT_TOKEN_TYPE_IDENT) {
    const kit_token* t1 = peekn(p, n++); // in
    return t1 && t1->type == KIT_TOKEN_TYPE_IN;
  }

  return false;
}

static RETURNS_ERRCODE int
parse_ranged_for(kit_parser* p, int node)
{
  char* iterator_name = NULL;
  int*  stmts         = NULL;
  u32   nstmts        = 0;

  bool given_open_parenthesis = false;
  if (peek(p)->type == KIT_TOKEN_TYPE_OPENPAREN) {
    given_open_parenthesis = true;
    next(p);
  }

  if (peek(p)->type == KIT_TOKEN_TYPE_LET) next(p);

  if (peek(p)->type != KIT_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected identifier, got '%s' [ranged for]\n", kit_token_type_to_string(peek(p)->type));
    goto err;
  }

  iterator_name = kit_strdup(peek(p)->val.ident);
  if (iterator_name == NULL) goto err;

  next(p);

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_IN)) {
    asterror(prev(p)->span, "Expected 'in' after loop variable, got '%s'  [ranged for]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  /* start..stop */
  int start = kit_ast_expr(p, 0);
  if (start < 0) {
    asterror(prev(p)->span, "Expected loop iterator's start value, got '%s'  [ranged for]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_DOTDOT)) {
    asterror(prev(p)->span, "Expected .. after loop iterator start, got '%s'  [ranged for]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  int stop = kit_ast_expr(p, 0);
  if (stop < 0) {
    asterror(prev(p)->span, "Expected loop iterator's stop value, got '%s'  [ranged for]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  if (given_open_parenthesis && kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')' after range expression\n");
    goto err;
  }

  kit_filespan prev_span = peek(p)->span;
  if (parse_body(p, &stmts, &nstmts)) {
    asterror(prev_span, "Failed to parse body [ranged for]\n");
    goto err;
  }

  KIT_GET_NODE(p->ast, node)->type                         = KIT_AST_NODE_RANGED_FOR;
  KIT_GET_NODE(p->ast, node)->for_range_stmt.iterator_name = iterator_name;
  KIT_GET_NODE(p->ast, node)->for_range_stmt.start         = start;
  KIT_GET_NODE(p->ast, node)->for_range_stmt.stop          = stop;
  KIT_GET_NODE(p->ast, node)->for_range_stmt.stmts         = stmts;
  KIT_GET_NODE(p->ast, node)->for_range_stmt.nstmts        = nstmts;

  return node;

err:
  if (iterator_name) free(iterator_name);
  for (u32 i = 0; i < nstmts; i++) kit_ast_node_free(p->ast, stmts[i]);
  free(stmts);
  return -1;
}

static RETURNS_ERRCODE int
parse_for(kit_parser* p, int node)
{
  if (is_ranged_for_statement(p)) { return parse_ranged_for(p, node); }

  int init = -1;
  int cond = -1;

  int* stmts  = NULL;
  u32  nstmts = 0;

  u32  niterators = 0;
  u32  capacity   = 8;
  int* iterators  = kit_xalloc(capacity, sizeof(int));
  if (!iterators) goto err;

  // (
  if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected ( after 'for', got '%s' [for statement]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  // let i = 0
  if (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_SEMICOLON) {
    kit_filespan prev_span = peek(p)->span;

    init = kit_ast_expr(p, 0);
    if (init < 0) {
      asterror(prev_span, "Expected expression (initializers), got '%s' [for statement]\n", kit_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (kit_ast_expect(p, KIT_TOKEN_TYPE_SEMICOLON)) {
    asterror(prev(p)->span, "Expected ';' after initializer list, got '%s' [for statement]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  // i < 10
  if (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_SEMICOLON) {
    kit_filespan prev_span = peek(p)->span;
    cond                   = kit_ast_expr(p, 0);
    if (cond < 0) {
      asterror(prev_span, "Expected expression (condition), got '%s' [for statement]\n", kit_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    next(p);
  }

  // ;
  if (kit_ast_expect(p, KIT_TOKEN_TYPE_SEMICOLON)) {
    asterror(prev(p)->span, "Expected ';' after condition, got '%s' [for statement]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  // i++,y++

  if (peek(p) && peek(p)->type == KIT_TOKEN_TYPE_SEMICOLON) {
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

      kit_filespan prev_span = peek(p)->span;
      int          iter      = kit_ast_expr(p, 0);
      if (iter < 0) {
        asterror(prev_span, "Expected expression (iterators), got '%s' [for statement]\n", kit_token_type_to_string(prev(p)->type));
        goto err;
      }

      iterators[niterators++] = iter;

      if (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_COMMA) break;
      next(p); // consume ,
    }
  }

  // )
  if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')' after header, got '%s' [for statement]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  kit_filespan prev_span = peek(p)->span;

  if (parse_body(p, &stmts, &nstmts)) {
    asterror(prev_span, "Failed to parse body [for statement]\n");
    goto err;
  }

  KIT_GET_NODE(p->ast, node)->type                  = KIT_AST_NODE_FOR;
  KIT_GET_NODE(p->ast, node)->for_stmt.initializers = init;
  KIT_GET_NODE(p->ast, node)->for_stmt.condition    = cond;
  KIT_GET_NODE(p->ast, node)->for_stmt.niterators   = niterators;
  KIT_GET_NODE(p->ast, node)->for_stmt.iterators    = iterators;
  KIT_GET_NODE(p->ast, node)->for_stmt.stmts        = stmts;
  KIT_GET_NODE(p->ast, node)->for_stmt.nstmts       = nstmts;

  return node;

err:
  if (init >= 0) kit_ast_node_free(p->ast, init);
  if (cond >= 0) kit_ast_node_free(p->ast, cond);

  for (u32 i = 0; i < niterators; i++) kit_ast_node_free(p->ast, iterators[i]);
  for (u32 i = 0; i < nstmts; i++) kit_ast_node_free(p->ast, stmts[i]);

  free(iterators);
  free(stmts);
  return -1;
}

static RETURNS_ERRCODE int
parse_function(kit_parser* p, bool external, int node)
{
  if (node < 0) return node;

  u32              names_capacity       = 0;
  u32              arg_names_size       = 0;
  const char**     arg_names            = NULL;
  const char*      intern_function_name = NULL;
  const char*      arg_name             = NULL;
  const kit_token* name_tk              = NULL;
  int*             stmts                = NULL;
  u32              nstmts               = 0;

  name_tk = peek(p);
  if (kit_ast_expect(p, KIT_TOKEN_TYPE_IDENT)) {
    asterror(prev(p)->span, "Expected function name after 'fn'/'extern', got '%s' [function header]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPENPAREN)) {
    asterror(prev(p)->span, "Expected (, got '%s' [function header]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  intern_function_name = kit_str_intern(name_tk->val.ident, p->ast->interner);

  names_capacity = 8; // NON ZERO
  arg_names_size = 0;
  arg_names      = (const char**)kit_xalloc(names_capacity, sizeof(char*));

  while (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_CLOSEPAREN) {
    const kit_token* tk = peek(p);
    if (tk->type != KIT_TOKEN_TYPE_IDENT) {
      asterror(peek(p)->span, "Expected parameter name, got '%s' [function header]\n", kit_token_type_to_string(prev(p)->type));
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

    arg_name = kit_str_intern(tk->val.ident, p->ast->interner);
    if (!arg_name) {
      asterror(peek(p)->span, "Alloc error [function argument name]\n");
      goto err;
    }

    arg_names[arg_names_size] = arg_name;
    arg_names_size++;

    if (peek(p) && peek(p)->type == KIT_TOKEN_TYPE_CLOSEPAREN) { break; }

    if (kit_ast_expect(p, KIT_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',', got '%s' [function header]\n", kit_token_type_to_string(prev(p)->type));
      goto err;
    }
  }

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEPAREN)) {
    asterror(prev(p)->span, "Expected ')', got '%s' [function header]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  kit_filespan parsing_span = peek(p)->span;

  if (external) {
    if (kit_ast_expect(p, KIT_TOKEN_TYPE_SEMICOLON)) {
      asterror(prev(p)->span, "Expected ')', got '%s' [external function decleration]\n", kit_token_type_to_string(prev(p)->type));
      goto err;
    }
  } else {
    if (parse_body(p, &stmts, &nstmts)) {
      asterror(parsing_span, "Failed to parse function body [function definition]\n");
      goto err;
    }
  }

  KIT_GET_NODE(p->ast, node)->type        = KIT_AST_NODE_FUNCTION_DEFINITION;
  KIT_GET_NODE(p->ast, node)->func.name   = intern_function_name;
  KIT_GET_NODE(p->ast, node)->func.args   = arg_names;
  KIT_GET_NODE(p->ast, node)->func.nargs  = arg_names_size;
  KIT_GET_NODE(p->ast, node)->func.stmts  = stmts;
  KIT_GET_NODE(p->ast, node)->func.nstmts = nstmts;

  return node;

err:
  free((void*)arg_names);
  for (u32 i = 0; i < nstmts; i++) { kit_ast_node_free(p->ast, stmts[i]); }
  free(stmts);
  return -1;
}

static RETURNS_ERRCODE int
parse_function_call(kit_parser* p, int left, int node)
{
  u32  capacity = 16;
  u32  nargs    = 0;
  int* args     = kit_xalloc(capacity, sizeof(int)); // argument nodes

  if (!args) goto err;

  KIT_GET_NODE(p->ast, node)->type      = KIT_AST_NODE_CALL;
  KIT_GET_NODE(p->ast, node)->call.func = left;
  // printf("AST: Detected function call: %s\n", tk->val.ident);

  while (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_CLOSEPAREN) {
    if (nargs >= capacity) {
      u32  newcap       = MAX(capacity * 2, 1);
      int* new_args     = realloc(args, newcap * sizeof(int));
      u32  new_capacity = newcap;

      if (new_args == NULL) goto err;

      args     = new_args;
      capacity = new_capacity;
    }

    int expr = kit_ast_expr(p, 0);
    if (expr < 0) goto err;

    /**
     * 0 binding power is to parse everything
     * until the closing parenthesis.
     */
    args[nargs++] = expr;

    if (peek(p)->type == KIT_TOKEN_TYPE_CLOSEPAREN) { break; }

    if (kit_ast_expect(p, KIT_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ')', got '%s'\n", kit_token_type_to_string(prev(p)->type));
      goto err;
    }
  }
  // consume )
  next(p);

  KIT_GET_NODE(p->ast, node)->call.args  = args;
  KIT_GET_NODE(p->ast, node)->call.nargs = nargs;

  return node;

err:
  for (u32 i = 0; i < nargs; i++) kit_ast_node_free(p->ast, args[i]);
  free(args);
  return -1;
}

/**
 * On success, returns the node id.
 */
static inline RETURNS_ERRCODE int
make_literal_node(kit_ast* p, int node, const kit_token* tk)
{
  if (node < 0) return node;

  kit_ast_node* nodep = kit_ast_get_node(p, node);
  if (tk->type == KIT_TOKEN_TYPE_INT) {
    nodep->type = KIT_AST_NODE_INT;
    nodep->i.i  = tk->val.i;
  } else if (tk->type == KIT_TOKEN_TYPE_FLOAT) {
    nodep->type = KIT_AST_NODE_FLOAT;
    nodep->f.f  = tk->val.f;
  } else if (tk->type == KIT_TOKEN_TYPE_BOOL) {
    nodep->type = KIT_AST_NODE_BOOL;
    nodep->b.b  = tk->val.b;
  } else if (tk->type == KIT_TOKEN_TYPE_CHAR) {
    nodep->type = KIT_AST_NODE_CHAR;
    nodep->c.c  = tk->val.c;
  } else if (tk->type == KIT_TOKEN_TYPE_STRING) {
    nodep->type = KIT_AST_NODE_STRING;
    nodep->s.s  = kit_strdup(tk->val.s);
    if (!nodep->s.s) return -1;
  }
  return node;
}

static RETURNS_ERRCODE int
parse_list(kit_parser* p, int node)
{
  u32  capelems = 16;
  u32  nelems   = 0;
  int* elems    = kit_xalloc(capelems, sizeof(int));

  while (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_CLOSEBRACKET) {
    int elem = kit_ast_expr(p, 0);

    if (nelems >= capelems) {
      u32  new_cap   = MAX(capelems * 2, 1);
      int* new_elems = realloc(elems, new_cap * sizeof(int));
      if (!new_elems) { goto err; }

      elems    = new_elems;
      capelems = new_cap;
    }

    elems[nelems++] = elem;

    if (peek(p) && peek(p)->type == KIT_TOKEN_TYPE_CLOSEBRACKET) { break; }

    if (kit_ast_expect(p, KIT_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ']', got '%s' [list literal]\n", kit_token_type_to_string(prev(p)->type));
      goto err;
    }
  }

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEBRACKET)) {
    asterror(prev(p)->span, "Expected ']', got '%s' [list literal]\n", kit_token_type_to_string(prev(p)->type));
    goto err;
  }

  KIT_GET_NODE(p->ast, node)->type        = KIT_AST_NODE_LIST;
  KIT_GET_NODE(p->ast, node)->list.elems  = elems;
  KIT_GET_NODE(p->ast, node)->list.nelems = nelems;

  return node;

err:
  for (u32 i = 0; i < nelems; i++) { kit_ast_node_free(p->ast, elems[i]); }
  free(elems);
  return -1;
}

static RETURNS_ERRCODE int
parse_map(kit_parser* p, int node)
{
  // if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPEN_BRACE)) {
  //   asterror(prev(p)->span, "Expected '{' after '#' [map literal]\n");
  //   return -1;
  // }
  int e = 0;

  u32 capacity = 8;
  u32 npairs   = 0;

  int* keys   = kit_xalloc(capacity, sizeof(int));
  int* values = kit_xalloc(capacity, sizeof(int));

  if (!keys || !values) goto ERR;

  while (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_CLOSEBRACE) {
    // key
    int key = kit_ast_expr(p, 0);
    if (key < 0) goto ERR;

    if (kit_ast_expect(p, KIT_TOKEN_TYPE_COLON)) {
      asterror(prev(p)->span, "Expected ':' [map literal]\n");
      goto ERR;
    }

    // value
    int value = kit_ast_expr(p, 0);
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

    if (peek(p)->type == KIT_TOKEN_TYPE_CLOSEBRACE) break;

    if (kit_ast_expect(p, KIT_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or '}' [map literal]\n");
      goto ERR;
    }
  }

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEBRACE)) {
    asterror(prev(p)->span, "Expected '}' [map literal]\n");
    goto ERR;
  }

  KIT_GET_NODE(p->ast, node)->type       = KIT_AST_NODE_MAP;
  KIT_GET_NODE(p->ast, node)->map.keys   = keys;
  KIT_GET_NODE(p->ast, node)->map.values = values;
  KIT_GET_NODE(p->ast, node)->map.npairs = npairs;

  return node;

ERR:
  for (u32 i = 0; i < npairs; i++) {
    kit_ast_node_free(p->ast, keys[i]);
    kit_ast_node_free(p->ast, values[i]);
  }
  free(keys);
  free(values);
  return e ? e : -1;
}

static RETURNS_ERRCODE int
parse_namespace_decleration(kit_parser* p, int node)
{
  int  e      = 0;
  int* stmts  = NULL;
  u32  nstmts = 0;

  if (!peek(p) || peek(p)->type != KIT_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected namespace name, got '%s' [namespace decleration]\n", kit_token_type_to_string(prev(p)->type));
    goto ERR;
  }

  const kit_token* name_tk = next(p);

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPENBRACE)) {
    asterror(prev(p)->span, "Expected '{' after namespace name, got '%s' [namespace decleration]\n", kit_token_type_to_string(prev(p)->type));
    goto ERR;
  }

  kit_filespan store = peek(p)->span;

  e = parse_braces(p, &stmts, &nstmts);
  if (e) {
    asterror(store, "Failed to parse namespace decleration [namespace decleration :: collection pass]\n");
    goto ERR;
  }

  for (u32 i = 0; i < nstmts; i++) {
    kit_ast_node_type type = KIT_GET_NODE(p->ast, stmts[i])->common.type;
    kit_filespan      span = KIT_GET_NODE(p->ast, stmts[i])->common.span;

    if (type == KIT_AST_NODE_STATEMENT_LIST) {
      u32  list_nstmts = KIT_GET_NODE(p->ast, stmts[i])->stmts.nstmts;
      int* list_stmts  = KIT_GET_NODE(p->ast, stmts[i])->stmts.stmts;
      for (u32 j = 0; j < list_nstmts; j++) {
        type = KIT_GET_NODE(p->ast, list_stmts[j])->common.type;
        if (type != KIT_AST_NODE_FUNCTION_DEFINITION && type != KIT_AST_NODE_STRUCT_DECL && type != KIT_AST_NODE_NAMESPACE_DECL
            && type != KIT_AST_NODE_VARIABLE_DECL && type != KIT_AST_NODE_ASSERT) {
          asterror(span, "Expected only function definitions, variable declerations, namespace declerations or assertions in namespace scope\n");

          goto ERR;
        }
      }
    } else if (
        type != KIT_AST_NODE_FUNCTION_DEFINITION && type != KIT_AST_NODE_STRUCT_DECL && type != KIT_AST_NODE_NAMESPACE_DECL
        && type != KIT_AST_NODE_VARIABLE_DECL && type != KIT_AST_NODE_ASSERT) {
      asterror(span, "Expected only function definitions, variable declerations, namespace declerations or assertions in namespace scope\n");

      goto ERR;
    }
  }

  const char* ident = kit_str_intern(name_tk->val.ident, p->ast->interner);
  if (!ident) { goto ERR; }

  KIT_GET_NODE(p->ast, node)->type                  = KIT_AST_NODE_NAMESPACE_DECL;
  KIT_GET_NODE(p->ast, node)->namespace_decl.name   = ident;
  KIT_GET_NODE(p->ast, node)->namespace_decl.stmts  = stmts;
  KIT_GET_NODE(p->ast, node)->namespace_decl.nstmts = nstmts;

  return node;

ERR:
  for (u32 i = 0; i < nstmts; i++) kit_ast_node_free(p->ast, stmts[i]);

  free(stmts);
  return e ? e : -1;
}

static RETURNS_ERRCODE int
parse_namespace_call(kit_parser* p, int left, int node)
{
  u32  capacity = 16;
  u32  nargs    = 0;
  int* args     = kit_xalloc(capacity, sizeof(int));
  if (!args) { goto ERR; }

  next(p); // '('

  while (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_CLOSEPAREN) {
    if (nargs >= capacity) {
      u32  newcap   = MAX(capacity * 2, 1);
      int* new_args = realloc(args, sizeof(int) * newcap);
      if (!new_args) { goto ERR; }
      args     = new_args;
      capacity = newcap;
    }

    args[nargs++] = kit_ast_expr(p, 0);

    if (peek(p)->type == KIT_TOKEN_TYPE_CLOSEPAREN) break;

    if (kit_ast_expect(p, KIT_TOKEN_TYPE_COMMA)) {
      asterror(prev(p)->span, "Expected ',' or ')' [qualified function call]\n");
      goto ERR;
    }
  }

  next(p); // ')'

  KIT_GET_NODE(p->ast, node)->type       = KIT_AST_NODE_CALL;
  KIT_GET_NODE(p->ast, node)->call.func  = left;
  KIT_GET_NODE(p->ast, node)->call.args  = args;
  KIT_GET_NODE(p->ast, node)->call.nargs = nargs;

  return node;

ERR:
  for (u32 i = 0; i < nargs; i++) kit_ast_node_free(p->ast, args[i]);
  free(args);
  return -1;
}

static RETURNS_ERRCODE int
parse_namespace_access(kit_parser* p, int leftidx, int node)
{
  if (!peek(p) || peek(p)->type != KIT_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected identifier after :: [namespace access]\n");

    return -1;
  }

  const kit_token* ident = next(p);

  if (KIT_GET_NODE(p->ast, leftidx)->type != KIT_AST_NODE_VARIABLE) {
    asterror(peek(p)->span, "LHS of :: must be an identifier [namespace access]\n");
    return -1;
  }

  const char* left_name = KIT_GET_NODE(p->ast, leftidx)->ident.ident;

  size_t len      = strlen(left_name) + strlen(ident->val.ident) + 3; // +2 for ::, 1 for \0
  char*  fullname = kit_xalloc(1, len);
  if (!fullname) { return -1; }

  snprintf(fullname, len, "%s::%s", left_name, ident->val.ident);

  const char* interned = kit_str_intern(fullname, p->ast->interner);

  free(fullname);

  // just a namespaced variable.
  KIT_GET_NODE(p->ast, node)->type        = KIT_AST_NODE_VARIABLE;
  KIT_GET_NODE(p->ast, node)->ident.ident = interned;

  return node;
}

static RETURNS_ERRCODE int
parse_struct_decleration(kit_parser* p, int node)
{
  int e = 0;

  int* stmts  = NULL;
  u32  nstmts = 0;

  if (!peek(p) || peek(p)->type != KIT_TOKEN_TYPE_IDENT) {
    asterror(peek(p)->span, "Expected structure name, got '%s' [structure decleration]\n", kit_token_type_to_string(peek(p)->type));
    goto ERR;
  }

  const kit_token* name_tk = next(p);

  if (kit_ast_expect(p, KIT_TOKEN_TYPE_OPENBRACE)) {
    asterror(prev(p)->span, "Expected '{' after structure name, got '%s' [structure decleration]\n", kit_token_type_to_string(prev(p)->type));
    goto ERR;
  }

  kit_filespan store = peek(p)->span;

  e = parse_braces(p, &stmts, &nstmts);
  if (e) {
    asterror(store, "Failed to parse structure decleration body [structure decleration :: collection pass]\n");
    goto ERR;
  }

  for (u32 i = 0; i < nstmts; i++) {
    kit_ast_node_type type = KIT_GET_NODE(p->ast, stmts[i])->common.type;
    kit_filespan      span = KIT_GET_NODE(p->ast, stmts[i])->common.span;

    const char* s = "Expected only function definitions or variable declerations in structure decleration scope\n";

    if (type == KIT_AST_NODE_STATEMENT_LIST) {
      u32  list_nstmts = KIT_GET_NODE(p->ast, stmts[i])->stmts.nstmts;
      int* list_stmts  = KIT_GET_NODE(p->ast, stmts[i])->stmts.stmts;
      for (u32 j = 0; j < list_nstmts; j++) {
        type = KIT_GET_NODE(p->ast, list_stmts[j])->common.type;
        if (type != KIT_AST_NODE_FUNCTION_DEFINITION && type != KIT_AST_NODE_VARIABLE_DECL) {
          asterror(span, "%s", s);
          goto ERR;
        }
      }
    } else if (type != KIT_AST_NODE_FUNCTION_DEFINITION && type != KIT_AST_NODE_VARIABLE_DECL) {
      asterror(span, "%s", s);
      goto ERR;
    }
  }

  const char* interned = kit_str_intern(name_tk->val.ident, p->ast->interner);
  if (!interned) goto ERR;

  KIT_GET_NODE(p->ast, node)->type               = KIT_AST_NODE_STRUCT_DECL;
  KIT_GET_NODE(p->ast, node)->struct_decl.name   = interned;
  KIT_GET_NODE(p->ast, node)->struct_decl.stmts  = stmts;
  KIT_GET_NODE(p->ast, node)->struct_decl.nstmts = nstmts;

  return node;

ERR:
  for (u32 i = 0; i < nstmts; i++) kit_ast_node_free(p->ast, stmts[i]);
  free(stmts);
  return e ? e : -1;
}

int
kit_ast_nud(kit_parser* p, const kit_token* tk)
{
  if (!tk || !p) return -1;

  /**
   * !!!
   * Node is owned by this function, and led!!!
   * If any error occurs, only these function are allowed
   * to free the node.
   */
  int node = kit_ast_make_node(p->ast);
  if (node < 0) return node; // ERROR!

  /* zero out the node for safety. */
  memset(KIT_GET_NODE(p->ast, node), 0, sizeof(kit_ast_node));

  KIT_GET_NODE(p->ast, node)->common.span = clonespan(p->ast, tk->span);

  // if (tk->type == KIT_TOKEN_TYPE_EOF) return -1;

  switch (tk->type) {
    case KIT_TOKEN_TYPE_INT:
    case KIT_TOKEN_TYPE_CHAR:
    case KIT_TOKEN_TYPE_BOOL:
    case KIT_TOKEN_TYPE_FLOAT:
    case KIT_TOKEN_TYPE_STRING: {
      int e = make_literal_node(p->ast, node, tk);
      if (e < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_DEFER: {
      u32  nstmts = 0;
      int* stmts  = NULL;
      if (parse_body(p, &stmts, &nstmts) < 0) {
        asterror(tk->span, "Failed to parse statements [defer]\n");
        kit_ast_node_free(p->ast, node);
        return -1;
      }

      KIT_GET_NODE(p->ast, node)->type         = KIT_AST_NODE_DEFER;
      KIT_GET_NODE(p->ast, node)->defer.nstmts = nstmts;
      KIT_GET_NODE(p->ast, node)->defer.stmts  = stmts;
      return node;
    }

    case KIT_TOKEN_TYPE_ASSERT: {
      int expr = kit_ast_expr(p, 0);
      if (expr < 0) return -1;

      KIT_GET_NODE(p->ast, node)->type                     = KIT_AST_NODE_ASSERT;
      KIT_GET_NODE(p->ast, node)->assertion.stmt           = expr;
      KIT_GET_NODE(p->ast, node)->assertion.assertion_line = kit_strdup(tk->val.assertion_line);
      return node;
    }

    /* List declerator ([elem1,elem2,]) */
    case KIT_TOKEN_TYPE_OPENBRACKET: {
      if (parse_list(p, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    /* Map declerator */
    case KIT_TOKEN_TYPE_HASH_OPENBRACE: {
      if (parse_map(p, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_IDENT: {
      const char* interned = kit_str_intern(tk->val.ident, p->ast->interner);
      if (!interned) goto err1;

      // Just a variable!
      KIT_GET_NODE(p->ast, node)->type        = KIT_AST_NODE_VARIABLE;
      KIT_GET_NODE(p->ast, node)->ident.ident = interned;

      return node;

    err1:
      kit_ast_node_free(p->ast, node);
      return -1;
    }

    case KIT_TOKEN_TYPE_OPENPAREN: {
      int old_node = node;

      node = kit_ast_expr(p, 0);
      if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEPAREN)) {
        kit_filespan opening_paren_span = KIT_GET_NODE(p->ast, old_node)->common.span;
        // asterror(opening_paren_span, "Expected ')' here [%s:%i:%i] for this '('\n", peek(p)->span.file, peek(p)->span.line, peek(p)->span.col);
        asterror(
            prev(p)->span,
            "Expected ')' for this '(' [%s:%i:%i], got '%s'\n",
            opening_paren_span.file,
            opening_paren_span.line,
            opening_paren_span.col,
            kit_token_type_to_string(prev(p)->type));

        kit_ast_node_free(p->ast, node);
        return -1;
      }

      // Replaced it with expression
      kit_ast_node_free(p->ast, old_node);
      return node;
    }

    /* { stmts;stmts;stmts; } */
    case KIT_TOKEN_TYPE_OPENBRACE: {
      int* stmts  = NULL;
      u32  nstmts = 0;
      if (parse_braces(p, &stmts, &nstmts)) {
        kit_ast_node_free(p->ast, node);
        asterror(tk->span, "Failed to parse braced statement\n");
        return -1;
      }

      KIT_GET_NODE(p->ast, node)->type         = KIT_AST_NODE_STATEMENT_LIST;
      KIT_GET_NODE(p->ast, node)->stmts.stmts  = stmts;
      KIT_GET_NODE(p->ast, node)->stmts.nstmts = nstmts;

      return node;
    }

    case KIT_TOKEN_TYPE_BREAK: {
      KIT_GET_NODE(p->ast, node)->type = KIT_AST_NODE_BREAK;
      return node;
    }

    case KIT_TOKEN_TYPE_CONTINUE: {
      KIT_GET_NODE(p->ast, node)->type = KIT_AST_NODE_CONTINUE;
      return node;
    }

    case KIT_TOKEN_TYPE_RETURN: {
      KIT_GET_NODE(p->ast, node)->type = KIT_AST_NODE_RETURN;
      if (peek(p) && peek(p)->type != KIT_TOKEN_TYPE_SEMICOLON) {
        // Expect an expression
        int expr = kit_ast_expr(p, 0);
        if (expr < 0) {
          kit_ast_node_free(p->ast, node);
          return -1; // Parser will eat semicolon
        }

        KIT_GET_NODE(p->ast, node)->ret.expr_id          = expr;
        KIT_GET_NODE(p->ast, node)->ret.has_return_value = true;
      } else {
        KIT_GET_NODE(p->ast, node)->ret.expr_id          = -1;
        KIT_GET_NODE(p->ast, node)->ret.has_return_value = false;
      }

      return node;
    }

    case KIT_TOKEN_TYPE_CONST:
    case KIT_TOKEN_TYPE_LET: {
      // const x = 69;
      // const let gay = true;
      bool is_const = (tk->type == KIT_TOKEN_TYPE_CONST);

      // skip the 2nd let tok if it exists
      if (tk->type == KIT_TOKEN_TYPE_CONST && peek(p) && peek(p)->type == KIT_TOKEN_TYPE_LET) { next(p); }

      // skip the 2nd const tok if it exists
      if (tk->type == KIT_TOKEN_TYPE_LET && peek(p) && peek(p)->type == KIT_TOKEN_TYPE_CONST) {
        is_const = true;
        next(p);
      }

      if (parse_variable_decleration(p, is_const, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_STRUCT: {
      if (parse_struct_decleration(p, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_IF: {
      if (parse_if(p, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_WHILE: {
      if (parse_while(p, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_FOR: {
      if (parse_for(p, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_FN: {
      if (parse_function(p, false, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_EXTERN: {
      if (parse_function(p, true, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_NAMESPACE: {
      if (parse_namespace_decleration(p, node) < 0) {
        kit_ast_node_free(p->ast, node);
        return -1;
      }
      return node;
    }

    case KIT_TOKEN_TYPE_SEMICOLON: {
      KIT_GET_NODE(p->ast, node)->type = KIT_AST_NODE_NOP;
      return node;
    }

    case KIT_TOKEN_TYPE_MINUS:
    case KIT_TOKEN_TYPE_NOT:
    case KIT_TOKEN_TYPE_BNOT: {
      int left_bp  = 0;
      int right_bp = 0;
      if (!kit_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(tk->span, "Unexpected token: '%s'\n", kit_token_type_to_string(tk->type));

        kit_ast_node_free(p->ast, node);
        return -1;
      }

      kit_operator op = conv_token_type_to_operator(tk->type);
      if (op < 0) {
        asterror(tk->span, "Unexpected token: '%s'\n", kit_token_type_to_string(tk->type));

        kit_ast_node_free(p->ast, node);
        return -1;
      }

      int right = kit_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(tk->span, "Failed to evaluate RHS of unary operator\n");

        kit_ast_node_free(p->ast, node);
        return -1;
      }

      KIT_GET_NODE(p->ast, node)->type                = KIT_AST_NODE_UNARYOP;
      KIT_GET_NODE(p->ast, node)->unaryop.op          = op;
      KIT_GET_NODE(p->ast, node)->unaryop.is_compound = tk->val.op.is_compound;
      KIT_GET_NODE(p->ast, node)->unaryop.right       = right;
      return node;
    }

    default: asterror(tk->span, "Unexpected token: '%s'\n", kit_token_type_to_string(tk->type)); return -1;
  }

  /* The handlers consume all the tokens that nud() is supposed to consume. */
  return -1;
}

int
kit_ast_led(kit_parser* p, const kit_token* tk, int leftidx, int rbp)
{
  if (!tk || !p || leftidx < 0) return -1;

  int node = kit_ast_make_node(p->ast);
  if (node < 0) return node; // ERROR!

  KIT_GET_NODE(p->ast, node)->common.span = clonespan(p->ast, tk->span);

  switch (tk->type) {
    case KIT_TOKEN_TYPE_OPENPAREN:
      if (parse_function_call(p, leftidx, node) < 0) { return -1; }
      return node;

    case KIT_TOKEN_TYPE_EQUAL: {
      // printf("ASSIGN YAYY!!\n");

      // Assigning to a member of a (list/struct)
      if (kit_ast_get_node(p->ast, leftidx)->type == KIT_AST_NODE_INDEX) {
        int rightidx = kit_ast_expr(p, rbp);
        if (rightidx < 0) {
          asterror(peek(p)->span, "Failed to parse RHS [index assignment]\n");

          return -1;
        }

        KIT_GET_NODE(p->ast, node)->type               = KIT_AST_NODE_INDEX_ASSIGN;
        KIT_GET_NODE(p->ast, node)->index_assign.base  = KIT_GET_NODE(p->ast, leftidx)->index.base;
        KIT_GET_NODE(p->ast, node)->index_assign.index = KIT_GET_NODE(p->ast, leftidx)->index.index;
        KIT_GET_NODE(p->ast, node)->index_assign.value = rightidx;

        // we stole lefts' children, we don't want to recursively free it.
        KIT_GET_NODE(p->ast, leftidx)->common.span.file = NULL;
        KIT_GET_NODE(p->ast, leftidx)->type             = KIT_AST_NODE_NOP;

        return node;
      }
      if (kit_ast_get_node(p->ast, leftidx)->type == KIT_AST_NODE_MEMBER_ACCESS) {
        int rightidx = kit_ast_expr(p, rbp);
        if (rightidx < 0) {
          asterror(peek(p)->span, "Failed to parse RHS [member assignment]\n");

          return -1;
        }

        const char* right = KIT_GET_NODE(p->ast, leftidx)->member_access.right;

        KIT_GET_NODE(p->ast, node)->type                = KIT_AST_NODE_MEMBER_ASSIGN;
        KIT_GET_NODE(p->ast, node)->member_assign.left  = KIT_GET_NODE(p->ast, leftidx)->member_assign.left;
        KIT_GET_NODE(p->ast, node)->member_assign.right = right;
        KIT_GET_NODE(p->ast, node)->member_assign.value = rightidx;

        // we stole lefts' children, we don't want to recursively free it.
        KIT_GET_NODE(p->ast, leftidx)->common.span.file = NULL;
        KIT_GET_NODE(p->ast, leftidx)->type             = KIT_AST_NODE_NOP;

        return node;
      }

      int rightidx = kit_ast_expr(p, rbp);
      if (rightidx < 0) {
        asterror(peek(p)->span, "Failed to parse RHS [assignment]\n");

        return -1;
      }

      KIT_GET_NODE(p->ast, node)->type         = KIT_AST_NODE_ASSIGN;
      KIT_GET_NODE(p->ast, node)->assign.left  = leftidx;
      KIT_GET_NODE(p->ast, node)->assign.right = rightidx;

      return node;
    }

    // Index: LEFT [ INDEX ]
    case KIT_TOKEN_TYPE_OPENBRACKET: {
      kit_filespan span = peek(p)->span;

      int index = kit_ast_expr(p, 0);
      if (index < 0) {
        cerror(span, "Failed to parse index statement\n");

        return -1;
      }

      span = peek(p)->span;
      if (kit_ast_expect(p, KIT_TOKEN_TYPE_CLOSEBRACKET)) {
        cerror(span, "Expected ']' [Index expression]\n");
        return -1;
      }

      KIT_GET_NODE(p->ast, node)->type        = KIT_AST_NODE_INDEX;
      KIT_GET_NODE(p->ast, node)->index.base  = leftidx;
      KIT_GET_NODE(p->ast, node)->index.index = index;
      return node;
    }

    // Member access
    // x.y
    case KIT_TOKEN_TYPE_DOT: {
      char* s = peek(p)->val.ident;
      if (kit_ast_expect(p, KIT_TOKEN_TYPE_IDENT)) {
        cerror(prev(p)->span, "Expected identifier [Member Access]\n");
        return -1;
      }

      KIT_GET_NODE(p->ast, node)->type                = KIT_AST_NODE_MEMBER_ACCESS;
      KIT_GET_NODE(p->ast, node)->member_access.left  = leftidx;
      KIT_GET_NODE(p->ast, node)->member_access.right = kit_str_intern(s, p->ast->interner);

      return node;
    }

    // Namespace access
    // x::y
    case KIT_TOKEN_TYPE_DOUBLE_COLON: {
      if (parse_namespace_access(p, leftidx, node) < 0) { return -1; }
      return node;
    }

    case KIT_TOKEN_TYPE_PLUSPLUS: {
      KIT_GET_NODE(p->ast, node)->type                = KIT_AST_NODE_UNARYOP;
      KIT_GET_NODE(p->ast, node)->unaryop.op          = KIT_OPERATOR_INC;
      KIT_GET_NODE(p->ast, node)->unaryop.is_compound = true;
      KIT_GET_NODE(p->ast, node)->unaryop.right       = leftidx; // operand is left
      return node;
    }

    case KIT_TOKEN_TYPE_MINUSMINUS: {
      KIT_GET_NODE(p->ast, node)->type                = KIT_AST_NODE_UNARYOP;
      KIT_GET_NODE(p->ast, node)->unaryop.op          = KIT_OPERATOR_DEC;
      KIT_GET_NODE(p->ast, node)->unaryop.is_compound = true;
      KIT_GET_NODE(p->ast, node)->unaryop.right       = leftidx;
      return node;
    }

    case KIT_TOKEN_TYPE_DOUBLEEQUAL:
    case KIT_TOKEN_TYPE_NOTEQUAL:
    case KIT_TOKEN_TYPE_PLUS:
    case KIT_TOKEN_TYPE_MINUS:
    case KIT_TOKEN_TYPE_MULTIPLY:
    case KIT_TOKEN_TYPE_DIVIDE:
    case KIT_TOKEN_TYPE_EXPONENT:
    case KIT_TOKEN_TYPE_MOD:
    case KIT_TOKEN_TYPE_BAND:
    case KIT_TOKEN_TYPE_BOR:
    case KIT_TOKEN_TYPE_AND:
    case KIT_TOKEN_TYPE_OR:
    case KIT_TOKEN_TYPE_XOR:
    case KIT_TOKEN_TYPE_BNOT:
    case KIT_TOKEN_TYPE_LT:
    case KIT_TOKEN_TYPE_LTE:
    case KIT_TOKEN_TYPE_GT:
    case KIT_TOKEN_TYPE_GTE: {
      int left_bp  = 0;
      int right_bp = 0;
      if (!kit_getbp(tk->type, &left_bp, &right_bp)) {
        asterror(
            tk->span,
            "Operator %s doesn't have a binding power set in kit_getbp! assuming 0 [binary operator]\n",
            kit_token_type_to_string(tk->type));
        //
        // return -1;
      }

      kit_operator op = conv_token_type_to_operator(tk->type);
      if (op == -1) {
        asterror(tk->span, "Unexpected token: '%s', Expected operator [binary operator]\n", kit_token_type_to_string(tk->type));

        return -1;
      }

      kit_filespan prev_span = peek(p)->span;

      int right = kit_ast_expr(p, right_bp);
      if (right < 0) {
        asterror(prev_span, "Failed to evaluate RHS [binary operator]\n");

        return -1;
      }

      KIT_GET_NODE(p->ast, node)->type                 = KIT_AST_NODE_BINARYOP;
      KIT_GET_NODE(p->ast, node)->binaryop.op          = op;
      KIT_GET_NODE(p->ast, node)->binaryop.left        = leftidx;
      KIT_GET_NODE(p->ast, node)->binaryop.is_compound = tk->val.op.is_compound;
      KIT_GET_NODE(p->ast, node)->binaryop.right       = right;
      return node;
    }

    default: {
      asterror(tk->span, "Unexpected token: '%s'\n", kit_token_type_to_string(tk->type));

      return -1;
    }
  }
}

int
kit_parse(kit_parser* p)
{
  if (!p) return -1;

  int node = -1;

  while (peek(p)) {
    kit_filespan take_span = peek(p)->span;

    node = kit_ast_expr(p, 0);
    if (node < 0) {
      asterror(take_span, "Failed to parse expression [Root statement compilation failed, retreating]\n");
      goto ERR;
    }

    kit_ast_node_type type = KIT_GET_NODE(p->ast, node)->type;
    if (type != KIT_AST_NODE_FUNCTION_DEFINITION && type != KIT_AST_NODE_NAMESPACE_DECL && type != KIT_AST_NODE_STRUCT_DECL
        && type != KIT_AST_NODE_VARIABLE_DECL && type != KIT_AST_NODE_ASSERT) {
      // asterror(take_span, "Expected function definition or variable declerations in global scope\n");
      asterror(
          take_span,
          "Expected only function definitions, variable declerations, namespace declerations, struct declerations or assertions in global scope "
          "(%i)\n",
          type);
      kit_ast_node_free(p->ast, node);
      goto ERR;
    }

    if (!kit_ast_is_limiter_exempt(kit_ast_get_node(p->ast, node)->type) && kit_ast_expect(p, KIT_TOKEN_TYPE_SEMICOLON)) {
      asterror(prev(p)->span, "Expected semicolon after expression\n");
      kit_ast_node_free(p->ast, node);
      goto ERR;
    }

    int** pstmts  = &KIT_GET_NODE(p->ast, p->ast->root)->root.stmts;
    u32*  pnstmts = &KIT_GET_NODE(p->ast, p->ast->root)->root.nstmts;
    u32*  pcstmts = &KIT_GET_NODE(p->ast, p->ast->root)->root.capacity;

    if (*pnstmts >= *pcstmts) {
      u32  newcap   = MAX((*pcstmts) * 2, 1);
      int* newstmts = realloc(*pstmts, newcap * sizeof(int));
      if (!newstmts) {
        kit_ast_node_free(p->ast, node);
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
kit_ast_init(kit_str_interner* interner, kit_ast* ast)
{
  if (ast) {
    *ast = (kit_ast){
      .nodes    = kit_xalloc(init_node_capacity, sizeof(kit_ast_node)),
      .nnodes   = 0,
      .capacity = init_node_capacity,
      .interner = interner,
      .root     = -1,
    };
    if (!ast->nodes) return -1;

    // Initialize our single root node.
    ast->root = kit_ast_make_node(ast);
    if (ast->root < 0) return -1;

    ast->nodes[ast->root].type = KIT_AST_NODE_ROOT;
  }
  return 0;
}

void
kit_ast_free(kit_ast* ast)
{
  if (ast->root >= 0) kit_ast_node_free(ast, ast->root);
  free(ast->nodes);

  memset(ast, 0, sizeof *ast);
}

int
kit_parser_init(const kit_token* tokens, u32 ntokens, kit_ast* ast, kit_parser* parser)
{
  *parser = (kit_parser){
    .toks  = tokens,
    .ntoks = ntokens,
    .head  = 0,
    .ast   = ast,
  };
  return 0;
}

void
kit_parser_free(kit_parser* parser)
{ /* Didn't allocate anything. */
}
