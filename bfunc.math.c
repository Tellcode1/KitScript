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

#include "cast.h"
#include "mathstrucs.h"
#include "pool.h"
#include "stdafx.h"
#include "var.h"

#include <assert.h>
#include <math.h>

e_var
eb_vec2(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC2, .val.vec2 = { [0] = e_cast_to_float(&args[0]), [1] = e_cast_to_float(&args[1]) } };
}

e_var
eb_vec3(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type     = E_VARTYPE_VEC3,
                  .val.vec3 = { [0] = e_cast_to_float(&args[0]), [1] = e_cast_to_float(&args[1]), [2] = e_cast_to_float(&args[2]) } };
}

e_var
eb_vec4(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){
    .type     = E_VARTYPE_VEC4,
    .val.vec4 = { [0] = e_cast_to_float(&args[0]), [1] = e_cast_to_float(&args[1]), [2] = e_cast_to_float(&args[2]), [3] = e_cast_to_float(&args[3]) }
  };
}

e_var
eb_vec2_zero(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC2, .val.vec4 = { 0 } };
}
e_var
eb_vec3_zero(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC3, .val.vec4 = { 0 } };
}
e_var
eb_vec4_zero(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC4, .val.vec4 = { 0 } };
}

e_var
eb_mat3(e_var* args, u32 nargs)
{
  (void)nargs;

  e_var m = {
    .type     = E_VARTYPE_MAT3,
    .val.mat3 = e_refdobj_pool_acquire(&ge_pool),
  };
  for (u32 i = 0; i < 3; i++) { memcpy(E_VAR_AS_MAT3(&m)->m[i], &args[i].val.vec3, sizeof(e_vec3)); }
  return m;
}

e_var
eb_mat4(e_var* args, u32 nargs)
{
  (void)nargs;

  e_var m = {
    .type     = E_VARTYPE_MAT4,
    .val.mat4 = e_refdobj_pool_acquire(&ge_pool),
  };
  for (u32 i = 0; i < 4; i++) { memcpy(E_VAR_AS_MAT4(&m)->m[i], &args[i].val.vec4, sizeof(e_vec4)); }
  return m;
}

int
evector_zero_extend(const e_var* v, e_vec4 out)
{
  if (v->type == E_VARTYPE_VEC2) {
    e_vec4 x = { v->val.vec2[0], v->val.vec2[1], 0.0, 0.0 };
    memcpy(out, x, sizeof(e_vec4));
  } else if (v->type == E_VARTYPE_VEC3) {
    e_vec4 x = { v->val.vec3[0], v->val.vec3[1], v->val.vec3[2], 0.0 };
    memcpy(out, x, sizeof(e_vec4));
  } else if (v->type == E_VARTYPE_VEC4) {
    memcpy(out, v->val.vec4, sizeof(e_vec4));
  } else {
    e_vec4 x = { 0.0, 0.0, 0.0, 0.0 };
    memcpy(out, x, sizeof(e_vec4));
  }

  return 0;
}

int
e_create_vec4(const e_var* v, e_vec4 out)
{
  if (v->type == E_VARTYPE_VEC2) {
    e_vec4 x = { v->val.vec2[0], v->val.vec2[1], 0.0, 0.0 };
    memcpy(out, x, sizeof(e_vec4));
  } else if (v->type == E_VARTYPE_VEC3) {
    e_vec4 x = { v->val.vec3[0], v->val.vec3[1], v->val.vec3[2], 0.0 };
    memcpy(out, x, sizeof(e_vec4));
  } else if (v->type == E_VARTYPE_VEC4) {
    memcpy(out, v->val.vec4, sizeof(e_vec4));
  } else if (v->type == E_VARTYPE_INT || v->type == E_VARTYPE_CHAR || v->type == E_VARTYPE_BOOL) {
    int    x = e_var_to_int(*v);
    e_vec4 y = { x, 0.0, 0.0, 0.0 };
    memcpy(out, y, sizeof(e_vec4));
  } else if (v->type == E_VARTYPE_FLOAT) {
    e_vec4 y = { v->val.f, 0.0, 0.0, 0.0 };
    memcpy(out, y, sizeof(e_vec4));
  } else {
    e_vec4 x = { 0.0, 0.0, 0.0, 0.0 };
    memcpy(out, x, sizeof(e_vec4));
  }
  return 0;
}

e_var
evector_length(const e_var* v)
{
  e_vec4 x;
  evector_zero_extend(v, x);

  return (e_var){ .type = E_VARTYPE_FLOAT, .val.f = sqrt((x[0] * x[0]) + (x[1] * x[1]) + (x[2] * x[2]) + (x[3] * x[3])) };
}

double
clamp(double x, double l, double h)
{
  if (x < l) return l;
  if (x > h) return h;
  return x;
}

double
smoothstep(double a, double b, double x)
{
  double t = clamp((x - a) / (b - a), 0.0, 1.0);
  return t * t * (3.0 - (2.0 * t));
}

void
smoothstep_vec(const e_vec4 a, const e_vec4 b, e_vec4 r, double x)
{
  for (int i = 0; i < 4; i++) { r[i] = smoothstep(a[i], b[i], x); }
}

void
larp_vec(const e_vec4 a, const e_vec4 b, e_vec4 r, double x)
{
  for (int i = 0; i < 4; i++) { r[i] = a[i] + ((b[i] - a[i]) * x); }
}

e_var
eb_smoothstep(e_var* args, u32 nargs)
{
  e_vec4 a;
  e_vec4 b;
  e_vec4 r;
  e_create_vec4(&args[0], a);
  e_create_vec4(&args[1], b);

  smoothstep_vec(a, b, r, e_var_to_float(args[2]));
  e_var x = { .type = MAX(args[0].type, args[1].type) };
  memcpy(x.val.vec4, r, sizeof(r));

  return x;
}

e_var
eb_lerp(e_var* args, u32 nargs)
{
  (void)nargs;

  e_vec4 a;
  e_vec4 b;
  e_vec4 r;
  e_create_vec4(&args[0], a);
  e_create_vec4(&args[1], b);

  larp_vec(a, b, r, e_var_to_float(args[2]));
  e_var x = { .type = MAX(args[0].type, args[1].type) };
  memcpy(x.val.vec4, r, sizeof(r));

  return x;
}
