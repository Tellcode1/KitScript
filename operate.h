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

#ifndef E_OPERATE_H
#define E_OPERATE_H

#include "bc.h"
#include "cast.h"
#include "var.h"

#define BINOP(l, r, op)                                                                                                                              \
  (l).type == E_VARTYPE_NULL || (r).type == E_VARTYPE_NULL ? E_NULLVAR                                                                               \
      : (l.type == E_VARTYPE_FLOAT || r.type == E_VARTYPE_FLOAT)                                                                                     \
      ? (e_var){ .type = E_VARTYPE_FLOAT, .val.f = (double)(l).val.f op(double)(r).val.f }                                                           \
      : (e_var)                                                                                                                                      \
  { .type = E_VARTYPE_INT, .val.i = (l).val.i op(r).val.i }
#define BOOLEAN_BINOP(l, r, op)                                                                                                                      \
  (l).type == E_VARTYPE_NULL || (r).type == E_VARTYPE_NULL ? E_NULLVAR                                                                               \
      : ((l).type == E_VARTYPE_FLOAT || (r).type == E_VARTYPE_FLOAT)                                                                                 \
      ? (e_var){ .type = E_VARTYPE_BOOL, .val.b = (double)(l).val.f op(double)(r).val.f }                                                            \
      : (e_var)                                                                                                                                      \
  { .type = E_VARTYPE_BOOL, .val.b = (l).val.i op(r).val.i }

static inline bool
is_float(e_var v)
{ return v.type == E_VARTYPE_FLOAT; }

#define COERCE_BINOP(l, r, op)                                                                                                                       \
  (l).type == E_VARTYPE_NULL || (r).type == E_VARTYPE_NULL ? E_NULLVAR                                                                               \
      : (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_FLOAT, .val.f = e_cast_to_float(&(l)) op e_cast_to_float(&(r)) }                   \
                                     : (e_var)                                                                                                       \
  { .type = E_VARTYPE_INT, .val.i = e_cast_to_int(&(l)) op e_cast_to_int(&(r)) }

#define COERCE_BOOLEAN_BINOP(l, r, op)                                                                                                               \
  (l).type == E_VARTYPE_NULL || (r).type == E_VARTYPE_NULL ? E_NULLVAR                                                                               \
      : (is_float(l) || is_float(r)) ? (e_var){ .type = E_VARTYPE_BOOL, .val.b = e_cast_to_float(&(l)) op e_cast_to_float(&(r)) }                    \
                                     : (e_var)                                                                                                       \
  { .type = E_VARTYPE_BOOL, .val.b = e_cast_to_int(&(l)) op e_cast_to_int(&(r)) }

static inline bool
is_vector(const e_var* v)
{ return (v->type == E_VARTYPE_VEC2 || v->type == E_VARTYPE_VEC3 || v->type == E_VARTYPE_VEC4); }

static inline bool
is_scalar(const e_var* v)
{ return (v->type == E_VARTYPE_INT || v->type == E_VARTYPE_FLOAT || v->type == E_VARTYPE_CHAR || v->type == E_VARTYPE_BOOL); }

