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

#include "../inc/cc.h"

#include "../inc/arena.h"
#include "../inc/ast.h"
#include "../inc/bfunc.h"
#include "../inc/bstructs.h"
#include "../inc/bvar.h"
#include "../inc/cerr.h"
#include "../inc/ir.h"
#include "../inc/operate.h"
#include "../inc/pool.h"
#include "../inc/reg.h"
#include "../inc/regalloc.h"
#include "../inc/rwhelp.h"
#include "../inc/stdafx.h"
#include "../inc/strint.h"
#include "../inc/var.h"

#include <assert.h>
#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct val_t;

static const u32 init_code_capacity        = 1024;
static const u32 init_literal_capacity     = 64;
static const u32 init_function_capacity    = 32;
static const u32 init_namespaces_capacity  = 16;
static const u32 init_label_jump_capacity  = 128;
static const u32 init_structs_capacity     = 64;
static const u32 init_fields_capacity      = 32;
static const u32 init_defer_entry_capacity = 32;
static const u32 init_jumps_capacity       = 64;

static ATTR_NODISCARD char* qualify_name(const e_compiler* cc, const char* name);

static inline RETURNS_ERRCODE int defer_push_scope(e_compiler* cc);
static inline void                defer_pop_scope(e_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_current_scope(e_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_all_scopes(e_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_to_depth(e_compiler* cc, u32 target_depth);
static inline u32                 defer_get_current_depth(e_compiler* cc);

static inline ATTR_NODISCARD u32  label_find(u32 label_id, const ecc_label_table* table);
static inline RETURNS_ERRCODE int emit_and_record_jmp(e_compiler* cc, eir_opcode opcode, ereg_t condition, u32 label_id);
static inline void                define_and_emit_label(e_compiler* cc, u32 label_id);

static RETURNS_ERRCODE int           append_defer_entry(e_compiler* cc, int* exprs, u32 nexprs);
static RETURNS_ERRCODE int           append_function_entry(e_arena* a, ecc_function_table* funcs, const ecc_function* func);
static RETURNS_ERRCODE int           append_struct_decleration(e_arena* a, const char* name, ecc_struct_information* deposit);
static RETURNS_ERRCODE int           append_literal_variable(ecc_literal_table* literals, const e_var* v);
static inline ecc_label_jumps_table* append_label_entry(e_arena* a, u32 label_id, ecc_label_table* table);

static RETURNS_ERRCODE int collect_struct_declerations(e_compiler* cc, int* stmts, u32 nstmts, ecc_struct_information* deposit);
static RETURNS_ERRCODE int append_struct_info(e_compiler* cc, const ecc_struct_information* data);

static inline RETURNS_ERRCODE int compiler_make_fork(const e_compiler* old_c, e_compiler* new_c);
static inline void                compiler_join_fork(const e_compiler* copy, e_compiler* cc);

static RETURNS_ERRCODE int    value_init(e_compiler* cc, int node, struct val_t* d);
static void                   value_free(struct val_t* lv);
static RETURNS_ERRCODE ereg_t emit_lvalue_load(e_compiler* cc, struct val_t* lv);
static RETURNS_ERRCODE ereg_t emit_lvalue_assign(e_compiler* cc, ereg_t value, struct val_t* lv);

static inline e_ast_node_type     var_type_to_node_type(e_vartype var_type);
static inline RETURNS_ERRCODE int convert_literal_to_node(e_ast* p, int override_node, const e_var* lit);
static inline RETURNS_ERRCODE int convert_node_to_literal(const e_compiler* cc, int node, e_var* o);
static inline bool                is_literal_value(const e_ast* ast, int node);

/* Return register ID of result, -1 on error */
static RETURNS_ERRCODE ereg_t compile_and_push_literal_variable(e_compiler* cc, const e_var* v);
static RETURNS_ERRCODE ereg_t compile_literal(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_list(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_map(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_function_definition(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_binary_op(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_unary_op(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_index(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_function_call(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_if_statement(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_while_statement(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_for_statement(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_member_access(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_assign(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_return(e_compiler* cc, int node);
// static RETURNS_ERRCODE ereg_t compile_struct_constructor(e_compiler* fork, e_filespan span, const ecc_struct_information* struc);
static RETURNS_ERRCODE ereg_t compile_struct_decleration(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_variable_decleration(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_variable_load(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_namespace_decleration(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_builtin_structure(e_compiler* cc, const e_builtin_struct* b);
static RETURNS_ERRCODE ereg_t compile_builtin_structures(e_compiler* cc);
static RETURNS_ERRCODE ereg_t compile_function(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile_root(e_compiler* cc, int node);
static RETURNS_ERRCODE ereg_t compile(e_compiler* cc, int node);

static inline ereg_t
vreg_alloc(e_compiler* cc)
{ return cc->next_reg++; }

static ereg_t
scope_define(e_compiler* cc, e_filespan span, u32 hash, bool is_const)
{
  ereg_t reg = vreg_alloc(cc);

  ecc_var* v      = e_arnalloc(cc->arena, sizeof(ecc_var));
  v->name_hash    = hash;
  v->slot.reg     = reg;
  v->is_const     = is_const;
  v->is_global    = (cc->scope->parent == NULL); /* no scope above this? global. */
  v->next         = cc->scope->vars;
  cc->scope->vars = v;
  return reg;
}

static int
scope_lookup_reg(e_compiler* cc, u32 hash)
{
  ecc_scope* s = cc->scope;
  while (s) {
    ecc_var* v = s->vars;
    while (v) {
      if (v->name_hash == hash) return v->slot.reg;
      v = v->next;
    }
    s = s->parent;
  }
  return -1;
}

static ecc_var*
scope_lookup_info(e_compiler* cc, u32 hash)
{
  ecc_scope* s = cc->scope;
  while (s) {
    ecc_var* v = s->vars;
    while (v) {
      if (v->name_hash == hash) return v;
      v = v->next;
    }
    s = s->parent;
  }
  return NULL;
}

static void
scope_push(e_compiler* cc)
{
  ecc_scope* s = e_arnalloc(cc->arena, sizeof(ecc_scope));
  s->vars      = NULL;
  s->parent    = cc->scope;
  cc->scope    = s;
}

static void
scope_pop(e_compiler* cc)
{
  assert(cc->scope && cc->scope->parent);
  cc->scope = cc->scope->parent;
}

/**
 * Operators like SUB (-) can be used
 * as unary operators and that would add confusion
 * to the parser.
 * Only supports binary operators.
 * Unary operators must be handled seperately.
 */
static inline eir_opcode
e_binary_operator_to_opcode(e_operator op)
{
  switch (op) {
    case E_OPERATOR_ADD: return EIR_OPCODE_ADD;
    case E_OPERATOR_SUB: return EIR_OPCODE_SUB;
    case E_OPERATOR_MUL: return EIR_OPCODE_MUL;
    case E_OPERATOR_DIV: return EIR_OPCODE_DIV;
    case E_OPERATOR_MOD: return EIR_OPCODE_MOD;
    case E_OPERATOR_EXP: return EIR_OPCODE_EXP;
    case E_OPERATOR_AND: return EIR_OPCODE_AND;
    case E_OPERATOR_OR: return EIR_OPCODE_OR;
    case E_OPERATOR_BAND: return EIR_OPCODE_BAND;
    case E_OPERATOR_BOR: return EIR_OPCODE_BOR;
    case E_OPERATOR_XOR: return EIR_OPCODE_XOR;
    case E_OPERATOR_ISEQL: return EIR_OPCODE_EQL;
    case E_OPERATOR_ISNEQ: return EIR_OPCODE_NEQ;
    case E_OPERATOR_LT: return EIR_OPCODE_LT;
    case E_OPERATOR_LTE: return EIR_OPCODE_LTE;
    case E_OPERATOR_GT: return EIR_OPCODE_GT;
    case E_OPERATOR_GTE: return EIR_OPCODE_GTE;
    case E_OPERATOR_NOT: return EIR_OPCODE_NOT;
    case E_OPERATOR_BNOT: return EIR_OPCODE_BNOT;
    case E_OPERATOR_DEC: return EIR_OPCODE_DEC;
    case E_OPERATOR_INC: return EIR_OPCODE_INC;
  }
  return -1;
}

typedef enum e_lval_type {
  E_LVAL_VAR,
  E_LVAL_GVAR,    // global variable
  E_LVAL_MEMBER,  // struct member
  E_LVAL_INDEX,   // indexed array
  E_LVAL_UNKNOWN, // Error!
} lval_type;

typedef union e_lval_value {
  struct {
    u32   id;
    char* name; // allocated
  } var;
  u32 gvar;
  struct {
    int         base;
    u32         member_hash;
    const char* member;
  } member;
  struct {
    int left_node;  // Compile to get LHS of index. For vec[16], it will push vec to stack.
    int index_node; // Compile to get index. For vec[16], it will push 16 to stack.

    // ereg_t cache_base;
    // ereg_t cache_index;
  } index;
} lval_value;

typedef struct val_t {
  e_filespan* span;
  lval_type   type;
  lval_value  val;
} val_t;

static inline bool
can_make_value(const e_ast* ast, int node)
{
  if (ast == NULL || node < 0) return false;
  return E_GET_NODE(ast, node)->type == E_AST_NODE_VARIABLE || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX
      || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX_ASSIGN || E_GET_NODE(ast, node)->type == E_AST_NODE_MEMBER_ACCESS
      || E_GET_NODE(ast, node)->type == E_AST_NODE_MEMBER_ASSIGN || E_GET_NODE(ast, node)->type == E_AST_NODE_VARIABLE_DECL;
}

static inline RETURNS_ERRCODE int
defer_push_scope(e_compiler* cc)
{
  ecc_defer_scope* scope = e_arnalloc(cc->arena, sizeof(ecc_defer_scope));
  if (!scope) return -1;

  scope->entries = e_xalloc(init_defer_entry_capacity, sizeof(ecc_defer_entry));
  if (!scope->entries) return -1;

  scope->count    = 0;
  scope->capacity = init_defer_entry_capacity;
  scope->parent   = cc->defer_stack;
  cc->defer_stack = scope;
  return 0;
}

static inline void
defer_pop_scope(e_compiler* cc)
{
  ecc_defer_scope* scope = cc->defer_stack;
  cc->defer_stack        = scope->parent;
  free(scope->entries);
}

static inline RETURNS_ERRCODE int
append_defer_entry(e_compiler* cc, int* exprs, u32 nexprs)
{
  ecc_defer_scope* scope = cc->defer_stack;
  if (scope->count >= scope->capacity) {
    u32              new_capacity = MAX(scope->capacity * 2, 4);
    ecc_defer_entry* new_entries  = realloc(scope->entries, sizeof(ecc_defer_entry) * new_capacity);
    if (!new_entries) return -1;

    scope->entries  = new_entries;
    scope->capacity = new_capacity;
  }

  for (u32 i = 0; i < nexprs; i++) {
    if (E_GET_NODE(cc->ast, exprs[i])->type == E_AST_NODE_DEFER) {
      cerror(E_GET_NODE(cc->ast, exprs[i])->common.span, "Defer statement in another defer statement\n");
      return -1;
    }
  }

  scope->entries[scope->count++] = (ecc_defer_entry){ .exprs = exprs, .nexprs = nexprs };

  return 0;
}

// LIFO
static inline RETURNS_ERRCODE int
defer_emit_current_scope(e_compiler* cc)
{
  ecc_defer_scope* scope = cc->defer_stack;
  if (!scope) return 0;

  for (i64 i = (i64)scope->count - 1; i >= 0; i--) {
    u32        nexprs = scope->entries[i].nexprs;
    const int* exprs  = scope->entries[i].exprs;
    for (u32 j = 0; j < nexprs; j++) {
      int e = compile(cc, exprs[j]);
      if (e < 0) return e;
    }
  }
  return 0;
}

/**
 * Emit the deferred statements for all scopes up to now.
 */
static inline RETURNS_ERRCODE int
defer_emit_all_scopes(e_compiler* cc)
{
  ecc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    for (i64 i = (i64)scope->count - 1; i >= 0; i--) {
      u32        nexprs = scope->entries[i].nexprs;
      const int* exprs  = scope->entries[i].exprs;
      for (u32 j = 0; j < nexprs; j++) {
        int e = compile(cc, exprs[j]);
        if (e < 0) {
          cerror(E_GET_NODE(cc->ast, exprs[j])->common.span, "Failed to compile deferred statement [defer]\n");
          return e;
        }
      }
    }
    scope = scope->parent;
  }
  return 0;
}

static inline u32
defer_get_current_depth(e_compiler* cc)
{
  u32              d     = 0;
  ecc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    d++;
    scope = scope->parent;
  }
  return d;
}

// flush all defers up to (but not including) depth
static inline RETURNS_ERRCODE int
defer_emit_to_depth(e_compiler* cc, u32 target_depth)
{
  ecc_defer_scope* scope = cc->defer_stack;
  u32              depth = defer_get_current_depth(cc);
  while (scope && depth > target_depth) {
    for (i64 i = (i64)scope->count - 1; i >= 0; i--) {
      for (u32 j = 0; j < scope->entries[i].nexprs; j++) {
        int e = compile(cc, scope->entries[i].exprs[j]);
        if (e < 0) return e;
      }
    }
    scope = scope->parent;
    depth--;
  }
  return 0;
}

/**
 * Returns UINT32_MAX on no find.
 */
static inline u32
label_find(u32 label_id, const ecc_label_table* table)
{
  for (u32 i = 0; i < table->labels_count; i++) {
    if (table->labels[i].label_id == label_id) { return i; }
  }
  return UINT32_MAX;
}

static inline ecc_label_jumps_table*
append_label_entry(e_arena* a, u32 label_id, ecc_label_table* table)
{
  if (table->labels_count >= table->labels_capacity) {
    u32                    new_c     = MAX(table->labels_capacity * 2, 4);
    ecc_label_jumps_table* new_table = (ecc_label_jumps_table*)realloc(table->labels, new_c * sizeof(ecc_label_jumps_table));
    if (!new_table) return NULL;

    table->labels          = new_table;
    table->labels_capacity = new_c;
  }

  u32 index = label_find(label_id, table);

  ecc_label_jumps_table* end = NULL;
  if (index == UINT32_MAX) {
    // Add the entry at the end
    end = &table->labels[table->labels_count];
    memset(end, 0, sizeof(*end)); // zero it out for safe

    table->labels_count++;
  } else {
    end = &table->labels[index];
  }

  return end;
}

/**
 * Add the jump to the label's stream.
 * opcode is needed because there are multiple
 * jump instructions (JMP,JE,JNE,JZ,JNZ,etc.)
 */
static inline int
emit_and_record_jmp(e_compiler* cc, eir_opcode opcode, ereg_t condition, u32 label_id)
{
  ecc_label_jumps_table* label = append_label_entry(cc->arena, label_id, cc->label_table);

  if (!label->jumps_target_offsets) {
    *label = (ecc_label_jumps_table){
      .label_id             = label_id,
      .defined              = false,
      .label_offset         = 0,
      .jumps_count          = 0,
      .jumps_capacity       = init_jumps_capacity,
      .jumps_target_offsets = e_xalloc(init_jumps_capacity, sizeof(u32)),
    };
  }

  u32 patch_offset = cc->ninstructions;

  if (label->defined) {
    /**
     * Label already defined. Just point out jump to
     * the label.
     */
    switch (opcode) {
      case EIR_OPCODE_JMP: e_emit_ins(cc, (e_ins){ .jmp = { .opcode = EIR_OPCODE_JMP, .target = label->label_offset } }); break;
      case EIR_OPCODE_JZ: e_emit_ins(cc, (e_ins){ .jz = { .opcode = EIR_OPCODE_JZ, .condition = condition, .target = label->label_offset } }); break;
      case EIR_OPCODE_JNZ:
        e_emit_ins(cc, (e_ins){ .jnz = { .opcode = EIR_OPCODE_JNZ, .condition = condition, .target = label->label_offset } });
        break;

      default: return -1;
    }

  } else {
    /**
     * Label not defined. Add an entry that we'll patch in later.
     */
    if (label->jumps_count >= label->jumps_capacity) {
      u32  new_jumps_capacity       = MAX(label->jumps_capacity * 2, 4);
      u32* new_jumps_target_offsets = realloc(label->jumps_target_offsets, sizeof(u32) * new_jumps_capacity);
      if (!new_jumps_target_offsets) return -1;

      label->jumps_target_offsets = new_jumps_target_offsets;
      label->jumps_capacity       = new_jumps_capacity;
    }

    label->jumps_target_offsets[label->jumps_count++] = patch_offset;

    const u32 will_patch_later = 0xDEADBEEF;
    switch (opcode) {
      case EIR_OPCODE_JMP: e_emit_ins(cc, (e_ins){ .jmp = { .opcode = EIR_OPCODE_JMP, .target = will_patch_later } }); break;
      case EIR_OPCODE_JZ: e_emit_ins(cc, (e_ins){ .jz = { .opcode = EIR_OPCODE_JZ, .condition = condition, .target = will_patch_later } }); break;
      case EIR_OPCODE_JNZ: e_emit_ins(cc, (e_ins){ .jnz = { .opcode = EIR_OPCODE_JNZ, .condition = condition, .target = will_patch_later } }); break;

      default: return -1;
    }
    // e_emit_u32(cc, will_patch_later);
  }

  return 0;
}

static inline void
define_and_emit_label(e_compiler* cc, u32 label_id)
{
  e_emit_ins(cc, (e_ins){ .label = { .opcode = EIR_OPCODE_LABEL, .id = label_id } });
  u32 destination_offset = cc->ninstructions;

  ecc_label_jumps_table* label = append_label_entry(cc->arena, label_id, cc->label_table);

  if (!label->jumps_target_offsets) {
    *label = (ecc_label_jumps_table){
      .label_id             = label_id,
      .defined              = true,
      .label_offset         = destination_offset,
      .jumps_count          = 0,
      .jumps_capacity       = init_jumps_capacity,
      .jumps_target_offsets = e_xalloc(init_jumps_capacity, sizeof(u32)),
    };
    return;
  }

  label->defined      = true;
  label->label_offset = destination_offset;

  for (u32 i = 0; i < label->jumps_count; i++) {
    u32 patch_offset                         = label->jumps_target_offsets[i];
    cc->instructions[patch_offset].cj.target = destination_offset;
  }

  // patched every jump up.
  label->jumps_count = 0;
}

static inline RETURNS_ERRCODE int
compiler_make_fork(const e_compiler* old_c, e_compiler* new_c)
{
  ecc_scope* top_scope = old_c->scope;
  while (top_scope && top_scope->parent) { top_scope = top_scope->parent; }

  *new_c = (e_compiler){
    .arena             = old_c->arena,
    .ast               = old_c->ast,
    .info              = old_c->info,
    .loop              = NULL, // reset loop on function.
    .lit_table         = old_c->lit_table,
    .builtin_var_table = old_c->builtin_var_table,
    .function_table    = old_c->function_table,
    .label_table       = old_c->label_table,
    .struct_table      = old_c->struct_table,
    .next_label        = old_c->next_label,
    .scope             = top_scope,
    .next_reg          = E_REG_GENERAL_BEGIN, // Seperate register for each function
    .ns                = old_c->ns,
    .stack             = old_c->stack,
    .instructions      = (e_ins*)e_xalloc(sizeof(e_ins), init_code_capacity),
    .ninstructions     = 0,
    .cinstructions     = init_code_capacity,
  };
  scope_push(new_c);
  return new_c->instructions ? 0 : -1;
}

static inline void
compiler_join_fork(const e_compiler* copy, e_compiler* cc)
{
  /* The tables are stored on the main compile function stack. Their address SHOULD NOT change. */
  if (cc->lit_table != copy->lit_table || cc->builtin_var_table != copy->builtin_var_table || cc->function_table != copy->function_table
      || cc->ns != copy->ns) {
    puts("Compiler structure corrupted");
    // abort(); How remove this ?
  }

  /* Ensure we don't ever get two labels in different streams with the same ID */
  cc->next_label = copy->next_label;

  /* Can't modify builtin variable count */
}

/**
 * for error paths. Free everything owned by this fork.
 */
static inline void
compiler_free_fork_entirely(e_compiler* cc)
{
  free(cc->instructions);
  while (cc->defer_stack) defer_pop_scope(cc);
  memset(cc, 0, sizeof *cc);
}

static inline u32
make_label_id(e_compiler* cc)
{ return cc->next_label++; }

static int
value_init(e_compiler* cc, int node, val_t* d)
{
  val_t l = { 0 };

  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_AST_NODE_VARIABLE: {
      char* name = qualify_name(cc, E_GET_NODE(cc->ast, node)->ident.ident);

      u32 id = e_hash(name, strlen(name));

      ecc_var* v = scope_lookup_info(cc, id);
      if (!v) return -1;

      l.span         = &E_GET_NODE(cc->ast, node)->common.span;
      l.type         = v->is_global ? E_LVAL_GVAR : E_LVAL_VAR;
      l.val.var.id   = id;
      l.val.var.name = name;

      *d = l;
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      char* name = qualify_name(cc, E_GET_NODE(cc->ast, node)->let.name);

      u32      id = e_hash(name, strlen(name));
      ecc_var* v  = scope_lookup_info(cc, id);
      if (!v) return -1;

      l.span         = &E_GET_NODE(cc->ast, node)->common.span;
      l.type         = v->is_global ? E_LVAL_GVAR : E_LVAL_VAR;
      l.val.var.id   = id;
      l.val.var.name = name;
      *d             = l;
      return 0;
    }

    case E_AST_NODE_INDEX: {
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                 = E_LVAL_INDEX;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index.index;
      *d                     = l;
      return 0;
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                 = E_LVAL_INDEX;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index_assign.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index_assign.index;
      *d                     = l;
      return 0;
    }

    case E_AST_NODE_MEMBER_ACCESS: {
      l.span                   = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                   = E_LVAL_MEMBER;
      l.val.member.base        = E_GET_NODE(cc->ast, node)->member_access.left;
      l.val.member.member      = E_GET_NODE(cc->ast, node)->member_access.right;
      l.val.member.member_hash = e_hash(l.val.member.member, strlen(l.val.member.member));
      *d                       = l;
      return 0;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      l.type                   = E_LVAL_MEMBER;
      l.span                   = &E_GET_NODE(cc->ast, node)->common.span;
      l.val.member.base        = E_GET_NODE(cc->ast, node)->member_assign.left;
      l.val.member.member      = E_GET_NODE(cc->ast, node)->member_assign.right;
      l.val.member.member_hash = e_hash(l.val.member.member, strlen(l.val.member.member));
      *d                       = l;
      return 0;
    }

    default:
      cerror(E_GET_NODE(cc->ast, node)->common.span, "%i can not be represented as a value (it is %u)\n", node, E_GET_NODE(cc->ast, node)->type);
      return -1;
  }

  return -1;
}

static void
value_free(val_t* lv)
{
  if (lv->type == E_LVAL_VAR) { /* free(lv->val.var.name); */
  }
  memset(lv, 0, sizeof(*lv));
}

static inline int
emit_lvalue_assign(e_compiler* cc, ereg_t value, val_t* lv)
{
  switch (lv->type) {
    case E_LVAL_VAR: {
      ecc_var* v = scope_lookup_info(cc, lv->val.var.id);
      if (!v) return -1;

      /* local to local operation. global to global operations need a temporary register. */
      e_emit_ins(cc, (e_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = v->slot.reg, .src = value } });
      break;
    }
    case E_LVAL_GVAR: {
      ecc_var* v = scope_lookup_info(cc, lv->val.var.id);
      if (!v) return -1;

      /* writing to a global variable from a local register */
      e_emit_ins(cc, (e_ins){ .setg = { .opcode = EIR_OPCODE_SETG, .dst = v->slot.global_id, .src = value } });
      break;
    }
    case E_LVAL_INDEX: {
      ereg_t base  = -1;
      ereg_t index = -1;

      val_t left = { 0 };
      if (value_init(cc, lv->val.index.left_node, &left) < 0) return -1;

      /* prep base */
      base = emit_lvalue_load(cc, &left);
      if (base < 0) return base;

      /* prep index */
      index = compile(cc, lv->val.index.index_node);
      if (index < 0) return index;

      /* index_assign modifies base. */
      e_emit_ins(cc, (e_ins){ .index_assign = { .opcode = EIR_OPCODE_INDEX_ASSIGN, .value = value, .base = base, .index = index } });

      /* write base back to its slot */
      if (emit_lvalue_assign(cc, base, &left) < 0) return -1;

      value_free(&left);
      break;
    }
    case E_LVAL_MEMBER: {
      int left      = lv->val.member.base;
      u32 member_id = lv->val.member.member_hash;

      /* pushes stack top */
      ereg_t base = compile(cc, left);
      if (base < 0) return base;

      e_emit_ins(cc, (e_ins){ .member_assign = { .opcode = EIR_OPCODE_MEMBER_ASSIGN, .value = value, .base = base, .member_id = member_id } });
      break;
    }
    case E_LVAL_UNKNOWN: return -1;
  }

  /* Propogate assigned value */
  return value;
}

static int
emit_lvalue_load(e_compiler* cc, val_t* lv)
{
  ereg_t dst = vreg_alloc(cc);

  if (lv->type == E_LVAL_VAR) {
    ereg_t var_reg = scope_lookup_reg(cc, lv->val.var.id);
    if (var_reg < 0) return -1;

    e_emit_ins(cc, (e_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = dst, .src = var_reg } });
  } else if (lv->type == E_LVAL_GVAR) {
    ecc_var* v = scope_lookup_info(cc, lv->val.var.id);
    if (!v) return -1;

    /* emit a GETG to fetch the global variable into our local register */
    e_emit_ins(cc, (e_ins){ .mov = { .opcode = EIR_OPCODE_GETG, .dst = dst, .src = v->slot.global_id } });
  } else if (lv->type == E_LVAL_INDEX) {
    ereg_t base  = -1;
    ereg_t index = -1;

    base = compile(cc, lv->val.index.left_node);
    if (base < 0) return base;

    index = compile(cc, lv->val.index.index_node);
    if (index < 0) return index;

    e_emit_ins(cc, (e_ins){ .index = { .opcode = EIR_OPCODE_INDEX, .dst = dst, .base = base, .index = index } });
  } else if (lv->type == E_LVAL_MEMBER) {
    int left      = lv->val.member.base;
    u32 member_id = lv->val.member.member_hash;

    /* pushes stack top */
    ereg_t base = compile(cc, left);
    if (base < 0) return base;

    e_emit_ins(cc, (e_ins){ .member_access = { .opcode = EIR_OPCODE_MEMBER_ACCESS, .dst = dst, .base = base, .member_id = member_id } });
  }

  return dst;
}

static inline RETURNS_ERRCODE int
make_string_variable(char* s, e_var* v) // s will be onwed by variable after this
{
  e_refdobj* obj = e_refdobj_pool_acquire(&ge_pool);
  if (!obj) return -1;

  E_OBJ_AS_STRING(obj)->s = s;

  *v = (e_var){ .type = E_VARTYPE_STRING, .val.s = obj };
  return 0;
}

static inline e_ast_node_type
var_type_to_node_type(e_vartype var_type)
{
  switch (var_type) {
    case E_VARTYPE_INT: return E_AST_NODE_INT;
    case E_VARTYPE_BOOL: return E_AST_NODE_BOOL;
    case E_VARTYPE_CHAR: return E_AST_NODE_CHAR;
    case E_VARTYPE_FLOAT: return E_AST_NODE_FLOAT;
    case E_VARTYPE_STRING: return E_AST_NODE_STRING;
    default: return E_AST_NODE_NOP;
  }
}

static inline RETURNS_ERRCODE int
convert_literal_to_node(e_ast* p, int override_node, const e_var* lit)
{
  /* Set literal type */
  E_GET_NODE(p, override_node)->type = var_type_to_node_type(lit->type);

  /* Set literal value */
  switch (lit->type) {
    case E_VARTYPE_INT: E_GET_NODE(p, override_node)->i.i = lit->val.i; break;
    case E_VARTYPE_BOOL: E_GET_NODE(p, override_node)->b.b = lit->val.b; break;
    case E_VARTYPE_CHAR: E_GET_NODE(p, override_node)->c.c = lit->val.c; break;
    case E_VARTYPE_FLOAT: E_GET_NODE(p, override_node)->f.f = lit->val.f; break;
    case E_VARTYPE_STRING: E_GET_NODE(p, override_node)->i.i = lit->val.i; break;
    default: return -1;
  }
  return 0;
}

static inline bool
is_literal_value(const e_ast* ast, int node)
{
  e_ast_node_type type = E_GET_NODE(ast, node)->type;
  return type == E_AST_NODE_INT || type == E_AST_NODE_CHAR || type == E_AST_NODE_BOOL || type == E_AST_NODE_STRING || type == E_AST_NODE_FLOAT;
}

static inline RETURNS_ERRCODE int
convert_node_to_literal(const e_compiler* cc, int node, e_var* o)
{
  e_ast* p = cc->ast;
  switch (E_GET_NODE(p, node)->type) {
    case E_AST_NODE_INT: *o = (e_var){ .type = E_VARTYPE_INT, .val.i = E_GET_NODE(p, node)->i.i }; return 0;
    case E_AST_NODE_FLOAT: *o = (e_var){ .type = E_VARTYPE_FLOAT, .val.f = E_GET_NODE(p, node)->f.f }; return 0;
    case E_AST_NODE_CHAR: *o = (e_var){ .type = E_VARTYPE_CHAR, .val.c = E_GET_NODE(p, node)->c.c }; return 0;
    case E_AST_NODE_BOOL: *o = (e_var){ .type = E_VARTYPE_BOOL, .val.b = E_GET_NODE(p, node)->b.b }; return 0;
    case E_AST_NODE_STRING: return make_string_variable(e_arnstrdup(cc->arena, E_GET_NODE(p, node)->s.s), o);

    case E_AST_NODE_LIST:
    case E_AST_NODE_MAP: /* TODO: Implement */ abort(); break;

    default: return 1;
  }
  return 1;
}

static RETURNS_ERRCODE int
ns_push(e_compiler* cc, const char* name)
{
  if (cc->ns->nnamespaces >= cc->ns->capacity) {
    u32    new_cnamespaces = cc->ns->capacity * 2;
    char** new_namespaces  = (char**)realloc((void*)cc->ns->namespaces, new_cnamespaces * sizeof(char*));
    if (!new_namespaces) return -1;

    cc->ns->namespaces = new_namespaces;
    cc->ns->capacity   = new_cnamespaces;
  }

  cc->ns->namespaces[cc->ns->nnamespaces++] = e_arnstrdup(cc->arena, name);

  return 0;
}

static void
ns_pop(e_compiler* cc)
{
  cc->ns->nnamespaces--;
  /* free(cc->ns->namespaces[--cc->ns->nnamespaces]); */
}

/**
 * Use the namespace stack to build
 * a fully qualified name for the variable.
 */
static char*
qualify_name(const e_compiler* cc, const char* name)
{
  size_t len = strlen(name) + 1;

  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    len += strlen(cc->ns->namespaces[i]) + 2; // ::
  }

  char* out = e_arnalloc(cc->arena, len);
  out[0]    = '\0';

  char* p = out;
  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    p = e_strlpcat(p, cc->ns->namespaces[i], out, len);
    p = e_strlpcat(p, "::", out, len);
  }

  p = e_strlpcat(p, name, out, len);
  (void)p;
  return out;
}

static int
append_literal_variable(ecc_literal_table* literals, const e_var* v)
{
  if (literals->literals_count >= literals->literals_capacity) {
    u32 new_c = MAX(literals->literals_capacity * 2, 4);

    e_var* new_literals       = (e_var*)realloc(literals->literals, sizeof(e_var) * new_c);
    u32*   new_literal_hashes = (u32*)realloc(literals->literal_hashes, sizeof(u32) * new_c);

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

  memcpy(&literals->literals[id], v, sizeof(e_var));
  literals->literal_hashes[id] = e_var_hash(v);

  literals->literals_count++;

  return 0;
}

static RETURNS_ERRCODE int
add_literal_to_track(e_compiler* cc, const e_var* v)
{
  ecc_literal_table* literals = cc->lit_table;
  u32                hash     = e_var_hash(v);
  bool               found    = false;

  for (u32 i = 0; i < literals->literals_count; i++) {
    if (literals->literal_hashes[i] == hash && e_var_equal(v, &literals->literals[i])) {
      found = true;
      break;
    }
  }

  if (!found) {
    int e = append_literal_variable(literals, v);
    if (e < 0) return e;
  }

  return 0;
}

static RETURNS_ERRCODE int
compile_and_push_literal_variable(e_compiler* cc, const e_var* v)
{
  /* Search for the literal in our table. */
  u32 hash = e_var_hash(v);

  int e = add_literal_to_track(cc, v);
  if (e < 0) return e;

  int dst = vreg_alloc(cc);

  e_emit_ins(cc, (e_ins){ .loadk = { .opcode = EIR_OPCODE_LOADK, .dst = dst, .id = hash } });

  return dst;
}

static RETURNS_ERRCODE int
compile_literal(e_compiler* cc, int node)
{
  // Convert the node to a variable and
  e_var v = { .type = E_VARTYPE_NULL };
  if (convert_node_to_literal(cc, node, &v)) return -1;

  // Compile it
  return compile_and_push_literal_variable(cc, &v);
}

static RETURNS_ERRCODE int
compile_list(e_compiler* cc, int node)
{
  u32 nelems = E_GET_NODE(cc->ast, node)->list.nelems;
  assert(nelems < INT32_MAX);

  ereg_t dst = vreg_alloc(cc);

  for (u32 i = 0; i < nelems; i++) {
    int elem_node = E_GET_NODE(cc->ast, node)->list.elems[i];

    ereg_t elem = compile(cc, elem_node);
    if (elem < 0) return elem;

    /* Move at most 16 elements from their registers to our argument list. */
    if (i < E_REG_ARG_COUNT) e_emit_ins(cc, (e_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = E_REG_ARG0 + i, .src = elem } });
    /* else push them to the stack */
    else e_emit_ins(cc, (e_ins){ .push = { .opcode = EIR_OPCODE_PUSH, .reg = elem } });
  }

  e_emit_ins(cc, (e_ins){ .mk_list = { .opcode = EIR_OPCODE_MK_LIST, .dst = dst, .nelems = nelems } });

  /* remove spilled elements from the stack */
  if (nelems > 16) {
    e_var move_sp_down_by = e_var_from_int((int)nelems - 16);

    ereg_t sp_reduce = compile_and_push_literal_variable(cc, &move_sp_down_by);
    if (sp_reduce < 0) return -1;

    /* Subtract the number of spilled registers from sp and store back to it. */
    e_emit_ins(cc, (e_ins){ .binop = { .opcode = EIR_OPCODE_SUB, .a = E_REG_SP, .b = sp_reduce, .dst = E_REG_SP } });
  }

  return dst;
}

static RETURNS_ERRCODE int
compile_map(e_compiler* cc, int node)
{
  u32 npairs = E_GET_NODE(cc->ast, node)->map.npairs;

  ereg_t dst = vreg_alloc(cc);

  assert(npairs < 8);

  for (u32 i = 0; i < npairs; i++) {
    int key = E_GET_NODE(cc->ast, node)->map.keys[i];
    int val = E_GET_NODE(cc->ast, node)->map.values[i];

    ereg_t k = compile(cc, key);
    if (k < 0) return k;

    ereg_t v = compile(cc, val);
    if (v < 0) return v;

    /* copy KV pairs into our argument vector */
    e_emit_ins(cc, (e_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = E_REG_ARG0 + (i * 2), .src = k } });
    e_emit_ins(cc, (e_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = E_REG_ARG0 + (i * 2) + 1, .src = v } });
  }

  e_emit_ins(cc, (e_ins){ .mk_map = { .opcode = EIR_OPCODE_MK_MAP, .dst = dst, .npairs = npairs } });

  return dst;
}

static int
append_function_entry(e_arena* a, ecc_function_table* funcs, const ecc_function* func)
{
  if (funcs->functions_count >= funcs->functions_capacity) {
    u32           new_capacity  = MAX(funcs->functions_capacity * 2, 4);
    ecc_function* new_functions = (ecc_function*)realloc(funcs->functions, sizeof(ecc_function) * new_capacity);
    if (!new_functions) return -1;

    funcs->functions          = new_functions;
    funcs->functions_capacity = new_capacity;
  }

  funcs->functions[funcs->functions_count] = *func;
  funcs->functions_count++;

  return 0;
}

static ereg_t
compile_function_definition(e_compiler* cc, int node)
{
  e_compiler  fork          = { 0 };
  const char* function_name = E_GET_NODE(cc->ast, node)->func.name;
  char*       full          = qualify_name(cc, function_name);

  const u32 hash = e_hash(full, strlen(full));

  int e = 0;

  /* Ensure it doesn't already exist */
  const ecc_function_table* func_table = cc->function_table;
  for (u32 i = 0; i < func_table->functions_count; i++) {
    ecc_function* func = &func_table->functions[i];
    if (func->name_hash == hash) {
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Function \"%s\" redefined\n", function_name);
      e = -1;
      goto ERR;
    }
  }

  /* Ensure we aren't overriding a builtin function */
  for (u32 i = 0; i < E_ARRLEN(eb_funcs); i++) {
    const e_builtin_func* func      = &eb_funcs[i];
    u32                   func_hash = e_hash(func->name, strlen(func->name));
    if (func_hash == hash) {
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Attempt to overshadow builtin function \"%s\"\n", function_name);
      e = -1;
      goto ERR;
    }
  }

  u32     nargs     = E_GET_NODE(cc->ast, node)->func.nargs;
  ereg_t* arg_slots = NULL;

  e = compiler_make_fork(cc, &fork);
  if (e < 0) goto ERR;

  /* Setup argument variables so the function knows they exist. */
  if (nargs > 0) {
    arg_slots = (ereg_t*)e_arnalloc(cc->arena, sizeof(ereg_t) * nargs);
    if (!arg_slots) goto ERR;

    for (u32 i = 0; i < nargs; i++) {
      const char* arg_name = E_GET_NODE(cc->ast, node)->func.args[i];
      e_filespan  span     = E_GET_NODE(cc->ast, node)->func.span;

      char* full_arg_name = qualify_name(cc, arg_name);
      if (!full_arg_name) goto ERR;

      u32 arg_hash = e_hash(full_arg_name, strlen(full_arg_name));

      /* Define argument variable */
      arg_slots[i] = scope_define(&fork, span, arg_hash, false);
      if (e < 0) goto ERR;
    }
  }

  e = defer_push_scope(&fork);
  if (e < 0) goto ERR;

  e = compile_function(&fork, node);
  if (e < 0) {
    e_filespan span = E_GET_NODE(cc->ast, node)->common.span;
    cerror(span, "Failed to compile function body [function definition]\n");
    goto ERR;
  }

  // emit the defer before the return you asshole
  e = defer_emit_current_scope(&fork);
  if (e < 0) goto ERR;

  /* Always need this! */
  // e_emit_instruction(&fork, EIR_OPCODE_RET);
  // e_emit_u8(&fork, false);

  defer_pop_scope(&fork);

  compiler_join_fork(&fork, cc);

  /* perform register allocation */
  era_register_allocation_pass(&fork);

  ecc_function f = {
    .code       = fork.instructions,
    .code_count = fork.ninstructions,
    .arg_slots  = arg_slots,
    .name_hash  = hash,
    .nargs      = nargs,
  };

  e = append_function_entry(cc->arena, cc->function_table, &f);
  if (e < 0) goto ERR;

  return e;

ERR:
  compiler_free_fork_entirely(&fork);
  return e == 0 ? -1 : e;
}

static ereg_t
compile_binary_op(e_compiler* cc, int node)
{
  val_t lv = { 0 };

  bool is_compound = E_GET_NODE(cc->ast, node)->binaryop.is_compound;
  int  left        = E_GET_NODE(cc->ast, node)->binaryop.left;
  int  right       = E_GET_NODE(cc->ast, node)->binaryop.right;

  eir_opcode opcode = e_binary_operator_to_opcode(E_GET_NODE(cc->ast, node)->binaryop.op);
  if (opcode < 0) {
    cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a binary operator\n", E_GET_NODE(cc->ast, node)->binaryop.op);
    goto err;
  }

  if (is_compound && !can_make_value(cc->ast, left)) {
    cerror(E_GET_NODE(cc->ast, left)->common.span, "Can not assign to left\n");
    goto err;
  }

  ereg_t dst = vreg_alloc(cc);

  if (is_compound) {
    // Verified  earlier that we can make it into an lvalue
    int e = value_init(cc, left, &lv);
    if (e < 0) goto err;

    /* Load left */
    ereg_t l = emit_lvalue_load(cc, &lv);
    if (l < 0) goto err;

    /* Load right */
    ereg_t r = compile(cc, right);
    if (r < 0) goto err;

    /* Emit operator */
    e_emit_ins(cc, (e_ins){ .binop = { .opcode = opcode, .dst = dst, .a = l, .b = r } });

    if (emit_lvalue_assign(cc, dst, &lv) < 0) return -1;

    value_free(&lv);
  } else {
    ereg_t a = compile(cc, left);
    if (a < 0) goto err;

    ereg_t b = compile(cc, right);
    if (b < 0) goto err;

    e_emit_ins(cc, (e_ins){ .binop = { .opcode = opcode, .dst = dst, .a = a, .b = b } });
  }

  return dst;

err:
  value_free(&lv);
  return -1;
}

static ereg_t
compile_inc_or_dec(e_compiler* cc, int node)
{
  eir_opcode opcode = -1;
  if (E_GET_NODE(cc->ast, node)->unaryop.op == E_OPERATOR_INC) {
    opcode = EIR_OPCODE_ADD;
  } else if (E_GET_NODE(cc->ast, node)->unaryop.op == E_OPERATOR_DEC) {
    opcode = EIR_OPCODE_SUB;
  }

  int right = E_GET_NODE(cc->ast, node)->unaryop.right;

  if (E_GET_NODE(cc->ast, right)->type != E_AST_NODE_VARIABLE) {
    cerror(E_GET_NODE(cc->ast, right)->common.span, "Can only increment/decrement variables\n");
    return -1;
  }

  val_t lv = { 0 };
  int   e  = value_init(cc, right, &lv);
  if (e < 0) return -1;

  ereg_t rhs = emit_lvalue_load(cc, &lv);

  e_var  one     = e_var_from_int(1);
  ereg_t one_reg = compile_and_push_literal_variable(cc, &one);

  ereg_t tmp = vreg_alloc(cc);
  e_emit_ins(cc, (e_ins){ .binop = { .opcode = opcode, .dst = tmp, .a = rhs, .b = one_reg } });

  /* Emit operator, both dst and a point to the variables slot */
  tmp = emit_lvalue_assign(cc, tmp, &lv);
  if (tmp < 0) return tmp;

  /* Propogate new value */
  return tmp;
}

static ereg_t
compile_unary_op(e_compiler* cc, int node)
{
  /* Special case, INC and DEC assign to variables directly. */
  e_operator oper = E_GET_NODE(cc->ast, node)->unaryop.op;
  if (oper == E_OPERATOR_INC || oper == E_OPERATOR_DEC) { return compile_inc_or_dec(cc, node); }

  eir_opcode opcode = -1;

  switch (E_GET_NODE(cc->ast, node)->unaryop.op) {
    case E_OPERATOR_NOT: opcode = EIR_OPCODE_NOT; break;
    case E_OPERATOR_BNOT: opcode = EIR_OPCODE_BNOT; break;
    case E_OPERATOR_SUB: opcode = EIR_OPCODE_NEG; break;
    case E_OPERATOR_ADD: opcode = EIR_OPCODE_NOP; break;
    default:
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a unary operator\n", E_GET_NODE(cc->ast, node)->unaryop.op);
      return -1;
  }

  bool is_compound = E_GET_NODE(cc->ast, node)->unaryop.is_compound;

  int right = E_GET_NODE(cc->ast, node)->unaryop.right;
  int e     = 0;

  ereg_t dst = vreg_alloc(cc);

  if (is_compound) {
    val_t lv = { 0 };

    if (!can_make_value(cc->ast, right)) {
      cerror(E_GET_NODE(cc->ast, right)->common.span, "Can not assign to right\n");
      return -1;
    }

    // Verified  earlier that we can make it into an lvalue
    e = value_init(cc, right, &lv);
    if (e < 0) goto err;

    /* Load right */
    ereg_t r = compile(cc, right);
    if (r < 0) goto err;

    /* Emit operator */
    e_emit_ins(cc, (e_ins){ .unop = { .opcode = opcode, .dst = dst, .a = r } });

    /* Emit actual assign instruction (takes value produced earlier and assigns it) */
    dst = emit_lvalue_assign(cc, dst, &lv);
    if (dst < 0) goto err;

    value_free(&lv);
  } else {
    /* Load right */
    ereg_t rreg = compile(cc, right);
    if (rreg < 0) goto err;

    /* Emit operator */
    e_emit_ins(cc, (e_ins){ .unop = { .opcode = opcode, .dst = dst, .a = rreg } });
  }

  return dst;

err:
  return -1;
}

static ereg_t
compile_index(e_compiler* cc, int node)
{
  ereg_t dst = vreg_alloc(cc);

  ereg_t base = compile(cc, E_GET_NODE(cc->ast, node)->index.base);
  if (base < 0) return base;

  ereg_t index = compile(cc, E_GET_NODE(cc->ast, node)->index.index);
  if (index < 0) return index;

  e_emit_ins(cc, (e_ins){ .index = { .opcode = EIR_OPCODE_INDEX, .dst = dst, .base = base, .index = index } });

  return dst;
}

static ereg_t
compile_index_assign(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->index_assign.value;

  if (!can_make_value(cc->ast, node)) {
    e_filespan left_span = E_GET_NODE(cc->ast, node)->index_assign.span;
    cerror(left_span, "Can not assign to indexed expression: Failed to lower to lvalue\n");
    return -1;
  }

  val_t v = { 0 };

  ereg_t eval_value = compile(cc, value);

  int e = value_init(cc, node, &v);
  if (e < 0) return e;

  ereg_t dst = emit_lvalue_assign(cc, eval_value, &v);
  value_free(&v);

  return dst;
}

static inline bool
is_builtin_func(const char* name)
{
  for (size_t i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (strcmp(eb_funcs[i].name, name) == 0) return true;
  }
  return false;
}

static inline const e_builtin_func*
get_builtin_func(const char* name)
{
  for (size_t i = 0; i < E_ARRLEN(eb_funcs); i++) {
    if (strcmp(eb_funcs[i].name, name) == 0) return &eb_funcs[i];
  }
  return NULL;
}

static inline int
fold_vector(e_compiler* cc, int replace_node, const int* elems, u32 nelems)
{
  e_vec4 vector = { 0 };
  for (u32 i = 0; i < nelems; i++) {
    e_var elem = E_NULLVAR;
    int   e    = convert_node_to_literal(cc, elems[i], &elem);
    if (e < 0) return e;

    double d  = e_cast_to_float(&elem);
    vector[i] = d;
  }

  e_var v = {
    .type = E_VARTYPE_NULL,
  };
  switch (nelems) {
    default:
    case 2: v.type = E_VARTYPE_VEC2; break;
    case 3: v.type = E_VARTYPE_VEC3; break;
    case 4: v.type = E_VARTYPE_VEC4; break;
  }
  memcpy(v.val.vec4, vector, sizeof(vector));

  /* Compile this vector into a literal */
  int e = compile_and_push_literal_variable(cc, &v);
  if (e < 0) return e;

  return 0;
}

static ereg_t
compile_function_call(e_compiler* cc, int node)
{
  e_filespan function_span = E_GET_NODE(cc->ast, node)->common.span;
  u32        nargs         = E_GET_NODE(cc->ast, node)->call.nargs;
  int*       args          = E_GET_NODE(cc->ast, node)->call.args;

  const char* function_name = E_GET_NODE(cc->ast, node)->call.function_name;

  char* full = qualify_name(cc, function_name);
  u32   hash = e_hash(full, strlen(full));

  if (is_builtin_func(function_name)) {
    /* Validate arguments */
    const e_builtin_func* func = get_builtin_func(function_name);

    if (nargs > func->max_args || nargs < func->min_args) {
      cerror(
          function_span,
          "Builtin function '%s' expects between [%u-%u] arguments, but [%u] were given\n",
          func->name,
          func->min_args,
          func->max_args,
          nargs);
      fprintf(stderr, "[%s:%i] Builtin function is declared as: %s : %s\n", __FILE__, __LINE__, func->signature, func->desc);
      return -1;
    }

  }
  // Find the function (user defined) and check if the argument count matches
  else {
    ecc_function*       func       = NULL;
    ecc_function_table* func_table = cc->function_table;
    for (u32 i = 0; i < func_table->functions_count; i++) {
      if (func_table->functions[i].name_hash == hash) {
        func = &func_table->functions[i];
        break;
      }
    }

    if (func && func->nargs != nargs) {
      cerror(
          E_GET_NODE(cc->ast, node)->common.span,
          "User defined function '%s' expects [%u] arguments, but [%u] were given\n",
          function_name,
          func->nargs,
          nargs);
      return -1;
      assert(func->nargs < 16);
    }
  }

  /**
   * OPTIMIZATION: Replace vec* initializers with a literal value
   * if they have constant values.
   */
  if (cc->info->opt_level >= 1) {
    if (strcmp(function_name, "vec2") == 0 || strcmp(function_name, "vec3") == 0 || strcmp(function_name, "vec4") == 0) {
      bool constant_vector = true;
      for (u32 i = 0; i < nargs; i++) {
        if (!is_literal_value(cc->ast, args[i])) {
          constant_vector = false;
          break;
        }
      }

      if (constant_vector) { return fold_vector(cc, node, args, nargs); }
    }
  }

  int e = 0;
  for (u32 i = 0; i < nargs; i++) {
    ereg_t reg = compile(cc, args[i]); // Pushes stack top
    if (reg < 0) {
      cerror(E_GET_NODE(cc->ast, args[i])->common.span, "Failed to compile argument #%i [function call]\n", i);
      return e;
    }
    /* move to our arguments register */
    if (i < E_REG_ARG_COUNT) {
      e_emit_ins(cc, (e_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = E_REG_ARG0 + i, .src = reg } });
    } else {
      /* Spill the rest of the arguments on the stack */
      e_emit_ins(cc, (e_ins){ .push = { .opcode = EIR_OPCODE_PUSH, .reg = reg } });
    }
  }

  ereg_t dst = vreg_alloc(cc);

  e_emit_ins(cc, (e_ins){ .call = { .opcode = EIR_OPCODE_CALL, .dst = dst, .function_id = hash, .nargs = nargs } });

  /* Cleanup the stack. */
  if (nargs > 16) {
    e_var move_sp_down_by = e_var_from_int((int)nargs - 16);

    ereg_t sp_reduce = compile_and_push_literal_variable(cc, &move_sp_down_by);
    if (sp_reduce < 0) return -1;

    /* Subtract the number of spilled registers from sp and store back to it. */
    e_emit_ins(cc, (e_ins){ .binop = { .opcode = EIR_OPCODE_SUB, .a = E_REG_SP, .b = sp_reduce, .dst = E_REG_SP } });
  }

  return dst;
}

// This is the dirtiest of them all...
static ereg_t
compile_if_statement(e_compiler* cc, int node)
{
  /* Label after if statements body */
  u32 end_label = make_label_id(cc);

  const int condition = E_GET_NODE(cc->ast, node)->if_stmt.condition;
  int       e         = 0;

  /**
   * Label of the next else if / else to jump to. Still used if no branches are present, there's
   * just a jmp instruction directly after the JMP to where the branch would be.
   */
  u32 next_in_chain_label = make_label_id(cc);

  scope_push(cc);

  // condition
  ereg_t cond = compile(cc, condition);
  if (cond < 0) {
    cerror(E_GET_NODE(cc->ast, condition)->common.span, "Failed to compile condition of top if statement [if statement]\n");
    goto ERR;
  }

  // condition failed :<
  e = emit_and_record_jmp(cc, EIR_OPCODE_JZ, cond, next_in_chain_label); // Jump to the next in chain. else if/else/end of if statement
  if (e < 0) goto ERR;

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  // BODY OF ROOT IF STATEMENT
  const int* if_body = E_GET_NODE(cc->ast, node)->if_stmt.body;
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nstmts; i++) {
    e = compile(cc, if_body[i]);
    if (e < 0) {
      cerror(E_GET_NODE(cc->ast, if_body[i])->common.span, "Failed to compile if statement body [if statement]\n");
      goto ERR;
    }
  }

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  defer_pop_scope(cc);
  scope_pop(cc);

  // Still inside the body, JMP over all other branches
  // since we're done executing the body of the if statement
  e = emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, end_label); // JUMP!
  if (e < 0) goto ERR;

  // ELSE IFS
  for (u32 else_if_idx = 0; else_if_idx < E_GET_NODE(cc->ast, node)->if_stmt.nelse_ifs; else_if_idx++) {
    // Emit the next in chain label for instructions to jump to.
    define_and_emit_label(cc, next_in_chain_label);
    next_in_chain_label = make_label_id(cc);

    scope_push(cc);

    // dont worry about it dont worry about it dont worry about it dont worry about it
    struct e_if_stmt* elif = &E_GET_NODE(cc->ast, node)->if_stmt.else_ifs[else_if_idx];

    // CONDITION
    ereg_t elif_cond = compile(cc, elif->condition);
    if (elif_cond < 0) {
      cerror(E_GET_NODE(cc->ast, elif->condition)->common.span, "Failed to compile condition of else if statement [if statement]\n");
      goto ERR;
    }

    /* Failed. Jump to the next in chain. */
    e = emit_and_record_jmp(cc, EIR_OPCODE_JZ, elif_cond, next_in_chain_label);
    if (e < 0) goto ERR;

    // JZ pops condition

    e = defer_push_scope(cc);
    if (e < 0) goto ERR;

    /* Condition true! Execute the body */
    for (u32 i = 0; i < elif->nstmts; i++) {
      e = compile(cc, elif->body[i]);
      if (e < 0) {
        cerror(E_GET_NODE(cc->ast, elif->body[i])->common.span, "Failed to compile body of else if statement [if statement]\n");
        goto ERR;
      }
    }

    e = defer_emit_current_scope(cc);
    if (e < 0) goto ERR;

    defer_pop_scope(cc);

    scope_pop(cc);

    /* JMP over all other branches. */
    e = emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, end_label); // skip remaining elseifs and else
    if (e < 0) goto ERR;
  }

  /* Emit the final next in chain label for the else statement. */
  define_and_emit_label(cc, next_in_chain_label); // BAM!

  scope_push(cc);

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  /* ELSE BODY */
  int* else_body = E_GET_NODE(cc->ast, node)->if_stmt.else_body;
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nelse_stmts; i++) {
    e = compile(cc, else_body[i]);
    if (e < 0) {
      cerror(E_GET_NODE(cc->ast, else_body[i])->common.span, "Failed to compile body of else statement [if statement]\n");
      goto ERR;
    }
    /* No need to jump! we're already at the end :> */
  }

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  defer_pop_scope(cc);
  scope_pop(cc);

  /* END LABEL. There's still one instruction after this and it's to ensure we always pop our variables. */
  define_and_emit_label(cc, end_label);

  /* Pop scope. */

  return 0;

ERR:
  return e ? e : -1;
}

/**
 * Since we push frames before the condition and down to the body,
 * the overall stack impact of a while statement is 0.
 * However, since each LOAD/etc. will still increment
 * stack top, we store the stack top and restore it at the end.
 *
 * Bytecode emitted:
 * LABEL: Precondition
 * EVAL: Condition
 * JZ: End // Condition failed (==0), goto end.
 *
 * BODY:...
 *
 * EVAL: Deferred statements
 * JMP: Precondition
 * LABEL: End
 */
static ereg_t
compile_while_statement(e_compiler* cc, int node)
{
  int e = 0;

  /* Computes the condition and jumps to the end label (breaks) if condition is false */
  const u32 pre_condition_label = make_label_id(cc);

  /* After the while loop, with one POP_VARIABLES to ensure we always pop our variables. */
  const u32 end_label = make_label_id(cc);

  define_and_emit_label(cc, pre_condition_label);

  /* CONDITION */
  ereg_t cond = compile(cc, E_GET_NODE(cc->ast, node)->while_stmt.condition);
  if (cond < 0) goto ERR;

  // Break out of loop if condition is false.
  e = emit_and_record_jmp(cc, EIR_OPCODE_JZ, cond, end_label);
  if (e < 0) goto ERR;

  scope_push(cc);

  /**
   * Push frame for the stack
   */
  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  /* Append a loop entry to our compiler. */
  ecc_loop_location loop = {
    .continue_label = pre_condition_label,
    .break_label    = end_label,
    .defer_depth    = defer_get_current_depth(cc),
  };

  /* Ensure we don't overwrite it... */
  ecc_loop_location* last = cc->loop;
  cc->loop                = &loop;

  // WHILE BODY
  const int* while_body = E_GET_NODE(cc->ast, node)->while_stmt.stmts;
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->while_stmt.nstmts; i++) {
    e = compile(cc, while_body[i]);
    if (e < 0) goto ERR;
  }

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  /* Jump to condition, body is done executing */
  e = emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, pre_condition_label);
  if (e < 0) goto ERR;

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  // End label.
  define_and_emit_label(cc, end_label);

  // Pop the scope
  scope_pop(cc);
  defer_pop_scope(cc);

  // swap the old loop metadata back in
  cc->loop = last;

  return 0;

ERR:
  return e ? e : -1;
}

static ereg_t
compile_for_statement(e_compiler* cc, int node)
{
  int    initializers = -1;
  ereg_t cond_reg     = -1;

  /**
   * The for statement is compiled as:
   * Initializers (inlined)
   * LABEL: Precondition
   * EVAL: Condition
   * JZ: End
   *
   * BODY: ...
   *
   * LABEL: Iterators:
   * EVAL: Deferred statements
   * EVAL: Iterators
   * JMP: Precondition
   * LABEL: End
   */

  const u32 top_label      = make_label_id(cc);
  const u32 end_label      = make_label_id(cc);
  const u32 iterator_label = make_label_id(cc); // Needed so continue can jmp directly to iterators

  int e = 0;

  /* Append a loop entry to our compiler. */
  ecc_loop_location loop = {
    .continue_label = iterator_label,
    .break_label    = end_label,
    .defer_depth    = defer_get_current_depth(cc),
  };

  /* Ensure we don't overwrite it... */
  ecc_loop_location* last = cc->loop;
  cc->loop                = &loop;

  // See comment over while loop compilation
  scope_push(cc);

  // INITIALIZERS
  initializers = E_GET_NODE(cc->ast, node)->for_stmt.initializers;
  if (initializers >= 0 && compile(cc, initializers) < 0) {
    cerror(E_GET_NODE(cc->ast, initializers)->common.span, "Failed to compile initializers [for statement]\n");
    goto ERR;
  }

  // TOP_LABEL
  define_and_emit_label(cc, top_label);

  // CONDITION
  int cond = E_GET_NODE(cc->ast, node)->for_stmt.condition;
  cond_reg = compile(cc, cond);
  if (cond_reg < 0) {
    cerror(E_GET_NODE(cc->ast, cond_reg)->common.span, "Failed to compile condition [for statement]");
    goto ERR;
  }

  // JZ END_LABEL
  e = emit_and_record_jmp(cc, EIR_OPCODE_JZ, cond_reg, end_label);
  if (e < 0) goto ERR;

  // JZ pops condition variable

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  u32 old_depth = cc->loop->defer_depth;
  if (cc->loop) cc->loop->defer_depth = defer_get_current_depth(cc);

  // BODY
  u32  nstmts = E_GET_NODE(cc->ast, node)->for_stmt.nstmts;
  int* stmts  = E_GET_NODE(cc->ast, node)->for_stmt.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    if (compile(cc, stmts[i]) < 0) {
      cerror(E_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement in body [for statement]\n");
      goto ERR;
    }
  }

  /**
   * Deposit the deferred statements before the
   * iterator region so all iterator values are
   * correct.
   */
  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  defer_pop_scope(cc);

  if (cc->loop) cc->loop->defer_depth = old_depth;

  // ITERATOR_LABEL
  define_and_emit_label(cc, iterator_label);

  // ITERATORS
  u32  niterators = E_GET_NODE(cc->ast, node)->for_stmt.niterators;
  int* iterators  = E_GET_NODE(cc->ast, node)->for_stmt.iterators;
  for (u32 i = 0; i < niterators; i++) {
    if (compile(cc, iterators[i]) < 0) {
      cerror(E_GET_NODE(cc->ast, iterators[i])->common.span, "Failed to compile iterators [for statement]");
      goto ERR;
    }
  }

  /* Pop body scope */

  // JMP TOP_LABEL
  e = emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, top_label);
  if (e < 0) goto ERR;

  // END_LABEL
  define_and_emit_label(cc, end_label);

  scope_pop(cc);

  cc->loop = last;

  return 0;

ERR:
  /* ensure we don't leave a dangling reference to loop */
  cc->loop = last;
  return e ? e : -1;
}

static ereg_t
compile_member_access(e_compiler* cc, int node)
{
  if (!can_make_value(cc->ast, node)) {
    e_filespan span = E_GET_NODE(cc->ast, node)->common.span;
    cerror(span, "Failed to compile member access: Failed to lower to rvalue\n");
    return -1;
  }
  // Passthrough
  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  return emit_lvalue_load(cc, &lv);
}

static ereg_t
compile_assign(e_compiler* cc, int node)
{
  int right = E_GET_NODE(cc->ast, node)->assign.right;
  int left  = E_GET_NODE(cc->ast, node)->assign.left;

  if (!can_make_value(cc->ast, left)) {
    e_filespan left_span = E_GET_NODE(cc->ast, left)->common.span;
    cerror(left_span, "Can not assign to left: Failed to lower to lvalue\n");
    return -1;
  }

  val_t lv;
  int   e = value_init(cc, left, &lv);
  if (e < 0) return e;

  ecc_builtin_variables_table* builtin_vars_table = cc->builtin_var_table;

  ecc_var* exists = scope_lookup_info(cc, lv.val.var.id);

  if (!exists && E_GET_NODE(cc->ast, left)->type == E_AST_NODE_VARIABLE) {
    // Doesn't exist and node is supposed to be a variable (Not member access or index)

    /* Check if the user is trying to modify a builtin variable. */
    for (u32 i = 0; i < builtin_vars_table->builtin_vars_count; i++) {
      if (lv.val.var.id == builtin_vars_table->builtin_var_hashes[i]) {
        cerror(E_GET_NODE(cc->ast, left)->common.span, "Attempting to modify builtin constant '%s'\n", lv.val.var.name);
        value_free(&lv);
        return -1;
      }
    }

    cerror(E_GET_NODE(cc->ast, left)->common.span, "Undeclared variable '%s'\n", lv.val.var.name);
    value_free(&lv);
    return -1;
  }

  if (exists && exists->is_const) {
    cerror(E_GET_NODE(cc->ast, left)->common.span, "Can not assign to const qualified variable '%s'\n", lv.val.var.name);
    value_free(&lv);
    return -1;
  }

  ereg_t rreg = compile(cc, right);
  if (rreg < 0) return rreg;

  e = emit_lvalue_assign(cc, rreg, &lv);
  value_free(&lv);
  if (e < 0) return e;

  /* Propogate the RHS' register */
  return rreg;
}

static ereg_t
compile_member_assign(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->member_assign.value;

  if (!can_make_value(cc->ast, node)) {
    cerror(E_GET_NODE(cc->ast, node)->common.span, "Can not assign to member access: Failed to lower to lvalue\n");
    return -1;
  }

  ereg_t rhs = compile(cc, value);
  if (rhs < 0) { return rhs; }

  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  ereg_t propogate_rhs = emit_lvalue_assign(cc, rhs, &lv);
  value_free(&lv);

  return propogate_rhs;
}

static ereg_t
compile_return(e_compiler* cc, int node)
{
  int r = defer_emit_all_scopes(cc);
  if (r < 0) return r;

  if (E_GET_NODE(cc->ast, node)->ret.has_return_value) {
    int ret_node = E_GET_NODE(cc->ast, node)->ret.expr_id;
    /* Compile the return value */
    ereg_t rv = compile(cc, ret_node);
    if (rv < 0) {
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Failed to compile return value [return]\n");
      return rv;
    }

    e_emit_ins(cc, (e_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = rv } });
  } else {
    e_var  nil    = E_NULLVAR;
    ereg_t nilvar = compile_and_push_literal_variable(cc, &nil);
    if (nilvar < 0) return nilvar;

    e_emit_ins(cc, (e_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = nilvar } });
  }
  return r;
}

