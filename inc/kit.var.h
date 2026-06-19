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

#ifndef KIT_VAR_H
#define KIT_VAR_H

#include "kit.list.h"
#include "kit.map.h"
#include "kit.mathstrucs.h"
#include "kit.pool.h"
#include "kit.stdafx.h"
#include "kit.string.h"
#include "kit.struct.h"

#define KIT_OBJ_AS_STRING(obj) ((kit_string*)((obj)->data))
#define KIT_OBJ_AS_LIST(obj) ((kit_list*)((obj)->data))
#define KIT_OBJ_AS_MAP(obj) ((kit_map*)((obj)->data))
#define KIT_OBJ_AS_STRUCT(obj) ((kit_struct*)((obj)->data))
#define KIT_OBJ_AS_MAT3(obj) ((kit_mat3*)((obj)->data))
#define KIT_OBJ_AS_MAT4(obj) ((kit_mat4*)((obj)->data))

#define KIT_VAR_AS_STRING(var) (((var)->type == KIT_VARTYPE_STRING) ? (kit_string*)((var)->val.s->data) : NULL)
#define KIT_VAR_AS_LIST(var) (((var)->type == KIT_VARTYPE_LIST) ? (kit_list*)((var)->val.list->data) : NULL)
#define KIT_VAR_AS_MAP(var) (((var)->type == KIT_VARTYPE_MAP) ? (kit_map*)((var)->val.map->data) : NULL)
#define KIT_VAR_AS_STRUCT(var) (((var)->type == KIT_VARTYPE_STRUCT) ? (kit_struct*)((var)->val.struc->data) : NULL)
#define KIT_VAR_AS_MAT3(var) (((var)->type == KIT_VARTYPE_MAT3) ? (kit_mat3*)((var)->val.mat3->data) : NULL)
#define KIT_VAR_AS_MAT4(var) (((var)->type == KIT_VARTYPE_MAT4) ? (kit_mat4*)((var)->val.mat4->data) : NULL)

struct kit_var;
struct kit_list;
struct kit_map;
struct kit_struct;
struct kit_refdobj_pool;

/**
 * Bitmask to allow functions to just check
 * the masks. No variable can have more than one
 * bit set though.
 */
typedef enum kit_var_type {
  KIT_VARTYPE_NULL       = 1 << 0, // only type tag matters for this
  KIT_VARTYPE_INT        = 1 << 2,
  KIT_VARTYPE_BOOL       = 1 << 3,
  KIT_VARTYPE_CHAR       = 1 << 4,
  KIT_VARTYPE_FLOAT      = 1 << 5,
  KIT_VARTYPE_STRING     = 1 << 6,
  KIT_VARTYPE_LIST       = 1 << 7,
  KIT_VARTYPE_MAP        = 1 << 8,
  KIT_VARTYPE_STRUCT     = 1 << 9,
  KIT_VARTYPE_VEC2       = 1 << 10, /* we have first class support for vectors. */
  KIT_VARTYPE_VEC3       = 1 << 11,
  KIT_VARTYPE_VEC4       = 1 << 12,
  KIT_VARTYPE_MAT3       = 1 << 13,
  KIT_VARTYPE_MAT4       = 1 << 14,
  KIT_VARTYPE_DESCRIPTOR = 1 << 15, // pointer, internally
} kit_var_type;

/**
 * Variable payload
 */
typedef union kit_var_payload {
  int  i;
  char c;
  bool b;

  /* No 32 bit floats :) */
  double f;

  kit_vec2 vec2;
  kit_vec3 vec3;
  kit_vec4 vec4;

  struct kit_refdobj* mat3;
  struct kit_refdobj* mat4;

  struct kit_refdobj* s;     // Use KIT_VAR_AS_STRING to access as kit_string*
  struct kit_refdobj* list;  // Use KIT_VAR_AS_LIST to access as kit_list*
  struct kit_refdobj* map;   // Use KIT_VAR_AS_MAP to access as kit_map*
  struct kit_refdobj* struc; // Use KIT_VAR_AS_STRUCT to access as kit_struct*

  void* descriptor;
} kit_var_payload;

