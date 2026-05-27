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

#include "../../inc/bfunc.h"

#include "../../inc/pool.h"
#include "../../inc/var.h"

e_var
eb_list_make(e_var* args, u32 nargs)
{
  e_var v = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };

  int e = e_list_init(args, nargs, E_VAR_AS_LIST(&v));
  if (e) {
    e_refdobj_pool_return(&ge_pool, v.val.list);
    return E_NULLVAR;
  }

  return v;
}

e_var
eb_list_append(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l = E_VAR_AS_LIST(&args[0]);
  (void)e_list_append(&args[1], l);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_list_pop(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l = E_VAR_AS_LIST(&args[0]);
  e_list_pop(l);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_list_remove(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l     = E_VAR_AS_LIST(&args[0]);
  int     index = eb_cast_int(&args[1], 1).val.i;

  if (index >= l->size) { return (e_var){ .type = E_VARTYPE_NULL }; }

  e_list_remove((u32)index, l);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_list_insert(e_var* args, u32 nargs)
{
  (void)nargs;

  e_list* l     = E_VAR_AS_LIST(&args[0]);
  int     index = eb_cast_int(&args[1], 1).val.i;
  e_var   value = args[2];

  if (index < 0 || (u32)index > l->size) { return (e_var){ .type = E_VARTYPE_NULL }; }

  e_list_insert((u32)index, &value, l);

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_list_find(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l      = E_VAR_AS_LIST(&args[0]);
  e_var*  search = &args[1];
  for (u32 i = 0; i < l->size; i++) {
    if (e_var_equal(search, &l->vars[i])) { return (e_var){ .type = E_VARTYPE_INT, .val.i = (int)i }; }
  }
  return (e_var){ .type = E_VARTYPE_INT, .val.i = -1 };
}

e_var
eb_list_rfind(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l      = E_VAR_AS_LIST(&args[0]);
  e_var*  search = &args[1];
  for (i64 i = l->size - 1; i >= 0; i++) {
    if (e_var_equal(search, &l->vars[i])) { return (e_var){ .type = E_VARTYPE_INT, .val.i = (int)i }; }
  }
  return (e_var){ .type = E_VARTYPE_INT, .val.i = -1 };
}

e_var
eb_list_exists(e_var* args, u32 nargs)
{
  e_list* l      = E_VAR_AS_LIST(&args[0]);
  e_var*  search = &args[1];

  bool exists = false;
  for (u32 i = 0; i < l->size; i++) {
    if (e_var_equal(search, &l->vars[i])) {
      exists = true;
      break;
    }
  }
  return (e_var){ .type = E_VARTYPE_BOOL, .val.b = exists };
}

e_var
eb_list_len(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l = E_VAR_AS_LIST(&args[0]);
  return (e_var){ .type = E_VARTYPE_INT, .val.i = (int)l->size };
}

e_var
eb_list_reserve(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l                 = E_VAR_AS_LIST(&args[0]);
  int     nelems_to_reserve = eb_cast_int(&args[1], 1).val.i;
  (void)e_list_reserve(l->capacity + nelems_to_reserve, l);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_list_resize(e_var* args, u32 nargs)
{
  (void)nargs;
  e_list* l        = E_VAR_AS_LIST(&args[0]);
  int     new_size = eb_cast_int(&args[1], 1).val.i;
  (void)e_list_resize(new_size, l);
  return (e_var){ .type = E_VARTYPE_NULL };
}