static int
append_struct_decleration(e_arena* a, const char* name, ecc_struct_information* deposit)
{
  u32 hash = e_hash(name, strlen(name));

  if (deposit->fields_count >= deposit->field_capacity || !deposit->field_hashes) {
    u32  field_cap_new    = MAX(deposit->field_capacity * 2, 4);
    u32* field_hashes_new = (u32*)realloc(deposit->field_hashes, field_cap_new * sizeof(u32));
    if (!field_hashes_new) { return -1; }

    char** field_names_new = (char**)realloc((void*)deposit->field_names, field_cap_new * sizeof(char*));
    if (!field_names_new) {
      free(field_hashes_new);
      return -1;
    }

    deposit->field_capacity = field_cap_new;
    deposit->field_hashes   = field_hashes_new;
    deposit->field_names    = field_names_new;
  }

  deposit->field_hashes[deposit->fields_count] = hash;
  deposit->field_names[deposit->fields_count]  = e_arnstrdup(a, name);

  deposit->fields_count++;

  return 0;
}

static int
collect_struct_declerations(e_compiler* cc, int* stmts, u32 nstmts, ecc_struct_information* deposit)
{
  for (u32 i = 0; i < nstmts; i++) {
    e_ast_node_type type = E_GET_NODE(cc->ast, stmts[i])->type;
    if (type == E_AST_NODE_STATEMENT_LIST) {
      int* list_stmts  = E_GET_NODE(cc->ast, stmts[i])->stmts.stmts;
      u32  list_nstmts = E_GET_NODE(cc->ast, stmts[i])->stmts.nstmts;
      // RECURSE!
      int e = collect_struct_declerations(cc, list_stmts, list_nstmts, deposit);
      if (e < 0) return e;
    } else if (type == E_AST_NODE_VARIABLE_DECL) {
      bool is_const = E_GET_NODE(cc->ast, stmts[i])->let.is_const;
      if (is_const) {
        cerror(E_GET_NODE(cc->ast, stmts[i])->let.span, "A member of a struct cannot be declared 'const' [unsupported]\n");
        return -1;
      }

      const char* name = E_GET_NODE(cc->ast, stmts[i])->let.name;
      int         e    = append_struct_decleration(cc->arena, name, deposit);
      if (e < 0) return e;
    } else {
      const char* member_name = E_GET_NODE(cc->ast, stmts[i])->let.name;
      cerror(E_GET_NODE(cc->ast, stmts[i])->let.span, "Member index %u, with name %s is not allowed in a struct\n", i, member_name);
      return -1;
    }
  }

  return 0;
}

