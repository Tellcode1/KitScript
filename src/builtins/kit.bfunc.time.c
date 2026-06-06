#define _POSIX_C_SOURCE 199309L

#include "../../inc/kit.bfunc.time.h"

#include "../../inc/kit.cast.h"
#include "../../inc/kit.pool.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.struct.h"
#include "../../inc/kit.var.h"

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

kit_var
kit_builtins_time_now(kit_var* args, u32 nargs)
{
  return (kit_var){
    .type  = KIT_VARTYPE_FLOAT,
    .val.f = pl_now(),
  };
}

kit_var
kit_builtins_time_mono(kit_var* args, u32 nargs)
{
  return (kit_var){
    .type  = KIT_VARTYPE_FLOAT,
    .val.f = pl_mono(),
  };
}

static inline void
extract_time_from_tm(const struct tm* t, kit_var* s)
{
  kit_struct_member_pair pairs[] = {
    (kit_struct_member_pair){ .name = "sec", .value = kit_var_from_int(t->tm_sec) },
    (kit_struct_member_pair){ .name = "min", .value = kit_var_from_int(t->tm_min) },
    (kit_struct_member_pair){ .name = "hour", .value = kit_var_from_int(t->tm_hour) },
    (kit_struct_member_pair){ .name = "day", .value = kit_var_from_int(t->tm_mday) },
    (kit_struct_member_pair){ .name = "wday", .value = kit_var_from_int(t->tm_wday + 1) },
    (kit_struct_member_pair){ .name = "yday", .value = kit_var_from_int(t->tm_yday) },
    (kit_struct_member_pair){ .name = "mon", .value = kit_var_from_int(t->tm_mon + 1) },
    (kit_struct_member_pair){ .name = "year", .value = kit_var_from_int(t->tm_year + 1900) },
  };
  kit_struct_set_member_pairs(KIT_VAR_AS_STRUCT(s), KIT_ARRLEN(pairs), pairs);
}

kit_var
kit_builtins_time_local(kit_var* args, u32 nargs)
{
  struct tm t   = { 0 };
  time_t    now = time(NULL);

  struct tm* tmp = localtime(&now);
  if (!tmp) return KIT_NULLVAR;

  memcpy(&t, tmp, sizeof(struct tm));

  kit_var s   = { 0 };
  s.type      = KIT_VARTYPE_STRUCT;
  s.val.struc = kit_refdobj_pool_acquire(&ge_pool);

  const char* members[]                = { "sec", "min", "hour", "day", "wday", "yday", "mon", "year" };
  KIT_VAR_AS_STRUCT(&s)->name          = "time::timestamp";
  KIT_VAR_AS_STRUCT(&s)->member_count  = KIT_ARRLEN(members);
  KIT_VAR_AS_STRUCT(&s)->member_hashes = kit_xalloc(KIT_ARRLEN(members), sizeof(u32));
  KIT_VAR_AS_STRUCT(&s)->member_names  = (const char**)kit_xalloc(KIT_ARRLEN(members), sizeof(char*));
  KIT_VAR_AS_STRUCT(&s)->members       = kit_xalloc(KIT_ARRLEN(members), sizeof(kit_var));

  for (u32 i = 0; i < KIT_ARRLEN(members); i++) {
    KIT_VAR_AS_STRUCT(&s)->member_hashes[i] = kit_hash(members[i], strlen(members[i]));
    KIT_VAR_AS_STRUCT(&s)->member_names[i]  = members[i];
  }

  extract_time_from_tm(&t, &s);

  return s;
}

kit_var
kit_builtins_time_utc(kit_var* args, u32 nargs)
{
  struct tm t   = { 0 };
  time_t    now = time(NULL);

  struct tm* tmp = localtime(&now);
  if (!tmp) return KIT_NULLVAR;

  memcpy(&t, tmp, sizeof(struct tm));

  kit_var s   = { 0 };
  s.type      = KIT_VARTYPE_STRUCT;
  s.val.struc = kit_refdobj_pool_acquire(&ge_pool);

  const char* members[] = { "sec", "min", "hour", "day", "wday", "yday", "mon", "year" };

  KIT_VAR_AS_STRUCT(&s)->name          = "time::timestamp";
  KIT_VAR_AS_STRUCT(&s)->member_count  = KIT_ARRLEN(members);
  KIT_VAR_AS_STRUCT(&s)->member_hashes = kit_xalloc(KIT_ARRLEN(members), sizeof(u32));
  KIT_VAR_AS_STRUCT(&s)->member_names  = (const char**)kit_xalloc(KIT_ARRLEN(members), sizeof(char*));
  KIT_VAR_AS_STRUCT(&s)->members       = kit_xalloc(KIT_ARRLEN(members), sizeof(kit_var));

  for (u32 i = 0; i < KIT_ARRLEN(members); i++) {
    KIT_VAR_AS_STRUCT(&s)->member_hashes[i] = kit_hash(members[i], strlen(members[i]));
    KIT_VAR_AS_STRUCT(&s)->member_names[i]  = members[i];
  }

  extract_time_from_tm(&t, &s);

  return s;
}