typedef struct kit_var {
  kit_var_type    type;
  kit_var_payload val;
} kit_var;

/* defined here since we need definition of kit_var */
typedef struct kit_struct_member_pair {
  const char*    name;
  struct kit_var value;
} kit_struct_member_pair;

/* Constant */
#define KIT_NULLVAR                                                                                                                                  \
  (kit_var) { .type = KIT_VARTYPE_NULL }

kit_var kit_make_var_from_string(kit_refdobj_pool* object_pool, char* s);

int kit_var_shallow_cpy(const kit_var* var, kit_var* dst);
int kit_var_deep_cpy(kit_refdobj_pool* object_pool, const kit_var* var, kit_var* dst);

void   kit_var_print(const struct kit_var* v, FILE* f);
void   kit_var_to_string(const struct kit_var* v, char* buffer, size_t buffer_size);
size_t kit_var_to_string_size(const struct kit_var* v);

i32  kit_var_acquire(kit_refdobj_pool* object_pool, kit_var* v);
void kit_var_release(kit_refdobj_pool* object_pool, kit_var* v);

u32  kit_var_hash(const kit_var* var);
bool kit_var_equal(const kit_var* a, const kit_var* b);

/**
 * Index a container (excluding strings).
 * Strings *are* supported.
 */
int kit_var_index(kit_refdobj_pool* object_pool, const kit_var* base, const kit_var* index, kit_var* result);

/**
 * Set the element at the index given to the specified value.
 * Strings are *not* supported (they are immutable).
 */
int kit_var_index_assign(kit_refdobj_pool* object_pool, kit_var* base, const kit_var* index, const kit_var* value);

void kit_var_free(kit_refdobj_pool* object_pool, kit_var* var);

/**
 * Extend the given vector to be a vec4.
 * Fills the remaining slots with 0.
 */
int evector_zero_extend(const kit_var* v, kit_vec4 out);

/**
 * Like evector_zero_extent, but with support for casting
 * integral types to vec4s.
 * For use to simplify mathematical builtins
 */
int kit_create_vec4(const kit_var* v, kit_vec4 out);

kit_var evector_length(const kit_var* v);

static inline const char*
kit_var_type_to_string(kit_var_type type)
{
  switch (type) {
    case KIT_VARTYPE_INT: return "int";
    case KIT_VARTYPE_BOOL: return "bool";
    case KIT_VARTYPE_CHAR: return "char";
    case KIT_VARTYPE_FLOAT: return "float";
    case KIT_VARTYPE_STRING: return "string";
    case KIT_VARTYPE_LIST: return "list";
    case KIT_VARTYPE_MAP: return "map";
    case KIT_VARTYPE_NULL: return "null";
    case KIT_VARTYPE_STRUCT: return "struct";
    case KIT_VARTYPE_VEC2: return "vec2";
    case KIT_VARTYPE_VEC3: return "vec3";
    case KIT_VARTYPE_VEC4: return "vec4";
    case KIT_VARTYPE_MAT3: return "mat3";
    case KIT_VARTYPE_MAT4: return "mat4";
    case KIT_VARTYPE_DESCRIPTOR: return "descriptor";
  }
  return "unknown";
}

static inline kit_var
kit_make_vec4(double x, double y, double z, double w)
{
  kit_var out = {
    .type = KIT_VARTYPE_VEC4,
    .val  = { .vec4 = { x, y, z, w } },
  };
  return out;
}

static inline kit_var
kit_make_vec3(double x, double y, double z)
{
  kit_var out = {
    .type = KIT_VARTYPE_VEC3,
    .val  = { .vec3 = { x, y, z } },
  };
  return out;
}

static inline kit_var
kit_make_vec2(double x, double y)
{
  kit_var out = {
    .type = KIT_VARTYPE_VEC2,
    .val  = { .vec2 = { x, y } },
  };
  return out;
}

#endif // KIT_VAR__H
