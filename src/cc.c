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
#include "../inc/bc.h"
#include "../inc/bfunc.h"
#include "../inc/bstructs.h"
#include "../inc/bvar.h"
#include "../inc/cerr.h"
#include "../inc/operate.h"
#include "../inc/pool.h"
#include "../inc/rwhelp.h"
#include "../inc/stackemu.h"
#include "../inc/stdafx.h"
#include "../inc/strint.h"
#include "../inc/var.h"

#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
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

/**
 * Operators like SUB (-) can be used
 * as unary operators and that would add confusion
 * to the parser.
 * Only supports binary operators.
 * Unary operators must be handled seperately.
 */
static inline e_opcode
e_binary_operator_to_opcode(e_operator op)
{
  switch (op) {
    case E_OPERATOR_ADD: return E_OPCODE_ADD;
    case E_OPERATOR_SUB: return E_OPCODE_SUB;
    case E_OPERATOR_MUL: return E_OPCODE_MUL;
    case E_OPERATOR_DIV: return E_OPCODE_DIV;
    case E_OPERATOR_MOD: return E_OPCODE_MOD;
    case E_OPERATOR_EXP: return E_OPCODE_EXP;
    case E_OPERATOR_AND: return E_OPCODE_AND;
    case E_OPERATOR_OR: return E_OPCODE_OR;
    case E_OPERATOR_BAND: return E_OPCODE_BAND;
    case E_OPERATOR_BOR: return E_OPCODE_BOR;
    case E_OPERATOR_XOR: return E_OPCODE_XOR;
    case E_OPERATOR_ISEQL: return E_OPCODE_EQL;
    case E_OPERATOR_ISNEQ: return E_OPCODE_NEQ;
    case E_OPERATOR_LT: return E_OPCODE_LT;
    case E_OPERATOR_LTE: return E_OPCODE_LTE;
    case E_OPERATOR_GT: return E_OPCODE_GT;
    case E_OPERATOR_GTE: return E_OPCODE_GTE;
    case E_OPERATOR_NOT: return E_OPCODE_NOT;
    case E_OPERATOR_BNOT: return E_OPCODE_BNOT;
    case E_OPERATOR_DEC: return E_OPCODE_DEC;
    case E_OPERATOR_INC: return E_OPCODE_INC;
  }
  return -1;
}

static ATTR_NODISCARD char* qualify_name(const e_compiler* cc, const char* name);

