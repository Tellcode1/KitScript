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
#include "../../inc/kit.pool.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.sysexpose.h"
#include "../../inc/kit.var.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char** kit_argv = NULL;
int    kit_argc = 0;

kit_var
kit_builtins_sys_get_cmd_args(kit_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  kit_var l = {
    .type     = KIT_VARTYPE_LIST,
    .val.list = kit_refdobj_pool_acquire(&ge_pool),
  };
  kit_list_init(NULL, 0, KIT_VAR_AS_LIST(&l));

  /* Start from 2. We don't need to expose the executor and script file */
  for (u32 i = 2; i < kit_argc; i++) {
    kit_var arg = kit_make_var_from_string(kit_strdup(kit_argv[i]));
    kit_list_append(&arg, KIT_VAR_AS_LIST(&l));

    kit_var_release(&arg);
  }

  return l;
}

#ifdef _WIN32
#  include <direct.h>
#  define getcwd _getcwd
#elif defined(__unix__) || defined(__posix__)
#  include <unistd.h>
#else
#  define NO_CWD_SUPPORT
#endif

kit_var
kit_builtins_sys_get_cwd(kit_var* args, u32 nargs)
{
#ifdef NO_CWD_SUPPORT
  return KIT_NULLVAR;
#endif

  char buffer[FILENAME_MAX];
  if (!getcwd(buffer, sizeof(buffer))) {
    perror("Failed to get current working directory");
    return KIT_NULLVAR;
  }

  const size_t len = strlen(buffer);

  // Add a backslash at the end
  char* s = kit_xalloc(1, len + 2);
  memcpy(s, buffer, len + 1);
  s[len]     = '/';
  s[len + 1] = 0;

  kit_var cwd = {
    .type  = KIT_VARTYPE_STRING,
    .val.s = kit_refdobj_pool_acquire(&ge_pool),
  };
  KIT_VAR_AS_STRING(&cwd)->s = s;

  return cwd;
}

kit_var
kit_builtins_sys_shell(kit_var* args, u32 nargs)
{
  const char* cmd = KIT_VAR_AS_STRING(&args[0])->s;
  return (kit_var){
    .type  = KIT_VARTYPE_INT,
    .val.i = system(cmd),
  };
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

kit_var
kit_builtins_sys_sleep(kit_var* args, u32 nargs)
{
  int ms = kit_cast_to_int(&args[0]);
  sleep_ms(ms);
  return KIT_NULLVAR;
}

kit_var
kit_builtins_sys_getenv(kit_var* args, u32 nargs)
{
  const char* s = KIT_VAR_AS_STRING(&args[0])->s;
  return kit_make_var_from_string(kit_strdup(getenv(s)));
}

kit_var
kit_builtins_sys_setenv(kit_var* args, u32 nargs)
{
  const char* name  = KIT_VAR_AS_STRING(&args[0])->s;
  const char* value = KIT_VAR_AS_STRING(&args[1])->s;

#ifdef _WIN32
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif

  return KIT_NULLVAR;
}