static inline e_var
v4_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type != E_VARTYPE_VEC4 && r.type != E_VARTYPE_VEC4) return (e_var){ .type = E_VARTYPE_NULL };

  e_vec4 lv = { 0 };
  e_vec4 rv = { 0 };

  bool lvec = l.type == E_VARTYPE_VEC4;
  bool rvec = r.type == E_VARTYPE_VEC4;

  if (lvec) memcpy(lv, l.val.vec3, sizeof(e_vec4));
  if (rvec) memcpy(rv, r.val.vec3, sizeof(e_vec4));

  switch (op) {
    case E_OPCODE_ADD:
      if (lvec && rvec) return e_make_vec4(lv[0] + rv[0], lv[1] + rv[1], lv[2] + rv[2], lv[3] + rv[3]);
      break;

    case E_OPCODE_SUB:
      if (lvec && rvec) return e_make_vec4(lv[0] - rv[0], lv[1] - rv[1], lv[2] - rv[2], lv[3] - rv[3]);
      break;

    case E_OPCODE_MUL:
      if (lvec && is_scalar(&r)) {
        double s = e_cast_to_float(&r);
        return e_make_vec4(lv[0] * s, lv[1] * s, lv[2] * s, lv[3] * s);
      }
      if (rvec && is_scalar(&l)) {
        double s = e_cast_to_float(&l);
        return e_make_vec4(rv[0] * s, rv[1] * s, rv[2] * s, rv[3] * s);
      }
      break;

    case E_OPCODE_DIV:
      if (lvec && is_scalar(&r)) {
        double s = e_cast_to_float(&r);
        return e_make_vec4(lv[0] / s, lv[1] / s, lv[2] / s, lv[3] / s);
      }
      break;

    case E_OPCODE_NEG:
      if (rvec) return e_make_vec4(-rv[0], -rv[1], -rv[2], -rv[3]);
      break;

    default: break;
  }

  return E_NULLVAR;
}

static inline e_var
v3_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type != E_VARTYPE_VEC3 && r.type != E_VARTYPE_VEC3) return (e_var){ .type = E_VARTYPE_NULL };

  e_vec3 lv = { 0 };
  e_vec3 rv = { 0 };

  bool lvec = l.type == E_VARTYPE_VEC3;
  bool rvec = r.type == E_VARTYPE_VEC3;

  if (lvec) memcpy(lv, l.val.vec3, sizeof(e_vec3));
  if (rvec) memcpy(rv, r.val.vec3, sizeof(e_vec3));

  switch (op) {
    case E_OPCODE_ADD:
      if (lvec && rvec) return e_make_vec3(lv[0] + rv[0], lv[1] + rv[1], lv[2] + rv[2]);
      break;

    case E_OPCODE_SUB:
      if (lvec && rvec) return e_make_vec3(lv[0] - rv[0], lv[1] - rv[1], lv[2] - rv[2]);
      break;

    case E_OPCODE_MUL:
      if (lvec && is_scalar(&r)) {
        double s = e_cast_to_float(&r);
        return e_make_vec3(lv[0] * s, lv[1] * s, lv[2] * s);
      }
      if (rvec && is_scalar(&l)) {
        double s = e_cast_to_float(&l);
        return e_make_vec3(rv[0] * s, rv[1] * s, rv[2] * s);
      }
      break;

    case E_OPCODE_DIV:
      if (lvec && is_scalar(&r)) {
        double s = e_cast_to_float(&r);
        return e_make_vec3(lv[0] / s, lv[1] / s, lv[2] / s);
      }
      break;

    case E_OPCODE_NEG:
      if (rvec) return e_make_vec3(-rv[0], -rv[1], -rv[2]);
      break;

    default: break;
  }

  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
v2_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type != E_VARTYPE_VEC2 && r.type != E_VARTYPE_VEC2) return (e_var){ .type = E_VARTYPE_NULL };

  e_vec2 lv = { 0 };
  e_vec2 rv = { 0 };

  bool lhs_is_vec = l.type == E_VARTYPE_VEC2;
  bool rhs_is_vec = r.type == E_VARTYPE_VEC2;

  if (lhs_is_vec) memcpy(lv, l.val.vec2, sizeof(e_vec2));
  if (rhs_is_vec) memcpy(rv, r.val.vec2, sizeof(e_vec2));

  switch (op) {
    case E_OPCODE_ADD:
      if (lhs_is_vec && rhs_is_vec) return e_make_vec2(lv[0] + rv[0], lv[1] + rv[1]);
      break;

    case E_OPCODE_SUB:
      if (lhs_is_vec && rhs_is_vec) return e_make_vec2(lv[0] - rv[0], lv[1] - rv[1]);
      break;

    case E_OPCODE_MUL:
      if (lhs_is_vec && is_scalar(&r)) {
        double s = e_cast_to_float(&r);
        return e_make_vec2(lv[0] * s, lv[1] * s);
      }
      if (rhs_is_vec && is_scalar(&l)) {
        double s = e_cast_to_float(&l);
        return e_make_vec2(rv[0] * s, rv[1] * s);
      }
      break;

    case E_OPCODE_DIV:
      if (lhs_is_vec && is_scalar(&r)) {
        double s = e_cast_to_float(&r);
        return e_make_vec2(lv[0] / s, lv[1] / s);
      }
      break;

    case E_OPCODE_NEG:
      if (rhs_is_vec) return e_make_vec2(-rv[0], -rv[1]);
      break;

    default: break;
  }

  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
