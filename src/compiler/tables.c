#include "tables.h"

#include "../../inc/kit.ast.h"

RETURNS_ERRCODE int
append_defer_entry(kit_compiler* cc, int* exprs, u32 nexprs)
{
  kitc_defer_scope* scope = cc->defer_stack;
  if (scope->count >= scope->capacity) {
    u32               new_capacity = MAX(scope->capacity * 2, 4);
    kitc_defer_entry* new_entries  = realloc(scope->entries, sizeof(kitc_defer_entry) * new_capacity);
    if (!new_entries) return -1;

    scope->entries  = new_entries;
    scope->capacity = new_capacity;
  }

  for (u32 i = 0; i < nexprs; i++) {
    if (KIT_GET_NODE(cc->ast, exprs[i])->type == KIT_AST_NODE_DEFER) {
      cerror(KIT_GET_NODE(cc->ast, exprs[i])->common.span, "Defer statement in another defer statement\n");
      return -1;
    }
  }

  scope->entries[scope->count++] = (kitc_defer_entry){ .exprs = exprs, .nexprs = nexprs };

  return 0;
}

int
append_literal_variable(kitc_literal_table* literals, const kit_var* v)
{
  if (literals->literals_count >= literals->literals_capacity) {
    u32 new_c = MAX(literals->literals_capacity * 2, 4);

    kit_var* new_literals       = (kit_var*)realloc(literals->literals, sizeof(kit_var) * new_c);
    u32*     new_literal_hashes = (u32*)realloc(literals->literal_hashes, sizeof(u32) * new_c);

    if (!new_literals || !new_literal_hashes) {
      free(new_literals); // free(NULL) = noop
      free(new_literal_hashes);
      return -1;
    }

    literals->literals          = new_literals;
    literals->literal_hashes    = new_literal_hashes;
    literals->literals_capacity = new_c;
  }

  u32 id = literals->literals_count;

  memcpy(&literals->literals[id], v, sizeof(kit_var));
  literals->literal_hashes[id] = kit_var_hash(v);

  literals->literals_count++;

  return 0;
}

RETURNS_ERRCODE int
add_literal_to_track(kit_compiler* cc, const kit_var* v)
{
  kitc_literal_table* literals = cc->lit_table;
  const u32           hash     = kit_var_hash(v);
  bool                found    = false;

  for (u32 i = 0; i < literals->literals_count; i++) {
    if (literals->literal_hashes[i] == hash && kit_var_equal(v, &literals->literals[i])) {
      if (found) {
        fprintf(stderr, "Hit! Hash %u is equal, and vars are:", hash);
        exit(-1);
        kit_var_print(v, stderr);
        fprintf(stderr, " | ");
        kit_var_print(&literals->literals[i], stderr);
        fprintf(stderr, "\n");
      }

      found = true;
      // break;
    }
  }

  if (!found) {
    int e = append_literal_variable(literals, v);
    if (e < 0) return e;
  }

  return 0;
}

int
append_function_entry(kit_arena* a, kitc_function_table* funcs, const kitc_function* func)
{
  if (funcs->functions_count >= funcs->functions_capacity) {
    u32            new_capacity  = MAX(funcs->functions_capacity * 2, 4);
    kitc_function* new_functions = (kitc_function*)realloc(funcs->functions, sizeof(kitc_function) * new_capacity);
    if (!new_functions) return -1;

    funcs->functions          = new_functions;
    funcs->functions_capacity = new_capacity;
  }

  funcs->functions[funcs->functions_count] = *func;
  funcs->functions_count++;

  return 0;
}

int
append_struct_info(kit_compiler* cc, const kitc_struct_information* data)
{
  kitc_struct_table* table = cc->struct_table;
  if (table->structs_count >= table->structs_capacity) {
    u32                      new_capacity = MAX(table->structs_capacity * 2, 4);
    kitc_struct_information* new_structs  = realloc(table->structs, new_capacity * sizeof(kitc_struct_information));
    if (!new_structs) return -1;

    table->structs_capacity = new_capacity;
    table->structs          = new_structs;
  }

  memcpy(&table->structs[table->structs_count], data, sizeof(kitc_struct_information));
  table->structs_count++;

  return 0;
}

int
append_struct_decleration(kit_arena* a, const char* name, kitc_struct_information* deposit)
{
  u32 hash = kit_hash(name, strlen(name));

  if (deposit->fields_count >= deposit->field_capacity || !deposit->field_hashes) {
    u32  field_cap_new    = MAX(deposit->field_capacity * 2, 4);
    u32* field_hashes_new = (u32*)realloc(deposit->field_hashes, field_cap_new * sizeof(u32));
    if (!field_hashes_new) { return -1; }

    deposit->field_capacity = field_cap_new;
    deposit->field_hashes   = field_hashes_new;

    char** field_names_new = (char**)realloc((void*)deposit->field_names, field_cap_new * sizeof(char*));
    if (!field_names_new) {
      free(field_hashes_new);
      return -1;
    }

    deposit->field_names = field_names_new;
  }

  deposit->field_hashes[deposit->fields_count] = hash;
  deposit->field_names[deposit->fields_count]  = kit_arnstrdup(a, name);

  deposit->fields_count++;

  return 0;
}

RETURNS_ERRCODE int
ns_push(kit_compiler* cc, const char* name)
{
  if (cc->ns->nnamespaces >= cc->ns->capacity) {
    u32    new_cnamespaces = cc->ns->capacity * 2;
    char** new_namespaces  = (char**)realloc((void*)cc->ns->namespaces, new_cnamespaces * sizeof(char*));
    if (!new_namespaces) return -1;

    cc->ns->namespaces = new_namespaces;
    cc->ns->capacity   = new_cnamespaces;
  }

  cc->ns->namespaces[cc->ns->nnamespaces++] = kit_arnstrdup(cc->arena, name);

  return 0;
}

void
ns_pop(kit_compiler* cc)
{
  cc->ns->nnamespaces--;
  /* free(cc->ns->namespaces[--cc->ns->nnamespaces]); */
}