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

#ifndef E_BUILTIN_FUNCTIONS_H
#define E_BUILTIN_FUNCTIONS_H

#include "bfunc.io.h"
#include "bfunc.list.h"
#include "bfunc.math.h"
#include "bfunc.str.h"
#include "bfunc.vec.h"
#include "stdafx.h"
#include "var.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct e_builtin_func {
  /**
   * Function name in script.
   * Not checked for conflicts. First one is used.
   */
  const char* name;

  /**
   * Description string, printed when a compile
   * error / warning takes place. Must be single line.
   */
  const char* desc;

  /**
   * String containing how the function looks if it
   * were written in the script. To reduce ambiguity on
   * errors.
   */
  const char* signature;

  /**
   * Variable types that are allowed.
   * Excludes VOID always. You can not pass void as a variable.
   * If a variable does not have a bit set in the mask,
   * The compiler will fail and error out.
   */
  u32 allowed_mask;

  u32 min_args;

  u32 max_args;

  /**
   * Returned variables must have a reference counter
   * initialized to 1, except void.
   */
  e_var (*func)(e_var* args, u32 nargs);
} e_builtin_func;

// COPYPASTA
/*
  { "name", "desc", "fn name() -> returns", 0xFFFFFFFF, min_args, max_args, funcp },
*/

e_var eb_print(e_var* args, u32 nargs);
e_var eb_readln(e_var* args, u32 nargs);

