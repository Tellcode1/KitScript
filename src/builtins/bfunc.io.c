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

#include "../../inc/bfunc.io.h"

#include "../../inc/bvar.h"
#include "../../inc/cast.h"
#include "../../inc/list.h"
#include "../../inc/pool.h"
#include "../../inc/var.h"

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
file_from_var(const e_var* v)
{
  FILE* f = NULL;
  if (v->type == E_VARTYPE_DESCRIPTOR) {
    f = (FILE*)v->val.descriptor;
  } else if (v->type == E_VARTYPE_INT) {
    if (v->val.i == EB_IO_STDOUT) {
      f = stdout; // you can change the stdout/in/err pointer btw!
    } else if (v->val.i == EB_IO_STDIN) {
      f = stdin;
    } else if (v->val.i == EB_IO_STDERR) {
      f = stderr;
    }
  }
  return f;
}

e_var
var_from_file(FILE* f)
{
  e_var yay          = { .type = E_VARTYPE_DESCRIPTOR };
  yay.val.descriptor = (void*)f;
  return yay;
}

e_var
eb_io_read(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int   nbytes = args[1].val.i;
  char* s      = e_xalloc(1, nbytes + 1);

  size_t nread = fread(s, 1, nbytes, f);
  if (nread < nbytes) s[nread] = 0;

  e_var v = {
    .type  = E_VARTYPE_STRING,
    .val.s = e_refdobj_pool_acquire(&ge_pool),
  };
  E_VAR_AS_STRING(&v)->s = s;

  return v;
}

e_var
eb_io_write(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  char* s = E_VAR_AS_STRING(&args[1])->s;
  fwrite(s, 1, strlen(s), f);

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_seek(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int offset      = e_cast_to_int(&args[1]);
  int relative_to = e_cast_to_int(&args[2]);

  int c_rel = 0;
  switch (relative_to) {
    case EB_IO_REL_TO_START: c_rel = SEEK_SET; break;
    case EB_IO_REL_TO_END: c_rel = SEEK_END; break;
    default:
    case EB_IO_REL_TO_CURR: c_rel = SEEK_CUR; break;
  }

  fseek(f, offset, c_rel);

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_ptell(e_var* args, u32 nargs)
{
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  return (e_var){ .type = E_VARTYPE_INT, .val.i = (int)ftell(f) };
}

e_var
eb_io_readln(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  return e_make_var_from_string(e_read_full_line(f));
}

e_var
eb_io_println(e_var* args, u32 nargs)
{
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  for (u32 i = 1; i < nargs; i++) { e_var_print(&args[i], f); }
  fputc('\n', f);

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_print(e_var* args, u32 nargs)
{
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  for (u32 i = 1; i < nargs; i++) { e_var_print(&args[i], f); }

  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_getc(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int i = fgetc(f);

  return (e_var){
    .type  = i < 0 ? E_VARTYPE_NULL : E_VARTYPE_CHAR,
    .val.c = (char)i,
  };
}

e_var
eb_io_putc(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;

  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  int ch = e_cast_to_int(&args[1]);
  fputc(ch, f);

  return (e_var){
    .type = E_VARTYPE_NULL,
  };
}

e_var
eb_io_at_eof(e_var* args, u32 nargs)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  return (e_var){ .type = E_VARTYPE_BOOL, .val.b = (bool)feof(f) };
}

e_var
eb_io_open(e_var* args, u32 nargs)
{
  (void)nargs;
  const char* path = E_VAR_AS_STRING(&args[0])->s;
  const char* mode = E_VAR_AS_STRING(&args[1])->s;

  FILE* f = fopen(path, mode);
  if (!f) { return (e_var){ .type = E_VARTYPE_NULL }; }

  return var_from_file(f);
}

e_var
eb_io_flush(e_var* args, u32 nargs)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  fflush(f);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_close(e_var* args, u32 nargs)
{
  (void)nargs;
  FILE* f = file_from_var(&args[0]);
  if (!f) return (e_var){ .type = E_VARTYPE_NULL };

  fclose(f);
  return (e_var){ .type = E_VARTYPE_NULL };
}

e_var
eb_io_error(e_var* args, u32 nargs)
{
  (void)args;
  (void)nargs;
  return e_make_var_from_string(e_strdup(strerror(errno)));
}

e_var
eb_io_type(e_var* args, u32 nargs)
{
  (void)nargs;
  const char* path = E_VAR_AS_STRING(&args[0])->s;
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES) { return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_UNKNOWN }; }

  if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
    return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_LINK };
  } else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
    return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_DIRECTORY };
  } else {
    return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_FILE };
  }
#else
  struct stat sb;
  if (stat(path, &sb) == -1) { return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_UNKNOWN }; }

  if (S_ISLNK(sb.st_mode)) { return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_LINK }; }
  if (S_ISDIR(sb.st_mode)) { return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_DIRECTORY }; }
  if (S_ISREG(sb.st_mode)) { return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_FILE }; }
#endif

  return (e_var){ .type = E_VARTYPE_INT, .val.i = EB_IO_UNKNOWN };
}

e_var
eb_io_listdir(e_var* args, u32 nargs)
{
  const char* path = E_VAR_AS_STRING(&args[0])->s;
  if (!path) return E_NULLVAR;

  e_var l = { .type = E_VARTYPE_LIST, .val.list = e_refdobj_pool_acquire(&ge_pool) };
  if (e_list_init(NULL, 0, E_VAR_AS_LIST(&l))) {
    e_refdobj_pool_return(&ge_pool, l.val.list);
    return E_NULLVAR;
  }

  DIR* d = opendir(path);
  if (!d) return E_NULLVAR;

  struct dirent* ent = NULL;
  while ((ent = readdir(d)) != NULL) {
    /**
     * .. are present even for directories that reasonably
     * won't have an upper level (root FS)? weird.
     */
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

    e_var v = e_make_var_from_string(e_strdup(ent->d_name));
    e_list_append(&v, E_VAR_AS_LIST(&l));
    e_var_release(&v);
  }

  closedir(d);

  return l;
}

e_var
eb_io_exists(e_var* args, u32 nargs)
{
  const char* path = E_VAR_AS_STRING(&args[0])->s;

  /* fopen returns null for directories on windows. won't blame em. */
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(path);
  return e_var_from_bool(attr != INVALID_FILE_ATTRIBUTES);
#else
  return e_var_from_bool(access(path, F_OK) == 0);
#endif
}

e_var
eb_io_mkdir(e_var* args, u32 nargs)
{
  const char* path = E_VAR_AS_STRING(&args[0])->s;

#ifdef _WIN32
#  define xmkdir(path) _mkdir(path)
#else
#  define xmkdir(path) mkdir(path, 0755)
#endif

  int e = xmkdir(path);
  return e_var_from_int(e);
}
