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

#include "../inc/var.h"

#include "../inc/cast.h"
#include "../inc/pool.h"
#include "../inc/stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
e_var_shallow_cpy(const e_var* var, e_var* dst)
{
  if (!dst || !var) return -1;

  dst->type = var->type;

  /**
   * Variables are stored elsewhere, not in containers.
   * This allows the container to just maintain a pointer to those variables,
   * Which can be shallow copied easily.
   * The reference counter pointer is also copied.
   */
  dst->val = var->val;

  return 0;
}

int
e_var_deep_cpy(const e_var* var, e_var* dst)
{
  if (!dst || !var) return -1;

  *dst      = (e_var){ .type = E_VARTYPE_NULL };
  dst->type = var->type;

  switch (var->type) {
    case E_VARTYPE_NULL: /* only type tag needs to be copied */
    default: break;

    case E_VARTYPE_INT:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_VEC2:
    case E_VARTYPE_VEC3:
    case E_VARTYPE_VEC4:
    case E_VARTYPE_DESCRIPTOR: /* we don't have enough data to deep copy */
    case E_VARTYPE_FLOAT: dst->val = var->val; break;

    case E_VARTYPE_STRING:
      dst->val.s = e_refdobj_pool_acquire(&ge_pool);
      if (!dst->val.s) {
        dst->type = E_VARTYPE_NULL;
        return -1;
      }
      E_VAR_AS_STRING(dst)->s = e_strdup(E_VAR_AS_STRING(var)->s);
      break;
    case E_VARTYPE_LIST: {
      dst->val.list = e_refdobj_pool_acquire(&ge_pool);
      if (!dst->val.list) {
        dst->type = E_VARTYPE_NULL;
        return -1;
      }

      return e_list_init(E_VAR_AS_LIST(var)->vars, E_VAR_AS_LIST(var)->size, E_VAR_AS_LIST(dst));
    }
    case E_VARTYPE_STRUCT: {
      e_struct* s = E_VAR_AS_STRUCT(var);
      if (!s) return -1;

      dst->val.struc = e_refdobj_pool_acquire(&ge_pool);
      if (!dst->val.struc) return -1;

      if (e_struct_init_from(s, E_VAR_AS_STRUCT(dst))) return -1;

      return 0;
    }
    case E_VARTYPE_MAP: {
      dst->val.map = e_refdobj_pool_acquire(&ge_pool);
      /**
       * Create an array of all key value pairs as a map
       * And use it to create the map.
       */
      u32    npairs    = E_VAR_AS_MAP(var)->size;
      e_var* flattened = calloc(npairs * 2ULL, sizeof(e_var));
      memcpy(flattened, E_VAR_AS_MAP(var)->keys, sizeof(e_var) * E_VAR_AS_MAP(var)->size);
      memcpy(flattened + E_VAR_AS_MAP(var)->size, E_VAR_AS_MAP(var)->vals, sizeof(e_var) * E_VAR_AS_MAP(var)->size);
      int e = e_map_init(flattened, E_VAR_AS_MAP(var)->size, E_VAR_AS_MAP(dst));
      free(flattened);
      return e;
    }
    case E_VARTYPE_MAT3: {
      dst->val.mat3 = e_refdobj_pool_acquire(&ge_pool);
      if (dst->val.mat3 && var->val.mat3) { memcpy(dst->val.mat3->data, var->val.mat3->data, sizeof(e_mat3)); }
      return 0;
    }
    case E_VARTYPE_MAT4: {
      dst->val.mat4 = e_refdobj_pool_acquire(&ge_pool);
      if (dst->val.mat4 && var->val.mat4) { memcpy(dst->val.mat4->data, var->val.mat4->data, sizeof(e_mat4)); }
      return 0;
    }
  }

  return 0;
}

