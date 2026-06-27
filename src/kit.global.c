#include "../inc/kit.global.h"

#include "../inc/kit.cc.h"

int
kit_global_namespace_init(const struct kitc_function* funcs, u32 nfuncs, const struct kit_var* vars, u32 nvars, kit_global_namespace* dst)
{
  u32 func_capacity = MAX(8, nfuncs);
  u32 var_capacity  = MAX(8, nvars);

  struct kitc_function* funcs_array       = calloc(func_capacity, sizeof(kitc_function));
  struct kit_var*       vars_array        = calloc(var_capacity, sizeof(kit_var));
  u32*                  vars_hashes_array = calloc(var_capacity, sizeof(u32));

  memcpy(funcs_array, funcs, nfuncs * sizeof(kitc_function));
  memcpy(vars_array, vars, nvars * sizeof(kit_var)); /* can do a shallow copy but whatever */

  for (u32 i = 0; i < nvars; i++) { vars_hashes_array[i] = kit_var_hash(&vars[i]); }

  dst->vars       = vars_array;
  dst->funcs      = funcs_array;
  dst->var_hashes = vars_hashes_array;

  dst->vars_count  = nvars;
  dst->funcs_count = nfuncs;

  dst->vars_capacity  = var_capacity;
  dst->funcs_capacity = func_capacity;

  return 0;
}

void
kit_global_namespace_free(kit_global_namespace* space)
{
  free(space->funcs);
  free(space->vars);
  free(space->var_hashes);
}

int
kit_global_namespace_add_function(struct kitc_function* func, kit_global_namespace* space)
{
  if (space->funcs_count + 1 >= space->funcs_capacity) {
    u32            new_capacity = space->funcs_capacity * 2;
    kitc_function* new_funcs    = realloc(space->funcs, new_capacity * sizeof(kitc_function));

    if (!new_funcs) return -1;

    space->funcs          = new_funcs;
    space->funcs_capacity = new_capacity;
  }

  memcpy(&space->funcs[space->funcs_count], func, sizeof(kitc_function));

  space->funcs_count++;

  return 0;
}

int
kit_global_namespace_add_variable(u32 variable_name_hash, struct kit_var* var, kit_global_namespace* space)
{
  if (space->vars_count + 1 >= space->vars_capacity) {
    u32      new_capacity = space->vars_capacity * 2;
    kit_var* new_vars     = realloc(space->vars, new_capacity * sizeof(kit_var));

    if (!new_vars) return -1;

    space->vars          = new_vars;
    space->vars_capacity = new_capacity;
  }

  memcpy(&space->vars[space->vars_count], var, sizeof(kit_var));
  space->var_hashes[space->vars_count] = variable_name_hash;

  space->vars_count++;

  return 0;
}

struct kitc_function*
kit_global_namespace_find_function(u32 id, kit_global_namespace* space)
{
  for (u32 i = 0; i < space->funcs_count; i++) {
    if (space->funcs[i].name_hash == id) { return &space->funcs[i]; }
  }

  return NULL;
}

struct kit_var*
kit_global_namespace_find_variable(u32 id, kit_global_namespace* space)
{
  for (u32 i = 0; i < space->vars_count; i++) {
    if (space->var_hashes[i] == id) { return &space->vars[i]; }
  }

  return NULL;
}
