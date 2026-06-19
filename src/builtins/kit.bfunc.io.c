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

#ifdef _WIN32
#  include "dirent.h"
#else
#  include <dirent.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <direct.h>
#  include <windows.h>
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

FILE*
file_from_var(const kit_var* v)
{
  FILE* f = NULL;
  if (v->type == KIT_VARTYPE_DESCRIPTOR) {
    f = (FILE*)v->val.descriptor;
  } else if (v->type == KIT_VARTYPE_INT) {
    if (v->val.i == KIT_BUILTIN_CONST_IO_STDOUT) {
      f = stdout; // you can change the stdout/in/err pointer btw!
    } else if (v->val.i == KIT_BUILTIN_CONST_IO_STDIN) {
      f = stdin;
    } else if (v->val.i == KIT_BUILTIN_CONST_IO_STDERR) {
      f = stderr;
    }
  }
  return f;
}

kit_var
var_from_file(FILE* f)
{
  kit_var yay        = { .type = KIT_VARTYPE_DESCRIPTOR };
  yay.val.descriptor = (void*)f;
  return yay;
}

kit_ecode
kit_builtins_io_read(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_OK;
  }

  int   nbytes = args[1].val.i;
  char* s      = kit_xalloc(1, nbytes + 1);

  size_t nread = fread(s, 1, nbytes, f);
  if (nread < nbytes) s[nread] = 0;

  kit_var v = {
    .type  = KIT_VARTYPE_STRING,
    .val.s = kit_refdobj_pool_acquire(vm->pool),
  };
  KIT_VAR_AS_STRING(&v)->s = s;

  *result = v;
  return KIT_OK;
}

kit_ecode
kit_builtins_io_write(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = KIT_NULLVAR;
    return KIT_EMALFORM;
  }

  char* s = KIT_VAR_AS_STRING(&args[1])->s;
  fwrite(s, 1, strlen(s), f);

  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_io_seek(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_OK;
  }

  int offset      = kit_cast_to_int(&args[1]);
  int relative_to = kit_cast_to_int(&args[2]);

  int c_rel = 0;
  switch (relative_to) {
    case KIT_BUILTIN_CONST_IO_REL_TO_START: c_rel = SEEK_SET; break;
    case KIT_BUILTIN_CONST_IO_REL_TO_END: c_rel = SEEK_END; break;
    default:
    case KIT_BUILTIN_CONST_IO_REL_TO_CURR: c_rel = SEEK_CUR; break;
  }

  fseek(f, offset, c_rel);

  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_ptell(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_EMALFORM;
  }

  *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = (int)ftell(f) };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_readln(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_EMALFORM;
  }

  *result = kit_make_var_from_string(vm->pool, kit_read_full_line(f));
  return KIT_OK;
}

kit_ecode
kit_builtins_io_println(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_EMALFORM;
  }

  for (u32 i = 1; i < nargs; i++) { kit_var_print(&args[i], f); }
  fputc('\n', f);

  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_print(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_EMALFORM;
  }

  for (u32 i = 1; i < nargs; i++) { kit_var_print(&args[i], f); }

  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_getc(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = KIT_NULLVAR;
    return KIT_OK;
  }

  int i = fgetc(f);

  *result = (kit_var){
    .type  = i < 0 ? KIT_VARTYPE_NULL : KIT_VARTYPE_CHAR,
    .val.c = (char)i,
  };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_putc(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_OK;
  }

  int ch = kit_cast_to_int(&args[1]);
  fputc(ch, f);

  *result = (kit_var){
    .type = KIT_VARTYPE_NULL,
  };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_at_eof(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_OK;
  }

  *result = (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = (bool)feof(f) };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_open(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  const char* path = KIT_VAR_AS_STRING(&args[0])->s;
  const char* mode = KIT_VAR_AS_STRING(&args[1])->s;

  FILE* f = fopen(path, mode);
  if (!f) { *result = (kit_var){ .type = KIT_VARTYPE_NULL }; }
  return KIT_OK;

  *result = var_from_file(f);
  return KIT_OK;
}

kit_ecode
kit_builtins_io_flush(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_OK;
  }

  fflush(f);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_close(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) {
    *result = (kit_var){ .type = KIT_VARTYPE_NULL };
    return KIT_OK;
  }

  fclose(f);
  *result = (kit_var){ .type = KIT_VARTYPE_NULL };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_error(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)args;
  (void)nargs;
  *result = kit_make_var_from_string(vm->pool, kit_strdup(strerror(errno)));
  return KIT_OK;
}

kit_ecode
kit_builtins_io_type(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  (void)nargs;
  const char* path = KIT_VAR_AS_STRING(&args[0])->s;
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_UNKNOWN };
    return KIT_OK;
  }

  if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_LINK };
    return KIT_OK;
  } else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_DIRECTORY };
    return KIT_OK;
  } else {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_FILE };
    return KIT_OK;
  }
#else
  struct stat sb;
  if (stat(path, &sb) == -1) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_UNKNOWN };
    return KIT_OK;
  }

  if (S_ISLNK(sb.st_mode)) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_LINK };
    return KIT_OK;
  }
  if (S_ISDIR(sb.st_mode)) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_DIRECTORY };
    return KIT_OK;
  }
  if (S_ISREG(sb.st_mode)) {
    *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_FILE };
    return KIT_OK;
  }
#endif

  *result = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_BUILTIN_CONST_IO_UNKNOWN };
  return KIT_OK;
}

kit_ecode
kit_builtins_io_listdir(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* path = KIT_VAR_AS_STRING(&args[0])->s;
  if (!path) {
    *result = KIT_NULLVAR;
    return KIT_OK;
  }

  kit_var l = { .type = KIT_VARTYPE_LIST, .val.list = kit_refdobj_pool_acquire(vm->pool) };
  if (kit_list_init(vm->pool, NULL, 0, KIT_VAR_AS_LIST(&l))) {
    kit_refdobj_pool_return(vm->pool, l.val.list);

    *result = KIT_NULLVAR;
    return KIT_OK;
  }

  DIR* d = opendir(path);
  if (!d) {
    *result = KIT_NULLVAR;
    return KIT_OK;
  }

  struct dirent* ent = NULL;
  while ((ent = readdir(d)) != NULL) {
    /**
     * .. are present even for directories that reasonably
     * won't have an upper level (root FS)? weird.
     */
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

    kit_var v = kit_make_var_from_string(vm->pool, kit_strdup(ent->d_name));
    kit_list_append(vm->pool, &v, KIT_VAR_AS_LIST(&l));
    kit_var_release(vm->pool, &v);
  }

  closedir(d);

  *result = l;
  return KIT_OK;
}

kit_ecode
kit_builtins_io_exists(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* path = KIT_VAR_AS_STRING(&args[0])->s;

  /* fopen returns null for directories on windows. won't blame em. */
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path);
  *result    = kit_var_from_bool(attr != INVALID_FILE_ATTRIBUTES);
  return KIT_OK;
#else
  *result = kit_var_from_bool(access(path, F_OK) == 0);
  return KIT_OK;
#endif
}

kit_ecode
kit_builtins_io_mkdir(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  const char* path = KIT_VAR_AS_STRING(&args[0])->s;

#ifdef _WIN32
#  define xmkdir(path) _mkdir(path)
#else
#  define xmkdir(path) mkdir(path, 0755)
#endif

  int e   = xmkdir(path);
  *result = kit_var_from_int(e);
  return KIT_OK;
}
