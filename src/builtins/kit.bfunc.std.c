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

#include "../../inc/kit.bfunc.io.h"
#include "../../inc/kit.bvar.h"
#include "../../inc/kit.cast.h"
#include "../../inc/kit.list.h"
#include "../../inc/kit.pool.h"
#include "../../inc/kit.var.h"

kit_ecode
kit_builtins_std_max(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = KIT_NULLVAR;
  return KIT_EUNDEFINED;
}

kit_ecode
kit_builtins_std_min(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = KIT_NULLVAR;
  return KIT_EUNDEFINED;
}