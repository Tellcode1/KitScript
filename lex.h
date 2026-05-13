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

#ifndef E_LEX_H
#define E_LEX_H

#include "cerr.h"
#include "stdafx.h"
#include "strint.h"

typedef enum e_token_type {
  E_TOKEN_TYPE_SEMICOLON,
  E_TOKEN_TYPE_COLON,
  E_TOKEN_TYPE_DOUBLE_COLON,
  E_TOKEN_TYPE_COMMA,
  E_TOKEN_TYPE_DOT,
  E_TOKEN_TYPE_HASH_OPENBRACE, // #{
  E_TOKEN_TYPE_OPENBRACE,
  E_TOKEN_TYPE_CLOSEBRACE,
  E_TOKEN_TYPE_OPENBRACKET,
  E_TOKEN_TYPE_CLOSEBRACKET,
  E_TOKEN_TYPE_OPENPAREN,
  E_TOKEN_TYPE_CLOSEPAREN,

  E_TOKEN_TYPE_INT,
  E_TOKEN_TYPE_CHAR,
  E_TOKEN_TYPE_BOOL,
  E_TOKEN_TYPE_FLOAT,
  E_TOKEN_TYPE_STRING,

  E_TOKEN_TYPE_FN,     // fn keyword for functions, no value
  E_TOKEN_TYPE_EXTERN, // local function
  E_TOKEN_TYPE_STRUCT, // structure declerations.
  E_TOKEN_TYPE_LET,    // let keyword for variable declerations, no value.
  E_TOKEN_TYPE_DEFER,  // defer keyword
  E_TOKEN_TYPE_CONST,  // const qualifier
  E_TOKEN_TYPE_IF,
  E_TOKEN_TYPE_ELSE,
  E_TOKEN_TYPE_WHILE,
  E_TOKEN_TYPE_FOR,
  E_TOKEN_TYPE_BREAK,
  E_TOKEN_TYPE_CONTINUE,
  E_TOKEN_TYPE_RETURN,
  E_TOKEN_TYPE_NAMESPACE,
  E_TOKEN_TYPE_ASSERT,

  E_TOKEN_TYPE_PLUS,       // +
  E_TOKEN_TYPE_MINUS,      // -
  E_TOKEN_TYPE_MULTIPLY,   // *
  E_TOKEN_TYPE_DIVIDE,     // /
  E_TOKEN_TYPE_EXPONENT,   // ^
  E_TOKEN_TYPE_MOD,        // %
  E_TOKEN_TYPE_PLUSPLUS,   // ++
  E_TOKEN_TYPE_MINUSMINUS, // --

  E_TOKEN_TYPE_AND, // &&
  E_TOKEN_TYPE_OR,  // ||
  E_TOKEN_TYPE_NOT, // !

  E_TOKEN_TYPE_BAND, // &
  E_TOKEN_TYPE_BOR,  // |
  E_TOKEN_TYPE_XOR,  // ^
  E_TOKEN_TYPE_BNOT, // ~

  E_TOKEN_TYPE_EQUAL,
  E_TOKEN_TYPE_NOTEQUAL,
  E_TOKEN_TYPE_DOUBLEEQUAL,

  E_TOKEN_TYPE_LT,
  E_TOKEN_TYPE_LTE,
  E_TOKEN_TYPE_GT,
  E_TOKEN_TYPE_GTE,

  E_TOKEN_TYPE_IDENT, // identifier/name
} e_token_type;

typedef union e_token_val {
  int    i;
  char   c;
  bool   b;
  double f;
  char*  s;
  char*  assertion_line;
  char*  ident;
  struct {
    char c;
    bool is_compound;
  } op;
} e_token_val;

typedef struct e_token {
  e_token_type type;
  e_token_val  val;
  e_filespan   span; // Span has interned string, do NOT FREE!
} e_token;

/**
 * Perform lexical analysis / tokenization on the input string.
 * outtoks will be set to a an array of tokens, allocated on the heap. Free using e_freetoks.
 * advertised_file is a string that should contain the file name that is shown to the user on errors/warnings by the compiler.
 */