static int
append_struct_info(e_compiler* cc, const ecc_struct_information* data)
{
  ecc_struct_table* table = cc->struct_table;
  if (table->structs_count >= table->structs_capacity) {
    u32                     new_capacity = MAX(table->structs_capacity * 2, 4);
    ecc_struct_information* new_structs  = realloc(table->structs, new_capacity * sizeof(ecc_struct_information));
    if (!new_structs) return -1;

    table->structs_capacity = new_capacity;
    table->structs          = new_structs;
  }

  memcpy(&table->structs[table->structs_count], data, sizeof(ecc_struct_information));
  table->structs_count++;

  return 0;
}

static ereg_t
compile_struct_constructor(e_compiler* fork, e_filespan span, const ecc_struct_information* struc)
{
  u32    struct_id = e_hash(struc->name, strlen(struc->name));
  ereg_t tmp       = vreg_alloc(fork);

  /* Setup each argument slot to move to a holding ground, which is moved to the register array */
  ereg_t* arg_slots_imitate = e_arnalloc(fork->arena, sizeof(ereg_t) * struc->fields_count);
  for (u32 i = 0; i < struc->fields_count; i++) {
    u32    hash          = struc->field_hashes[i];
    ereg_t reg           = scope_define(fork, span, hash, true); // let the compiler say it's constant
    arg_slots_imitate[i] = reg;

    // Copy from reg to our register array
    e_emit_ins(fork, (e_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = E_REG_ARG0 + i, .src = reg } });
  }

  e_emit_ins(fork, (e_ins){ .mk_struct = { .opcode = EIR_OPCODE_MK_STRUCT, .dst = tmp, .struct_id = struct_id } });

  /* Return from our temporary register */
  e_emit_ins(fork, (e_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = tmp } });

  ecc_function f = {
    .code       = fork->instructions,
    .code_count = fork->ninstructions,
    .arg_slots  = arg_slots_imitate,
    .name_hash  = struct_id,
    .nargs      = struc->fields_count,
  };

  int e = append_function_entry(fork->arena, fork->function_table, &f);
  if (e < 0) return e;

  return 0;
}

