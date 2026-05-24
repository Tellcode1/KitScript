#include "bfunc.h"

#include "var.h"

#include <stdio.h>
#include <time.h>

FILE* e_log_file = NULL;

static inline void
gettime(char* buffer, size_t size)
{
  time_t     now     = time(NULL);
  struct tm* tm_info = localtime(&now);
  strftime(buffer, size, "%H:%M:%S", tm_info);
}

e_var
eb_log_err(e_var* args, u32 nargs)
{
  char buffer[32];
  gettime(buffer, sizeof(buffer));

  FILE* f = e_log_file ? e_log_file : stderr;
  fprintf(f, "[%s] [error] -- ", buffer);

  for (u32 i = 0; i < nargs; i++) { e_var_print(&args[i], f); }

  fputc('\n', f);

  return E_NULLVAR;
}

e_var
eb_log_warn(e_var* args, u32 nargs)
{
  char buffer[32];
  gettime(buffer, sizeof(buffer));

  FILE* f = e_log_file ? e_log_file : stderr;
  fprintf(f, "[%s] [warn] -- ", buffer);

  for (u32 i = 0; i < nargs; i++) { e_var_print(&args[i], f); }

  fputc('\n', f);

  return E_NULLVAR;
}

e_var
eb_log_info(e_var* args, u32 nargs)
{
  char buffer[32];
  gettime(buffer, sizeof(buffer));

  FILE* f = e_log_file ? e_log_file : stderr;
  fprintf(f, "[%s] [info] -- ", buffer);

  for (u32 i = 0; i < nargs; i++) { e_var_print(&args[i], f); }

  fputc('\n', f);

  return E_NULLVAR;
}
