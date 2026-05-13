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

#include "bfunc.h"

#include "pool.h"
#include "stdafx.h"
#include "var.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char*
e_read_full_line(FILE* fp)
{
  if (fp == NULL) return NULL;

  size_t size = 128;
  size_t len  = 0;

  char* line = (char*)calloc(1, size);
  if (line == NULL) return NULL;

  char tmp[128] = { 0 };

  while (fgets(tmp, sizeof(tmp), fp) != NULL) {
    size_t tmp_len = strlen(tmp);

    while (len + tmp_len + 1 > size) {
      size *= 2;

      char* new_line = (char*)realloc(line, size);
      if (new_line == NULL) {
        free(line);
        return NULL;
      }

      line = new_line;
    }

    memcpy(line + len, tmp, tmp_len + 1);
    len += tmp_len;

    if (len > 0 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
      break;
    }
  }

  if (len == 0) {
    free(line);
    return NULL;
  }

  return line;
}

e_var
eb_print(e_var* args, u32 nargs)
{
  for (u32 i = 0; i < nargs; i++) e_var_print(&args[i], stdout);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_readln(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;
  return e_make_var_from_string(e_read_full_line(stdin));
}

e_var
eb_cast_int(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_INT, .val.i = evar_to_int(args[0]) };
}

e_var
eb_cast_char(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_CHAR, .val.c = (char)evar_to_int(args[0]) };
}

e_var
eb_cast_bool(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_BOOL, .val.b = evar_to_bool(args[0]) };
}

e_var
eb_cast_float(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_FLOAT, .val.f = evar_to_float(args[0]) };
}

e_var
eb_cast_string(e_var* args, u32 nargs)
{
  (void)nargs;

  size_t len = e_var_to_string_size(&args[0]);
  char*  s   = e_xalloc(1, len + 1);
  s[len]     = 0;

  e_var_to_string(&args[0], s, len + 1);

  e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);

  E_OBJ_AS_STRING(obj)->s = s;

  return (e_var){ .type = E_VARTYPE_STRING, .val.s = obj };
}

e_var
eb_cast_list(e_var* args, u32 nargs)
{
  e_var new_list = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };

  if (!new_list.val.list || e_list_init(args, nargs, E_OBJ_AS_LIST(new_list.val.list))) // acquires the elements. Stack frees them later.
  {
    e_refdobj_pool_return(&ge_pool, new_list.val.list);
    return (e_var){ .type = E_VARTYPE_NULL };
  }

  return new_list;
}
