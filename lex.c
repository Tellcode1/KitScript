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

#include "lex.h"

#include "cerr.h"
#include "cvt.h"
#include "stdafx.h"
#include "strint.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if E_LEX_NO_SPAN
#  define SPAN (e_filespan){ 0 }
#else
#  define SPAN span
#endif

static inline void
internal_advance(const char** s, int* line, int* col)
{
  if (**s == '\n') {
    (*line)++;
    (*col) = 1;
  } else if (strcmp(*s, "\r\n") == 0) {
    (*line)++;
    (*line)++;
    (*col) = 1;
  } else {
    (*col)++;
  }
  (*s)++;
}

/**
 * Walk through the string and parse all backslash escape sequences.
 */
static inline char*
parse_backslash_sequences(const char* s, size_t max)
{
  char*  news = e_xalloc(1, max + 1);
  size_t r    = 0;
  size_t w    = 0;

  while (s[r] && r < max) {
    if (s[r] != '\\') {
      news[w++] = s[r++];
      continue;
    }
    r++; // skip backslash

    if (r >= max) break;

    switch (s[r]) {
      case '\'': news[w] = '\''; break;
      case '\"': news[w] = '\"'; break;
      case '\\': news[w] = '\\'; break;
      case 'n': news[w] = '\n'; break;
      case 't': news[w] = '\t'; break;
      case '0': news[w] = '\0'; break;
      default:
        fprintf(stderr, "Unrecognized escape sequence\n");
        r++;
        continue;
    }
    r++;
    w++;
  }
  news[w] = 0;

  return news;
}

#define advance(s, line, col) internal_advance((&(s)), &(line), &(col))

struct tklist {
  e_token* toks;
  u32      ntoks;
  u32      capacity;
};

static inline int
tklist_init(u32 capacity, struct tklist* list)
{
  if (capacity <= 0) { capacity = 1; }

  list->ntoks = 0;
  list->toks  = e_xalloc(capacity, sizeof(e_token));
  if (!list->toks) { return -1; }

  list->capacity = capacity;

  return 0;
}

static inline int
tklist_resize(struct tklist* toks, u32 newcap)
{
  if (newcap == 0) { newcap = 1; }

  e_token* newtoks = realloc(toks->toks, newcap * sizeof(e_token));
  if (!newtoks) { return -1; }

  toks->toks     = newtoks;
  toks->capacity = newcap;

  return 0;
}

static inline void
tklist_append(struct tklist* toks, const e_token* tk)
{
  if (toks->ntoks + 1 >= toks->capacity) {
    if (tklist_resize(toks, MAX(toks->capacity * 2, 1))) { return; }
  }

  memcpy(&toks->toks[toks->ntoks++], tk, sizeof(e_token));
}

#define collectspan                                                                                                                                  \
  (struct e_filespan) { .file = advertised_file, .line = line, .col = col }

static inline void
internal_lexerror(const char* scriptfile, int script_line, int script_col, const char* this_file, int this_line, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  fprintf(stderr, "[%s:%i] ", this_file, this_line);
  fprintf(stderr, "[%s:%i:%i] ", scriptfile, script_line, script_col);
  vfprintf(stderr, fmt, ap);

  va_end(ap);
}
#define lexerror(line, col, ...) internal_lexerror(advertised_file, line, col, __FILE__, __LINE__, __VA_ARGS__)

/**
 * strndup is POSIX extension.
 */
static inline char*
internal_strndup(const char* s, size_t n)
{
  size_t l  = strlen(s);
  size_t cp = MIN(l, n);

  char* new = e_xalloc(1, cp + 1);
  strncpy(new, s, cp);
  new[cp] = 0;

  return new;
}

