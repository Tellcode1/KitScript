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

#ifndef KIT_OPERATE_H
#define KIT_OPERATE_H

#include "kit.cast.h"
#include "kit.ir.h"
#include "kit.var.h"

#define BINOP(l, r, op)                                                                                                                              \
  (l).type == KIT_VARTYPE_NULL || (r).type == KIT_VARTYPE_NULL ? KIT_NULLVAR                                                                         \
      : ((l).type == KIT_VARTYPE_FLOAT || (r).type == KIT_VARTYPE_FLOAT)                                                                             \
      ? (kit_var){ .type = KIT_VARTYPE_FLOAT, .val.f = (double)(l).val.f op(double)(r).val.f }                                                       \
      : (kit_var)                                                                                                                                    \
  { .type = KIT_VARTYPE_INT, .val.i = (l).val.i op(r).val.i }
#define BOOLEAN_BINOP(l, r, op)                                                                                                                      \
  (l).type == KIT_VARTYPE_NULL || (r).type == KIT_VARTYPE_NULL ? KIT_NULLVAR                                                                         \
      : ((l).type == KIT_VARTYPE_FLOAT || (r).type == KIT_VARTYPE_FLOAT)                                                                             \
      ? (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = (double)(l).val.f op(double)(r).val.f }                                                        \
      : (kit_var)                                                                                                                                    \
  { .type = KIT_VARTYPE_BOOL, .val.b = (l).val.i op(r).val.i }

static inline bool
is_float(kit_var v)
{ return v.type == KIT_VARTYPE_FLOAT; }

#define COERCE_BINOP(l, r, op)                                                                                                                       \
  (l).type == KIT_VARTYPE_NULL || (r).type == KIT_VARTYPE_NULL ? KIT_NULLVAR                                                                         \
      : (is_float(l) || is_float(r)) ? (kit_var){ .type = KIT_VARTYPE_FLOAT, .val = { .f = kit_cast_to_float(&(l)) op kit_cast_to_float(&(r)) } }    \
                                     : (kit_var)                                                                                                     \
  {                                                                                                                                                  \
    .type = KIT_VARTYPE_INT, .val = {.i = kit_cast_to_int(&(l)) op kit_cast_to_int(&(r)) }                                                           \
  }

#define COERCE_BOOLEAN_BINOP(l, r, op)                                                                                                               \
  (l).type == KIT_VARTYPE_NULL || (r).type == KIT_VARTYPE_NULL ? KIT_NULLVAR                                                                         \
      : (is_float(l) || is_float(r)) ? (kit_var){ .type = KIT_VARTYPE_BOOL, .val = { .b = kit_cast_to_float(&(l)) op kit_cast_to_float(&(r)) } }     \
                                     : (kit_var)                                                                                                     \
  {                                                                                                                                                  \
    .type = KIT_VARTYPE_BOOL, .val = {.b = kit_cast_to_int(&(l)) op kit_cast_to_int(&(r)) }                                                          \
  }

static inline bool
is_vector(const kit_var* v)
{ return (v->type == KIT_VARTYPE_VEC2 || v->type == KIT_VARTYPE_VEC3 || v->type == KIT_VARTYPE_VEC4); }

static inline bool
is_scalar(const kit_var* v)
{ return (v->type == KIT_VARTYPE_INT || v->type == KIT_VARTYPE_FLOAT || v->type == KIT_VARTYPE_CHAR || v->type == KIT_VARTYPE_BOOL); }

