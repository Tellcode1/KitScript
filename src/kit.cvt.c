#include "../inc/kit.cvt.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>

static inline bool
will_mul_overflow(int a, int b)
{
  if (a > 0) {
    if (b > 0) return a > INT_MAX / b;
    return b < INT_MIN / a;
  }
  if (a < 0) {
    if (a == -1) return b == INT_MIN;
    if (b > 0) return a < INT_MIN / b;
    return b < INT_MAX / a;
  }
  return false;
}

static inline bool
will_add_overflow(int a, int b)
{
  if ((b > 0) && (a > INT_MAX - b)) return true;
  return false;
}

static inline bool
will_add_underflow(int a, int b)
{
  if ((b < 0) && (a < INT_MIN - b)) return true;
  return false;
}

static inline const char*
lstrip(const char* s)
{
  while (*s && isspace(*s)) s++;
  return s;
}

kit_cvt_err
kit_cvt_int(const char* s, const char** end, int* o)
{
  bool neg = false;

  s = lstrip(s);
  if (*s == '-') {
    neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }

  int base = 10;

  s = lstrip(s);
  if (*s == '0') {
    // num = 0600
    base = 8;
    s++;

    if (tolower(*s) == 'x') { // 0X is valid too
      // num = 0x600
      base = 16;
      s++;
    }
  }

  int cvt = 0;
  while (isdigit(*s) || (base == 16 && (tolower(*s) >= 'a') && (tolower(*s) <= 'f'))) {
    int  c             = (unsigned char)tolower(*s);
    bool is_hex        = (tolower(*s) >= 'a') && (tolower(*s) <= 'f');
    bool is_dec_or_oct = *s >= '0' && *s <= '9';

    if (is_hex && base != 16) { return KIT_CVT_ERROR_MALFORMED_INPUT; }
    if (base == 8 && c > '8') { return KIT_CVT_ERROR_MALFORMED_INPUT; }

    int dig = 0;
    if (is_hex) {
      dig = (c - 'a') + 10;
    } else if (is_dec_or_oct) {
      dig = (c - '0');
      if (base == 8 && dig >= 8) { return KIT_CVT_ERROR_MALFORMED_INPUT; }
    } else {
      return KIT_CVT_ERROR_MALFORMED_INPUT;
    }

    if (will_mul_overflow(cvt, base)) return KIT_CVT_ERROR_OVERFLOW;
    if (will_add_overflow(cvt * base, dig)) return KIT_CVT_ERROR_OVERFLOW;
    if (will_add_underflow(cvt * base, dig)) return KIT_CVT_ERROR_UNDERFLOW;

    cvt = (cvt * base) + dig;
    s++;
  }

  if (end) *end = s;

  if (neg) cvt = -cvt;
  if (o) *o = cvt;

  return KIT_CVT_OK;
}

kit_cvt_err
kit_cvt_double(const char* s, const char** end, double* o)
{
  errno = 0;

  char*  endptr = NULL;
  double value  = strtod(s, &endptr);

  if (errno == ERANGE) {
    if (value == HUGE_VAL || value == -HUGE_VAL) { return KIT_CVT_ERROR_OVERFLOW; }

    if (value == 0.0) { return KIT_CVT_ERROR_UNDERFLOW; }
  }

  if (end) *end = endptr;
  if (o) *o = value;

  return KIT_CVT_OK;
}
