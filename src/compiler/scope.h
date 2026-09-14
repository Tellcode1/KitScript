#ifndef KIT_COMPILER_SCOPE_H
#define KIT_COMPILER_SCOPE_H

#include "../../inc/kit.cc.h"
#include "vreg.h"

typedef enum kit_name_kind {
  KIT_NAME_LOCAL,       /* local vreg */
  KIT_NAME_GLOBAL,      /* global slot (user declared global) */
  KIT_NAME_BUILTIN_VAR, /* builtin constant variable */
  KIT_NAME_CALLABLE,    /* function */
} kit_name_kind;

typedef struct kit_name_resolution {
  kit_name_kind kind;
  union {
    kit_vreg_t reg;         /* LOCAL */
    u32        global_id;   /* GLOBAL */
    kit_var    builtin_val; /* BUILTIN_VAR */
    u32        fn_hash;     /* CALLABLE */
  };
} kit_name_resolution;

/**
 * Use the namespace stack to build
 * a fully qualified name for the variable.
 */
static inline char*
qualify_name(const kit_compiler* cc, const char* name)
{
  size_t len = strlen(name) + 1;

  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    len += strlen(cc->ns->namespaces[i]) + 2; // ::
  }

  char* out = (char*)kit_arnalloc(cc->arena, len);
  out[0]    = '\0';

  char* p = out;
  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    p = kit_strlpcat(p, cc->ns->namespaces[i], out, len);
    p = kit_strlpcat(p, "::", out, len);
  }

  p = kit_strlpcat(p, name, out, len);
  (void)p;
  return out;
}

static inline kit_vreg_t
scope_define(kit_compiler* cc, kit_filespan span, const char* name, bool is_const)
{
  kit_vreg_t reg = vreg_alloc(cc);

  kitc_var* v     = (kitc_var*)kit_arnalloc(cc->arena, sizeof(kitc_var));
  v->name         = name;
  v->name_hash    = kit_hash(name, strlen(name));
  v->is_const     = is_const;
  v->is_global    = (cc->scope->parent == NULL); /* no scope above this? global. */
  v->span         = span;
  v->next         = cc->scope->vars;
  cc->scope->vars = v;
  if (v->is_global) {
    v->slot.global_id = cc->next_global++;
  } else {
    v->slot.reg = reg;
  }

  return v->is_global ? (kit_vreg_t)v->slot.global_id : v->slot.reg;
}

static inline void
scope_define_in_register(kit_compiler* cc, kit_vreg_t reg, kit_filespan span, const char* name, bool is_const)
{
  kitc_var* v     = (kitc_var*)kit_arnalloc(cc->arena, sizeof(kitc_var));
  v->name         = name;
  v->name_hash    = kit_hash(name, strlen(name));
  v->is_const     = is_const;
  v->is_global    = (cc->scope->parent == NULL); /* no scope above this? global. */
  v->span         = span;
  v->next         = cc->scope->vars;
  cc->scope->vars = v;
  if (v->is_global) { v->slot.global_id = cc->next_global++; }

  v->slot.reg = reg;
}

static inline int
scope_lookup_reg(kit_compiler* cc, u32 hash)
{
  kitc_scope* s = cc->scope;
  while (s) {
    kitc_var* v = s->vars;
    while (v) {
      if (v->name_hash == hash) return v->slot.reg;
      v = v->next;
    }
    s = s->parent;
  }
  return -1;
}

static inline kitc_var*
scope_lookup_info(kit_compiler* cc, u32 hash)
{
  kitc_scope* s = cc->scope;
  while (s) {
    kitc_var* v = s->vars;
    while (v) {
      if (v->name_hash == hash) return v;
      v = v->next;
    }
    s = s->parent;
  }
  return NULL;
}

static inline void
scope_push(kit_compiler* cc)
{
  kitc_scope* s = (kitc_scope*)kit_arnalloc(cc->arena, sizeof(kitc_scope));
  s->vars       = NULL;
  s->parent     = cc->scope;
  cc->scope     = s;
}

static inline void
scope_pop(kit_compiler* cc)
{ cc->scope = cc->scope->parent; }

int        resolve_name(kit_compiler* cc, u32 hash, const char* full, kit_name_resolution* out);
kit_vreg_t emit_name_load(kit_compiler* cc, const kit_name_resolution* r);

#endif // KIT_COMPILER_SCOPE_H