static inline kit_var
v4_operate(kit_var l, kit_var r, eir_opcode op)
{
  if (l.type != KIT_VARTYPE_VEC4 && r.type != KIT_VARTYPE_VEC4) return (kit_var){ .type = KIT_VARTYPE_NULL };

  if (op == EIR_OPCODE_EQL) { return kit_var_from_bool(kit_var_equal(&l, &r)); }
  if (op == EIR_OPCODE_NEQ) { return kit_var_from_bool(!kit_var_equal(&l, &r)); }

  kit_vec4 lv = { 0 };
  kit_vec4 rv = { 0 };

  bool lvec = l.type == KIT_VARTYPE_VEC4;
  bool rvec = r.type == KIT_VARTYPE_VEC4;

  if (lvec) memcpy(lv, l.val.vec3, sizeof(kit_vec4));
  if (rvec) memcpy(rv, r.val.vec3, sizeof(kit_vec4));

  switch (op) {
    case EIR_OPCODE_ADD:
      if (lvec && rvec) return kit_make_vec4(lv[0] + rv[0], lv[1] + rv[1], lv[2] + rv[2], lv[3] + rv[3]);
      break;

    case EIR_OPCODE_SUB:
      if (lvec && rvec) return kit_make_vec4(lv[0] - rv[0], lv[1] - rv[1], lv[2] - rv[2], lv[3] - rv[3]);
      break;

    case EIR_OPCODE_MUL:
      if (lvec && is_scalar(&r)) {
        double s = kit_cast_to_float(&r);
        return kit_make_vec4(lv[0] * s, lv[1] * s, lv[2] * s, lv[3] * s);
      }
      if (rvec && is_scalar(&l)) {
        double s = kit_cast_to_float(&l);
        return kit_make_vec4(rv[0] * s, rv[1] * s, rv[2] * s, rv[3] * s);
      }
      break;

    case EIR_OPCODE_DIV:
      if (lvec && is_scalar(&r)) {
        double s = kit_cast_to_float(&r);
        return kit_make_vec4(lv[0] / s, lv[1] / s, lv[2] / s, lv[3] / s);
      }
      break;

    case EIR_OPCODE_NEG:
      if (rvec) return kit_make_vec4(-rv[0], -rv[1], -rv[2], -rv[3]);
      break;

    default: break;
  }

  return KIT_NULLVAR;
}

