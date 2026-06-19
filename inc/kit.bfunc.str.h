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

#include "kit.perr.h"
#include "kit.var.h"
#include "kit.vm.h"

// Form a string from a list of characters
kit_ecode kit_builtins_str_from_list(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
static inline kit_ecode
kit_builtins_str_equal(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  *result = (kit_var){ .type = KIT_VARTYPE_BOOL, .val = { .b = strcmp(KIT_VAR_AS_STRING(&args[0])->s, KIT_VAR_AS_STRING(&args[1])->s) == 0 } };
  return KIT_OK;
}
kit_ecode kit_builtins_str_cat(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_str_substr(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // substring: string, int start, int length
kit_ecode kit_builtins_str_repeat(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // repeat: string, int times
kit_ecode kit_builtins_str_replace(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_str_ltrim(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_str_rtrim(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_str_trim(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_str_split(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); // Get list of strings partition by args[1] (string)
kit_ecode kit_builtins_str_len(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_str_find(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);
kit_ecode kit_builtins_str_rfind(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result);

#endif // KIT_STR_BUILTIN_FUNCTIONS_H