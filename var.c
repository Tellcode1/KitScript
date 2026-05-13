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

#include "var.h"

#include "cast.h"
#include "pool.h"
#include "stdafx.h"

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
    case E_VARTYPE_NULL:
    default: break;

    case E_VARTYPE_VOID:
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

      e_var*       members = calloc(s->member_count, sizeof(e_var));
      const char** names   = (const char**)calloc(s->member_count, sizeof(char*));
      u32*         hashes  = calloc(s->member_count, sizeof(u32));

      for (u32 i = 0; i < s->member_count; i++) {
        e_var_deep_cpy(&s->members[i], &members[i]);
        hashes[i] = s->member_hashes[i];
        names[i]  = s->member_names[i];
      }

      dst->val.struc = e_refdobj_pool_acquire(&ge_pool);
      if (dst->val.struc) {
        E_VAR_AS_STRUCT(dst)->members       = members;
        E_VAR_AS_STRUCT(dst)->member_hashes = hashes;
        E_VAR_AS_STRUCT(dst)->member_names  = names;
        E_VAR_AS_STRUCT(dst)->member_count  = s->member_count;
      }

      return 0;
    }
    case E_VARTYPE_MAP: {
      dst->val.map = e_refdobj_pool_acquire(&ge_pool);
      /**
       * Create an array of all key value pairs as a map
       * And use it to create the map.
       */
      u32    npairs    = E_VAR_AS_MAP(var)->size;
      e_var* flattened = calloc(npairs, sizeof(e_var) * 2);
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
  int* refc = nullptr;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = &v->val.s->refc; break;
    case E_VARTYPE_STRUCT: refc = &v->val.struc->refc; break;
    default: refc = nullptr; break;
  }

  if (refc == nullptr) return -1;
  return (*refc)++;
}

void
e_var_release(e_var* v)
{
  if (!v) return;
  int* refc = nullptr;
  switch (v->type) {
    case E_VARTYPE_MAP: refc = &v->val.map->refc; break;
    case E_VARTYPE_LIST: refc = &v->val.list->refc; break;
    case E_VARTYPE_STRING: refc = &v->val.s->refc; break;
    case E_VARTYPE_STRUCT: refc = &v->val.struc->refc; break;
    default: refc = nullptr; break;
  }

  if (refc == nullptr) return;

  (*refc)--;
  if (*refc <= 0) e_var_free(v);
}

