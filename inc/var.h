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

#ifndef ESL_VAR_H
#define ESL_VAR_H

#include "list.h"
#include "map.h"
#include "mathstrucs.h"
#include "pool.h"
#include "stdafx.h"
#include "string.h"
#include "struct.h"

#define E_OBJ_AS_STRING(obj) ((e_string*)((obj)->data))
#define E_OBJ_AS_LIST(obj) ((e_list*)((obj)->data))
#define E_OBJ_AS_MAP(obj) ((e_map*)((obj)->data))
#define E_OBJ_AS_STRUCT(obj) ((e_struct*)((obj)->data))
#define E_OBJ_AS_MAT3(obj) ((e_mat3*)((obj)->data))
#define E_OBJ_AS_MAT4(obj) ((e_mat4*)((obj)->data))

#define E_VAR_AS_STRING(var) (((var)->type == E_VARTYPE_STRING) ? (e_string*)((var)->val.s->data) : NULL)
#define E_VAR_AS_LIST(var) (((var)->type == E_VARTYPE_LIST) ? (e_list*)((var)->val.list->data) : NULL)
#define E_VAR_AS_MAP(var) (((var)->type == E_VARTYPE_MAP) ? (e_map*)((var)->val.map->data) : NULL)
#define E_VAR_AS_STRUCT(var) (((var)->type == E_VARTYPE_STRUCT) ? (e_struct*)((var)->val.struc->data) : NULL)
#define E_VAR_AS_MAT3(var) (((var)->type == E_VARTYPE_MAT3) ? (e_mat3*)((var)->val.mat3->data) : NULL)
#define E_VAR_AS_MAT4(var) (((var)->type == E_VARTYPE_MAT4) ? (e_mat4*)((var)->val.mat4->data) : NULL)

struct e_var;
struct e_list;
struct e_map;
struct e_struct;
struct e_refdobj_pool;

/**
 * Bitmask to allow functions to just check
 * the masks. No variable can have more than one
 * bit set though.
 */
typedef enum e_vartype {
  E_VARTYPE_NULL       = 1 << 0, // only type tag matters for this
  E_VARTYPE_INT        = 1 << 2,
  E_VARTYPE_BOOL       = 1 << 3,
  E_VARTYPE_CHAR       = 1 << 4,
  E_VARTYPE_FLOAT      = 1 << 5,
  E_VARTYPE_STRING     = 1 << 6,
  E_VARTYPE_LIST       = 1 << 7,
  E_VARTYPE_MAP        = 1 << 8,
  E_VARTYPE_STRUCT     = 1 << 9,
  E_VARTYPE_VEC2       = 1 << 10, /* we have first class support for vectors. */
  E_VARTYPE_VEC3       = 1 << 11,
  E_VARTYPE_VEC4       = 1 << 12,
  E_VARTYPE_MAT3       = 1 << 13,
  E_VARTYPE_MAT4       = 1 << 14,
  E_VARTYPE_DESCRIPTOR = 1 << 15, // pointer, internally
} e_vartype;
// typedef u32 e_vartype;

/**
 * Variable payload
 */
typedef union e_varval {
  int  i;
  char c;
  bool b;

  /* No 32 bit floats :) */
  double f;

  e_vec2 vec2;
  e_vec3 vec3;
  e_vec4 vec4;

  struct e_refdobj* mat3;
  struct e_refdobj* mat4;

  struct e_refdobj* s;     // Use E_VAR_AS_STRING to access as e_string*
  struct e_refdobj* list;  // Use E_VAR_AS_LIST to access as e_list*
  struct e_refdobj* map;   // Use E_VAR_AS_MAP to access as e_map*
  struct e_refdobj* struc; // Use E_VAR_AS_STRUCT to access as e_struct*

  void* descriptor;
} e_varval;

typedef struct e_var {
  e_vartype type;
  e_varval  val;
} e_var;

typedef struct e_struct_member_pair {
  const char*  name;
  struct e_var value;
} e_struct_member_pair;

#define E_NULLVAR                                                                                                                                    \
  (e_var) { .type = E_VARTYPE_NULL }

e_var e_make_var_from_string(char* s);

int e_var_shallow_cpy(const e_var* var, e_var* dst);
int e_var_deep_cpy(const e_var* var, e_var* dst);

void   e_var_print(const struct e_var* v, FILE* f);
void   e_var_to_string(const struct e_var* v, char* buffer, size_t buffer_size);
size_t e_var_to_string_size(const struct e_var* v);

i32  e_var_acquire(e_var* v);
void e_var_release(e_var* v);

u32  e_var_hash(const e_var* var);
bool e_var_equal(const e_var* a, const e_var* b);

/**
 * Index a container (excluding strings).
 * strings are tightly packed so individual elements can not
 * be accessed.
 */
int e_var_index(const e_var* left, const e_var* right, e_var** result);

/**
 * Get a character from a string
 */
int e_str_index(e_string* s, int i, e_var* o);

void e_var_free(e_var* var);

/**
 * Extend the given vector to be a vec4.
 * Fills the remaining slots with 0.
 */
int evector_zero_extend(const e_var* v, e_vec4 out);

/**
 * Like evector_zero_extent, but with support for casting
 * integral types to vec4s.
 * For use to simplify mathematical builtins
 */
int e_create_vec4(const e_var* v, e_vec4 out);

e_var evector_length(const e_var* v);

static inline const char*
e_var_type_to_string(e_vartype type)
{
  switch (type) {
    case E_VARTYPE_INT: return "int";
    case E_VARTYPE_BOOL: return "bool";
    case E_VARTYPE_CHAR: return "char";
    case E_VARTYPE_FLOAT: return "float";
    case E_VARTYPE_STRING: return "string";
    case E_VARTYPE_LIST: return "list";
    case E_VARTYPE_MAP: return "map";
    case E_VARTYPE_NULL: return "null";
    case E_VARTYPE_STRUCT: return "struct";
    case E_VARTYPE_VEC2: return "vec2";
    case E_VARTYPE_VEC3: return "vec3";
    case E_VARTYPE_VEC4: return "vec4";
    case E_VARTYPE_MAT3: return "mat3";
    case E_VARTYPE_MAT4: return "mat4";
    case E_VARTYPE_DESCRIPTOR: return "descriptor";
  }
  return "unknown";
}

static inline e_var
e_make_vec4(double x, double y, double z, double w)
{
  e_var out = {
    .type = E_VARTYPE_VEC4,
    .val  = { .vec4 = { x, y, z, w } },
  };
  return out;
}

static inline e_var
e_make_vec3(double x, double y, double z)
{
  e_var out = {
    .type = E_VARTYPE_VEC3,
    .val  = { .vec3 = { x, y, z } },
  };
  return out;
}

static inline e_var
e_make_vec2(double x, double y)
{
  e_var out = {
    .type = E_VARTYPE_VEC2,
    .val  = { .vec2 = { x, y } },
  };
  return out;
}

#endif // ESL_H
