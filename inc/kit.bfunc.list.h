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

#ifndef KIT_LIST_BUILTIN_FUNCTIONS_H
#define KIT_LIST_BUILTIN_FUNCTIONS_H

#include "kit.perr.h"
#include "kit.var.h"
#include "kit.vm.h"

// Make list from elements
kit_ecode kit_builtins_list_make(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
// append(list, value)
kit_ecode kit_builtins_list_append(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
// pop(list)
kit_ecode kit_builtins_list_pop(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // fast
// remove(list, index)
kit_ecode kit_builtins_list_remove(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // expensive
// insert(list, index, value)
kit_ecode kit_builtins_list_insert(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // Replaces value if it exists!
// find(list, value) => -1 if non existent
kit_ecode kit_builtins_list_find(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);  // Returns index, -1 if it doesn't exist.
kit_ecode kit_builtins_list_rfind(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // Returns index, -1 if it doesn't exist.
// exists(list, val) => false if not in list
kit_ecode kit_builtins_list_exists(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // Returns if element is in list
// reserve(list, elements_to_reserve)
kit_ecode kit_builtins_list_reserve(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // number of new variables to reserve
// resize(list, new_size)
kit_ecode kit_builtins_list_resize(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
// len(list)
kit_ecode kit_builtins_list_len(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
/* sort a list in place using tim sort */
kit_ecode kit_builtins_list_sort(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

#endif // KIT_LIST_BUILTIN_FUNCTIONS_H