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

#include "kit.var.h"

// Make list from elements
kit_var kit_builtins_list_make(kit_var* args, u32 nargs);
// append(list, value)
kit_var kit_builtins_list_append(kit_var* args, u32 nargs);
// pop(list)
kit_var kit_builtins_list_pop(kit_var* args, u32 nargs); // fast
// remove(list, index)
kit_var kit_builtins_list_remove(kit_var* args, u32 nargs); // expensive
// insert(list, index, value)
kit_var kit_builtins_list_insert(kit_var* args, u32 nargs); // Replaces value if it exists!
// find(list, value) => -1 if non existent
kit_var kit_builtins_list_find(kit_var* args, u32 nargs);  // Returns index, -1 if it doesn't exist.
kit_var kit_builtins_list_rfind(kit_var* args, u32 nargs); // Returns index, -1 if it doesn't exist.
// exists(list, val) => false if not in list
kit_var kit_builtins_list_exists(kit_var* args, u32 nargs); // Returns if element is in list
// reserve(list, elements_to_reserve)
kit_var kit_builtins_list_reserve(kit_var* args, u32 nargs); // number of new variables to reserve
// resize(list, new_size)
kit_var kit_builtins_list_resize(kit_var* args, u32 nargs);
// len(list)
kit_var kit_builtins_list_len(kit_var* args, u32 nargs);
/* sort a list in place using tim sort */
kit_var kit_builtins_list_sort(kit_var* args, u32 nargs);

#endif // KIT_LIST_BUILTIN_FUNCTIONS_H