int e_tokenize(const char* input, const char* advertised_file, e_str_interner* interner, e_token** outtoks, u32* ntoks) RETURNS_ERRCODE;

void e_freetoks(e_token* toks, u32 ntoks);

static inline const char*
e_token_type_to_string(e_token_type e)
{
  switch (e) {
    case E_TOKEN_TYPE_SEMICOLON: return ";";
    case E_TOKEN_TYPE_COLON: return ":";
    case E_TOKEN_TYPE_COMMA: return ",";
    case E_TOKEN_TYPE_INT: return "int";
    case E_TOKEN_TYPE_FLOAT: return "float";
    case E_TOKEN_TYPE_STRING: return "string";
    case E_TOKEN_TYPE_FN: return "fn";
    case E_TOKEN_TYPE_LET: return "let";
    case E_TOKEN_TYPE_PLUS: return "+";
    case E_TOKEN_TYPE_MINUS: return "-";
    case E_TOKEN_TYPE_MULTIPLY: return "*";
    case E_TOKEN_TYPE_DIVIDE: return "/";
    case E_TOKEN_TYPE_EXPONENT: return "**";
    case E_TOKEN_TYPE_MOD: return "%";
    case E_TOKEN_TYPE_HASH_OPENBRACE: return "#";
    case E_TOKEN_TYPE_AND: return "&&";
    case E_TOKEN_TYPE_OR: return "||";
    case E_TOKEN_TYPE_EQUAL: return "=";
    case E_TOKEN_TYPE_DOUBLEEQUAL: return "==";
    case E_TOKEN_TYPE_IDENT: return "Identifier";
    case E_TOKEN_TYPE_OPENBRACE: return "{";
    case E_TOKEN_TYPE_CLOSEBRACE: return "}";
    case E_TOKEN_TYPE_OPENPAREN: return "(";
    case E_TOKEN_TYPE_CLOSEPAREN: return ")";
    case E_TOKEN_TYPE_CONST: return "const";
    case E_TOKEN_TYPE_CHAR: return "char";
    case E_TOKEN_TYPE_BOOL: return "bool";
    case E_TOKEN_TYPE_STRUCT: return "struct";
    case E_TOKEN_TYPE_IF: return "if";
    case E_TOKEN_TYPE_ELSE: return "else";
    case E_TOKEN_TYPE_WHILE: return "while";
    case E_TOKEN_TYPE_ASSERT: return "assert";
    case E_TOKEN_TYPE_FOR: return "for";
    case E_TOKEN_TYPE_BREAK: return "break";
    case E_TOKEN_TYPE_CONTINUE: return "continue";
    case E_TOKEN_TYPE_RETURN: return "return";
    case E_TOKEN_TYPE_PLUSPLUS: return "++";
    case E_TOKEN_TYPE_MINUSMINUS: return "--";
    case E_TOKEN_TYPE_NOT: return "!";
    case E_TOKEN_TYPE_BNOT: return "~";
    case E_TOKEN_TYPE_NOTEQUAL: return "!=";
    case E_TOKEN_TYPE_LT: return "<";
    case E_TOKEN_TYPE_LTE: return "<=";
    case E_TOKEN_TYPE_GT: return ">";
    case E_TOKEN_TYPE_GTE: return ">=";
    case E_TOKEN_TYPE_DOT: return ".";
    case E_TOKEN_TYPE_OPENBRACKET: return "[";
    case E_TOKEN_TYPE_CLOSEBRACKET: return "]";
    case E_TOKEN_TYPE_EXTERN: return "extern";
    case E_TOKEN_TYPE_NAMESPACE: return "namespace";
    case E_TOKEN_TYPE_BAND: return "&";
    case E_TOKEN_TYPE_BOR: return "|";
    case E_TOKEN_TYPE_XOR: return "^";
    case E_TOKEN_TYPE_DOUBLE_COLON: return "::";
    case E_TOKEN_TYPE_DEFER: return "defer";
  }
  return "UNKNOWN";
}

#endif // E_LEX_H
