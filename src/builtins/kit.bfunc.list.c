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
#include "../../inc/kit.var.h"

kit_var
kit_builtins_list_make(kit_var* args, u32 nargs)
{
  kit_var v = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(&kit_g_obj_pool),
  };

  int e = kit_list_init(args, nargs, KIT_VAR_AS_LIST(&v));
  if (e) {
    kit_refdobj_pool_return(&kit_g_obj_pool, v.val.list);
    return KIT_NULLVAR;
  }

  return v;
}

kit_var
kit_builtins_list_append(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l = KIT_VAR_AS_LIST(&args[0]);
  (void)kit_list_append(&args[1], l);
  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

kit_var
kit_builtins_list_pop(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l = KIT_VAR_AS_LIST(&args[0]);
  kit_list_pop(l);
  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

kit_var
kit_builtins_list_remove(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l     = KIT_VAR_AS_LIST(&args[0]);
  int       index = kit_builtins_cast_int(&args[1], 1).val.i;

  if (index >= l->size) { return (kit_var){ .type = KIT_VARTYPE_NULL }; }

  kit_list_remove((u32)index, l);
  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

kit_var
kit_builtins_list_insert(kit_var* args, u32 nargs)
{
  (void)nargs;

  kit_list* l     = KIT_VAR_AS_LIST(&args[0]);
  int       index = kit_builtins_cast_int(&args[1], 1).val.i;
  kit_var   value = args[2];

  if (index < 0 || (u32)index > l->size) { return (kit_var){ .type = KIT_VARTYPE_NULL }; }

  kit_list_insert((u32)index, &value, l);

  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

kit_var
kit_builtins_list_find(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l      = KIT_VAR_AS_LIST(&args[0]);
  kit_var*  search = &args[1];
  for (u32 i = 0; i < l->size; i++) {
    if (kit_var_equal(search, &l->vars[i])) { return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)i }; }
  }
  return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = -1 };
}

kit_var
kit_builtins_list_rfind(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l      = KIT_VAR_AS_LIST(&args[0]);
  kit_var*  search = &args[1];
  for (i64 i = l->size - 1; i >= 0; i++) {
    if (kit_var_equal(search, &l->vars[i])) { return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)i }; }
  }
  return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = -1 };
}

kit_var
kit_builtins_list_exists(kit_var* args, u32 nargs)
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
  return (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = exists };
}

kit_var
kit_builtins_list_len(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l = KIT_VAR_AS_LIST(&args[0]);
  return (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)l->size };
}

kit_var
kit_builtins_list_reserve(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l                 = KIT_VAR_AS_LIST(&args[0]);
  int       nelems_to_reserve = kit_builtins_cast_int(&args[1], 1).val.i;
  (void)kit_list_reserve(l->capacity + nelems_to_reserve, l);
  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

kit_var
kit_builtins_list_resize(kit_var* args, u32 nargs)
{
  (void)nargs;
  kit_list* l        = KIT_VAR_AS_LIST(&args[0]);
  int       new_size = kit_builtins_cast_int(&args[1], 1).val.i;
  (void)kit_list_resize(new_size, l);
  return (kit_var){ .type = KIT_VARTYPE_NULL };
}

kit_var
kit_builtins_list_sort(kit_var* args, u32 nargs)
{
  kit_list* list = KIT_VAR_AS_LIST(&args[0]);
  (void)kit_tim_sort(list->vars, list->size);

  return KIT_NULLVAR;
}