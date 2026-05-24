#ifndef WYHASH_WHY_DO_YOU_NOT_HAVE_A_HEADER_GUARD
#define WYHASH_WHY_DO_YOU_NOT_HAVE_A_HEADER_GUARD

/**
 * I (Tellcode) added the header guard and my formatter formatted the code, removed the __* prefix and replaced it with wyinternals_*.
 * I don't need to state this, but I will.
 * Also later made it compliant with my clang tidy in like 2 minutes. May have broken something.
 */

// Author: Wang Yi <godspeed_china@yeah.net>
#include <stdint.h>
#include <string.h>
#ifndef WYHASH32_BIG_ENDIAN
static inline unsigned
wyinternals_wyr32(const uint8_t* p)
{
  unsigned v = 0;
  memcpy(&v, p, 4);
  return v;
}
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
static inline unsigned
wyinternals_wyr32(const uint8_t* p)
{
  unsigned v;
  memcpy(&v, p, 4);
  return __builtin_bswap32(v);
}
#elif defined(_MSC_VER)
static inline unsigned
wyinternals_wyr32(const uint8_t* p)
{
  unsigned v;
  memcpy(&v, p, 4);
  return _byteswap_ulong(v);
}
#endif
static inline unsigned
wyinternals_wyr24(const uint8_t* p, unsigned k)
{ return (((unsigned)p[0]) << 16) | (((unsigned)p[k >> 1]) << 8) | p[k - 1]; }
static inline void
wyinternals_wymix32(unsigned* A, unsigned* B)
{
  uint64_t c = *A ^ 0x53c5ca59U;
  c *= *B ^ 0x74743c1bU;
  *A = (unsigned)c;
  *B = (unsigned)(c >> 32);
}
// This version is vulnerable when used with a few bad seeds, which should be skipped beforehand:
// 0x429dacdd, 0xd637dbf3
static inline unsigned
wyhash32(const void* key, uint64_t len, unsigned seed)
{
  const uint8_t* p    = (const uint8_t*)key;
  uint64_t       i    = len;
  unsigned       see1 = (unsigned)len;
  seed ^= (unsigned)(len >> 32);
  wyinternals_wymix32(&seed, &see1);
  for (; i > 8; i -= 8, p += 8) {
    seed ^= wyinternals_wyr32(p);
    see1 ^= wyinternals_wyr32(p + 4);
    wyinternals_wymix32(&seed, &see1);
  }
  if (i >= 4) {
    seed ^= wyinternals_wyr32(p);
    see1 ^= wyinternals_wyr32(p + i - 4);
  } else if (i) {
    seed ^= wyinternals_wyr24(p, (unsigned)i);
  }
  wyinternals_wymix32(&seed, &see1);
  wyinternals_wymix32(&seed, &see1);
  return seed ^ see1;
}
// duplicate definition in wyhash.h also
#ifndef wyhash_final_version_3
static inline uint64_t
wyrand(uint64_t* seed)
{
  *seed += 0xa0761d6478bd642fULL;
  uint64_t see1 = *seed ^ 0xe7037ed1a0b428dbULL;
  see1 *= (see1 >> 32) | (see1 << 32);
  return (*seed * ((*seed >> 32) | (*seed << 32))) ^ ((see1 >> 32) | (see1 << 32));
}
#endif
static inline unsigned
wy32x32(unsigned a, unsigned b)
{
  wyinternals_wymix32(&a, &b);
  wyinternals_wymix32(&a, &b);
  return a ^ b;
}
static inline float
wy2u01(unsigned r)
{
  const float _wynorm = 1.0F / (1ULL << 23);
  return (float)(r >> 9) * _wynorm;
}
static inline float
wy2gau(unsigned r)
{
  const float _wynorm = 1.0F / (1ULL << 9);
  return ((float)((r & 0x3FF) + ((r >> 10) & 0x3FF) + ((r >> 20) & 0x3FF)) * _wynorm) - 3.0F;
}

#endif // WYHASH_WHY_DO_YOU_NOT_HAVE_A_HEADER_GUARD