static ereg_t
compile_struct_decleration(e_compiler* cc, int node)
{
  int                    e           = 0;
  e_compiler             fork        = { 0 };
  ecc_struct_information struct_data = { 0 };

  const char* struct_name        = E_GET_NODE(cc->ast, node)->struct_decl.name;
  int*        struct_decl_stmts  = E_GET_NODE(cc->ast, node)->struct_decl.stmts;
  u32         struct_decl_nstmts = E_GET_NODE(cc->ast, node)->struct_decl.nstmts;

  /* intern structure name for debug symbols. */
  e_str_intern(struct_name, cc->ast->interner);

  /* Gather all information the user provided into one big structure. */
  struct_data = (ecc_struct_information){
    .name           = e_arnstrdup(cc->arena, struct_name),
    .name_hash      = e_hash(struct_name, strlen(struct_name)),
    .field_hashes   = (u32*)e_xalloc(init_fields_capacity, sizeof(u32)),
    .field_names    = (char**)e_xalloc(init_fields_capacity, sizeof(char**)),
    .field_capacity = init_fields_capacity,
    .fields_count   = 0,
  };
  if (!struct_data.field_hashes) goto ERR;

  e = collect_struct_declerations(cc, struct_decl_stmts, struct_decl_nstmts, &struct_data);
  if (e < 0) goto ERR;

  /* Generate the constructor function */
  e = compiler_make_fork(cc, &fork);
  if (e < 0) goto ERR;

  scope_push(&fork);

  e = defer_push_scope(&fork);
  if (e < 0) goto ERR;

  e = compile_struct_constructor(&fork, E_GET_NODE(cc->ast, node)->common.span, &struct_data);
  if (e < 0) goto ERR;

  e = append_struct_info(cc, &struct_data);
  if (e < 0) goto ERR;

  defer_pop_scope(&fork);
  scope_pop(&fork);

  compiler_join_fork(&fork, cc);

  return 0;

ERR:
  if (struct_data.field_hashes) free(struct_data.field_hashes);
  if (struct_data.field_names) free((void*)struct_data.field_names);
  compiler_free_fork_entirely(&fork);
  return e ? e : -1;
}

