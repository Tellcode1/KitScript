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

#ifndef KIT_CONVERTER_H
#define KIT_CONVERTER_H

#include "kit.stdafx.h"

typedef enum kit_cvt_err {
  KIT_CVT_OK,
  KIT_CVT_ERROR_MALFORMED_INPUT,
  KIT_CVT_ERROR_OVERFLOW,
  KIT_CVT_ERROR_UNDERFLOW,
  KIT_CVT_ERROR_EOF,
} kit_cvt_err;

kit_cvt_err kit_cvt_int(const char* s, const char** end, int* o) RETURNS_ERRCODE;
kit_cvt_err kit_cvt_double(const char* s, const char** end, double* o) RETURNS_ERRCODE;

#endif // KIT_CONVERTER_H