i32
e_var_acquire(e_var* v)
{
  int* refc = NULL;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = &v->val.s->refc; break;
    case E_VARTYPE_STRUCT: refc = &v->val.struc->refc; break;
    default: refc = NULL; break;
  }

  if (refc == NULL) return -1;
  // fprintf(stderr, "ACQUIRE %p -> %d\n", (void*)v->val.list, *refc);
  return (*refc)++;
}

void
e_var_release(e_var* v)
{
  return;

  if (!v) return;
  int* refc = NULL;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = &v->val.s->refc; break;
    case E_VARTYPE_STRUCT: refc = &v->val.struc->refc; break;
    default: refc = NULL; break;
  }

  if (refc == NULL) return;

  (*refc)--;
  fprintf(stderr, "RELEASE %p -> %d\n", (void*)v->val.list, *refc);

  if (*refc <= 0) {
    fprintf(stderr, "FREE %p -> %d\n", (void*)v->val.list, *refc);
    e_var_free(v);
  }
}

void
e_var_free(e_var* var)
{
  if (!var) return;

  switch (var->type) {
    default:
    case E_VARTYPE_NULL:
    case E_VARTYPE_INT:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_DESCRIPTOR:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_VEC2:
    case E_VARTYPE_VEC3:
    case E_VARTYPE_VEC4:
    case E_VARTYPE_FLOAT: break;

    case E_VARTYPE_STRING:
      free(E_VAR_AS_STRING(var)->s);
      e_refdobj_pool_return(&ge_pool, var->val.s);
      break;

    case E_VARTYPE_LIST:
      e_list_free(E_VAR_AS_LIST(var));
      e_refdobj_pool_return(&ge_pool, var->val.list);
      break;

    case E_VARTYPE_STRUCT:
      for (u32 i = 0; i < E_VAR_AS_STRUCT(var)->member_count; i++) { e_var_release(&E_VAR_AS_STRUCT(var)->members[i]); }

      free(E_VAR_AS_STRUCT(var)->members);
      free(E_VAR_AS_STRUCT(var)->member_hashes);
      free((void*)E_VAR_AS_STRUCT(var)->member_names);
      e_refdobj_pool_return(&ge_pool, var->val.struc);
      break;

    case E_VARTYPE_MAP:
      e_map_free(E_VAR_AS_MAP(var));
      e_refdobj_pool_return(&ge_pool, var->val.map);
      break;

    case E_VARTYPE_MAT3: {
      e_refdobj_pool_return(&ge_pool, var->val.mat3);
      break;
    }

    case E_VARTYPE_MAT4: {
      e_refdobj_pool_return(&ge_pool, var->val.mat4);
      break;
    }
  }

  /* safety: Zero out the variable */
  *var = E_NULLVAR;
}