static ereg_t
compile_variable_decleration(e_compiler* cc, int node)
{
  const char* name = E_GET_NODE(cc->ast, node)->let.name;
  char*       full = qualify_name(cc, name);
  if (!full) return -1;

  // fprintf(stderr, "declared: %s\n", full);

  u32 hash        = e_hash(full, strlen(full));
  int initializer = E_GET_NODE(cc->ast, node)->let.initializer;

  ecc_var* v = cc->scope->vars;
  while (v) {
    if (v->name_hash == hash) {
      cerror(v->span, "Variable redeclared in same scope\n");
      return -1;
    }
    v = v->next;
  }

  /* Add variable entry to stack */

  const bool initializer_provided = initializer >= 0;

  e_filespan span     = E_GET_NODE(cc->ast, node)->let.span;
  bool       is_const = E_GET_NODE(cc->ast, node)->let.is_const;

  /* Define the variable */
  ereg_t new_var = scope_define(cc, span, hash, is_const);

  if (initializer_provided) {
    /* need to use the lval system for handling global variable declerations... */
    val_t lv = { 0 };
    if (value_init(cc, node, &lv) < 0) return -1;

    ereg_t init_reg = compile(cc, initializer);
    if (init_reg < 0) return init_reg;

    if (emit_lvalue_assign(cc, init_reg, &lv) < 0) return -1;
  }

  return new_var;
}

