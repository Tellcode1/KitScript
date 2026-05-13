#include "cvt.h"

#include <ctype.h>
#include <stdbool.h>

const char*
lstrip(const char* s)
{
  while (*s && isspace(*s)) s++;
  return s;
}

e_cvt_err
e_cvt_int(const char* s, const char** end, int* o)
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

    if (tolower(*s) == 'x') {
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

    if (is_hex && base != 16) { return E_CVT_ERROR_MALFORMED_INPUT; }
    if (base == 8 && c > '8') { return E_CVT_ERROR_MALFORMED_INPUT; }

    int dig = 0;
    if (is_hex) {
      dig = (c - 'a');
    } else if (is_dec_or_oct) {
      dig = (c - '0');
      if (base == 8 && dig >= 8) { return E_CVT_ERROR_MALFORMED_INPUT; }
    } else {
      return E_CVT_ERROR_MALFORMED_INPUT;
    }

    cvt = (cvt * base) + dig;
    s++;
  }

  if (end) *end = s;

  if (neg) cvt = -cvt;
  if (o) *o = cvt;

  return E_CVT_OK;
}

e_cvt_err
e_cvt_double(const char* s, const char** end, double* o)
{
}