static inline e_var
eb_println(e_var* args, u32 nargs)
{
  eb_print(args, nargs);
  fputc('\n', stdout);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var eb_cast_int(e_var* args, u32 nargs);
e_var eb_cast_char(e_var* args, u32 nargs);
e_var eb_cast_bool(e_var* args, u32 nargs);
e_var eb_cast_float(e_var* args, u32 nargs);
e_var eb_cast_string(e_var* args, u32 nargs);
e_var eb_cast_list(e_var* args, u32 nargs);

/**
 * Duplicate a variable.
 */
static inline e_var
eb_var_dup(e_var* args, u32 nargs)
{
  (void)nargs;
  e_var v;
  e_var_deep_cpy(&args[0], &v);
  return v;
}

e_var eb_get_command_line_args(e_var* args, u32 nargs);
e_var eb_get_cwd(e_var* args, u32 nargs);
e_var eb_shell(e_var* args, u32 nargs);

e_var eb_vec2(e_var* args, u32 nargs);
e_var eb_vec3(e_var* args, u32 nargs);
e_var eb_vec4(e_var* args, u32 nargs);
e_var eb_vec2_zero(e_var* args, u32 nargs);
e_var eb_vec3_zero(e_var* args, u32 nargs);
e_var eb_vec4_zero(e_var* args, u32 nargs);
e_var eb_mat3(e_var* args, u32 nargs);
e_var eb_mat4(e_var* args, u32 nargs);

e_var eb_rand_seed(e_var* args, u32 nargs);
e_var eb_rand_int(e_var* args, u32 nargs);
e_var eb_rand_float(e_var* args, u32 nargs);
e_var eb_rand_vec2(e_var* args, u32 nargs);
e_var eb_rand_vec3(e_var* args, u32 nargs);
e_var eb_rand_vec4(e_var* args, u32 nargs);
e_var eb_rand_list(e_var* args, u32 nargs);

static inline e_var
eb_len(e_var* args, u32 nargs)
{
  (void)nargs;

  int len = 0;
  if (args[0].type == E_VARTYPE_STRING) {
    len = (int)strlen(E_VAR_AS_STRING(&args[0])->s);
  } else if (args[0].type == E_VARTYPE_LIST) {
    len = (int)E_VAR_AS_LIST(&args[0])->size;
  } else if (args[0].type == E_VARTYPE_MAP) {
    len = (int)E_VAR_AS_MAP(&args[0])->size;
  }
  return (e_var){ .type = E_VARTYPE_INT, .val = { .i = len } };
}

#define E_ALL_TYPES (E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_LIST | E_VARTYPE_MAP | E_VARTYPE_STRING)

// clang-format off
static const e_builtin_func eb_funcs[] = {
  /* Can print anything. */
  { "print", "Print all provided variables to stdout", "fn print(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, eb_print },
  { "println", "Print all provided variables and a newline to stdout", "fn println(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, eb_println },

  /* Read line from stdin */
  { "readln", "Read a line from stdin and retrieve it as a string. null on error.", "fn readln(...) -> string|null", 0, 0, 0, eb_readln },
  
  {"vec2", "Cast two floats in to a vec2", "fn vec2(x, y) -> vec2", E_VARTYPE_FLOAT, 2, 2, eb_vec2},
  {"vec3", "Cast two floats in to a vec3", "fn vec3(x, y, z) -> vec3", E_VARTYPE_FLOAT, 3, 3, eb_vec3},
  {"vec4", "Cast two floats in to a vec4", "fn vec4(x, y, z, w) -> vec4", E_VARTYPE_FLOAT, 4, 4, eb_vec4},
  
  {"vec2", "Create an empty vec2 (0 initialized)", "fn vec2::zero() -> vec2", E_VARTYPE_FLOAT, 0, 0, eb_vec2_zero},
  {"vec3", "Create an empty vec3 (0 initialized)", "fn vec3::zero() -> vec3", E_VARTYPE_FLOAT, 0, 0, eb_vec3_zero},
  {"vec4", "Create an empty vec4 (0 initialized)", "fn vec4::zero() -> vec4", E_VARTYPE_FLOAT, 0, 0, eb_vec4_zero},
  
  {"mat3", "Cast three vector3's into a mat3", "fn mat3(row0, row1, row2) -> mat3", E_VARTYPE_VEC3, 3, 3, eb_mat3},
  {"mat4", "Cast three vector4's into a mat3", "fn mat4(row0, row1, row2, row3) -> mat4", E_VARTYPE_VEC4, 4, 4, eb_mat4},
  
    /* Can convert anything to string. */
  { "string", "Cast a variable to a string", "fn string(v) -> string", 0xFFFFFFFF, 1, 1, eb_cast_string },

  /* Scalar types */
  { "int", "Cast a variable to a int", "fn int(v) -> int", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_int },
  { "char", "Cast a variable to a char", "fn char(v) -> char", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_char },
  { "bool", "Cast a variable to a bool", "fn bool(v) -> bool", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_bool },
  { "float", "Cast a variable to a float", "fn float(v) -> float", E_VARTYPE_INT | E_VARTYPE_CHAR | E_VARTYPE_BOOL | E_VARTYPE_FLOAT | E_VARTYPE_STRING,    1,    1, eb_cast_float },

  { "rand::seed", "Seed the random number generator with a string.", "fn rand::seed(str) -> null", 0, 1, 1, eb_rand_seed },
  { "rand::list", "Get a random list of num integers between min and max (inclusive of both)", "fn rand::list(min, max, num) -> []", 0, 3, 3, eb_rand_list },
  { "rand::int", "Get a random integer between 0 and int::MAX (inclusive of both)", "fn rand::int() -> int", 0, 0, 0, eb_rand_int },
  { "rand::float", "Get a random float between 0 and 1 (inclusive of both)", "fn rand::float() -> float", 0, 0, 0, eb_rand_float },
  { "rand::vec2", "Get a vector2 of two random floats between 0 and 1", "fn rand::vec2() -> vec2", 0, 0, 0, eb_rand_vec2 },
  { "rand::vec3", "Get a vector3 of three random floats between 0 and 1", "fn rand::vec3() -> vec3", 0, 0, 0, eb_rand_vec3 },
  { "rand::vec4", "Get a vector4 of four random floats between 0 and 1", "fn rand::vec4() -> vec4", 0, 0, 0, eb_rand_vec4 },

  /* Anything can be added as an element to a list */
  { "list", "Make a list of provided elements", "fn list(...) -> list", 0xFFFFFF, 1, UINT32_MAX, eb_cast_list },

  { "len", "Get the type dependant length of the given variable (String=Length, List=Size, Map=NPairs)", "fn len(v : string|list|map) -> int", E_VARTYPE_STRING | E_VARTYPE_LIST | E_VARTYPE_MAP,    1,    1, eb_len },

  /* Math builtins */
  { "math::sin", "Compute the sine of the given argument (radians)", "fn math::sin(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_sin },
  { "math::cos", "Compute the cosine of the given argument (radians)", "fn math::cos(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_cos },
  { "math::tan", "Compute the tangent of the given argument (radians)", "fn math::tan(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_tan },
  { "math::asin", "Compute the arcsine of the given argument (radians)", "fn math::asin(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_asin },
  { "math::acos", "Compute the arccosine of the given argument (radians)", "fn math::acos(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_acos },
  { "math::atan", "Compute the arctangent of the given argument (radians)", "fn math::atan(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_atan },
  { "math::atan2", "Compute the arctangent of the given arguments (x, y) (radians)", "fn math::atan2(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_atan2 },
  { "math::sinh", "Compute the hyperbolic sine of the given argument (radians)", "fn math::sinh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_sinh },
  { "math::cosh", "Compute the hyperbolic cosine of the given argument (radians)", "fn math::cosh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_cosh },
  { "math::tanh", "Compute the hyperbolic tangent of the given argument (radians)", "fn math::tanh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_tanh },
  { "math::asinh", "Compute the hyperbolic arcsine of the given argument (radians)", "fn math::asinh(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_asinh },
  { "math::acosh", "Compute the hyperbolic arccosine of the given argument (radians)", "fn math::acosh(x) -> float", E_VARTYPE_FLOAT,    1,    1, eb_acosh },
  { "math::atanh", "Compute the hyperbolic arctangent of the given argument (radians)", "fn math::atanh(x) -> float", E_VARTYPE_FLOAT,    1,    1, eb_atanh },
  { "math::exp", "Compute e^n (n is argument)", "fn math::exp(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_exp },
  { "math::log", "Compute the natural logarithm of the given argument", "fn math::log(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_log },
  { "math::log10", "Compute the logarithm in base 10 of the given argument", "fn math::log10(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_log10 },
  { "math::pow", "Compute x ^ y, with both x and y being arguments", "fn math::pow(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_pow },
  { "math::sqrt", "Compute the square root of the given argument", "fn math::sqrt(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_sqrt },
  { "math::ceil", "Round the given value up", "fn math::ceil(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_ceil },
  { "math::floor", "Round the given value down", "fn math::floor(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_floor },
  { "math::mod", "Compute the remainder of the given arguments", "fn math::mod(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_fmod },
  { "math::round", "Round the given argument", "fn math::round(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_round },
  { "math::trunc", "Truncate the decimal part of the argument (result is still floating point)", "fn math::trunc(x) -> float", E_VARTYPE_FLOAT,    1,    1, eb_trunc },
  { "math::abs", "Compute the absolute value of the given argument", "fn math::abs(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_abs },
  { "math::hypot", "Compute the sqrt(x^2 + y^2), with maximum accuracy", "fn math::hypot(x, y) -> float", E_VARTYPE_FLOAT, 2, 2, eb_hypot },
  { "math::signbit", "Get the sign bit of the given floating point input (0 positive, 1 negative)", "fn math::signbit(x) -> int", E_VARTYPE_FLOAT, 1, 1, eb_signbit },
  { "math::deg2rad", "Convert input degrees to radians", "fn math::deg2rad(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_deg2rad },
  { "math::rad2deg", "Convert input radians to degrees", "fn math::deg2rad(x) -> float", E_VARTYPE_FLOAT, 1, 1, eb_rad2deg },

  { "io::open", "Open a file. null on error.", "fn io::open( path:string, mode:string ) -> fd", E_VARTYPE_INT, 2, 2, eb_io_open },
  { "io::close", "Close a file.", "fn io::close( fd ) -> null", E_VARTYPE_INT, 1, 1, eb_io_close },
  { "io::putc", "\"Put\" a character into file.", "fn io::putc( fd, ch ) -> null", E_VARTYPE_INT, 1, 1, eb_io_close },
  { "io::getc", "Get a character from file. null on error.", "fn io::getc( fd ) -> char|null", E_VARTYPE_INT, 1, 1, eb_io_close },
  { "io::read", "Read given number of bytes from file. null on error.", "fn io::read( fd, nbytes:int ) -> string|null", E_VARTYPE_INT, 2, 2, eb_io_read },
  { "io::readln", "Read a line from file. null on error.", "fn io::readln( fd ) -> string|null", E_VARTYPE_INT, 1, 1, eb_io_readln },
  { "io::write", "Write string to file. null on error. number of bytes written on success.", "fn io::write( fd, data : string ) -> int|null", E_VARTYPE_INT, 2, 2, eb_io_write },
  { "io::flush", "Flush the stream to make changes immediately visible", "fn io::flush( fd ) -> null", E_VARTYPE_INT, 1, 1, eb_io_flush },
  { "io::seek", "Seek to a random position in file.", "fn io::seek( fd, offset, relative_to ) -> null", E_VARTYPE_INT, 3, 3, eb_io_seek },
  { "io::ptell", "Get location in file. null on error.", "fn io::ptell( fd ) -> int|null", E_VARTYPE_INT, 1, 1, eb_io_ptell },
  { "io::println", "Print all given variables and a newline to file. null on error. number of bytes written on success.", "fn io::println( fd, ... ) -> int|null", E_VARTYPE_INT,    1, UINT32_MAX, eb_io_println },
  { "io::print", "Print all given variables to file. null on error. number of bytes written on success.", "fn io::print( fd, ... ) -> int|null", E_VARTYPE_INT, 2, UINT32_MAX, eb_io_print },
  { "io::type", "Get the type of an object on the disk. Possible return values are io::FILE|io::LINK|io::DIRECTORY|io::UNKNOWN", "fn io::type( path ) -> int", E_VARTYPE_INT, 1, 1, eb_io_type },
  { "io::list_dir", "List a directory. Returns the file (and directory) paths as a list.", "fn io::list_dir( path ) -> list", E_VARTYPE_STRING, 1, 1, eb_io_list_dir },
  { "io::at_eof", "At the end of file? Boolean result only", "fn io::at_eof(fd) -> bool", E_VARTYPE_INT, 1, 1, eb_io_at_eof },
  { "io::error", "Get error string. Won't always be success for successful operations.", "fn io::error(fd) -> string", 0, 0, 0, eb_io_error },

  { "str::dup", "Duplicate a string. Alias to generic::dup", "fn str::dup(s : string) -> string", E_VARTYPE_STRING, 1, 1, eb_var_dup },
  { "str::equal", "Compare two strings. Boolean result.", "fn str::equal(s1 : string, s2 : string) -> bool", E_VARTYPE_STRING, 2, 2, eb_str_equal },
  { "str::cat", "Catenate all given strings. null on error.", "fn str::cat(...) -> string", E_VARTYPE_STRING, 1, UINT32_MAX, eb_str_cat },
  { "str::substr", "Make a substring of given string, starting from 'start' with the length 'len'", "fn str::substr(s : string, start : int, len : int) -> string", E_VARTYPE_STRING, 3, 3, eb_str_substr },
  { "str::repeat", "Repeat a string n times", "fn str::repeat(s : string, n : int) -> string", E_VARTYPE_STRING, 2, 2, eb_str_repeat },
  { "str::ltrim", "Trim all spaces from the left in the string and return a copy", "fn str::ltrim(s : string) -> string", E_VARTYPE_STRING,    1,    1, eb_str_ltrim },
  { "str::rtrim", "Trim all spaces from the right in the string and return a copy", "fn str::rtrim(s : string) -> string", E_VARTYPE_STRING,    1,    1, eb_str_rtrim },
  { "str::trim", "Trim all spaces from the string and return a copy", "fn str::trim(s : string) -> string", E_VARTYPE_STRING, 1, 1, eb_str_trim },
  { "str::split", "Split a string by a another string (the delimiter) and return a list containing each token", "fn str::split(s : string, delim : string)", E_VARTYPE_STRING, 2, 2, eb_str_split },
  { "str::len", "Get the length of the string. Alias to len", "fn str::len(s : string) -> int", E_VARTYPE_STRING, 1, 1, eb_str_len }, // equivalent to len()

  { "list::dup", "Duplicate a list. Alias to dup", "fn list::dup(list) -> list", E_VARTYPE_LIST, 1, 1, eb_var_dup },
  { "list::make", "Make a list using given elements", "fn list::make(...) -> list", E_ALL_TYPES, 1, UINT32_MAX, eb_list_make },
  { "list::append", "Append a value to the list", "fn list::append(list, var) -> null", E_ALL_TYPES, 2, 2, eb_list_append },
  { "list::pop", "Remove the last element of the list and return it", "fn list::pop(list) -> var", E_ALL_TYPES, 1, 1, eb_list_pop },
  { "list::remove", "Remove the element from the index in the list and return it", "fn list::remove(list, index:int) -> var", E_VARTYPE_LIST | E_VARTYPE_INT, 2, 2, eb_list_remove },
  { "list::insert", "Insert an element into the given index in the list", "fn list::insert(list, index : int) -> null", E_ALL_TYPES, 3, 3, eb_list_insert },
  { "list::find", "Find an element in the list. -1 if nonexistent.", "fn list::find(list, to_find : var) -> int", E_ALL_TYPES, 2, 2, eb_list_find },
  { "list::rfind", "Find an element in the list, starting the search from the back of the list. -1 if nonexistent.", "fn list::rfind(list, to_find : var) -> int", E_ALL_TYPES, 2, 2, eb_list_rfind },
  { "list::reserve", "Reserve capacity for n elements.", "fn list::reserve(list, elems_to_reserve:int) -> null", E_VARTYPE_LIST | E_VARTYPE_INT, 2, 2, eb_list_reserve },
  { "list::resize", "Resize the list to n elements, truncating or adding new if necessary.", "fn list::resize(list, new_size:int) -> null", E_VARTYPE_LIST | E_VARTYPE_INT, 2, 2, eb_list_resize },
  { "list::len", "Get number of elements in list.", "fn list::len(list) -> int", E_VARTYPE_LIST, 1, 1, eb_list_len },

  { "sys::get_command_line_args", "Get the command line arguments passed, as a list", "fn sys::get_command_line_args() -> list|null", E_VARTYPE_VOID, 0, 0, eb_get_command_line_args },
  { "sys::get_cwd", "Get the current working directory as a string. null if OS has no such concept (if there even is an OS)", "fn sys::get_cwd() -> string|null", E_VARTYPE_VOID, 0, 0, eb_get_cwd },
  { "sys::shell", "Execute the command through the system shell. Returns its return code, null if error outside of the command occurs.", "fn sys::shell(str) -> int|null", E_VARTYPE_VOID, 1, 1, eb_shell},

  { "vec::norm", "Get the normalized vector", "fn vec::norm(vec) -> vec2|null", E_VARTYPE_VEC2|E_VARTYPE_VEC3|E_VARTYPE_VEC4, 1, 1, eb_vec_norm },
  { "vec::len", "Get the length (magnitude) of a vector", "fn vec::len(vec) -> float|null", E_VARTYPE_VEC2|E_VARTYPE_VEC3|E_VARTYPE_VEC4, 1, 1, eb_vec_len },
  { "vec::len2", "Get the squared length (magnitude) of a vector", "fn vec::len2(vec) -> float|null", E_VARTYPE_VEC2|E_VARTYPE_VEC3|E_VARTYPE_VEC4, 1, 1, eb_vec_len2 },
  { "vec::dist", "Get (euclidean) distance between two vectors", "fn vec::dist(v1,v2) -> float|null", E_VARTYPE_VEC2|E_VARTYPE_VEC3|E_VARTYPE_VEC4, 1, 1, eb_vec_dist },
  { "vec::dist2", "Get squared (euclidean) distance between two vectors", "fn vec::dist2(v1,v2) -> float|null", E_VARTYPE_VEC2|E_VARTYPE_VEC3|E_VARTYPE_VEC4, 1, 1, eb_vec_dist2 },

  { "vec3::zx", "Zero extend given vector up to a vec3. If input is vec4, truncates 4th dimension", "fn vec3::zx(vec) -> vec3", E_VARTYPE_VEC2|E_VARTYPE_VEC3|E_VARTYPE_VEC4, 1, 1, eb_vec3_zx },
  { "vec4::zx", "Zero extend given vector up to a vec4.", "fn vec4::zx(vec) -> vec4", E_VARTYPE_VEC2|E_VARTYPE_VEC3|E_VARTYPE_VEC4, 1, 1, eb_vec4_zx },
};
// clang-format on

#endif // E_BUILTIN_FUNCTIONS_H