static ereg_t
compile_root(e_compiler* cc, int node)
{
  /**
   * Initializes all global variables and namespaced
   * variables.
   *
   * Shouldn't take lots of time if the user hasn't
   * done something extremely stupid.
   * AKA. initializing a global variable to the
   * value of a non trivial function call.
   */
  e_ast_node* root = E_GET_NODE(cc->ast, node);
  for (u32 i = 0; i < root->root.nstmts; i++) {
    int e = compile(cc, root->root.stmts[i]);
    if (e < 0) {
      cerror(E_GET_NODE(cc->ast, root->root.stmts[i])->common.span, "Failed to compile root, bailing out [root]\n");
      return e;
    }
  }

  /**
   * Done with initialization!
   * Now we return from here
   * and expect the interpreter to jump
   * to the entry point
   */
  e_var  v      = E_NULLVAR;
  ereg_t nilvar = compile_and_push_literal_variable(cc, &v);
  e_emit_ins(cc, (e_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = nilvar } });
  return 0; // Done!
}

static ereg_t
compile_statement_list(e_compiler* cc, int node)
{
  e_ast_node* nodep  = E_GET_NODE(cc->ast, node);
  const int*  stmts  = nodep->stmts.stmts;
  u32         nstmts = nodep->stmts.nstmts;

  for (u32 i = 0; i < nstmts; i++) {
    int e = compile(cc, stmts[i]);
    if (e < 0) {
      cerror(E_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement [statement list]\n");
      return e;
    }
  }

  return 0;
}

static ereg_t
compile_variable_load(e_compiler* cc, int node)
{
  char* full = qualify_name(cc, E_GET_NODE(cc->ast, node)->ident.ident);
  u32   hash = e_hash(full, strlen(full));

  for (u32 i = 0; i < cc->builtin_var_table->builtin_vars_count; i++) {
    const e_builtin_var* builtin_var      = &cc->builtin_var_table->builtin_vars[i];
    u32                  builtin_var_hash = cc->builtin_var_table->builtin_var_hashes[i];

    if (builtin_var_hash == hash) {
      /* Instantiate a builtin variable only if it is used. */
      e_var v = {
        .type = builtin_var->type,
        .val  = builtin_var->value,
      };

      return compile_and_push_literal_variable(cc, &v); // compile_literal_variable loads the value! Return.
    }
  }

  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  ereg_t dst = emit_lvalue_load(cc, &lv);
  if (e < 0) {
    value_free(&lv);
    return -1;
  }

  value_free(&lv);
  return dst;
}

static ereg_t
compile_namespace_decleration(e_compiler* cc, int node)
{
  const char* ns_name        = E_GET_NODE(cc->ast, node)->namespace_decl.name;
  int*        ns_decl_stmts  = E_GET_NODE(cc->ast, node)->namespace_decl.stmts;
  u32         ns_decl_nstmts = E_GET_NODE(cc->ast, node)->namespace_decl.nstmts;

  int e = ns_push(cc, ns_name);
  if (e < 0) return e;

  for (u32 i = 0; i < ns_decl_nstmts; i++) {
    e = compile(cc, ns_decl_stmts[i]);
    if (e < 0) {
      cerror(E_GET_NODE(cc->ast, ns_decl_stmts[i])->common.span, "Failed to compile namespace decleration [namespace decleration]\n");
      ns_pop(cc);
      return e;
    }
  }

  ns_pop(cc);

  return 0;
}

static ereg_t
compile_function(e_compiler* cc, int node)
{
  u32  nstmts = E_GET_NODE(cc->ast, node)->func.nstmts;
  int* stmts  = E_GET_NODE(cc->ast, node)->func.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    if (compile(cc, stmts[i]) < 0) return -1;

    /**
     * OPTIMIZATION: If we're in the function stream,
     * and a node pushes a value to the stack that we do not want,
     * pop it.
     */
    // if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, stmts[i]);
  }
  return 0;
}

