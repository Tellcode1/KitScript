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

#ifndef KIT_BUILTIN_FUNCTIONS_H
#define KIT_BUILTIN_FUNCTIONS_H

#include "kit.bfunc.io.h"
#include "kit.bfunc.list.h"
#include "kit.bfunc.math.h"
#include "kit.bfunc.rt.h"
#include "kit.bfunc.str.h"
#include "kit.bfunc.sys.h"
#include "kit.bfunc.time.h"
#include "kit.bfunc.vec.h"
#include "kit.cast.h"
#include "kit.perr.h"
#include "kit.stdafx.h"
#include "kit.var.h"
#include "kit.vm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef KIT_PANIC_FUNCTION_NAME
#  define KIT_PANIC_FUNCTION_NAME "PANIC"
#endif

typedef struct kit_builtin_func {
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
   * Excludes NULL always. You can not pass void as a variable.
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
  kit_ecode (*func)(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
} kit_builtin_func;

// COPYPASTA
/*
  { "name", "desc", "fn name() -> returns", 0xFFFFFFFF, min_args, max_args, funcp },
*/

kit_ecode kit_builtins_print(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_readln(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

static inline kit_ecode
kit_builtins_println(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_ecode e = kit_builtins_print(vm, args, nargs, result);
  fputc('\n', stdout);
  return e;
}

kit_ecode kit_builtins_cast_int(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_cast_char(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_cast_bool(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_cast_float(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_cast_string(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_cast_list(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

/**
 * Duplicate a variable.
 */
static inline kit_ecode
kit_builtins_var_dup(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_var v;
  kit_var_deep_cpy(vm->pool, &args[0], &v);

  *result = v;
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_var_hash(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = kit_var_from_int((int)kit_hash(&args[0], sizeof(kit_var)));
  return KIT_OK;
}

kit_ecode kit_builtins_struct_name(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_struct_member_count(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

kit_ecode kit_builtins_rand_seed(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rand_int(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rand_range(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rand_float(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rand_vec2(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rand_vec3(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rand_vec4(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_rand_list(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

kit_ecode kit_builtins_log_err(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_log_warn(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_log_info(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

static inline kit_ecode
kit_builtins_len(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  int len = 0;
  if (args[0].type == KIT_VARTYPE_STRING) {
    len = (int)strlen(KIT_VAR_AS_STRING(&args[0])->s);
  } else if (args[0].type == KIT_VARTYPE_LIST) {
    len = (int)KIT_VAR_AS_LIST(&args[0])->size;
  } else if (args[0].type == KIT_VARTYPE_MAP) {
    len = (int)KIT_VAR_AS_MAP(&args[0])->size;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_INT, .val = { .i = len } };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_type_of(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = kit_var_from_int(args[0].type);
  return KIT_OK;
}

#define KIT_ALL_TYPES                                                                                                                                \
  (KIT_VARTYPE_INT | KIT_VARTYPE_CHAR | KIT_VARTYPE_BOOL | KIT_VARTYPE_FLOAT | KIT_VARTYPE_LIST | KIT_VARTYPE_MAP | KIT_VARTYPE_STRING)

// clang-format off
static const kit_builtin_func kit_builtins_funcs[] = {
  /* Can print anything. */
  { "print", "Print all provided variables to stdout", "fn print(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_print },
  { "printf", "Print formatted string to stdout", "fn printf(fmt, ...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_print },
  { "println", "Print all provided variables and a newline to stdout", "fn println(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_println },
  // { "printfln", "Print formatted line to stdout", "fn printfln(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_println },

  /* Read line from stdin */
  { "readln", "Read a line from stdin and retrieve it as a string. null on error.", "fn readln(...) -> string|null", 0, 0, 0, kit_builtins_readln },

  // If you intend to change the name here, goto exec.c and replace the name there too. 
  // Only kept here for signature.
  { KIT_PANIC_FUNCTION_NAME, "Error out and stop execution", "fn " KIT_PANIC_FUNCTION_NAME "() -> null (noreturn)", 0, 0, 0, NULL }, // Function pointer is NULL, handled by VM itself.


  { "log::info", "Print an informational message to either the log file (or stderr if unset)", "fn log::info(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_log_info },
  { "log::error", "Print an error message to either the log file (or stderr if unset)", "fn log::err(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_log_err },
  { "log::err", "Print an error message to either the log file (or stderr if unset) (alias to log::error)", "fn log::err(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_log_err },
  { "log::warn", "Print a warning message to either the log file (or stderr if unset)", "fn log::warn(...) -> null", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_log_warn },
  
  { "vec2", "Cast two floats in to a vec2", "fn vec2(x, y) -> vec2", KIT_VARTYPE_FLOAT, 2, 2, kit_builtins_vec2 },
  { "vec3", "Cast two floats in to a vec3", "fn vec3(x, y, z) -> vec3", KIT_VARTYPE_FLOAT, 3, 3, kit_builtins_vec3 },
  { "vec4", "Cast two floats in to a vec4", "fn vec4(x, y, z, w) -> vec4", KIT_VARTYPE_FLOAT, 4, 4, kit_builtins_vec4 },

  {"mat3", "Cast three vector3's into a mat3", "fn mat3(row0, row1, row2) -> mat3", KIT_VARTYPE_VEC3, 3, 3, kit_builtins_mat3},
  {"mat4", "Cast three vector4's into a mat3", "fn mat4(row0, row1, row2, row3) -> mat4", KIT_VARTYPE_VEC4, 4, 4, kit_builtins_mat4},
  
  
  { "kit::type_of", "Get the type of the input, as an enumerator (see bvar.h, KIT_TYPE_* constants)", "fn kit::type_of(input) -> int", 0xFFFFFFFF, 1, 1, kit_builtins_type_of },
  { "kit::struct::name", "Get the name of the structure, as a string", "fn kit::struct::name(input) -> string", 0xFFFFFFFF, 1, 1, kit_builtins_struct_name },
  { "kit::struct::member_count", "Get the number of members in the given structure", "fn kit::struct::member_count(struc) -> int", 0xFFFFFFFF, 1, 1, kit_builtins_struct_member_count },
  { "kit::dup", "Duplicate the given variable, performing a deep copy, and return it", "fn kit::dup(x : any) -> any", 0xFFFFFFFF, 1, 1, kit_builtins_var_dup },
  { "kit::hash", "Hash a given variable. This is not a cryptographic hash.", "fn kit::hash(x : any) -> int", 0xFFFFFFFF, 1, 1, kit_builtins_var_hash },

  /* Can convert anything to string. */
  { "string", "Cast a variable to a string", "fn string(v) -> string", 0xFFFFFFFF, 1, 1, kit_builtins_cast_string },

  /* Scalar types */
  { "int", "Cast a variable to a int", "fn int(v) -> int", KIT_VARTYPE_INT | KIT_VARTYPE_CHAR | KIT_VARTYPE_BOOL | KIT_VARTYPE_FLOAT | KIT_VARTYPE_STRING,    1,    1, kit_builtins_cast_int },
  { "char", "Cast a variable to a char", "fn char(v) -> char", KIT_VARTYPE_INT | KIT_VARTYPE_CHAR | KIT_VARTYPE_BOOL | KIT_VARTYPE_FLOAT | KIT_VARTYPE_STRING,    1,    1, kit_builtins_cast_char },
  { "bool", "Cast a variable to a bool", "fn bool(v) -> bool", KIT_VARTYPE_INT | KIT_VARTYPE_CHAR | KIT_VARTYPE_BOOL | KIT_VARTYPE_FLOAT | KIT_VARTYPE_STRING | KIT_VARTYPE_VEC2 | KIT_VARTYPE_VEC3 | KIT_VARTYPE_VEC4,    1,    1, kit_builtins_cast_bool },
  { "float", "Cast a variable to a float", "fn float(v) -> float", KIT_VARTYPE_INT | KIT_VARTYPE_CHAR | KIT_VARTYPE_BOOL | KIT_VARTYPE_FLOAT | KIT_VARTYPE_STRING,    1,    1, kit_builtins_cast_float },
  
  { "rand::seed", "Seed the random number generator with a string.", "fn rand::seed(str) -> null", 0, 1, 1, kit_builtins_rand_seed },
  { "rand::list", "Get a random list of num integers between min and max (inclusive of both)", "fn rand::list(min, max, num) -> []", 0, 3, 3, kit_builtins_rand_list },
  { "rand::int", "Get a random integer between 0 and int::MAX (inclusive of both)", "fn rand::int() -> int", 0, 0, 0, kit_builtins_rand_int },
  { "rand::range", "Get a random integer between min and max (inclusive of both)", "fn rand::range(min, max) -> int", 0, 2, 2, kit_builtins_rand_range },
  { "rand::float", "Get a random float between 0 and 1 (inclusive of both)", "fn rand::float() -> float", 0, 0, 0, kit_builtins_rand_float },
  { "rand::vec2", "Get a vector2 of two random floats between 0 and 1", "fn rand::vec2() -> vec2", 0, 0, 0, kit_builtins_rand_vec2 },
  { "rand::vec3", "Get a vector3 of three random floats between 0 and 1", "fn rand::vec3() -> vec3", 0, 0, 0, kit_builtins_rand_vec3 },
  { "rand::vec4", "Get a vector4 of four random floats between 0 and 1", "fn rand::vec4() -> vec4", 0, 0, 0, kit_builtins_rand_vec4 },

  /* Anything can be added as an element to a list */
  { "list", "Make a list of provided elements", "fn list(...) -> list", 0xFFFFFFFF, 1, UINT32_MAX, kit_builtins_cast_list },

  { "len", "Get the type dependant length of the given variable (String => Length, List -> length, Map => Number of pairs)", "fn len(v : string|list|map) -> int", KIT_VARTYPE_STRING | KIT_VARTYPE_LIST | KIT_VARTYPE_MAP,    1,    1, kit_builtins_len },

  /* Math builtins */
  { "math::sin", "Compute the sine of the given argument (radians)", "fn math::sin(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_sin },
  { "math::cos", "Compute the cosine of the given argument (radians)", "fn math::cos(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_cos },
  { "math::tan", "Compute the tangent of the given argument (radians)", "fn math::tan(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_tan },
  { "math::asin", "Compute the arcsine of the given argument (radians)", "fn math::asin(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_asin },
  { "math::acos", "Compute the arccosine of the given argument (radians)", "fn math::acos(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_acos },
  { "math::atan", "Compute the arctangent of the given argument (radians)", "fn math::atan(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_atan },
  { "math::atan2", "Compute the arctangent of the given arguments (x, y) (radians)", "fn math::atan2(x, y) -> float", KIT_VARTYPE_FLOAT, 2, 2, kit_builtins_atan2 },
  { "math::sinh", "Compute the hyperbolic sine of the given argument (radians)", "fn math::sinh(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_sinh },
  { "math::cosh", "Compute the hyperbolic cosine of the given argument (radians)", "fn math::cosh(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_cosh },
  { "math::tanh", "Compute the hyperbolic tangent of the given argument (radians)", "fn math::tanh(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_tanh },
  { "math::asinh", "Compute the hyperbolic arcsine of the given argument (radians)", "fn math::asinh(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_asinh },
  { "math::acosh", "Compute the hyperbolic arccosine of the given argument (radians)", "fn math::acosh(x) -> float", KIT_VARTYPE_FLOAT,    1,    1, kit_builtins_acosh },
  { "math::atanh", "Compute the hyperbolic arctangent of the given argument (radians)", "fn math::atanh(x) -> float", KIT_VARTYPE_FLOAT,    1,    1, kit_builtins_atanh },
  { "math::exp", "Compute e^n (n is argument)", "fn math::exp(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_exp },
  { "math::log", "Compute the natural logarithm of the given argument", "fn math::log(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_log },
  { "math::log10", "Compute the logarithm in base 10 of the given argument", "fn math::log10(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_log10 },
  { "math::pow", "Compute x ^ y, with both x and y being arguments", "fn math::pow(x, y) -> float", KIT_VARTYPE_FLOAT, 2, 2, kit_builtins_pow },
  { "math::sqrt", "Compute the square root of the given argument", "fn math::sqrt(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_sqrt },
  { "math::ceil", "Round the given value up", "fn math::ceil(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_ceil },
  { "math::floor", "Round the given value down", "fn math::floor(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_floor },
  { "math::mod", "Compute the remainder of the given arguments", "fn math::mod(x, y) -> float", KIT_VARTYPE_FLOAT, 2, 2, kit_builtins_fmod },
  { "math::round", "Round the given argument", "fn math::round(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_round },
  { "math::trunc", "Truncate the decimal part of the argument (result is still floating point)", "fn math::trunc(x) -> float", KIT_VARTYPE_FLOAT,    1,    1, kit_builtins_trunc },
  { "math::abs", "Compute the absolute value of the given argument", "fn math::abs(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_abs },
  { "math::hypot", "Compute the sqrt(x^2 + y^2), with maximum accuracy", "fn math::hypot(x, y) -> float", KIT_VARTYPE_FLOAT, 2, 2, kit_builtins_hypot },
  { "math::signbit", "Get the sign bit of the given floating point input (0 positive, 1 negative)", "fn math::signbit(x) -> int", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_signbit },
  { "math::deg2rad", "Convert input degrees to radians", "fn math::deg2rad(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_deg2rad },
  { "math::rad2deg", "Convert input radians to degrees", "fn math::rad2deg(x) -> float", KIT_VARTYPE_FLOAT, 1, 1, kit_builtins_rad2deg },
  { "math::min", "Smallest between two given inputs", "fn math::min(x,y) -> float", KIT_VARTYPE_FLOAT, 2, 2, kit_builtins_min },
  { "math::max", "Largest between two given inputs", "fn math::max(x,y) -> float", KIT_VARTYPE_FLOAT, 2, 2, kit_builtins_max },
  { "math::clamp", "Largest between two given inputs", "fn math::clamp(x, min, max) -> float", KIT_VARTYPE_FLOAT, 3, 3, kit_builtins_clamp },
  { "math::lerp", "Linearly interpolate between two integral or vector values", "fn math::lerp(a, b, t) -> int|float|char|bool", KIT_VARTYPE_FLOAT, 3, 3, kit_builtins_lerp },
  { "math::smoothstep", "Smoothstep between two integral or vector values", "fn math::smoothstep(a, b, t) -> int|float|char|bool", KIT_VARTYPE_FLOAT, 3, 3, kit_builtins_smoothstep },

  { "io::open", "Open a file. null on error. File descriptor (integer) on success.", "fn io::open( path:string, mode:string ) -> fd", KIT_VARTYPE_INT, 2, 2, kit_builtins_io_open },
  { "io::close", "Close a file.", "fn io::close( fd ) -> null", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_close },
  { "io::mkdir", "Create a directory. 0 on success, anything else otherwise.", "fn io::mkdir( path:string, mode ) -> int", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_mkdir },
  { "io::putc", "\"Put\" a character into file.", "fn io::putc( fd, ch ) -> null", KIT_VARTYPE_INT, 2, 2, kit_builtins_io_putc },
  { "io::getc", "Get a character from file. null on error.", "fn io::getc( fd ) -> char|null", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_getc },
  { "io::read", "Read given number of bytes from file. null on error.", "fn io::read( fd, nbytes:int ) -> string|null", KIT_VARTYPE_INT, 2, 2, kit_builtins_io_read },
  { "io::readln", "Read a line from file. null on error.", "fn io::readln( fd ) -> string|null", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_readln },
  { "io::write", "Write string to file. null on error. number of bytes written on success.", "fn io::write( fd, data : string ) -> int|null", KIT_VARTYPE_INT, 2, 2, kit_builtins_io_write },
  { "io::flush", "Flush the stream to make changes immediately visible", "fn io::flush( fd ) -> null", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_flush },
  { "io::seek", "Seek to a random position in file.", "fn io::seek( fd, offset, relative_to ) -> null", KIT_VARTYPE_INT, 3, 3, kit_builtins_io_seek },
  { "io::ptell", "Get location in file. null on error.", "fn io::ptell( fd ) -> int|null", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_ptell },
  { "io::println", "Print all given variables and a newline to file. null on error. number of bytes written on success.", "fn io::println( fd, ... ) -> int|null", KIT_VARTYPE_INT,    1, UINT32_MAX, kit_builtins_io_println },
  { "io::print", "Print all given variables to file. null on error. number of bytes written on success.", "fn io::print( fd, ... ) -> int|null", KIT_VARTYPE_INT, 2, UINT32_MAX, kit_builtins_io_print },
  { "io::exists", "Check whether a file exists or not.", "fn io::exists( path ) -> bool", KIT_VARTYPE_STRING, 1, 1, kit_builtins_io_exists },
  { "io::type", "Get the type of an object on the disk. Possible return values are io::FILE|io::LINK|io::DIRECTORY|io::UNKNOWN", "fn io::type( path ) -> int", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_type },
  { "io::listdir", "List a directory. Returns the file (and directory) paths as a list. Paths in list are relative to path.", "fn io::list_dir( path ) -> list", KIT_VARTYPE_STRING, 1, 1, kit_builtins_io_listdir },
  { "io::at_eof", "At the end of file? Boolean result only", "fn io::at_eof(fd) -> bool", KIT_VARTYPE_INT, 1, 1, kit_builtins_io_at_eof },
  { "io::error", "Get error string. Won't always be success for successful operations.", "fn io::error(fd) -> string", 0, 0, 0, kit_builtins_io_error },

  { "str::dup", "Duplicate a string. Alias to generic::dup", "fn str::dup(s : string) -> string", KIT_VARTYPE_STRING, 1, 1, kit_builtins_var_dup },
  { "str::equal", "Compare two strings. Boolean result.", "fn str::equal(s1 : string, s2 : string) -> bool", KIT_VARTYPE_STRING, 2, 2, kit_builtins_str_equal },
  { "str::cat", "Catenate all given strings. null on error.", "fn str::cat(...) -> string", KIT_VARTYPE_STRING, 1, UINT32_MAX, kit_builtins_str_cat },
  { "str::find", "Find a substring within a string. -1 if non existent, >=0 if found.", "fn str::find(hay, needle) -> int", KIT_VARTYPE_STRING, 2, 2, kit_builtins_str_find },
  { "str::replace", "Replace all occurences of a substring within a string. Returns the number of occurences", "fn str::replace(string, replace_this, replace_with) -> int", KIT_VARTYPE_STRING, 3, 3, kit_builtins_str_replace },
  { "str::substr", "Make a substring of given string, starting from 'start' with the length 'len'", "fn str::substr(s : string, start : int, len : int) -> string", KIT_VARTYPE_STRING, 3, 3, kit_builtins_str_substr },
  { "str::repeat", "Repeat a string n times", "fn str::repeat(s : string, n : int) -> string", KIT_VARTYPE_STRING, 2, 2, kit_builtins_str_repeat },
  { "str::ltrim", "Trim all spaces from the left in the string and return a copy", "fn str::ltrim(s : string) -> string", KIT_VARTYPE_STRING,    1,    1, kit_builtins_str_ltrim },
  { "str::rtrim", "Trim all spaces from the right in the string and return a copy", "fn str::rtrim(s : string) -> string", KIT_VARTYPE_STRING,    1,    1, kit_builtins_str_rtrim },
  { "str::trim", "Trim all spaces from the string and return a copy", "fn str::trim(s : string) -> string", KIT_VARTYPE_STRING, 1, 1, kit_builtins_str_trim },
  { "str::split", "Split a string by a another string (the delimiter) and return a list containing each token", "fn str::split(s : string, delim : string)", KIT_VARTYPE_STRING, 2, 2, kit_builtins_str_split },
  { "str::len", "Get the length of the string. Alias to len", "fn str::len(s : string) -> int", KIT_VARTYPE_STRING, 1, 1, kit_builtins_str_len }, // equivalent to len()

  { "list::dup", "Duplicate a list. Alias to dup", "fn list::dup(list) -> list", KIT_VARTYPE_LIST, 1, 1, kit_builtins_var_dup },
  { "list::make", "Make a list using given elements", "fn list::make(...) -> list", KIT_ALL_TYPES, 1, UINT32_MAX, kit_builtins_list_make },
  { "list::append", "Append a value to the list", "fn list::append(list, var) -> null", KIT_ALL_TYPES, 2, 2, kit_builtins_list_append },
  { "list::pop", "Remove the last element of the list and return it", "fn list::pop(list) -> var", KIT_ALL_TYPES, 1, 1, kit_builtins_list_pop },
  { "list::remove", "Remove the element from the index in the list and return it", "fn list::remove(list, index:int) -> var", KIT_VARTYPE_LIST | KIT_VARTYPE_INT, 2, 2, kit_builtins_list_remove },
  { "list::insert", "Insert an element into the given index in the list", "fn list::insert(list, index : int) -> null", KIT_ALL_TYPES, 3, 3, kit_builtins_list_insert },
  { "list::find", "Find an element in the list. -1 if nonexistent.", "fn list::find(list, to_find : var) -> int", KIT_ALL_TYPES, 2, 2, kit_builtins_list_find },
  { "list::rfind", "Find an element in the list, starting the search from the back of the list. -1 if nonexistent.", "fn list::rfind(list, to_find : var) -> int", KIT_ALL_TYPES, 2, 2, kit_builtins_list_rfind },
  { "list::exists", "Check if an element exists in the list", "fn list::exists(list, to_find : var) -> bool", KIT_ALL_TYPES, 2, 2, kit_builtins_list_exists },
  { "list::reserve", "Reserve capacity for n elements.", "fn list::reserve(list, elems_to_reserve:int) -> null", KIT_VARTYPE_LIST | KIT_VARTYPE_INT, 2, 2, kit_builtins_list_reserve },
  { "list::resize", "Resize the list to n elements, truncating or adding new if necessary.", "fn list::resize(list, new_size:int) -> null", KIT_VARTYPE_LIST | KIT_VARTYPE_INT, 2, 2, kit_builtins_list_resize },
  { "list::sort", "Sort a list.", "fn list::sort(list) -> null", KIT_VARTYPE_LIST, 1, 1, kit_builtins_list_sort },
  { "list::len", "Get number of elements in list.", "fn list::len(list) -> int", KIT_VARTYPE_LIST, 1, 1, kit_builtins_list_len },

  { "sys::get_cmd_args", "Get the command line arguments passed, as a list", "fn sys::get_cmd_args() -> list|null", KIT_VARTYPE_NULL, 0, 0, kit_builtins_sys_get_cmd_args },
  { "sys::get_cwd", "Get the current working directory as a string. null if OS has no such concept (if there even is an OS)", "fn sys::get_cwd() -> string|null", KIT_VARTYPE_NULL, 0, 0, kit_builtins_sys_get_cwd },
  { "sys::shell", "Execute the command through the system shell. Returns its return code, null if error outside of the command occurs.", "fn sys::shell(str) -> int|null", KIT_VARTYPE_NULL, 1, 1, kit_builtins_sys_shell},
  { "sys::sleepms", "Sleep for the number of milliseconds given. Actual time slept may be larger.", "fn sys::sleepms(ms : int) -> null", KIT_VARTYPE_NULL, 1, 1, kit_builtins_sys_sleep},
  { "sys::getenv", "Get the value of the enivronment variable as a string. NULL if no such env var.", "fn sys::getenv(name : string) -> string|null", KIT_VARTYPE_STRING, 1, 1, kit_builtins_sys_getenv},
  { "sys::setenv", "Set the environment variable to given value, creating if it doesn't exist, overwriting if it does.", "fn sys::setenv(name : string, value : string) -> null", KIT_VARTYPE_STRING, 1, 1, kit_builtins_sys_setenv},
 
  // We dispatch to the correct length inside the function. Having each individual function for each vector type is bloat.
#define X(d)\
  { "vec"#d"::norm", "Get the normalized vector", "fn vec"#d"::norm(vec) -> vec"#d"|null", KIT_VARTYPE_VEC##d, 1, 1, kit_builtins_vec_norm },\
  { "vec"#d"::len", "Get the length (magnitude) of a vector", "fn vec"#d"::len(vec) -> float|null", KIT_VARTYPE_VEC##d, 1, 1, kit_builtins_vec_len },\
  { "vec"#d"::len2", "Get the squared length (magnitude) of a vector", "fn"#d" vec::len2(vec) -> float|null", KIT_VARTYPE_VEC##d, 1, 1, kit_builtins_vec_len2 },\
  { "vec"#d"::dist", "Get (euclidean) distance between two vectors", "fn vec::"#d"dist(v1,v2) -> float|null", KIT_VARTYPE_VEC##d, 2, 2, kit_builtins_vec_dist },\
  { "vec"#d"::dist2", "Get squared (euclidean) distance between two vectors", "fn vec"#d"::dist2(v1,v2) -> float|null", KIT_VARTYPE_VEC##d, 2, 2, kit_builtins_vec_dist2 },\
  { "vec"#d"::dot", "Get the dot product of two vectors.", "fn vec::"#d"dot(v1,v2) -> float", KIT_VARTYPE_VEC##d, 2, 2, kit_builtins_vec_dot },
  
  X(2)
  X(3)
  X(4)
#undef X

  { "vec3::zx", "Zero extend given vector up to a vec3. If input is vec4, truncates 4th dimension", "fn vec3::zx(vec) -> vec3", KIT_VARTYPE_VEC2|KIT_VARTYPE_VEC3|KIT_VARTYPE_VEC4, 1, 1, kit_builtins_vec3_zx },
  { "vec4::zx", "Zero extend given vector up to a vec4.", "fn vec4::zx(vec) -> vec4", KIT_VARTYPE_VEC2|KIT_VARTYPE_VEC3|KIT_VARTYPE_VEC4, 1, 1, kit_builtins_vec4_zx },
  { "vec3::cross", "Get the cross product of two vector3s.", "fn vec3::cross(v1,v2) -> vec3", KIT_VARTYPE_VEC3, 2, 2, kit_builtins_vec3_cross },

  { "time::now", "Get the number of seconds since unix epoch as a float", "fn time::now() -> float", 0, 0, 0, kit_builtins_time_now },
  { "time::mono", "Get the number of monotonic (stable) seconds since unix epoch as a float", "fn time::mono() -> float", 0, 0, 0, kit_builtins_time_mono },
  { "time::local", "Get a time::timestamp structure containing current system time", "fn time::local() -> time::timestamp", 0, 0, 0, kit_builtins_time_local },
  { "time::utc", "Get a time::timestamp structure containing UTC relative system time", "fn time::utc() -> time::timestamp", 0, 0, 0, kit_builtins_time_utc },

  { "kit::compile_and_exec", "Compile and execute the given kit::exec_info. Returns the return value", "fn kit::compile_and_exec(info : kit::exec_info) -> any", KIT_VARTYPE_STRUCT, 1, 1, kit_builtins_rt_compile_and_exec },
  { "kit::tokenize", "Tokenize the source file and get a list of kit::token's, null on failure", "fn kit::tokenize(source_file : string) -> []", KIT_VARTYPE_STRING, 1, 1, kit_builtins_rt_tokenize },
  { "kit::ast::init", "Create an AST, null on failure", "fn kit::ast::init() -> opaque/descriptor", KIT_VARTYPE_NULL, 0, 0, kit_builtins_rt_ast_init },
  { "kit::ast::free", "Free an AST", "fn kit::ast::free(ast) -> null", KIT_VARTYPE_NULL, 1, 1, kit_builtins_rt_ast_free },
  { "kit::parse", "Parse tokens into an AST. 0 on success, null on failure.", "fn kit::parse(ast, token_stream) -> int", KIT_VARTYPE_LIST, 2, 2, kit_builtins_rt_parse },
  { "kit::compile", "Compile an AST into an instance of kit::compiled_obj, null on failure", "fn kit::compile(info : kit::compile_info) -> kit::compiled_obj", KIT_VARTYPE_LIST, 1, 1, kit_builtins_rt_compile },
};
// clang-format on

#endif // KIT_BUILTIN_FUNCTIONS_H
