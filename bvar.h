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
};

#endif // E_BUILTIN_VARIABLES_H