static ereg_t
compile_builtin_structure(e_compiler* cc, const e_builtin_struct* b)
{
  e_compiler fork = { 0 };
  int        e    = 0;

  /* intern its name for debug symbols. */
  e_str_intern(b->name, cc->ast->interner);

  ecc_struct_information st = {
    .name           = e_arnstrdup(cc->arena, b->name),
    .name_hash      = e_hash(b->name, strlen(b->name)),
    .field_hashes   = e_xalloc(b->fields_count, sizeof(u32)),
    .field_names    = (char**)e_xalloc(b->fields_count, sizeof(char*)),
    .fields_count   = b->fields_count,
    .field_capacity = b->fields_count,
  };
  if (!st.field_hashes || !st.field_names) { goto ERR; }

  for (u32 j = 0; j < b->fields_count; j++) {
    st.field_hashes[j] = e_hash(b->fields[j], strlen(b->fields[j]));
    st.field_names[j]  = e_arnstrdup(cc->arena, b->fields[j]);
  }

  /**
   * We don't need to push or pop stack frames here!
   * This function just recurses to compile_struct_constructor
   * which just emits the bytecode to form the structure.
   */

  /* Generate the constructor function */
  e = compiler_make_fork(cc, &fork);
  if (e < 0) goto ERR;

  e = compile_struct_constructor(&fork, E_GET_NODE(cc->ast, cc->ast->root)->common.span, &st);
  if (e < 0) goto ERR;

  compiler_join_fork(&fork, cc);

  e = append_struct_info(cc, &st);
  if (e < 0) goto ERR;

  return 0;

ERR:
  free(st.field_hashes);
  free((void*)st.field_names);
  compiler_free_fork_entirely(&fork);
  return e >= 0 ? -1 : e;
}

/**
 * Load all builtin structures, even if they
 * aren't used. We can not safely say a structure
 * is used or not before compilation.
 * (Compiling the structurs only when they are seems
 *  to work, but will cause issues later, like with
 *  dynamic script execution).
 */
static ereg_t
compile_builtin_structures(e_compiler* cc)
{
  for (u32 i = 0; i < E_ARRLEN(eb_structs); i++) {
    const e_builtin_struct* b = &eb_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e < 0) return e;
  }

  /**
   * Compile all hooked builtin structures.
   */
  for (u32 i = 0; i < cc->info->nhooked_structs; i++) {
    const e_builtin_struct* b = &cc->info->hook_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e < 0) return e;
  }

  return 0;
}

static inline int
fold(e_compiler* cc, int node)
{
  int e = 0;

  e_ast_node* nodep = e_ast_get_node(cc->ast, node);
  switch (nodep->type) {
    case E_AST_NODE_BINARYOP: {
      int l = nodep->binaryop.left;
      int r = nodep->binaryop.right;

      e = fold(cc, l);
      if (e < 0) return e;

      e = fold(cc, r);
      if (e < 0) return e;

      if (!is_literal_value(cc->ast, l) || !is_literal_value(cc->ast, r)) return 0;

      e_var lv;
      e_var rv;

      e = convert_node_to_literal(cc, l, &lv);
      if (e < 0) return e;

      e = convert_node_to_literal(cc, r, &rv);
      if (e < 0) return e;

      e_var result = operate(lv, rv, e_binary_operator_to_opcode(E_GET_NODE(cc->ast, node)->binaryop.op));

      e = convert_literal_to_node(cc->ast, node, &result);
      if (e < 0) return e;
      break;
    }

    case E_AST_NODE_ROOT: {
      for (u32 i = 0; i < nodep->root.nstmts; i++) {
        e = fold(cc, nodep->root.stmts[i]);
        if (e < 0) return e;
      }
      return 0;
    }
    case E_AST_NODE_STATEMENT_LIST: {
      for (u32 i = 0; i < nodep->stmts.nstmts; i++) {
        e = fold(cc, nodep->stmts.stmts[i]);
        if (e < 0) return e;
      }
      return 0;
    }
    case E_AST_NODE_DEFER: {
      for (u32 i = 0; i < nodep->defer.nstmts; i++) {
        e = fold(cc, nodep->defer.stmts[i]);
        if (e < 0) return e;
      }
      return 0;
    }

    case E_AST_NODE_FUNCTION_DEFINITION: {
      for (u32 i = 0; i < nodep->func.nstmts; i++) {
        e = fold(cc, nodep->func.stmts[i]);
        if (e < 0) return e;
      }
      /* Can not optimize arguments in function definition. CALL should do that. */
      return 0;
    }

    case E_AST_NODE_NAMESPACE_DECL: {
      for (u32 i = 0; i < nodep->namespace_decl.nstmts; i++) {
        e = fold(cc, nodep->namespace_decl.stmts[i]);
        if (e < 0) return e;
      }
      return 0;
    }

    case E_AST_NODE_CALL: {
      for (u32 i = 0; i < nodep->call.nargs; i++) {
        e = fold(cc, nodep->call.args[i]);
        if (e < 0) return e;
      }
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      if (nodep->let.initializer >= 0) {
        e = fold(cc, nodep->let.initializer);
        if (e < 0) return e;
      }
      return 0;
    }

    case E_AST_NODE_ASSIGN: {
      e = fold(cc, nodep->assign.left);
      if (e < 0) return e;
      e = fold(cc, nodep->assign.right);
      return e;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      e = fold(cc, nodep->member_assign.left);
      if (e < 0) return e;
      e = fold(cc, nodep->member_assign.value);
      return e;
    }

    case E_AST_NODE_INDEX: {
      e = fold(cc, nodep->index.index);
      return 0;
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      e = fold(cc, nodep->index_assign.index);
      e = fold(cc, nodep->index_assign.value);
      return 0;
    }

    case E_AST_NODE_FOR: {
      e = fold(cc, nodep->for_stmt.condition);
      if (e < 0) return e;

      e = fold(cc, nodep->for_stmt.initializers);
      if (e < 0) return e;

      for (u32 i = 0; i < nodep->for_stmt.nstmts; i++) {
        e = fold(cc, nodep->for_stmt.stmts[i]);
        if (e < 0) return e;
      }

      for (u32 i = 0; i < nodep->for_stmt.niterators; i++) {
        e = fold(cc, nodep->for_stmt.iterators[i]);
        if (e < 0) return e;
      }
      return 0;
    }

    case E_AST_NODE_WHILE: {
      for (u32 i = 0; i < nodep->while_stmt.nstmts; i++) {
        e = fold(cc, nodep->while_stmt.stmts[i]);
        if (e < 0) return e;
      }

      e = fold(cc, nodep->while_stmt.condition);
      if (e < 0) return e;

      return 0;
    }

    case E_AST_NODE_IF: {
      e = fold(cc, nodep->if_stmt.condition);
      if (e < 0) return e;

      for (u32 i = 0; i < nodep->if_stmt.nstmts; i++) {
        e = fold(cc, nodep->if_stmt.body[i]);
        if (e < 0) return e;
      }

      u32 nelse_ifs = nodep->if_stmt.nelse_ifs;
      for (u32 i = 0; i < nelse_ifs; i++) {
        struct e_if_stmt* else_if = &nodep->if_stmt.else_ifs[i];

        e = fold(cc, else_if->condition);
        if (e < 0) return e;

        for (u32 j = 0; j < else_if->nstmts; j++) {
          e = fold(cc, else_if->body[j]);
          if (e < 0) return e;
        }
      }

      for (u32 i = 0; i < nodep->if_stmt.nelse_stmts; i++) {
        e = fold(cc, nodep->if_stmt.else_body[i]);
        if (e < 0) return e;
      }

      return 0;
    }

    default: break;
  }

  return 0;
}