int
e_tokenize(const char* input, const char* advertised_file, e_str_interner* interner, e_token** outtoks, u32* ntoks)
{
  int         e = 0;
  const char* s = input;

  int line = 1;
  int col  = 1;

  struct tklist toks;
  e = tklist_init(128, &toks);
  if (e) { return e; }

  const char* interned_filename = e_str_intern(advertised_file, interner);

  while (*s) {
    while (isspace(*s) || *s == '\n' || *s == '\r') { advance(s, line, col); }

    if (s[0] && s[0] == '/' && s[1] == '/') {
      advance(s, line, col); // skip over comment start
      advance(s, line, col);
      while (*s && *s != '\n' && *s != '\r') { advance(s, line, col); }
      continue;
    }
    if (s[0] == '/' && s[1] == '*') {
      advance(s, line, col); // /*
      advance(s, line, col);

      while (*s && !(s[0] == '*' && s[1] == '/')) { advance(s, line, col); }

      if (*s) {
        advance(s, line, col); // */
        advance(s, line, col);
      } else {
        lexerror(line, col, "Unterminated multiline comment\n");
        goto err;
      }

      continue;
    }

    if (!*s) break;

#if !E_LEX_NO_SPAN
    const struct e_filespan span = { .file = interned_filename, .line = line, .col = col };
#endif
    if (isdigit(*s)) {
      char* end = NULL;

      double f = strtod(s, &end);

      if (end == s) {
        lexerror(span.line, span.col, "Invalid numeric literal\n");
        goto err;
      }

      bool is_float = false;
      for (const char* p = s; p < end; p++) {
        if (*p == '.' || *p == 'e' || *p == 'E') {
          is_float = true;
          break;
        }
      }

      if (is_float) {
        e_token tk = { .type = E_TOKEN_TYPE_FLOAT, .val.f = f, .span = SPAN };
        tklist_append(&toks, &tk);
      } else {
        int i = 0;

        e_cvt_err err = e_cvt_int(s, NULL, &i);
        if (err != E_CVT_OK) {
          switch (err) {
            case E_CVT_ERROR_MALFORMED_INPUT: lexerror(span.line, span.col, "Malformed integer literal\n"); return -1;
            case E_CVT_ERROR_OVERFLOW: lexerror(span.line, span.col, "Can not represent integer in 32 bits: will overflow\n"); return -1;
            case E_CVT_ERROR_UNDERFLOW: lexerror(span.line, span.col, "Can not represent integer in 32 bits: will underflow\n"); return -1;
            case E_CVT_ERROR_EOF: return -1;
            case E_CVT_OK: break;
          }
        }

        e_token tk = { .type = E_TOKEN_TYPE_INT, .val.i = i, .span = SPAN };
        tklist_append(&toks, &tk);
      }

      // Advance correctly
      while (s < end) { advance(s, line, col); }
    } else if (isalpha(*s) || *s == '_') {
      const char* snap = s;
      while (isalpha(*s) || isdigit(*s) || *s == '_') { advance(s, line, col); }

      u32 len = (u32)(s - snap);

      e_token tk = { 0 };
      if (len == strlen("fn") && strncmp(snap, "fn", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_FN, .span = SPAN };
      } else if (len == strlen("let") && strncmp(snap, "let", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_LET, .span = SPAN };
      } else if (len == strlen("defer") && strncmp(snap, "defer", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_DEFER, .span = SPAN };
      } else if (len == strlen("const") && strncmp(snap, "const", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_CONST, .span = SPAN };
      } else if (len == strlen("true") && strncmp(snap, "true", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_BOOL, .val.b = true, .span = SPAN };
      } else if (len == strlen("false") && strncmp(snap, "false", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_BOOL, .val.b = false, .span = SPAN };
      } else if (len == strlen("if") && strncmp(snap, "if", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_IF, .span = SPAN };
      } else if (len == strlen("else") && strncmp(snap, "else", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_ELSE, .span = SPAN };
      } else if (len == strlen("while") && strncmp(snap, "while", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_WHILE, .span = SPAN };
      } else if (len == strlen("for") && strncmp(snap, "for", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_FOR, .span = SPAN };
      } else if (len == strlen("break") && strncmp(snap, "break", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_BREAK, .span = SPAN };
      } else if (len == strlen("continue") && strncmp(snap, "continue", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_CONTINUE, .span = SPAN };
      } else if (len == strlen("return") && strncmp(snap, "return", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_RETURN, .span = SPAN };
      } else if (len == strlen("namespace") && strncmp(snap, "namespace", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_NAMESPACE, .span = SPAN };
      } else if (len == strlen("extern") && strncmp(snap, "extern", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_EXTERN, .span = SPAN };
      } else if (len == strlen("struct") && strncmp(snap, "struct", len) == 0) {
        tk = (e_token){ .type = E_TOKEN_TYPE_STRUCT, .span = SPAN };
      } else if (len == strlen("assert") && strncmp(snap, "assert", len) == 0) {
        // Special case. Copy everything after the assert token until a semicolon.
        // So the VM can print errors like 'e == 0: Assertion failed".
        // Keep in mind we don't advance the actual head here, just a lookahead to get the statement
        const char* head = s; // skip spaces
        const char* leg  = head;
        while (isspace(*head)) {
          head++;
          leg++;
        }
        while (*leg && *leg != ';') leg++;

        // Simplify bracketed expressions
        if (*head == '(' && *leg == ';' && *(leg - 1) == ')') { // leg is at ;, and previous character is closing paren
          head++;
          leg--;
        }

        tk = (e_token){ .type = E_TOKEN_TYPE_ASSERT, .val.assertion_line = internal_strndup(head, leg - head), .span = SPAN };
      } else {
        tk = (e_token){ .type = E_TOKEN_TYPE_IDENT, .val.ident = internal_strndup(snap, len), .span = SPAN };
      }
      tklist_append(&toks, &tk);
    } else if (*s == '"') {
      const int line_snap = line;
      const int col_snap  = col;

      advance(s, line, col);

      const char* snap = s;
      while (*s && *s != '"' && *s != '\n' && *s != '\r') {
        if (*s == '\\') advance(s, line, col);
        advance(s, line, col);
      }

      // reached end of file
      if (!*s || *s == '\n' || *s == '\r') {
        lexerror(line_snap, col_snap, "Unterminated string literal\n");
        goto err;
      }

      u32 len = (u32)(s - snap);

      char* parsed = parse_backslash_sequences(snap, len);

      e_token tk = (e_token){ .type = E_TOKEN_TYPE_STRING, .val.s = parsed, .span = SPAN };
      tklist_append(&toks, &tk);

      advance(s, line, col);
    }

    else if (*s == '\'') {
      advance(s, line, col);

      char ch = -1;
      if (*s != '\\') {
        ch = *s;
        advance(s, line, col);
      } else {
        advance(s, line, col);
        switch (*s) {
          case 'n': ch = '\n'; break;
          case 't': ch = '\t'; break;
          case 'r': ch = '\r'; break;
          case 'b': ch = '\b'; break;
          case '\\': ch = '\\'; break;
          case '0': ch = '\0'; break;
          default: lexerror(line, col, "Expected one of [n,r,b,t,0]. Invalid backslash escaped sequence\n"); goto err;
        }
        advance(s, line, col);
      }

      if (*s != '\'') {
        lexerror(line, col, "Expected closing quote after character literal\n");
        goto err;
      }

      advance(s, line, col);

      e_token tk = (e_token){ .type = E_TOKEN_TYPE_CHAR, .val.c = ch, .span = SPAN };
      tklist_append(&toks, &tk);
    } else {
      const char   ch = *s;
      e_token_type type;
      bool         is_compound = false;

      if (s[1] == '=') // non compound
      {
        // clang-format off
        switch (*s) {
          case '+': type = E_TOKEN_TYPE_PLUS; is_compound = true; break;
          case '-': type = E_TOKEN_TYPE_MINUS; is_compound = true; break;
          case '*': type = E_TOKEN_TYPE_MULTIPLY; is_compound = true; break;
          case '/': type = E_TOKEN_TYPE_DIVIDE; is_compound = true; break;
          case '%': type = E_TOKEN_TYPE_MOD; is_compound = true; break;
          case '~': type = E_TOKEN_TYPE_BNOT; is_compound = true; break;
          case '|': type = E_TOKEN_TYPE_BOR; is_compound = true; break;
          case '^': type = E_TOKEN_TYPE_XOR; is_compound = true; break;
          case '&': type = E_TOKEN_TYPE_BAND; is_compound = true; break;
          
          case '=': type = E_TOKEN_TYPE_DOUBLEEQUAL; break;
          case '!': type = E_TOKEN_TYPE_NOTEQUAL; break;
          case '<': type = E_TOKEN_TYPE_LTE; break;
          case '>': type = E_TOKEN_TYPE_GTE; break;
          default: lexerror(line, col, "Unrecognized sequence or character\n"); goto err;
        }
        // clang-format on
        advance(s, line, col);
        advance(s, line, col);
      } else if (s[0] == '+' && s[1] == '+') {
        type        = E_TOKEN_TYPE_PLUSPLUS;
        is_compound = true;
        advance(s, line, col);
        advance(s, line, col);
      } else if (s[0] == '-' && s[1] == '-') {
        type        = E_TOKEN_TYPE_MINUSMINUS;
        is_compound = true;
        advance(s, line, col);
        advance(s, line, col);
      } else if (s[0] == '*' && s[1] == '*') {
        type        = E_TOKEN_TYPE_EXPONENT;
        is_compound = false;
        advance(s, line, col);
        advance(s, line, col);
      } else if (s[0] == ':' && s[1] == ':') {
        type        = E_TOKEN_TYPE_DOUBLE_COLON;
        is_compound = false;
        advance(s, line, col);
        advance(s, line, col);
      } else if (s[0] == '&' && s[1] == '&') {
        type        = E_TOKEN_TYPE_AND;
        is_compound = false;
        advance(s, line, col);
        advance(s, line, col);
      } else if (s[0] == '|' && s[1] == '|') {
        type        = E_TOKEN_TYPE_OR;
        is_compound = false;
        advance(s, line, col);
        advance(s, line, col);
      } else if (s[0] == '#' && s[1] == '{') {
        type        = E_TOKEN_TYPE_HASH_OPENBRACE;
        is_compound = false;
        advance(s, line, col);
        advance(s, line, col);
      } else {
        switch (*s) {
          case '+': type = E_TOKEN_TYPE_PLUS; break;
          case '-': type = E_TOKEN_TYPE_MINUS; break;
          case '*': type = E_TOKEN_TYPE_MULTIPLY; break;
          case '/': type = E_TOKEN_TYPE_DIVIDE; break;
          case '%': type = E_TOKEN_TYPE_MOD; break;
          case '=': type = E_TOKEN_TYPE_EQUAL; break;
          case '~': type = E_TOKEN_TYPE_BNOT; break;
          case '|': type = E_TOKEN_TYPE_BOR; break;
          case '^': type = E_TOKEN_TYPE_XOR; break;
          case '&': type = E_TOKEN_TYPE_BAND; break;
          case '!': type = E_TOKEN_TYPE_NOT; break;
          case '<': type = E_TOKEN_TYPE_LT; break;
          case '>': type = E_TOKEN_TYPE_GT; break;

          case ';': type = E_TOKEN_TYPE_SEMICOLON; break;
          case ':': type = E_TOKEN_TYPE_COLON; break;
          case ',': type = E_TOKEN_TYPE_COMMA; break;
          case '.': type = E_TOKEN_TYPE_DOT; break;
          case '{': type = E_TOKEN_TYPE_OPENBRACE; break;
          case '}': type = E_TOKEN_TYPE_CLOSEBRACE; break;
          case '[': type = E_TOKEN_TYPE_OPENBRACKET; break;
          case ']': type = E_TOKEN_TYPE_CLOSEBRACKET; break;
          case '(': type = E_TOKEN_TYPE_OPENPAREN; break;
          case ')': type = E_TOKEN_TYPE_CLOSEPAREN; break;

          default: lexerror(line, col, "Unrecognized sequence or character\n"); goto err;
        }
        advance(s, line, col);
      }
      e_token tk = { .type = type, .val.op = { .c = ch, .is_compound = is_compound }, .span = SPAN };
      tklist_append(&toks, &tk);
    }

    continue;

  err:
    e_freetoks(toks.toks, toks.ntoks);
    return -1;
  }

  if (outtoks) { *outtoks = toks.toks; }
  if (ntoks) { *ntoks = toks.ntoks; }

  return 0;
}

void
e_freetoks(e_token* toks, u32 ntoks)
{
  for (u32 i = 0; i < ntoks; i++) {
    if (toks[i].type == E_TOKEN_TYPE_STRING || toks[i].type == E_TOKEN_TYPE_IDENT) {
      free(toks[i].val.s);
    } else if (toks[i].type == E_TOKEN_TYPE_ASSERT) {
      free(toks[i].val.assertion_line);
    }
  }
  free(toks);
}