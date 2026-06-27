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

#include "../inc/kit.var.h"

#include "../inc/kit.cast.h"
#include "../inc/kit.pool.h"
#include "../inc/kit.stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
kit_var_shallow_cpy(const kit_var* var, kit_var* dst)
{
  if (!dst || !var) return -1;

  memmove(dst, var, sizeof(kit_var));
  return 0;

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
kit_var_deep_cpy(kit_refdobj_pool* object_pool, const kit_var* var, kit_var* dst)
{
  if (!dst || !var) return -1;

  *dst      = (kit_var){ .type = KIT_VARTYPE_NULL };
  dst->type = var->type;

  switch (var->type) {
    case KIT_VARTYPE_NULL: /* only type tag needs to be copied */
    default: break;

    case KIT_VARTYPE_INT:
    case KIT_VARTYPE_BOOL:
    case KIT_VARTYPE_CHAR:
    case KIT_VARTYPE_FUNCTION:
    case KIT_VARTYPE_VEC2:
    case KIT_VARTYPE_VEC3:
    case KIT_VARTYPE_VEC4:
    case KIT_VARTYPE_DESCRIPTOR: /* we don't have enough data to deep copy */
    case KIT_VARTYPE_FLOAT: dst->val = var->val; break;

    case KIT_VARTYPE_STRING:
      dst->val.s = kit_refdobj_pool_acquire(object_pool);
      if (!dst->val.s) {
        dst->type = KIT_VARTYPE_NULL;
        return -1;
      }
      KIT_VAR_AS_STRING(dst)->s = kit_strdup(KIT_VAR_AS_STRING(var)->s);
      break;
    case KIT_VARTYPE_LIST: {
      dst->val.list = kit_refdobj_pool_acquire(object_pool);
      if (!dst->val.list) {
        dst->type = KIT_VARTYPE_NULL;
        return -1;
      }

      return kit_list_init(object_pool, KIT_VAR_AS_LIST(var)->vars, KIT_VAR_AS_LIST(var)->size, KIT_VAR_AS_LIST(dst));
    }
    case KIT_VARTYPE_STRUCT: {
      kit_struct* s = KIT_VAR_AS_STRUCT(var);
      if (!s) return -1;

      dst->val.struc = kit_refdobj_pool_acquire(object_pool);
      if (!dst->val.struc) return -1;

      if (kit_struct_init_from(object_pool, s, KIT_VAR_AS_STRUCT(dst))) return -1;

      return 0;
    }
    case KIT_VARTYPE_MAP: {
      dst->val.map = kit_refdobj_pool_acquire(object_pool);
      /**
       * Create an array of all key value pairs as a map
       * And use it to create the map.
       */
      u32      npairs    = KIT_VAR_AS_MAP(var)->size;
      kit_var* flattened = calloc(npairs * 2ULL, sizeof(kit_var));

      memcpy(flattened, KIT_VAR_AS_MAP(var)->keys, sizeof(kit_var) * KIT_VAR_AS_MAP(var)->size);
      memcpy(flattened + KIT_VAR_AS_MAP(var)->size, KIT_VAR_AS_MAP(var)->vals, sizeof(kit_var) * KIT_VAR_AS_MAP(var)->size);

      int e = kit_map_init(object_pool, flattened, KIT_VAR_AS_MAP(var)->size, KIT_VAR_AS_MAP(dst));

      free(flattened);
      return e;
    }
    case KIT_VARTYPE_MAT3: {
      dst->val.mat3 = kit_refdobj_pool_acquire(object_pool);
      if (dst->val.mat3 && var->val.mat3) { memcpy(dst->val.mat3->data, var->val.mat3->data, sizeof(kit_mat3)); }
      return 0;
    }
    case KIT_VARTYPE_MAT4: {
      dst->val.mat4 = kit_refdobj_pool_acquire(object_pool);
      if (dst->val.mat4 && var->val.mat4) { memcpy(dst->val.mat4->data, var->val.mat4->data, sizeof(kit_mat4)); }
      return 0;
    }
  }

  return 0;
}

i32
kit_var_acquire(kit_refdobj_pool* object_pool, kit_var* v)
{
  int* refc = NULL;
  switch (v->type) {
    case KIT_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case KIT_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case KIT_VARTYPE_STRING: refc = &v->val.s->refc; break;
    case KIT_VARTYPE_STRUCT: refc = &v->val.struc->refc; break;
    default: refc = NULL; break;
  }

  if (refc == NULL) return -1;
  // fprintf(stderr, "ACQUIRE %p -> %d\n", (void*)v->val.list, *refc);
  return (*refc)++;
}

void
kit_var_release(kit_refdobj_pool* object_pool, kit_var* v)
{
  if (!v) return;
  int* refc = NULL;
  switch (v->type) {
    case KIT_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case KIT_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case KIT_VARTYPE_STRING: refc = &v->val.s->refc; break;
    case KIT_VARTYPE_STRUCT: refc = &v->val.struc->refc; break;
    default: refc = NULL; break;
  }

  if (refc == NULL) return;

  (*refc)--;

  if (*refc <= 0) { kit_var_free(object_pool, v); }
}

void
kit_var_free(kit_refdobj_pool* object_pool, kit_var* var)
{
  if (!var) return;

  switch (var->type) {
    default:
    case KIT_VARTYPE_FUNCTION:
    case KIT_VARTYPE_NULL:
    case KIT_VARTYPE_INT:
    case KIT_VARTYPE_CHAR:
    case KIT_VARTYPE_DESCRIPTOR:
    case KIT_VARTYPE_BOOL:
    case KIT_VARTYPE_VEC2:
    case KIT_VARTYPE_VEC3:
    case KIT_VARTYPE_VEC4:
    case KIT_VARTYPE_FLOAT: break;

    case KIT_VARTYPE_STRING: {
      char* s = KIT_VAR_AS_STRING(var)->s;
      free(s);
      kit_refdobj_pool_return(object_pool, var->val.s);
      break;
    }

    case KIT_VARTYPE_LIST:
      kit_list_free(object_pool, KIT_VAR_AS_LIST(var));
      kit_refdobj_pool_return(object_pool, var->val.list);
      break;

    case KIT_VARTYPE_STRUCT:
      for (u32 i = 0; i < KIT_VAR_AS_STRUCT(var)->member_count; i++) { kit_var_release(object_pool, &KIT_VAR_AS_STRUCT(var)->members[i]); }

      free(KIT_VAR_AS_STRUCT(var)->members);
      free(KIT_VAR_AS_STRUCT(var)->member_hashes);
      free((void*)KIT_VAR_AS_STRUCT(var)->member_names);
      kit_refdobj_pool_return(object_pool, var->val.struc);
      break;

    case KIT_VARTYPE_MAP:
      kit_map_free(object_pool, KIT_VAR_AS_MAP(var));
      kit_refdobj_pool_return(object_pool, var->val.map);
      break;

    case KIT_VARTYPE_MAT3: {
      kit_refdobj_pool_return(object_pool, var->val.mat3);
      break;
    }

    case KIT_VARTYPE_MAT4: {
      kit_refdobj_pool_return(object_pool, var->val.mat4);
      break;
    }
  }

  /* safety: Zero out the variable */
  *var = KIT_NULLVAR;
}

void
kit_var_print(const struct kit_var* v, FILE* f)
{
  switch (v->type) {
    default:
    case KIT_VARTYPE_NULL: fprintf(f, "null"); break;
    case KIT_VARTYPE_FUNCTION: fprintf(f, "%u", v->val.func.hash); break;
    case KIT_VARTYPE_INT: fprintf(f, "%i", v->val.i); break;
    case KIT_VARTYPE_DESCRIPTOR: fprintf(f, "%p", v->val.descriptor); break;
    case KIT_VARTYPE_CHAR: fprintf(f, "%c", v->val.c); break;
    case KIT_VARTYPE_BOOL: fprintf(f, "%s", (int)v->val.b ? "true" : "false"); break;
    case KIT_VARTYPE_FLOAT: fprintf(f, "%g", v->val.f); break;
    case KIT_VARTYPE_STRING: fprintf(f, "%s", KIT_VAR_AS_STRING(v)->s); break;
    case KIT_VARTYPE_LIST: {
      fputc('[', f);
      for (u32 i = 0; i < KIT_VAR_AS_LIST(v)->size; i++) {
        const kit_var* elem = &KIT_VAR_AS_LIST(v)->vars[i];

        if (elem->type == KIT_VARTYPE_STRING) fputc('"', f);
        kit_var_print(elem, f);
        if (elem->type == KIT_VARTYPE_STRING) fputc('"', f);

        if (i < KIT_VAR_AS_LIST(v)->size - 1) { fputs(", ", f); }
      }
      fputc(']', f);
      break;
    }
    case KIT_VARTYPE_STRUCT: {
      fputc('<', f);
      fputs(KIT_VAR_AS_STRUCT(v)->name, f);
      fputc('>', f);

      fputc('{', f);
      for (u32 i = 0; i < KIT_VAR_AS_STRUCT(v)->member_count; i++) {
        const kit_var* elem        = &KIT_VAR_AS_STRUCT(v)->members[i];
        const char*    member_name = KIT_VAR_AS_STRUCT(v)->member_names[i];

        fputs(member_name, f);
        fputs("=", f);

        if (elem->type == KIT_VARTYPE_STRING) fputc('"', f);
        kit_var_print(elem, f);
        if (elem->type == KIT_VARTYPE_STRING) fputc('"', f);

        if (i < KIT_VAR_AS_STRUCT(v)->member_count - 1) { fputs(", ", f); }
      }
      fputc('}', f);
      break;
    }
    case KIT_VARTYPE_MAP: {
      fputs("#{", f);
      for (u32 i = 0; i < KIT_VAR_AS_MAP(v)->size; i++) {
        const kit_var* key = &KIT_VAR_AS_MAP(v)->keys[i];
        const kit_var* val = &KIT_VAR_AS_MAP(v)->vals[i];

        if (key->type == KIT_VARTYPE_STRING) fputc('"', f);
        kit_var_print(key, f);
        if (key->type == KIT_VARTYPE_STRING) fputc('"', f);

        fputs(":", f);

        if (val->type == KIT_VARTYPE_STRING) fputc('"', f);
        kit_var_print(val, f);
        if (val->type == KIT_VARTYPE_STRING) fputc('"', f);

        if (i < KIT_VAR_AS_MAP(v)->size - 1) { fputs(", ", f); }
      }
      fputc('}', f);
      break;
    }
    case KIT_VARTYPE_VEC2: {
      fprintf(f, "<x=%g y=%g>", v->val.vec2[0], v->val.vec2[1]);
      break;
    }
    case KIT_VARTYPE_VEC3: {
      fprintf(f, "<x=%g y=%g z=%g>", v->val.vec3[0], v->val.vec3[1], v->val.vec3[2]);
      break;
    }
    case KIT_VARTYPE_VEC4: {
      fprintf(f, "<x=%g y=%g z=%g w=%g>", v->val.vec4[0], v->val.vec4[1], v->val.vec4[2], v->val.vec4[3]);
      break;
    }
    case KIT_VARTYPE_MAT4: {
      fprintf(f, "4x4[ ");
      for (u32 i = 0; i < 4; i++) {
        fprintf(f, "<");
        for (u32 j = 0; j < 4; j++) {
          fprintf(f, "%g", KIT_VAR_AS_MAT4(v)->m[j][i]);
          if (j != 3) { fprintf(f, ", "); }
        }
        fprintf(f, ">");
        if (i != 3) { fprintf(f, ", "); }
      }
      fprintf(f, " ]");
      break;
    }
    case KIT_VARTYPE_MAT3: {
      fprintf(f, "3x3[ ");
      for (u32 i = 0; i < 3; i++) {
        fprintf(f, "<");
        for (u32 j = 0; j < 3; j++) {
          fprintf(f, "%g", KIT_VAR_AS_MAT3(v)->m[j][i]);
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
kit_var_to_string(const struct kit_var* v, char* buffer, size_t buffer_size)
{
  switch (v->type) {
    default:
    case KIT_VARTYPE_NULL: strncpy(buffer, "null", buffer_size - 1); break;
    case KIT_VARTYPE_INT: snprintf(buffer, buffer_size, "%i", v->val.i); break;
    case KIT_VARTYPE_DESCRIPTOR: snprintf(buffer, buffer_size, "%p", v->val.descriptor); break;
    case KIT_VARTYPE_CHAR: snprintf(buffer, buffer_size, "%c", v->val.c); break;
    case KIT_VARTYPE_BOOL: snprintf(buffer, buffer_size, "%s", (int)v->val.b ? "true" : "false"); break;
    case KIT_VARTYPE_FLOAT: snprintf(buffer, buffer_size, "%g", v->val.f); break;
    case KIT_VARTYPE_STRING: snprintf(buffer, buffer_size, "%s", KIT_VAR_AS_STRING(v)->s); break;
    case KIT_VARTYPE_VEC2: snprintf(buffer, buffer_size, "<x=%g y=%g>", v->val.vec2[0], v->val.vec2[1]); break;
    case KIT_VARTYPE_VEC3: snprintf(buffer, buffer_size, "<x=%g y=%g z=%g>", v->val.vec3[0], v->val.vec3[1], v->val.vec3[2]); break;
    case KIT_VARTYPE_VEC4:
      snprintf(buffer, buffer_size, "<x=%g y=%g z=%g w=%g>", v->val.vec4[0], v->val.vec4[1], v->val.vec4[2], v->val.vec4[3]);
      break;
    case KIT_VARTYPE_LIST: {
      strncpy(buffer, "[", buffer_size - 1);
      buffer[buffer_size - 1] = 0;

      size_t offset = strlen(buffer);

      for (u32 i = 0; i < KIT_VAR_AS_LIST(v)->size; i++) {
        kit_var_to_string(&KIT_VAR_AS_LIST(v)->vars[i], buffer + offset, buffer_size - offset);

        offset = strlen(buffer);

        if (i < KIT_VAR_AS_LIST(v)->size - 1) {
          kit_strlcat(buffer, ", ", buffer_size);
          offset = strlen(buffer);
        }
      }

      kit_strlcat(buffer, "]", buffer_size);

      break;
    }
    case KIT_VARTYPE_MAP: break;
  }
}

size_t
kit_var_to_string_size(const struct kit_var* v)
{
  size_t total = 0;
  switch (v->type) {
    default:
    case KIT_VARTYPE_NULL: total += strlen("null"); break;
    case KIT_VARTYPE_INT: total += snprintf(NULL, 0, "%i", v->val.i); break;
    case KIT_VARTYPE_CHAR: total += snprintf(NULL, 0, "%c", v->val.c); break;
    case KIT_VARTYPE_BOOL: total += strlen((int)v->val.b ? "true" : "false"); break;
    case KIT_VARTYPE_FLOAT: total += snprintf(NULL, 0, "%g", v->val.f); break;
    case KIT_VARTYPE_VEC2:
    case KIT_VARTYPE_VEC3:
    case KIT_VARTYPE_VEC4: {
      kit_vec4 vooctor;
      evector_zero_extend(v, vooctor);
      for (u32 i = 0; i < 4; i++) {
        total += snprintf(NULL, 0, "c=%g", vooctor[i]);
        if (i != 3) total += strlen(", ");
      }
      break;
    }

    case KIT_VARTYPE_MAT3: {
      kit_vec4 vooctor;
      evector_zero_extend(v, vooctor);
      for (u32 i = 0; i < 3; i++) {
        for (u32 j = 0; j < 3; j++) { total += snprintf(NULL, 0, "%g", KIT_VAR_AS_MAT3(v)->m[i][j]); }
      }
      break;
    }
    case KIT_VARTYPE_STRING: total += strlen(KIT_VAR_AS_STRING(v)->s); break;
    case KIT_VARTYPE_LIST: {
      total += 1;
      for (u32 i = 0; i < KIT_VAR_AS_LIST(v)->size; i++) {
        total += kit_var_to_string_size(&KIT_VAR_AS_LIST(v)->vars[i]);
        if (i < KIT_VAR_AS_LIST(v)->size - 1) { total += 2; } // ", "
      }
      total += 1;
      break;
    }
    case KIT_VARTYPE_MAP: break;
  }
  return total;
}

static inline u32
hash_list(const kit_var* list, u32 nelems)
{
  /**
   * This is needed so that two lists with the
   * same members but in different order compute
   * to two different hashes.
   */
  const u32 seeds[4] = { 256, 35092, 0xDEADBEEF, 1234567890 };

  u32 hash = nelems;
  for (u32 i = 0; i < nelems; i++) { hash += seeds[i % 4] * kit_var_hash(&list[i]); }

  return hash;
}

u32
kit_var_hash(const kit_var* var)
{
  u32 haha = var->type; /* Include the variable's type in the hash */
  switch (var->type) {
    case KIT_VARTYPE_NULL: return haha ^ kit_hash(&var->type, sizeof(var->type));
    case KIT_VARTYPE_INT: return haha ^ kit_hash(&var->val.i, sizeof(var->val.i));
    case KIT_VARTYPE_FUNCTION: return haha ^ kit_hash(&var->val.func.hash, sizeof(var->val.func.hash));
    case KIT_VARTYPE_BOOL: return haha ^ kit_hash(&var->val.b, sizeof(bool));
    case KIT_VARTYPE_CHAR: return haha ^ kit_hash(&var->val.c, sizeof(char));
    case KIT_VARTYPE_FLOAT: return haha ^ kit_hash(&var->val.f, sizeof(var->val.f));
    case KIT_VARTYPE_DESCRIPTOR: return haha ^ kit_hash((void*)&var->val.descriptor, sizeof(void*)); // hash the pointer.
    case KIT_VARTYPE_STRING: return haha ^ kit_hash(KIT_VAR_AS_STRING(var)->s, strlen(KIT_VAR_AS_STRING(var)->s));
    case KIT_VARTYPE_VEC2: return haha ^ kit_hash(var->val.vec2, sizeof(kit_vec2));
    case KIT_VARTYPE_VEC3: return haha ^ kit_hash(var->val.vec3, sizeof(kit_vec3));
    case KIT_VARTYPE_VEC4: return haha ^ kit_hash(var->val.vec4, sizeof(kit_vec4));
    case KIT_VARTYPE_MAT3: return haha ^ kit_hash(KIT_VAR_AS_MAT3(var)->m, sizeof(kit_mat3));
    case KIT_VARTYPE_MAT4: return haha ^ kit_hash(KIT_VAR_AS_MAT4(var)->m, sizeof(kit_mat4));
    case KIT_VARTYPE_LIST: return haha ^ hash_list(KIT_VAR_AS_LIST(var)->vars, KIT_VAR_AS_LIST(var)->size);
    case KIT_VARTYPE_MAP: {
      const u32 random_prime = 61;
      return haha
          ^ (hash_list(KIT_VAR_AS_MAP(var)->keys, KIT_VAR_AS_MAP(var)->size)
             + (random_prime * hash_list(KIT_VAR_AS_MAP(var)->vals, KIT_VAR_AS_MAP(var)->size)));
    }
    case KIT_VARTYPE_STRUCT: {
      return haha ^ hash_list(KIT_VAR_AS_STRUCT(var)->members, KIT_VAR_AS_STRUCT(var)->member_count);
    }
    default: return kit_hash(&var->val, sizeof(var->val));
  }
}

static inline bool
is_integral(kit_var_type type)
{
  switch (type) {
    case KIT_VARTYPE_INT:
    case KIT_VARTYPE_BOOL:
    case KIT_VARTYPE_CHAR:
    case KIT_VARTYPE_FLOAT: return true;
    default: break;
  }
  return false;
}

bool
kit_var_equal(const kit_var* a, const kit_var* b)
{
  if (a->type == KIT_VARTYPE_NULL || b->type == KIT_VARTYPE_NULL) { return a->type == b->type; }

  switch (a->type) {
    default: {
      /* SHOULD NEVER REACH HERE!! */
      return false;
    }

    case KIT_VARTYPE_INT: return is_integral(b->type) && (a->val.i == kit_cast_to_int(b));
    case KIT_VARTYPE_FUNCTION: return b->type == KIT_VARTYPE_FUNCTION && (a->val.func.hash == b->val.func.hash);
    case KIT_VARTYPE_BOOL: return is_integral(b->type) && (a->val.b == kit_cast_to_bool(b));
    case KIT_VARTYPE_CHAR: return is_integral(b->type) && (a->val.c == kit_cast_to_char(b));
    case KIT_VARTYPE_FLOAT: return is_integral(b->type) && (a->val.f == kit_cast_to_float(b));
    case KIT_VARTYPE_DESCRIPTOR: return (a->type == b->type) && a->val.descriptor == b->val.descriptor;
    case KIT_VARTYPE_STRING: return a->type == b->type ? strcmp(KIT_VAR_AS_STRING(a)->s, KIT_VAR_AS_STRING(b)->s) == 0 : false;

    case KIT_VARTYPE_VEC2: {
      return a->type == b->type ? (a->val.vec2[0] == b->val.vec2[0] && a->val.vec2[1] == b->val.vec2[1]) : false;
    }
    case KIT_VARTYPE_VEC3: {
      return a->type == b->type ? (a->val.vec3[0] == b->val.vec3[0] && a->val.vec3[1] == b->val.vec3[1] && a->val.vec3[2] == b->val.vec3[2]) : false;
    }
    case KIT_VARTYPE_VEC4: {
      return a->type == b->type ? (a->val.vec4[0] == b->val.vec4[0] && a->val.vec4[1] == b->val.vec4[1] && a->val.vec4[2] == b->val.vec4[2]
                                   && a->val.vec4[3] == b->val.vec4[3])
                                : false;
    }

    case KIT_VARTYPE_MAT3: {
      if (b->type != a->type) return false;
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
          if (KIT_VAR_AS_MAT3(a)->m[j][i] != KIT_VAR_AS_MAT3(b)->m[j][i]) { return false; }
        }
      }
      return true;
    }
    case KIT_VARTYPE_MAT4: {
      if (b->type != a->type) return false;
      for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
          if (KIT_VAR_AS_MAT4(a)->m[j][i] != KIT_VAR_AS_MAT4(b)->m[j][i]) { return false; }
        }
      }
      return true;
    }

    case KIT_VARTYPE_LIST:
      if (b->type != KIT_VARTYPE_LIST) return false;
      if (KIT_VAR_AS_LIST(a)->size != KIT_VAR_AS_LIST(b)->size) return false;
      for (size_t i = 0; i < KIT_VAR_AS_LIST(a)->size; i++) {
        if (!kit_var_equal(&KIT_VAR_AS_LIST(a)->vars[i], &KIT_VAR_AS_LIST(b)->vars[i])) return false;
      }
      return true;

    case KIT_VARTYPE_STRUCT:
      if (b->type != KIT_VARTYPE_STRUCT) return false;
      if (KIT_VAR_AS_STRUCT(a)->member_count != KIT_VAR_AS_STRUCT(b)->member_count) return false;
      for (size_t i = 0; i < KIT_VAR_AS_STRUCT(a)->member_count; i++) {
        if (KIT_VAR_AS_STRUCT(a)->member_hashes[i] != KIT_VAR_AS_STRUCT(b)->member_hashes[i]) return false;
        if (!kit_var_equal(&KIT_VAR_AS_STRUCT(a)->members[i], &KIT_VAR_AS_STRUCT(b)->members[i])) return false;
      }
      return true;

    case KIT_VARTYPE_MAP:
      if (b->type != KIT_VARTYPE_MAP) return false;

      if (KIT_VAR_AS_MAP(a)->size != KIT_VAR_AS_MAP(b)->size) return false;
      for (u32 i = 0; i < KIT_VAR_AS_MAP(a)->size; i++) {
        if (!kit_var_equal(&KIT_VAR_AS_MAP(a)->keys[i], &KIT_VAR_AS_MAP(b)->keys[i])) return false;
        if (!kit_var_equal(&KIT_VAR_AS_MAP(a)->vals[i], &KIT_VAR_AS_MAP(b)->vals[i])) return false;
      }
      return true;
  }
  return false;
}

kit_var
kit_make_var_from_string(kit_refdobj_pool* object_pool, char* s)
{
  if (!s) { return (kit_var){ .type = KIT_VARTYPE_NULL }; }
  kit_var ret                = { .type = KIT_VARTYPE_STRING };
  ret.val.s                  = kit_refdobj_pool_acquire(object_pool);
  KIT_VAR_AS_STRING(&ret)->s = s; // we just allocated
  return ret;
}

struct kit_var*
kit_struct_get_member(u32 hash, const kit_struct* s)
{
  if (!s) return NULL;
  for (u32 i = 0; i < s->member_count; i++) {
    if (s->member_hashes[i] == hash) { return &s->members[i]; }
  }
  return NULL;
}

int
kit_var_index(kit_refdobj_pool* object_pool, const kit_var* base, const kit_var* index, kit_var* result)
{
  if (result) *result = KIT_NULLVAR;

  switch (base->type) {
    case KIT_VARTYPE_LIST: {
      kit_list* list = KIT_VAR_AS_LIST(base);
      int       idx  = kit_cast_to_int(index);

      kit_var* ptr = kit_list_index(object_pool, list, idx);
      if (!ptr) return -1;

      if (result) kit_var_shallow_cpy(ptr, result);
      return 0;
    }
    case KIT_VARTYPE_MAP: {
      kit_map* map    = KIT_VAR_AS_MAP(base);
      kit_var* search = kit_map_find(map, index);

      if (result) {
        if (search == NULL) {
          *result = KIT_NULLVAR;
        } else {
          kit_var_shallow_cpy(search, result);
        }
      }
      return 0;
    }
    case KIT_VARTYPE_STRING: {
      const char* s = KIT_VAR_AS_STRING(base)->s;
      int         i = kit_cast_to_int(index);

      char ch = 0;
      if (i < strlen(s) && i >= 0) { ch = s[i]; }

      if (result) *result = kit_var_from_char(ch);
      return 0;
    }

    case KIT_VARTYPE_VEC2: {
      int idx = kit_cast_to_int(index);
      if (idx < 2 && result) *result = kit_var_from_float(base->val.vec2[idx]);
      return 0;
    }
    case KIT_VARTYPE_VEC3: {
      int idx = kit_cast_to_int(index);
      if (idx < 3 && result) *result = kit_var_from_float(base->val.vec3[idx]);
      return 0;
    }
    case KIT_VARTYPE_VEC4: {
      int idx = kit_cast_to_int(index);
      if (idx < 4 && result) *result = kit_var_from_float(base->val.vec4[idx]);
      return 0;
    }

    default: fprintf(stderr, "Attempt to index into invalid base: type=%s\n", kit_var_type_to_string(base->type)); return -1;
  }
}

int
kit_var_index_assign(kit_refdobj_pool* object_pool, kit_var* base, const kit_var* index, const kit_var* value)
{
  switch (base->type) {
    case KIT_VARTYPE_LIST: {
      kit_list* list = KIT_VAR_AS_LIST(base);
      int       idx  = kit_cast_to_int(index);

      kit_var* ptr = kit_list_index(object_pool, list, idx);
      if (!ptr) {
        fprintf(stderr, "Out of bounds write to list (idx=%i, list->size=%i)\n", idx, list->size);
        return -1;
      }

      if (ptr && value) {
        kit_var_release(object_pool, ptr);
        kit_var_shallow_cpy(value, ptr);
        kit_var_acquire(object_pool, ptr);
      }
      return 0;
    }
    case KIT_VARTYPE_MAP: {
      kit_map* map    = KIT_VAR_AS_MAP(base);
      kit_var* search = kit_map_find_or_insert(object_pool, map, index);

      if (search && value) {
        kit_var_release(object_pool, search);
        kit_var_shallow_cpy(value, search);
        kit_var_acquire(object_pool, search);
      }
      return 0;
    }
    case KIT_VARTYPE_VEC2: {
      int    idx    = kit_cast_to_int(index);
      double set_to = kit_cast_to_float(value);
      if (idx < 2) base->val.vec2[idx] = set_to;
      return 0;
    }
    case KIT_VARTYPE_VEC3: {
      int    idx    = kit_cast_to_int(index);
      double set_to = kit_cast_to_float(value);
      if (idx < 3) base->val.vec3[idx] = set_to;
      return 0;
    }
    case KIT_VARTYPE_VEC4: {
      int    idx    = kit_cast_to_int(index);
      double set_to = kit_cast_to_float(value);
      if (idx < 4) base->val.vec4[idx] = set_to;
      return 0;
    }
    case KIT_VARTYPE_MAT3: {
      if (value->type != KIT_VARTYPE_VEC3) {
        fprintf(stderr, "Attempt to write to matrix (3x3) with non vec3 value\n");
        return -1;
      }

      kit_mat3*       mat = KIT_VAR_AS_MAT3(base);
      const kit_vec3* vec = &value->val.vec3;
      int             idx = kit_cast_to_int(index);

      memcpy(mat->m[idx], *vec, sizeof(kit_vec3));
      return 0;
    }
    case KIT_VARTYPE_MAT4: {
      if (value->type != KIT_VARTYPE_VEC4) {
        fprintf(stderr, "Attempt to write to matrix (4x4) with non vec4 value\n");
        return -1;
      }

      kit_mat4*       mat = KIT_VAR_AS_MAT4(base);
      const kit_vec4* vec = &value->val.vec4;
      int             idx = kit_cast_to_int(index);

      memcpy(mat->m[idx], *vec, sizeof(kit_vec4));
      return 0;
    }
    default: fprintf(stderr, "Attempt to index assign to invalid base: type=%s\n", kit_var_type_to_string(base->type)); return -1;
  }
}