/* Register ID on success, <0 on error */
static int
compile(e_compiler* cc, int node)
{
  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_AST_NODE_NOP: return 0;
    case E_AST_NODE_INT:
    case E_AST_NODE_FLOAT:
    case E_AST_NODE_BOOL:
    case E_AST_NODE_CHAR:
    case E_AST_NODE_STRING: return compile_literal(cc, node);
    case E_AST_NODE_ROOT: return compile_root(cc, node);
    case E_AST_NODE_STATEMENT_LIST: return compile_statement_list(cc, node);
    case E_AST_NODE_FUNCTION_DEFINITION: return compile_function_definition(cc, node);
    case E_AST_NODE_LIST: return compile_list(cc, node);
    case E_AST_NODE_MAP: return compile_map(cc, node);
    case E_AST_NODE_VARIABLE_DECL: return compile_variable_decleration(cc, node);
    case E_AST_NODE_BINARYOP: return compile_binary_op(cc, node);
    case E_AST_NODE_UNARYOP: return compile_unary_op(cc, node);
    case E_AST_NODE_RETURN: return compile_return(cc, node);
    case E_AST_NODE_VARIABLE: return compile_variable_load(cc, node);
    case E_AST_NODE_INDEX: return compile_index(cc, node);
    case E_AST_NODE_INDEX_ASSIGN: return compile_index_assign(cc, node);
    case E_AST_NODE_ASSIGN: return compile_assign(cc, node);
    case E_AST_NODE_CALL: return compile_function_call(cc, node);
    case E_AST_NODE_FOR: return compile_for_statement(cc, node);
    case E_AST_NODE_WHILE: return compile_while_statement(cc, node);
    case E_AST_NODE_IF: return compile_if_statement(cc, node);
    case E_AST_NODE_STRUCT_DECL: return compile_struct_decleration(cc, node);
    case E_AST_NODE_MEMBER_ACCESS: return compile_member_access(cc, node);
    case E_AST_NODE_MEMBER_ASSIGN: return compile_member_assign(cc, node);
    case E_AST_NODE_NAMESPACE_DECL: return compile_namespace_decleration(cc, node);
    case E_AST_NODE_DEFER: {
      int* stmts  = E_GET_NODE(cc->ast, node)->defer.stmts;
      u32  nstmts = E_GET_NODE(cc->ast, node)->defer.nstmts;
      return append_defer_entry(cc, stmts, nstmts);
    }
    case E_AST_NODE_BREAK: {
      if (!cc->loop) {
        e_filespan span = E_GET_NODE(cc->ast, node)->common.span;
        cerror(span, "break used outside a loop\n");
        return -1;
      }

      if (defer_emit_to_depth(cc, cc->loop->defer_depth) < 0) return -1;

      u32 target = cc->loop->break_label;
      return emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, target);
    }
    case E_AST_NODE_CONTINUE: {
      if (!cc->loop) {
        e_filespan span = E_GET_NODE(cc->ast, node)->common.span;
        cerror(span, "continue used outside a loop\n");
        return -1;
      }

      if (defer_emit_to_depth(cc, cc->loop->defer_depth) < 0) return -1;

      u32 target = cc->loop->continue_label;
      return emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, target);
    }

    default: fprintf(stderr, "%i\n", E_GET_NODE(cc->ast, node)->type); assert(0);
  }
}

int
e_compile(const ecc_info* info, e_compilation_result* result)
{
  int e = 0;

  ecc_namespace_stack         namespace_stack   = { 0 };
  ecc_literal_table           lit_table         = { 0 };
  ecc_builtin_variables_table builtin_var_table = { 0 };
  ecc_function_table          func_table        = { 0 };
  ecc_label_table             label_table       = { 0 };
  ecc_struct_table            struct_table      = { 0 };
  e_compiler                  cc                = { 0 };

  namespace_stack = (ecc_namespace_stack){
    .namespaces  = (char**)e_xalloc(init_namespaces_capacity, sizeof(char*)),
    .nnamespaces = 0,
    .capacity    = init_namespaces_capacity,
  };
  if (!namespace_stack.namespaces) goto ERR;

  /**
   * Compiler's builtin variables and the hooked variables combined.
   */
  u32 total_builtin_variable_count = E_ARRLEN(eb_vars) + info->nhooked_vars;

  u32*           builtin_variable_hashes = (u32*)e_arnalloc(info->arena, sizeof(u32) * total_builtin_variable_count);
  e_builtin_var* builtin_variables       = (e_builtin_var*)e_arnalloc(info->arena, sizeof(e_builtin_var) * total_builtin_variable_count);
  if (!builtin_variable_hashes) goto ERR;
  if (!builtin_variables) goto ERR;

  /**
   * Load up every hash and variable into the arrays.
   * First, the compiler's builtins, and then the hooked variables.
   * This ensures the compiler's definitions are seen earlier
   * than the later ones, preventing overshadowing of primitive types.
   */
  u32 builtin_var_ctr = 0;
  for (u32 i = 0; i < E_ARRLEN(eb_vars); i++, builtin_var_ctr++) {
    builtin_variable_hashes[builtin_var_ctr] = e_hash(eb_vars[i].name, strlen(eb_vars[i].name));
    memcpy(&builtin_variables[builtin_var_ctr], &eb_vars[i], sizeof(e_builtin_var));
  }
  for (u32 i = 0; i < info->nhooked_vars; i++, builtin_var_ctr++) {
    builtin_variable_hashes[builtin_var_ctr] = e_hash(info->hook_vars[i].name, strlen(info->hook_vars[i].name));
    memcpy(&builtin_variables[builtin_var_ctr], &info->hook_vars[i], sizeof(e_builtin_var));
  }

  lit_table = (ecc_literal_table){
    .literals          = (e_var*)e_xalloc(init_literal_capacity, sizeof(e_var)),
    .literal_hashes    = (u32*)e_xalloc(init_literal_capacity, sizeof(u32)),
    .literals_count    = 0,
    .literals_capacity = init_literal_capacity,
  };
  if (!lit_table.literals || !lit_table.literal_hashes) {
    e = -1;
    goto ERR;
  }

  builtin_var_table = (ecc_builtin_variables_table){
    .builtin_vars       = builtin_variables,
    .builtin_var_hashes = builtin_variable_hashes,
    .builtin_vars_count = total_builtin_variable_count,
  };

  func_table = (ecc_function_table){
    .functions          = e_xalloc(init_function_capacity, sizeof(ecc_function)),
    .functions_capacity = init_function_capacity,
    .functions_count    = 0,
  };
  if (!func_table.functions) {
    e = -1;
    goto ERR;
  }

  label_table = (ecc_label_table){
    .labels_count    = 0,
    .labels_capacity = init_label_jump_capacity,
    .labels          = e_xalloc(init_label_jump_capacity, sizeof(ecc_label_jumps_table)),
  };
  if (!label_table.labels) {
    e = -1;
    goto ERR;
  }

  struct_table = (ecc_struct_table){
    .structs_count    = 0,
    .structs_capacity = init_structs_capacity,
    .structs          = e_xalloc(init_structs_capacity, sizeof(ecc_struct_information)),
  };

  cc = (e_compiler){
    .arena             = info->arena,
    .ast               = info->ast,
    .info              = info,
    .loop              = NULL,
    .ns                = &namespace_stack,
    .lit_table         = &lit_table,
    .builtin_var_table = &builtin_var_table,
    .function_table    = &func_table,
    .label_table       = &label_table,
    .struct_table      = &struct_table,
    .next_reg          = E_REG_GENERAL_BEGIN,
    .instructions      = (e_ins*)e_xalloc(sizeof(e_ins), init_code_capacity),
    .ninstructions     = 0,
    .cinstructions     = init_code_capacity,
  };
  if (!cc.instructions) return -1;

  scope_push(&cc);

  /* First of all, call the optimization routines (if requested) */
  if (info->opt_level >= 2) {
    e = fold(&cc, cc.ast->root);
    if (e < 0) goto ERR;
  }

  e = defer_push_scope(&cc);
  if (e < 0) goto ERR;

  /**
   * Generate constructors for all builtin && hooked
   * structures.
   */
  e = compile_builtin_structures(&cc);
  if (e < 0) goto ERR;

  e = compile(&cc, info->root_node);
  if (e < 0) goto ERR;

  defer_pop_scope(&cc);

  if (result) {
    result->literals           = cc.lit_table->literals;
    result->literals_count     = cc.lit_table->literals_count;
    result->literals_hashes    = cc.lit_table->literal_hashes;
    result->functions          = func_table.functions;
    result->functions_count    = func_table.functions_count;
    result->instructions_count = cc.ninstructions;
    result->instructions       = cc.instructions;
    result->structs_count      = cc.struct_table->structs_count;
    result->structs            = cc.struct_table->structs;

    u32          strings_count = cc.ast->interner->strings_count;
    const char** strings       = (const char**)cc.ast->interner->strings;

    result->names_count = strings_count;

    result->names_hashes = (u32*)e_xalloc(strings_count, sizeof(u32));
    if (!result->names_hashes) goto ERR;

    result->names = (char**)e_xalloc(strings_count, sizeof(char*));
    if (!result->names) goto ERR;

    for (u32 i = 0; i < strings_count; i++) {
      result->names[i] = e_strdup(strings[i]);
      if (!result->names[i]) { goto ERR; }
    }
  }

  for (u32 i = 0; i < label_table.labels_count; i++) { free(label_table.labels[i].jumps_target_offsets); }
  e_xfree((void**)&label_table.labels);
  e_xfree((void**)&namespace_stack.namespaces);
  // scope_pop(&cc);

  return e;

ERR: /* Seperate from successful return path. We free everything here, indiscriminately. */
  while (cc.defer_stack) defer_pop_scope(&cc);
  for (u32 i = 0; i < struct_table.structs_count; i++) {
    free((void*)struct_table.structs[i].field_names);
    free(struct_table.structs[i].field_hashes);
  }
  free(struct_table.structs);
  free((void*)namespace_stack.namespaces);
  free(lit_table.literal_hashes);
  free(lit_table.literals);
  for (u32 i = 0; i < func_table.functions_count; i++) {
    // arg_slots is arena allocated
    free(func_table.functions[i].code);
  }
  free(func_table.functions);
  for (u32 i = 0; i < label_table.labels_count; i++) free(label_table.labels[i].jumps_target_offsets);
  free(label_table.labels);
  free(cc.instructions);
  return e ? e : -1;
}