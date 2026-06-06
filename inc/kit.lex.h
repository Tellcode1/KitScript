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

#ifndef KIT_LEX_H
#define KIT_LEX_H

#include "kit.cerr.h"
#include "kit.stdafx.h"
#include "kit.strint.h"

typedef enum kit_token_type {
  KIT_TOKEN_TYPE_SEMICOLON,
  KIT_TOKEN_TYPE_COLON,
  KIT_TOKEN_TYPE_DOUBLE_COLON,
  KIT_TOKEN_TYPE_COMMA,
  KIT_TOKEN_TYPE_DOT,
  KIT_TOKEN_TYPE_HASH_OPENBRACE, // #{
  KIT_TOKEN_TYPE_OPENBRACE,
  KIT_TOKEN_TYPE_CLOSEBRACE,
  KIT_TOKEN_TYPE_OPENBRACKET,
  KIT_TOKEN_TYPE_CLOSEBRACKET,
  KIT_TOKEN_TYPE_OPENPAREN,
  KIT_TOKEN_TYPE_CLOSEPAREN,

  KIT_TOKEN_TYPE_INT,
  KIT_TOKEN_TYPE_CHAR,
  KIT_TOKEN_TYPE_BOOL,
  KIT_TOKEN_TYPE_FLOAT,
  KIT_TOKEN_TYPE_STRING,

  KIT_TOKEN_TYPE_FN,     // fn keyword for functions, no value
  KIT_TOKEN_TYPE_EXTERN, // local function
  KIT_TOKEN_TYPE_STRUCT, // structure declerations.
  KIT_TOKEN_TYPE_LET,    // let keyword for variable declerations, no value.
  KIT_TOKEN_TYPE_DEFER,  // defer keyword
  KIT_TOKEN_TYPE_CONST,  // const qualifier
  KIT_TOKEN_TYPE_IF,
  KIT_TOKEN_TYPE_IN,
  KIT_TOKEN_TYPE_ELSE,
  KIT_TOKEN_TYPE_WHILE,
  KIT_TOKEN_TYPE_FOR,
  KIT_TOKEN_TYPE_BREAK,
  KIT_TOKEN_TYPE_CONTINUE,
  KIT_TOKEN_TYPE_RETURN,
  KIT_TOKEN_TYPE_NAMESPACE,
  KIT_TOKEN_TYPE_ASSERT,

  KIT_TOKEN_TYPE_PLUS,       // +
  KIT_TOKEN_TYPE_MINUS,      // -
  KIT_TOKEN_TYPE_MULTIPLY,   // *
  KIT_TOKEN_TYPE_DIVIDE,     // /
  KIT_TOKEN_TYPE_EXPONENT,   // ^
  KIT_TOKEN_TYPE_MOD,        // %
  KIT_TOKEN_TYPE_PLUSPLUS,   // ++
  KIT_TOKEN_TYPE_MINUSMINUS, // --

  KIT_TOKEN_TYPE_AND, // &&
  KIT_TOKEN_TYPE_OR,  // ||
  KIT_TOKEN_TYPE_NOT, // !

  KIT_TOKEN_TYPE_BAND, // &
  KIT_TOKEN_TYPE_BOR,  // |
  KIT_TOKEN_TYPE_XOR,  // ^
  KIT_TOKEN_TYPE_BNOT, // ~

  KIT_TOKEN_TYPE_EQUAL,
  KIT_TOKEN_TYPE_NOTEQUAL,
  KIT_TOKEN_TYPE_DOUBLEEQUAL,

  KIT_TOKEN_TYPE_LT,
  KIT_TOKEN_TYPE_LTE,
  KIT_TOKEN_TYPE_GT,
  KIT_TOKEN_TYPE_GTE,

  KIT_TOKEN_TYPE_IDENT, // identifier/name
} kit_token_type;

typedef union kit_token_val {
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
} kit_token_val;

typedef struct kit_token {
  kit_token_type type;
  kit_token_val  val;
  kit_filespan   span; // Span has interned string, do NOT FREE!
} kit_token;

/**
 * Perform lexical analysis / tokenization on the input string.
 * outtoks will be set to a an array of tokens, allocated on the heap. Free using kit_freetoks.
 * advertised_file is a string that should contain the file name that is shown to the user on errors/warnings by the compiler.
 */