void
e_var_print(const struct e_var* v, FILE* f)
{
  switch (v->type) {
    default:
    case E_VARTYPE_NULL: fprintf(f, "null"); break;
    case E_VARTYPE_INT: fprintf(f, "%i", v->val.i); break;
    case E_VARTYPE_DESCRIPTOR: fprintf(f, "%p", v->val.descriptor); break;
    case E_VARTYPE_CHAR: fprintf(f, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: fprintf(f, "%s", (int)v->val.b ? "true" : "false"); break;
    case E_VARTYPE_FLOAT: fprintf(f, "%g", v->val.f); break;
    case E_VARTYPE_STRING: fprintf(f, "%s", E_VAR_AS_STRING(v)->s); break;
    case E_VARTYPE_LIST: {
      fputc('[', f);
      for (u32 i = 0; i < E_VAR_AS_LIST(v)->size; i++) {
        const e_var* elem = &E_VAR_AS_LIST(v)->vars[i];

        if (elem->type == E_VARTYPE_STRING) fputc('"', f);
        e_var_print(elem, f);
        if (elem->type == E_VARTYPE_STRING) fputc('"', f);

        if (i < E_VAR_AS_LIST(v)->size - 1) { fputs(", ", f); }
      }
      fputc(']', f);
      break;
    }
    case E_VARTYPE_STRUCT: {
      fputc('<', f);
      fputs(E_VAR_AS_STRUCT(v)->name, f);
      fputc('>', f);

      fputc('{', f);
      for (u32 i = 0; i < E_VAR_AS_STRUCT(v)->member_count; i++) {
        const e_var* elem        = &E_VAR_AS_STRUCT(v)->members[i];
        const char*  member_name = E_VAR_AS_STRUCT(v)->member_names[i];

        fputs(member_name, f);
        fputs("=", f);

        if (elem->type == E_VARTYPE_STRING) fputc('"', f);
        e_var_print(elem, f);
        if (elem->type == E_VARTYPE_STRING) fputc('"', f);

        if (i < E_VAR_AS_STRUCT(v)->member_count - 1) { fputs(", ", f); }
      }
      fputc('}', f);
      break;
    }
    case E_VARTYPE_MAP: {
      fputs("#{", f);
      for (u32 i = 0; i < E_VAR_AS_MAP(v)->size; i++) {
        const e_var* key = &E_VAR_AS_MAP(v)->keys[i];
        const e_var* val = &E_VAR_AS_MAP(v)->vals[i];

        if (key->type == E_VARTYPE_STRING) fputc('"', f);
        e_var_print(key, f);
        if (key->type == E_VARTYPE_STRING) fputc('"', f);

        fputs(":", f);

        if (val->type == E_VARTYPE_STRING) fputc('"', f);
        e_var_print(val, f);
        if (val->type == E_VARTYPE_STRING) fputc('"', f);

        if (i < E_VAR_AS_MAP(v)->size - 1) { fputs(", ", f); }
      }
      fputc('}', f);
      break;
    }
    case E_VARTYPE_VEC2: {
      fprintf(f, "<x=%g y=%g>", v->val.vec2[0], v->val.vec2[1]);
      break;
    }
    case E_VARTYPE_VEC3: {
      fprintf(f, "<x=%g y=%g z=%g>", v->val.vec3[0], v->val.vec3[1], v->val.vec3[2]);
      break;
    }
    case E_VARTYPE_VEC4: {
      fprintf(f, "<x=%g y=%g z=%g w=%g>", v->val.vec4[0], v->val.vec4[1], v->val.vec4[2], v->val.vec4[3]);
      break;
    }
    case E_VARTYPE_MAT4: {
      fprintf(f, "4x4[ ");
      for (u32 i = 0; i < 4; i++) {
        fprintf(f, "<");
        for (u32 j = 0; j < 4; j++) {
          fprintf(f, "%g", E_VAR_AS_MAT4(v)->m[j][i]);
          if (j != 3) { fprintf(f, ", "); }
        }
        fprintf(f, ">");
        if (i != 3) { fprintf(f, ", "); }
      }
      fprintf(f, " ]");
      break;
    }
    case E_VARTYPE_MAT3: {
      fprintf(f, "3x3[ ");
      for (u32 i = 0; i < 3; i++) {
        fprintf(f, "<");
        for (u32 j = 0; j < 3; j++) {
          fprintf(f, "%g", E_VAR_AS_MAT3(v)->m[j][i]);
          if (j != 2) { fprintf(f, ", "); }
        }
        fprintf(f, ">");
        if (i != 2) { fprintf(f, ", "); }
      }
      fprintf(f, " ]");
      break;
    }
  }
}

void
e_var_to_string(const struct e_var* v, char* buffer, size_t buffer_size)
{
  switch (v->type) {
    default:
    case E_VARTYPE_NULL: strncpy(buffer, "null", buffer_size - 1); break;
    case E_VARTYPE_INT: snprintf(buffer, buffer_size, "%i", v->val.i); break;
    case E_VARTYPE_DESCRIPTOR: snprintf(buffer, buffer_size, "%p", v->val.descriptor); break;
    case E_VARTYPE_CHAR: snprintf(buffer, buffer_size, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: snprintf(buffer, buffer_size, "%s", (int)v->val.b ? "true" : "false"); break;
    case E_VARTYPE_FLOAT: snprintf(buffer, buffer_size, "%g", v->val.f); break;
    case E_VARTYPE_STRING: snprintf(buffer, buffer_size, "%s", E_VAR_AS_STRING(v)->s); break;
    case E_VARTYPE_VEC2: snprintf(buffer, buffer_size, "<x=%g y=%g>", v->val.vec2[0], v->val.vec2[1]); break;
    case E_VARTYPE_VEC3: snprintf(buffer, buffer_size, "<x=%g y=%g z=%g>", v->val.vec3[0], v->val.vec3[1], v->val.vec3[2]); break;
    case E_VARTYPE_VEC4:
      snprintf(buffer, buffer_size, "<x=%g y=%g z=%g w=%g>", v->val.vec4[0], v->val.vec4[1], v->val.vec4[2], v->val.vec4[3]);
      break;
    case E_VARTYPE_LIST: {
      strncpy(buffer, "[", buffer_size - 1);
      buffer[buffer_size - 1] = 0;

      size_t offset = strlen(buffer);

      for (u32 i = 0; i < E_VAR_AS_LIST(v)->size; i++) {
        e_var_to_string(&E_VAR_AS_LIST(v)->vars[i], buffer + offset, buffer_size - offset);

        offset = strlen(buffer);

        if (i < E_VAR_AS_LIST(v)->size - 1) {
          e_strlcat(buffer, ", ", buffer_size);
          offset = strlen(buffer);
        }
      }

      e_strlcat(buffer, "]", buffer_size);

      break;
    }
    case E_VARTYPE_MAP: break;
  }
}

size_t
e_var_to_string_size(const struct e_var* v)
{
  size_t total = 0;
  switch (v->type) {
    default:
    case E_VARTYPE_NULL: total += strlen("null"); break;
    case E_VARTYPE_INT: total += snprintf(NULL, 0, "%i", v->val.i); break;
    case E_VARTYPE_CHAR: total += snprintf(NULL, 0, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: return strlen((int)v->val.b ? "true" : "false"); break;
    case E_VARTYPE_FLOAT: total += snprintf(NULL, 0, "%g", v->val.f); break;
    case E_VARTYPE_VEC2:
    case E_VARTYPE_VEC3:
    case E_VARTYPE_VEC4: {
      e_vec4 vooctor;
      evector_zero_extend(v, vooctor);
      for (u32 i = 0; i < 4; i++) {
        total += snprintf(NULL, 0, "c=%g", vooctor[i]);
        if (i != 3) total += strlen(", ");
      }
      break;
    }

    case E_VARTYPE_MAT3: {
      e_vec4 vooctor;
      evector_zero_extend(v, vooctor);
      for (u32 i = 0; i < 3; i++) {
        for (u32 j = 0; j < 3; j++) { total += snprintf(NULL, 0, "%g", E_VAR_AS_MAT3(v)->m[i][j]); }
      }
      break;
    }
    case E_VARTYPE_STRING: total += strlen(E_VAR_AS_STRING(v)->s); break;
    case E_VARTYPE_LIST: {
      total += 1;
      for (u32 i = 0; i < E_VAR_AS_LIST(v)->size; i++) {
        total += e_var_to_string_size(&E_VAR_AS_LIST(v)->vars[i]);
        if (i < E_VAR_AS_LIST(v)->size - 1) { total += 2; } // ", "
      }
      total += 1;
      break;
    }
    case E_VARTYPE_MAP: break;
  }
  return total;
}

static inline u32
hash_list(const e_var* list, u32 nelems)
{
  const u32 seeds[4] = { 256, 35092, 0xDEADBEEF, 1234567890 };

  u32 hash = nelems;
  for (u32 i = 0; i < nelems; i++) { hash += seeds[i % 4] * e_var_hash(&list[i]); }

  return hash;
}

u32
e_var_hash(const e_var* var)
{
  u32 haha = var->type;
  switch (var->type) {
    case E_VARTYPE_NULL: return haha ^ e_hash(&var->type, sizeof(var->type));
    case E_VARTYPE_INT: return haha ^ e_hash(&var->val.i, sizeof(var->val.i));
    case E_VARTYPE_BOOL: return haha ^ e_hash(&var->val.b, sizeof(bool));
    case E_VARTYPE_CHAR: return haha ^ e_hash(&var->val.c, sizeof(char));
    case E_VARTYPE_FLOAT: return haha ^ e_hash(&var->val.f, sizeof(var->val.f));
    case E_VARTYPE_DESCRIPTOR: return haha ^ e_hash((void*)&var->val.descriptor, sizeof(void*)); // hash the pointer.
    case E_VARTYPE_STRING: return haha ^ e_hash(E_VAR_AS_STRING(var)->s, strlen(E_VAR_AS_STRING(var)->s));
    case E_VARTYPE_VEC2: return haha ^ e_hash(var->val.vec2, sizeof(e_vec2));
    case E_VARTYPE_VEC3: return haha ^ e_hash(var->val.vec3, sizeof(e_vec3));
    case E_VARTYPE_VEC4: return haha ^ e_hash(var->val.vec4, sizeof(e_vec4));
    case E_VARTYPE_MAT3: return haha ^ e_hash(E_VAR_AS_MAT3(var)->m, sizeof(e_mat3));
    case E_VARTYPE_MAT4: return haha ^ e_hash(E_VAR_AS_MAT4(var)->m, sizeof(e_mat4));
    case E_VARTYPE_LIST: return haha ^ hash_list(E_VAR_AS_LIST(var)->vars, E_VAR_AS_LIST(var)->size);
    case E_VARTYPE_MAP: {
      const u32 random_prime = 61;
      return haha
          ^ (hash_list(E_VAR_AS_MAP(var)->keys, E_VAR_AS_MAP(var)->size)
             + (random_prime * hash_list(E_VAR_AS_MAP(var)->vals, E_VAR_AS_MAP(var)->size)));
    }
    case E_VARTYPE_STRUCT: {
      return haha ^ hash_list(E_VAR_AS_STRUCT(var)->members, E_VAR_AS_STRUCT(var)->member_count);
    }
    default: return e_hash(&var->val, sizeof(var->val));
  }
}

static inline bool
is_integral(e_vartype type)
{
  switch (type) {
    case E_VARTYPE_INT:
    case E_VARTYPE_BOOL:
    case E_VARTYPE_CHAR:
    case E_VARTYPE_FLOAT: return true;
    default: break;
  }
  return false;
}

bool
e_var_equal(const e_var* a, const e_var* b)
{
  if (a->type == E_VARTYPE_NULL || b->type == E_VARTYPE_NULL) { return a->type == b->type; }

  switch (a->type) {
    default: {
      printf("%i\n", a->type);
      assert(0);
    }

    case E_VARTYPE_INT: return is_integral(b->type) && (a->val.i == e_cast_to_int(b));
    case E_VARTYPE_BOOL: return is_integral(b->type) && (a->val.b == e_cast_to_bool(b));
    case E_VARTYPE_CHAR: return is_integral(b->type) && (a->val.c == e_cast_to_char(b));
    case E_VARTYPE_FLOAT: return is_integral(b->type) && (a->val.f == e_cast_to_float(b));
    case E_VARTYPE_DESCRIPTOR: return (a->type == b->type) && a->val.descriptor == b->val.descriptor;
    case E_VARTYPE_STRING: return a->type == b->type ? strcmp(E_VAR_AS_STRING(a)->s, E_VAR_AS_STRING(b)->s) == 0 : false;

    case E_VARTYPE_VEC2: {
      return a->type == b->type ? (a->val.vec2[0] == b->val.vec2[0] && a->val.vec2[1] == b->val.vec2[1]) : false;
    }
    case E_VARTYPE_VEC3: {
      return a->type == b->type ? (a->val.vec3[0] == b->val.vec3[0] && a->val.vec3[1] == b->val.vec3[1] && a->val.vec3[2] == b->val.vec3[2]) : false;
    }
    case E_VARTYPE_VEC4: {
      return a->type == b->type ? (a->val.vec4[0] == b->val.vec4[0] && a->val.vec4[1] == b->val.vec4[1] && a->val.vec4[2] == b->val.vec4[2]
                                   && a->val.vec4[3] == b->val.vec4[3])
                                : false;
    }

    case E_VARTYPE_MAT3: {
      if (b->type != a->type) return false;
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
          if (E_VAR_AS_MAT3(a)->m[j][i] != E_VAR_AS_MAT3(b)->m[j][i]) { return false; }
        }
      }
    }
    case E_VARTYPE_MAT4: {
      if (b->type != a->type) return false;
      for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
          if (E_VAR_AS_MAT4(a)->m[j][i] != E_VAR_AS_MAT4(b)->m[j][i]) { return false; }
        }
      }
    }

    case E_VARTYPE_LIST:
      if (b->type != E_VARTYPE_LIST) return false;
      if (E_VAR_AS_LIST(a)->size != E_VAR_AS_LIST(b)->size) return false;
      for (size_t i = 0; i < E_VAR_AS_LIST(a)->size; i++) {
        if (!e_var_equal(&E_VAR_AS_LIST(a)->vars[i], &E_VAR_AS_LIST(b)->vars[i])) return false;
      }
      return true;
    case E_VARTYPE_STRUCT:
      if (b->type != E_VARTYPE_STRUCT) return false;
      if (E_VAR_AS_STRUCT(a)->member_count != E_VAR_AS_STRUCT(b)->member_count) return false;
      for (size_t i = 0; i < E_VAR_AS_STRUCT(a)->member_count; i++) {
        if (E_VAR_AS_STRUCT(a)->member_hashes[i] != E_VAR_AS_STRUCT(b)->member_hashes[i]) return false;
        if (!e_var_equal(&E_VAR_AS_STRUCT(a)->members[i], &E_VAR_AS_STRUCT(b)->members[i])) return false;
      }
      return true;
    case E_VARTYPE_MAP:
      if (b->type != E_VARTYPE_MAP) return false;

      if (E_VAR_AS_MAP(a)->size != E_VAR_AS_MAP(b)->size) return false;
      for (u32 i = 0; i < E_VAR_AS_MAP(a)->size; i++) {
        if (!e_var_equal(&E_VAR_AS_MAP(a)->keys[i], &E_VAR_AS_MAP(b)->keys[i])) return false;
        if (!e_var_equal(&E_VAR_AS_MAP(a)->vals[i], &E_VAR_AS_MAP(b)->vals[i])) return false;
      }
      return true;
  }
  return false;
}

