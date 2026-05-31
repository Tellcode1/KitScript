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

#include "../../inc/bfunc.str.h"

#include "../../inc/cast.h"
#include "../../inc/pool.h"
#include "../../inc/stdafx.h"
#include "../../inc/var.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

e_var
eb_str_cat(e_var* args, u32 nargs)
{
  (void)nargs;

  u32 total_len = 0;
  for (u32 i = 0; i < nargs; i++) { total_len += strlen(E_VAR_AS_STRING(&args[i])->s); }

  char* big_s = e_xalloc(1, total_len + 1);

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

  char* new_s = e_xalloc(1, copy_len + 1);
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

  char* new_s = e_xalloc(1, new_len + 1);
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
  char*  new_s = e_xalloc(1, len + 1);
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
  char*  new_s = e_xalloc(1, len);
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

/* https://creativeandcritical.net/str-replace-c : Released under the public domain */
/* MODIFIED A BIT! */
char*
repl_str(const char* str, const char* from, const char* to)
{
  /* Adjust each of the below values to suit your needs. */

  /* Increment positions cache size initially by this number. */
  size_t cache_sz_inc = 16;
  /* Thereafter, each time capacity needs to be increased,
   * multiply the increment by this factor. */
  const size_t cache_sz_inc_factor = 2; // was 3

  char*       pret;
  char*       ret = NULL;
  const char* pstr2;
  const char* pstr  = str;
  size_t      i     = 0;
  size_t      count = 0;

#if (__STDC_VERSION__ >= 199901L)
  uintptr_t* pos_cache_tmp;
  uintptr_t* pos_cache = NULL;
#else
  ptrdiff_t* pos_cache_tmp;
  ptrdiff_t* pos_cache = NULL;
#endif
  size_t cache_sz = 0;
  size_t cpylen   = 0;
  size_t orglen   = 0;
  size_t retlen   = 0;
  size_t tolen    = 0;
  size_t fromlen  = strlen(from);

  /* Find all matches and cache their positions. */
  while ((pstr2 = strstr(pstr, from)) != NULL) {
    count++;

    /* Increase the cache size when necessary. */
    if (cache_sz < count) {
      cache_sz += cache_sz_inc;
      pos_cache_tmp = realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
      if (pos_cache_tmp == NULL) {
        goto end_repl_str;
      } else pos_cache = pos_cache_tmp;
      cache_sz_inc *= cache_sz_inc_factor;
    }

    pos_cache[count - 1] = pstr2 - str;
    pstr                 = pstr2 + fromlen;
  }

  orglen = pstr - str + strlen(pstr);

  /* Allocate memory for the post-replacement string. */
  if (count > 0) {
    tolen  = strlen(to);
    retlen = orglen + ((tolen - fromlen) * count);
  } else retlen = orglen;
  ret = e_xalloc(1, retlen + 1);
  if (ret == NULL) { goto end_repl_str; }

  if (count == 0) {
    /* If no matches, then just duplicate the string. */
    strcpy(ret, str);
  } else {
    /* Otherwise, duplicate the string whilst performing
     * the replacements using the position cache. */
    pret = ret;
    memcpy(pret, str, pos_cache[0]);
    pret += pos_cache[0];
    for (i = 0; i < count; i++) {
      memcpy(pret, to, tolen);
      pret += tolen;
      pstr   = str + pos_cache[i] + fromlen;
      cpylen = (i == count - 1 ? orglen : pos_cache[i + 1]) - pos_cache[i] - fromlen;
      memcpy(pret, pstr, cpylen);
      pret += cpylen;
    }
    ret[retlen] = '\0';
  }

end_repl_str:
  /* Free the cache and return the post-replacement string,
   * which will be NULL in the event of an error. */
  free(pos_cache);
  return ret;
}

e_var
eb_str_replace(e_var* args, u32 nargs)
{
  const char* str          = E_VAR_AS_STRING(&args[0])->s;
  const char* replace_this = E_VAR_AS_STRING(&args[1])->s;
  const char* replace_with = E_VAR_AS_STRING(&args[2])->s;

  char* s = repl_str(str, replace_this, replace_with);

  return e_make_var_from_string(s);
}

e_var
eb_str_find(e_var* args, u32 nargs)
{
  const char* hay    = E_VAR_AS_STRING(&args[0])->s;
  const char* needle = E_VAR_AS_STRING(&args[1])->s;

  const char* find = strstr(hay, needle);
  if (!find) return e_var_from_int(-1);

  return e_var_from_int((int)(find - hay));
}