int kit_tokenize(const char* input, const char* advertised_file, kit_str_interner* interner, kit_token** outtoks, u32* ntoks) RETURNS_ERRCODE;

void kit_freetoks(kit_token* toks, u32 ntoks);

static inline const char*
kit_token_type_to_string(kit_token_type e)
{
  switch (e) {
    case KIT_TOKEN_TYPE_SEMICOLON: return ";";
    case KIT_TOKEN_TYPE_COLON: return ":";
    case KIT_TOKEN_TYPE_COMMA: return ",";
    case KIT_TOKEN_TYPE_INT: return "int";
    case KIT_TOKEN_TYPE_FLOAT: return "float";
    case KIT_TOKEN_TYPE_STRING: return "string";
    case KIT_TOKEN_TYPE_FN: return "fn";
    case KIT_TOKEN_TYPE_IN: return "in";
    case KIT_TOKEN_TYPE_LET: return "let";
    case KIT_TOKEN_TYPE_PLUS: return "+";
    case KIT_TOKEN_TYPE_MINUS: return "-";
    case KIT_TOKEN_TYPE_MULTIPLY: return "*";
    case KIT_TOKEN_TYPE_DIVIDE: return "/";
    case KIT_TOKEN_TYPE_EXPONENT: return "**";
    case KIT_TOKEN_TYPE_MOD: return "%";
    case KIT_TOKEN_TYPE_HASH_OPENBRACE: return "#";
    case KIT_TOKEN_TYPE_AND: return "&&";
    case KIT_TOKEN_TYPE_OR: return "||";
    case KIT_TOKEN_TYPE_EQUAL: return "=";
    case KIT_TOKEN_TYPE_DOUBLEEQUAL: return "==";
    case KIT_TOKEN_TYPE_IDENT: return "Identifier";
    case KIT_TOKEN_TYPE_OPENBRACE: return "{";
    case KIT_TOKEN_TYPE_CLOSEBRACE: return "}";
    case KIT_TOKEN_TYPE_OPENPAREN: return "(";
    case KIT_TOKEN_TYPE_CLOSEPAREN: return ")";
    case KIT_TOKEN_TYPE_CONST: return "const";
    case KIT_TOKEN_TYPE_CHAR: return "char";
    case KIT_TOKEN_TYPE_BOOL: return "bool";
    case KIT_TOKEN_TYPE_STRUCT: return "struct";
    case KIT_TOKEN_TYPE_IF: return "if";
    case KIT_TOKEN_TYPE_ELSE: return "else";
    case KIT_TOKEN_TYPE_WHILE: return "while";
    case KIT_TOKEN_TYPE_ASSERT: return "assert";
    case KIT_TOKEN_TYPE_FOR: return "for";
    case KIT_TOKEN_TYPE_BREAK: return "break";
    case KIT_TOKEN_TYPE_CONTINUE: return "continue";
    case KIT_TOKEN_TYPE_RETURN: return "return";
    case KIT_TOKEN_TYPE_PLUSPLUS: return "++";
    case KIT_TOKEN_TYPE_MINUSMINUS: return "--";
    case KIT_TOKEN_TYPE_NOT: return "!";
    case KIT_TOKEN_TYPE_BNOT: return "~";
    case KIT_TOKEN_TYPE_NOTEQUAL: return "!=";
    case KIT_TOKEN_TYPE_LT: return "<";
    case KIT_TOKEN_TYPE_LTE: return "<=";
    case KIT_TOKEN_TYPE_GT: return ">";
    case KIT_TOKEN_TYPE_GTE: return ">=";
    case KIT_TOKEN_TYPE_DOT: return ".";
    case KIT_TOKEN_TYPE_OPENBRACKET: return "[";
    case KIT_TOKEN_TYPE_CLOSEBRACKET: return "]";
    case KIT_TOKEN_TYPE_EXTERN: return "extern";
    case KIT_TOKEN_TYPE_NAMESPACE: return "namespace";
    case KIT_TOKEN_TYPE_BAND: return "&";
    case KIT_TOKEN_TYPE_BOR: return "|";
    case KIT_TOKEN_TYPE_XOR: return "^";
    case KIT_TOKEN_TYPE_DOUBLE_COLON: return "::";
    case KIT_TOKEN_TYPE_DEFER: return "defer";
  }
  return "UNKNOWN";
}

#endif // KIT_LEX_H
