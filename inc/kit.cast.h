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

#ifndef KIT_VAR_CAST_H
#define KIT_VAR_CAST_H

#include "kit.var.h"

#include <string.h>

/**
 * Small and fast cast for integral types.
 * These casts are provided as an alternative to avoid calling
 * kit_builtins_cast_x, as those functions do everything in their power to
 * cast the given arguments (to and from strings, especially).
 * That isn't needed in most places.
 */

static inline int
kit_cast_to_int(const kit_var* v)
{
  switch (v->type) {
    case KIT_VARTYPE_NULL: return 0;
    case KIT_VARTYPE_INT: return v->val.i;
    case KIT_VARTYPE_FLOAT: return (int)v->val.f;
    case KIT_VARTYPE_CHAR: return (int)v->val.c;
    case KIT_VARTYPE_BOOL: return (int)v->val.b;
    default: return 0;
  }
}

static inline char
kit_cast_to_char(const kit_var* v)
{
  switch (v->type) {
    case KIT_VARTYPE_NULL: return 0;
    case KIT_VARTYPE_INT: return (char)v->val.i;
    case KIT_VARTYPE_FLOAT: return (char)v->val.f;
    case KIT_VARTYPE_CHAR: return (char)v->val.c;
    case KIT_VARTYPE_BOOL: return (char)v->val.b;
    default: return 0;
  }
}

static inline bool
kit_cast_to_bool(const kit_var* v)
{
  switch (v->type) {
    case KIT_VARTYPE_NULL: return false;
    case KIT_VARTYPE_INT: return (bool)v->val.i;
    case KIT_VARTYPE_FLOAT: return (bool)v->val.f;
    case KIT_VARTYPE_CHAR: return (bool)v->val.c;
    case KIT_VARTYPE_BOOL: return (bool)v->val.b;
    case KIT_VARTYPE_STRING: return strcmp(KIT_VAR_AS_STRING(v)->s, "") != 0;
    default: return false;
  }
}

static inline double
kit_cast_to_float(const kit_var* v)
{
  switch (v->type) {
    case KIT_VARTYPE_NULL: return 0.0;
    case KIT_VARTYPE_INT: return (double)v->val.i;
    case KIT_VARTYPE_CHAR: return (double)v->val.c;
    case KIT_VARTYPE_BOOL: return (double)v->val.b;
    case KIT_VARTYPE_FLOAT: return v->val.f;
    default: return 0;
  }
}

/* Help routines */

static inline kit_var
kit_var_from_int(int x)
{ return (kit_var){ .type = KIT_VARTYPE_INT, .val = { .i = x } }; }

static inline kit_var
kit_var_from_char(char x)
{ return (kit_var){ .type = KIT_VARTYPE_CHAR, .val = { .c = x } }; }

static inline kit_var
kit_var_from_bool(bool x)
{ return (kit_var){ .type = KIT_VARTYPE_BOOL, .val = { .b = x } }; }

static inline kit_var
kit_var_from_float(double x)
{ return (kit_var){ .type = KIT_VARTYPE_FLOAT, .val = { .f = x } }; }

static inline kit_var
kit_var_from_pointer(void* x)
{ return (kit_var){ .type = KIT_VARTYPE_DESCRIPTOR, .val = { .descriptor = x } }; }

/* The actual cast functions like int(), string(), etc. Do everything in your power to represent it in the other type. */

static inline int
kit_var_to_int(kit_var v)
{
  switch (v.type) {
    case KIT_VARTYPE_NULL: return 0;
    case KIT_VARTYPE_INT: return v.val.i;
    case KIT_VARTYPE_FLOAT: return (int)v.val.f;
    case KIT_VARTYPE_CHAR: return (int)v.val.c;
    case KIT_VARTYPE_BOOL: return (int)v.val.b;
    case KIT_VARTYPE_STRING: {
      /**
       * TIL, if you pass 0 to the base argument
       * for strtol or its kind, they will automatically
       * handle the base.
       */
      const int base = 0;

      char*     end = NULL;
      long long x   = strtol(KIT_VAR_AS_STRING(&v)->s, &end, base);
      if (KIT_VAR_AS_STRING(&v)->s == end) return -1;

      return (int)x;
    }
    case KIT_VARTYPE_LIST: return KIT_VAR_AS_LIST(&v)->size;
    case KIT_VARTYPE_MAP: return KIT_VAR_AS_MAP(&v)->size;
    case KIT_VARTYPE_STRUCT: return KIT_VAR_AS_STRUCT(&v)->member_count;
    case KIT_VARTYPE_DESCRIPTOR: return -1;
    default: return 0;
  }
}

static inline double
kit_var_to_float(kit_var v)
{
  switch (v.type) {
    case KIT_VARTYPE_NULL: return 0.0;
    case KIT_VARTYPE_FLOAT: return v.val.f;
    case KIT_VARTYPE_INT: return (double)v.val.i;
    case KIT_VARTYPE_CHAR: return (double)v.val.c;
    case KIT_VARTYPE_BOOL: return (double)v.val.b;
    case KIT_VARTYPE_STRING: {
      char*  end = NULL;
      double d   = strtod(KIT_VAR_AS_STRING(&v)->s, &end);
      if (end == KIT_VAR_AS_STRING(&v)->s) { return 0.0; }
      return d;
    }
    case KIT_VARTYPE_DESCRIPTOR: return -1.0;

    default: return (double)kit_var_to_int(v);
  }
}

static inline bool
kit_var_to_bool(kit_var v)
{
  switch (v.type) {
    case KIT_VARTYPE_NULL: return false;
    case KIT_VARTYPE_INT: return (bool)v.val.i;
    case KIT_VARTYPE_FLOAT: return (bool)v.val.f;
    case KIT_VARTYPE_CHAR: return (bool)v.val.c;
    case KIT_VARTYPE_BOOL: return v.val.b;
    // referenced structures always evaluate to true
    case KIT_VARTYPE_STRING:
    case KIT_VARTYPE_LIST:
    case KIT_VARTYPE_MAP:
    case KIT_VARTYPE_MAT3:
    case KIT_VARTYPE_MAT4:
    case KIT_VARTYPE_STRUCT: return true;
    case KIT_VARTYPE_VEC2: return (v.val.vec2[0] != 0 && v.val.vec2[1] != 0);
    case KIT_VARTYPE_VEC3: return (v.val.vec3[0] != 0 && v.val.vec3[1] != 0 && v.val.vec3[2] != 0);
    case KIT_VARTYPE_VEC4: return (v.val.vec4[0] != 0 && v.val.vec4[1] != 0 && v.val.vec4[2] != 0 && v.val.vec4[3] != 0);
    case KIT_VARTYPE_DESCRIPTOR: return v.val.descriptor != NULL;
    default: /* error*/ return false;
  }
}

#endif // KIT_VAR_CAST_H