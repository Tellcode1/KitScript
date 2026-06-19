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

#include "../../inc/kit.bfunc.h"

#include "../../inc/kit.pool.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.var.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char*
kit_read_full_line(FILE* fp)
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

kit_ecode
kit_builtins_print(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  for (u32 i = 0; i < nargs; i++) kit_var_print(&args[i], stdout);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_readln(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;
  *result = kit_make_var_from_string(vm->pool, kit_read_full_line(stdin));
  return KIT_OK;
}

kit_ecode
kit_builtins_cast_int(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = kit_var_to_int(args[0]) };
  return KIT_OK;
}

kit_ecode
kit_builtins_cast_char(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_CHAR, .val.c = (char)kit_var_to_int(args[0]) };
  return KIT_OK;
}

kit_ecode
kit_builtins_cast_bool(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = kit_var_to_bool(args[0]) };
  return KIT_OK;
}

kit_ecode
kit_builtins_cast_float(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_FLOAT, .val.f = kit_var_to_float(args[0]) };
  return KIT_OK;
}

kit_ecode
kit_builtins_cast_string(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  size_t len = kit_var_to_string_size(&args[0]);
  char*  s   = kit_xalloc(1, len + 1);
  s[len]     = 0;

  kit_var_to_string(&args[0], s, len + 1);

  kit_refdobj* obj = kit_refdobj_pool_acquire(vm->pool);

  KIT_OBJ_AS_STRING(obj)->s = s;

  *result = (kit_var){ .type = KIT_VARTYPE_STRING, .val.s = obj };
  return KIT_OK;
}

kit_ecode
kit_builtins_cast_list(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_var new_list = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(vm->pool),
  };

  if (!new_list.val.list
      || kit_list_init(vm->pool, args, nargs, KIT_OBJ_AS_LIST(new_list.val.list))) // acquires the elements. Stack frees them later.
  {
    kit_refdobj_pool_return(vm->pool, new_list.val.list);
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_OK;
  }

  *result = new_list;
  return KIT_OK;
}

kit_ecode
kit_builtins_struct_name(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = kit_make_var_from_string(vm->pool, kit_strdup(KIT_VAR_AS_STRUCT(&args[0])->name));
  return KIT_OK;
}

kit_ecode
kit_builtins_struct_member_count(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = kit_var_from_int(KIT_VAR_AS_STRUCT(&args[0])->member_count);
  return KIT_OK;
}