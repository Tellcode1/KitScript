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

#ifndef KIT_MATH_BUILTIN_FUNCTIONS_H
#define KIT_MATH_BUILTIN_FUNCTIONS_H

#include "kit.cast.h"
#include "kit.perr.h"
#include "kit.var.h"
#include "kit.vm.h"

#include <math.h>

// clang-format off
static inline double ex_clamp(double x, double min, double max)  {
   return fmax(min, fmin(x, max));
}
static inline double ex_lerp(double a, double b, double t)  {
   return a + ((b-a) * t);
}
static inline kit_ecode kit_builtins_sin(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = sin(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_cos(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = cos(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_tan(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = tan(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_asin(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = asin(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_acos(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = acos(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_atan(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = atan(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_atan2(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = atan2(kit_cast_to_float(&args[0]),kit_cast_to_float(&args[1]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_sinh(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = sinh(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_cosh(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = cosh(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_tanh(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = tanh(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_acosh(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = acosh(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_asinh(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = asinh(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_atanh(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = atanh(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_exp(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = exp(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_log(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = log(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_log10(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = log10(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_pow(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = pow(kit_cast_to_float(&args[0]), kit_cast_to_float(&args[1]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_sqrt(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = sqrt(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_ceil(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = ceil(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_floor(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = floor(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_fmod(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = fmod(kit_cast_to_float(&args[0]),kit_cast_to_float(&args[1]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_round(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = round(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_trunc(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = trunc(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_abs(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = fabs(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_hypot(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = hypot(kit_cast_to_float(&args[0]), kit_cast_to_float(&args[1]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_signbit(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_INT, .val = {.i = signbit(kit_cast_to_float(&args[0]))}}; return KIT_OK; }
static inline kit_ecode kit_builtins_deg2rad(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = kit_cast_to_float(&args[0]) * 3.14159265358979323846 / 180.0}}; return KIT_OK; }
static inline kit_ecode kit_builtins_rad2deg(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = {.f = kit_cast_to_float(&args[0]) * 180.0 / 3.14159265358979323846}}; return KIT_OK; }
static inline kit_ecode kit_builtins_min(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = { .f = fmin(kit_cast_to_float(&args[0]), kit_cast_to_float(&args[1])) }}; return KIT_OK; }
static inline kit_ecode kit_builtins_max(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = { .f = fmax(kit_cast_to_float(&args[0]), kit_cast_to_float(&args[1])) }}; return KIT_OK; }
static inline kit_ecode kit_builtins_clamp(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result) { (void)nargs; *result =  (kit_var){.type = KIT_VARTYPE_FLOAT, .val = { .f = ex_clamp(kit_cast_to_float(&args[0]), kit_cast_to_float(&args[1]), kit_cast_to_float(&args[2])) }}; return KIT_OK; }
// clang-format on

/* constructors */
kit_ecode kit_builtins_vec2(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_vec3(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_vec4(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_mat3(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_mat4(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

/* interpolation */
kit_ecode kit_builtins_smoothstep(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_lerp(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

#endif // KIT_MATH_BUILTIN_FUNCTIONS_H