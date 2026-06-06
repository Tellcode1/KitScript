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

#ifndef KIT_STR_BUILTIN_FUNCTIONS_H
#define KIT_STR_BUILTIN_FUNCTIONS_H

#include "kit.var.h"

// Form a string from a list of characters
kit_var kit_builtins_str_from_list(kit_var* args, u32 nargs);
static inline kit_var
kit_builtins_str_equal(kit_var* args, u32 nargs)
{
  (void)nargs;
  return (kit_var){ .type = KIT_VARTYPE_BOOL, .val = { .b = strcmp(KIT_VAR_AS_STRING(&args[0])->s, KIT_VAR_AS_STRING(&args[1])->s) == 0 } };
}
kit_var kit_builtins_str_cat(kit_var* args, u32 nargs);
kit_var kit_builtins_str_substr(kit_var* args, u32 nargs); // substring: string, int start, int length
kit_var kit_builtins_str_repeat(kit_var* args, u32 nargs); // repeat: string, int times
kit_var kit_builtins_str_replace(kit_var* args, u32 nargs);
kit_var kit_builtins_str_ltrim(kit_var* args, u32 nargs);
kit_var kit_builtins_str_rtrim(kit_var* args, u32 nargs);
kit_var kit_builtins_str_trim(kit_var* args, u32 nargs);
kit_var kit_builtins_str_split(kit_var* args, u32 nargs); // Get list of strings partition by args[1] (string)
kit_var kit_builtins_str_len(kit_var* args, u32 nargs);
kit_var kit_builtins_str_find(kit_var* args, u32 nargs);
kit_var kit_builtins_str_rfind(kit_var* args, u32 nargs);

#endif // KIT_STR_BUILTIN_FUNCTIONS_H