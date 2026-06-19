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

#include "../../inc/kit.bfunc.str.h"

#include "../../inc/kit.bfunc.h"
#include "../../inc/kit.cast.h"
#include "../../inc/kit.pool.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.var.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

kit_ecode
kit_builtins_str_cat(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  u32 total_len = 0;
  for (u32 i = 0; i < nargs; i++) { total_len += kit_var_to_string_size(&args[i]); }

  char* big_s = kit_xalloc(1, total_len + 1);

  char* p = big_s;
  p[0]    = 0;
  for (u32 i = 0; i < nargs; i++) {
    kit_var s = KIT_NULLVAR;

    kit_ecode e = kit_builtins_cast_string(vm, &args[i], 1, &s);
    if (e < 0) return e;

    p = kit_strlpcat(p, KIT_VAR_AS_STRING(&s)->s, big_s, total_len + 1);

    kit_var_release(vm->pool, &s);
  }

  *result = kit_make_var_from_string(vm->pool, big_s);
  return KIT_OK;
}

kit_ecode
kit_builtins_str_substr(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  const char* s     = KIT_VAR_AS_STRING(&args[0])->s;
  int         start = kit_cast_to_int(&args[1]);
  int         len   = kit_cast_to_int(&args[2]);

  size_t s_len = strlen(s);
  if (start > s_len) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = -1 };
    return KIT_OK;
  }

  size_t copy_len = MIN(len, s_len - start);

  char* new_s = kit_xalloc(1, copy_len + 1);
  strncpy(new_s, s + start, copy_len);
  new_s[copy_len] = 0;

  *result = kit_make_var_from_string(vm->pool, new_s);
  return KIT_OK;
}

kit_ecode
kit_builtins_str_repeat(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  const char* s     = KIT_VAR_AS_STRING(&args[0])->s;
  int         times = kit_cast_to_int(&args[1]);

  int new_len = (int)strlen(s) * times;

  char* new_s = kit_xalloc(1, new_len + 1);
  new_s[0]    = 0;

  char* p = new_s;
  for (int i = 0; i < times; i++) { p = kit_strlpcat(p, s, new_s, new_len + 1); }

  *result = kit_make_var_from_string(vm->pool, new_s);
  return KIT_OK;
}

kit_ecode
kit_builtins_str_ltrim(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  const char* s = KIT_VAR_AS_STRING(&args[0])->s;

  while (*s && isspace(*s)) { s++; }
  char* new_s = kit_strdup(s);

  *result = kit_make_var_from_string(vm->pool, new_s);
  return KIT_OK;
}

kit_ecode
kit_builtins_str_rtrim(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  const char* s = KIT_VAR_AS_STRING(&args[0])->s;

  const char* end = s + strlen(s) - 1;
  while (end >= s && isspace(*end)) { end--; }

  size_t len   = (end - s) + 1; // include end character
  char*  new_s = kit_xalloc(1, len + 1);
  strncpy(new_s, s, len);
  new_s[len] = 0;

  *result = kit_make_var_from_string(vm->pool, new_s);
  return KIT_OK;
}

kit_ecode
kit_builtins_str_trim(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  const char* s   = KIT_VAR_AS_STRING(&args[0])->s;
  const char* end = s + strlen(s) - 1;

  while (isspace(*s) && *s) s++;
  while (end >= s && isspace(*end)) end--;

  size_t len   = end - s + 1; // include end
  char*  new_s = kit_xalloc(1, len);
  strncpy(new_s, s, len);
  new_s[len] = 0;

  *result = kit_make_var_from_string(vm->pool, new_s);
  return KIT_OK;
}

kit_ecode
kit_builtins_str_len(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)strlen(KIT_VAR_AS_STRING(&args[0])->s) };
  return KIT_OK;
}

kit_ecode
kit_builtins_str_split(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  const char* to_split = KIT_VAR_AS_STRING(&args[0])->s;
  const char* split_by = KIT_VAR_AS_STRING(&args[1])->s;

  kit_var returned_list = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(vm->pool),
  };

  kit_list_init(vm->pool, NULL, 0, KIT_VAR_AS_LIST(&returned_list));

  char* to_split_copy = kit_strdup(to_split);
  char* tok           = strtok(to_split_copy, split_by);

  while (tok) {
    kit_var s = kit_make_var_from_string(vm->pool, kit_strdup(tok));
    kit_list_append(vm->pool, &s, KIT_VAR_AS_LIST(&returned_list));

    // Hand over ownership to list.
    kit_var_release(vm->pool, &s);

    tok = strtok(NULL, split_by);
  }

  free(to_split_copy);
  *result = returned_list;
  return KIT_OK;
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
  ret = kit_xalloc(1, retlen + 1);
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

kit_ecode
kit_builtins_str_replace(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* str          = KIT_VAR_AS_STRING(&args[0])->s;
  const char* replace_this = KIT_VAR_AS_STRING(&args[1])->s;
  const char* replace_with = KIT_VAR_AS_STRING(&args[2])->s;

  char* s = repl_str(str, replace_this, replace_with);

  *result = kit_make_var_from_string(vm->pool, s);
  return KIT_OK;
}

kit_ecode
kit_builtins_str_find(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* hay    = KIT_VAR_AS_STRING(&args[0])->s;
  const char* needle = KIT_VAR_AS_STRING(&args[1])->s;

  const char* find = strstr(hay, needle);
  if (!find) {
    *result = kit_var_from_int(-1);
    return KIT_OK; /* substring does not exist in string, this is successful */
  }

  *result = kit_var_from_int((int)(find - hay));
  return KIT_OK;
}