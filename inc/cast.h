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

#ifndef E_VAR_CAST_H
#define E_VAR_CAST_H

#include "var.h"

#include <string.h>

/**
 * Small and fast cast for integral types.
 * These casts are provided as an alternative to avoid calling
 * eb_cast_x, as those functions do everything in their power to
 * cast the given arguments (to and from strings, especially).
 * That isn't needed in most places.
 */

static inline int
e_cast_to_int(const e_var* v)
{
  switch (v->type) {
    case E_VARTYPE_NULL: return 0;
    case E_VARTYPE_INT: return v->val.i;
    case E_VARTYPE_FLOAT: return (int)v->val.f;
    case E_VARTYPE_CHAR: return (int)v->val.c;
    case E_VARTYPE_BOOL: return (int)v->val.b;
    default: return 0;
  }
}

static inline char
e_cast_to_char(const e_var* v)
{
  switch (v->type) {
    case E_VARTYPE_NULL: return 0;
    case E_VARTYPE_INT: return (char)v->val.i;
    case E_VARTYPE_FLOAT: return (char)v->val.f;
    case E_VARTYPE_CHAR: return (char)v->val.c;
    case E_VARTYPE_BOOL: return (char)v->val.b;
    default: return 0;
  }
}

static inline bool
e_cast_to_bool(const e_var* v)
{
  switch (v->type) {
    case E_VARTYPE_NULL: return false;
    case E_VARTYPE_INT: return (bool)v->val.i;
    case E_VARTYPE_FLOAT: return (bool)v->val.f;
    case E_VARTYPE_CHAR: return (bool)v->val.c;
    case E_VARTYPE_BOOL: return (bool)v->val.b;
    case E_VARTYPE_STRING: return strcmp(E_VAR_AS_STRING(v)->s, "") != 0;
    default: return false;
  }
}

static inline double
e_cast_to_float(const e_var* v)
{
  switch (v->type) {
    case E_VARTYPE_NULL: return 0.0;
    case E_VARTYPE_INT: return (double)v->val.i;
    case E_VARTYPE_CHAR: return (double)v->val.c;
    case E_VARTYPE_BOOL: return (double)v->val.b;
    case E_VARTYPE_FLOAT: return v->val.f;
    default: return 0;
  }
}

/* Help routines */

static inline e_var
e_var_from_int(int x)
{ return (e_var){ .type = E_VARTYPE_INT, .val = { .i = x } }; }

static inline e_var
e_var_from_char(char x)
{ return (e_var){ .type = E_VARTYPE_CHAR, .val = { .c = x } }; }

static inline e_var
e_var_from_bool(bool x)
{ return (e_var){ .type = E_VARTYPE_BOOL, .val = { .b = x } }; }

static inline e_var
e_var_from_float(double x)
{ return (e_var){ .type = E_VARTYPE_FLOAT, .val = { .f = x } }; }

static inline e_var
e_var_from_pointer(void* x)
{ return (e_var){ .type = E_VARTYPE_DESCRIPTOR, .val = { .descriptor = x } }; }

/* The actual cast functions like int(), string(), etc. Do everything in your power to represent it in the other type. */

static inline int
e_var_to_int(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_NULL: return 0;
    case E_VARTYPE_INT: return v.val.i;
    case E_VARTYPE_FLOAT: return (int)v.val.f;
    case E_VARTYPE_CHAR: return (int)v.val.c;
    case E_VARTYPE_BOOL: return (int)v.val.b;
    case E_VARTYPE_STRING: {
      const int base = 10;

      char* end = NULL;
      int   x   = (int)strtol(E_VAR_AS_STRING(&v)->s, &end, base);
      if (E_VAR_AS_STRING(&v)->s == end) return -1;
      return x;
    }
    case E_VARTYPE_LIST: return E_VAR_AS_LIST(&v)->size;
    case E_VARTYPE_MAP: return E_VAR_AS_MAP(&v)->size;
    case E_VARTYPE_STRUCT: return E_VAR_AS_STRUCT(&v)->member_count;
    case E_VARTYPE_DESCRIPTOR: return -1;
    default: return 0;
  }
}

static inline double
e_var_to_float(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_NULL: return 0.0;
    case E_VARTYPE_FLOAT: return v.val.f;
    case E_VARTYPE_INT: return (double)v.val.i;
    case E_VARTYPE_CHAR: return (double)v.val.c;
    case E_VARTYPE_BOOL: return (double)v.val.b;
    case E_VARTYPE_STRING: {
      char*  end = NULL;
      double d   = strtod(E_VAR_AS_STRING(&v)->s, &end);
      if (end == E_VAR_AS_STRING(&v)->s) { return 0.0; }
      return d;
    }
    case E_VARTYPE_DESCRIPTOR: return -1.0;

    default: return (double)e_var_to_int(v);
  }
}

static inline bool
e_var_to_bool(e_var v)
{
  switch (v.type) {
    case E_VARTYPE_NULL: return false;
    case E_VARTYPE_INT: return (bool)v.val.i;
    case E_VARTYPE_FLOAT: return (bool)v.val.f;
    case E_VARTYPE_CHAR: return (bool)v.val.c;
    case E_VARTYPE_BOOL: return v.val.b;
    // referenced structures always evaluate to true
    case E_VARTYPE_STRING:
    case E_VARTYPE_LIST:
    case E_VARTYPE_MAP:
    case E_VARTYPE_MAT3:
    case E_VARTYPE_MAT4:
    case E_VARTYPE_STRUCT: return true;
    case E_VARTYPE_VEC2: return (v.val.vec2[0] != 0 && v.val.vec2[1] != 0);
    case E_VARTYPE_VEC3: return (v.val.vec3[0] != 0 && v.val.vec3[1] != 0 && v.val.vec3[2] != 0);
    case E_VARTYPE_VEC4: return (v.val.vec4[0] != 0 && v.val.vec4[1] != 0 && v.val.vec4[2] != 0 && v.val.vec4[3] != 0);
    case E_VARTYPE_DESCRIPTOR: return v.val.descriptor != NULL;
    default: /* error*/ return false;
  }
}

#endif // E_VAR_CAST_H