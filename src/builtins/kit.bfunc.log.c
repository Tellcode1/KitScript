#include "../../inc/kit.bfunc.h"
#include "../../inc/kit.var.h"

#include <stdio.h>
#include <time.h>

FILE* kit_log_file = NULL;

static inline void
gettime(char* buffer, size_t size)
{
  time_t     now     = time(NULL);
  struct tm* tm_info = localtime(&now);
  strftime(buffer, size, "%H:%M:%S", tm_info);
}

kit_ecode
kit_builtins_log_err(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  char buffer[32];
  gettime(buffer, sizeof(buffer));

  FILE* f = kit_log_file ? kit_log_file : stderr;
  fprintf(f, "[%s] [error] -- ", buffer);

  for (u32 i = 0; i < nargs; i++) { kit_var_print(&args[i], f); }

  fputc('\n', f);

  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_log_warn(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  char buffer[32];
  gettime(buffer, sizeof(buffer));

  FILE* f = kit_log_file ? kit_log_file : stderr;
  fprintf(f, "[%s] [warn] -- ", buffer);

  for (u32 i = 0; i < nargs; i++) { kit_var_print(&args[i], f); }

  fputc('\n', f);

  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_log_info(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  char buffer[32];
  gettime(buffer, sizeof(buffer));

  FILE* f = kit_log_file ? kit_log_file : stderr;
  fprintf(f, "[%s] [info] -- ", buffer);

  for (u32 i = 0; i < nargs; i++) { kit_var_print(&args[i], f); }

  fputc('\n', f);

  *result = KIT_NULLVAR;
  return KIT_OK;
}
