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

#ifndef E_STDAFX_H
#define E_STDAFX_H

#include "wyhash32.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef nullptr
#  define nullptr (0)
#endif

#ifndef MAX
#  define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#  define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef E_ARRLEN
#  define E_ARRLEN(array) (sizeof(array) / sizeof(*(array)))
#endif

#ifndef E_ARR_ALLOC
#  define E_ARR_ALLOC(elem_type, n) ((elem_type*)e_xalloc((n), sizeof(elem_type)))
#endif

#ifndef E_ARR_REALLOC
#  define E_ARR_REALLOC(arr, elem_type, n) ((elem_type*)realloc((void*)(arr), sizeof(*(arr)) * (n)))
#endif

#ifndef E_ARR_FREE
#  define E_ARR_FREE(arr) (free((void*)(arr)))
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define ALIGNAS(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#  define ALIGNAS(n) __declspec(align(n))
#else
#  define ALIGNAS(n)
#endif

#if defined(__has_attribute) && __has_attribute(format)
#  define ATTR_FORMAT(...) __attribute__((format(__VA_ARGS__)))
#endif

#if defined(__has_attribute) && __has_attribute(warn_unused_result)
#  define ATTR_NODISCARD __attribute__((warn_unused_result))
#else
#  define ATTR_NODISCARD
#endif

/**
 * Returns an error code that should be checked.
 */
#define RETURNS_ERRCODE ATTR_NODISCARD

typedef uint8_t  uchar;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

static inline u32
e_hash(const void* data, size_t size)
{
#ifndef E_DONT_USE_WYHASH
  return wyhash32(data, size, 0xDEADBEEFU);
#endif

  const uchar* current    = (const uchar*)data;
  const u32    init_prime = 2166136261U;
  const u32    fnv_prime  = 16777619U;

  u32 hash = init_prime;
  for (size_t i = 0; i < size; ++i) {
    hash ^= current[i];
    hash *= fnv_prime;
  }

  return hash;
}

/**
 * Allocate a pointer with the gurantee that you will get a valid memory block in return
 * or a crash.
 */
static inline ATTR_NODISCARD void*
e_xalloc(size_t nmembers, size_t size)
{
  void* p;
  while (true) {
    p = calloc(nmembers, size);
    if (p) break;
  }
  return p;
}

static inline void
e_xfree(void** pptr)
{
  if (!pptr || !*pptr) return;
  free(*pptr);
  // If we get a SIGSEGV, we won't return, will we?
  *pptr = NULL;
}

// static inline void*
// allocator_callback(const char* file, size_t line, size_t size)
// {
//   printf("[%s:%zu] %zu bytes allocated\n", file, line, size);
//   memory_usage += size;
//   return e_xalloc(size, 1);
// }
// #define e_xalloc(size, n) allocator_callback(__FILE__, __LINE__, (size) * (n))
// #define malloc(size) allocator_callback(__FILE__, __LINE__, (size))

static inline size_t
e_strlcat(char* d, const char* s, size_t dsize)
{
  size_t dl = strlen(d);
  size_t sl = strlen(s);
  size_t i;

  if (dsize <= dl) { return dsize + sl; }

  for (i = 0; i < sl && (dl + i) < dsize - 1; i++) { d[dl + i] = s[i]; }
  d[dl + i] = 0;

  return dl + sl;
}

static inline char*
e_strlpcat(char* d, const char* s, const char* dabs, size_t dsize)
{
  size_t off = d - dabs;
  if (off >= dsize) return d;
  dsize -= off;

  size_t dl = strlen(d);
  size_t sl = strlen(s);
  size_t i;

  if (dsize <= dl) { return d; }

  for (i = 0; i < sl && (dl + i) < dsize - 1; i++) { d[dl + i] = s[i]; }
  d[dl + i] = 0;

  return &d[dl + i];
}

/* strdup not in C99. */
static inline char*
e_strdup(const char* s)
{
  size_t len = strlen(s) + 1;
  char*  p   = (char*)e_xalloc(1, len);
  if (p) { memcpy(p, s, len); }
  return p;
}

static inline char*
read_file(const char* path, u64* size)
{
  FILE* f        = nullptr;
  char* contents = nullptr;
  long  fsize    = 0;

  f = fopen(path, "rb");
  if (f == nullptr) return nullptr;

  if ((bool)fseek(f, 0, SEEK_END)) goto CLEANUP;

  fsize = ftell(f);
  if (fsize <= 0) goto CLEANUP;

  if (size != nullptr) *size = (int)fsize;

  if ((bool)fseek(f, 0, SEEK_SET)) goto CLEANUP;

  contents = (char*)malloc(fsize + 1);
  if (fread(contents, fsize, 1, f) != 1) { goto CLEANUP; }
  contents[fsize] = 0;

  fclose(f);

  return contents;

CLEANUP:
  if (f != nullptr) fclose(f);
  if (contents != nullptr) free(contents);
  return nullptr;
}

static inline void* e_aligned_malloc(size_t size, size_t alignment);
static inline void* e_aligned_realloc(void* ptr, size_t old_size, size_t new_size, size_t alignment);
static inline void  e_aligned_free(void* ptr);

static inline void*
e_align_ptr(void* ptr, size_t alignment)
{
  char* s = (char*)ptr;
  while ((uintptr_t)s % alignment != 0) { s++; }
  return (void*)s;
}

static inline size_t
e_align_size(size_t size, size_t alignment)
{ return (size + alignment - 1) & ~(alignment - 1); }

static inline void*
e_aligned_malloc(size_t size, size_t alignment)
{
  if (alignment < sizeof(void*) || (alignment & (alignment - 1))) return NULL; // must be power of 2

  size_t total    = size + alignment - 1 + sizeof(void*);
  void*  original = malloc(total);
  if (!original) return NULL;

  void* aligned = e_align_ptr((char*)original + sizeof(void*), alignment);

  ((void**)aligned)[-1] = original;

  return aligned;
}

static inline void*
e_aligned_realloc(void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
  if (!ptr) return e_aligned_malloc(new_size, alignment);

  void* new_ptr = e_aligned_malloc(new_size, alignment);
  if (!new_ptr) return NULL;

  size_t copy_size = old_size < new_size ? old_size : new_size;
  memcpy(new_ptr, ptr, copy_size);

  free(((void**)ptr)[-1]);

  return new_ptr;
}

static inline void
e_aligned_free(void* ptr)
{
  if (ptr) free(((void**)ptr)[-1]);
}

#ifdef __cplusplus
}
#endif

#endif // E_STDAFX_H