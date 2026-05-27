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

#ifndef E_LIST_BUILTIN_FUNCTIONS_H
#define E_LIST_BUILTIN_FUNCTIONS_H

#include "var.h"

// Make list from elements
e_var eb_list_make(e_var* args, u32 nargs);
// append(list, value)
e_var eb_list_append(e_var* args, u32 nargs);
// pop(list)
e_var eb_list_pop(e_var* args, u32 nargs); // fast
// remove(list, index)
e_var eb_list_remove(e_var* args, u32 nargs); // expensive
// insert(list, index, value)
e_var eb_list_insert(e_var* args, u32 nargs); // Replaces value if it exists!
// find(list, value) => -1 if non existent
e_var eb_list_find(e_var* args, u32 nargs);  // Returns index, -1 if it doesn't exist.
e_var eb_list_rfind(e_var* args, u32 nargs); // Returns index, -1 if it doesn't exist.
// exists(list, val) => false if not in list
e_var eb_list_exists(e_var* args, u32 nargs); // Returns if element is in list
// reserve(list, elements_to_reserve)
e_var eb_list_reserve(e_var* args, u32 nargs); // number of new variables to reserve
// resize(list, new_size)
e_var eb_list_resize(e_var* args, u32 nargs);
// len(list)
e_var eb_list_len(e_var* args, u32 nargs);

#endif // E_LIST_BUILTIN_FUNCTIONS_H