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

#ifndef E_BUILTIN_VARIABLES_H
#define E_BUILTIN_VARIABLES_H

// #include "lex.h"
#include "var.h"

#include <float.h>
#include <stdint.h>

/**
 * Builtin variables structure.
 * All variables within are loaded by the compiler ONCE at
 * start of initialization.
 * Any occurence of these variables are replaced with their values.
 */
typedef struct e_builtin_var {
  const char* name;
  e_vartype   type;
  e_varval    value;
} e_builtin_var;

typedef enum eb_io_constants {
  EB_IO_UNKNOWN      = -1,
  EB_IO_STDOUT       = 0,
  EB_IO_STDIN        = 2,
  EB_IO_STDERR       = 3,
  EB_IO_REL_TO_START = 4,
  EB_IO_REL_TO_CURR  = 5,
  EB_IO_REL_TO_END   = 6,
  EB_IO_FILE         = 7,
  EB_IO_LINK         = 8,
  EB_IO_DIRECTORY    = 9,
} eb_io_constants;

/**
 * List is not checked for conflicting entries!
 * All variables must have constant, compile time (literal) values.
 */
static const e_builtin_var eb_vars[] = {
  /**
   * Special null value, only needs the type tag.
   */
  { .name = "null", .type = E_VARTYPE_NULL, .value = { 0 } },

  { .name = "math::PI", .type = E_VARTYPE_FLOAT, .value = { .f = 3.14159265358979323846 } },
  { .name = "math::PIBY2", .type = E_VARTYPE_FLOAT, .value = { .f = 1.57079632679489661923 } },
  { .name = "math::PIBY4", .type = E_VARTYPE_FLOAT, .value = { .f = 0.78539816339744830962 } },
  { .name = "math::PIX2", .type = E_VARTYPE_FLOAT, .value = { .f = 3.14159265358979323846 * 2.0 } },
  { .name = "math::TAU", .type = E_VARTYPE_FLOAT, .value = { .f = 3.14159265358979323846 * 2.0 } },
  { .name = "math::PHI", .type = E_VARTYPE_FLOAT, .value = { .f = 1.6180339887498948 } }, // golden ratio
  { .name = "math::e", .type = E_VARTYPE_FLOAT, .value = { .f = 2.7182818284590452354 } },
  { .name = "math::ROOT2", .type = E_VARTYPE_FLOAT, .value = { .f = 1.4142135623730950 } },
  { .name = "math::ROOT3", .type = E_VARTYPE_FLOAT, .value = { .f = 1.7320508075688772 } },
  { .name = "math::ROOT5", .type = E_VARTYPE_FLOAT, .value = { .f = 2.2360679774997896 } },
  { .name = "math::LN2", .type = E_VARTYPE_FLOAT, .value = { .f = 0.69314718055994530942 } },
  { .name = "math::LN10", .type = E_VARTYPE_FLOAT, .value = { .f = 2.30258509299404568402 } },

  { .name = "int::MAX", .type = E_VARTYPE_INT, .value = { .i = INT32_MAX } },
  { .name = "int::MIN", .type = E_VARTYPE_INT, .value = { .i = INT32_MIN } },

  { .name = "float::MAX", .type = E_VARTYPE_FLOAT, .value = { .f = DBL_MAX } },
  { .name = "float::MIN", .type = E_VARTYPE_FLOAT, .value = { .f = DBL_MIN } },

  { .name = "vec2::ZERO", .type = E_VARTYPE_VEC2, .value = { .vec2 = { 0.0, 0.0 } } },
  { .name = "vec2::ONE", .type = E_VARTYPE_VEC2, .value = { .vec2 = { 1.0, 1.0 } } },

  { .name = "vec3::ZERO", .type = E_VARTYPE_VEC3, .value = { .vec3 = { 0.0, 0.0, 0.0 } } },
  { .name = "vec3::ONE", .type = E_VARTYPE_VEC3, .value = { .vec3 = { 1.0, 1.0, 1.0 } } },

  { .name = "vec4::ZERO", .type = E_VARTYPE_VEC4, .value = { .vec4 = { 0.0, 0.0, 0.0, 0.0 } } },
  { .name = "vec4::ONE", .type = E_VARTYPE_VEC4, .value = { .vec4 = { 1.0, 1.0, 1.0, 1.0 } } },

  { .name = "io::STDOUT", .type = E_VARTYPE_INT, .value = { .i = EB_IO_STDOUT } },
  { .name = "io::STDIN", .type = E_VARTYPE_INT, .value = { .i = EB_IO_STDIN } },
  { .name = "io::STDERR", .type = E_VARTYPE_INT, .value = { .i = EB_IO_STDERR } },
  { .name = "io::REL_TO_START", .type = E_VARTYPE_INT, .value = { .i = EB_IO_REL_TO_START } },
  { .name = "io::REL_TO_CURR", .type = E_VARTYPE_INT, .value = { .i = EB_IO_REL_TO_CURR } },
  { .name = "io::REL_TO_END", .type = E_VARTYPE_INT, .value = { .i = EB_IO_REL_TO_END } },
  { .name = "io::FILE", .type = E_VARTYPE_INT, .value = { .i = EB_IO_FILE } },
  { .name = "io::LINK", .type = E_VARTYPE_INT, .value = { .i = EB_IO_LINK } },
  { .name = "io::DIRECTORY", .type = E_VARTYPE_INT, .value = { .i = EB_IO_DIRECTORY } },
  { .name = "io::UNKNOWN", .type = E_VARTYPE_INT, .value = { .i = EB_IO_UNKNOWN } },

  { .name = "type::NULL", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_NULL } },
  { .name = "type::VOID", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_VOID } },
  { .name = "type::INT", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_INT } },
  { .name = "type::BOOL", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_BOOL } },
  { .name = "type::CHAR", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_CHAR } },
  { .name = "type::FLOAT", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_FLOAT } },
  { .name = "type::STRING", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_STRING } },
  { .name = "type::LIST", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_LIST } },
  { .name = "type::MAP", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_MAP } },
  { .name = "type::STRUCT", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_STRUCT } },
  { .name = "type::VEC2", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_VEC2 } },
  { .name = "type::VEC3", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_VEC3 } },
  { .name = "type::VEC4", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_VEC4 } },
  { .name = "type::MAT3", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_MAT3 } },
  { .name = "type::MAT4", .type = E_VARTYPE_INT, .value = { .i = E_VARTYPE_MAT4 } },

  // { .name = "jit::token::type::SEMICOLON", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_SEMICOLON } },
  // { .name = "jit::token::type::COLON", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_COLON } },
  // { .name = "jit::token::type::DOUBLE_COLON", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_DOUBLE_COLON } },
  // { .name = "jit::token::type::COMMA", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_COMMA } },
  // { .name = "jit::token::type::DOT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_DOT } },
  // { .name = "jit::token::type::HASH_OPENBRACE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_HASH_OPENBRACE } },
  // { .name = "jit::token::type::OPENBRACE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_OPENBRACE } },
  // { .name = "jit::token::type::CLOSEBRACE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_CLOSEBRACE } },
  // { .name = "jit::token::type::OPENBRACKET", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_OPENBRACKET } },
  // { .name = "jit::token::type::CLOSEBRACKET", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_CLOSEBRACKET } },
  // { .name = "jit::token::type::OPENPAREN", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_OPENPAREN } },
  // { .name = "jit::token::type::CLOSEPAREN", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_CLOSEPAREN } },
  // { .name = "jit::token::type::INT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_INT } },
  // { .name = "jit::token::type::CHAR", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_CHAR } },
  // { .name = "jit::token::type::BOOL", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_BOOL } },
  // { .name = "jit::token::type::FLOAT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_FLOAT } },
  // { .name = "jit::token::type::STRING", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_STRING } },
  // { .name = "jit::token::type::FN", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_FN } },
  // { .name = "jit::token::type::EXTERN", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_EXTERN } },
  // { .name = "jit::token::type::STRUCT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_STRUCT } },
  // { .name = "jit::token::type::LET", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_LET } },
  // { .name = "jit::token::type::DEFER", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_DEFER } },
  // { .name = "jit::token::type::CONST", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_CONST } },
  // { .name = "jit::token::type::IF", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_IF } },
  // { .name = "jit::token::type::ELSE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_ELSE } },
  // { .name = "jit::token::type::WHILE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_WHILE } },
  // { .name = "jit::token::type::FOR", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_FOR } },
  // { .name = "jit::token::type::BREAK", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_BREAK } },
  // { .name = "jit::token::type::CONTINUE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_CONTINUE } },
  // { .name = "jit::token::type::RETURN", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_RETURN } },
  // { .name = "jit::token::type::NAMESPACE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_NAMESPACE } },
  // { .name = "jit::token::type::ASSERT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_ASSERT } },
  // { .name = "jit::token::type::PLUS", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_PLUS } },
  // { .name = "jit::token::type::MINUS", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_MINUS } },
  // { .name = "jit::token::type::MULTIPLY", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_MULTIPLY } },
  // { .name = "jit::token::type::DIVIDE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_DIVIDE } },
  // { .name = "jit::token::type::EXPONENT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_EXPONENT } },
  // { .name = "jit::token::type::MOD", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_MOD } },
  // { .name = "jit::token::type::PLUSPLUS", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_PLUSPLUS } },
  // { .name = "jit::token::type::MINUSMINUS", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_MINUSMINUS } },
  // { .name = "jit::token::type::AND", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_AND } },
  // { .name = "jit::token::type::OR", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_OR } },
  // { .name = "jit::token::type::NOT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_NOT } },
  // { .name = "jit::token::type::BAND", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_BAND } },
  // { .name = "jit::token::type::BOR", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_BOR } },
  // { .name = "jit::token::type::XOR", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_XOR } },
  // { .name = "jit::token::type::BNOT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_BNOT } },
  // { .name = "jit::token::type::EQUAL", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_EQUAL } },
  // { .name = "jit::token::type::NOTEQUAL", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_NOTEQUAL } },
  // { .name = "jit::token::type::DOUBLEEQUAL", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_DOUBLEEQUAL } },
  // { .name = "jit::token::type::LT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_LT } },
  // { .name = "jit::token::type::LTE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_LTE } },
  // { .name = "jit::token::type::GT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_GT } },
  // { .name = "jit::token::type::GTE", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_GTE } },
  // { .name = "jit::token::type::IDENT", .type = E_VARTYPE_INT, .value = { .i = E_TOKEN_TYPE_IDENT } },
};

#endif // E_BUILTIN_VARIABLES_H