vector_operate(e_var l, e_var r, e_opcode op)
{
  if (l.type == E_VARTYPE_VEC4 || r.type == E_VARTYPE_VEC4) { return v4_operate(l, r, op); }
  if (l.type == E_VARTYPE_VEC3 || r.type == E_VARTYPE_VEC3) { return v3_operate(l, r, op); }
  if (l.type == E_VARTYPE_VEC2 || r.type == E_VARTYPE_VEC2) { return v2_operate(l, r, op); }
  return (e_var){ .type = E_VARTYPE_NULL };
}

static inline e_var
operate(e_var l, e_var r, e_opcode op)
{
  if (is_vector(&l) || is_vector(&r)) { return vector_operate(l, r, op); }

  if (op == E_OPCODE_NOT) return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)!e_var_to_bool(r) };

  switch (op) {
    case E_OPCODE_ADD: return COERCE_BINOP(l, r, +);
    case E_OPCODE_SUB: return COERCE_BINOP(l, r, -);
    case E_OPCODE_MUL: return COERCE_BINOP(l, r, *);
    case E_OPCODE_DIV: return COERCE_BINOP(l, r, /);
    case E_OPCODE_MOD: return (e_var){ .type = E_VARTYPE_INT, .val.i = e_cast_to_int(&l) % e_cast_to_int(&r) };
    case E_OPCODE_EQL: return (e_var){ .type = E_VARTYPE_BOOL, .val.b = e_var_equal(&l, &r) };
    case E_OPCODE_NEQ: return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)!e_var_equal(&l, &r) };
    case E_OPCODE_LT: return COERCE_BOOLEAN_BINOP(l, r, <);
    case E_OPCODE_LTE: return COERCE_BOOLEAN_BINOP(l, r, <=);
    case E_OPCODE_GT: return COERCE_BOOLEAN_BINOP(l, r, >);
    case E_OPCODE_GTE: return COERCE_BOOLEAN_BINOP(l, r, >=);
    case E_OPCODE_AND: return COERCE_BOOLEAN_BINOP(l, r, &&);
    case E_OPCODE_OR: return COERCE_BOOLEAN_BINOP(l, r, ||);
    case E_OPCODE_NEG:
      switch (r.type) {
        case E_VARTYPE_INT: return (e_var){ .type = E_VARTYPE_INT, .val.i = -r.val.i };
        case E_VARTYPE_CHAR: return (e_var){ .type = E_VARTYPE_CHAR, .val.c = (char)-(int)r.val.c };
        case E_VARTYPE_BOOL: return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)-(int)r.val.b };
        case E_VARTYPE_FLOAT: return (e_var){ .type = E_VARTYPE_FLOAT, .val.f = -r.val.f };
        default: break;
      }
    case E_OPCODE_BNOT: return (e_var){ .type = E_VARTYPE_INT, .val.i = ~e_cast_to_int(&r) };
    case E_OPCODE_BAND: return (e_var){ .type = E_VARTYPE_INT, .val.i = e_cast_to_int(&l) & e_cast_to_int(&r) };
    case E_OPCODE_BOR: return (e_var){ .type = E_VARTYPE_INT, .val.i = e_cast_to_int(&l) | e_cast_to_int(&r) };
    case E_OPCODE_XOR: return (e_var){ .type = E_VARTYPE_INT, .val.i = e_cast_to_int(&l) ^ e_cast_to_int(&r) };
    default: break;
  }
  return E_NULLVAR;
}

#endif // E_OPERATE_H