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

#ifndef E_CERR_H
#define E_CERR_H

#include "stdafx.h"

#include <stdarg.h>
#include <stdio.h>

typedef struct e_filespan {
  const char* file;
  int         line;
  int         col;
} e_filespan;

static inline ATTR_FORMAT(printf, 4, 5) int e_internal_cerror(const char* file, size_t line, e_filespan span, const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);

  fprintf(stderr, "[%s:%zu] [%s:%i:%i] compilation error: ", file, line, span.file, span.line, span.col);
  vfprintf(stderr, msg, ap);

  va_end(ap);

  return 0;
}

static inline ATTR_FORMAT(printf, 3, 4) int e_internal_xerror(const char* file, size_t line, const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);

  fprintf(stderr, "[%s:%zu] [error] ", file, line);
  vfprintf(stderr, msg, ap);

  va_end(ap);

  return 0;
}

#define cerror(span, ...)                                                                                                                            \
  do {                                                                                                                                               \
    e_internal_cerror(__FILE__, __LINE__, span, __VA_ARGS__);                                                                                        \
  } while (0)

#define e_xerror(...)                                                                                                                                \
  do {                                                                                                                                               \
    e_internal_xerror(__FILE__, __LINE__, __VA_ARGS__);                                                                                              \
  } while (0)

#endif // E_CERR_H