void
e_var_free(e_var* var)
{
  if (!var) return;

  switch (var->type) {
    default:
    case E_VARTYPE_NULL:
    case E_VARTYPE_VOID:
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
      e_refdobj_pool_return(&ge_pool, var->val.list);
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
  memset(var, 0, sizeof(*var));
}

void
e_var_print(const struct e_var* v, FILE* f)
{
  switch (v->type) {
    default:
    case E_VARTYPE_NULL: fprintf(f, "null"); break;
    case E_VARTYPE_VOID: fprintf(f, "void"); break;
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

        if (elem->type == E_VARTYPE_STRING) fputc('\'', f);
        e_var_print(elem, f);
        if (elem->type == E_VARTYPE_STRING) fputc('\'', f);

        if (i < E_VAR_AS_LIST(v)->size - 1) { fputs(", ", f); }
      }
      fputc(']', f);
      break;
    }
    case E_VARTYPE_STRUCT: {
      fputc('{', f);
      for (u32 i = 0; i < E_VAR_AS_STRUCT(v)->member_count; i++) {
        const e_var* elem        = &E_VAR_AS_STRUCT(v)->members[i];
        const char*  member_name = E_VAR_AS_STRUCT(v)->member_names[i];

        fputs(member_name, f);
        fputs(":", f);

        if (elem->type == E_VARTYPE_STRING) fputc('\'', f);
        e_var_print(elem, f);
        if (elem->type == E_VARTYPE_STRING) fputc('\'', f);

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

        if (key->type == E_VARTYPE_STRING) fputc('\'', f);
        e_var_print(key, f);
        if (key->type == E_VARTYPE_STRING) fputc('\'', f);

        fputs(":", f);

        if (key->type == E_VARTYPE_STRING) fputc('\'', f);
        e_var_print(val, f);
        if (key->type == E_VARTYPE_STRING) fputc('\'', f);

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
    case E_VARTYPE_VOID: strncpy(buffer, "void", buffer_size - 1); break;
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
      buffer[buffer_size] = 0;

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
    case E_VARTYPE_VOID: total += strlen("void"); break;
    case E_VARTYPE_INT: total += snprintf(nullptr, 0, "%i", v->val.i); break;
    case E_VARTYPE_CHAR: total += snprintf(nullptr, 0, "%c", v->val.c); break;
    case E_VARTYPE_BOOL: return strlen((int)v->val.b ? "true" : "false"); break;
    case E_VARTYPE_FLOAT: total += snprintf(nullptr, 0, "%g", v->val.f); break;
    case E_VARTYPE_VEC2:
    case E_VARTYPE_VEC3:
    case E_VARTYPE_VEC4: {
      e_vec4 vooctor;
      evector_zero_extend(v, vooctor);
      for (u32 i = 0; i < 4; i++) {
        total += snprintf(nullptr, 0, "c=%g", vooctor[i]);
        if (i != 3) total += strlen(", ");
      }
      break;
    }

    case E_VARTYPE_MAT3: {
      e_vec4 vooctor;
      evector_zero_extend(v, vooctor);
      for (u32 i = 0; i < 3; i++) {
        for (u32 j = 0; j < 3; j++) { total += snprintf(nullptr, 0, "%g", E_VAR_AS_MAT3(v)->m[i][j]); }
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
  switch (var->type) {
    case E_VARTYPE_VOID:
    case E_VARTYPE_NULL: return 0;
    case E_VARTYPE_INT: return e_hash(&var->val.i, sizeof(var->val.i));
    case E_VARTYPE_BOOL: return e_hash(&var->val.b, sizeof(bool));
    case E_VARTYPE_CHAR: return e_hash(&var->val.c, sizeof(char));
    case E_VARTYPE_FLOAT: return e_hash(&var->val.f, sizeof(var->val.f));
    case E_VARTYPE_DESCRIPTOR: return e_hash((void*)&var->val.descriptor, sizeof(var->val.descriptor)); // hash the pointer.
    case E_VARTYPE_STRING: return e_hash(E_VAR_AS_STRING(var)->s, strlen(E_VAR_AS_STRING(var)->s));
    case E_VARTYPE_VEC2: return e_hash(var->val.vec2, sizeof(e_vec2));
    case E_VARTYPE_VEC3: return e_hash(var->val.vec3, sizeof(e_vec3));
    case E_VARTYPE_VEC4: return e_hash(var->val.vec4, sizeof(e_vec4));
    case E_VARTYPE_MAT3: return e_hash(E_VAR_AS_MAT3(var)->m, sizeof(e_mat3));
    case E_VARTYPE_MAT4: return e_hash(E_VAR_AS_MAT4(var)->m, sizeof(e_mat4));
    case E_VARTYPE_LIST: return hash_list(E_VAR_AS_LIST(var)->vars, E_VAR_AS_LIST(var)->size);
    case E_VARTYPE_MAP: {
      const u32 random_prime = 61;
      return hash_list(E_VAR_AS_MAP(var)->keys, E_VAR_AS_MAP(var)->size)
          + (random_prime * hash_list(E_VAR_AS_MAP(var)->vals, E_VAR_AS_MAP(var)->size));
    }
    case E_VARTYPE_STRUCT: {
      return hash_list(E_VAR_AS_STRUCT(var)->members, E_VAR_AS_STRUCT(var)->member_count);
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
  if (a->type == E_VARTYPE_VOID || b->type == E_VARTYPE_VOID) { return a->type == b->type; }

  switch (a->type) {
    default: assert(0);

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
  if (!s) return nullptr;
  for (u32 i = 0; i < s->member_count; i++) {
    if (s->member_hashes[i] == hash) { return &s->members[i]; }
  }
  return nullptr;
}
