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
// #define _POSIX_C_SOURCE 199309L
#define _POSIX_C_SOURCE 200112L

#include "../../inc/kit.cast.h"
#include "../../inc/kit.list.h"
#include "../../inc/kit.perr.h"
#include "../../inc/kit.pool.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.var.h"
#include "../../inc/kit.vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

kit_ecode
kit_builtins_sys_get_cmd_args(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;

  kit_var l = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(vm->pool),
  };
  kit_list_init(vm->pool, NULL, 0, KIT_VAR_AS_LIST(&l));

  /* Start from 2. We don't need to expose the executor and script file */
  for (u32 i = 2; i < vm->argc; i++) {
    kit_var arg = kit_make_var_from_string(vm->pool, kit_strdup(vm->argv[i]));
    kit_list_append(vm->pool, &arg, KIT_VAR_AS_LIST(&l));

    kit_var_release(vm->pool, &arg);
  }

  *result = l;
  return KIT_OK;
}

#ifdef _WIN32
#  include <direct.h>
#  define getcwd _getcwd
#elif defined(__unix__) || defined(__posix__)
#  include <unistd.h>
#else
#  define NO_CWD_SUPPORT
#endif

kit_ecode
kit_builtins_sys_get_cwd(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
#ifdef NO_CWD_SUPPORT
  *result = KIT_NULLVAR;
  return KIT_OK;
#endif

  char buffer[FILENAME_MAX];
  if (!getcwd(buffer, sizeof(buffer))) {
    perror("Failed to get current working directory");
    *result = KIT_NULLVAR;
    return KIT_OK;
  }

  const size_t len = strlen(buffer);

  // Add a backslash at the end
  char* s = kit_xalloc(1, len + 2);
  memcpy(s, buffer, len + 1);
  s[len]     = '/';
  s[len + 1] = 0;

  kit_var cwd = {
    .type  = KIT_VARTYPE_STRING,
    .val.s = kit_refdobj_pool_acquire(vm->pool),
  };
  KIT_VAR_AS_STRING(&cwd)->s = s;

  *result = cwd;
  return KIT_OK;
}

kit_ecode
kit_builtins_sys_shell(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* cmd = KIT_VAR_AS_STRING(&args[0])->s;
  *result         = (kit_var){
    .type  = KIT_VARTYPE_INT,
    .val.i = system(cmd),
  };
  return KIT_OK;
}

void
sleep_ms(int milliseconds)
{
#ifdef WIN32
  Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
  struct timespec ts;
  ts.tv_sec  = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000L) * 1000000L;
  nanosleep(&ts, NULL);
#else
  usleep(milliseconds * 1000);
#endif
}

kit_ecode
kit_builtins_sys_sleep(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  int ms = kit_cast_to_int(&args[0]);
  sleep_ms(ms);
  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_sys_getenv(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* s = KIT_VAR_AS_STRING(&args[0])->s;

  *result = kit_make_var_from_string(vm->pool, kit_strdup(getenv(s)));
  return KIT_OK;
}

kit_ecode
kit_builtins_sys_setenv(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* name  = KIT_VAR_AS_STRING(&args[0])->s;
  const char* value = KIT_VAR_AS_STRING(&args[1])->s;

#ifdef _WIN32
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif

  *result = KIT_NULLVAR;
  return KIT_OK;
}
