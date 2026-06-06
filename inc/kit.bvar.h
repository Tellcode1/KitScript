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

#ifndef KIT_BUILTIN_VARIABLES_H
#define KIT_BUILTIN_VARIABLES_H

// #include "kit.lex.h"
#include "kit.var.h"

#include <float.h>
#include <stdint.h>

/**
 * Builtin variables structure.
 * All variables within are loaded by the compiler ONCE at
 * start of initialization.
 * Any occurence of these variables are replaced with their values.
 */
typedef struct kit_builtin_var {
  const char* name;
  kit_vartype type;
  kit_varval  value;
} kit_builtin_var;

typedef enum kit_builtins_io_constants {
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
} kit_builtins_io_constants;

/**
 * List is not checked for conflicting entries!
 * All variables must have constant, compile time (literal) values.
 */
static const kit_builtin_var kit_builtins_vars[] = {
  /**
   * Special null value, only needs the type tag.
   */
  { .name = "null", .type = KIT_VARTYPE_NULL, .value = { 0 } },

  { .name = "math::PI", .type = KIT_VARTYPE_FLOAT, .value = { .f = 3.14159265358979323846 } },
  { .name = "math::PIBY2", .type = KIT_VARTYPE_FLOAT, .value = { .f = 1.57079632679489661923 } },
  { .name = "math::PIBY4", .type = KIT_VARTYPE_FLOAT, .value = { .f = 0.78539816339744830962 } },
  { .name = "math::PIX2", .type = KIT_VARTYPE_FLOAT, .value = { .f = 3.14159265358979323846 * 2.0 } },
  { .name = "math::TAU", .type = KIT_VARTYPE_FLOAT, .value = { .f = 3.14159265358979323846 * 2.0 } },
  { .name = "math::PHI", .type = KIT_VARTYPE_FLOAT, .value = { .f = 1.6180339887498948 } }, // golden ratio
  { .name = "math::e", .type = KIT_VARTYPE_FLOAT, .value = { .f = 2.7182818284590452354 } },
  { .name = "math::ROOT2", .type = KIT_VARTYPE_FLOAT, .value = { .f = 1.4142135623730950 } },
  { .name = "math::ROOT3", .type = KIT_VARTYPE_FLOAT, .value = { .f = 1.7320508075688772 } },
  { .name = "math::ROOT5", .type = KIT_VARTYPE_FLOAT, .value = { .f = 2.2360679774997896 } },
  { .name = "math::LN2", .type = KIT_VARTYPE_FLOAT, .value = { .f = 0.69314718055994530942 } },
  { .name = "math::LN10", .type = KIT_VARTYPE_FLOAT, .value = { .f = 2.30258509299404568402 } },

  { .name = "int::MAX", .type = KIT_VARTYPE_INT, .value = { .i = INT32_MAX } },
  { .name = "int::MIN", .type = KIT_VARTYPE_INT, .value = { .i = INT32_MIN } },

  { .name = "float::MAX", .type = KIT_VARTYPE_FLOAT, .value = { .f = DBL_MAX } },
  { .name = "float::MIN", .type = KIT_VARTYPE_FLOAT, .value = { .f = DBL_MIN } },
  { .name = "float::EPSILON", .type = KIT_VARTYPE_FLOAT, .value = { .f = DBL_EPSILON } },

  { .name = "vec2::ZERO", .type = KIT_VARTYPE_VEC2, .value = { .vec2 = { 0.0, 0.0 } } },
  { .name = "vec2::ONE", .type = KIT_VARTYPE_VEC2, .value = { .vec2 = { 1.0, 1.0 } } },

  { .name = "vec3::ZERO", .type = KIT_VARTYPE_VEC3, .value = { .vec3 = { 0.0, 0.0, 0.0 } } },
  { .name = "vec3::ONE", .type = KIT_VARTYPE_VEC3, .value = { .vec3 = { 1.0, 1.0, 1.0 } } },

  { .name = "vec4::ZERO", .type = KIT_VARTYPE_VEC4, .value = { .vec4 = { 0.0, 0.0, 0.0, 0.0 } } },
  { .name = "vec4::ONE", .type = KIT_VARTYPE_VEC4, .value = { .vec4 = { 1.0, 1.0, 1.0, 1.0 } } },

  { .name = "io::STDOUT", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_STDOUT } },
  { .name = "io::STDIN", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_STDIN } },
  { .name = "io::STDERR", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_STDERR } },
  { .name = "io::REL_TO_START", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_REL_TO_START } },
  { .name = "io::REL_TO_CURR", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_REL_TO_CURR } },
  { .name = "io::REL_TO_END", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_REL_TO_END } },
  { .name = "io::FILE", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_FILE } },
  { .name = "io::LINK", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_LINK } },
  { .name = "io::DIRECTORY", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_DIRECTORY } },
  { .name = "io::UNKNOWN", .type = KIT_VARTYPE_INT, .value = { .i = EB_IO_UNKNOWN } },

  { .name = "type::NULL", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_NULL } },
  { .name = "type::INT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_INT } },
  { .name = "type::BOOL", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_BOOL } },
  { .name = "type::CHAR", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_CHAR } },
  { .name = "type::FLOAT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_FLOAT } },
  { .name = "type::STRING", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_STRING } },
  { .name = "type::LIST", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_LIST } },
  { .name = "type::MAP", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_MAP } },
  { .name = "type::STRUCT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_STRUCT } },
  { .name = "type::VEC2", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_VEC2 } },
  { .name = "type::VEC3", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_VEC3 } },
  { .name = "type::VEC4", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_VEC4 } },
  { .name = "type::MAT3", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_MAT3 } },
  { .name = "type::MAT4", .type = KIT_VARTYPE_INT, .value = { .i = KIT_VARTYPE_MAT4 } },

  // { .name = "kit::token::type::SEMICOLON", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_SEMICOLON } },
  // { .name = "kit::token::type::COLON", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_COLON } },
  // { .name = "kit::token::type::DOUBLE_COLON", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_DOUBLE_COLON } },
  // { .name = "kit::token::type::COMMA", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_COMMA } },
  // { .name = "kit::token::type::DOT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_DOT } },
  // { .name = "kit::token::type::HASH_OPENBRACE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_HASH_OPENBRACE } },
  // { .name = "kit::token::type::OPENBRACE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_OPENBRACE } },
  // { .name = "kit::token::type::CLOSEBRACE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_CLOSEBRACE } },
  // { .name = "kit::token::type::OPENBRACKET", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_OPENBRACKET } },
  // { .name = "kit::token::type::CLOSEBRACKET", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_CLOSEBRACKET } },
  // { .name = "kit::token::type::OPENPAREN", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_OPENPAREN } },
  // { .name = "kit::token::type::CLOSEPAREN", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_CLOSEPAREN } },
  // { .name = "kit::token::type::INT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_INT } },
  // { .name = "kit::token::type::CHAR", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_CHAR } },
  // { .name = "kit::token::type::BOOL", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_BOOL } },
  // { .name = "kit::token::type::FLOAT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_FLOAT } },
  // { .name = "kit::token::type::STRING", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_STRING } },
  // { .name = "kit::token::type::FN", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_FN } },
  // { .name = "kit::token::type::EXTERN", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_EXTERN } },
  // { .name = "kit::token::type::STRUCT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_STRUCT } },
  // { .name = "kit::token::type::LET", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_LET } },
  // { .name = "kit::token::type::DEFER", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_DEFER } },
  // { .name = "kit::token::type::CONST", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_CONST } },
  // { .name = "kit::token::type::IF", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_IF } },
  // { .name = "kit::token::type::ELSE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_ELSE } },
  // { .name = "kit::token::type::WHILE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_WHILE } },
  // { .name = "kit::token::type::FOR", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_FOR } },
  // { .name = "kit::token::type::BREAK", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_BREAK } },
  // { .name = "kit::token::type::CONTINUE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_CONTINUE } },
  // { .name = "kit::token::type::RETURN", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_RETURN } },
  // { .name = "kit::token::type::NAMESPACE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_NAMESPACE } },
  // { .name = "kit::token::type::ASSERT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_ASSERT } },
  // { .name = "kit::token::type::PLUS", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_PLUS } },
  // { .name = "kit::token::type::MINUS", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_MINUS } },
  // { .name = "kit::token::type::MULTIPLY", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_MULTIPLY } },
  // { .name = "kit::token::type::DIVIDE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_DIVIDE } },
  // { .name = "kit::token::type::EXPONENT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_EXPONENT } },
  // { .name = "kit::token::type::MOD", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_MOD } },
  // { .name = "kit::token::type::PLUSPLUS", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_PLUSPLUS } },
  // { .name = "kit::token::type::MINUSMINUS", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_MINUSMINUS } },
  // { .name = "kit::token::type::AND", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_AND } },
  // { .name = "kit::token::type::OR", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_OR } },
  // { .name = "kit::token::type::NOT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_NOT } },
  // { .name = "kit::token::type::BAND", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_BAND } },
  // { .name = "kit::token::type::BOR", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_BOR } },
  // { .name = "kit::token::type::XOR", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_XOR } },
  // { .name = "kit::token::type::BNOT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_BNOT } },
  // { .name = "kit::token::type::EQUAL", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_EQUAL } },
  // { .name = "kit::token::type::NOTEQUAL", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_NOTEQUAL } },
  // { .name = "kit::token::type::DOUBLEEQUAL", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_DOUBLEEQUAL } },
  // { .name = "kit::token::type::LT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_LT } },
  // { .name = "kit::token::type::LTE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_LTE } },
  // { .name = "kit::token::type::GT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_GT } },
  // { .name = "kit::token::type::GTE", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_GTE } },
  // { .name = "kit::token::type::IDENT", .type = KIT_VARTYPE_INT, .value = { .i = KIT_TOKEN_TYPE_IDENT } },
};

#endif // KIT_BUILTIN_VARIABLES_H