static inline RETURNS_ERRCODE int defer_push_scope(e_compiler* cc);
static inline void                defer_pop_scope(e_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_current_scope(e_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_all_scopes(e_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_to_depth(e_compiler* cc, u32 target_depth);
static inline u32                 defer_get_current_depth(e_compiler* cc);

static inline ATTR_NODISCARD u32  label_find(u32 label_id, const ecc_label_table* table);
static inline RETURNS_ERRCODE int emit_and_record_jmp(e_compiler* cc, e_opcode opcode, u32 label_id);
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

static RETURNS_ERRCODE int        value_init(e_compiler* cc, int node, struct val_t* d);
static void                       value_free(struct val_t* lv);
static RETURNS_ERRCODE int        emit_lvalue_load(e_compiler* cc, struct val_t lv);
static RETURNS_ERRCODE int        emit_lvalue_assign(e_compiler* cc, int value, struct val_t lv);
static inline RETURNS_ERRCODE int emit_lvalue_assign_prologue(e_compiler* cc, struct val_t lv);
static inline RETURNS_ERRCODE int emit_lvalue_assign_epilogue(e_compiler* cc, struct val_t lv);

static RETURNS_ERRCODE int compile_and_push_literal_variable(e_compiler* cc, e_var v);
static RETURNS_ERRCODE int compile_literal(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_list(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_map(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_function_definition(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_binary_op(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_unary_op(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_index(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_index_assign(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_index_compound(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_function_call(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_if_statement(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_while_statement(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_for_statement(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_member_access(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_assign(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_return(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_struct_constructor(e_compiler* fork, e_filespan span, const ecc_struct_information* struc);
static RETURNS_ERRCODE int compile_struct_decleration(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_variable_decleration(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_variable_load(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_namespace_decleration(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_builtin_structure(e_compiler* cc, const e_builtin_struct* b);
static RETURNS_ERRCODE int compile_builtin_structures(e_compiler* cc);
static RETURNS_ERRCODE int compile_function(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile_root(e_compiler* cc, int node);
static RETURNS_ERRCODE int compile(e_compiler* cc, int node);

typedef enum e_lval_type {
  E_LVAL_VAR,
  E_LVAL_MEMBER,  // struct member
  E_LVAL_INDEX,   // indexed array
  E_LVAL_UNKNOWN, // Error!
} lval_type;

typedef union e_lval_value {
  struct {
    u32   id;
    char* name; // allocated
  } var;
  struct {
    int         base;
    const char* member;
  } member;
  struct {
    int left_node;  // Compile to get LHS of index. For vec[16], it will push vec to stack.
    int index_node; // Compile to get index. For vec[16], it will push 16 to stack.
  } index;
} lval_value;

typedef struct val_t {
  e_filespan* span;
  lval_type   type;
  lval_value  val;
  bool        is_compound;
  e_operator  compound_operator;
} val_t;

static inline bool
can_make_value(const e_ast* ast, int node)
{
  if (ast == nullptr || node < 0) return false;
  return E_GET_NODE(ast, node)->type == E_AST_NODE_VARIABLE || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX
      || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX_ASSIGN || E_GET_NODE(ast, node)->type == E_AST_NODE_INDEX_COMPOUND_OP
      || E_GET_NODE(ast, node)->type == E_AST_NODE_MEMBER_ACCESS || E_GET_NODE(ast, node)->type == E_AST_NODE_MEMBER_ASSIGN
      || E_GET_NODE(ast, node)->type == E_AST_NODE_VARIABLE_DECL;
}

static inline ATTR_NODISCARD bool
does_node_push_to_stack(const e_compiler* cc, int node)
{
  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_AST_NODE_NOP:
    case E_AST_NODE_ROOT:
    case E_AST_NODE_STATEMENT_LIST:
    case E_AST_NODE_FOR:
    case E_AST_NODE_WHILE:
    case E_AST_NODE_BREAK:
    case E_AST_NODE_CONTINUE:
    case E_AST_NODE_ASSERT: // pushes and immediately pops
    case E_AST_NODE_IF:
    case E_AST_NODE_NAMESPACE_DECL:
    case E_AST_NODE_RETURN: /* No need to optimize this! */
    case E_AST_NODE_DEFER:  /* Doesn't push anything: statements inside do (which are handled) */
    case E_AST_NODE_FUNCTION_DEFINITION:
    case E_AST_NODE_STRUCT_DECL: return false;

    case E_AST_NODE_VARIABLE_DECL: {
      return E_GET_NODE(cc->ast, node)->let.initializer >= 0; // If initializer is valid, then it pushes it to the stack.
    }

    case E_AST_NODE_CALL:
    case E_AST_NODE_BINARYOP:
    case E_AST_NODE_UNARYOP:
    case E_AST_NODE_VARIABLE:
    case E_AST_NODE_ASSIGN:
    case E_AST_NODE_INDEX:
    case E_AST_NODE_INDEX_ASSIGN:
    case E_AST_NODE_INDEX_COMPOUND_OP:
    case E_AST_NODE_MEMBER_ACCESS:
    case E_AST_NODE_MEMBER_ASSIGN:
    case E_AST_NODE_INT:
    case E_AST_NODE_CHAR:
    case E_AST_NODE_BOOL:
    case E_AST_NODE_STRING:
    case E_AST_NODE_FLOAT:
    case E_AST_NODE_LIST:
    case E_AST_NODE_MAP: return true;
  }
  return false;
}

/**
 * If the node pushes a value to the stack,
 * pop it. Only use this in places where you're sure their
 * values will not be used (like inside body of if statements, etc.).
 */
static inline void
pop_value_if_pushes(e_compiler* cc, int node)
{
  if (does_node_push_to_stack(cc, node)) { e_emit_instruction(cc, E_OPCODE_POP); }
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
      if (e) return e;

      if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, exprs[i]);
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
        if (e) {
          cerror(E_GET_NODE(cc->ast, exprs[j])->common.span, "Failed to compile deferred statement [defer]\n");
          return e;
        }

        if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, exprs[i]);
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
        if (e) return e;

        if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, scope->entries[i].exprs[j]);
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
    if (!new_table) return nullptr;

    table->labels          = new_table;
    table->labels_capacity = new_c;
  }

  u32 index = label_find(label_id, table);

  ecc_label_jumps_table* end = nullptr;
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
emit_and_record_jmp(e_compiler* cc, e_opcode opcode, u32 label_id)
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

  e_emit_instruction(cc, opcode);

  // Need to align it to 4 bytes.. (sizeof(u32)->label ID)
  u32 patch_offset = e_align_size(cc->num_bytes_emitted, 4);

  if (label->defined) {
    /**
     * Label already defined. Just point out jump to
     * the label.
     */
    e_emit_u32(cc, label->label_offset);
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
    e_emit_u32(cc, will_patch_later);
  }

  return 0;
}

static inline void
define_and_emit_label(e_compiler* cc, u32 label_id)
{
  e_emit_u8(cc, E_OPCODE_LABEL);
  e_emit_u32(cc, label_id);

  u32 destination_offset = cc->num_bytes_emitted;

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
    u32 patch_offset = label->jumps_target_offsets[i];
    memcpy(cc->emit + patch_offset, &destination_offset, sizeof(u32));
  }

  // patched every jump up.
  label->jumps_count = 0;
}

static inline RETURNS_ERRCODE int
compiler_make_fork(const e_compiler* old_c, e_compiler* new_c)
{
  *new_c = (e_compiler){
    .arena             = old_c->arena,
    .ast               = old_c->ast,
    .info              = old_c->info,
    .loop              = nullptr, // reset loop on function.
    .lit_table         = old_c->lit_table,
    .builtin_var_table = old_c->builtin_var_table,
    .function_table    = old_c->function_table,
    .label_table       = old_c->label_table,
    .struct_table      = old_c->struct_table,
    .next_label        = old_c->next_label,
    .ns                = old_c->ns,
    .stack             = old_c->stack,
    .emit              = (u8*)e_xalloc(1, init_code_capacity),
    .num_bytes_emitted = 0,
    .emit_capacity     = init_code_capacity,
  };
  return new_c->emit ? 0 : -1;
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
  free(cc->emit);
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

      l.span         = &E_GET_NODE(cc->ast, node)->common.span;
      l.type         = E_LVAL_VAR;
      l.val.var.id   = e_hash(name, strlen(name));
      l.val.var.name = name;

      *d = l;
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      char* name = qualify_name(cc, E_GET_NODE(cc->ast, node)->let.name);

      l.span         = &E_GET_NODE(cc->ast, node)->common.span;
      l.type         = E_LVAL_VAR;
      l.val.var.id   = e_hash(name, strlen(name));
      l.val.var.name = name;
      *d             = l;
      return 0;
    }

    case E_AST_NODE_INDEX: {
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                 = E_LVAL_INDEX;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index.index;
      l.is_compound          = false;
      *d                     = l;
      return 0;
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.type                 = E_LVAL_INDEX;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index_assign.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index_assign.index;
      l.is_compound          = false;
      *d                     = l;
      return 0;
    }

    case E_AST_NODE_INDEX_COMPOUND_OP: {
      l.type                 = E_LVAL_INDEX;
      l.span                 = &E_GET_NODE(cc->ast, node)->common.span;
      l.val.index.left_node  = E_GET_NODE(cc->ast, node)->index_compound.base;
      l.val.index.index_node = E_GET_NODE(cc->ast, node)->index_compound.index;
      l.compound_operator    = E_GET_NODE(cc->ast, node)->index_compound.op;
      l.is_compound          = true;
      *d                     = l;
      return 0;
    }

    case E_AST_NODE_MEMBER_ACCESS: {
      l.span              = &E_GET_NODE(cc->ast, node)->common.span;
      l.type              = E_LVAL_MEMBER;
      l.val.member.base   = E_GET_NODE(cc->ast, node)->member_access.left;
      l.val.member.member = E_GET_NODE(cc->ast, node)->member_access.right;
      *d                  = l;
      return 0;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      l.type              = E_LVAL_MEMBER;
      l.span              = &E_GET_NODE(cc->ast, node)->common.span;
      l.val.member.base   = E_GET_NODE(cc->ast, node)->member_assign.left;
      l.val.member.member = E_GET_NODE(cc->ast, node)->member_assign.right;
      *d                  = l;
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

/* Returns 0 on succcess. */
static inline RETURNS_ERRCODE int
emit_lvalue_assign_prologue(e_compiler* cc, val_t lv)
{
  if (lv.type == E_LVAL_VAR) {
  }
  /* LVAL_INDEX handles all three of INDEX, INDEX_ASSIGN and INDEX_COMPOUND */
  else if (lv.type == E_LVAL_INDEX) {
    if (!can_make_value(cc->ast, lv.val.index.left_node)) {
      cerror(*lv.span, "Can not assign back to indexed structure\n");
      return -1;
    }

    val_t left;
    int   e = value_init(cc, lv.val.index.left_node, &left);
    if (e) return e;

    e = emit_lvalue_load(cc, left);
    if (e) return e;

    value_free(&left);

    e = compile(cc, lv.val.index.index_node);
    if (e) return e;

  } else if (lv.type == E_LVAL_MEMBER) {
    if (!can_make_value(cc->ast, lv.val.member.base)) {
      cerror(*lv.span, "Can not assign back to structure member\n");
      return -1;
    }

    val_t base = { 0 };
    int   e    = value_init(cc, lv.val.member.base, &base);
    if (e) return e;

    e = emit_lvalue_assign_prologue(cc, base);
    if (e) {
      value_free(&base);
      return e;
    }

    e = emit_lvalue_load(cc, base);
    if (e) {
      value_free(&base);
      return e;
    }
  }

  return 0;
}

/* Returns 0 on succcess. */
static inline RETURNS_ERRCODE int
emit_lvalue_assign_epilogue(e_compiler* cc, val_t lv)
{
  int e = 0;

  if (lv.type == E_LVAL_VAR) {
    e_emit_instruction(cc, E_OPCODE_ASSIGN);
    e_emit_u32(cc, lv.val.var.id);
  }
  /* LVAL_INDEX handles all three of INDEX, INDEX_ASSIGN and INDEX_COMPOUND */
  else if (lv.type == E_LVAL_INDEX) {
    e_emit_instruction(cc, E_OPCODE_INDEX_ASSIGN);

    val_t base;
    e = value_init(cc, lv.val.index.left_node, &base);
    if (e) return e;

    if (base.type == E_LVAL_VAR) {
      e_emit_instruction(cc, E_OPCODE_ASSIGN);
      e_emit_u32(cc, base.val.var.id);
    }
    value_free(&base);

    e_emit_instruction(cc, E_OPCODE_POP);
  } else if (lv.type == E_LVAL_MEMBER) {
    e_emit_instruction(cc, E_OPCODE_MEMBER_ASSIGN);
    e_emit_u32(cc, e_hash(lv.val.member.member, strlen(lv.val.member.member)));

    // MEMBER_ASSIGN pops value

    /* Assign struct back */
    val_t base;
    e = value_init(cc, lv.val.member.base, &base);
    if (e) return e;

    e = emit_lvalue_assign_epilogue(cc, base);
    value_free(&base);
    if (e) return e;
  }

  return 0;
}

static int
emit_lvalue_assign(e_compiler* cc, int value, val_t lv)
{
  int e = emit_lvalue_assign_prologue(cc, lv);
  if (e) return e;

  if (lv.type == E_LVAL_INDEX && lv.is_compound) {
    e_emit_instruction(cc, E_OPCODE_INDEX_PEEK);

    e = compile(cc, value);
    if (e) return e;

    e_emit_instruction(cc, e_binary_operator_to_opcode(lv.compound_operator));
    e_emit_instruction(cc, E_OPCODE_INDEX_ASSIGN);

    val_t base;
    e = value_init(cc, lv.val.index.left_node, &base);
    if (e) return e;

    if (base.type == E_LVAL_VAR) {
      e_emit_instruction(cc, E_OPCODE_ASSIGN);
      e_emit_u32(cc, base.val.var.id);
    }
    value_free(&base);

    e_emit_instruction(cc, E_OPCODE_POP);

    return 0; // DONE!
  }

  e = compile(cc, value);
  if (e) return e;

  e = emit_lvalue_assign_epilogue(cc, lv);
  if (e) return e;

  return 0;
}

static int
emit_lvalue_load(e_compiler* cc, val_t lv)
{
  if (lv.type == E_LVAL_VAR) {
    e_emit_instruction(cc, E_OPCODE_LOAD);
    e_emit_u32(cc, lv.val.var.id);
  } else if (lv.type == E_LVAL_INDEX) {
    /* Pushes stack top */
    int e = compile(cc, lv.val.index.left_node);
    if (e) return e;

    /* Pushes stack top */
    e = compile(cc, lv.val.index.index_node);
    if (e) return e;

    e_emit_instruction(cc, E_OPCODE_INDEX);

    return 0;
  } else if (lv.type == E_LVAL_MEMBER) {
    int         left  = lv.val.member.base;
    const char* right = lv.val.member.member;

    /* pushes stack top */
    int e = compile(cc, left);
    if (e) return e;

    e_emit_instruction(cc, E_OPCODE_MEMBER_ACCESS);
    e_emit_u32(cc, e_hash(right, strlen(right)));
  }

  return 0;
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

static inline RETURNS_ERRCODE int
lower_node_to_literal(const e_compiler* cc, int node, e_var* o)
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
compile_and_push_literal_variable(e_compiler* cc, e_var v)
{
  ecc_literal_table* literals = cc->lit_table;

  bool found = false;

  /* Search for the literal in our table. */
  u32 hash = e_var_hash(&v);
  for (u32 i = 0; i < literals->literals_count; i++) {
    if (literals->literal_hashes[i] == hash && e_var_equal(&v, &literals->literals[i])) {
      found = true;
      break;
    }
  }

  if (!found) {
    int e = append_literal_variable(literals, &v);
    if (e) return e;
  } else if (v.type == E_VARTYPE_STRING) {
    /**
     * We allocate strings on the ge_pool ourselves
     * And so need to free them.
     */
    // free(E_VAR_AS_STRING(&v)->s);
    e_refdobj_pool_return(&ge_pool, v.val.s);

    /**
     * Since the variables all have the same lifetime, using the refcounter
     * here would just complicate things (need to store more literals
     * on the compilers literal table, then free them).
     * What we do instead is:
     * Create all the literals during compilation.
     * Free all the literals after.
     * Done!
     */
    // e_var_acquire(&cc->literals[id]); // refcounter++ for the reused variable
  }

  e_emit_instruction(cc, E_OPCODE_LITERAL);
  e_emit_u32(cc, hash); // Its hash!

  return 0;
}

static RETURNS_ERRCODE int
compile_literal(e_compiler* cc, int node)
{
  // Convert the node to a variable and
  e_var v = { .type = E_VARTYPE_NULL };
  if (lower_node_to_literal(cc, node, &v)) return -1;

  // Compile it
  return compile_and_push_literal_variable(cc, v);
}

static RETURNS_ERRCODE int
compile_list(e_compiler* cc, int node)
{
  u32 nelems = E_GET_NODE(cc->ast, node)->list.nelems;
  for (u32 i = 0; i < nelems; i++) {
    int elem_node = E_GET_NODE(cc->ast, node)->list.elems[i];
    int e         = compile(cc, elem_node);
    if (e) return e;
  }

  e_emit_instruction(cc, E_OPCODE_MK_LIST);
  e_emit_u32(cc, nelems);

  return 0;
}

static RETURNS_ERRCODE int
compile_map(e_compiler* cc, int node)
{
  int e = 0;

  u32 npairs = E_GET_NODE(cc->ast, node)->map.npairs;
  for (u32 i = 0; i < npairs; i++) {
    int key = E_GET_NODE(cc->ast, node)->map.keys[i];
    int val = E_GET_NODE(cc->ast, node)->map.values[i];

    e = compile(cc, key); // Pushes stack top
    if (e) return e;
    e = compile(cc, val); // Pushes stack top
    if (e) return e;
  }

  e_emit_instruction(cc, E_OPCODE_MK_MAP);
  e_emit_u32(cc, npairs);

  return 0;
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

static int
compile_function_definition(e_compiler* cc, int node)
{
  e_compiler fork  = { 0 };
  e_stackemu stack = { 0 };

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

  u32  nargs     = E_GET_NODE(cc->ast, node)->func.nargs;
  u32* arg_slots = nullptr;

  e = e_stackemu_init(&stack);
  if (e) goto ERR;

  /* Copy global state from our compiler to the fork */
  e = e_stackemu_copy_top_scope(&stack, cc->stack);
  if (e) goto ERR;

  e = compiler_make_fork(cc, &fork);
  if (e) goto ERR;

  fork.stack = &stack;

  if (nargs > 0) {
    arg_slots = (u32*)e_arnalloc(cc->arena, sizeof(u32) * nargs);
    if (!arg_slots) goto ERR;

    for (u32 i = 0; i < nargs; i++) {
      const char* arg_name = E_GET_NODE(cc->ast, node)->func.args[i];

      char* full_arg_name = qualify_name(cc, arg_name);
      if (!full_arg_name) goto ERR;

      u32 arg_hash = e_hash(full_arg_name, strlen(full_arg_name));
      // free(full_arg_name);

      arg_slots[i] = arg_hash;

      /* Add variable entry to stack */
      ecc_variable_information info = {
        .initializer   = -1, // Arguments aren't initialized.
        .current_value = -1, // Or initialized to void if you think about it.
        .name_hash     = arg_hash,
        .span          = E_GET_NODE(cc->ast, node)->common.span,
        .is_const      = false, // User can override the argument any time.
      };
      e = e_stackemu_push_var(&stack, &info);
      if (e) goto ERR;
    }
  }

  e = defer_push_scope(&fork);
  if (e) goto ERR;

  e = compile_function(&fork, node);
  if (e) {
    e_filespan span = E_GET_NODE(cc->ast, node)->common.span;
    cerror(span, "Failed to compile function body [function definition]\n");
    goto ERR;
  }

  // emit the defer before the return you asshole
  e = defer_emit_current_scope(&fork);
  if (e) goto ERR;

  /* Always need this! */
  e_emit_instruction(&fork, E_OPCODE_RETURN);
  e_emit_u8(&fork, false);

  defer_pop_scope(&fork);

  compiler_join_fork(&fork, cc);

  ecc_function f = {
    .code      = fork.emit,
    .code_size = fork.num_bytes_emitted,
    .arg_slots = arg_slots,
    .name_hash = hash,
    .nargs     = nargs,
  };

  e = append_function_entry(cc->arena, cc->function_table, &f);
  if (e) goto ERR;

  e_stackemu_free(&stack);

  return e;

ERR:
  compiler_free_fork_entirely(&fork);
  e_stackemu_free(&stack);
  return e == 0 ? -1 : e;
}

static int
compile_binary_op(e_compiler* cc, int node)
{
  val_t lv = { 0 };
  int   e  = 0;

  bool is_compound = E_GET_NODE(cc->ast, node)->binaryop.is_compound;
  int  left        = E_GET_NODE(cc->ast, node)->binaryop.left;
  int  right       = E_GET_NODE(cc->ast, node)->binaryop.right;

  e_opcode opcode = e_binary_operator_to_opcode(E_GET_NODE(cc->ast, node)->binaryop.op);
  if (opcode < 0) {
    cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a binary operator\n", E_GET_NODE(cc->ast, node)->binaryop.op);
    goto err;
  }

  if (is_compound && !can_make_value(cc->ast, left)) {
    cerror(E_GET_NODE(cc->ast, left)->common.span, "Can not assign to left\n");
    goto err;
  }

  if (is_compound) {
    // Verified  earlier that we can make it into an lvalue
    e = value_init(cc, left, &lv);
    if (e) goto err;

    /* Load data to point to where we want to assign it (Not stack information!) */
    e = emit_lvalue_assign_prologue(cc, lv);
    if (e) goto err;

    /* Load left */
    e = emit_lvalue_load(cc, lv);
    if (e) goto err;

    /* Load right */
    e = compile(cc, right);
    if (e) goto err;

    /* Emit operator */
    e_emit_instruction(cc, opcode);

    /* Emit actual assign instruction (takes value produced earlier and assigns it) */
    e = emit_lvalue_assign_epilogue(cc, lv);
    if (e) goto err;

    value_free(&lv);
  } else {
    e = compile(cc, left);
    if (e) goto err;

    e = compile(cc, right);
    if (e) goto err;

    e_emit_instruction(cc, opcode);
  }

  return e;

err:
  value_free(&lv);
  return -1;
}

static int
compile_unary_op(e_compiler* cc, int node)
{
  e_opcode opcode = -1;

  switch (E_GET_NODE(cc->ast, node)->unaryop.op) {
    case E_OPERATOR_NOT: opcode = E_OPCODE_NOT; break;
    case E_OPERATOR_BNOT: opcode = E_OPCODE_BNOT; break;
    case E_OPERATOR_INC: opcode = E_OPCODE_INC; break;
    case E_OPERATOR_DEC: opcode = E_OPCODE_DEC; break;
    case E_OPERATOR_SUB: opcode = E_OPCODE_NEG; break;
    case E_OPERATOR_ADD: opcode = E_OPCODE_NOOP; break;
    default:
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a unary operator\n", E_GET_NODE(cc->ast, node)->unaryop.op);
      return -1;
  }

  bool is_compound = E_GET_NODE(cc->ast, node)->unaryop.is_compound;

  int right = E_GET_NODE(cc->ast, node)->unaryop.right;
  int e     = 0;

  if (is_compound) {
    val_t lv = { 0 };

    if (!can_make_value(cc->ast, right)) {
      cerror(E_GET_NODE(cc->ast, right)->common.span, "Can not assign to right\n");
      return -1;
    }

    // Verified  earlier that we can make it into an lvalue
    e = value_init(cc, right, &lv);
    if (e) goto err;

    /* Load data to point to where we want to assign it (Not stack information!) */
    e = emit_lvalue_assign_prologue(cc, lv);
    if (e) goto err;

    /* Load right */
    e = compile(cc, right);
    if (e) goto err;

    /* Emit operator */
    e_emit_instruction(cc, opcode);

    /* Emit actual assign instruction (takes value produced earlier and assigns it) */
    e = emit_lvalue_assign_epilogue(cc, lv);
    if (e) goto err;

    value_free(&lv);
  } else {
    /* Load right */
    e = compile(cc, right);
    if (e) goto err;

    /* Emit operator */
    e_emit_instruction(cc, opcode);
  }

  return 0;

err:
  return -1;
}

static int
compile_index(e_compiler* cc, int node)
{
  int e = compile(cc, E_GET_NODE(cc->ast, node)->index.base);
  if (e) return e;

  e = compile(cc, E_GET_NODE(cc->ast, node)->index.index);
  if (e) return e;

  e_emit_instruction(cc, E_OPCODE_INDEX);

  return 0;
}

static int
compile_index_assign(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->index_assign.value;

  if (!can_make_value(cc->ast, node)) {
    e_filespan left_span = E_GET_NODE(cc->ast, node)->index_assign.span;
    cerror(left_span, "Can not assign to indexed expression: Failed to lower to lvalue\n");
    return -1;
  }

  val_t v;
  int   e = value_init(cc, node, &v);
  if (e) return e;

  e = emit_lvalue_assign(cc, value, v);
  value_free(&v);

  return e;
}

static int
compile_index_compound(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->index_compound.value;

  if (!can_make_value(cc->ast, node)) {
    e_filespan left_span = E_GET_NODE(cc->ast, node)->common.span;
    cerror(left_span, "Can not assign to indexed compound expression: Failed to lower to lvalue\n");
    return -1;
  }

  val_t v;
  int   e = value_init(cc, node, &v);
  if (e) return e;

  e = emit_lvalue_assign(cc, value, v);
  value_free(&v);

  return e;
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
  return nullptr;
}

static int
compile_function_call(e_compiler* cc, int node)
{
  e_filespan function_span = E_GET_NODE(cc->ast, node)->common.span;
  u32        nargs         = E_GET_NODE(cc->ast, node)->call.nargs;
  int*       args          = E_GET_NODE(cc->ast, node)->call.args;

  const char* function_name = E_GET_NODE(cc->ast, node)->call.function_name;

  char* full = qualify_name(cc, function_name);
  u32   hash = e_hash(full, strlen(full));

  int e = 0;
  for (u32 i = 0; i < nargs; i++) {
    e = compile(cc, args[i]); // Pushes stack top
    if (e) {
      cerror(E_GET_NODE(cc->ast, args[i])->common.span, "Failed to compile argument #%i [function call]\n", i);
      return e;
    }
  }

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
    ecc_function*       func       = nullptr;
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
    }
  }

  e_emit_instruction(cc, E_OPCODE_CALL); // 1 byte
  e_emit_u32(cc, hash);                  // 4 bytes, function ID
  e_emit_u16(cc, nargs);                 // 2 bytes, number of arguments

  return 0;
}

// This is the dirtiest of them all...
static int
compile_if_statement(e_compiler* cc, int node)
{
  /* Label after if statements body */
  u32 end_label = make_label_id(cc);

  const int condition = E_GET_NODE(cc->ast, node)->if_stmt.condition;

  /**
   * Label of the next else if / else to jump to. Still used if no branches are present, there's
   * just a jmp instruction directly after the JMP to where the branch would be.
   */
  u32 next_in_chain_label = make_label_id(cc);

  int e = e_stackemu_push_frame(cc->stack);
  if (e) goto ERR;

  e_emit_instruction(cc, E_OPCODE_PUSH_FRAME);

  // condition
  e = compile(cc, condition);
  if (e) {
    cerror(E_GET_NODE(cc->ast, condition)->common.span, "Failed to compile condition of top if statement [if statement]\n");
    goto ERR;
  }

  // condition failed :<
  e = emit_and_record_jmp(cc, E_OPCODE_JZ, next_in_chain_label); // Jump to the next in chain. else if/else/end of if statement
  if (e) goto ERR;

  e = defer_push_scope(cc);
  if (e) goto ERR;

  // BODY OF ROOT IF STATEMENT
  const int* if_body = E_GET_NODE(cc->ast, node)->if_stmt.body;
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nstmts; i++) {
    e = compile(cc, if_body[i]);
    if (e) {
      cerror(E_GET_NODE(cc->ast, if_body[i])->common.span, "Failed to compile if statement body [if statement]\n");
      goto ERR;
    }

    if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, if_body[i]);
  }

  e = defer_emit_current_scope(cc);
  if (e) goto ERR;

  defer_pop_scope(cc);
  e_stackemu_pop_frame(cc->stack);

  // Still inside the body, JMP over all other branches
  // since we're done executing the body of the if statement
  e = emit_and_record_jmp(cc, E_OPCODE_JMP, end_label); // JUMP!
  if (e) goto ERR;

  // ELSE IFS
  for (u32 else_if_idx = 0; else_if_idx < E_GET_NODE(cc->ast, node)->if_stmt.nelse_ifs; else_if_idx++) {
    // Emit the next in chain label for instructions to jump to.
    define_and_emit_label(cc, next_in_chain_label);
    next_in_chain_label = make_label_id(cc);

    e = e_stackemu_push_frame(cc->stack);
    if (e) goto ERR;

    // dont worry about it dont worry about it dont worry about it dont worry about it
    struct e_if_stmt* elif = &E_GET_NODE(cc->ast, node)->if_stmt.else_ifs[else_if_idx];

    // CONDITION
    e = compile(cc, elif->condition);
    if (e) {
      cerror(E_GET_NODE(cc->ast, elif->condition)->common.span, "Failed to compile condition of else if statement [if statement]\n");
      goto ERR;
    }

    /* Failed. Jump to the next in chain. */
    e = emit_and_record_jmp(cc, E_OPCODE_JZ, next_in_chain_label);
    if (e) goto ERR;

    // JZ pops condition

    e = defer_push_scope(cc);
    if (e) goto ERR;

    /* Condition true! Execute the body */
    for (u32 i = 0; i < elif->nstmts; i++) {
      e = compile(cc, elif->body[i]);
      if (e) {
        cerror(E_GET_NODE(cc->ast, elif->body[i])->common.span, "Failed to compile body of else if statement [if statement]\n");
        goto ERR;
      }

      if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, elif->body[i]);
    }

    e = defer_emit_current_scope(cc);
    if (e) goto ERR;

    defer_pop_scope(cc);

    e_stackemu_pop_frame(cc->stack);

    /* JMP over all other branches. */
    e = emit_and_record_jmp(cc, E_OPCODE_JMP, end_label); // skip remaining elseifs and else
    if (e) goto ERR;
  }

  /* Emit the final next in chain label for the else statement. */
  define_and_emit_label(cc, next_in_chain_label); // BAM!

  e = e_stackemu_push_frame(cc->stack);
  if (e) goto ERR;

  e = defer_push_scope(cc);
  if (e) goto ERR;

  /* ELSE BODY */
  int* else_body = E_GET_NODE(cc->ast, node)->if_stmt.else_body;
  for (u32 i = 0; i < E_GET_NODE(cc->ast, node)->if_stmt.nelse_stmts; i++) {
    e = compile(cc, else_body[i]);
    if (e) {
      cerror(E_GET_NODE(cc->ast, else_body[i])->common.span, "Failed to compile body of else statement [if statement]\n");
      goto ERR;
    }

    if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, else_body[i]);
    /* No need to jump! we're already at the end :> */
  }

  e = defer_emit_current_scope(cc);
  if (e) goto ERR;

  defer_pop_scope(cc);
  e_stackemu_pop_frame(cc->stack);

  /* END LABEL. There's still one instruction after this and it's to ensure we always pop our variables. */
  define_and_emit_label(cc, end_label);

  /* Pop scope. */
  e_emit_instruction(cc, E_OPCODE_POP_FRAME);

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
static int
compile_while_statement(e_compiler* cc, int node)
{
  int e = 0;

  /* Computes the condition and jumps to the end label (breaks) if condition is false */
  const u32 pre_condition_label = make_label_id(cc);

  /* After the while loop, with one POP_VARIABLES to ensure we always pop our variables. */
  const u32 end_label = make_label_id(cc);

  define_and_emit_label(cc, pre_condition_label);

  /* CONDITION */
  e = compile(cc, E_GET_NODE(cc->ast, node)->while_stmt.condition);
  if (e) goto ERR;

  // Break out of loop if condition is false.
  e = emit_and_record_jmp(cc, E_OPCODE_JZ, end_label);
  if (e) goto ERR;

  e_emit_instruction(cc, E_OPCODE_PUSH_FRAME);
  e = e_stackemu_push_frame(cc->stack);
  if (e) goto ERR;

  /**
   * Push frame for the stack
   */
  e = defer_push_scope(cc);
  if (e) goto ERR;

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
    if (e) goto ERR;

    if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, while_body[i]);
  }

  // Pop the scope
  e_emit_instruction(cc, E_OPCODE_POP_FRAME);
  e_stackemu_pop_frame(cc->stack);

  /* Jump to condition, body is done executing */
  e = emit_and_record_jmp(cc, E_OPCODE_JMP, pre_condition_label);
  if (e) goto ERR;

  // End label.
  define_and_emit_label(cc, end_label);

  e = defer_emit_current_scope(cc);
  if (e) goto ERR;
  defer_pop_scope(cc);

  // swap the old loop metadata back in
  cc->loop = last;

  return 0;

ERR:
  return e ? e : -1;
}

static int
compile_for_statement(e_compiler* cc, int node)
{
  int initializers = -1;
  int condition    = -1;

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

  /* For the compiler too */
  int e = e_stackemu_push_frame(cc->stack);
  if (e) goto ERR;

  e_emit_instruction(cc, E_OPCODE_PUSH_FRAME);

  e = e_stackemu_push_frame(cc->stack);
  if (e) goto ERR;

  // INITIALIZERS
  initializers = E_GET_NODE(cc->ast, node)->for_stmt.initializers;
  if (initializers >= 0 && compile(cc, initializers) < 0) {
    cerror(E_GET_NODE(cc->ast, initializers)->common.span, "Failed to compile initializers [for statement]\n");
    goto ERR;
  }

  // TOP_LABEL
  define_and_emit_label(cc, top_label);

  /* Push a new scope */
  e_emit_instruction(cc, E_OPCODE_PUSH_FRAME);
  e = e_stackemu_push_frame(cc->stack);
  if (e) goto ERR;

  // CONDITION
  condition = E_GET_NODE(cc->ast, node)->for_stmt.condition;
  if (condition < 0) {
    // Can't use condition's span!
    cerror(E_GET_NODE(cc->ast, initializers)->common.span, "Empty for statement conditions are not currently supported [for statement]\n");
    goto ERR;
  }

  if (condition >= 0 && compile(cc, condition) < 0) {
    cerror(E_GET_NODE(cc->ast, condition)->common.span, "Failed to compile condition [for statement]");
    goto ERR;
  }

  // JZ END_LABEL
  e = emit_and_record_jmp(cc, E_OPCODE_JZ, end_label);
  if (e) goto ERR;

  // JZ pops condition variable

  e = defer_push_scope(cc);
  if (e) goto ERR;

  u32 old_depth = cc->loop->defer_depth;
  if (cc->loop) cc->loop->defer_depth = defer_get_current_depth(cc);

  // BODY
  u32  nstmts = E_GET_NODE(cc->ast, node)->for_stmt.nstmts;
  int* stmts  = E_GET_NODE(cc->ast, node)->for_stmt.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    if (compile(cc, stmts[i]) < 0) {
      cerror(E_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement in body [for statement]");
      goto ERR;
    }

    if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, stmts[i]);
  }

  /**
   * Deposit the deferred statements before the
   * iterator region so all iterator values are
   * correct.
   */
  e = defer_emit_current_scope(cc);
  if (e) goto ERR;

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
  e_emit_instruction(cc, E_OPCODE_POP_FRAME);

  e_stackemu_pop_frame(cc->stack);

  // JMP TOP_LABEL
  e = emit_and_record_jmp(cc, E_OPCODE_JMP, top_label);
  if (e) goto ERR;

  // END_LABEL
  define_and_emit_label(cc, end_label);

  e_emit_instruction(cc, E_OPCODE_POP_FRAME); // Pop entire for scope
  e_stackemu_pop_frame(cc->stack);

  cc->loop = last;

  return 0;

ERR:
  /* ensure we don't leave a dangling reference to loop */
  cc->loop = last;
  return e ? e : -1;
}

static int
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
  if (e) return e;

  return emit_lvalue_load(cc, lv);
}

static int
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
  if (e) return e;

  ecc_builtin_variables_table* builtin_vars_table = cc->builtin_var_table;

  ecc_variable_information* exists = e_stackemu_find_var(cc->stack, lv.val.var.id);

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

  e = emit_lvalue_assign(cc, right, lv);
  value_free(&lv);
  if (e) return e;

  return 0;
}

static int
compile_member_assign(e_compiler* cc, int node)
{
  int value = E_GET_NODE(cc->ast, node)->member_assign.value;

  if (!can_make_value(cc->ast, node)) {
    cerror(E_GET_NODE(cc->ast, node)->common.span, "Can not assign to member access: Failed to lower to lvalue\n");
    return -1;
  }

  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e) return e;

  e = emit_lvalue_assign(cc, value, lv);
  value_free(&lv);

  return e;
}

static int
compile_return(e_compiler* cc, int node)
{
  int e = 0;
  if (E_GET_NODE(cc->ast, node)->ret.has_return_value) {
    /* Compile the return value */
    e = compile(cc, E_GET_NODE(cc->ast, node)->ret.expr_id);
    if (e) {
      cerror(E_GET_NODE(cc->ast, node)->common.span, "Failed to compile return value [return]\n");
      return e;
    }

    e = defer_emit_all_scopes(cc);
    if (e) return e;

    e_emit_instruction(cc, E_OPCODE_RETURN);
    e_emit_u8(cc, true); // Specify that we're returning a value
  } else {
    e = defer_emit_all_scopes(cc);
    if (e) return e;

    e_emit_instruction(cc, E_OPCODE_RETURN);
    e_emit_u8(cc, false); /* Returning void! */
  }
  return e;
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
      if (e) return e;
    } else if (type == E_AST_NODE_VARIABLE_DECL) {
      bool is_const = E_GET_NODE(cc->ast, stmts[i])->let.is_const;
      if (is_const) {
        cerror(E_GET_NODE(cc->ast, stmts[i])->let.span, "A member of a struct cannot be declared 'const' [unsupported]\n");
        return -1;
      }

      const char* name = E_GET_NODE(cc->ast, stmts[i])->let.name;
      int         e    = append_struct_decleration(cc->arena, name, deposit);
      if (e) return e;
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

static int
compile_struct_constructor(e_compiler* fork, e_filespan span, const ecc_struct_information* struc)
{
  u32* arg_slots = (u32*)e_arnalloc(fork->arena, sizeof(u32) * struc->fields_count);
  for (u32 i = 0; i < struc->fields_count; i++) {
    u32 arg_hash = struc->field_hashes[i];
    arg_slots[i] = arg_hash;
  }

  u32 struct_id = e_hash(struc->name, strlen(struc->name));

  if (fork->info->opt_level >= 1) {
    /* This sums up to about ~7 bytes. */
    e_emit_instruction(fork, E_OPCODE_STRUCT_CONSTRUCT);
    e_emit_u32(fork, struct_id);

    /* struct_construct doesn't return the value... */
    e_emit_instruction(fork, E_OPCODE_RETURN);
    e_emit_u8(fork, true); // has_return_value = true
  } else {
    // Slow path :(
    int e = e_stackemu_push_frame(fork->stack);
    if (e) return e;

    for (u32 i = 0; i < struc->fields_count; i++) {
      /* Add member entry to stack */
      ecc_variable_information info = {
        .initializer   = -1, // Arguments aren't initialized.
        .current_value = -1, // Or initialized to null if you think about it.
        .name_hash     = arg_slots[i],
        .span          = span,
        .is_const      = false, // User can override the argument any time.
      };

      e = e_stackemu_push_var(fork->stack, &info);
      if (e) return e;
    }

    e_emit_instruction(fork, E_OPCODE_MK_STRUCT);
    e_emit_u32(fork, struct_id); /* its ID */

    /**
     * Assign the arguments to all members
     */
    for (u32 i = 0; i < struc->fields_count; i++) {
      e_emit_instruction(fork, E_OPCODE_DUP); // Duplicate struct (shallow copy)

      e_emit_instruction(fork, E_OPCODE_LOAD); // Load the argument
      e_emit_u32(fork, struc->field_hashes[i]);

      e_emit_instruction(fork, E_OPCODE_MEMBER_ASSIGN);
      e_emit_u32(fork, struc->field_hashes[i]); // Assign to that field
      e_emit_instruction(fork, E_OPCODE_POP);   // Pop member assign pushing the value back up. We only
                                                // want the struct to be on the stack (Member assign pushes struct + value, in that order).
    }

    // Return the accumulated struct.
    e_emit_instruction(fork, E_OPCODE_RETURN);
    e_emit_u8(fork, true); // has_return_value = true

    e_stackemu_pop_frame(fork->stack);
  }

  ecc_function f = {
    .code      = fork->emit,
    .code_size = fork->num_bytes_emitted,
    .arg_slots = arg_slots,
    .name_hash = struct_id,
    .nargs     = struc->fields_count,
  };

  int e = append_function_entry(fork->arena, fork->function_table, &f);
  if (e) return e;

  return 0;
}

static int
compile_struct_decleration(e_compiler* cc, int node)
{
  int                    e           = 0;
  e_compiler             fork        = { 0 };
  ecc_struct_information struct_data = { 0 };

  const char* struct_name        = E_GET_NODE(cc->ast, node)->struct_decl.name;
  int*        struct_decl_stmts  = E_GET_NODE(cc->ast, node)->struct_decl.stmts;
  u32         struct_decl_nstmts = E_GET_NODE(cc->ast, node)->struct_decl.nstmts;

  e_str_intern(struct_name, cc->ast->interner);

  /* Gather all information the user provided into one big structure. */
  struct_data = (ecc_struct_information){
    .name           = e_arnstrdup(cc->arena, struct_name),
    .field_hashes   = (u32*)e_xalloc(init_fields_capacity, sizeof(u32)),
    .field_names    = (char**)e_xalloc(init_fields_capacity, sizeof(char**)),
    .field_capacity = init_fields_capacity,
    .fields_count   = 0,
  };
  if (!struct_data.field_hashes) goto ERR;

  e = collect_struct_declerations(cc, struct_decl_stmts, struct_decl_nstmts, &struct_data);
  if (e) goto ERR;

  /* Generate the constructor function */
  e = compiler_make_fork(cc, &fork);
  if (e) goto ERR;

  e = e_stackemu_push_frame(cc->stack);
  if (e) goto ERR;

  e = defer_push_scope(&fork);
  if (e) goto ERR;

  e = compile_struct_constructor(&fork, E_GET_NODE(cc->ast, node)->common.span, &struct_data);
  if (e) goto ERR;

  e = append_struct_info(cc, &struct_data);
  if (e) goto ERR;

  defer_pop_scope(&fork);
  compiler_join_fork(&fork, cc);

  e_stackemu_pop_frame(cc->stack);

  return 0;

ERR:
  if (struct_data.field_hashes) free(struct_data.field_hashes);
  if (struct_data.field_names) free((void*)struct_data.field_names);
  compiler_free_fork_entirely(&fork);
  return e ? e : -1;
}

static int
compile_variable_decleration(e_compiler* cc, int node)
{
  const char* name = E_GET_NODE(cc->ast, node)->let.name;
  char*       full = qualify_name(cc, name);
  if (!full) return -1;

  // fprintf(stderr, "declared: %s\n", full);

  u32 hash        = e_hash(full, strlen(full));
  int initializer = E_GET_NODE(cc->ast, node)->let.initializer;

  ecc_variable_information* exists = e_stackemu_find_var_in_curr_scope(cc->stack, hash);
  if (exists != nullptr) {
    cerror(
        E_GET_NODE(cc->ast, node)->common.span,
        "Variable %s redeclared in same scope. Earlier occurence at [%s:%i:%i]\n",
        full,
        exists->span.file,
        exists->span.line,
        exists->span.col);
    return -1;
  }

  /* Add variable entry to stack */

  const bool initializer_provided = initializer >= 0;

  e_emit_instruction(cc, E_OPCODE_INIT);
  e_emit_u32(cc, hash);

  /* make_value has a specialized path for variable decleration nodes */
  if (!can_make_value(cc->ast, node)) {
    fprintf(stderr, "Fatal error (possible memory corruption)\n");
    return -1;
  }

  /* Acquire a refdobj */
  const ecc_variable_information info = {
    .initializer   = initializer,
    .current_value = initializer, // current value is initializer, -1 if none
    .name_hash     = hash,
    .span          = E_GET_NODE(cc->ast, node)->common.span,
    .is_const      = E_GET_NODE(cc->ast, node)->let.is_const,
  };

  int e = e_stackemu_push_var(cc->stack, &info);
  if (e) return -1;

  if (initializer_provided) {
    val_t lv;
    e = value_init(cc, node, &lv);
    if (e) return e;

    e = emit_lvalue_assign(cc, initializer, lv);
    value_free(&lv);
    if (e) return e;
  }

  return 0;
}

static int
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
    if (e) {
      cerror(E_GET_NODE(cc->ast, root->root.stmts[i])->common.span, "Failed to compile root, bailing out [root]\n");
      return e;
    }
  }

  /**
   * Done with initialization!
   * Now we emit a HALT and expect
   * the VM to call main() or its desired
   * function (say, update())
   */
  e_emit_instruction(cc, E_OPCODE_HALT);
  e_emit_u32(cc, 0);
  return 0; // Done!
}

static int
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

      return compile_and_push_literal_variable(cc, v); // compile_literal_variable loads the value! Return.
    }
  }

  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e) return e;

  if (emit_lvalue_load(cc, lv) < 0) {
    value_free(&lv);
    return -1;
  }

  value_free(&lv);
  return 0;
}

static int
compile_namespace_decleration(e_compiler* cc, int node)
{
  const char* ns_name        = E_GET_NODE(cc->ast, node)->namespace_decl.name;
  int*        ns_decl_stmts  = E_GET_NODE(cc->ast, node)->namespace_decl.stmts;
  u32         ns_decl_nstmts = E_GET_NODE(cc->ast, node)->namespace_decl.nstmts;

  int e = ns_push(cc, ns_name);
  if (e) return e;

  for (u32 i = 0; i < ns_decl_nstmts; i++) {
    e = compile(cc, ns_decl_stmts[i]);
    if (e) {
      cerror(E_GET_NODE(cc->ast, ns_decl_stmts[i])->common.span, "Failed to compile namespace decleration [namespace decleration]\n");
      ns_pop(cc);
      return e;
    }
  }

  ns_pop(cc);

  return 0;
}

static int
compile(e_compiler* cc, int node)
{
  if (node < 0) return -1;

  /**
   * pop_if_not_used can't be used in most places here, (this function is recursive).
   * We can't make any assumptions about whether values are used or not
   * and have to rely on statement handlers to pop most unused variables.
   */

  switch (E_GET_NODE(cc->ast, node)->type) {
    case E_AST_NODE_NOP: return 0;

    case E_AST_NODE_ROOT: {
      if (compile_root(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_ASSERT: {
      const char* error_string = E_GET_NODE(cc->ast, node)->assertion.assertion_line;

      e_var error_string_var = E_NULLVAR;

      int e = make_string_variable(e_arnstrdup(cc->arena, error_string), &error_string_var);
      if (e) return e;

      // add the error string to the literal table
      e = append_literal_variable(cc->lit_table, &error_string_var);
      if (e) return e;

      // Compile statement and emit assert with error string ID
      e = compile(cc, E_GET_NODE(cc->ast, node)->assertion.stmt);
      if (e) return e;

      e_emit_instruction(cc, E_OPCODE_ASSERT);
      e_emit_u32(cc, e_var_hash(&error_string_var));

      return 0;
    }

    /* Don't push a scope */
    case E_AST_NODE_STATEMENT_LIST: {
      u32        nstmts = E_GET_NODE(cc->ast, node)->stmts.nstmts;
      const int* stmts  = E_GET_NODE(cc->ast, node)->stmts.stmts;
      for (u32 i = 0; i < nstmts; i++) {
        if (compile(cc, stmts[i]) < 0) return -1;

        /* Statement list, never push a value */
        if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, stmts[i]);
      }
      return 0;
    }

    case E_AST_NODE_DEFER: {
      int* stmts  = E_GET_NODE(cc->ast, node)->defer.stmts;
      u32  nstmts = E_GET_NODE(cc->ast, node)->defer.nstmts;
      return append_defer_entry(cc, stmts, nstmts);
    }

    case E_AST_NODE_NAMESPACE_DECL: {
      if (compile_namespace_decleration(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_STRUCT_DECL: {
      if (compile_struct_decleration(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INDEX: {
      if (compile_index(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_BINARYOP: {
      if (compile_binary_op(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_IF: {
      if (compile_if_statement(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_UNARYOP: {
      if (compile_unary_op(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_WHILE: {
      if (compile_while_statement(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_FOR: {
      if (compile_for_statement(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_MEMBER_ACCESS: {
      if (compile_member_access(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_BREAK: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->common.span, "break used outside loop\n");
        return -1;
      }
      int e = defer_emit_to_depth(cc, cc->loop->defer_depth);
      if (e) return e;

      e = emit_and_record_jmp(cc, E_OPCODE_JMP, cc->loop->break_label);
      if (e) return e;

      return 0;
    }

    case E_AST_NODE_CONTINUE: {
      if (!cc->loop) {
        cerror(E_GET_NODE(cc->ast, node)->common.span, "continue used outside loop\n");
        return -1;
      }
      int e = defer_emit_to_depth(cc, cc->loop->defer_depth);
      if (e) return e;

      e = emit_and_record_jmp(cc, E_OPCODE_JMP, cc->loop->continue_label);
      if (e) return e;

      return 0;
    }

    case E_AST_NODE_RETURN: {
      if (compile_return(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_VARIABLE: {
      if (compile_variable_load(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      if (compile_variable_decleration(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_ASSIGN: {
      if (compile_assign(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INDEX_ASSIGN: {
      if (compile_index_assign(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      if (compile_member_assign(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INDEX_COMPOUND_OP: {
      if (compile_index_compound(cc, node)) { return -1; }
      return 0;
    }

    case E_AST_NODE_CALL: {
      if (compile_function_call(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_INT:
    case E_AST_NODE_CHAR:
    case E_AST_NODE_BOOL:
    case E_AST_NODE_STRING:
    case E_AST_NODE_FLOAT: {
      if (compile_literal(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_LIST: {
      if (compile_list(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_MAP: {
      if (compile_map(cc, node) < 0) { return -1; }
      return 0;
    }

    case E_AST_NODE_FUNCTION_DEFINITION: {
      if (compile_function_definition(cc, node) < 0) { return -1; }
      return 0;
    }
  }

  return -1;
}

static int
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
    if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, stmts[i]);
  }
  return 0;
}

static int
compile_builtin_structure(e_compiler* cc, const e_builtin_struct* b)
{
  e_compiler fork = { 0 };
  int        e    = 0;

  e_str_intern(b->name, cc->ast->interner);

  ecc_struct_information st = {
    .name           = e_arnstrdup(cc->arena, b->name),
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
  if (e) goto ERR;

  e = compile_struct_constructor(&fork, E_GET_NODE(cc->ast, cc->ast->root)->common.span, &st);
  if (e) goto ERR;

  compiler_join_fork(&fork, cc);

  e = append_struct_info(cc, &st);
  if (e) goto ERR;

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
static int
compile_builtin_structures(e_compiler* cc)
{
  for (u32 i = 0; i < E_ARRLEN(eb_structs); i++) {
    const e_builtin_struct* b = &eb_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e) return e;
  }

  /**
   * Compile all hooked builtin structures.
   */
  for (u32 i = 0; i < cc->info->nhooked_structs; i++) {
    const e_builtin_struct* b = &cc->info->hook_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e) return e;
  }

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
  E_GET_NODE(p, override_node)->type = var_type_to_node_type(lit->type);
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
      if (e) return e;

      e = fold(cc, r);
      if (e) return e;

      if (!is_literal_value(cc->ast, l) || !is_literal_value(cc->ast, r)) return 0;

      e_var lv;
      e_var rv;

      e = lower_node_to_literal(cc, l, &lv);
      if (e) return e;

      e = lower_node_to_literal(cc, r, &rv);
      if (e) return e;

      e_var result = operate(lv, rv, e_binary_operator_to_opcode(E_GET_NODE(cc->ast, node)->binaryop.op));

      e = convert_literal_to_node(cc->ast, node, &result);
      if (e) return e;
      break;
    }

    case E_AST_NODE_ROOT: {
      for (u32 i = 0; i < nodep->root.nstmts; i++) {
        e = fold(cc, nodep->root.stmts[i]);
        if (e) return e;
      }
      return 0;
    }
    case E_AST_NODE_STATEMENT_LIST: {
      for (u32 i = 0; i < nodep->stmts.nstmts; i++) {
        e = fold(cc, nodep->stmts.stmts[i]);
        if (e) return e;
      }
      return 0;
    }
    case E_AST_NODE_DEFER: {
      for (u32 i = 0; i < nodep->defer.nstmts; i++) {
        e = fold(cc, nodep->defer.stmts[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_FUNCTION_DEFINITION: {
      for (u32 i = 0; i < nodep->func.nstmts; i++) {
        e = fold(cc, nodep->func.stmts[i]);
        if (e) return e;
      }
      /* Can not optimize arguments in function definition. CALL should do that. */
      return 0;
    }

    case E_AST_NODE_NAMESPACE_DECL: {
      for (u32 i = 0; i < nodep->namespace_decl.nstmts; i++) {
        e = fold(cc, nodep->namespace_decl.stmts[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_CALL: {
      for (u32 i = 0; i < nodep->call.nargs; i++) {
        e = fold(cc, nodep->call.args[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_VARIABLE_DECL: {
      if (nodep->let.initializer >= 0) {
        e = fold(cc, nodep->let.initializer);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_ASSIGN: {
      e = fold(cc, nodep->assign.left);
      if (e) return e;
      e = fold(cc, nodep->assign.right);
      return e;
    }

    case E_AST_NODE_MEMBER_ASSIGN: {
      e = fold(cc, nodep->member_assign.left);
      if (e) return e;
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

    case E_AST_NODE_INDEX_COMPOUND_OP: {
      e = fold(cc, nodep->index_compound.index);
      e = fold(cc, nodep->index_compound.value);
      return 0;
    }

    case E_AST_NODE_FOR: {
      e = fold(cc, nodep->for_stmt.condition);
      if (e) return e;

      e = fold(cc, nodep->for_stmt.initializers);
      if (e) return e;

      for (u32 i = 0; i < nodep->for_stmt.nstmts; i++) {
        e = fold(cc, nodep->for_stmt.stmts[i]);
        if (e) return e;
      }

      for (u32 i = 0; i < nodep->for_stmt.niterators; i++) {
        e = fold(cc, nodep->for_stmt.iterators[i]);
        if (e) return e;
      }
      return 0;
    }

    case E_AST_NODE_WHILE: {
      for (u32 i = 0; i < nodep->while_stmt.nstmts; i++) {
        e = fold(cc, nodep->while_stmt.stmts[i]);
        if (e) return e;
      }

      e = fold(cc, nodep->while_stmt.condition);
      if (e) return e;

      return 0;
    }

    case E_AST_NODE_IF: {
      e = fold(cc, nodep->if_stmt.condition);
      if (e) return e;

      for (u32 i = 0; i < nodep->if_stmt.nstmts; i++) {
        e = fold(cc, nodep->if_stmt.body[i]);
        if (e) return e;
      }

      u32 nelse_ifs = nodep->if_stmt.nelse_ifs;
      for (u32 i = 0; i < nelse_ifs; i++) {
        struct e_if_stmt* else_if = &nodep->if_stmt.else_ifs[i];

        e = fold(cc, else_if->condition);
        if (e) return e;

        for (u32 j = 0; j < else_if->nstmts; j++) {
          e = fold(cc, else_if->body[j]);
          if (e) return e;
        }
      }

      for (u32 i = 0; i < nodep->if_stmt.nelse_stmts; i++) {
        e = fold(cc, nodep->if_stmt.else_body[i]);
        if (e) return e;
      }

      return 0;
    }

    default: break;
  }

  return 0;
}

static inline int
replace_assigns_with_movs(u8* code, u32 code_size)
{
  u8* ip = code;

  while (ip < (code + code_size)) {
    e_ins i = e_read_ins((const u8**)&ip);
    switch ((e_opcode_bck)i.opcode) {
      default: break;
    }
  }

  return 0;
}

int
e_compile(const ecc_info* info, e_compilation_result* result)
{
  int e = 0;

  ecc_namespace_stack         namespace_stack   = { 0 };
  e_stackemu                  stack             = { 0 };
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
    .stack             = &stack,
    .lit_table         = &lit_table,
    .builtin_var_table = &builtin_var_table,
    .function_table    = &func_table,
    .label_table       = &label_table,
    .struct_table      = &struct_table,
    .emit              = (u8*)e_xalloc(1, init_code_capacity),
    .num_bytes_emitted = 0,
    .emit_capacity     = init_code_capacity,
  };
  if (!cc.emit) return -1;

  /* First of all, call the optimization routines (if requested) */
  e = fold(&cc, cc.ast->root);
  if (e) goto ERR;

  e = defer_push_scope(&cc);
  if (e) goto ERR;

  e = e_stackemu_init(&stack);
  if (e) goto ERR;

  /**
   * Generate constructors for all builtin && hooked
   * structures.
   */
  e = compile_builtin_structures(&cc);
  if (e) goto ERR;

  e = compile(&cc, info->root_node);
  if (e) goto ERR;

  defer_pop_scope(&cc);

  if (result) {
    result->literals           = cc.lit_table->literals;
    result->literals_count     = cc.lit_table->literals_count;
    result->literals_hashes    = cc.lit_table->literal_hashes;
    result->functions          = func_table.functions;
    result->functions_count    = func_table.functions_count;
    result->instructions_count = cc.num_bytes_emitted;
    result->instructions       = cc.emit;
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

  e_stackemu_free(&stack);

  for (u32 i = 0; i < label_table.labels_count; i++) { free(label_table.labels[i].jumps_target_offsets); }
  e_xfree((void**)&label_table.labels);
  e_xfree((void**)&namespace_stack.namespaces);

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
  e_stackemu_free(&stack);
  free(cc.emit);
  return e ? e : -1;
}