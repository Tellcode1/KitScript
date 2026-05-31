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

#include "../../inc/cast.h"
#include "../../inc/list.h"
#include "../../inc/pool.h"
#include "../../inc/stdafx.h"
#include "../../inc/sysexpose.h"
#include "../../inc/var.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char** e_argv = NULL;
int    e_argc = 0;

e_var
eb_sys_get_cmd_args(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  e_var l = {
    .type     = E_VARTYPE_LIST,
    .val.list = e_refdobj_pool_acquire(&ge_pool),
  };
  e_list_init(NULL, 0, E_VAR_AS_LIST(&l));

  /* Start from 2. We don't need to expose the executor and script file */
  for (u32 i = 2; i < e_argc; i++) {
    e_var arg = e_make_var_from_string(e_strdup(e_argv[i]));
    e_list_append(&arg, E_VAR_AS_LIST(&l));

    e_var_release(&arg);
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

e_var
eb_sys_get_cwd(e_var* args, u32 nargs)
{
#ifdef NO_CWD_SUPPORT
  return E_NULLVAR;
#endif

  char buffer[FILENAME_MAX];
  if (!getcwd(buffer, sizeof(buffer))) {
    perror("Failed to get current working directory");
    return E_NULLVAR;
  }

  const size_t len = strlen(buffer);

  // Add a backslash at the end
  char* s = e_xalloc(1, len + 2);
  memcpy(s, buffer, len + 1);
  s[len]     = '/';
  s[len + 1] = 0;

  e_var cwd = {
    .type  = E_VARTYPE_STRING,
    .val.s = e_refdobj_pool_acquire(&ge_pool),
  };
  E_VAR_AS_STRING(&cwd)->s = s;

  return cwd;
}

e_var
eb_sys_shell(e_var* args, u32 nargs)
{
  const char* cmd = E_VAR_AS_STRING(&args[0])->s;
  return (e_var){
    .type  = E_VARTYPE_INT,
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

e_var
eb_sys_sleep(e_var* args, u32 nargs)
{
  int ms = e_cast_to_int(&args[0]);
  sleep_ms(ms);
  return E_NULLVAR;
}

e_var
eb_sys_getenv(e_var* args, u32 nargs)
{
  const char* s = E_VAR_AS_STRING(&args[0])->s;
  return e_make_var_from_string(e_strdup(getenv(s)));
}

e_var
eb_sys_setenv(e_var* args, u32 nargs)
{
  const char* name  = E_VAR_AS_STRING(&args[0])->s;
  const char* value = E_VAR_AS_STRING(&args[1])->s;

#ifdef _WIN32
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif

  return E_NULLVAR;
}
