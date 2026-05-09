#include "cast.h"
#include "list.h"
#include "stdafx.h"
#include "var.h"

#include <time.h>

e_var
eb_rand_int(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_INT, .val = { .i = rand() } };
}

e_var
eb_rand_float(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_FLOAT, .val = { .f = (double)rand() / (double)RAND_MAX } };
}

e_var
eb_rand_vec2(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC2, .val = { .vec2 = { (double)rand() / (double)RAND_MAX, (double)rand() / (double)RAND_MAX } } };
}

e_var
eb_rand_vec3(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC3,
                  .val  = { .vec3 = { (double)rand() / (double)RAND_MAX, (double)rand() / (double)RAND_MAX, (double)rand() / (double)RAND_MAX } } };
}

e_var
eb_rand_vec4(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC4,
                  .val  = { .vec4 = { (double)rand() / (double)RAND_MAX,
                                      (double)rand() / (double)RAND_MAX,
                                      (double)rand() / (double)RAND_MAX,
                                      (double)rand() / (double)RAND_MAX } } };
}

static inline u32
rol(const u32 x, u32 k)
{ return (x << k) | (x >> (32 - k)); }

static inline u32
xoshiro(u32* s)
{
  const u32 result = rol(s[1] * 5U, 7U) * 9U;

  const u32 t = s[1] << 9U;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rol(s[3], 11U);

  return result;
}

e_var
eb_rand_list(e_var* args, u32 nargs)
{
  (void)nargs;

  int min = e_cast_to_int(&args[0]);
  int max = e_cast_to_int(&args[1]);
  int num = e_cast_to_int(&args[2]);
  if (max < min) {
    int tmp = min;
    min     = max;
    max     = tmp;
  }

  e_var v    = (e_var){ .type = E_VARTYPE_LIST };
  v.val.list = e_refdobj_pool_acquire(&ge_pool);

  e_list_init(NULL, 0, E_VAR_AS_LIST(&v));

  u32 seed[4] = { time(NULL), (u32)(u64)&v, (u32)(u64)rand, clock() };

  u64 range = (u64)(i64)max - (u64)(i64)min + 1;
  for (i32 i = 0; i < num; i++) {
    e_var e = {
      .type  = E_VARTYPE_INT,
      .val.i = (int)((i64)min + (i64)(xoshiro(seed) % range)),
    };
    int err = e_list_append(&e, E_VAR_AS_LIST(&v));
    if (err) return E_NULLVAR;
  }

  return v;
}