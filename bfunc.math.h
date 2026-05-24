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

#ifndef E_MATH_BUILTIN_FUNCTIONS_H
#define E_MATH_BUILTIN_FUNCTIONS_H

#include "cast.h"
#include "var.h"

#include <math.h>

// clang-format off
static inline double ex_clamp(double x, double min, double max)  {
   return fmax(min, fmin(x, max));
}
static inline double ex_lerp(double a, double b, double t)  {
   return a + ((b-a) * t);
}
static inline e_var eb_sin(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sin(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_cos(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = cos(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_tan(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = tan(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_asin(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = asin(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_acos(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = acos(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_atan(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atan(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_atan2(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atan2(e_cast_to_float(&args[0]),e_cast_to_float(&args[1]))}}; }
static inline e_var eb_sinh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sinh(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_cosh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = cosh(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_tanh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = tanh(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_acosh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = acosh(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_asinh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = asinh(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_atanh(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = atanh(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_exp(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = exp(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_log(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = log(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_log10(e_var* args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = log10(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_pow(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = pow(e_cast_to_float(&args[0]), e_cast_to_float(&args[1]))}}; }
static inline e_var eb_sqrt(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = sqrt(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_ceil(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = ceil(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_floor(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = floor(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_fmod(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = fmod(e_cast_to_float(&args[0]),e_cast_to_float(&args[1]))}}; }
static inline e_var eb_round(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = round(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_trunc(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = trunc(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_abs(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = fabs(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_hypot(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = hypot(e_cast_to_float(&args[0]), e_cast_to_float(&args[1]))}}; }
static inline e_var eb_signbit(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_INT, .val = {.i = signbit(e_cast_to_float(&args[0]))}}; }
static inline e_var eb_deg2rad(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = e_cast_to_float(&args[0]) * 3.14159265358979323846 / 180.0}}; }
static inline e_var eb_rad2deg(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = {.f = e_cast_to_float(&args[0]) * 180.0 / 3.14159265358979323846}}; }
static inline e_var eb_min(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = { .f = fmin(e_cast_to_float(&args[0]), e_cast_to_float(&args[1])) }}; }
static inline e_var eb_max(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = { .f = fmax(e_cast_to_float(&args[0]), e_cast_to_float(&args[1])) }}; }
static inline e_var eb_clamp(e_var*args, u32 nargs) { (void)nargs; return (e_var){.type = E_VARTYPE_FLOAT, .val = { .f = ex_clamp(e_cast_to_float(&args[0]), e_cast_to_float(&args[1]), e_cast_to_float(&args[2])) }}; }
// clang-format on

/* constructors */
e_var eb_vec2(e_var* args, u32 nargs);
e_var eb_vec3(e_var* args, u32 nargs);
e_var eb_vec4(e_var* args, u32 nargs);
e_var eb_mat3(e_var* args, u32 nargs);
e_var eb_mat4(e_var* args, u32 nargs);
e_var eb_smoothstep(e_var* args, u32 nargs);
e_var eb_lerp(e_var* args, u32 nargs);

#endif // E_MATH_BUILTIN_FUNCTIONS_H