static inline kit_var
v3_operate(kit_var l, kit_var r, eir_opcode op)
{
  if (l.type != KIT_VARTYPE_VEC3 && r.type != KIT_VARTYPE_VEC3) return (kit_var){ .type = KIT_VARTYPE_NULL };

  if (op == EIR_OPCODE_EQL) { return kit_var_from_bool(kit_var_equal(&l, &r)); }
  if (op == EIR_OPCODE_NEQ) { return kit_var_from_bool(!kit_var_equal(&l, &r)); }

  kit_vec3 lv = { 0 };
  kit_vec3 rv = { 0 };

  bool lvec = l.type == KIT_VARTYPE_VEC3;
  bool rvec = r.type == KIT_VARTYPE_VEC3;

  if (lvec) memcpy(lv, l.val.vec3, sizeof(kit_vec3));
  if (rvec) memcpy(rv, r.val.vec3, sizeof(kit_vec3));

  switch (op) {
    case EIR_OPCODE_ADD:
      if (lvec && rvec) return kit_make_vec3(lv[0] + rv[0], lv[1] + rv[1], lv[2] + rv[2]);
      break;

    case EIR_OPCODE_SUB:
      if (lvec && rvec) return kit_make_vec3(lv[0] - rv[0], lv[1] - rv[1], lv[2] - rv[2]);
      break;

    case EIR_OPCODE_MUL:
      if (lvec && is_scalar(&r)) {
        double s = kit_cast_to_float(&r);
        return kit_make_vec3(lv[0] * s, lv[1] * s, lv[2] * s);
      }
      if (rvec && is_scalar(&l)) {
        double s = kit_cast_to_float(&l);
        return kit_make_vec3(rv[0] * s, rv[1] * s, rv[2] * s);
      }
      break;

    case EIR_OPCODE_DIV:
      if (lvec && is_scalar(&r)) {
        double s = kit_cast_to_float(&r);
        return kit_make_vec3(lv[0] / s, lv[1] / s, lv[2] / s);
      }
      break;

    case EIR_OPCODE_NEG:
      if (rvec) return kit_make_vec3(-rv[0], -rv[1], -rv[2]);
      break;

    default: break;
  }

  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

static inline kit_var
v2_operate(kit_var l, kit_var r, eir_opcode op)
{
  if (l.type != KIT_VARTYPE_VEC2 && r.type != KIT_VARTYPE_VEC2) return (kit_var){ .type = KIT_VARTYPE_NULL };

  if (op == EIR_OPCODE_EQL) { return kit_var_from_bool(kit_var_equal(&l, &r)); }
  if (op == EIR_OPCODE_NEQ) { return kit_var_from_bool(!kit_var_equal(&l, &r)); }

  kit_vec2 lv = { 0 };
  kit_vec2 rv = { 0 };

  bool lhs_is_vec = l.type == KIT_VARTYPE_VEC2;
  bool rhs_is_vec = r.type == KIT_VARTYPE_VEC2;

  if (lhs_is_vec) memcpy(lv, l.val.vec2, sizeof(kit_vec2));
  if (rhs_is_vec) memcpy(rv, r.val.vec2, sizeof(kit_vec2));

  switch (op) {
    case EIR_OPCODE_ADD:
      if (lhs_is_vec && rhs_is_vec) return kit_make_vec2(lv[0] + rv[0], lv[1] + rv[1]);
      break;

    case EIR_OPCODE_SUB:
      if (lhs_is_vec && rhs_is_vec) return kit_make_vec2(lv[0] - rv[0], lv[1] - rv[1]);
      break;

    case EIR_OPCODE_MUL:
      if (lhs_is_vec && is_scalar(&r)) {
        double s = kit_cast_to_float(&r);
        return kit_make_vec2(lv[0] * s, lv[1] * s);
      }
      if (rhs_is_vec && is_scalar(&l)) {
        double s = kit_cast_to_float(&l);
        return kit_make_vec2(rv[0] * s, rv[1] * s);
      }
      break;

    case EIR_OPCODE_DIV:
      if (lhs_is_vec && is_scalar(&r)) {
        double s = kit_cast_to_float(&r);
        return kit_make_vec2(lv[0] / s, lv[1] / s);
      }
      break;

    case EIR_OPCODE_NEG:
      if (rhs_is_vec) return kit_make_vec2(-rv[0], -rv[1]);
      break;

    default: break;
  }

  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

static inline kit_var
vector_operate(kit_var l, kit_var r, eir_opcode op)
{
  if (l.type == KIT_VARTYPE_VEC4 || r.type == KIT_VARTYPE_VEC4) { return v4_operate(l, r, op); }
  if (l.type == KIT_VARTYPE_VEC3 || r.type == KIT_VARTYPE_VEC3) { return v3_operate(l, r, op); }
  if (l.type == KIT_VARTYPE_VEC2 || r.type == KIT_VARTYPE_VEC2) { return v2_operate(l, r, op); }
  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

static inline kit_var
operate(kit_var l, kit_var r, eir_opcode op)
{
  if (is_vector(&l) || is_vector(&r)) { return vector_operate(l, r, op); }

  if (op == EIR_OPCODE_NOT) return (kit_var){ .type = KIT_VARTYPE_BOOL, .val = { .b = !kit_var_to_bool(r) } };

  switch (op) {
    case EIR_OPCODE_ADD: return COERCE_BINOP(l, r, +);
    case EIR_OPCODE_SUB: return COERCE_BINOP(l, r, -);
    case EIR_OPCODE_MUL: return COERCE_BINOP(l, r, *);
    case EIR_OPCODE_DIV:
      if (kit_cast_to_int(&r) == 0) return KIT_NULLVAR;
      return COERCE_BINOP(l, r, /);
    case EIR_OPCODE_MOD:
      if (kit_cast_to_int(&r) == 0) return KIT_NULLVAR;
      return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = kit_cast_to_int(&l) % kit_cast_to_int(&r) };
    case EIR_OPCODE_EQL: return (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = kit_var_equal(&l, &r) };
    case EIR_OPCODE_NEQ: return (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = (bool)!kit_var_equal(&l, &r) };
    case EIR_OPCODE_LT: return COERCE_BOOLEAN_BINOP(l, r, <);
    case EIR_OPCODE_LTE: return COERCE_BOOLEAN_BINOP(l, r, <=);
    case EIR_OPCODE_GT: return COERCE_BOOLEAN_BINOP(l, r, >);
    case EIR_OPCODE_GTE: return COERCE_BOOLEAN_BINOP(l, r, >=);
    case EIR_OPCODE_AND: return COERCE_BOOLEAN_BINOP(l, r, &&);
    case EIR_OPCODE_OR: return COERCE_BOOLEAN_BINOP(l, r, ||);
    case EIR_OPCODE_NEG:
      switch (r.type) {
        case KIT_VARTYPE_INT: return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = -r.val.i };
        case KIT_VARTYPE_CHAR: return (kit_var){ .type = KIT_VARTYPE_CHAR, .val.c = (char)-(int)r.val.c };
        case KIT_VARTYPE_BOOL: return (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = (bool)-(int)r.val.b };
        case KIT_VARTYPE_FLOAT: return (kit_var){ .type = KIT_VARTYPE_FLOAT, .val.f = -r.val.f };
        default: break;
      }
    case EIR_OPCODE_BNOT: return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = ~kit_cast_to_int(&r) };
    case EIR_OPCODE_BAND: return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = kit_cast_to_int(&l) & kit_cast_to_int(&r) };
    case EIR_OPCODE_BOR: return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = kit_cast_to_int(&l) | kit_cast_to_int(&r) };
    case EIR_OPCODE_XOR: return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = kit_cast_to_int(&l) ^ kit_cast_to_int(&r) };
    default: break;
  }
  return KIT_NULLVAR;
}

#endif // KIT_OPERATE_H