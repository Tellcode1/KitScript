#include "../../inc/cast.h"
#include "../../inc/list.h"
#include "../../inc/stdafx.h"
#include "../../inc/var.h"

#include <time.h>

// START -- NOT MY CODE! -- Xoshiro128++

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdint.h>

/* This is xoshiro128++ 1.0, one of our 32-bit all-purpose, rock-solid
   generators. It has excellent speed, a state size (128 bits) that is
   large enough for mild parallelism, and it passes all tests we are aware
   of.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoshiro128+ is even faster.

   The state must be seeded so that it is not everywhere zero. */

static inline uint32_t
rotl(const uint32_t x, int k)
{ return (x << k) | (x >> (32 - k)); }

static uint32_t s[4];

uint32_t
next(void)
{
  const uint32_t result = rotl(s[0] + s[3], 7) + s[0];

  const uint32_t t = s[1] << 9;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 11);

  return result;
}

/* This is the jump function for the generator. It is equivalent
   to 2^64 calls to next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */

void
jump(void)
{
  static const uint32_t JUMP[] = { 0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b };

  uint32_t s0 = 0;
  uint32_t s1 = 0;
  uint32_t s2 = 0;
  uint32_t s3 = 0;
  for (int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
    for (int b = 0; b < 32; b++) {
      if (JUMP[i] & UINT32_C(1) << b) {
        s0 ^= s[0];
        s1 ^= s[1];
        s2 ^= s[2];
        s3 ^= s[3];
      }
      next();
    }

  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
}

/* This is the long-jump function for the generator. It is equivalent to
   2^96 calls to next(); it can be used to generate 2^32 starting points,
   from each of which jump() will generate 2^32 non-overlapping
   subsequences for parallel distributed computations. */

void
long_jump(void)
{
  static const uint32_t LONG_JUMP[] = { 0xb523952e, 0x0b6f099f, 0xccf5a0ef, 0x1c580662 };

  uint32_t s0 = 0;
  uint32_t s1 = 0;
  uint32_t s2 = 0;
  uint32_t s3 = 0;
  for (int i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
    for (int b = 0; b < 32; b++) {
      if (LONG_JUMP[i] & UINT32_C(1) << b) {
        s0 ^= s[0];
        s1 ^= s[1];
        s2 ^= s[2];
        s3 ^= s[3];
      }
      next();
    }

  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
}

// END -- Xoshiro128++

static inline void
seed_xoshiro(const char* str)
{
  u64 h1 = 0xdeadbeef;
  u64 h2 = 0x41c6ce57;

  const size_t len = strlen(str);
  for (size_t i = 0; i < len; i++) {
    const uchar c = (uchar)str[i];

    h1 ^= c;
    h1 = (h1 * 0x9E3779B97F4A7C15);
    h2 ^= c;
    h2 = h2 * 0xBF58476D1CE4E5B9;
  }

  h1 ^= h1 >> 33;
  h1 *= 0xff51afd7ed558ccd;
  h1 ^= h1 >> 33;

  h2 ^= h2 >> 33;
  h2 *= 0xc4ceb9fe1a85ec53;
  h2 ^= h2 >> 33;

  s[0] = h1 & 0xFFFFFFFF;
  s[1] = (h1 >> 32) & 0xFFFFFFFF;
  s[2] = h2 & 0xFFFFFFFF;
  s[3] = (h2 >> 32) & 0xFFFFFFFF;
}

e_var
eb_rand_seed(e_var* args, u32 nargs)
{
  seed_xoshiro(E_VAR_AS_STRING(&args[0])->s);
  return E_NULLVAR;
}

static inline u32
xrand(void)
{ return next(); }

e_var
eb_rand_int(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_INT, .val = { .i = (int)xrand() } };
}

e_var
eb_rand_range(e_var* args, u32 nargs)
{
  (void)nargs;
  int min = e_cast_to_int(&args[0]);
  int max = e_cast_to_int(&args[1]);
  return (e_var){ .type = E_VARTYPE_INT, .val = { .i = ((int)xrand() * (max - min + 1)) + min } };
}

e_var
eb_rand_float(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_FLOAT, .val = { .f = (double)xrand() / (double)UINT32_MAX } };
}

#include "../../inc/bfunc.h"
e_var
eb_rand_vec2(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC2, .val = { .vec2 = { (double)xrand() / (double)UINT32_MAX, (double)xrand() / (double)UINT32_MAX } } };
}

e_var
eb_rand_vec3(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC3,
                  .val = { .vec3 = {
                               (double)xrand() / (double)UINT32_MAX, (double)xrand() / (double)UINT32_MAX, (double)xrand() / (double)UINT32_MAX } } };
}

e_var
eb_rand_vec4(e_var* args, u32 nargs)
{
  (void)nargs;
  return (e_var){ .type = E_VARTYPE_VEC4,
                  .val  = { .vec4 = { (double)xrand() / (double)UINT32_MAX,
                                      (double)xrand() / (double)UINT32_MAX,
                                      (double)xrand() / (double)UINT32_MAX,
                                      (double)xrand() / (double)UINT32_MAX } } };
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

  u64 range = (u64)(i64)max - (u64)(i64)min + 1;
  for (i32 i = 0; i < num; i++) {
    e_var e = {
      .type  = E_VARTYPE_INT,
      .val.i = (int)((i64)min + (i64)(xrand() % range)),
    };
    int err = e_list_append(&e, E_VAR_AS_LIST(&v));
    if (err) return E_NULLVAR;
  }

  return v;
}