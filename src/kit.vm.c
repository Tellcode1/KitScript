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

#include "../inc/kit.vm.h"

int
kit_vm_init(int argc, const char** argv, kit_refdobj_pool* pool, u32 function_id, kit_vm* vm)
{
  *vm = (kit_vm){
    .instruction_idx = 0,
    .curr_func_hash  = function_id,
    .argc            = argc,
    .argv            = argv,
    .log_file        = stderr,
    .pool            = pool,
  };
  for (u32 i = 0; i < KIT_ARRLEN(vm->hash_state); i++) { vm->hash_state[i] = rand(); }

  return 0;
}

void
kit_vm_free(kit_vm* vm)
{ memset(vm, 0, sizeof(*vm)); }

int
kit_vm_fork(u32 new_function_id, const kit_vm* old_vm, kit_vm* new_vm)
{
  *new_vm = (kit_vm){
    .instruction_idx = 0,
    .curr_func_hash  = new_function_id,
    .argc            = old_vm->argc,
    .argv            = old_vm->argv,
    .log_file        = old_vm->log_file,
    .pool            = old_vm->pool,
  };
  memcpy(new_vm->hash_state, old_vm->hash_state, sizeof(new_vm->hash_state));

  return 0;
}