e_var
e_make_var_from_string(char* s)
{
  if (!s) { return (e_var){ .type = E_VARTYPE_NULL }; }
  e_var ret                = { .type = E_VARTYPE_STRING };
  ret.val.s                = e_refdobj_pool_acquire(&ge_pool);
  E_VAR_AS_STRING(&ret)->s = s; // we just allocated
  return ret;
}

struct e_var*
e_struct_get_member(u32 hash, const e_struct* s)
{
  if (!s) return NULL;
  for (u32 i = 0; i < s->member_count; i++) {
    if (s->member_hashes[i] == hash) { return &s->members[i]; }
  }
  return NULL;
}

int
e_var_index(const e_var* base, const e_var* index, e_var* result)
{
  if (result) *result = E_NULLVAR;

  switch (base->type) {
    case E_VARTYPE_LIST: {
      e_list* list = E_VAR_AS_LIST(base);
      int     idx  = e_cast_to_int(index);

      e_var* ptr = e_list_index(list, idx);
      if (!ptr) return -1;

      if (result) e_var_shallow_cpy(ptr, result);
      return 0;
    }
    case E_VARTYPE_MAP: {
      e_map* map    = E_VAR_AS_MAP(base);
      e_var* search = e_map_find(map, index);

      if (result) {
        if (search == NULL) {
          *result = E_NULLVAR;
        } else {
          e_var_shallow_cpy(search, result);
        }
      }
      return 0;
    }
    case E_VARTYPE_STRING: {
      const char* s = E_VAR_AS_STRING(base)->s;
      int         i = e_cast_to_int(index);

      char ch = 0;
      if (i < strlen(s) && i >= 0) { ch = s[i]; }

      if (result) *result = e_var_from_char(ch);
      return 0;
    }

    case E_VARTYPE_VEC2: {
      int idx = e_cast_to_int(index);
      if (idx < 2 && result) *result = e_var_from_float(base->val.vec2[idx]);
      return 0;
    }
    case E_VARTYPE_VEC3: {
      int idx = e_cast_to_int(index);
      if (idx < 3 && result) *result = e_var_from_float(base->val.vec3[idx]);
      return 0;
    }
    case E_VARTYPE_VEC4: {
      int idx = e_cast_to_int(index);
      if (idx < 4 && result) *result = e_var_from_float(base->val.vec4[idx]);
      return 0;
    }

    default: fprintf(stderr, "Attempt to index into invalid base: type=%s\n", e_var_type_to_string(base->type)); return -1;
  }
}

