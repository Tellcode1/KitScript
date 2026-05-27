#define _POSIX_C_SOURCE 199309L

#include "../../inc/bfunc.time.h"

#include "../../inc/cast.h"
#include "../../inc/pool.h"
#include "../../inc/stdafx.h"
#include "../../inc/struct.h"
#include "../../inc/var.h"

#include <time.h>

#if defined(_WIN32)

#  define WIN32_LEAN_AND_MEAN
#  include <fileapi.h>
#  include <windows.h>

static double
pl_now(void)
{
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);

  ULARGE_INTEGER t;
  t.LowPart  = ft.dwLowDateTime;
  t.HighPart = ft.dwHighDateTime;

  /* FILETIME = 100ns intervals since Jan 1 1601 */
  const uint64_t WINDOWS_TO_UNIX_EPOCH = 116444736000000000ULL;

  uint64_t time = t.QuadPart - WINDOWS_TO_UNIX_EPOCH;

  return (double)time / 10000000.0;
}

static double
pl_mono(void)
{
  static LARGE_INTEGER freq;
  static bool          initialized = 0;

  if (!initialized) {
    QueryPerformanceFrequency(&freq);
    initialized = true;
  }

  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);

  return (double)counter.QuadPart / (double)freq.QuadPart;
}

#else

static double
pl_now(void)
{
  struct timespec ts;

#  if defined(CLOCK_REALTIME)
  clock_gettime(CLOCK_REALTIME, &ts);
#  endif

  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
}

static double
pl_mono(void)
{
  struct timespec ts;

#  if defined(CLOCK_MONOTONIC)
  clock_gettime(CLOCK_MONOTONIC, &ts);
#  else
  /* this is really bad!! */
  clock_gettime(CLOCK_REALTIME, &ts);
#  endif

  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
}

#endif

e_var
eb_time_now(e_var* args, u32 nargs)
{
  return (e_var){
    .type  = E_VARTYPE_FLOAT,
    .val.f = pl_now(),
  };
}

e_var
eb_time_mono(e_var* args, u32 nargs)
{
  return (e_var){
    .type  = E_VARTYPE_FLOAT,
    .val.f = pl_mono(),
  };
}

static inline void
extract_time_from_tm(const struct tm* t, e_var* s)
{
  e_struct_member_pair pairs[] = {
    (e_struct_member_pair){ .name = "sec", .value = e_var_from_int(t->tm_sec) },
    (e_struct_member_pair){ .name = "min", .value = e_var_from_int(t->tm_min) },
    (e_struct_member_pair){ .name = "hour", .value = e_var_from_int(t->tm_hour) },
    (e_struct_member_pair){ .name = "day", .value = e_var_from_int(t->tm_mday) },
    (e_struct_member_pair){ .name = "wday", .value = e_var_from_int(t->tm_wday + 1) },
    (e_struct_member_pair){ .name = "yday", .value = e_var_from_int(t->tm_yday) },
    (e_struct_member_pair){ .name = "mon", .value = e_var_from_int(t->tm_mon + 1) },
    (e_struct_member_pair){ .name = "year", .value = e_var_from_int(t->tm_year + 1900) },
  };
  e_struct_set_member_pairs(E_VAR_AS_STRUCT(s), E_ARRLEN(pairs), pairs);
}

e_var
eb_time_local(e_var* args, u32 nargs)
{
  struct tm t   = { 0 };
  time_t    now = time(NULL);

  struct tm* tmp = localtime(&now);
  if (!tmp) return E_NULLVAR;

  memcpy(&t, tmp, sizeof(struct tm));

  e_var s     = { 0 };
  s.type      = E_VARTYPE_STRUCT;
  s.val.struc = e_refdobj_pool_acquire(&ge_pool);

  const char* members[]              = { "sec", "min", "hour", "day", "wday", "yday", "mon", "year" };
  E_VAR_AS_STRUCT(&s)->member_count  = E_ARRLEN(members);
  E_VAR_AS_STRUCT(&s)->member_hashes = e_xalloc(E_ARRLEN(members), sizeof(u32));
  E_VAR_AS_STRUCT(&s)->member_names  = (const char**)e_xalloc(E_ARRLEN(members), sizeof(char*));
  E_VAR_AS_STRUCT(&s)->members       = e_xalloc(E_ARRLEN(members), sizeof(e_var));

  for (u32 i = 0; i < E_ARRLEN(members); i++) {
    E_VAR_AS_STRUCT(&s)->member_hashes[i] = e_hash(members[i], strlen(members[i]));
    E_VAR_AS_STRUCT(&s)->member_names[i]  = members[i];
  }

  extract_time_from_tm(&t, &s);

  return s;
}

e_var
eb_time_utc(e_var* args, u32 nargs)
{
  struct tm t   = { 0 };
  time_t    now = time(NULL);

  struct tm* tmp = localtime(&now);
  if (!tmp) return E_NULLVAR;

  memcpy(&t, tmp, sizeof(struct tm));

  e_var s     = { 0 };
  s.type      = E_VARTYPE_STRUCT;
  s.val.struc = e_refdobj_pool_acquire(&ge_pool);

  const char* members[] = { "sec", "min", "hour", "day", "wday", "yday", "mon", "year" };

  E_VAR_AS_STRUCT(&s)->member_count  = E_ARRLEN(members);
  E_VAR_AS_STRUCT(&s)->member_hashes = e_xalloc(E_ARRLEN(members), sizeof(u32));
  E_VAR_AS_STRUCT(&s)->member_names  = (const char**)e_xalloc(E_ARRLEN(members), sizeof(char*));
  E_VAR_AS_STRUCT(&s)->members       = e_xalloc(E_ARRLEN(members), sizeof(e_var));

  for (u32 i = 0; i < E_ARRLEN(members); i++) {
    E_VAR_AS_STRUCT(&s)->member_hashes[i] = e_hash(members[i], strlen(members[i]));
    E_VAR_AS_STRUCT(&s)->member_names[i]  = members[i];
  }

  extract_time_from_tm(&t, &s);

  return s;
}
