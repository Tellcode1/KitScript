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

#include "bfunc.str.h"

#include "cast.h"
#include "pool.h"
#include "stdafx.h"
#include "var.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

e_var
eb_str_cat(e_var* args, u32 nargs)
{
  (void)nargs;

  u32 total_len = 0;
  for (u32 i = 0; i < nargs; i++) { total_len += strlen(E_VAR_AS_STRING(&args[i])->s); }

  char* big_s = calloc(1, total_len + 1);

  char* p = big_s;
  p[0]    = 0;
  for (u32 i = 0; i < nargs; i++) { p = e_strlpcat(p, E_VAR_AS_STRING(&args[i])->s, big_s, total_len + 1); }

  return e_make_var_from_string(big_s);
}

e_var
eb_str_substr(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s     = E_VAR_AS_STRING(&args[0])->s;
  int         start = e_cast_to_int(&args[1]);
  int         len   = e_cast_to_int(&args[2]);

  size_t s_len = strlen(s);
  if (start > s_len) { return (e_var){ .type = E_VARTYPE_INT, .val.i = -1 }; }

  size_t copy_len = MIN(len, s_len - start);

  char* new_s = calloc(1, copy_len + 1);
  strncpy(new_s, s + start, copy_len);
  new_s[copy_len] = 0;

  return e_make_var_from_string(new_s);
}

e_var
eb_str_repeat(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s     = E_VAR_AS_STRING(&args[0])->s;
  int         times = e_cast_to_int(&args[1]);

  int new_len = (int)strlen(s) * times;

  char* new_s = calloc(1, new_len + 1);
  new_s[0]    = 0;

  char* p = new_s;
  for (int i = 0; i < times; i++) { p = e_strlpcat(p, s, new_s, new_len + 1); }

  return e_make_var_from_string(new_s);
}

e_var
eb_str_ltrim(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s = E_VAR_AS_STRING(&args[0])->s;

  while (*s && isspace(*s)) { s++; }
  char* new_s = e_strdup(s);

  return e_make_var_from_string(new_s);
}

e_var
eb_str_rtrim(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s = E_VAR_AS_STRING(&args[0])->s;

  const char* end = s + strlen(s) - 1;
  while (end >= s && isspace(*end)) { end--; }

  size_t len   = (end - s) + 1; // include end character
  char*  new_s = calloc(1, len + 1);
  strncpy(new_s, s, len);
  new_s[len] = 0;

  return e_make_var_from_string(new_s);
}

e_var
eb_str_trim(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* s   = E_VAR_AS_STRING(&args[0])->s;
  const char* end = s + strlen(s) - 1;

  while (isspace(*s) && *s) s++;
  while (end >= s && isspace(*end)) end--;

  size_t len   = end - s + 1; // include end
  char*  new_s = calloc(1, len);
  strncpy(new_s, s, len);
  new_s[len] = 0;

  return e_make_var_from_string(new_s);
}

e_var
eb_str_len(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_INT, .val.i = (int)strlen(E_VAR_AS_STRING(&args[0])->s) };
}

e_var
eb_str_split(e_var* args, u32 nargs)
{
  (void)nargs;

  const char* to_split = E_VAR_AS_STRING(&args[0])->s;
  const char* split_by = E_VAR_AS_STRING(&args[1])->s;

  e_var returned_list = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };

  e_list_init(NULL, 0, E_VAR_AS_LIST(&returned_list));

  char* to_split_copy = e_strdup(to_split);
  char* tok           = strtok(to_split_copy, split_by);

  while (tok) {
    e_var s = e_make_var_from_string(e_strdup(tok));
    e_list_append(&s, E_VAR_AS_LIST(&returned_list));

    // Hand over ownership to list.
    e_var_release(&s);

    tok = strtok(NULL, split_by);
  }

  free(to_split_copy);
  return returned_list;
}