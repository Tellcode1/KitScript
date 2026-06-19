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

#include "../../inc/kit.cast.h"
#include "../../inc/kit.mathstrucs.h"
#include "../../inc/kit.perr.h"
#include "../../inc/kit.pool.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.var.h"
#include "../../inc/kit.vm.h"

#include <assert.h>
#include <math.h>

kit_ecode
kit_builtins_vec2(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_VEC2, .val.vec2 = { [0] = kit_cast_to_float(&args[0]), [1] = kit_cast_to_float(&args[1]) } };
  return KIT_OK;
}

kit_ecode
kit_builtins_vec3(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type     = KIT_VARTYPE_VEC3,
                       .val.vec3 = { [0] = kit_cast_to_float(&args[0]), [1] = kit_cast_to_float(&args[1]), [2] = kit_cast_to_float(&args[2]) } };
  return KIT_OK;
}

kit_ecode
kit_builtins_vec4(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type     = KIT_VARTYPE_VEC4,
                       .val.vec4 = { [0] = kit_cast_to_float(&args[0]),
                                     [1] = kit_cast_to_float(&args[1]),
                                     [2] = kit_cast_to_float(&args[2]),
                                     [3] = kit_cast_to_float(&args[3]) } };
  return KIT_OK;
}

kit_ecode
kit_builtins_vec2_zero(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_VEC2, .val.vec4 = { 0 } };
  return KIT_OK;
}
kit_ecode
kit_builtins_vec3_zero(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_VEC3, .val.vec4 = { 0 } };
  return KIT_OK;
}
kit_ecode
kit_builtins_vec4_zero(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_VEC4, .val.vec4 = { 0 } };
  return KIT_OK;
}

kit_ecode
kit_builtins_mat3(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  kit_var m = {
    .type     = KIT_VARTYPE_MAT3,
    .val.mat3 = kit_refdobj_pool_acquire(vm->pool),
  };
  for (u32 i = 0; i < 3; i++) { memcpy(KIT_VAR_AS_MAT3(&m)->m[i], &args[i].val.vec3, sizeof(kit_vec3)); }
  *result = m;
  return KIT_OK;
}

kit_ecode
kit_builtins_mat4(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  kit_var m = {
    .type     = KIT_VARTYPE_MAT4,
    .val.mat4 = kit_refdobj_pool_acquire(vm->pool),
  };
  for (u32 i = 0; i < 4; i++) { memcpy(KIT_VAR_AS_MAT4(&m)->m[i], &args[i].val.vec4, sizeof(kit_vec4)); }
  *result = m;
  return KIT_OK;
}

int
evector_zero_extend(const kit_var* v, kit_vec4 out)
{
  if (v->type == KIT_VARTYPE_VEC2) {
    kit_vec4 x = { v->val.vec2[0], v->val.vec2[1], 0.0, 0.0 };
    memcpy(out, x, sizeof(kit_vec4));
  } else if (v->type == KIT_VARTYPE_VEC3) {
    kit_vec4 x = { v->val.vec3[0], v->val.vec3[1], v->val.vec3[2], 0.0 };
    memcpy(out, x, sizeof(kit_vec4));
  } else if (v->type == KIT_VARTYPE_VEC4) {
    memcpy(out, v->val.vec4, sizeof(kit_vec4));
  } else {
    kit_vec4 x = { 0.0, 0.0, 0.0, 0.0 };
    memcpy(out, x, sizeof(kit_vec4));
  }

  return KIT_OK;
}

int
kit_create_vec4(const kit_var* v, kit_vec4 out)
{
  if (v->type == KIT_VARTYPE_VEC2) {
    kit_vec4 x = { v->val.vec2[0], v->val.vec2[1], 0.0, 0.0 };
    memcpy(out, x, sizeof(kit_vec4));
  } else if (v->type == KIT_VARTYPE_VEC3) {
    kit_vec4 x = { v->val.vec3[0], v->val.vec3[1], v->val.vec3[2], 0.0 };
    memcpy(out, x, sizeof(kit_vec4));
  } else if (v->type == KIT_VARTYPE_VEC4) {
    memcpy(out, v->val.vec4, sizeof(kit_vec4));
  } else if (v->type == KIT_VARTYPE_INT || v->type == KIT_VARTYPE_CHAR || v->type == KIT_VARTYPE_BOOL) {
    int      x = kit_var_to_int(*v);
    kit_vec4 y = { x, 0.0, 0.0, 0.0 };
    memcpy(out, y, sizeof(kit_vec4));
  } else if (v->type == KIT_VARTYPE_FLOAT) {
    kit_vec4 y = { v->val.f, 0.0, 0.0, 0.0 };
    memcpy(out, y, sizeof(kit_vec4));
  } else {
    kit_vec4 x = { 0.0, 0.0, 0.0, 0.0 };
    memcpy(out, x, sizeof(kit_vec4));
  }
  return KIT_OK;
}

kit_var
evector_length(const kit_var* v)
{
  kit_vec4 x;
  evector_zero_extend(v, x);

  return (kit_var){ .type = KIT_VARTYPE_FLOAT, .val.f = sqrt((x[0] * x[0]) + (x[1] * x[1]) + (x[2] * x[2]) + (x[3] * x[3])) };
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
smoothstep_vec(const kit_vec4 a, const kit_vec4 b, kit_vec4 r, double x)
{
  for (int i = 0; i < 4; i++) { r[i] = smoothstep(a[i], b[i], x); }
}

void
larp_vec(const kit_vec4 a, const kit_vec4 b, kit_vec4 r, double x)
{
  for (int i = 0; i < 4; i++) { r[i] = a[i] + ((b[i] - a[i]) * x); }
}

kit_ecode
kit_builtins_smoothstep(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_vec4 a;
  kit_vec4 b;
  kit_vec4 r;
  kit_create_vec4(&args[0], a);
  kit_create_vec4(&args[1], b);

  smoothstep_vec(a, b, r, kit_var_to_float(args[2]));
  kit_var x = { .type = MAX(args[0].type, args[1].type) };
  memcpy(x.val.vec4, r, sizeof(r));

  *result = x;
  return KIT_OK;
}

kit_ecode
kit_builtins_lerp(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  kit_vec4 a;
  kit_vec4 b;
  kit_vec4 r;
  kit_create_vec4(&args[0], a);
  kit_create_vec4(&args[1], b);

  larp_vec(a, b, r, kit_var_to_float(args[2]));
  kit_var x = { .type = MAX(args[0].type, args[1].type) };
  memcpy(x.val.vec4, r, sizeof(r));

  *result = x;
  return KIT_OK;
}
