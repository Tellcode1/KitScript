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

#include "../../inc/kit.cast.h"
#include "../../inc/kit.perr.h"
#include "../../inc/kit.pool.h"
#include "../../inc/kit.var.h"
#include "../../inc/kit.vm.h"

kit_ecode
kit_builtins_list_make(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_var v = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(vm->pool),
  };

  int e = kit_list_init(vm->pool, args, nargs, KIT_VAR_AS_LIST(&v));
  if (e) {
    kit_refdobj_pool_return(vm->pool, v.val.list);
    *result = KIT_NULLVAR;
    return KIT_OK;
  }

  *result = v;
  return KIT_OK;
}

kit_ecode
kit_builtins_list_append(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l = KIT_VAR_AS_LIST(&args[0]);
  (void)kit_list_append(vm->pool, &args[1], l);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_pop(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l = KIT_VAR_AS_LIST(&args[0]);
  kit_list_pop(vm->pool, l);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_remove(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l     = KIT_VAR_AS_LIST(&args[0]);
  int       index = kit_cast_to_int(&args[1]);

  if (index >= l->size) { *result = (kit_var){ .type = KIT_VARTYPE_NULL }; }
  return KIT_OK;

  kit_list_remove(vm->pool, (u32)index, l);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_insert(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  kit_list* l     = KIT_VAR_AS_LIST(&args[0]);
  int       index = kit_cast_to_int(&args[1]);
  kit_var   value = args[2];

  if (index < 0 || (u32)index > l->size) { *result = (kit_var){ .type = KIT_VARTYPE_NULL }; }
  return KIT_OK;

  kit_list_insert(vm->pool, (u32)index, &value, l);

  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_find(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l      = KIT_VAR_AS_LIST(&args[0]);
  kit_var*  search = &args[1];
  for (u32 i = 0; i < l->size; i++) {
    if (kit_var_equal(search, &l->vars[i])) { *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)i }; }
    return KIT_OK;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = -1 };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_rfind(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l      = KIT_VAR_AS_LIST(&args[0]);
  kit_var*  search = &args[1];
  for (i64 i = l->size - 1; i >= 0; i--) {
    if (kit_var_equal(search, &l->vars[i])) { *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)i }; }
    return KIT_OK;
  }
  *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = -1 };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_exists(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_list* l      = KIT_VAR_AS_LIST(&args[0]);
  kit_var*  search = &args[1];

  bool exists = false;
  for (u32 i = 0; i < l->size; i++) {
    if (kit_var_equal(search, &l->vars[i])) {
      exists = true;
      break;
    }
  }
  *result = (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = exists };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_len(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l = KIT_VAR_AS_LIST(&args[0]);
  *result     = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)l->size };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_reserve(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l                 = KIT_VAR_AS_LIST(&args[0]);
  int       nelems_to_reserve = kit_cast_to_int(&args[1]);
  (void)kit_list_reserve(l->capacity + nelems_to_reserve, l);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_resize(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  kit_list* l        = KIT_VAR_AS_LIST(&args[0]);
  int       new_size = kit_cast_to_int(&args[1]);
  (void)kit_list_resize(vm->pool, new_size, l);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_list_sort(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_list* list = KIT_VAR_AS_LIST(&args[0]);
  (void)kit_tim_sort(list->vars, list->size);

  *result = KIT_NULLVAR;
  return KIT_OK;
}