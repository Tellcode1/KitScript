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

#ifndef KIT_VECTOR_BUILTINS_H
#define KIT_VECTOR_BUILTINS_H

#include "kit.cast.h"
#include "kit.mathstrucs.h"
#include "kit.perr.h"
#include "kit.var.h"
#include "kit.vm.h"

#include <math.h>

static inline kit_ecode
kit_builtins_vec_norm(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  double len = evector_length(&args[0]).val.f;

  kit_vec4 zx;
  evector_zero_extend(&args[0], zx);

  (void)nargs;
  switch (args[0].type) {
    case KIT_VARTYPE_VEC2: *result = kit_make_vec2(zx[0] / len, zx[1] / len); return KIT_OK;
    case KIT_VARTYPE_VEC3: *result = kit_make_vec3(zx[0] / len, zx[1] / len, zx[2] / len); return KIT_OK;
    case KIT_VARTYPE_VEC4: *result = kit_make_vec4(zx[0] / len, zx[1] / len, zx[2] / len, zx[3] / len); return KIT_OK;
    default: break;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec_len(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  switch (args[0].type) {
    case KIT_VARTYPE_VEC2:
    case KIT_VARTYPE_VEC3:
    case KIT_VARTYPE_VEC4: *result = evector_length(&args[0]); return KIT_OK;
    default: break;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec_len2(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_vec4 zx;
  evector_zero_extend(&args[0], zx);

  switch (args[0].type) {
    case KIT_VARTYPE_VEC2: *result = kit_var_from_float((zx[0] * zx[0]) + (zx[1] * zx[1])); return KIT_OK;
    case KIT_VARTYPE_VEC3: *result = kit_var_from_float((zx[0] * zx[0]) + (zx[1] * zx[1]) + (zx[2] * zx[2])); return KIT_OK;
    case KIT_VARTYPE_VEC4: *result = kit_var_from_float((zx[0] * zx[0]) + (zx[1] * zx[1]) + (zx[2] * zx[2]) + (zx[3] * zx[3])); return KIT_OK;
    default: break;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec_dist(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_vec4 x;
  kit_vec4 y;

  evector_zero_extend(&args[0], x);
  evector_zero_extend(&args[1], y);

  double xl = x[0] - y[0];
  double yl = x[1] - y[1];
  double zl = x[2] - y[2]; // 0 if not vec3, zero extended
  double wl = x[3] - y[3]; // 0 if not vec4, zero extended

  switch (args[0].type) {
    case KIT_VARTYPE_VEC2:
    case KIT_VARTYPE_VEC3:
    case KIT_VARTYPE_VEC4: *result = kit_var_from_float(sqrt(xl + yl + zl + wl)); return KIT_OK;
    default: break;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec_dist2(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_vec4 x;
  kit_vec4 y;
  evector_zero_extend(&args[0], x);
  evector_zero_extend(&args[1], y);

  double xl = x[0] - y[0];
  double yl = x[1] - y[1];
  double zl = x[2] - y[2]; // 0 if not vec3, zero extended
  double wl = x[3] - y[3]; // 0 if not vec4, zero extended

  switch (args[0].type) {
    case KIT_VARTYPE_VEC2:
    case KIT_VARTYPE_VEC3:
    case KIT_VARTYPE_VEC4: *result = kit_var_from_float(xl + yl + zl + wl); return KIT_OK;
    default: break;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec3_zx(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_vec4 x;
  evector_zero_extend(&args[0], x);
  *result = (kit_var){ .type = KIT_VARTYPE_VEC3, .val = { .vec3 = KIT_VEC3_INIT(x) } };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec4_zx(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_vec4 x;
  evector_zero_extend(&args[0], x);
  *result = (kit_var){ .type = KIT_VARTYPE_VEC3, .val = { .vec4 = KIT_VEC4_INIT(x) } };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec_dot(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_vec4 x;
  evector_zero_extend(&args[0], x);
  kit_vec4 y;
  evector_zero_extend(&args[1], y);

  double r = 0.0;
  for (u32 i = 0; i < 4; i++) { r += x[i] * y[i]; }

  *result = (kit_var){ .type = KIT_VARTYPE_FLOAT, .val = { .f = r } };
  return KIT_OK;
}

static inline kit_ecode
kit_builtins_vec3_cross(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_vec3 x;
  kit_vec3 y;
  memcpy(x, args[0].val.vec3, sizeof(kit_vec3));
  memcpy(y, args[1].val.vec3, sizeof(kit_vec3));

  kit_vec3 r;
  r[0] = (x[1] * y[2]) - (x[2] * y[1]);
  r[1] = (x[2] * y[0]) - (x[0] * y[2]);
  r[2] = (x[0] * y[1]) - (x[1] * y[0]);

  *result = (kit_var){ .type = KIT_VARTYPE_VEC3, .val = { .vec3 = KIT_VEC3_INIT(r) } };
  return KIT_OK;
}

#endif // KIT_VECTOR_BUILTINS_H