int
e_var_index_assign(e_var* base, const e_var* index, const e_var* value)
{
  switch (base->type) {
    case E_VARTYPE_LIST: {
      e_list* list = E_VAR_AS_LIST(base);
      int     idx  = e_cast_to_int(index);

      e_var* ptr = e_list_index(list, idx);
      if (!ptr) {
        fprintf(stderr, "Out of bounds write to list (idx=%i, list->size=%i)\n", idx, list->size);
        return -1;
      }

      if (ptr && value) {
        e_var_release(ptr);
        e_var_shallow_cpy(value, ptr);
        e_var_acquire(ptr);
      }
      return 0;
    }
    case E_VARTYPE_MAP: {
      e_map* map    = E_VAR_AS_MAP(base);
      e_var* search = e_map_find_or_insert(map, index);

      if (search && value) {
        e_var_release(search);
        e_var_shallow_cpy(value, search);
        e_var_acquire(search);
      }
      return 0;
    }
    case E_VARTYPE_VEC2: {
      int    idx    = e_cast_to_int(index);
      double set_to = e_cast_to_float(value);
      if (idx < 2) base->val.vec2[idx] = set_to;
      return 0;
    }
    case E_VARTYPE_VEC3: {
      int    idx    = e_cast_to_int(index);
      double set_to = e_cast_to_float(value);
      if (idx < 3) base->val.vec3[idx] = set_to;
      return 0;
    }
    case E_VARTYPE_VEC4: {
      int    idx    = e_cast_to_int(index);
      double set_to = e_cast_to_float(value);
      if (idx < 4) base->val.vec4[idx] = set_to;
      return 0;
    }
    case E_VARTYPE_MAT3: {
      if (value->type != E_VARTYPE_VEC3) {
        fprintf(stderr, "Attempt to write to matrix (3x3) with non vec3 value\n");
        return -1;
      }

      e_mat3*       mat = E_VAR_AS_MAT3(base);
      const e_vec3* vec = &value->val.vec3;
      int           idx = e_cast_to_int(index);

      memcpy(mat->m[idx], *vec, sizeof(e_vec3));
      return 0;
    }
    case E_VARTYPE_MAT4: {
      if (value->type != E_VARTYPE_VEC4) {
        fprintf(stderr, "Attempt to write to matrix (4x4) with non vec4 value\n");
        return -1;
      }

      e_mat4*       mat = E_VAR_AS_MAT4(base);
      const e_vec4* vec = &value->val.vec4;
      int           idx = e_cast_to_int(index);

      memcpy(mat->m[idx], *vec, sizeof(e_vec4));
      return 0;
    }
    default: fprintf(stderr, "Attempt to index assign to invalid base: type=%s\n", e_var_type_to_string(base->type)); return -1;
  }
}
