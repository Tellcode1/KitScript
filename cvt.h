#ifndef E_CONVERTER_H
#define E_CONVERTER_H

#include "stdafx.h"

typedef enum e_cvt_err {
  E_CVT_OK,
  E_CVT_ERROR_MALFORMED_INPUT,
  E_CVT_ERROR_OVERFLOW,
  E_CVT_ERROR_UNDERFLOW,
  E_CVT_ERROR_EOF,
} e_cvt_err;

e_cvt_err e_cvt_int(const char* s, const char** end, int* o) RETURNS_ERRCODE;
e_cvt_err e_cvt_double(const char* s, const char** end, double* o) RETURNS_ERRCODE;

#endif // E_CONVERTER_H