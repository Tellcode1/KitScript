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

#ifndef E_STR_BUILTIN_FUNCTIONS_H
#define E_STR_BUILTIN_FUNCTIONS_H

#include "var.h"

// Form a string from a list of characters
e_var eb_str_from_list(e_var* args, u32 nargs);
static inline e_var
eb_str_equal(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_BOOL, .val = { .b = strcmp(E_VAR_AS_STRING(&args[0])->s, E_VAR_AS_STRING(&args[1])->s) == 0 } };
}
e_var eb_str_cat(e_var* args, u32 nargs);
e_var eb_str_substr(e_var* args, u32 nargs); // substring: string, int start, int length
e_var eb_str_repeat(e_var* args, u32 nargs); // repeat: string, int times
e_var eb_str_replace(e_var* args, u32 nargs);
e_var eb_str_ltrim(e_var* args, u32 nargs);
e_var eb_str_rtrim(e_var* args, u32 nargs);
e_var eb_str_trim(e_var* args, u32 nargs);
e_var eb_str_split(e_var* args, u32 nargs); // Get list of strings partition by args[1] (string)
e_var eb_str_len(e_var* args, u32 nargs);
e_var eb_str_find(e_var* args, u32 nargs);
e_var eb_str_rfind(e_var* args, u32 nargs);

#endif // E_STR_BUILTIN_FUNCTIONS_H