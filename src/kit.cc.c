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

#include "../inc/kit.cc.h"

#include "../inc/kit.arena.h"
#include "../inc/kit.ast.h"
#include "../inc/kit.bfunc.h"
#include "../inc/kit.bstructs.h"
#include "../inc/kit.bvar.h"
#include "../inc/kit.cerr.h"
#include "../inc/kit.ir.h"
#include "../inc/kit.operate.h"
#include "../inc/kit.pool.h"
#include "../inc/kit.reg.h"
#include "../inc/kit.regalloc.h"
#include "../inc/kit.rwhelp.h"
#include "../inc/kit.stdafx.h"
#include "../inc/kit.strint.h"
#include "../inc/kit.var.h"

#include <assert.h>
#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct val_t;

static const u32 init_code_capacity        = 128;
static const u32 init_literal_capacity     = 64;
static const u32 init_function_capacity    = 32;
static const u32 init_namespaces_capacity  = 16;
static const u32 init_structs_capacity     = 64;
static const u32 init_fields_capacity      = 32;
static const u32 init_defer_entry_capacity = 32;

typedef struct codeblock {
  u32 start; /* first instruction (inclusive) */
  u32 end;   /* last instruction (inclusive) */

  /* Block can really only have 2 successors max (JZ and JNZ, namely the condition failed branch and the condition pass branch) */
  u32 nsuccessors;
  u32 npredecessors;

  u32  successors[2];
  u32* predecessors;

  bool* defines;
  bool* uses;

  bool* live_in;
  bool* live_out;
} codeblock;

/* our CFG */
typedef struct codegraph {
  kit_arena* arena;
  codeblock* blocks; /* arena allocated  */
  u32        nblocks;
  u32        nvregs;
} codegraph;

static ATTR_NODISCARD char* qualify_name(const kit_compiler* cc, const char* name);

static ATTR_NODISCARD int codegraph_init(kit_arena* a, kit_compiler* cc, codegraph* dst);
static int                codegraph_liveliness_analysis(kit_compiler* cc, codegraph* dst);
static int                codegraph_build_successor_list(kit_compiler* cc, codegraph* dst);
static void               codegraph_free(kit_compiler* cc, codegraph* graph);

static bool codegraph_dead_store_elimination(kit_compiler* cc, codegraph* cfg);
static bool codegraph_constant_folding(kit_compiler* cc, codegraph* cfg);
static bool codegraph_preliminary_dead_store_elimination(kit_compiler* cc, const codegraph* cfg);
static bool codegraph_redundant_move_elimination(kit_compiler* cc, codegraph* cfg);
static bool codegraph_eliminate_unreachable_code(kit_compiler* cc, codegraph* cfg);
static bool codegraph_constant_propagation(kit_compiler* cc, codegraph* cfg);
static bool codegraph_local_copy_propagation(kit_compiler* cc, codegraph* cfg);

static void opt_inline_function_calls(kit_compiler* cc);

static void forward_dead_moves(kit_compiler* cc);
static int  remove_jmp_where_it_would_fallthrough(kit_compiler* cc);
static int  strip_noops(kit_compiler* cc);
static u32  label_pass(kit_arena* arena, kit_ins* instructions, u32 ninstructions, u32 label_count);

static inline RETURNS_ERRCODE int defer_push_scope(kit_compiler* cc);
static inline void                defer_pop_scope(kit_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_current_scope(kit_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_all_scopes(kit_compiler* cc);
static inline RETURNS_ERRCODE int defer_emit_to_depth(kit_compiler* cc, u32 target_depth);
static inline u32                 defer_get_current_depth(kit_compiler* cc);

static inline int fold_vector(kit_compiler* cc, const int* elems, u32 nelems);

static inline RETURNS_ERRCODE int emit_and_record_jmp(kit_compiler* cc, eir_opcode opcode, kit_vreg_t condition, u32 label_id);
static inline void                define_and_emit_label(kit_compiler* cc, u32 label_id);

static RETURNS_ERRCODE int append_defer_entry(kit_compiler* cc, int* exprs, u32 nexprs);
static RETURNS_ERRCODE int append_function_entry(kit_arena* a, kitc_function_table* funcs, const kitc_function* func);
static RETURNS_ERRCODE int append_struct_decleration(kit_arena* a, const char* name, kitc_struct_information* deposit);
static RETURNS_ERRCODE int append_literal_variable(kitc_literal_table* literals, const kit_var* v);

static RETURNS_ERRCODE int collect_struct_declerations(kit_compiler* cc, int* stmts, u32 nstmts, kitc_struct_information* deposit);
static RETURNS_ERRCODE int append_struct_info(kit_compiler* cc, const kitc_struct_information* data);

static inline RETURNS_ERRCODE int compiler_make_fork(const kit_compiler* old_c, kit_compiler* new_c);
static inline void                compiler_join_fork(kit_compiler* copy, kit_compiler* cc);

static RETURNS_ERRCODE int        value_init(kit_compiler* cc, int node, struct val_t* d);
static void                       value_free(struct val_t* lv);
static RETURNS_ERRCODE kit_vreg_t emit_lvalue_load(kit_compiler* cc, struct val_t* lv);
static RETURNS_ERRCODE kit_vreg_t emit_lvalue_assign(kit_compiler* cc, kit_vreg_t value, struct val_t* lv);

static inline RETURNS_ERRCODE int convert_node_to_literal(kit_compiler* cc, int node, kit_var* o);
static inline bool                is_literal_value(const kit_ast* ast, int node);

/* Return register ID of result, -1 on error */
static RETURNS_ERRCODE kit_vreg_t compile_and_push_literal_variable(kit_compiler* cc, const kit_var* v);
static RETURNS_ERRCODE kit_vreg_t compile_literal(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_list(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_map(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_function_definition(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_binary_op(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_unary_op(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_index(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_function_call(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_if_statement(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_while_statement(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_for_statement(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_ranged_for_statement(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_member_access(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_assign(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_return(kit_compiler* cc, int node);
// static RETURNS_ERRCODE ereg_t compile_struct_constructor(kit_compiler* fork, kit_filespan span, const kitc_struct_information* struc);
static RETURNS_ERRCODE kit_vreg_t compile_struct_decleration(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_variable_decleration(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_variable_load(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_namespace_decleration(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_builtin_structure(kit_compiler* cc, const kit_builtin_struct* b);
static RETURNS_ERRCODE kit_vreg_t compile_builtin_structures(kit_compiler* cc);
static RETURNS_ERRCODE kit_vreg_t compile_function(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile_root(kit_compiler* cc, int node);
static RETURNS_ERRCODE kit_vreg_t compile(kit_compiler* cc, int node);

static inline kit_vreg_t
vreg_alloc(kit_compiler* cc)
{ return cc->next_vreg++; }

static kit_vreg_t
scope_define(kit_compiler* cc, kit_filespan span, const char* name, bool is_const)
{
  kit_vreg_t reg = vreg_alloc(cc);

  kitc_var* v     = kit_arnalloc(cc->arena, sizeof(kitc_var));
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

static void
scope_define_in_register(kit_compiler* cc, kit_vreg_t reg, kit_filespan span, const char* name, bool is_const)
{
  kitc_var* v     = kit_arnalloc(cc->arena, sizeof(kitc_var));
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

static int
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

static kitc_var*
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

static void
scope_push(kit_compiler* cc)
{
  kitc_scope* s = kit_arnalloc(cc->arena, sizeof(kitc_scope));
  s->vars       = NULL;
  s->parent     = cc->scope;
  cc->scope     = s;
}

static void
scope_pop(kit_compiler* cc)
{ cc->scope = cc->scope->parent; }

/* the lesser the bias, the more likely it is to inline a function */
static int
get_cost_for_inlining_function(const kit_compiler* cc, int bias, const kitc_function* fn)
{
  int cost = bias;

  /* never inline with optimizations disabled */
  if (cc->info->opt_level == 0) { return INT32_MAX; }

  if (cc->info->opt_level == 3) { cost -= 1000; /* make it much more likely to be inlined */ }

  cost += (int)(fn->code_count > INT32_MAX ? INT32_MAX : fn->code_count) / 30;
  cost += (int)fn->nargs * 10; /* each further argument increases register pressure in loops */

  return cost;
}

/**
 * Operators like SUB (-) can be used
 * as unary operators and that would add confusion
 * to the parser.
 * Only supports binary operators.
 * Unary operators must be handled seperately.
 */
static inline eir_opcode
kit_binary_operator_to_opcode(kit_operator op)
{
  switch (op) {
    case KIT_OPERATOR_ADD: return EIR_OPCODE_ADD;
    case KIT_OPERATOR_SUB: return EIR_OPCODE_SUB;
    case KIT_OPERATOR_MUL: return EIR_OPCODE_MUL;
    case KIT_OPERATOR_DIV: return EIR_OPCODE_DIV;
    case KIT_OPERATOR_MOD: return EIR_OPCODE_MOD;
    case KIT_OPERATOR_EXP: return EIR_OPCODE_EXP;
    case KIT_OPERATOR_AND: return EIR_OPCODE_AND;
    case KIT_OPERATOR_OR: return EIR_OPCODE_OR;
    case KIT_OPERATOR_BAND: return EIR_OPCODE_BAND;
    case KIT_OPERATOR_BOR: return EIR_OPCODE_BOR;
    case KIT_OPERATOR_XOR: return EIR_OPCODE_XOR;
    case KIT_OPERATOR_ISEQL: return EIR_OPCODE_EQL;
    case KIT_OPERATOR_ISNEQ: return EIR_OPCODE_NEQ;
    case KIT_OPERATOR_LT: return EIR_OPCODE_LT;
    case KIT_OPERATOR_LTE: return EIR_OPCODE_LTE;
    case KIT_OPERATOR_GT: return EIR_OPCODE_GT;
    case KIT_OPERATOR_GTE: return EIR_OPCODE_GTE;
    case KIT_OPERATOR_NOT: return EIR_OPCODE_NOT;
    case KIT_OPERATOR_BNOT: return EIR_OPCODE_BNOT;
    case KIT_OPERATOR_DEC: return EIR_OPCODE_DEC;
    case KIT_OPERATOR_INC: return EIR_OPCODE_INC;
  }
  return -1;
}

static inline bool
is_instruction_binary_operation(eir_opcode op)
{
  switch (op) {
    case EIR_OPCODE_ADD:
    case EIR_OPCODE_SUB:
    case EIR_OPCODE_MUL:
    case EIR_OPCODE_DIV:
    case EIR_OPCODE_MOD:
    case EIR_OPCODE_EXP:
    case EIR_OPCODE_AND:
    case EIR_OPCODE_OR:
    case EIR_OPCODE_BAND:
    case EIR_OPCODE_BOR:
    case EIR_OPCODE_XOR:
    case EIR_OPCODE_EQL:
    case EIR_OPCODE_NEQ:
    case EIR_OPCODE_LT:
    case EIR_OPCODE_LTE:
    case EIR_OPCODE_GT:
    case EIR_OPCODE_GTE: return true;
    default: return false;
  }
  return -1;
}

static inline bool
is_instruction_unary_operation(eir_opcode op)
{
  switch (op) {
    case EIR_OPCODE_NOT:
    case EIR_OPCODE_BNOT:
    case EIR_OPCODE_DEC:
    case EIR_OPCODE_INC: return true;
    default: return false;
  }
  return -1;
}

typedef enum kit_lval_type {
  KIT_LVAL_VAR,
  KIT_LVAL_GVAR,    // global variable
  KIT_LVAL_MEMBER,  // struct member
  KIT_LVAL_INDEX,   // indexed array
  KIT_LVAL_UNKNOWN, // Error!
} lval_type;

typedef union kit_lval_value {
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
  kit_filespan* span;
  lval_type     type;
  lval_value    val;
} val_t;

static inline bool
can_make_value(const kit_ast* ast, int node)
{
  if (ast == NULL || node < 0) return false;
  return KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_VARIABLE || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_INDEX
      || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_INDEX_ASSIGN || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_MEMBER_ACCESS
      || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_MEMBER_ASSIGN || KIT_GET_NODE(ast, node)->type == KIT_AST_NODE_VARIABLE_DECL;
}

static inline RETURNS_ERRCODE int
defer_push_scope(kit_compiler* cc)
{
  kitc_defer_scope* scope = kit_arnalloc(cc->arena, sizeof(kitc_defer_scope));
  if (!scope) return -1;

  scope->entries = kit_xalloc(init_defer_entry_capacity, sizeof(kitc_defer_entry));
  if (!scope->entries) return -1;

  scope->count    = 0;
  scope->capacity = init_defer_entry_capacity;
  scope->parent   = cc->defer_stack;
  cc->defer_stack = scope;
  return 0;
}

static inline void
defer_pop_scope(kit_compiler* cc)
{
  kitc_defer_scope* scope = cc->defer_stack;
  cc->defer_stack         = scope->parent;
  free(scope->entries);
}

static inline RETURNS_ERRCODE int
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

// LIFO
static inline RETURNS_ERRCODE int
defer_emit_current_scope(kit_compiler* cc)
{
  kitc_defer_scope* scope = cc->defer_stack;
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
defer_emit_all_scopes(kit_compiler* cc)
{
  kitc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    for (i64 i = (i64)scope->count - 1; i >= 0; i--) {
      u32        nexprs = scope->entries[i].nexprs;
      const int* exprs  = scope->entries[i].exprs;
      for (u32 j = 0; j < nexprs; j++) {
        int e = compile(cc, exprs[j]);
        if (e < 0) {
          cerror(KIT_GET_NODE(cc->ast, exprs[j])->common.span, "Failed to compile deferred statement [defer]\n");
          return e;
        }
      }
    }
    scope = scope->parent;
  }
  return 0;
}

static inline u32
defer_get_current_depth(kit_compiler* cc)
{
  u32               d     = 0;
  kitc_defer_scope* scope = cc->defer_stack;
  while (scope) {
    d++;
    scope = scope->parent;
  }
  return d;
}

// flush all defers up to (but not including) depth
static inline RETURNS_ERRCODE int
defer_emit_to_depth(kit_compiler* cc, u32 target_depth)
{
  kitc_defer_scope* scope = cc->defer_stack;
  u32               depth = defer_get_current_depth(cc);
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
 * Add the jump to the label's stream.
 * opcode is needed because there are multiple
 * jump instructions (JMP,JE,JNE,JZ,JNZ,etc.)
 */
static inline int
emit_and_record_jmp(kit_compiler* cc, eir_opcode opcode, kit_vreg_t condition, u32 label_id)
{
  switch (opcode) {
    case EIR_OPCODE_JMP: kit_emit_ins(cc, (kit_ins){ .jmp = { .opcode = EIR_OPCODE_JMP, .target = label_id } }); break;
    case EIR_OPCODE_JZ: kit_emit_ins(cc, (kit_ins){ .jz = { .opcode = EIR_OPCODE_JZ, .condition = condition, .target = label_id } }); break;
    case EIR_OPCODE_JNZ: kit_emit_ins(cc, (kit_ins){ .jnz = { .opcode = EIR_OPCODE_JNZ, .condition = condition, .target = label_id } }); break;

    default: return -1;
  }

  return 0;
}

static inline void
define_and_emit_label(kit_compiler* cc, u32 label_id)
{ kit_emit_ins(cc, (kit_ins){ .label = { .opcode = EIR_OPCODE_LABEL, .id = label_id } }); }

static inline RETURNS_ERRCODE int
compiler_make_fork(const kit_compiler* old_c, kit_compiler* new_c)
{
  kitc_scope* top_scope = old_c->scope;
  while (top_scope && top_scope->parent) { top_scope = top_scope->parent; }

  *new_c = (kit_compiler){
    .arena             = old_c->arena,
    .ast               = old_c->ast,
    .info              = old_c->info,
    .loop              = NULL, // reset loop on function.
    .lit_table         = old_c->lit_table,
    .builtin_var_table = old_c->builtin_var_table,
    .function_table    = old_c->function_table,
    .struct_table      = old_c->struct_table,
    .next_label        = old_c->next_label,
    .next_global       = old_c->next_global,
    .next_vreg         = KIT_REG_GENERAL_BEGIN, // Seperate register for each function
    .scope             = top_scope,
    .ns                = old_c->ns,
    .stack             = old_c->stack,
    .instructions      = (kit_ins*)kit_xalloc(sizeof(kit_ins), init_code_capacity),
    .ninstructions     = 0,
    .cinstructions     = init_code_capacity,
  };
  for (u32 i = 0; i < init_code_capacity; i++) { new_c->instructions[i] = (kit_ins){ .opcode = EIR_OPCODE_NOP }; }
  scope_push(new_c);
  return new_c->instructions ? 0 : -1;
}

static inline void
compiler_join_fork(kit_compiler* copy, kit_compiler* cc)
{
  /* The tables are stored on the main compile function stack. Their address SHOULD NOT change. */
  if (cc->lit_table != copy->lit_table || cc->builtin_var_table != copy->builtin_var_table || cc->function_table != copy->function_table
      || cc->ns != copy->ns) {
    puts("Compiler structure corrupted");
    // abort(); How remove this ?
  }

  /* Ensure we don't ever get two labels in different streams with the same ID */
  cc->next_label  = copy->next_label;
  cc->next_global = copy->next_global;

  /* Can't modify builtin variable count */
}

/**
 * for error paths. Free everything owned by this fork.
 */
static inline void
compiler_free_fork_entirely(kit_compiler* cc)
{
  free(cc->instructions);
  while (cc->defer_stack) defer_pop_scope(cc);
  memset(cc, 0, sizeof *cc);
}

static inline u32
make_label_id(kit_compiler* cc)
{ return cc->next_label++; }

static int
value_init(kit_compiler* cc, int node, val_t* d)
{
  val_t l = { 0 };

  switch (KIT_GET_NODE(cc->ast, node)->type) {
    case KIT_AST_NODE_VARIABLE: {
      kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
      char*        name = qualify_name(cc, KIT_GET_NODE(cc->ast, node)->ident.ident);

      u32 id = kit_hash(name, strlen(name));

      kitc_var* v = scope_lookup_info(cc, id);
      if (!v) {
        cerror(span, "Undeclared variable %s\n", name);
        return -1;
      }

      l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type         = v->is_global ? KIT_LVAL_GVAR : KIT_LVAL_VAR;
      l.val.var.id   = id;
      l.val.var.name = name;

      *d = l;
      return 0;
    }

    case KIT_AST_NODE_VARIABLE_DECL: {
      char* name = qualify_name(cc, KIT_GET_NODE(cc->ast, node)->let.name);

      u32       id = kit_hash(name, strlen(name));
      kitc_var* v  = scope_lookup_info(cc, id);
      if (!v) return -1;

      l.span         = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type         = v->is_global ? KIT_LVAL_GVAR : KIT_LVAL_VAR;
      l.val.var.id   = id;
      l.val.var.name = name;
      *d             = l;
      return 0;
    }

    case KIT_AST_NODE_INDEX: {
      l.span                 = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type                 = KIT_LVAL_INDEX;
      l.val.index.left_node  = KIT_GET_NODE(cc->ast, node)->index.base;
      l.val.index.index_node = KIT_GET_NODE(cc->ast, node)->index.index;
      *d                     = l;
      return 0;
    }

    case KIT_AST_NODE_INDEX_ASSIGN: {
      l.span                 = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type                 = KIT_LVAL_INDEX;
      l.val.index.left_node  = KIT_GET_NODE(cc->ast, node)->index_assign.base;
      l.val.index.index_node = KIT_GET_NODE(cc->ast, node)->index_assign.index;
      *d                     = l;
      return 0;
    }

    case KIT_AST_NODE_MEMBER_ACCESS: {
      l.span                   = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.type                   = KIT_LVAL_MEMBER;
      l.val.member.base        = KIT_GET_NODE(cc->ast, node)->member_access.left;
      l.val.member.member      = KIT_GET_NODE(cc->ast, node)->member_access.right;
      l.val.member.member_hash = kit_hash(l.val.member.member, strlen(l.val.member.member));
      *d                       = l;
      return 0;
    }

    case KIT_AST_NODE_MEMBER_ASSIGN: {
      l.type                   = KIT_LVAL_MEMBER;
      l.span                   = &KIT_GET_NODE(cc->ast, node)->common.span;
      l.val.member.base        = KIT_GET_NODE(cc->ast, node)->member_assign.left;
      l.val.member.member      = KIT_GET_NODE(cc->ast, node)->member_assign.right;
      l.val.member.member_hash = kit_hash(l.val.member.member, strlen(l.val.member.member));
      *d                       = l;
      return 0;
    }

    default:
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "%i can not be represented as a value (it is %u)\n", node, KIT_GET_NODE(cc->ast, node)->type);
      return -1;
  }

  return -1;
}

static void
value_free(val_t* lv)
{
  if (lv->type == KIT_LVAL_VAR) { /* free(lv->val.var.name); */
  }
  memset(lv, 0, sizeof(*lv));
}

static inline int
emit_lvalue_assign(kit_compiler* cc, kit_vreg_t value, val_t* lv)
{
  switch (lv->type) {
    case KIT_LVAL_VAR: {
      kitc_var* v = scope_lookup_info(cc, lv->val.var.id);
      if (!v) {
        cerror(*lv->span, "Undeclared variable %s\n", lv->val.var.name);
        return -1;
      }

      if (v->is_const) {
        cerror(*lv->span, "Can not assign to const variable %s\n", lv->val.var.name);
        return -1;
      }

      /* local to local operation. global to global operations need a temporary register. */
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = v->slot.reg, .src = value } });
      break;
    }
    case KIT_LVAL_GVAR: {
      kitc_var* v = scope_lookup_info(cc, lv->val.var.id);

      if (!v) {
        cerror(*lv->span, "Undeclared global variable %s\n", lv->val.var.name);
        return -1;
      }

      if (v->is_const) {
        cerror(*lv->span, "Can not assign to const global variable %s\n", lv->val.var.name);
        return -1;
      }

      /* writing to a global variable from a local register */
      kit_emit_ins(cc, (kit_ins){ .setg = { .opcode = EIR_OPCODE_SETG, .dst = v->slot.global_id, .src = value } });
      break;
    }
    case KIT_LVAL_INDEX: {
      kit_vreg_t base  = -1;
      kit_vreg_t index = -1;

      // base = compile(cc, lv->val.index.left_node);
      // if (base < 0) return base;

      // index = compile(cc, lv->val.index.index_node);
      // if (index < 0) return index;

      // kit_emit_ins(cc, (kit_ins){ .index_assign = { .opcode = EIR_OPCODE_INDEX_ASSIGN, .value = value, .base = base, .index = index } });

      val_t left = { 0 };
      if (value_init(cc, lv->val.index.left_node, &left) < 0) return -1;

      /* prep base */
      base = emit_lvalue_load(cc, &left);
      if (base < 0) return base;

      /* prep index */
      index = compile(cc, lv->val.index.index_node);
      if (index < 0) return index;

      /* index_assign modifies base. */
      kit_emit_ins(cc, (kit_ins){ .index_assign = { .opcode = EIR_OPCODE_INDEX_ASSIGN, .value = value, .base = base, .index = index } });

      /* write base back to its slot */
      if (emit_lvalue_assign(cc, base, &left) < 0) return -1;

      value_free(&left);
      break;
    }
    case KIT_LVAL_MEMBER: {
      int left      = lv->val.member.base;
      u32 member_id = lv->val.member.member_hash;

      val_t base_lv = { 0 };
      if (value_init(cc, left, &base_lv) < 0) return -1;

      /* prep base */
      kit_vreg_t base = emit_lvalue_load(cc, &base_lv);
      if (base < 0) return base;

      kit_emit_ins(cc, (kit_ins){ .member_assign = { .opcode = EIR_OPCODE_MEMBER_ASSIGN, .value = value, .base = base, .member_id = member_id } });

      /* write base back to its slot */
      if (emit_lvalue_assign(cc, base, &base_lv) < 0) return -1;

      value_free(&base_lv);
      break;
    }
    case KIT_LVAL_UNKNOWN: return -1;
  }

  /* Propogate assigned value */
  return value;
}

static int
emit_lvalue_load(kit_compiler* cc, val_t* lv)
{
  switch (lv->type) {
    case KIT_LVAL_VAR: {
      kit_vreg_t var_reg = scope_lookup_reg(cc, lv->val.var.id);
      if (var_reg < 0) return -1;

      return var_reg;
    }

    case KIT_LVAL_GVAR: {
      kitc_var* v = scope_lookup_info(cc, lv->val.var.id);
      if (!v) return -1;

      kit_vreg_t dst = vreg_alloc(cc);

      /* emit a GETG to fetch the global variable into our local register */
      kit_emit_ins(cc, (kit_ins){ .getg = { .opcode = EIR_OPCODE_GETG, .dst = dst, .src = v->slot.global_id } });
      return dst;
    }

    case KIT_LVAL_INDEX: {
      kit_vreg_t dst = vreg_alloc(cc);

      kit_vreg_t base  = -1;
      kit_vreg_t index = -1;

      base = compile(cc, lv->val.index.left_node);
      if (base < 0) return base;

      index = compile(cc, lv->val.index.index_node);
      if (index < 0) return index;

      kit_emit_ins(cc, (kit_ins){ .index = { .opcode = EIR_OPCODE_INDEX, .dst = dst, .base = base, .index = index } });

      return dst;
    }

    case KIT_LVAL_MEMBER: {
      int left      = lv->val.member.base;
      u32 member_id = lv->val.member.member_hash;

      /* pushes stack top */
      kit_vreg_t base = compile(cc, left);
      if (base < 0) return base;

      kit_vreg_t dst = vreg_alloc(cc);
      kit_emit_ins(cc, (kit_ins){ .member_access = { .opcode = EIR_OPCODE_MEMBER_ACCESS, .dst = dst, .base = base, .member_id = member_id } });

      return dst;
    }

    default:
    case KIT_LVAL_UNKNOWN: return -1;
  }

  abort();
  return -1;
}

static inline RETURNS_ERRCODE int
make_string_variable(kit_arena* a, char* s, kit_var* v) // s will be onwed by variable after this
{
  kit_refdobj* obj = kit_arnalloc(a, sizeof(kit_refdobj));
  if (!obj) return -1;

  obj->refc                 = 1;
  KIT_OBJ_AS_STRING(obj)->s = s;

  *v = (kit_var){ .type = KIT_VARTYPE_STRING, .val.s = obj };
  return 0;
}

static inline bool
is_literal_value(const kit_ast* ast, int node)
{
  kit_ast_node_type type = KIT_GET_NODE(ast, node)->type;

  if (type == KIT_AST_NODE_CALL) {
    const char* function_name = KIT_GET_NODE(ast, node)->call.function_name;
    const int*  args          = KIT_GET_NODE(ast, node)->call.args;
    u32         nargs         = KIT_GET_NODE(ast, node)->call.nargs;

    if (strcmp(function_name, "vec2") == 0 || strcmp(function_name, "vec3") == 0 || strcmp(function_name, "vec4") == 0) {
      bool constant_vector = true;
      for (u32 i = 0; i < nargs; i++) {
        if (!is_literal_value(ast, args[i])) {
          constant_vector = false;
          break;
        }
      }

      return constant_vector;
    }
  }
  return type == KIT_AST_NODE_INT || type == KIT_AST_NODE_CHAR || type == KIT_AST_NODE_BOOL || type == KIT_AST_NODE_STRING
      || type == KIT_AST_NODE_FLOAT;
}

static inline RETURNS_ERRCODE int
convert_node_to_literal(kit_compiler* cc, int node, kit_var* o)
{
  kit_ast* p = cc->ast;
  switch (KIT_GET_NODE(p, node)->type) {
    case KIT_AST_NODE_INT: *o = (kit_var){ .type = KIT_VARTYPE_INT, .val.i = KIT_GET_NODE(p, node)->i.i }; return 0;
    case KIT_AST_NODE_FLOAT: *o = (kit_var){ .type = KIT_VARTYPE_FLOAT, .val.f = KIT_GET_NODE(p, node)->f.f }; return 0;
    case KIT_AST_NODE_CHAR: *o = (kit_var){ .type = KIT_VARTYPE_CHAR, .val.c = KIT_GET_NODE(p, node)->c.c }; return 0;
    case KIT_AST_NODE_BOOL: *o = (kit_var){ .type = KIT_VARTYPE_BOOL, .val.b = KIT_GET_NODE(p, node)->b.b }; return 0;
    case KIT_AST_NODE_STRING: return make_string_variable(cc->arena, kit_arnstrdup(cc->arena, KIT_GET_NODE(p, node)->s.s), o);

    case KIT_AST_NODE_CALL: {
      const char* function_name = KIT_GET_NODE(cc->ast, node)->call.function_name;
      const int*  args          = KIT_GET_NODE(cc->ast, node)->call.args;
      u32         nargs         = KIT_GET_NODE(cc->ast, node)->call.nargs;

      if (strcmp(function_name, "vec2") == 0 || strcmp(function_name, "vec3") == 0 || strcmp(function_name, "vec4") == 0) {
        return fold_vector(cc, args, nargs);
      }
    }

    case KIT_AST_NODE_LIST:
    case KIT_AST_NODE_MAP: /* TODO: Implement */ abort(); break;

    default: return 1;
  }
  return 1;
}

static RETURNS_ERRCODE int
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

static void
ns_pop(kit_compiler* cc)
{
  cc->ns->nnamespaces--;
  /* free(cc->ns->namespaces[--cc->ns->nnamespaces]); */
}

/**
 * Use the namespace stack to build
 * a fully qualified name for the variable.
 */
static char*
qualify_name(const kit_compiler* cc, const char* name)
{
  size_t len = strlen(name) + 1;

  for (u32 i = 0; i < cc->ns->nnamespaces; i++) {
    len += strlen(cc->ns->namespaces[i]) + 2; // ::
  }

  char* out = kit_arnalloc(cc->arena, len);
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

static int
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

static RETURNS_ERRCODE int
add_literal_to_track(kit_compiler* cc, const kit_var* v)
{
  kitc_literal_table* literals = cc->lit_table;
  u32                 hash     = kit_var_hash(v);
  bool                found    = false;

  for (u32 i = 0; i < literals->literals_count; i++) {
    if (literals->literal_hashes[i] == hash && kit_var_equal(v, &literals->literals[i])) {
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
compile_and_push_literal_variable(kit_compiler* cc, const kit_var* v)
{
  /* OPTIMIZATION: If the variable is an integer or a float, emit a MOVI or a MOVF  */
  if (cc->info->opt_level >= 1 && v->type == KIT_VARTYPE_INT) {
    kit_vreg_t dst = vreg_alloc(cc);
    kit_emit_ins(cc, (kit_ins){ .movi = { .opcode = EIR_OPCODE_MOVI, .dst = dst, .value = v->val.i } });
    return dst;
  }

  if (cc->info->opt_level >= 1 && v->type == KIT_VARTYPE_FLOAT) {
    kit_vreg_t dst = vreg_alloc(cc);
    kit_emit_ins(cc, (kit_ins){ .movf = { .opcode = EIR_OPCODE_MOVF, .dst = dst, .value = v->val.f } });
    return dst;
  }

  if (cc->info->opt_level >= 1 && v->type == KIT_VARTYPE_NULL) { return KIT_REG_NIL; }

  /* Search for the literal in our table. */
  u32 hash = kit_var_hash(v);

  int e = add_literal_to_track(cc, v);
  if (e < 0) return e;

  kit_vreg_t dst = vreg_alloc(cc);

  kit_emit_ins(cc, (kit_ins){ .loadk = { .opcode = EIR_OPCODE_LOADK, .dst = dst, .id = hash } });

  return dst;
}

static RETURNS_ERRCODE kit_vreg_t
compile_literal(kit_compiler* cc, int node)
{
  // Convert the node to a variable and
  kit_var v = { .type = KIT_VARTYPE_NULL };
  if (convert_node_to_literal(cc, node, &v)) return -1;

  // Compile it
  return compile_and_push_literal_variable(cc, &v);
}

static RETURNS_ERRCODE int
compile_list(kit_compiler* cc, int node)
{
  u32 nelems = KIT_GET_NODE(cc->ast, node)->list.nelems;

  kit_vreg_t dst = vreg_alloc(cc);

  kit_vreg_t* elems = kit_arnalloc(cc->arena, sizeof(kit_vreg_t) * nelems);

  for (u32 i = 0; i < nelems; i++) {
    int elem_node = KIT_GET_NODE(cc->ast, node)->list.elems[i];

    elems[i] = compile(cc, elem_node);
    if (elems[i] < 0) return elems[i];
  }

  for (u32 i = 0; i < nelems; i++) {
    /* Move at most 16 elements from their registers to our argument list. */
    if (i < KIT_REG_ARG_COUNT) kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = KIT_REG_ARG0 + i, .src = elems[i] } });
    /* else push them to the stack */
    else kit_emit_ins(cc, (kit_ins){ .push = { .opcode = EIR_OPCODE_PUSH, .reg = elems[i] } });
  }

  kit_emit_ins(cc, (kit_ins){ .mk_list = { .opcode = EIR_OPCODE_MK_LIST, .dst = dst, .nelems = nelems } });

  /* remove spilled elements from the stack */
  kit_vreg_t tmp = vreg_alloc(cc);
  for (u32 i = 16; i < nelems; i++) {
    /* pop repeatedly into tmp */
    kit_emit_ins(cc, (kit_ins){ .pop = { .opcode = EIR_OPCODE_POP, .reg = tmp } });
  }

  return dst;
}

static RETURNS_ERRCODE int
compile_map(kit_compiler* cc, int node)
{
  u32 npairs = KIT_GET_NODE(cc->ast, node)->map.npairs;

  kit_vreg_t dst = vreg_alloc(cc);

  kit_vreg_t* pairs = kit_arnalloc(cc->arena, npairs * 2ULL * sizeof(kit_vreg_t));

  for (u32 i = 0; i < npairs; i++) {
    int key = KIT_GET_NODE(cc->ast, node)->map.keys[i];
    int val = KIT_GET_NODE(cc->ast, node)->map.values[i];

    kit_vreg_t k = compile(cc, key);
    if (k < 0) return k;

    kit_vreg_t v = compile(cc, val);
    if (v < 0) return v;

    pairs[((size_t)i * 2)]     = k;
    pairs[((size_t)i * 2) + 1] = v;
  }

  for (u32 i = 0; i < npairs; i++) {
    /* copy the first 8 pairs (16 registers) and spill the rest to the stack */
    kit_vreg_t k = pairs[((size_t)i * 2)];
    kit_vreg_t v = pairs[((size_t)i * 2) + 1];

    if (i < (KIT_REG_ARG_COUNT / 2)) {
      /* copy KV pairs into our argument vector */
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = KIT_REG_ARG0 + (i * 2), .src = k } });
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = KIT_REG_ARG0 + (i * 2) + 1, .src = v } });
    } else {
      /* spill! */
      kit_emit_ins(cc, (kit_ins){ .push = { .opcode = EIR_OPCODE_PUSH, .reg = k } });
      kit_emit_ins(cc, (kit_ins){ .push = { .opcode = EIR_OPCODE_PUSH, .reg = v } });
    }
  }

  kit_emit_ins(cc, (kit_ins){ .mk_map = { .opcode = EIR_OPCODE_MK_MAP, .dst = dst, .npairs = npairs } });

  /* Cleanup the stack. */
  kit_vreg_t tmp = vreg_alloc(cc);
  for (u32 i = (KIT_REG_ARG_COUNT / 2); i < npairs; i++) {
    /* pop repeatedly into tmp */
    kit_emit_ins(cc, (kit_ins){ .pop = { .opcode = EIR_OPCODE_POP, .reg = tmp } });
  }

  return dst;
}

static int
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

static inline int
codegraph_rebuild(kit_compiler* cc, codegraph* cfg)
{
  int e = codegraph_build_successor_list(cc, cfg);
  if (e < 0) return e;

  e = codegraph_liveliness_analysis(cc, cfg);
  if (e < 0) return e;

  return 0;
}

static kit_vreg_t
compile_function_definition(kit_compiler* cc, int node)
{
  kit_compiler fork          = { 0 };
  const char*  function_name = KIT_GET_NODE(cc->ast, node)->func.name;
  char*        full          = qualify_name(cc, function_name);

  const u32 hash = kit_hash(full, strlen(full));

  int e = 0;

  /* Ensure it doesn't already exist */
  const kitc_function_table* func_table = cc->function_table;
  for (u32 i = 0; i < func_table->functions_count; i++) {
    kitc_function* func = &func_table->functions[i];
    if (func->name_hash == hash) {
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Function \"%s\" redefined [function definition]\n", function_name);
      e = -1;
      goto ERR;
    }
  }

  /* Ensure we aren't overriding a builtin function */
  for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
    const kit_builtin_func* func      = &kit_builtins_funcs[i];
    u32                     func_hash = kit_hash(func->name, strlen(func->name));
    if (func_hash == hash) {
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Attempt to overshadow builtin function \"%s\" [function definition]\n", function_name);
      e = -1;
      goto ERR;
    }
  }

  u32 nargs = KIT_GET_NODE(cc->ast, node)->func.nargs;

  e = compiler_make_fork(cc, &fork);
  if (e < 0) goto ERR;

  /* Setup argument variables so the function knows they exist. */
  for (u32 i = 0; i < nargs; i++) {
    const char*  arg_name = KIT_GET_NODE(cc->ast, node)->func.args[i];
    kit_filespan span     = KIT_GET_NODE(cc->ast, node)->func.span;

    char* full_arg_name = qualify_name(&fork, arg_name);
    if (!full_arg_name) goto ERR;

    /* Move argument variables to local registers so they aren't overridden accidentally */
    kit_vreg_t dst = vreg_alloc(&fork);

    /* argument in vector, mov it to our register */
    if (i < KIT_REG_ARG_COUNT) {
      kit_vreg_t src = (kit_vreg_t)(KIT_REG_ARG0 + i);
      kit_emit_ins(&fork, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = dst, .src = src } });
    }
    /* argument on stack, mov it to our register */
    else {
      kit_emit_ins(&fork, (kit_ins){ .pop = { .opcode = EIR_OPCODE_POP, .reg = dst } });
    }

    /* Define argument variable */
    scope_define_in_register(&fork, dst, span, full_arg_name, false);
  }

  e = defer_push_scope(&fork);
  if (e < 0) goto ERR;

  e = compile_function(&fork, node);
  if (e < 0) {
    kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
    cerror(span, "Failed to compile function body [function definition]\n");
    goto ERR;
  }

  // emit the defer before the return you asshole
  e = defer_emit_current_scope(&fork);
  if (e < 0) goto ERR;

  defer_pop_scope(&fork);

  if (cc->info->opt_level >= 2) {
    kit_arena codegraph_arena = { 0 };
    if (kit_arena_init(1, &codegraph_arena) < 0) return -1;

    /* constant folding needs a clean instruction stream */
    if (!fork.info->feature_set.disable_function_inlining) { opt_inline_function_calls(&fork); }

    for (u32 pass = 0; pass < 4; pass++) {
      if (kit_arena_reset(&codegraph_arena) < 0) return -1;
      codegraph cfg = { 0 };

      e = codegraph_init(&codegraph_arena, &fork, &cfg);
      if (e < 0) goto ERR;

      // codegraph_preliminary_dead_store_elimination(&fork, &cfg);

      if (!fork.info->feature_set.disable_local_copy_propagation) {
        if (codegraph_local_copy_propagation(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      }
      // if (!fork.info->feature_set.disable_dead_store_elimination) {
      //   if (codegraph_dead_store_elimination(&fork, &cfg)) codegraph_rebuild(&fork, &cfg);
      // }
      // if (!fork.info->feature_set.disable_constant_propagation) {
      //   if (codegraph_constant_propagation(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      // }
      // if (!fork.info->feature_set.disable_constant_folding) {
      //   if (codegraph_constant_folding(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      // }
      // if (!fork.info->feature_set.disable_redundant_move_elimination) {
      //   if (codegraph_redundant_move_elimination(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      // }
      if (!fork.info->feature_set.disable_dead_branch_elimination) {
        if (codegraph_eliminate_unreachable_code(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      }

      /* codegraph invalidated after these calls */
      if (!fork.info->feature_set.disable_redundant_jump_elimination) { remove_jmp_where_it_would_fallthrough(&fork); }
      if (!fork.info->feature_set.disable_noop_stripping) { strip_noops(&fork); }
      // if (!fork.info->feature_set.disable_dead_move_forwarding) { forward_dead_moves(&fork); }

      codegraph_free(&fork, &cfg);
    }

    /* OUT OF THE LOOP!!!  */
    kit_arena_free(&codegraph_arena);
  }

  // if (!fork.info->feature_set.disable_register_allocation_i_know_what_im_doing) era_register_allocation_pass(&fork);
  // fork.ninstructions = label_pass(fork.arena, fork.instructions, fork.ninstructions, fork.next_label);

  compiler_join_fork(&fork, cc);

  /* write our info to the table */
  kitc_function f = {
    .code        = fork.instructions,
    .code_count  = fork.ninstructions,
    .name_hash   = hash,
    .nargs       = nargs,
    .vregs_used  = fork.next_vreg,
    .labels_used = fork.next_label,
  };

  e = append_function_entry(cc->arena, cc->function_table, &f);
  if (e < 0) goto ERR;

  return e;

ERR:
  compiler_free_fork_entirely(&fork);
  return e == 0 ? -1 : e;
}

static u32
next_real_ins(kit_compiler* cc, u32 start)
{
  for (u32 i = start; i < cc->ninstructions; i++) {
    eir_opcode op = cc->instructions[i].opcode;
    if (op != EIR_OPCODE_NOP && op != EIR_OPCODE_LABEL) return i;
  }
  return UINT32_MAX;
}

static kit_vreg_t
compile_binary_op(kit_compiler* cc, int node)
{
  val_t lv = { 0 };

  bool is_compound = KIT_GET_NODE(cc->ast, node)->binaryop.is_compound;
  int  left        = KIT_GET_NODE(cc->ast, node)->binaryop.left;
  int  right       = KIT_GET_NODE(cc->ast, node)->binaryop.right;

  eir_opcode opcode = kit_binary_operator_to_opcode(KIT_GET_NODE(cc->ast, node)->binaryop.op);
  if (opcode < 0) {
    cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a binary operator\n", KIT_GET_NODE(cc->ast, node)->binaryop.op);
    goto err;
  }

  /* optimization level 2 because requires a bit of work here and produces minimal gains */
  if (cc->info->opt_level >= 2 && is_literal_value(cc->ast, left) && is_literal_value(cc->ast, right)) {
    kit_var lhs = KIT_NULLVAR;
    kit_var rhs = KIT_NULLVAR;

    int e = convert_node_to_literal(cc, left, &lhs);
    if (e < 0) return e;

    e = convert_node_to_literal(cc, right, &rhs);
    if (e < 0) return e;

    kit_var result = operate(lhs, rhs, opcode);

    /* both input operands are constant. result must be a constant too */
    return compile_and_push_literal_variable(cc, &result);
  }

  if (is_compound && !can_make_value(cc->ast, left)) {
    cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Can not assign to left\n");
    goto err;
  }

  kit_vreg_t dst = vreg_alloc(cc);

  if (is_compound) {
    // Verified  earlier that we can make it into an lvalue
    int e = value_init(cc, left, &lv);
    if (e < 0) goto err;

    /* Load left */
    kit_vreg_t l = emit_lvalue_load(cc, &lv);
    if (l < 0) goto err;

    /* Load right */
    kit_vreg_t r = compile(cc, right);
    if (r < 0) goto err;

    /* Emit operator */
    kit_emit_ins(cc, (kit_ins){ .binop = { .opcode = opcode, .dst = dst, .a = l, .b = r } });

    if (emit_lvalue_assign(cc, dst, &lv) < 0) return -1;

    value_free(&lv);
  } else {
    kit_vreg_t a = compile(cc, left);
    if (a < 0) goto err;

    kit_vreg_t b = compile(cc, right);
    if (b < 0) goto err;

    kit_emit_ins(cc, (kit_ins){ .binop = { .opcode = opcode, .dst = dst, .a = a, .b = b } });
  }

  return dst;

err:
  value_free(&lv);
  return -1;
}

static kit_vreg_t
compile_inc_or_dec(kit_compiler* cc, int node)
{
  eir_opcode opcode = -1;
  if (KIT_GET_NODE(cc->ast, node)->unaryop.op == KIT_OPERATOR_INC) {
    opcode = EIR_OPCODE_ADD;
  } else if (KIT_GET_NODE(cc->ast, node)->unaryop.op == KIT_OPERATOR_DEC) {
    opcode = EIR_OPCODE_SUB;
  }

  int right = KIT_GET_NODE(cc->ast, node)->unaryop.right;

  if (KIT_GET_NODE(cc->ast, right)->type != KIT_AST_NODE_VARIABLE) {
    cerror(KIT_GET_NODE(cc->ast, right)->common.span, "Can only increment/decrement variables\n");
    return -1;
  }

  val_t lv = { 0 };
  int   e  = value_init(cc, right, &lv);
  if (e < 0) return -1;

  kit_vreg_t rhs = emit_lvalue_load(cc, &lv);

  kit_var    one     = kit_var_from_int(1);
  kit_vreg_t one_reg = compile_and_push_literal_variable(cc, &one);

  kit_vreg_t tmp = vreg_alloc(cc);
  kit_emit_ins(cc, (kit_ins){ .binop = { .opcode = opcode, .dst = tmp, .a = rhs, .b = one_reg } });

  /* Emit operator, both dst and a point to the variables slot */
  tmp = emit_lvalue_assign(cc, tmp, &lv);
  if (tmp < 0) return tmp;

  /* Propogate new value */
  return tmp;
}

static kit_vreg_t
compile_unary_op(kit_compiler* cc, int node)
{
  /* Special case, INC and DEC assign to variables directly. */
  kit_operator oper = KIT_GET_NODE(cc->ast, node)->unaryop.op;
  if (oper == KIT_OPERATOR_INC || oper == KIT_OPERATOR_DEC) { return compile_inc_or_dec(cc, node); }

  int        right       = KIT_GET_NODE(cc->ast, node)->unaryop.right;
  bool       is_compound = KIT_GET_NODE(cc->ast, node)->unaryop.is_compound;
  eir_opcode opcode      = -1;

  switch (KIT_GET_NODE(cc->ast, node)->unaryop.op) {
    case KIT_OPERATOR_NOT: opcode = EIR_OPCODE_NOT; break;
    case KIT_OPERATOR_BNOT: opcode = EIR_OPCODE_BNOT; break;
    case KIT_OPERATOR_SUB: opcode = EIR_OPCODE_NEG; break;
    case KIT_OPERATOR_ADD: opcode = EIR_OPCODE_NOP; break;
    default:
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a unary operator\n", KIT_GET_NODE(cc->ast, node)->unaryop.op);
      return -1;
  }

  /* opt level 2 because minimal gains */
  if (cc->info->opt_level >= 2 && is_literal_value(cc->ast, right)) {
    kit_var rhs = KIT_NULLVAR;

    int e = convert_node_to_literal(cc, right, &rhs);
    if (e < 0) return e;

    kit_var result = operate(KIT_NULLVAR, rhs, opcode);

    /* both input operands are constant. result must be a constant too */
    return compile_and_push_literal_variable(cc, &result);
  }

  int e = 0;

  kit_vreg_t dst = vreg_alloc(cc);

  if (is_compound) {
    val_t lv = { 0 };

    if (!can_make_value(cc->ast, right)) {
      cerror(KIT_GET_NODE(cc->ast, right)->common.span, "Can not assign to right\n");
      return -1;
    }

    // Verified  earlier that we can make it into an lvalue
    e = value_init(cc, right, &lv);
    if (e < 0) goto err;

    /* Load right */
    kit_vreg_t r = compile(cc, right);
    if (r < 0) goto err;

    /* Emit operator */
    kit_emit_ins(cc, (kit_ins){ .unop = { .opcode = opcode, .dst = dst, .a = r } });

    /* Emit actual assign instruction (takes value produced earlier and assigns it) */
    dst = emit_lvalue_assign(cc, dst, &lv);
    if (dst < 0) goto err;

    value_free(&lv);
  } else {
    /* Load right */
    kit_vreg_t rreg = compile(cc, right);
    if (rreg < 0) goto err;

    /* Emit operator */
    kit_emit_ins(cc, (kit_ins){ .unop = { .opcode = opcode, .dst = dst, .a = rreg } });
  }

  return dst;

err:
  return -1;
}

static kit_vreg_t
compile_index(kit_compiler* cc, int node)
{
  kit_vreg_t base = compile(cc, KIT_GET_NODE(cc->ast, node)->index.base);
  if (base < 0) return base;

  kit_vreg_t index = compile(cc, KIT_GET_NODE(cc->ast, node)->index.index);
  if (index < 0) return index;

  kit_vreg_t dst = vreg_alloc(cc);
  kit_emit_ins(cc, (kit_ins){ .index = { .opcode = EIR_OPCODE_INDEX, .dst = dst, .base = base, .index = index } });

  return dst;
}

static kit_vreg_t
compile_index_assign(kit_compiler* cc, int node)
{
  int value = KIT_GET_NODE(cc->ast, node)->index_assign.value;

  if (!can_make_value(cc->ast, node)) {
    kit_filespan left_span = KIT_GET_NODE(cc->ast, node)->index_assign.span;
    cerror(left_span, "Can not assign to indexed expression: Failed to lower to lvalue\n");
    return -1;
  }

  val_t v = { 0 };

  int e = value_init(cc, node, &v);
  if (e < 0) return e;

  kit_vreg_t eval_value = compile(cc, value);
  kit_vreg_t dst        = emit_lvalue_assign(cc, eval_value, &v);
  value_free(&v);

  return dst;
}

static inline bool
is_builtin_func(const char* name)
{
  for (size_t i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
    if (strcmp(kit_builtins_funcs[i].name, name) == 0) return true;
  }
  return false;
}

static inline const kit_builtin_func*
get_builtin_func(const char* name)
{
  for (size_t i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
    if (strcmp(kit_builtins_funcs[i].name, name) == 0) return &kit_builtins_funcs[i];
  }
  return NULL;
}

static inline int
fold_vector(kit_compiler* cc, const int* elems, u32 nelems)
{
  kit_vec4 vector = { 0 };
  for (u32 i = 0; i < nelems; i++) {
    kit_var elem = KIT_NULLVAR;
    int     e    = convert_node_to_literal(cc, elems[i], &elem);
    if (e < 0) return e;

    double d  = kit_cast_to_float(&elem);
    vector[i] = d;
  }

  kit_var v = {
    .type = KIT_VARTYPE_NULL,
  };
  switch (nelems) {
    default:
    case 2:
      v.type = KIT_VARTYPE_VEC2;
      memcpy(v.val.vec2, vector, sizeof(kit_vec2));
      break;
    case 3:
      v.type = KIT_VARTYPE_VEC3;
      memcpy(v.val.vec3, vector, sizeof(kit_vec3));
      break;
    case 4:
      v.type = KIT_VARTYPE_VEC4;
      memcpy(v.val.vec4, vector, sizeof(kit_vec4));
      break;
  }

  /* Compile this vector into a literal */
  kit_vreg_t compiled = compile_and_push_literal_variable(cc, &v);
  if (compiled < 0) return compiled;

  return compiled;
}

static kit_vreg_t
compile_function_call(kit_compiler* cc, int node)
{
  kit_filespan function_span = KIT_GET_NODE(cc->ast, node)->common.span;
  u32          nargs         = KIT_GET_NODE(cc->ast, node)->call.nargs;
  int*         args          = KIT_GET_NODE(cc->ast, node)->call.args;

  const char* function_name = KIT_GET_NODE(cc->ast, node)->call.function_name;
  kit_str_intern(function_name, cc->ast->interner);

  char* full = qualify_name(cc, function_name);
  u32   hash = kit_hash(full, strlen(full));

  if (is_builtin_func(function_name)) {
    /* Validate arguments */
    const kit_builtin_func* func = get_builtin_func(function_name);

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
    kitc_function*       func       = NULL;
    kitc_function_table* func_table = cc->function_table;
    for (u32 i = 0; i < func_table->functions_count; i++) {
      if (func_table->functions[i].name_hash == hash) {
        func = &func_table->functions[i];
        break;
      }
    }

    if (func && func->nargs != nargs) {
      cerror(
          KIT_GET_NODE(cc->ast, node)->common.span,
          "User defined function '%s' expects [%u] arguments, but [%u] were given\n",
          function_name,
          func->nargs,
          nargs);
      return -1;
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

      if (constant_vector) { return fold_vector(cc, args, nargs); }
    }
  }

  int e = 0;

  kit_vreg_t* arg_registers = (kit_vreg_t*)kit_arnalloc(cc->arena, sizeof(kit_vreg_t) * nargs);
  for (u32 i = 0; i < nargs; i++) {
    arg_registers[i] = compile(cc, args[i]); // Pushes stack top
    if (arg_registers[i] < 0) {
      cerror(KIT_GET_NODE(cc->ast, args[i])->common.span, "Failed to compile argument #%i [function call]\n", i);
      return e;
    }
  }

  /* compiled all of them. now move them to our registers. */
  for (u32 i = 0; i < nargs; i++) {
    /* move to our arguments register */
    if (i < KIT_REG_ARG_COUNT) {
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = KIT_REG_ARG0 + i, .src = arg_registers[i] } });
    } else {
      /* Spill the rest of the arguments on the stack */
      kit_emit_ins(cc, (kit_ins){ .push = { .opcode = EIR_OPCODE_PUSH, .reg = arg_registers[i] } });
    }
  }

  kit_vreg_t dst = vreg_alloc(cc);

  kit_emit_ins(cc, (kit_ins){ .call = { .opcode = EIR_OPCODE_CALL, .dst = dst, .function_id = hash, .nargs = nargs } });

  /* Cleanup the stack. */
  kit_vreg_t tmp = vreg_alloc(cc);
  for (u32 i = KIT_REG_ARG_COUNT; i < nargs; i++) {
    /* pop repeatedly into tmp */
    kit_emit_ins(cc, (kit_ins){ .pop = { .opcode = EIR_OPCODE_POP, .reg = tmp } });
  }

  return dst;
}

// This is the dirtiest of them all...
static kit_vreg_t
compile_if_statement(kit_compiler* cc, int node)
{
  /* Label after if statements body */
  u32 end_label = make_label_id(cc);

  const int condition = KIT_GET_NODE(cc->ast, node)->if_stmt.condition;
  int       e         = 0;

  /**
   * Label of the next else if / else to jump to. Still used if no branches are present, there's
   * just a jmp instruction directly after the JMP to where the branch would be.
   */
  u32 next_in_chain_label = make_label_id(cc);

  scope_push(cc);

  // condition
  kit_vreg_t cond = compile(cc, condition);
  if (cond < 0) {
    cerror(KIT_GET_NODE(cc->ast, condition)->common.span, "Failed to compile condition of top if statement [if statement]\n");
    goto ERR;
  }

  // condition failed :<
  e = emit_and_record_jmp(cc, EIR_OPCODE_JZ, cond, next_in_chain_label); // Jump to the next in chain. else if/else/end of if statement
  if (e < 0) goto ERR;

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  // BODY OF ROOT IF STATEMENT
  const int* if_body = KIT_GET_NODE(cc->ast, node)->if_stmt.body;
  for (u32 i = 0; i < KIT_GET_NODE(cc->ast, node)->if_stmt.nstmts; i++) {
    e = compile(cc, if_body[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, if_body[i])->common.span, "Failed to compile if statement body [if statement]\n");
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
  for (u32 else_if_idx = 0; else_if_idx < KIT_GET_NODE(cc->ast, node)->if_stmt.nelse_ifs; else_if_idx++) {
    // Emit the next in chain label for instructions to jump to.
    define_and_emit_label(cc, next_in_chain_label);
    next_in_chain_label = make_label_id(cc);

    scope_push(cc);

    // dont worry about it dont worry about it dont worry about it dont worry about it
    struct kit_if_stmt* elif = &KIT_GET_NODE(cc->ast, node)->if_stmt.else_ifs[else_if_idx];

    // CONDITION
    kit_vreg_t elif_cond = compile(cc, elif->condition);
    if (elif_cond < 0) {
      cerror(KIT_GET_NODE(cc->ast, elif->condition)->common.span, "Failed to compile condition of else if statement [if statement]\n");
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
        cerror(KIT_GET_NODE(cc->ast, elif->body[i])->common.span, "Failed to compile body of else if statement [if statement]\n");
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
  int* else_body = KIT_GET_NODE(cc->ast, node)->if_stmt.else_body;
  for (u32 i = 0; i < KIT_GET_NODE(cc->ast, node)->if_stmt.nelse_stmts; i++) {
    e = compile(cc, else_body[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, else_body[i])->common.span, "Failed to compile body of else statement [if statement]\n");
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
static kit_vreg_t
compile_while_statement(kit_compiler* cc, int node)
{
  int e = 0;

  /* Computes the condition and jumps to the end label (breaks) if condition is false */
  const u32 pre_condition_label = make_label_id(cc);

  /* After the while loop, with one POP_VARIABLES to ensure we always pop our variables. */
  const u32 end_label = make_label_id(cc);

  define_and_emit_label(cc, pre_condition_label);

  /* CONDITION */
  kit_vreg_t cond = compile(cc, KIT_GET_NODE(cc->ast, node)->while_stmt.condition);
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
  kitc_loop_location loop = {
    .continue_label = pre_condition_label,
    .break_label    = end_label,
    .defer_depth    = defer_get_current_depth(cc),
  };

  /* Ensure we don't overwrite it... */
  kitc_loop_location* last = cc->loop;
  cc->loop                 = &loop;

  // WHILE BODY
  const int* while_body = KIT_GET_NODE(cc->ast, node)->while_stmt.stmts;
  for (u32 i = 0; i < KIT_GET_NODE(cc->ast, node)->while_stmt.nstmts; i++) {
    e = compile(cc, while_body[i]);
    if (e < 0) goto ERR;
  }

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  /* Jump to condition, body is done executing */
  e = emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, pre_condition_label);
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

static kit_vreg_t
compile_for_statement(kit_compiler* cc, int node)
{
  int        initializers = -1;
  kit_vreg_t cond_reg     = -1; /* condition */
  kit_vreg_t init_reg     = -1; /* initializers */

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
  kitc_loop_location loop = {
    .continue_label = iterator_label,
    .break_label    = end_label,
    .defer_depth    = defer_get_current_depth(cc),
  };

  /* Ensure we don't overwrite it... */
  kitc_loop_location* last = cc->loop;
  cc->loop                 = &loop;

  // See comment over while loop compilation
  scope_push(cc);

  // INITIALIZERS
  initializers = KIT_GET_NODE(cc->ast, node)->for_stmt.initializers;

  if (initializers >= 0) {
    init_reg = compile(cc, initializers);
    if (init_reg < 0) {
      cerror(KIT_GET_NODE(cc->ast, initializers)->common.span, "Failed to compile initializers [for statement]\n");
      goto ERR;
    }
  }

  // TOP_LABEL
  define_and_emit_label(cc, top_label);

  // CONDITION
  int cond = KIT_GET_NODE(cc->ast, node)->for_stmt.condition;
  cond_reg = compile(cc, cond);
  if (cond_reg < 0) {
    cerror(KIT_GET_NODE(cc->ast, cond_reg)->common.span, "Failed to compile condition [for statement]");
    goto ERR;
  }

  // JZ END_LABEL
  e = emit_and_record_jmp(cc, EIR_OPCODE_JZ, cond_reg, end_label);
  if (e < 0) goto ERR;

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  u32 old_depth = cc->loop->defer_depth;
  if (cc->loop) cc->loop->defer_depth = defer_get_current_depth(cc);

  // BODY
  u32        nstmts = KIT_GET_NODE(cc->ast, node)->for_stmt.nstmts;
  const int* stmts  = KIT_GET_NODE(cc->ast, node)->for_stmt.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    if (compile(cc, stmts[i]) < 0) {
      cerror(KIT_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement in body [for statement]\n");
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
  u32        niterators = KIT_GET_NODE(cc->ast, node)->for_stmt.niterators;
  const int* iterators  = KIT_GET_NODE(cc->ast, node)->for_stmt.iterators;
  for (u32 i = 0; i < niterators; i++) {
    if (compile(cc, iterators[i]) < 0) {
      cerror(KIT_GET_NODE(cc->ast, iterators[i])->common.span, "Failed to compile iterators [for statement]");
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

static kit_vreg_t
compile_ranged_for_statement(kit_compiler* cc, int node)
{
  const int*  stmts    = KIT_GET_NODE(cc->ast, node)->for_range_stmt.stmts;
  u32         nstmts   = KIT_GET_NODE(cc->ast, node)->for_range_stmt.nstmts;
  int         start    = KIT_GET_NODE(cc->ast, node)->for_range_stmt.start;
  int         stop     = KIT_GET_NODE(cc->ast, node)->for_range_stmt.stop;
  const char* iterator = KIT_GET_NODE(cc->ast, node)->for_range_stmt.iterator_name;

  const char* interned_iterator = kit_str_intern(iterator, cc->ast->interner);
  char*       full_iterator     = qualify_name(cc, interned_iterator);

  kit_filespan span = KIT_GET_NODE(cc->ast, node)->for_range_stmt.span;

  kitc_loop_location* old_loop = cc->loop;

  scope_push(cc);
  if (defer_push_scope(cc) < 0) goto ERR;

  kitc_var* existing = scope_lookup_info(cc, kit_hash(full_iterator, strlen(full_iterator)));

  kit_vreg_t iterator_reg = -1;
  if (existing) {
    iterator_reg = existing->slot.reg;
  } else {
    iterator_reg = scope_define(cc, span, full_iterator, false);
  }

  /* Compile start value and move it to our register */
  kit_vreg_t iterator_start_value = compile(cc, start);
  kit_vreg_t iterator_stop_value  = compile(cc, stop);

  /* compile increment amount */
  kit_vreg_t iterator_increment_amount             = vreg_alloc(cc);
  kit_vreg_t iterator_increment_amount_calculation = vreg_alloc(cc);

  u32 reverse_iterator_direction_label         = make_label_id(cc);
  u32 end_iterator_direction_calculation_label = make_label_id(cc);

  kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = iterator_reg, .src = iterator_start_value } });

  /* calc = stop < start [for i in 10..0]*/
  kit_emit_ins(
      cc,
      (kit_ins){
          .binop = { .opcode = EIR_OPCODE_LT, .dst = iterator_increment_amount_calculation, .a = iterator_stop_value, .b = iterator_start_value } });

  /* jnz calc reverse_direction */
  kit_emit_ins(
      cc,
      (kit_ins){
          .jnz = { .opcode = EIR_OPCODE_JNZ, .target = reverse_iterator_direction_label, .condition = iterator_increment_amount_calculation } });

  /* jnz failed, normal iterator direction */
  kit_emit_ins(cc, (kit_ins){ .movi = { .opcode = EIR_OPCODE_MOVI, .dst = iterator_increment_amount, .value = 1 } });
  kit_emit_ins(cc, (kit_ins){ .jmp = { .opcode = EIR_OPCODE_JMP, .target = end_iterator_direction_calculation_label } });

  define_and_emit_label(cc, reverse_iterator_direction_label);

  /* reverse iterator direction */
  kit_emit_ins(cc, (kit_ins){ .movi = { .opcode = EIR_OPCODE_MOVI, .dst = iterator_increment_amount, .value = -1 } });

  define_and_emit_label(cc, end_iterator_direction_calculation_label);

  u32 start_label   = make_label_id(cc);
  u32 iterate_label = make_label_id(cc);
  u32 end_label     = make_label_id(cc);

  kitc_loop_location loop = {
    .continue_label = iterate_label,
    .break_label    = end_label,
    .defer_depth    = defer_get_current_depth(cc),
  };
  cc->loop = &loop; /* stored our old cc->loop earlier to prevent a dangling reference */

  define_and_emit_label(cc, start_label);

  /* check if iterator == stop */
  kit_vreg_t condition_check = vreg_alloc(cc);

  // neq dst=condition_check, iterator, stop
  kit_emit_ins(cc, (kit_ins){ .binop = { .opcode = EIR_OPCODE_EQL, .dst = condition_check, .a = iterator_reg, .b = iterator_stop_value } });
  if (emit_and_record_jmp(cc, EIR_OPCODE_JNZ, condition_check, end_label) < 0) goto ERR;

  for (u32 i = 0; i < nstmts; i++) {
    if (compile(cc, stmts[i]) < 0) {
      cerror(KIT_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement [ranged for]\n");
      goto ERR;
    }
  }

  /* iterate label, inc/decrement our iterator */
  define_and_emit_label(cc, iterate_label);

  if (defer_emit_current_scope(cc) < 0) goto ERR;

  /* increase iterator_reg by increment_amount (+1 or -1)  */
  kit_emit_ins(cc, (kit_ins){ .binop = { .opcode = EIR_OPCODE_ADD, .dst = iterator_reg, .a = iterator_reg, .b = iterator_increment_amount } });

  if (emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, start_label) < 0) goto ERR;

  define_and_emit_label(cc, end_label);

  scope_pop(cc);
  defer_pop_scope(cc);

  cc->loop = old_loop;

  return 0;

ERR:
  cc->loop = old_loop;
  return -1;
}

static kit_vreg_t
compile_member_access(kit_compiler* cc, int node)
{
  if (!can_make_value(cc->ast, node)) {
    kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
    cerror(span, "Failed to compile member access: Failed to lower to rvalue\n");
    return -1;
  }
  // Passthrough
  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  return emit_lvalue_load(cc, &lv);
}

static kit_vreg_t
compile_assign(kit_compiler* cc, int node)
{
  int right = KIT_GET_NODE(cc->ast, node)->assign.right;
  int left  = KIT_GET_NODE(cc->ast, node)->assign.left;

  if (!can_make_value(cc->ast, left)) {
    kit_filespan left_span = KIT_GET_NODE(cc->ast, left)->common.span;
    cerror(left_span, "Can not assign to left: Failed to lower to lvalue\n");
    return -1;
  }

  val_t lv;
  int   e = value_init(cc, left, &lv);
  if (e < 0) return e;

  kitc_builtin_variables_table* builtin_vars_table = cc->builtin_var_table;

  kitc_var* exists = scope_lookup_info(cc, lv.val.var.id);

  if (!exists && KIT_GET_NODE(cc->ast, left)->type == KIT_AST_NODE_VARIABLE) {
    // Doesn't exist and node is supposed to be a variable (Not member access or index)

    /* Check if the user is trying to modify a builtin variable. */
    for (u32 i = 0; i < builtin_vars_table->builtin_vars_count; i++) {
      if (lv.val.var.id == builtin_vars_table->builtin_var_hashes[i]) {
        cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Attempting to modify builtin constant '%s'\n", lv.val.var.name);
        value_free(&lv);
        return -1;
      }
    }

    cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Undeclared variable '%s'\n", lv.val.var.name);
    value_free(&lv);
    return -1;
  }

  if (exists && exists->is_const) {
    cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Can not assign to const qualified variable '%s'\n", lv.val.var.name);
    value_free(&lv);
    return -1;
  }

  kit_vreg_t rreg = compile(cc, right);
  if (rreg < 0) return rreg;

  e = emit_lvalue_assign(cc, rreg, &lv);
  value_free(&lv);
  if (e < 0) return e;

  /* Propogate the RHS' register */
  return rreg;
}

static kit_vreg_t
compile_member_assign(kit_compiler* cc, int node)
{
  int value = KIT_GET_NODE(cc->ast, node)->member_assign.value;

  if (!can_make_value(cc->ast, node)) {
    cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Can not assign to member access: Failed to lower to lvalue\n");
    return -1;
  }

  kit_vreg_t rhs = compile(cc, value);
  if (rhs < 0) { return rhs; }

  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  kit_vreg_t propogate_rhs = emit_lvalue_assign(cc, rhs, &lv);
  value_free(&lv);

  return propogate_rhs;
}

static kit_vreg_t
compile_return(kit_compiler* cc, int node)
{
  int r = defer_emit_all_scopes(cc);
  if (r < 0) return r;

  if (KIT_GET_NODE(cc->ast, node)->ret.has_return_value) {
    int ret_node = KIT_GET_NODE(cc->ast, node)->ret.expr_id;
    /* Compile the return value */
    kit_vreg_t rv = compile(cc, ret_node);
    if (rv < 0) {
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Failed to compile return value [return]\n");
      return rv;
    }

    kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = rv } });
  } else {
    kit_var    nil    = KIT_NULLVAR;
    kit_vreg_t nilvar = compile_and_push_literal_variable(cc, &nil);
    if (nilvar < 0) return nilvar;

    kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = nilvar } });
  }
  return r;
}

static int
append_struct_decleration(kit_arena* a, const char* name, kitc_struct_information* deposit)
{
  u32 hash = kit_hash(name, strlen(name));

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
  deposit->field_names[deposit->fields_count]  = kit_arnstrdup(a, name);

  deposit->fields_count++;

  return 0;
}

static int
collect_struct_declerations(kit_compiler* cc, int* stmts, u32 nstmts, kitc_struct_information* deposit)
{
  for (u32 i = 0; i < nstmts; i++) {
    kit_ast_node_type type = KIT_GET_NODE(cc->ast, stmts[i])->type;

    if (type == KIT_AST_NODE_STATEMENT_LIST) {
      int* list_stmts  = KIT_GET_NODE(cc->ast, stmts[i])->stmts.stmts;
      u32  list_nstmts = KIT_GET_NODE(cc->ast, stmts[i])->stmts.nstmts;
      // RECURSE!
      int e = collect_struct_declerations(cc, list_stmts, list_nstmts, deposit);
      if (e < 0) return e;
      continue;
    }

    if (type == KIT_AST_NODE_VARIABLE_DECL) {
      bool is_const = KIT_GET_NODE(cc->ast, stmts[i])->let.is_const;
      if (is_const) {
        cerror(KIT_GET_NODE(cc->ast, stmts[i])->let.span, "A member of a struct cannot be declared 'const' [struct decleration]\n");
        return -1;
      }

      int initializer = KIT_GET_NODE(cc->ast, stmts[i])->let.initializer;
      if (initializer >= 0) {
        cerror(KIT_GET_NODE(cc->ast, stmts[i])->let.span, "Initializer given in decleration of struct member [struct decleration]\n");
        return -1;
      }

      const char* name = KIT_GET_NODE(cc->ast, stmts[i])->let.name;
      int         e    = append_struct_decleration(cc->arena, name, deposit);
      if (e < 0) return e;
    } else {
      const char* member_name = KIT_GET_NODE(cc->ast, stmts[i])->let.name;
      cerror(
          KIT_GET_NODE(cc->ast, stmts[i])->let.span,
          "Member index %u, with name %s is not allowed in a struct [struct decleration]\n",
          i,
          member_name);
      return -1;
    }
  }

  return 0;
}

static int
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

static kit_vreg_t
compile_struct_constructor(kit_compiler* fork, kit_filespan span, const kitc_struct_information* struc)
{
  u32        struct_id = kit_hash(struc->name, strlen(struc->name));
  kit_vreg_t tmp       = vreg_alloc(fork);

  kit_str_intern(struc->name, fork->ast->interner);

  /* Since struct constructors are called as ordinary functions, we don't have to handle register spilling here */
  for (u32 i = 0; i < struc->fields_count; i++) {
    if (i < KIT_REG_ARG_COUNT) {
      kit_vreg_t reg = (kit_vreg_t)(KIT_REG_ARG0 + i);                        /* Define our variable in the argument register */
      scope_define_in_register(fork, reg, span, struc->field_names[i], true); // let the compiler say it's constant
    } else {
      /* The rest of the arguments will already be on the stack (call's job). We don't need to do anything. */
    }
  }

  /* Make the structure. Our inputs are already in the argument register and the stack (calling convention says so.) */
  kit_emit_ins(fork, (kit_ins){ .mk_struct = { .opcode = EIR_OPCODE_MK_STRUCT, .dst = tmp, .struct_id = struct_id } });

  /* Return from our temporary register */
  kit_emit_ins(fork, (kit_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = tmp } });

  /* No need to clean up the stack, the CALL handler is responsible for that */

  kitc_function f = {
    .code        = fork->instructions,
    .code_count  = fork->ninstructions,
    .name_hash   = struct_id,
    .nargs       = struc->fields_count,
    .vregs_used  = fork->next_vreg,
    .labels_used = fork->next_label,
  };

  int e = append_function_entry(fork->arena, fork->function_table, &f);
  if (e < 0) return e;

  return 0;
}

static kit_vreg_t
compile_struct_decleration(kit_compiler* cc, int node)
{
  int                     e           = 0;
  kit_compiler            fork        = { 0 };
  kitc_struct_information struct_data = { 0 };

  const char* struct_name        = KIT_GET_NODE(cc->ast, node)->struct_decl.name;
  int*        struct_decl_stmts  = KIT_GET_NODE(cc->ast, node)->struct_decl.stmts;
  u32         struct_decl_nstmts = KIT_GET_NODE(cc->ast, node)->struct_decl.nstmts;

  /* intern structure name for debug symbols. */
  kit_str_intern(struct_name, cc->ast->interner);

  /* Gather all information the user provided into one big structure. */
  struct_data = (kitc_struct_information){
    .name           = kit_arnstrdup(cc->arena, struct_name),
    .name_hash      = kit_hash(struct_name, strlen(struct_name)),
    .field_hashes   = (u32*)kit_xalloc(init_fields_capacity, sizeof(u32)),
    .field_names    = (char**)kit_xalloc(init_fields_capacity, sizeof(char**)),
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

  e = compile_struct_constructor(&fork, KIT_GET_NODE(cc->ast, node)->common.span, &struct_data);
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

static kit_vreg_t
compile_variable_decleration(kit_compiler* cc, int node)
{
  const char* name = KIT_GET_NODE(cc->ast, node)->let.name;
  char*       full = qualify_name(cc, name);
  if (!full) return -1;

  u32 hash        = kit_hash(full, strlen(full));
  int initializer = KIT_GET_NODE(cc->ast, node)->let.initializer;

  kitc_var* v = cc->scope->vars;
  while (v) {
    if (v->name_hash == hash) {
      cerror(v->span, "Variable redeclared in same scope\n");
      return -1;
    }
    v = v->next;
  }

  /* Add variable entry to stack */

  const bool initializer_provided = initializer >= 0;

  kit_filespan span     = KIT_GET_NODE(cc->ast, node)->let.span;
  bool         is_const = KIT_GET_NODE(cc->ast, node)->let.is_const;

  /* Define the variable */
  kit_vreg_t new_var = scope_define(cc, span, full, is_const);
  kitc_var*  info    = scope_lookup_info(cc, hash);

  if (initializer_provided) {
    /* cant use the lval system because it doesn't handle const global variables (this is a variable decleration, and if it is const,
     * emit_lvalue_assign will always error out) */

    kit_vreg_t init_reg = compile(cc, initializer);
    if (init_reg < 0) return init_reg;

    /* global scope? */
    if (info->is_global) {
      kit_emit_ins(cc, (kit_ins){ .setg = { .opcode = EIR_OPCODE_SETG, .dst = new_var, .src = init_reg } });
    } else {
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = new_var, .src = init_reg } });
    }
  } else {
    kit_var    nil = KIT_NULLVAR;
    kit_vreg_t n   = compile_and_push_literal_variable(cc, &nil);
    if (n < 0) return n;

    /* no initializer specified. initialize it to null. */
    if (info->is_global) {
      kit_emit_ins(cc, (kit_ins){ .setg = { .opcode = EIR_OPCODE_SETG, .dst = new_var, .src = n } });
    } else {
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = new_var, .src = n } });
    }
  }

  return new_var;
}

static kit_vreg_t
compile_root(kit_compiler* cc, int node)
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
  kit_ast_node* root = KIT_GET_NODE(cc->ast, node);
  for (u32 i = 0; i < root->root.nstmts; i++) {
    int e = compile(cc, root->root.stmts[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, root->root.stmts[i])->common.span, "Failed to compile root, bailing out [root]\n");
      return e;
    }
  }

  /**
   * Done with initialization!
   * Now we return from here
   * and expect the interpreter to jump
   * to the entry point
   */
  kit_var    v      = KIT_NULLVAR;
  kit_vreg_t nilvar = compile_and_push_literal_variable(cc, &v);
  kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = nilvar } });

  // if (!cc->info->feature_set.disable_register_allocation_i_know_what_im_doing) era_register_allocation_pass(cc);
  // cc->ninstructions = label_pass(cc->arena, cc->instructions, cc->ninstructions, cc->next_label);

  return 0; // Done!
}

static kit_vreg_t
compile_statement_list(kit_compiler* cc, int node)
{
  kit_ast_node* nodep  = KIT_GET_NODE(cc->ast, node);
  const int*    stmts  = nodep->stmts.stmts;
  u32           nstmts = nodep->stmts.nstmts;

  for (u32 i = 0; i < nstmts; i++) {
    int e = compile(cc, stmts[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement [statement list]\n");
      return e;
    }
  }

  return 0;
}

static kit_vreg_t
compile_variable_load(kit_compiler* cc, int node)
{
  char* full = qualify_name(cc, KIT_GET_NODE(cc->ast, node)->ident.ident);
  u32   hash = kit_hash(full, strlen(full));

  for (u32 i = 0; i < cc->builtin_var_table->builtin_vars_count; i++) {
    const kit_builtin_var* builtin_var      = &cc->builtin_var_table->builtin_vars[i];
    u32                    builtin_var_hash = cc->builtin_var_table->builtin_var_hashes[i];

    if (builtin_var_hash == hash) {
      /* Instantiate a builtin variable only if it is used. */
      kit_var v = {
        .type = builtin_var->type,
        .val  = builtin_var->value,
      };

      return compile_and_push_literal_variable(cc, &v); // compile_literal_variable loads the value! Return.
    }
  }

  val_t lv;
  int   e = value_init(cc, node, &lv);
  if (e < 0) return e;

  kit_vreg_t dst = emit_lvalue_load(cc, &lv);
  if (e < 0) {
    value_free(&lv);
    return -1;
  }

  value_free(&lv);
  return dst;
}

static kit_vreg_t
compile_namespace_decleration(kit_compiler* cc, int node)
{
  const char* ns_name        = KIT_GET_NODE(cc->ast, node)->namespace_decl.name;
  int*        ns_decl_stmts  = KIT_GET_NODE(cc->ast, node)->namespace_decl.stmts;
  u32         ns_decl_nstmts = KIT_GET_NODE(cc->ast, node)->namespace_decl.nstmts;

  int e = ns_push(cc, ns_name);
  if (e < 0) return e;

  for (u32 i = 0; i < ns_decl_nstmts; i++) {
    e = compile(cc, ns_decl_stmts[i]);
    if (e < 0) {
      cerror(KIT_GET_NODE(cc->ast, ns_decl_stmts[i])->common.span, "Failed to compile namespace decleration [namespace decleration]\n");
      ns_pop(cc);
      return e;
    }
  }

  ns_pop(cc);

  return 0;
}

static kit_vreg_t
compile_function(kit_compiler* cc, int node)
{
  u32  nstmts = KIT_GET_NODE(cc->ast, node)->func.nstmts;
  int* stmts  = KIT_GET_NODE(cc->ast, node)->func.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    if (compile(cc, stmts[i]) < 0) return -1;

    /**
     * OPTIMIZATION: If we're in the function stream,
     * and a node pushes a value to the stack that we do not want,
     * pop it.
     */
    // if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, stmts[i]);
  }

  kit_var    nil    = KIT_NULLVAR;
  kit_vreg_t nilvar = compile_and_push_literal_variable(cc, &nil);
  if (nilvar < 0) return nilvar;

  kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = EIR_OPCODE_RET, .return_value = nilvar } });
  return 0;
}

static kit_vreg_t
compile_builtin_structure(kit_compiler* cc, const kit_builtin_struct* b)
{
  kit_compiler fork = { 0 };
  int          e    = 0;

  /* intern its name for debug symbols. */
  kit_str_intern(b->name, cc->ast->interner);

  kitc_struct_information st = {
    .name           = kit_arnstrdup(cc->arena, b->name),
    .name_hash      = kit_hash(b->name, strlen(b->name)),
    .field_hashes   = kit_xalloc(b->fields_count, sizeof(u32)),
    .field_names    = (char**)kit_xalloc(b->fields_count, sizeof(char*)),
    .fields_count   = b->fields_count,
    .field_capacity = b->fields_count,
  };
  if (!st.field_hashes || !st.field_names) { goto ERR; }

  for (u32 j = 0; j < b->fields_count; j++) {
    st.field_hashes[j] = kit_hash(b->fields[j], strlen(b->fields[j]));
    st.field_names[j]  = kit_arnstrdup(cc->arena, b->fields[j]);
  }

  /**
   * We don't need to push or pop stack frames here!
   * This function just recurses to compile_struct_constructor
   * which just emits the bytecode to form the structure.
   */

  /* Generate the constructor function */
  e = compiler_make_fork(cc, &fork);
  if (e < 0) goto ERR;

  e = compile_struct_constructor(&fork, KIT_GET_NODE(cc->ast, cc->ast->root)->common.span, &st);
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
static kit_vreg_t
compile_builtin_structures(kit_compiler* cc)
{
  for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_structs); i++) {
    const kit_builtin_struct* b = &kit_builtins_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e < 0) return e;
  }

  /**
   * Compile all hooked builtin structures.
   */
  for (u32 i = 0; i < cc->info->nhooked_structs; i++) {
    const kit_builtin_struct* b = &cc->info->hook_structs[i];

    int e = compile_builtin_structure(cc, b);
    if (e < 0) return e;
  }

  return 0;
}

/* Register ID on success, <0 on error */
static int
compile(kit_compiler* cc, int node)
{
  switch (KIT_GET_NODE(cc->ast, node)->type) {
    case KIT_AST_NODE_NOP: return 0;
    case KIT_AST_NODE_INT:
    case KIT_AST_NODE_FLOAT:
    case KIT_AST_NODE_BOOL:
    case KIT_AST_NODE_CHAR:
    case KIT_AST_NODE_STRING: return compile_literal(cc, node);
    case KIT_AST_NODE_ROOT: return compile_root(cc, node);
    case KIT_AST_NODE_STATEMENT_LIST: return compile_statement_list(cc, node);
    case KIT_AST_NODE_FUNCTION_DEFINITION: return compile_function_definition(cc, node);
    case KIT_AST_NODE_LIST: return compile_list(cc, node);
    case KIT_AST_NODE_MAP: return compile_map(cc, node);
    case KIT_AST_NODE_VARIABLE_DECL: return compile_variable_decleration(cc, node);
    case KIT_AST_NODE_BINARYOP: return compile_binary_op(cc, node);
    case KIT_AST_NODE_UNARYOP: return compile_unary_op(cc, node);
    case KIT_AST_NODE_RETURN: return compile_return(cc, node);
    case KIT_AST_NODE_VARIABLE: return compile_variable_load(cc, node);
    case KIT_AST_NODE_INDEX: return compile_index(cc, node);
    case KIT_AST_NODE_INDEX_ASSIGN: return compile_index_assign(cc, node);
    case KIT_AST_NODE_ASSIGN: return compile_assign(cc, node);
    case KIT_AST_NODE_CALL: return compile_function_call(cc, node);
    case KIT_AST_NODE_FOR: return compile_for_statement(cc, node);
    case KIT_AST_NODE_RANGED_FOR: return compile_ranged_for_statement(cc, node);
    case KIT_AST_NODE_WHILE: return compile_while_statement(cc, node);
    case KIT_AST_NODE_IF: return compile_if_statement(cc, node);
    case KIT_AST_NODE_STRUCT_DECL: return compile_struct_decleration(cc, node);
    case KIT_AST_NODE_MEMBER_ACCESS: return compile_member_access(cc, node);
    case KIT_AST_NODE_MEMBER_ASSIGN: return compile_member_assign(cc, node);
    case KIT_AST_NODE_NAMESPACE_DECL: return compile_namespace_decleration(cc, node);
    case KIT_AST_NODE_DEFER: {
      int* stmts  = KIT_GET_NODE(cc->ast, node)->defer.stmts;
      u32  nstmts = KIT_GET_NODE(cc->ast, node)->defer.nstmts;
      return append_defer_entry(cc, stmts, nstmts);
    }
    case KIT_AST_NODE_BREAK: {
      if (!cc->loop) {
        kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
        cerror(span, "break used outside a loop\n");
        return -1;
      }

      if (defer_emit_to_depth(cc, cc->loop->defer_depth) < 0) return -1;

      u32 target = cc->loop->break_label;
      return emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, target);
    }
    case KIT_AST_NODE_CONTINUE: {
      if (!cc->loop) {
        kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
        cerror(span, "continue used outside a loop\n");
        return -1;
      }

      if (defer_emit_to_depth(cc, cc->loop->defer_depth) < 0) return -1;

      u32 target = cc->loop->continue_label;
      return emit_and_record_jmp(cc, EIR_OPCODE_JMP, -1, target);
    }
    case KIT_AST_NODE_ASSERT: {
      int   condition = KIT_GET_NODE(cc->ast, node)->assertion.stmt;
      char* line      = KIT_GET_NODE(cc->ast, node)->assertion.assertion_line;

      kit_vreg_t cond = compile(cc, condition);
      if (cond < 0) {
        kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
        cerror(span, "Failed to compile statement [assert]\n");
        return -1;
      }

      kit_var line_str = KIT_NULLVAR;
      if (make_string_variable(cc->arena, line, &line_str) < 0) return -1;

      if (add_literal_to_track(cc, &line_str) < 0) return -1;

      kit_emit_ins(cc, (kit_ins){ .assertion = { .opcode = EIR_OPCODE_ASSERT, .cond = cond, .line_id = kit_var_hash(&line_str) } });

      return 0;
    }

    default: return -1;
  }
}

static u32
get_destination_reg(const kit_ins* i)
{
  switch ((eir_opcode_bits)i->opcode) {
      /* getg, setg and movg  */
    case EIR_OPCODE_MOV: return i->mov.dst; break;

    case EIR_OPCODE_MOVI: return i->movi.dst; break; /* values are not registers */
    case EIR_OPCODE_MOVF: return i->movf.dst; break; /* values are not registers */

    case EIR_OPCODE_GETG: return i->mov.dst; break;

    case EIR_OPCODE_ASSERT:
    case EIR_OPCODE_SETG:
    case EIR_OPCODE_MOVG: {
      break;
    }
    case EIR_OPCODE_LOADK:
      return i->loadk.dst;
      break; /* id is not a register */

    // case EIR_OPCODE_LOADK: break;
    case EIR_OPCODE_ADD:
    case EIR_OPCODE_SUB:
    case EIR_OPCODE_MUL:
    case EIR_OPCODE_DIV:
    case EIR_OPCODE_MOD:
    case EIR_OPCODE_EXP:
    case EIR_OPCODE_AND:
    case EIR_OPCODE_OR:
    case EIR_OPCODE_BAND:
    case EIR_OPCODE_BOR:
    case EIR_OPCODE_XOR:
    case EIR_OPCODE_EQL:
    case EIR_OPCODE_NEQ:
    case EIR_OPCODE_LT:
    case EIR_OPCODE_LTE:
    case EIR_OPCODE_GT:
    case EIR_OPCODE_GTE: return i->binop.dst; break;
    case EIR_OPCODE_NOT:
    case EIR_OPCODE_NEG:
    case EIR_OPCODE_BNOT:
    case EIR_OPCODE_DEC:
    case EIR_OPCODE_INC: return i->unop.dst; break;
    case EIR_OPCODE_CALL: return i->call.dst; break;
    case EIR_OPCODE_INDEX: return i->index.dst; break;
    case EIR_OPCODE_INDEX_ASSIGN: break;
    case EIR_OPCODE_MK_LIST: return i->mk_list.dst; break;
    case EIR_OPCODE_MK_MAP: return i->mk_map.dst; break;
    case EIR_OPCODE_MK_STRUCT: return i->mk_struct.dst; break;
    case EIR_OPCODE_MEMBER_ACCESS: return i->member_access.dst; break;
    case EIR_OPCODE_MEMBER_ASSIGN: break;
    case EIR_OPCODE_RET:
    case EIR_OPCODE_JZ:
    case EIR_OPCODE_JNZ:
    case EIR_OPCODE_PUSH: {
      break;
    }
    case EIR_OPCODE_POP: {
      return i->pop.reg;
    }

      // default: break;
    case EIR_OPCODE_NOP:
    case EIR_OPCODE_LABEL:
    case EIR_OPCODE_JMP: {
      break;
    }
  }

  return UINT32_MAX;
}

static u32
get_source_registers(const kit_ins* i, u32 sources[32])
{
  u32 n = 0;
  switch ((eir_opcode_bits)i->opcode) {
      /* getg, setg and movg  */
    case EIR_OPCODE_MOV: sources[n++] = i->mov.src; break;

    case EIR_OPCODE_ASSERT: sources[n++] = i->assertion.cond; break;

    case EIR_OPCODE_MOVI:
    case EIR_OPCODE_MOVF: /* values are not registers */
    case EIR_OPCODE_GETG: break;

    case EIR_OPCODE_SETG: sources[n++] = i->mov.src; break;

    case EIR_OPCODE_MOVG:
    case EIR_OPCODE_LOADK: /* id is not a register */ break;

    case EIR_OPCODE_ADD:
    case EIR_OPCODE_SUB:
    case EIR_OPCODE_MUL:
    case EIR_OPCODE_DIV:
    case EIR_OPCODE_MOD:
    case EIR_OPCODE_EXP:
    case EIR_OPCODE_AND:
    case EIR_OPCODE_OR:
    case EIR_OPCODE_BAND:
    case EIR_OPCODE_BOR:
    case EIR_OPCODE_XOR:
    case EIR_OPCODE_EQL:
    case EIR_OPCODE_NEQ:
    case EIR_OPCODE_LT:
    case EIR_OPCODE_LTE:
    case EIR_OPCODE_GT:
    case EIR_OPCODE_GTE:
      sources[n++] = i->binop.a;
      sources[n++] = i->binop.b;
      break;

    case EIR_OPCODE_NOT:
    case EIR_OPCODE_NEG:
    case EIR_OPCODE_BNOT:
    case EIR_OPCODE_DEC:
    case EIR_OPCODE_INC: sources[n++] = i->unop.a; break;

    case EIR_OPCODE_MK_LIST:
      for (u32 j = KIT_REG_ARG0; j < MIN(KIT_REG_ARG_COUNT, i->mk_list.nelems); j++) { sources[n++] = (kit_vreg_t)(KIT_REG_ARG0 + j); }
      break;

    case EIR_OPCODE_MK_MAP:
      for (u32 j = KIT_REG_ARG0; j < MIN(KIT_REG_ARG_COUNT, i->mk_map.npairs * 2); j++) { sources[n++] = (kit_vreg_t)(KIT_REG_ARG0 + j); }
      break;

    case EIR_OPCODE_MK_STRUCT:
      /* finding out which argument registers to clear will take some time. just kill the entire vector. */
      for (u32 j = KIT_REG_ARG0; j < KIT_REG_ARG_COUNT; j++) { sources[n++] = (kit_vreg_t)(KIT_REG_ARG0 + j); }
      break;
    case EIR_OPCODE_CALL:
      /* record only the argument registers that were used */
      for (u32 j = KIT_REG_ARG0; j < MIN(KIT_REG_ARG_COUNT, i->call.nargs); j++) { sources[n++] = (kit_vreg_t)(KIT_REG_ARG0 + j); }
      break;

    case EIR_OPCODE_INDEX:
      sources[n++] = i->index.base;
      sources[n++] = i->index.index;
      break;
    case EIR_OPCODE_INDEX_ASSIGN:
      sources[n++] = i->index_assign.value;
      sources[n++] = i->index_assign.index;
      sources[n++] = i->index_assign.base;
      break;

    case EIR_OPCODE_MEMBER_ACCESS: sources[n++] = i->member_access.base; break;
    case EIR_OPCODE_MEMBER_ASSIGN:
      sources[n++] = i->member_assign.value;
      sources[n++] = i->member_assign.base;
      break;
    case EIR_OPCODE_RET: sources[n++] = i->ret.return_value; break;
    case EIR_OPCODE_JZ:
    case EIR_OPCODE_JNZ: sources[n++] = i->cj.condition; break;
    case EIR_OPCODE_PUSH: {
      sources[n++] = i->push.reg;
      break;
    }
    // default: break;
    case EIR_OPCODE_POP:
    case EIR_OPCODE_NOP:
    case EIR_OPCODE_LABEL:
    case EIR_OPCODE_JMP: {
      break;
    }
  }

  return n;
}

static inline u32
intersection(const u32* rpo_num, const u32* idom, u32 b1, u32 b2)
{
  u32 cursor1 = (b1);
  u32 cursor2 = (b2);
  while (cursor1 != cursor2) {
    while (rpo_num[cursor1] < rpo_num[cursor2]) cursor1 = idom[cursor1];
    while (rpo_num[cursor2] < rpo_num[cursor1]) cursor2 = idom[cursor2];
  }
  return cursor1;
}

static int
codegraph_build_successor_list(kit_compiler* cc, codegraph* dst)
{
  /* label ID -> Label instruction index */
  u32* label_map = kit_arnalloc(dst->arena, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32));

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == EIR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  const u32  nblocks = dst->nblocks;
  codeblock* blocks  = dst->blocks;

  const kit_ins* code      = cc->instructions;
  u32            code_size = cc->ninstructions;

  u32* ip_to_block = kit_arnalloc(dst->arena, code_size * sizeof(u32));
  for (u32 b = 0; b < nblocks; b++) {
    for (u32 ip = blocks[b].start; ip <= blocks[b].end; ip++) { ip_to_block[ip] = b; }
  }

  for (u32 i = 0; i < nblocks; i++) {
    codeblock* blk   = &blocks[i];
    blk->nsuccessors = 0; /* filling it in this loop */

    blk->uses     = NULL; /* define for later passes */
    blk->defines  = NULL;
    blk->live_in  = NULL;
    blk->live_out = NULL;

    u32            last_ins_ip = blk->end;
    const kit_ins* last_ins    = &code[last_ins_ip];

    /* jump to another code block */
    if (last_ins->opcode == EIR_OPCODE_JMP) {
      u32 target_ip = label_map[last_ins->jmp.target];
      if (target_ip < code_size) {
        u32 target_block                    = ip_to_block[target_ip];
        blk->successors[blk->nsuccessors++] = target_block;
      }
    }

    else if (last_ins->opcode == EIR_OPCODE_JZ || last_ins->opcode == EIR_OPCODE_JNZ) {
      // fallthrough
      if (i + 1 < nblocks) blk->successors[blk->nsuccessors++] = i + 1;

      // the branch taken (if condition were to be true)
      u32 target_ip = label_map[last_ins->cj.target];
      if (target_ip < code_size) {
        u32 target_block                    = ip_to_block[target_ip];
        blk->successors[blk->nsuccessors++] = target_block;
      }
    }

    else if (last_ins->opcode == EIR_OPCODE_RET) {
      // no successors ever.
    }

    /* normal instruction. fallthrough only. */
    else if ((i + 1) < nblocks) {
      blk->successors[blk->nsuccessors++] = i + 1;
    }
  }

  // u32* pred_count = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  // memset(pred_count, 0, nblocks * sizeof(u32));

  // for (u32 b = 0; b < nblocks; b++) {
  //   for (u32 s = 0; s < blocks[b].nsuccessors; s++) {
  //     u32 succ = blocks[b].successors[s];
  //     if (succ < nblocks) pred_count[succ]++;
  //   }
  // }

  // for (u32 b = 0; b < nblocks; b++) {
  //   blocks[b].npredecessors = pred_count[b];
  //   if (pred_count[b] > 0) {
  //     blocks[b].predecessors = kit_arnalloc(cc->arena, pred_count[b] * sizeof(u32));
  //   } else {
  //     blocks[b].predecessors = NULL;
  //   }
  // }

  // u32* fill_idx = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  // memset(fill_idx, 0, nblocks * sizeof(u32));

  // /* fill predecessors */
  // for (u32 b = 0; b < nblocks; b++) {
  //   for (u32 s = 0; s < blocks[b].nsuccessors; s++) {
  //     u32 succ = blocks[b].successors[s];
  //     if (succ < nblocks) { blocks[succ].predecessors[fill_idx[succ]++] = b; }
  //   }
  // }

  // /* reverse post ordered */
  // u32* rpo     = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  // u32* rpo_num = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  // u32  rpo_idx = 0;

  // /* post ordered */
  // u32* post     = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  // u32  post_idx = 0;

  // bool* visited = kit_arnalloc(cc->arena, nblocks * sizeof(bool));
  // memset(visited, 0, nblocks * sizeof(bool));

  // u32* stack = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  // u32  sp    = 0;

  // /* dfs thru our codegraph */
  // stack[0]   = 0;
  // visited[0] = true;
  // sp++;
  // while (sp > 0) {
  //   u32  b      = stack[sp - 1];
  //   bool pushed = false;

  //   for (u32 s = 0; s < blocks[b].nsuccessors; s++) {
  //     u32 succ = blocks[b].successors[s];
  //     if (!visited[succ]) {
  //       visited[succ] = true;
  //       stack[sp++]   = succ;
  //       pushed        = true;
  //       break;
  //     }
  //   }

  //   if (!pushed) {
  //     post[post_idx++] = b;
  //     sp--;
  //   }
  // }

  // /* reverse the post list to get RPO */
  // for (u32 i = 0; i < nblocks; i++) {
  //   rpo[i]          = post[nblocks - 1 - i];
  //   rpo_num[rpo[i]] = i;
  // }

  // u32* idom = kit_arnalloc(cc->arena, nblocks * sizeof(u32));
  // for (u32 i = 0; i < nblocks; i++) idom[i] = UINT32_MAX;
  // idom[0] = 0; /* entry point */

  // bool changed = true;
  // while (changed) {
  //   changed = false;

  //   for (u32 i = 1; i < nblocks; i++) { /* skip entry point */
  //     u32 b = rpo[i];

  //     /* find first processed predecessor */
  //     u32 new_idom = UINT32_MAX;
  //     for (u32 p = 0; p < blocks[b].npredecessors; p++) {
  //       u32 pred = blocks[b].predecessors[p];
  //       if (idom[pred] != UINT32_MAX) {
  //         new_idom = pred;
  //         break;
  //       }
  //     }

  //     /* interect with all other processed predecessors */
  //     for (u32 p = 0; p < blocks[b].npredecessors; p++) {
  //       u32 pred = blocks[b].predecessors[p];
  //       if (pred != new_idom && idom[pred] != UINT32_MAX) new_idom = intersection(rpo_num, idom, pred, new_idom);
  //     }

  //     if (new_idom != idom[b]) {
  //       idom[b] = new_idom;
  //       changed = true;
  //     }
  //   }
  // }

  // kit_arnfree(cc->arena, stack);
  // kit_arnfree(cc->arena, visited);
  // kit_arnfree(cc->arena, rpo_num);
  // kit_arnfree(cc->arena, rpo);
  // kit_arnfree(cc->arena, pred_count);
  // kit_arnfree(cc->arena, fill_idx);
  // kit_arnfree(dst->arena, ip_to_block);
  // kit_arnfree(dst->arena, label_map);

  return 0;
}

static int
codegraph_liveliness_analysis(kit_compiler* cc, codegraph* dst)
{
  const kit_ins* code      = cc->instructions;
  u32            code_size = cc->ninstructions;

  const u32 nvregs = cc->next_vreg;
  assert(nvregs != 0);

  for (u32 i = 0; i < dst->nblocks; i++) {
    codeblock* blk = &dst->blocks[i];

    blk->uses     = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    blk->defines  = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    blk->live_in  = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    blk->live_out = kit_arnalloc(dst->arena, nvregs * sizeof(bool));

    memset(blk->defines, 0, nvregs * sizeof(bool));
    memset(blk->uses, 0, nvregs * sizeof(bool));
    memset(blk->live_in, 0, nvregs * sizeof(bool));
    memset(blk->live_out, 0, nvregs * sizeof(bool));
  }

  for (u32 i = 0; i < dst->nblocks; i++) {
    codeblock* blk = &dst->blocks[i];

    bool* defined_so_far = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
    memset(defined_so_far, 0, nvregs * sizeof(bool));

    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      kit_ins ins = cc->instructions[ip];

      u32 dst_reg = get_destination_reg(&ins);

      u32 srcs[32] = { 0 };
      u32 nsrcs    = get_source_registers(&ins, srcs);

      for (u32 j = 0; j < nsrcs; j++) {
        u32 r = srcs[j];
        if (r < nvregs && !defined_so_far[r]) blk->uses[r] = true;
      }

      if (dst_reg != UINT32_MAX && dst_reg < nvregs) {
        blk->defines[dst_reg]   = true;
        defined_so_far[dst_reg] = true;
      }
    }
  }

  /* Find all registers used for returning values */
  bool* return_live = kit_arnalloc(dst->arena, nvregs * sizeof(bool));
  for (u32 i = 0; i < code_size; i++) {
    if (code[i].opcode == EIR_OPCODE_RET) return_live[code[i].ret.return_value] = true;
  }

  /* initialize live_out sets for exit blocks */
  for (u32 i = 0; i < dst->nblocks; i++) {
    codeblock* blk = &dst->blocks[i];
    if (blk->nsuccessors != 0) continue;
    for (u32 r = 0; r < nvregs; r++) {
      if (return_live[r]) { blk->live_out[r] = true; }
    }
  }

  bool* new_live_out = kit_arnalloc(dst->arena, nvregs);
  bool* new_live_in  = kit_arnalloc(dst->arena, nvregs);

  /* start out as true so while loop starts */
  bool changed = true;
  while (changed) {
    changed = false; /* assume we didn't change */

    for (i64 i = (i64)dst->nblocks - 1; i >= 0; i--) {
      codeblock* blk = &dst->blocks[i];

      memset(new_live_out, 0, nvregs);
      memset(new_live_in, 0, nvregs);

      /* new_live_out = U live_in[successor] */
      for (u32 s = 0; s < blk->nsuccessors; s++) {
        codeblock* successor = &dst->blocks[blk->successors[s]];

        for (u32 reg = 0; reg < nvregs; reg++) {
          if (successor->live_in[reg]) { new_live_out[reg] = true; }
        }
      }

      /* new_live_in = uses U (new_live_out - definitions) */
      for (u32 reg = 0; reg < nvregs; reg++) {
        if ((blk->uses[reg]) || (new_live_out[reg] && !blk->defines[reg])) { new_live_in[reg] = true; }
      }

      if (memcmp(new_live_in, blk->live_in, nvregs) != 0) { /* changed? */
        changed = true;
      }
      if (memcmp(new_live_out, blk->live_out, nvregs) != 0) { /* changed? */
        changed = true;
      }

      memcpy(blk->live_in, new_live_in, nvregs);
      memcpy(blk->live_out, new_live_out, nvregs);
    }
  }

  return 0;
}

static ATTR_NODISCARD int
codegraph_init(kit_arena* a, kit_compiler* cc, codegraph* dst)
{
  const kit_ins* code      = cc->instructions;
  u32            code_size = cc->ninstructions;

  const u32 nvregs = cc->next_vreg;

  u32* label_map = kit_arnalloc(a, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32));

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == EIR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  /* Find all block headers */
  bool* blk_header = kit_arnalloc(a, code_size * sizeof(bool));
  memset(blk_header, 0, code_size * sizeof(bool));

  /* first instruction */
  blk_header[0] = true;

  for (u32 i = 0; i < code_size; i++) {
    const kit_ins* ins = &code[i];

    if (ins->opcode == EIR_OPCODE_JMP) {
      u32 target = label_map[ins->jmp.target];
      if (target < cc->ninstructions) { blk_header[target] = true; }
      if (i + 1 < code_size) blk_header[i + 1] = true;
    } else if (ins->opcode == EIR_OPCODE_JZ || ins->opcode == EIR_OPCODE_JNZ) {
      u32 target = label_map[ins->cj.target];

      if (target < code_size) blk_header[target] = true;
      if (i + 1 < code_size) blk_header[i + 1] = true; // fallthrough
    }
  }

  // kit_arnfree(a, label_map);

  /* Divide the code into blocks using our block headers */
  codeblock* blocks  = kit_arnalloc(a, sizeof(codeblock) * code_size);
  size_t     nblocks = 0;

  u32 block_start = 0;
  for (u32 i = 0; i < code_size; i++) {
    if (blk_header[i] && i != 0) {
      blocks[nblocks].start = block_start;
      blocks[nblocks].end   = i - 1;
      nblocks++;
      block_start = i;
    }
  }

  // last block
  blocks[nblocks].start = block_start;
  blocks[nblocks].end   = code_size - 1;
  nblocks++;

  dst->blocks  = blocks;
  dst->nblocks = nblocks;
  dst->nvregs  = nvregs;
  dst->arena   = a;

  return 0;
}

static bool
is_instruction_impure(eir_opcode op)
{
  switch (op) {
    case EIR_OPCODE_CALL:
    case EIR_OPCODE_PUSH:
    case EIR_OPCODE_POP:
    case EIR_OPCODE_RET:
    case EIR_OPCODE_JMP:
    case EIR_OPCODE_JZ:
    case EIR_OPCODE_JNZ:
    case EIR_OPCODE_LABEL: // keep labels for control flow
    case EIR_OPCODE_SETG:  // writes global state
    case EIR_OPCODE_MOVG:
    case EIR_OPCODE_INDEX_ASSIGN:
    case EIR_OPCODE_MEMBER_ASSIGN:
    case EIR_OPCODE_MK_LIST:
    case EIR_OPCODE_MK_MAP:
    case EIR_OPCODE_MK_STRUCT: return true;
    default: return false;
  }
}

static bool
codegraph_dead_store_elimination(kit_compiler* cc, codegraph* cfg)
{
  codegraph_build_successor_list(cc, cfg);

  bool* live = kit_arnalloc(cfg->arena, cfg->nvregs);

  bool changed = false;

  for (u32 pass = 0; pass < 4; pass++) {
    /* reparse liveliness information */
    codegraph_liveliness_analysis(cc, cfg);

    for (u32 i = 0; i < cfg->nblocks; i++) {
      codeblock* blk = &cfg->blocks[i];

      memcpy(live, blk->live_out, cfg->nvregs);

      for (i64 ip = blk->end; ip >= blk->start; ip--) {
        kit_ins* ins = &cc->instructions[ip];

        if (ins->opcode == EIR_OPCODE_NOP || ins->opcode == EIR_OPCODE_LABEL) continue;

        u32 dst     = UINT32_MAX;
        u32 src[32] = { 0 };
        u32 nsrc    = 0;

        dst  = get_destination_reg(ins);
        nsrc = get_source_registers(ins, src);

        /* need to mark sources for impure instructions */
        if (!is_instruction_impure(ins->opcode) && (dst != UINT32_MAX && !live[dst])) {
          /* dead store */
          ins->opcode = EIR_OPCODE_NOP;
          changed     = true;
          continue;
        }

        /* mark all sources as live */
        for (u32 s = 0; s < nsrc; s++) { live[src[s]] = true; }
        if (dst != UINT32_MAX) live[dst] = false;
      }
    }
  }

  return changed;
}

static inline bool
is_instruction_noop(eir_opcode opcode)
{
  switch (opcode) {
    case EIR_OPCODE_NOP:
    case EIR_OPCODE_LABEL: return true;
    default: return false;
  }
}

static bool
codegraph_redundant_move_elimination(kit_compiler* cc, codegraph* cfg)
{
  /**
   * Find the pattern:
   * mov a, x
   * mov b, a
   *
   * or loadk a
   * mov b, a -> loadk b
   *
   * Then determine whether any register relies on a
   * (and can we switch a to b?). If no one does,
   * replace the pattern with a single mov b, x
   */
  bool changed = false;

  for (u32 pass = 0; pass < 4; pass++) {
    for (u32 i = 0; i < cc->ninstructions; i++) {
      if (i + 1 >= cc->ninstructions) break;

      u32 a_idx = i;
      u32 b_idx = i + 1;

      if (a_idx == UINT32_MAX || b_idx == UINT32_MAX) break;

      kit_ins* a = &cc->instructions[a_idx];
      kit_ins* b = &cc->instructions[b_idx];

      if (b->opcode != EIR_OPCODE_MOV) continue;

      u32 a_dst = get_destination_reg(a);

      if (a_dst == UINT32_MAX) continue; /* no destination */
      if (a_dst != b->mov.src) continue; /* isn't chained. */

      /* check if the first operation to a is a read (bad) or a write (good). */
      bool is_a_read_later = false;
      for (u32 j = b_idx + 1; j < cc->ninstructions; j++) {
        kit_ins* k   = &cc->instructions[j];
        u32      dst = get_destination_reg(k);

        u32 srcs[32] = { 0 };
        u32 nsrcs    = get_source_registers(k, srcs);

        /* a is read later. can't optimize :( */
        for (u32 s = 0; s < nsrcs; s++) {
          if (srcs[s] == a_dst) {
            is_a_read_later = true;

            break;
          }
        }

        if (is_a_read_later) break;

        /**
         * First operation to the register is a write
         * This means we can safely overwrite it.
         */
        if (dst == a_dst) break;
      }

      if (!is_a_read_later) {
        switch (a->opcode) {
          case EIR_OPCODE_MOV:
            // mov a,x  ;  mov b,a  →  mov b,x
            b->mov.src = a->mov.src;
            a->opcode  = EIR_OPCODE_NOP;
            changed    = true;
            break;

          case EIR_OPCODE_LOADK:
            // loadk a,id  ;  mov b,a  →  loadk b,id
            b->opcode    = EIR_OPCODE_LOADK;
            b->loadk.dst = b->mov.dst; // reuse the dst slot
            b->loadk.id  = a->loadk.id;
            a->opcode    = EIR_OPCODE_NOP;
            changed      = true;
            break;

          case EIR_OPCODE_MOVI:
            // movi a,val ; mov b,a → movi b,val
            b->opcode     = EIR_OPCODE_MOVI;
            b->movi.dst   = b->mov.dst;
            b->movi.value = a->movi.value;
            a->opcode     = EIR_OPCODE_NOP;
            changed       = true;
            break;

          case EIR_OPCODE_MOVF:
            // movf a,val ; mov b,a → movf b,val
            b->opcode     = EIR_OPCODE_MOVF;
            b->movf.dst   = b->mov.dst;
            b->movf.value = a->movf.value;
            a->opcode     = EIR_OPCODE_NOP;
            changed       = true;
            break;

          case EIR_OPCODE_GETG:
            // getg a,gid ; mov b,a → getg b,gid
            b->opcode   = EIR_OPCODE_GETG;
            b->getg.dst = b->mov.dst;
            b->getg.src = a->getg.src; // global id
            a->opcode   = EIR_OPCODE_NOP;
            changed     = true;
            break;

            // binop dst=x a,b ; move x, a
          case EIR_OPCODE_ADD:
          case EIR_OPCODE_SUB:
          case EIR_OPCODE_MUL:
          case EIR_OPCODE_DIV:
          case EIR_OPCODE_MOD:
          case EIR_OPCODE_EXP:
          case EIR_OPCODE_AND:
          case EIR_OPCODE_OR:
          case EIR_OPCODE_BAND:
          case EIR_OPCODE_BOR:
          case EIR_OPCODE_XOR:
          case EIR_OPCODE_EQL:
          case EIR_OPCODE_NEQ:
          case EIR_OPCODE_LT:
          case EIR_OPCODE_LTE:
          case EIR_OPCODE_GT:
          case EIR_OPCODE_GTE: {
            u32 dst = b->mov.dst;

            if (dst == a->binop.a || dst == a->binop.b) { continue; }

            b->opcode    = a->opcode;
            b->binop.a   = a->binop.a;
            b->binop.b   = a->binop.b;
            b->binop.dst = dst;

            a->opcode = EIR_OPCODE_NOP;

            changed = true;
            break;
          }

          default: break;
        }
      }

      /* There pairs are often produced from this pass, clean them up right here. */
      if (a->opcode == EIR_OPCODE_MOV && a->mov.src == a->mov.dst) { a->opcode = EIR_OPCODE_NOP; }
    }
  }

  return changed;
}

static bool
codegraph_preliminary_dead_store_elimination(kit_compiler* cc, const codegraph* cfg)
{
  bool changed = false;

  for (u32 b = 0; b < cfg->nblocks; b++) {
    codeblock* blk = &cfg->blocks[b];
    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      if (ip + 1 > blk->end) break;

      kit_ins* ins  = &cc->instructions[ip];
      kit_ins* next = &cc->instructions[ip + 1];

      u32 ins_dst = get_destination_reg(ins);
      u32 nxt_dst = get_destination_reg(next);

      if (ins_dst == nxt_dst && ins_dst != UINT32_MAX && nxt_dst != UINT32_MAX) {
        /* The first write is invalidated by the next. remove the first */
        ins->opcode = EIR_OPCODE_NOP;
        changed     = true;
      }
    }
  }

  return changed;
}

static int
remove_jmp_where_it_would_fallthrough(kit_compiler* cc)
{
  u32* label_map = kit_arnalloc(cc->arena, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32)); // UINT32_MAX = not found

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == EIR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  /* find a jump instruction */
  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];

    if (ins->opcode != EIR_OPCODE_JMP) continue;

    u32 target = label_map[ins->jmp.target];
    if (i > target) continue; /* skip backward jumps */

    /* check if there is nothing but noops in between the jump and the target */
    bool useless = true;
    for (u32 j = i + 1; j < target; j++) {
      kit_ins* k = &cc->instructions[j];

      // fprintf(stderr, "%i\n", k->opcode);

      if (!is_instruction_noop(k->opcode)) {
        useless = false;
        break;
      }
    }

    /* jmp is useless. noop it. */
    if (useless) { ins->opcode = EIR_OPCODE_NOP; }
  }

  // kit_arnfree(cc->arena, label_map);

  return 0;
}

static void
codegraph_free(kit_compiler* cc, codegraph* graph)
{
}

static inline bool
instruction_produces_constant_value(eir_opcode op)
{
  switch (op) {
    case EIR_OPCODE_MOVI:
    case EIR_OPCODE_MOVF:
    case EIR_OPCODE_LOADK: return true;
    default: return false;
  }
}

static inline int
get_instruction_constant_result(const kit_compiler* cc, const kit_ins* i, kit_var* result)
{
  switch (i->opcode) {
    case EIR_OPCODE_MOVI: {
      *result = kit_var_from_int(i->movi.value);
      return 0;
    }
    case EIR_OPCODE_MOVF: {
      *result = kit_var_from_float(i->movf.value);
      return 0;
    }
    case EIR_OPCODE_LOADK: {
      u32 id = i->loadk.id;
      for (u32 j = 0; j < cc->lit_table->literals_count; j++) {
        kit_var* lit = &cc->lit_table->literals[j];
        if (cc->lit_table->literal_hashes[j] != id) continue;

        kit_var_shallow_cpy(lit, result);

        return 0;
      }
      return -1;
    }
    default: break;
  }
  return -1;
}

static bool
codegraph_constant_folding(kit_compiler* cc, codegraph* cfg)
{
  bool changed_anything = false;

  bool changed = true;
  while (changed) {
    changed = false;
    /**
     * Read in 3 instructions at a time to find the following pattern:
     * movi a, 20
     * movf b, 30.0
     * neq dst=r0 a=a b=b
     *
     * and replace them with a single
     * loadk [true]
     *
     * We first do binary operations and then unary operations (slightly faster and cleaner).
     */
    u32 i = 0;
    while ((i = next_real_ins(cc, i)) != UINT32_MAX) {
      if (i + 2 >= cc->ninstructions) break;

      u32 one_idx   = i;
      u32 two_idx   = next_real_ins(cc, one_idx + 1);
      u32 three_idx = next_real_ins(cc, two_idx + 1);
      if (one_idx == UINT32_MAX || two_idx == UINT32_MAX || three_idx == UINT32_MAX) break;

      i = three_idx;

      kit_ins* one   = &cc->instructions[one_idx];
      kit_ins* two   = &cc->instructions[two_idx];
      kit_ins* three = &cc->instructions[three_idx];

      if (!instruction_produces_constant_value(one->opcode) || !instruction_produces_constant_value(two->opcode)) continue;
      // if (!is_instruction_binary_operation(three->opcode) && !is_instruction_unary_operation(three->opcode)) continue;
      if (!is_instruction_binary_operation(three->opcode)) continue;

      /* check if three depends on one and (or two) */
      u32 three_sources[32];
      get_source_registers(three, three_sources);

      u32 one_dst = get_destination_reg(one);
      u32 two_dst = get_destination_reg(two);

      bool found_one = three_sources[0] == one_dst;
      bool found_two = three_sources[1] == two_dst;

      /* if in reverse order, for some reason, swap one and two */
      if (three_sources[0] == two_dst && three_sources[1] == one_dst) {
        kit_ins* tmp = one;
        one          = two;
        two          = tmp;

        u32 tmp_dst = one_dst;
        one_dst     = two_dst;
        two_dst     = tmp_dst;

        found_one = true;
        found_two = true;
      }

      if (!found_one || !found_two) continue;

      kit_var one_result = KIT_NULLVAR;
      kit_var two_result = KIT_NULLVAR;

      if (get_instruction_constant_result(cc, one, &one_result) < 0) continue;
      if (get_instruction_constant_result(cc, two, &two_result) < 0) continue;

      kit_var result = operate(one_result, two_result, three->opcode);

      /* replace the three instructions with a single loadk */

      one->opcode = EIR_OPCODE_NOP;
      two->opcode = EIR_OPCODE_NOP;

      u32 three_dst = get_destination_reg(three);

      if (result.type == KIT_VARTYPE_INT) {
        three->opcode     = EIR_OPCODE_MOVI;
        three->movi.dst   = three_dst;
        three->movi.value = result.val.i;
      } else if (result.type == KIT_VARTYPE_FLOAT) {
        three->opcode     = EIR_OPCODE_MOVF;
        three->movf.dst   = three_dst;
        three->movf.value = result.val.f;
      } else if (result.type == KIT_VARTYPE_NULL) {
        three->opcode  = EIR_OPCODE_MOV;
        three->mov.dst = three_dst;
        three->mov.src = KIT_REG_NIL;
      } else {
        if (add_literal_to_track(cc, &result) < 0) continue;
        three->opcode    = EIR_OPCODE_LOADK;
        three->loadk.id  = kit_var_hash(&result);
        three->loadk.dst = three_dst;
      }

      changed          = true;
      changed_anything = true;
    }
  }

  /* now do unary operations */
  // for (u32 i = 0; i < cc->ninstructions; i++) {
  //   if (i + 2 >= cc->ninstructions) break;

  //   kit_ins* one = &cc->instructions[i];
  //   kit_ins* two = &cc->instructions[i + 1];

  //   if (!instruction_produces_constant_value(one->opcode)) continue;

  //   u32 one_dst = get_destination_reg(one);

  //   /* special case, conditional jumps */
  //   if ((two->opcode == EIR_OPCODE_JZ || two->opcode == EIR_OPCODE_JNZ) && two->cj.condition == one_dst) {
  //     /* load the constant */
  //     kit_var one_result = KIT_NULLVAR;
  //     if (get_instruction_constant_result(cc, one, &one_result) < 0) continue;

  //     bool b = kit_cast_to_bool(&one_result);

  //     if (two->opcode == EIR_OPCODE_JZ) {
  //       if (b) { /* condition always false, fallthrough */
  //         two->opcode = EIR_OPCODE_NOP;
  //       } else { /* condition always true, just jump to it */
  //         two->opcode     = EIR_OPCODE_JMP;
  //         two->jmp.target = two->cj.target;
  //       }
  //     }
  //     if (two->opcode == EIR_OPCODE_JNZ) {
  //       if (!b) { /* condition always false, fallthrough */
  //         two->opcode = EIR_OPCODE_NOP;
  //       } else { /* condition always true, just jump to it */
  //         two->opcode     = EIR_OPCODE_JMP;
  //         two->jmp.target = two->cj.target;
  //       }
  //     }
  //     continue;
  //   }

  //   // if (!is_instruction_binary_operation(three->opcode) && !is_instruction_unary_operation(three->opcode)) continue;
  //   if (!is_instruction_unary_operation(two->opcode)) continue;

  //   /* check if two depends on one */
  //   u32 two_sources[32];
  //   get_source_registers(two, two_sources);

  //   bool found_one = two_sources[0] == one_dst;

  //   if (!found_one) continue;

  //   kit_var one_result = KIT_NULLVAR;
  //   if (get_instruction_constant_result(cc, one, &one_result) < 0) continue;

  //   kit_var result = operate(KIT_NULLVAR, one_result, two->opcode);

  //   /* replace the three instructions with a single loadk */

  //   if (add_literal_to_track(cc, &result) < 0) continue;

  //   one->opcode = EIR_OPCODE_NOP;

  //   u32 two_dst = get_destination_reg(two);

  //   two->opcode    = EIR_OPCODE_LOADK;
  //   two->loadk.id  = kit_var_hash(&result);
  //   two->loadk.dst = two_dst;
  // }

  return changed_anything;
}

static int
strip_noops(kit_compiler* cc)
{
  /* just remove them. label pass will handle the indices later. */
  kit_ins* copy = kit_arnalloc(cc->arena, sizeof(kit_ins) * cc->ninstructions);
  memcpy(copy, cc->instructions, sizeof(kit_ins) * cc->ninstructions);

  u32 ctr = 0;
  for (u32 i = 0; i < cc->ninstructions; i++) {
    if (copy[i].opcode == EIR_OPCODE_NOP) continue;

    cc->instructions[ctr++] = copy[i];
  }

  cc->ninstructions = ctr;

  // kit_arnfree(cc->arena, copy);

  return 0;
}

/* Returns the new instruction count, -1 on error */
static u32
label_pass(kit_arena* arena, kit_ins* instructions, u32 ninstructions, u32 label_count)
{
  u32* label_map = calloc(label_count, sizeof(u32));
  memset(label_map, 0xFF, label_count * sizeof(u32)); // UINT32_MAX = not found

  kit_ins* copy = calloc(ninstructions, sizeof(kit_ins));
  memcpy(copy, instructions, sizeof(kit_ins) * ninstructions);

  u32 ctr = 0;
  for (u32 i = 0; i < ninstructions; i++) {
    kit_ins* ins = &instructions[i];

    if (copy[i].opcode == EIR_OPCODE_NOP) continue;

    if (ins->opcode == EIR_OPCODE_LABEL) {
      // fprintf(stderr, "defined %u vs %u\n", ins->label.id, label_count);
      label_map[ins->label.id] = ctr;
      continue;
    }

    instructions[ctr++] = copy[i];
  }

  ninstructions = ctr;

  /* patch jumps */
  for (u32 i = 0; i < ninstructions; i++) {
    kit_ins* ins = &instructions[i];
    switch (ins->opcode) {
      case EIR_OPCODE_JMP:
        // fprintf(stderr, "jmp %u vs %u\n", ins->jmp.target, label_count);
        ins->jmp.target = label_map[ins->jmp.target];
        break;
      case EIR_OPCODE_JZ:
      case EIR_OPCODE_JNZ:
        // fprintf(stderr, "cj %u vs %u\n", ins->cj.target, label_count);
        ins->cj.target = label_map[ins->cj.target];
        break;
      default: break;
    }
  }

  free(copy);
  free(label_map);

  return ctr;
}

static bool
codegraph_eliminate_unreachable_code(kit_compiler* cc, codegraph* cfg)
{
  bool changed = false;

  bool* reachable = kit_arnalloc(cfg->arena, cfg->nblocks * sizeof(bool));
  memset(reachable, 0, cfg->nblocks * sizeof(bool));
  reachable[0] = true;

  /* Do a DFS to find dead blocks. */
  u32* stack  = kit_arnalloc(cfg->arena, cfg->nblocks * sizeof(u32));
  u32  sp     = 0;
  stack[sp++] = 0;
  while (sp > 0) {
    u32 b = stack[--sp];
    for (u32 s = 0; s < cfg->blocks[b].nsuccessors; s++) {
      u32 successor = cfg->blocks[b].successors[s];
      if (!reachable[successor]) {
        reachable[successor] = true;
        stack[sp++]          = successor;
      }
    }
  }

  /* unreachable block. nop it out. */
  for (u32 i = 0; i < cfg->nblocks; i++) {
    if (reachable[i]) continue;

    codeblock* blk = &cfg->blocks[i];
    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      // dont kill labels!!
      if (cc->instructions[ip].opcode != EIR_OPCODE_LABEL) {
        changed                     = true;
        cc->instructions[ip].opcode = EIR_OPCODE_NOP;
      }
    }
  }

  return changed;
  // kit_arnfree(cfg->arena, reachable);
}

static inline int
replace_move_with_constant_load(kit_compiler* cc, const codegraph* cfg, kit_ins* ins, u32 dst_reg, const kit_var* value)
{
  if (value->type == KIT_VARTYPE_INT) {
    ins->opcode     = EIR_OPCODE_MOVI;
    ins->movi.dst   = dst_reg;
    ins->movi.value = value->val.i;
  } else if (value->type == KIT_VARTYPE_FLOAT) {
    ins->opcode     = EIR_OPCODE_MOVF;
    ins->movf.dst   = dst_reg;
    ins->movf.value = value->val.f;
  } else {
    if (add_literal_to_track(cc, value) < 0) return -1;

    ins->opcode    = EIR_OPCODE_LOADK;
    ins->loadk.id  = kit_var_hash(value);
    ins->loadk.dst = dst_reg;
  }

  return 0;
}

static bool
codegraph_constant_propagation(kit_compiler* cc, codegraph* cfg)
{
  kit_var* regs         = kit_arnalloc(cfg->arena, cfg->nvregs * sizeof(kit_var));
  bool*    values_known = kit_arnalloc(cfg->arena, cfg->nvregs * sizeof(bool));

  const u32 nvregs = cfg->nvregs;
  for (u32 i = 0; i < nvregs; i++) {
    regs[i]         = KIT_NULLVAR;
    values_known[i] = false;
  }

  bool changed = false;

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];

    if (ins->opcode == EIR_OPCODE_CALL) {
      memset(values_known, 0, nvregs * sizeof(bool));
      values_known[ins->call.dst] = false;
      continue;
    }

    /* impure instruction. flush state and move on. */
    if (ins->opcode != EIR_OPCODE_LABEL && ins->opcode != EIR_OPCODE_INDEX_ASSIGN && ins->opcode != EIR_OPCODE_MEMBER_ASSIGN
        && is_instruction_impure(ins->opcode)) {
      /* this is the first optimization pass, assume jumps wreck all registers */
      memset(values_known, 0, nvregs * sizeof(bool));
      continue;
    }

    switch (ins->opcode) {
      case EIR_OPCODE_NOP: break;

      case EIR_OPCODE_MOV: {
        u32 dst = ins->mov.dst;
        u32 src = ins->mov.src;

        /* If the value is already in dst, turn this instruction into a NOOP */
        if (values_known[dst] && values_known[src] && kit_var_equal(&regs[dst], &regs[src])) {
          changed     = true;
          ins->opcode = EIR_OPCODE_NOP;
          continue;
        }

        if (values_known[src]) {
          changed = true;
          replace_move_with_constant_load(cc, cfg, ins, dst, &regs[src]);
          values_known[dst] = true;
        }

        regs[dst]         = regs[src];
        values_known[dst] = values_known[src]; /* if the value of src is known, then the value of dst is known */

        break;
      }
      case EIR_OPCODE_MOVI: {
        regs[ins->movi.dst]         = kit_var_from_int(ins->movi.value);
        values_known[ins->movi.dst] = true;
        break;
      }
      case EIR_OPCODE_MOVF: {
        regs[ins->movf.dst]         = kit_var_from_float(ins->movf.value);
        values_known[ins->movf.dst] = true;
        break;
      }

      case EIR_OPCODE_GETG:
      case EIR_OPCODE_SETG:
      case EIR_OPCODE_MOVG: break;

      case EIR_OPCODE_NEQ:
      case EIR_OPCODE_EQL: {
        /* special case, if both point to the same register */
        u32 a   = ins->binop.a;
        u32 b   = ins->binop.b;
        u32 dst = ins->binop.dst;
        if (a == b) {
          values_known[dst] = true;
          changed           = true;
          /* register self comparison will always result in true (for EQL)*/
          regs[dst] = kit_var_from_bool(ins->opcode == EIR_OPCODE_EQL ? true : false);

          replace_move_with_constant_load(cc, cfg, ins, dst, &regs[dst]);
          break;
        }
        /* fallthrough */
      }

      case EIR_OPCODE_ADD:
      case EIR_OPCODE_SUB:
      case EIR_OPCODE_MUL:
      case EIR_OPCODE_DIV:
      case EIR_OPCODE_MOD:
      case EIR_OPCODE_EXP:
      case EIR_OPCODE_AND:
      case EIR_OPCODE_OR:
      case EIR_OPCODE_BAND:
      case EIR_OPCODE_BOR:
      case EIR_OPCODE_XOR:
      case EIR_OPCODE_LT:
      case EIR_OPCODE_LTE:
      case EIR_OPCODE_GT:
      case EIR_OPCODE_GTE:
        /* if value of both operands are known, compute it and store it */ {
          if (!values_known[ins->binop.a] || !values_known[ins->binop.b]) {
            values_known[ins->binop.dst] = false;
            break;
          }

          kit_var a      = regs[ins->binop.a];
          kit_var b      = regs[ins->binop.b];
          kit_var result = operate(a, b, ins->opcode);

          u32 dst = ins->binop.dst;
          replace_move_with_constant_load(cc, cfg, ins, dst, &result);

          changed = true;

          regs[dst]         = result;
          values_known[dst] = true;
          break;
        }

      case EIR_OPCODE_BNOT:
      case EIR_OPCODE_NEG:
      case EIR_OPCODE_NOT:
      case EIR_OPCODE_INC:
      case EIR_OPCODE_DEC: {
        if (!values_known[ins->unop.a]) {
          values_known[ins->unop.dst] = false;
          break;
        }

        kit_var a      = regs[ins->unop.a];
        kit_var result = operate(KIT_NULLVAR, a, ins->opcode);

        u32 dst = ins->unop.dst;
        replace_move_with_constant_load(cc, cfg, ins, dst, &result);

        changed = true;

        regs[dst]         = result;
        values_known[dst] = true;
        break;
      }

      case EIR_OPCODE_LOADK: {
        u32 id  = ins->loadk.id;
        u32 dst = ins->loadk.dst;

        values_known[dst] = false; // set when we find it.

        for (u32 j = 0; j < cc->lit_table->literals_count; j++) {
          kit_var* lit = &cc->lit_table->literals[j];
          if (cc->lit_table->literal_hashes[j] != id) continue;

          kit_var tmp = *lit;
          replace_move_with_constant_load(cc, cfg, ins, ins->loadk.dst, lit);

          changed           = true;
          regs[dst]         = tmp;
          values_known[dst] = true;

          break;
        }

        break;
      }

      case EIR_OPCODE_JMP:
      case EIR_OPCODE_JZ:
      case EIR_OPCODE_JNZ: {
        memset(values_known, 0, nvregs * sizeof(bool));
        break;
      }

      case EIR_OPCODE_ASSERT: {
        u32 cond = ins->assertion.cond;

        /* assertion is always true, noop it. */
        if (values_known[cond] && kit_var_to_bool(regs[cond])) { ins->opcode = EIR_OPCODE_NOP; }

        break;
      }

      default: break;
    }
  }

  return changed;
}

static inline void
update_reg(kit_reg_t* ptr, u32 offset)
{
  if (*ptr >= KIT_REG_GENERAL_BEGIN) *ptr += offset;
}

/* add the offset to each register in ins and pass it to kit_emit_ins */
static inline void
patch_instruction_for_function_inlining(
    kit_compiler* cc, kit_ins* ins, u32 register_offset, u32 label_offset, kit_vreg_t return_register, u32 exit_label)
{
  switch (ins->opcode) {
    case EIR_OPCODE_LABEL: {
      ins->label.id += label_offset;
      break;
    }

    case EIR_OPCODE_NOP: break;

    case EIR_OPCODE_MOV: {
      update_reg(&ins->mov.dst, register_offset);
      update_reg(&ins->mov.src, register_offset);
      break;
    }
    case EIR_OPCODE_MOVI: {
      update_reg(&ins->movi.dst, register_offset);
      break;
    }
    case EIR_OPCODE_MOVF: {
      update_reg(&ins->movf.dst, register_offset);
      break;
    }
    case EIR_OPCODE_GETG: {
      update_reg(&ins->getg.dst, register_offset);
      break;
    }
    case EIR_OPCODE_SETG: {
      update_reg(&ins->setg.src, register_offset);
      break;
    }
    case EIR_OPCODE_MOVG: {
      break;
    }
    case EIR_OPCODE_ADD:
    case EIR_OPCODE_SUB:
    case EIR_OPCODE_MUL:
    case EIR_OPCODE_DIV:
    case EIR_OPCODE_MOD:
    case EIR_OPCODE_EXP:
    case EIR_OPCODE_AND:
    case EIR_OPCODE_OR:
    case EIR_OPCODE_BAND:
    case EIR_OPCODE_BOR:
    case EIR_OPCODE_XOR:
    case EIR_OPCODE_EQL:
    case EIR_OPCODE_NEQ:
    case EIR_OPCODE_LT:
    case EIR_OPCODE_LTE:
    case EIR_OPCODE_GT:
    case EIR_OPCODE_GTE: {
      update_reg(&ins->binop.a, register_offset);
      update_reg(&ins->binop.b, register_offset);
      update_reg(&ins->binop.dst, register_offset);
      break;
    }

    case EIR_OPCODE_BNOT:
    case EIR_OPCODE_NEG:
    case EIR_OPCODE_NOT:
    case EIR_OPCODE_INC:
    case EIR_OPCODE_DEC: {
      update_reg(&ins->unop.a, register_offset);
      update_reg(&ins->unop.dst, register_offset);
      break;
    }

    case EIR_OPCODE_RET: {
      update_reg(&ins->ret.return_value, register_offset);

      /* move the return value to the common return register */
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = EIR_OPCODE_MOV, .dst = return_register, .src = ins->ret.return_value } });

      /* convert this ret into a jump to the exit label */
      ins->jmp.opcode = EIR_OPCODE_JMP;
      ins->jmp.target = exit_label;

      /* gets emitted after the switch */
      break;
    }

    case EIR_OPCODE_MK_LIST: {
      update_reg(&ins->mk_list.dst, register_offset);
      break;
    }

    case EIR_OPCODE_MK_MAP: {
      update_reg(&ins->mk_map.dst, register_offset);
      break;
    }

    case EIR_OPCODE_INDEX: {
      update_reg(&ins->index.dst, register_offset);
      update_reg(&ins->index.base, register_offset);
      update_reg(&ins->index.index, register_offset);
      break;
    }
    case EIR_OPCODE_INDEX_ASSIGN: {
      update_reg(&ins->index_assign.value, register_offset);
      update_reg(&ins->index_assign.base, register_offset);
      update_reg(&ins->index_assign.index, register_offset);
      break;
    }
    case EIR_OPCODE_MEMBER_ACCESS: {
      update_reg(&ins->member_access.dst, register_offset);
      update_reg(&ins->member_access.base, register_offset);
      break;
    }
    case EIR_OPCODE_MEMBER_ASSIGN: {
      update_reg(&ins->member_assign.value, register_offset);
      update_reg(&ins->member_assign.base, register_offset);
      break;
    }
    case EIR_OPCODE_MK_STRUCT: {
      update_reg(&ins->mk_struct.dst, register_offset);
      break;
    }
    case EIR_OPCODE_CALL: {
      update_reg(&ins->call.dst, register_offset);
      break;
    }

    case EIR_OPCODE_JZ:
    case EIR_OPCODE_JNZ: {
      ins->cj.target += label_offset;
      update_reg(&ins->cj.condition, register_offset);
      break;
    }

    case EIR_OPCODE_JMP: {
      ins->jmp.target += label_offset;
      break;
    }

    case EIR_OPCODE_PUSH:
    case EIR_OPCODE_POP: {
      update_reg(&ins->push.reg, register_offset);
      break;
    }

    case EIR_OPCODE_LOADK: {
      update_reg(&ins->loadk.dst, register_offset);
      break;
    }

    default: break;
  }
}

static void
opt_inline_function_calls(kit_compiler* cc)
{
  kit_ins* old_instructions = kit_arnalloc(cc->arena, sizeof(kit_ins) * cc->ninstructions);
  memcpy(old_instructions, cc->instructions, sizeof(kit_ins) * cc->ninstructions);

  const u32 old_instruction_count = cc->ninstructions;

  cc->ninstructions = 0;
  for (u32 i = 0; i < old_instruction_count; i++) {
    kit_ins* ins = &old_instructions[i];

    /* normal instruction, push it to our stream. */
    if (ins->opcode != EIR_OPCODE_CALL) {
      kit_emit_ins(cc, *ins);
      continue;
    }

    /* CALL: replace call contents with our stored bytecode */
    u32            hash   = ins->call.function_id;
    kitc_function* lookup = NULL;

    for (u32 j = 0; j < cc->function_table->functions_count; j++) {
      kitc_function* func = &cc->function_table->functions[j];

      if (func->name_hash == hash) {
        lookup = func;
        break;
      }
    }

    /* builtin function or something */
    if (!lookup || get_cost_for_inlining_function(cc, 0, lookup) > 100) {
      /**
       * Haha. I spent 2 hours (on and off) trying to find why the main function was empty.
       * I disabled all optimizations, noticed it was only happening with function inlining.
       * Oh no? Someone is removing the call println, presumably before inlining starts?
       * I must tread through the optimizer code and find out what is causing it.
       * I even went to claude with my problem, to no avail (kill me later).
       *
       * If you couldn't tell, any calls to println were being discarded by the absence of this line,
       * and the optimizer correctly deleted all argument register moves and as it kept optimizing
       * the function (16 passes btw), it eventually deleted the whole function (I think that's a bug).
       */
      kit_emit_ins(cc, *ins);
      continue;
    }

    kit_vreg_t call_dst = (kit_vreg_t)ins->call.dst;

    u32 register_offset = cc->next_vreg; // All registers in the function get this added to them
    u32 label_offset    = cc->next_label;

    u32 exit_label = cc->next_label + lookup->labels_used;

    /* start emitting the function code */
    for (u32 j = 0; j < lookup->code_count; j++) {
      kit_ins sdf = lookup->code[j];

      patch_instruction_for_function_inlining(cc, &sdf, register_offset, label_offset, call_dst, exit_label);

      kit_emit_ins(cc, sdf);
    }

    /* push the number of registers up  */
    cc->next_vreg += (kit_vreg_t)lookup->vregs_used;
    cc->next_label += lookup->labels_used;

    /* return instructions jump here */
    define_and_emit_label(cc, exit_label);
    cc->next_label++;
  }

  // kit_arnfree(cc->arena, old_instructions);
}

static bool
codegraph_local_copy_propagation(kit_compiler* cc, codegraph* cfg)
{
  bool changed = false;

  u32* copy_map = kit_arnalloc(cfg->arena, cc->next_vreg * sizeof(u32));
  for (u32 b = 0; b < cfg->nblocks; b++) {
    codeblock* blk = &cfg->blocks[b];

    for (u32 r = 0; r < cc->next_vreg; r++) copy_map[r] = r;

    for (u32 ip = blk->start; ip <= blk->end; ip++) {
      kit_ins* ins = &cc->instructions[ip];
      if (ins->opcode == EIR_OPCODE_NOP || ins->opcode == EIR_OPCODE_LABEL) continue;

      switch ((eir_opcode_bits)ins->opcode) {
          /* getg, setg and movg  */
        case EIR_OPCODE_MOV:
          ins->mov.src = copy_map[ins->mov.src];
          changed      = true;
          break;

        case EIR_OPCODE_ASSERT:
          ins->assertion.cond = copy_map[ins->assertion.cond];
          changed             = true;
          break;

        case EIR_OPCODE_MOVI:
        case EIR_OPCODE_MOVF: /* values are not registers */
        case EIR_OPCODE_GETG: break;

        case EIR_OPCODE_SETG:
          ins->mov.src = copy_map[ins->mov.src];
          changed      = true;
          break;

        case EIR_OPCODE_MOVG:
        case EIR_OPCODE_LOADK: /* id is not a register */ break;

        case EIR_OPCODE_ADD:
        case EIR_OPCODE_SUB:
        case EIR_OPCODE_MUL:
        case EIR_OPCODE_DIV:
        case EIR_OPCODE_MOD:
        case EIR_OPCODE_EXP:
        case EIR_OPCODE_AND:
        case EIR_OPCODE_OR:
        case EIR_OPCODE_BAND:
        case EIR_OPCODE_BOR:
        case EIR_OPCODE_XOR:
        case EIR_OPCODE_EQL:
        case EIR_OPCODE_NEQ:
        case EIR_OPCODE_LT:
        case EIR_OPCODE_LTE:
        case EIR_OPCODE_GT:
        case EIR_OPCODE_GTE:
          ins->binop.a = copy_map[ins->binop.a];
          ins->binop.b = copy_map[ins->binop.b];
          changed      = true;
          break;

        case EIR_OPCODE_NOT:
        case EIR_OPCODE_NEG:
        case EIR_OPCODE_BNOT:
        case EIR_OPCODE_DEC:
        case EIR_OPCODE_INC:
          ins->unop.a = copy_map[ins->unop.a];
          changed     = true;
          break;

        /* can't rewrite argument vector */
        case EIR_OPCODE_MK_LIST:
        case EIR_OPCODE_MK_MAP:
        case EIR_OPCODE_MK_STRUCT:
        case EIR_OPCODE_CALL: break;

        case EIR_OPCODE_INDEX:
          ins->index.base  = copy_map[ins->index.base];
          ins->index.index = copy_map[ins->index.index];
          changed          = true;
          break;
        case EIR_OPCODE_INDEX_ASSIGN:
          ins->index_assign.value = copy_map[ins->index_assign.value];
          ins->index_assign.index = copy_map[ins->index_assign.index];
          ins->index_assign.base  = copy_map[ins->index_assign.base];
          changed                 = true;
          break;

        case EIR_OPCODE_MEMBER_ACCESS:
          ins->member_access.base = copy_map[ins->member_access.base];
          changed                 = true;
          break;
        case EIR_OPCODE_MEMBER_ASSIGN:
          ins->member_assign.value = copy_map[ins->member_assign.value];
          ins->member_assign.base  = copy_map[ins->member_assign.base];
          changed                  = true;
          break;
        case EIR_OPCODE_RET:
          ins->ret.return_value = copy_map[ins->ret.return_value];
          changed               = true;
          break;
        case EIR_OPCODE_JZ:
        case EIR_OPCODE_JNZ:
          ins->cj.condition = copy_map[ins->cj.condition];
          changed           = true;
          break;
        case EIR_OPCODE_PUSH: {
          ins->push.reg = copy_map[ins->push.reg];
          changed       = true;
          break;
        }

        case EIR_OPCODE_POP:
        case EIR_OPCODE_NOP:
        case EIR_OPCODE_LABEL:
        case EIR_OPCODE_JMP: {
          break;
        }
      }

      u32 dst = get_destination_reg(ins);
      if (dst != UINT32_MAX) {
        if (ins->opcode == EIR_OPCODE_MOV) {
          u32 src = ins->mov.src;
          if (src < KIT_REG_GENERAL_BEGIN) {
            copy_map[dst] = dst; /* fixed registers always die in any circumstance */
            changed       = true;
          } else {
            copy_map[dst] = src;
            if (dst == src) ins->opcode = EIR_OPCODE_NOP;
            changed = true;
          }
        } else {
          /* kill the old value */
          copy_map[dst] = dst;
        }
      }
    }
  }

  // kit_arnfree(cfg->arena, copy_map);
  return changed;
}

static void
forward_dead_moves(kit_compiler* cc)
{
  return;
  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* mov = &cc->instructions[i];
    if (mov->opcode != EIR_OPCODE_MOV) continue;

    u32 dst = mov->mov.dst;
    u32 src = mov->mov.src;

    // Find the point where dst is redefined (or end)
    u32 redef_idx = UINT32_MAX;
    for (u32 j = i + 1; j < cc->ninstructions; j++) {
      if (is_instruction_noop(cc->instructions[j].opcode)) continue;
      u32 def = get_destination_reg(&cc->instructions[j]);
      if (def == dst) {
        redef_idx = j;
        break;
      }
    }
    u32 limit = (redef_idx == UINT32_MAX) ? cc->ninstructions : redef_idx;

    bool src_redefined = false;
    for (u32 j = i + 1; j < limit; j++) {
      const kit_ins* look = &cc->instructions[j];
      if (is_instruction_noop(look->opcode)) continue;

      if (is_instruction_impure(look->opcode)) break;

      u32 def = get_destination_reg(look);
      if (def == src) {
        src_redefined = true;
        break;
      }
    }
    if (src_redefined) continue;

    bool any_replaced = false;
    for (u32 j = i + 1; j < limit; j++) {
      kit_ins* ins = &cc->instructions[j];
      if (is_instruction_noop(ins->opcode)) continue;

      u32  srcs[32];
      u32  nsrc     = get_source_registers(ins, srcs);
      bool uses_dst = false;
      for (u32 s = 0; s < nsrc; s++) {
        if (srcs[s] == dst) {
          uses_dst = true;
          break;
        }
      }

      /* doesn't use our destination, skip */
      if (!uses_dst) continue;

      /* Replace dst with src in the instruction's source operands */
      switch (ins->opcode) {
        case EIR_OPCODE_MOV:
          if (ins->mov.src == dst) ins->mov.src = src;
          break;
        case EIR_OPCODE_SETG:
          if (ins->setg.src == dst) ins->setg.src = src;
          break;
        case EIR_OPCODE_ADD:
        case EIR_OPCODE_SUB:
        case EIR_OPCODE_MUL:
        case EIR_OPCODE_DIV:
        case EIR_OPCODE_MOD:
        case EIR_OPCODE_EXP:
        case EIR_OPCODE_AND:
        case EIR_OPCODE_OR:
        case EIR_OPCODE_BAND:
        case EIR_OPCODE_BOR:
        case EIR_OPCODE_XOR:
        case EIR_OPCODE_EQL:
        case EIR_OPCODE_NEQ:
        case EIR_OPCODE_LT:
        case EIR_OPCODE_LTE:
        case EIR_OPCODE_GT:
        case EIR_OPCODE_GTE:
          if (ins->binop.a == dst) ins->binop.a = src;
          if (ins->binop.b == dst) ins->binop.b = src;
          break;
        case EIR_OPCODE_NOT:
        case EIR_OPCODE_NEG:
        case EIR_OPCODE_BNOT:
        case EIR_OPCODE_INC:
        case EIR_OPCODE_DEC:
          if (ins->unop.a == dst) ins->unop.a = src;
          break;
        case EIR_OPCODE_RET:
          if (ins->ret.return_value == dst) ins->ret.return_value = src;
          break;
        case EIR_OPCODE_INDEX:
          if (ins->index.base == dst) ins->index.base = src;
          if (ins->index.index == dst) ins->index.index = src;
          break;
        case EIR_OPCODE_INDEX_ASSIGN:
          if (ins->index_assign.base == dst) ins->index_assign.base = src;
          if (ins->index_assign.index == dst) ins->index_assign.index = src;
          if (ins->index_assign.value == dst) ins->index_assign.value = src;
          break;
        case EIR_OPCODE_MEMBER_ACCESS:
          if (ins->member_access.base == dst) ins->member_access.base = src;
          break;
        case EIR_OPCODE_MEMBER_ASSIGN:
          if (ins->member_assign.base == dst) ins->member_assign.base = src;
          if (ins->member_assign.value == dst) ins->member_assign.value = src;
          break;
        case EIR_OPCODE_JZ:
        case EIR_OPCODE_JNZ:
          if (ins->cj.condition == dst) ins->cj.condition = src;
          break;
        case EIR_OPCODE_PUSH:
          if (ins->push.reg == dst) ins->push.reg = src;
          break;
        case EIR_OPCODE_ASSERT:
          if (ins->assertion.cond == dst) ins->assertion.cond = src;
          break;
        default: break;
      }
      any_replaced = true;
    }

    if (any_replaced) {
      mov->opcode = EIR_OPCODE_NOP; // safe to delete the move
    }
  }
}

static int
era_compute_ranges(kit_compiler* cc, era_state* ra)
{
  memset(ra, 0, sizeof(*ra));

  const u32 vreg_count = cc->next_vreg;

  ra->ranges = kit_xalloc(vreg_count, sizeof(era_range));
  if (!ra->ranges) return -1;

  ra->vreg_to_phys = kit_xalloc(vreg_count, sizeof(u32));
  if (!ra->vreg_to_phys) {
    free(ra->ranges);
    ra->ranges = NULL;
    return -1;
  }

  for (u32 i = 0; i < vreg_count; i++) {
    ra->ranges[i].start = UINT32_MAX;
    ra->ranges[i].end   = 0;
    ra->ranges[i].vreg  = i;
  }

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins ins = cc->instructions[i];

    /* mark vreg is defined at this instruction */
#define WRITES_TO(r)                                                                                                                                 \
  do {                                                                                                                                               \
    u32 _r = (r);                                                                                                                                    \
    if (ra->ranges[_r].start == UINT32_MAX) ra->ranges[_r].start = i;                                                                                \
    if (i > ra->ranges[_r].end) ra->ranges[_r].end = i;                                                                                              \
  } while (0)

    /* mark vreg is used at this instruction */
#define READS_FROM(r)                                                                                                                                \
  do {                                                                                                                                               \
    u32 _r = (r);                                                                                                                                    \
    if (i > ra->ranges[_r].end) ra->ranges[_r].end = i;                                                                                              \
  } while (0)

    switch ((eir_opcode_bits)ins.opcode) {
      /* getg, setg and movg  */
      case EIR_OPCODE_MOV:
        WRITES_TO(ins.mov.dst);
        READS_FROM(ins.mov.src);
        break;

      case EIR_OPCODE_MOVI: WRITES_TO(ins.movi.dst); break; /* values are not registers */
      case EIR_OPCODE_MOVF: WRITES_TO(ins.movf.dst); break; /* values are not registers */

      case EIR_OPCODE_ASSERT: READS_FROM(ins.assertion.cond); break; /* values are not registers */

      case EIR_OPCODE_GETG: WRITES_TO(ins.mov.dst); break;
      case EIR_OPCODE_SETG: READS_FROM(ins.mov.src); break;
      case EIR_OPCODE_MOVG: {
        break;
      }
      case EIR_OPCODE_LOADK:
        WRITES_TO(ins.loadk.dst);
        break; /* id is not a register */

      // case EIR_OPCODE_LOADK: break;
      case EIR_OPCODE_ADD:
      case EIR_OPCODE_SUB:
      case EIR_OPCODE_MUL:
      case EIR_OPCODE_DIV:
      case EIR_OPCODE_MOD:
      case EIR_OPCODE_EXP:
      case EIR_OPCODE_AND:
      case EIR_OPCODE_OR:
      case EIR_OPCODE_BAND:
      case EIR_OPCODE_BOR:
      case EIR_OPCODE_XOR:
      case EIR_OPCODE_EQL:
      case EIR_OPCODE_NEQ:
      case EIR_OPCODE_LT:
      case EIR_OPCODE_LTE:
      case EIR_OPCODE_GT:
      case EIR_OPCODE_GTE:
        WRITES_TO(ins.binop.dst);
        READS_FROM(ins.binop.a);
        READS_FROM(ins.binop.b);
        break;
      case EIR_OPCODE_NOT:
      case EIR_OPCODE_NEG:
      case EIR_OPCODE_BNOT:
      case EIR_OPCODE_DEC:
      case EIR_OPCODE_INC:
        WRITES_TO(ins.unop.dst);
        READS_FROM(ins.unop.a);
        break;
      case EIR_OPCODE_CALL:
        for (u32 j = KIT_REG_ARG0; j < MIN(ins.call.nargs, KIT_REG_ARG_COUNT); j++) { READS_FROM(j); }
        WRITES_TO(ins.call.dst);
        break;
      case EIR_OPCODE_INDEX:
        WRITES_TO(ins.index.dst);
        READS_FROM(ins.index.base);
        READS_FROM(ins.index.index);
        break;
      case EIR_OPCODE_INDEX_ASSIGN:
        READS_FROM(ins.index_assign.value);
        READS_FROM(ins.index_assign.index);
        READS_FROM(ins.index_assign.base);
        break;
      case EIR_OPCODE_MK_LIST:
        for (u32 j = KIT_REG_ARG0; j < MIN(ins.mk_list.nelems, KIT_REG_ARG_COUNT); j++) { READS_FROM(j); }
        WRITES_TO(ins.mk_list.dst);
        break;
      case EIR_OPCODE_MK_MAP:
        for (u32 j = KIT_REG_ARG0; j < MIN(ins.mk_map.npairs * 2, KIT_REG_ARG_COUNT); j++) { READS_FROM(j); }
        WRITES_TO(ins.mk_map.dst);
        break;
      case EIR_OPCODE_MK_STRUCT:
        /* we don't know how many members this instruction will initialize. clobber the entire argument vector. */
        for (u32 j = KIT_REG_ARG0; j < KIT_REG_ARG_COUNT; j++) { READS_FROM(j); }
        WRITES_TO(ins.mk_struct.dst);
        break;
      case EIR_OPCODE_MEMBER_ACCESS:
        WRITES_TO(ins.member_access.dst);
        READS_FROM(ins.member_access.base);
        break;
      case EIR_OPCODE_MEMBER_ASSIGN:
        READS_FROM(ins.member_assign.value);
        READS_FROM(ins.member_assign.base);
        break;
      case EIR_OPCODE_RET: READS_FROM(ins.ret.return_value); break;
      case EIR_OPCODE_JZ:
      case EIR_OPCODE_JNZ: READS_FROM(ins.cj.condition); break;
      case EIR_OPCODE_PUSH: {
        READS_FROM(ins.push.reg);
        break;
      }
      case EIR_OPCODE_POP: {
        WRITES_TO(ins.pop.reg);
        break;
      }

        // default: break;
      case EIR_OPCODE_NOP:
      case EIR_OPCODE_LABEL:
      case EIR_OPCODE_JMP: {
        break;
      }
    }
#undef WRITES_TO
#undef READS_FROM
  }

  // collect only ranges that were actually used
  ra->nranges = 0;
  for (u32 i = 0; i < vreg_count; i++) {
    if (ra->ranges[i].start != UINT32_MAX) { ra->ranges[ra->nranges++] = ra->ranges[i]; }
  }

  return 0;
}

static int
era_cmp_start(const void* a, const void* b)
{ return (int)((const era_range*)a)->start - (int)((const era_range*)b)->start; }
// static int
// era_cmp_end(const void* a, const void* b)
// { return (int)((const era_range*)a)->end - (int)((const era_range*)b)->end; }

static int
era_allocate(int opt_level, era_state* ra)
{
  qsort(ra->ranges, ra->nranges, sizeof(era_range), era_cmp_start);

  bool phys_free[ERA_NUM_PHYS];
  memset(phys_free, 1, sizeof(phys_free));

  /* mark non general registers as always in use */
  if (opt_level >= 1) {
    /* OPTIMIZATION: Allow usage of non general registers (they're marked free). */
  } else {
    for (u32 i = 0; i < KIT_REG_GENERAL_BEGIN; i++) { phys_free[i] = false; }
  }

  phys_free[ERA_SPILL_SCRATCH] = false;

  era_range* active[ERA_NUM_PHYS] = { 0 };
  u32        nactive              = 0;
  u32        next_spill           = 0;

  for (u32 i = 0; i < ra->nranges; i++) {
    era_range* r = &ra->ranges[i];

    // expire intervals that ended before this one starts
    u32 new_nactive = 0;
    for (u32 j = 0; j < nactive; j++) {
      if (active[j]->end < r->start) {
        phys_free[active[j]->phys] = true;
      } else {
        active[new_nactive++] = active[j];
      }
    }
    nactive = new_nactive;

    // find a free physical register
    u32 phys = UINT32_MAX;

    /* skip non general registers */
    for (u32 p = KIT_REG_GENERAL_BEGIN; p < ERA_NUM_PHYS; p++) {
      if (phys_free[p]) {
        phys = p;
        break;
      }
    }

    // need spill?
    if (phys == UINT32_MAX) {
      // find active interval with furthest end
      u32 spill_idx = 0;
      for (u32 j = 1; j < nactive; j++) {
        if (active[j]->end > active[spill_idx]->end) spill_idx = j;
      }
      era_range* spill = active[spill_idx];

      if (spill->end > r->end) {
        r->phys             = spill->phys;
        spill->spill_offset = next_spill * 8;
        spill->phys         = ERA_SPILL_FLAG | next_spill++;
        active[spill_idx]   = r; // replace
      } else {
        // r itself gets spilled
        r->spill_offset           = next_spill * 8;
        r->phys                   = ERA_SPILL_FLAG | next_spill++;
        ra->vreg_to_phys[r->vreg] = r->phys;
        continue;
      }
    } else {
      phys_free[phys] = false;
      r->phys         = phys;
      // insert into active sorted by end
      u32 ins_pos = nactive++;
      while (ins_pos > 0 && active[ins_pos - 1]->end > r->end) {
        active[ins_pos] = active[ins_pos - 1];
        ins_pos--;
      }
      active[ins_pos] = r;
    }

    ra->vreg_to_phys[r->vreg] = r->phys;
  }

  return 0;
}

static int
era_rewrite(kit_compiler* cc, const u32* vreg_to_phys)
{
#define MAP(r)                                                                                                                                       \
  do {                                                                                                                                               \
    u32 _r = (r);                                                                                                                                    \
    /*                                                                                                                                               \
    if (_r & ERA_SPILL_FLAG) {                                                                                                                       \
      fprintf(stderr, "Spilled register %u not rewritten\n", r);                                                                                     \
      (r) = KIT_REG_NIL;                                                                                                                             \
    }                                                                                                                                                \
      */                                                                                                                                             \
    if (_r < KIT_REG_GENERAL_BEGIN) break;                                                                                                           \
    if (!(_r & ERA_SPILL_FLAG)) (r) = vreg_to_phys[_r];                                                                                              \
  } while (0)

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins ins = cc->instructions[i];

    switch ((eir_opcode_bits)ins.opcode) {
      case EIR_OPCODE_MOV:
        MAP(ins.mov.dst);
        MAP(ins.mov.src);
        break;

      case EIR_OPCODE_MOVI: MAP(ins.movi.dst); break;
      case EIR_OPCODE_MOVF: MAP(ins.movf.dst); break;

      case EIR_OPCODE_PUSH:
      case EIR_OPCODE_POP: {
        MAP(ins.push.reg);
        break;
      }

      case EIR_OPCODE_ASSERT: {
        MAP(ins.assertion.cond);
        break;
      }

      case EIR_OPCODE_LOADK: MAP(ins.loadk.dst); break;
      // case EIR_OPCODE_LOADK: break;
      case EIR_OPCODE_ADD:
      case EIR_OPCODE_SUB:
      case EIR_OPCODE_MUL:
      case EIR_OPCODE_DIV:
      case EIR_OPCODE_MOD:
      case EIR_OPCODE_EXP:
      case EIR_OPCODE_AND:
      case EIR_OPCODE_OR:
      case EIR_OPCODE_BAND:
      case EIR_OPCODE_BOR:
      case EIR_OPCODE_XOR:
      case EIR_OPCODE_EQL:
      case EIR_OPCODE_NEQ:
      case EIR_OPCODE_LT:
      case EIR_OPCODE_LTE:
      case EIR_OPCODE_GT:
      case EIR_OPCODE_GTE:
        MAP(ins.binop.dst);
        MAP(ins.binop.a);
        MAP(ins.binop.b);
        break;
      case EIR_OPCODE_NOT:
      case EIR_OPCODE_NEG:
      case EIR_OPCODE_BNOT:
      case EIR_OPCODE_INC:
      case EIR_OPCODE_DEC:
        MAP(ins.unop.dst);
        MAP(ins.unop.a);
        break;
      case EIR_OPCODE_CALL: MAP(ins.call.dst); break;
      case EIR_OPCODE_INDEX:
        MAP(ins.index.dst);
        MAP(ins.index.base);
        MAP(ins.index.index);
        break;
      case EIR_OPCODE_INDEX_ASSIGN:
        MAP(ins.index_assign.value);
        MAP(ins.index_assign.base);
        MAP(ins.index_assign.index);
        break;
      case EIR_OPCODE_MEMBER_ACCESS:
        MAP(ins.member_access.dst);
        MAP(ins.member_access.base);
        break;
      case EIR_OPCODE_MEMBER_ASSIGN:
        MAP(ins.member_assign.value);
        MAP(ins.member_assign.base);
        break;
      case EIR_OPCODE_RET: MAP(ins.ret.return_value); break;
      case EIR_OPCODE_JZ:
      case EIR_OPCODE_JNZ: MAP(ins.cj.condition); break;
      case EIR_OPCODE_GETG: MAP(ins.getg.dst); break;
      case EIR_OPCODE_SETG: MAP(ins.setg.src); break;

      case EIR_OPCODE_MK_LIST: MAP(ins.mk_list.dst); break;
      case EIR_OPCODE_MK_MAP: MAP(ins.mk_map.dst); break;
      case EIR_OPCODE_MK_STRUCT:
        MAP(ins.mk_struct.dst);
        break;

        // default: break;

      case EIR_OPCODE_NOP:
      case EIR_OPCODE_MOVG:
      case EIR_OPCODE_LABEL:
      case EIR_OPCODE_JMP: break;
    }

    // write the patched instruction back to the same location
    cc->instructions[i] = ins;
  }
#undef MAP

  return 0;
}

static int
era_compute_ranges_from_cfg(kit_compiler* cc, era_state* ra)
{
  kit_arena cfg_arena;
  if (kit_arena_init(1, &cfg_arena) < 0) return -1;

  codegraph cfg = { 0 };
  if (codegraph_init(&cfg_arena, cc, &cfg) < 0) goto die;
  if (codegraph_build_successor_list(cc, &cfg) < 0) goto die;
  if (codegraph_liveliness_analysis(cc, &cfg) < 0) goto die;

  const u32 nvregs = cc->next_vreg;

  for (u32 b = 0; b < cfg.nblocks; b++) {
    codeblock* blk          = &cfg.blocks[b];
    u32        block_end_ip = blk->end;
    for (u32 r = 0; r < nvregs; r++) {
      if (blk->live_out[r] && ra->ranges[r].end < block_end_ip) { ra->ranges[r].end = block_end_ip; }
    }
  }

  for (i64 b = cfg.nblocks - 1; b >= 0; b--) {
    codeblock* blk = &cfg.blocks[b];

    bool* live = kit_arnalloc(&cfg_arena, nvregs * sizeof(bool));
    if (!live) goto die;

    memcpy(live, blk->live_out, nvregs * sizeof(bool));

    for (i64 ip = blk->end; ip >= (i64)blk->start; ip--) {
      kit_ins* ins = &cc->instructions[ip];

      u32 srcs[32];
      u32 nsrcs = get_source_registers(ins, srcs);
      for (u32 s = 0; s < nsrcs; s++) {
        u32 r = srcs[s];
        if (r < nvregs && !live[r]) {
          live[r] = true;
          if (ra->ranges[r].start > (u32)ip) ra->ranges[r].start = (u32)ip;
        }
      }

      u32 dst = get_destination_reg(ins);
      if (dst < nvregs) live[dst] = false;
    }

    kit_arnfree(&cfg_arena, live);
  }

  u32* label_map = kit_arnalloc(cfg.arena, cc->next_label * sizeof(u32));
  memset(label_map, 0xFF, cc->next_label * sizeof(u32));

  for (u32 i = 0; i < cc->ninstructions; i++) {
    kit_ins* ins = &cc->instructions[i];
    if (ins->opcode == EIR_OPCODE_LABEL) label_map[ins->label.id] = i;
  }

  /* If theres a loop, extend all registers to the end of the jump */
  for (u32 i = 0; i < cc->ninstructions; i++) {
    const kit_ins* ins = &cc->instructions[i];
    if (ins->opcode != EIR_OPCODE_JMP && ins->opcode != EIR_OPCODE_JZ && ins->opcode != EIR_OPCODE_JNZ) continue;

    u32 target = label_map[ins->opcode == EIR_OPCODE_JMP ? ins->jmp.target : ins->cj.target];
    if (i > target) {
      for (u32 r = 0; r < cc->next_vreg; r++) {
        if (ra->ranges[r].start != UINT32_MAX && ra->ranges[r].end >= target // currently reaches into the loop
            && ra->ranges[r].end < i) {                                      // but doesn't reach the back-edge yet
          ra->ranges[r].end = i + 1;
        }
      }
    }
  }

  kit_arena_free(&cfg_arena);
  return 0;

die:
  kit_arena_free(&cfg_arena);
  return -1;
}

int
era_register_allocation_pass(struct kit_compiler* cc)
{
  era_state state;
  int       e = era_compute_ranges(cc, &state);
  if (e < 0) return e;

  e = era_compute_ranges_from_cfg(cc, &state);
  if (e < 0) {
    free(state.ranges);
    free(state.vreg_to_phys);
    return e;
  }

  e = era_allocate(cc->info->opt_level, &state);
  if (e < 0) {
    free(state.ranges);
    free(state.vreg_to_phys);
    return e;
  }

  e = era_rewrite(cc, state.vreg_to_phys);
  free(state.ranges);
  free(state.vreg_to_phys);
  return e;
}

static inline const char*
lookup(const kit_compilation_result* r, u32 name)
{
  for (u32 i = 0; i < r->names_count; i++) {
    if (r->names_hashes[i] == name) { return r->names[i]; }
  }
  return "[symbol not found]";
}

static inline const char*
get_register_name(u32 reg_id, char buff[32])
{
  if (reg_id >= KIT_REG_ARG0 && reg_id < KIT_REG_ARG_COUNT) {
    snprintf(buff, 32, "arg%i", reg_id - KIT_REG_ARG0);
  } else if (reg_id >= KIT_REG_GENERAL_BEGIN && reg_id <= KIT_REG_GENERAL_END) {
    snprintf(buff, 32, "r%i", reg_id - KIT_REG_GENERAL_BEGIN);
  } else if (reg_id == KIT_REG_SP) {
    snprintf(buff, 32, "rsp");
  } else if (reg_id == KIT_REG_IP) {
    snprintf(buff, 32, "rip");
  } else if (reg_id == KIT_REG_NIL) {
    snprintf(buff, 32, "rnil");
  } else {
    snprintf(buff, 32, "??%i", reg_id);
  }
  return buff;
}

static inline void
kit_print_instruction(kit_ins i, const kit_compilation_result* r, FILE* f)
{
  char buf0[32];
  char buf1[32];
  char buf2[32];
  switch ((eir_opcode_bits)i.opcode) {
    case EIR_OPCODE_LOADK: {
      fprintf(f, "loadk dst=%s, id=%u\n", get_register_name(i.loadk.dst, buf0), i.loadk.id);
      break;
    }
    case EIR_OPCODE_ASSERT: {
      fprintf(f, "assert condition=%s, line_id=%u\n", get_register_name(i.assertion.cond, buf0), i.assertion.line_id);
      break;
    }
    case EIR_OPCODE_MOV: {
      fprintf(f, "mov dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1));
      break;
    }

    case EIR_OPCODE_MOVI: {
      fprintf(f, "movi dst=%s, value=%i\n", get_register_name(i.movi.dst, buf0), i.movi.value);
      break;
    }
    case EIR_OPCODE_MOVF: {
      fprintf(f, "movf dst=%s, value=%f\n", get_register_name(i.movf.dst, buf0), i.movf.value);
      break;
    }

    case EIR_OPCODE_ADD:
      fprintf(
          f,
          "add dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_SUB:
      fprintf(
          f,
          "sub dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MUL:
      fprintf(
          f,
          "mul dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_DIV:
      fprintf(
          f,
          "div dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_MOD:
      fprintf(
          f,
          "mod dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EXP:
      fprintf(
          f,
          "exp dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_AND:
      fprintf(
          f,
          "and dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_OR:
      fprintf(
          f, "or dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BAND:
      fprintf(
          f,
          "band dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_BOR:
      fprintf(
          f,
          "bor dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_XOR:
      fprintf(
          f,
          "xor dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_EQL:
      fprintf(
          f,
          "eql dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_NEQ:
      fprintf(
          f,
          "neq dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LT:
      fprintf(
          f, "lt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_LTE:
      fprintf(
          f,
          "lte dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GT:
      fprintf(
          f, "gt dst=%s, a=%s, b=%s\n", get_register_name(i.binop.dst, buf0), get_register_name(i.binop.a, buf1), get_register_name(i.binop.b, buf2));
      break;
    case EIR_OPCODE_GTE:
      fprintf(
          f,
          "gte dst=%s, a=%s, b=%s\n",
          get_register_name(i.binop.dst, buf0),
          get_register_name(i.binop.a, buf1),
          get_register_name(i.binop.b, buf2));
      break;

    case EIR_OPCODE_INC: fprintf(f, "inc dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_DEC: fprintf(f, "dec dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_BNOT: fprintf(f, "bnot dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NEG: fprintf(f, "neg dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;
    case EIR_OPCODE_NOT: fprintf(f, "not dst=%s, src=%s\n", get_register_name(i.unop.dst, buf0), get_register_name(i.unop.a, buf1)); break;

    case EIR_OPCODE_RET: {
      fprintf(f, "ret val=%s\n", get_register_name(i.ret.return_value, buf0));
      break;
    }
    case EIR_OPCODE_NOP: fprintf(f, "nop\n"); break;
    case EIR_OPCODE_MK_LIST: fprintf(f, "mk_list dst=%s, nelems=%u\n", get_register_name(i.mk_list.dst, buf0), i.mk_list.nelems); break;
    case EIR_OPCODE_MK_MAP: fprintf(f, "mk_map dst=%s, npairs=%u\n", get_register_name(i.mk_map.dst, buf0), i.mk_map.npairs); break;
    case EIR_OPCODE_INDEX:
      fprintf(
          f,
          "index dst=%s, base=%s, index=%s\n",
          get_register_name(i.index.dst, buf0),
          get_register_name(i.index.base, buf1),
          get_register_name(i.index.index, buf2));
      break;
    case EIR_OPCODE_CALL:
      fprintf(
          f,
          "call dst=%s, id=%u|name=%s, nargs=%u\n",
          get_register_name(i.call.dst, buf0),
          i.call.function_id,
          lookup(r, i.call.function_id),
          i.call.nargs);
      break;
    case EIR_OPCODE_INDEX_ASSIGN:
      fprintf(
          f,
          "index_assign value=%s, base=%s, index=%s\n",
          get_register_name(i.index_assign.value, buf0),
          get_register_name(i.index_assign.base, buf1),
          get_register_name(i.index_assign.index, buf2));
      break;

    case EIR_OPCODE_LABEL: fprintf(f, "label id=%u\n", i.label.id); break;
    case EIR_OPCODE_JMP: fprintf(f, "jmp target=%u\n", i.jmp.target); break;
    case EIR_OPCODE_JZ: fprintf(f, "jz target=%u, condition=%s\n", i.jz.target, get_register_name(i.jz.condition, buf0)); break;
    case EIR_OPCODE_JNZ: fprintf(f, "jnz target=%u, condition=%s\n", i.jnz.target, get_register_name(i.jnz.condition, buf0)); break;
    case EIR_OPCODE_MEMBER_ACCESS:
      fprintf(
          f,
          "member_access dst=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_access.dst, buf0),
          get_register_name(i.member_access.base, buf1),
          i.member_access.member_id);
      break;
    case EIR_OPCODE_MEMBER_ASSIGN:
      fprintf(
          f,
          "member_assign value=%s, base=%s, member_id=%u\n",
          get_register_name(i.member_assign.value, buf0),
          get_register_name(i.member_assign.base, buf1),
          i.member_assign.member_id);
      break;
    case EIR_OPCODE_MK_STRUCT: fprintf(f, "mk_struct dst=%s, id=%u\n", get_register_name(i.mk_struct.dst, buf0), i.mk_struct.struct_id); break;
    case EIR_OPCODE_GETG: fprintf(f, "getg dst=%s, src=gv%u\n", get_register_name(i.mov.dst, buf0), i.mov.src); break;
    case EIR_OPCODE_SETG: fprintf(f, "setg dst=gv%u, src=%s\n", i.mov.dst, get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_MOVG: fprintf(f, "movg dst=%s, src=%s\n", get_register_name(i.mov.dst, buf0), get_register_name(i.mov.src, buf1)); break;
    case EIR_OPCODE_PUSH: fprintf(f, "push %s\n", get_register_name(i.push.reg, buf0)); break;
    case EIR_OPCODE_POP: fprintf(f, "pop %s\n", get_register_name(i.pop.reg, buf0)); break;
  }
}

static inline void
kit_print_instruction_stream(const kit_compilation_result* r, const kit_ins* ins, u32 nins, int indent, FILE* f)
{
  for (u32 ip = 0; ip < nins; ip++) {
    u32 instruction_offset = ip;

    for (int j = 0; j < indent; j++) fputc(' ', stdout);

    fprintf(f, "%-4u: ", instruction_offset); // Print offset of instruction
    kit_print_instruction(ins[ip], r, f);
  }
}

static void
dump_asm(const kit_compilation_result* r)
{
  kit_print_instruction_stream(r, r->instructions, r->instructions_count, 0, stdout);
  for (u32 i = 0; i < r->functions_count; i++) {
    const kitc_function* func = &r->functions[i];
    fprintf(stdout, "[%s|%u](%u):\n", lookup(r, func->name_hash), func->name_hash, func->nargs);
    kit_print_instruction_stream(r, func->code, func->code_count, 4, stdout);
  }

  fprintf(stdout, "\nstructures:\n");
  for (u32 i = 0; i < r->structs_count; i++) {
    const kitc_struct_information* info = &r->structs[i];
    fprintf(stdout, "[%s] = {", info->name);
    for (u32 j = 0; j < info->fields_count; j++) {
      fprintf(stdout, "%s/%u", info->field_names[j], info->field_hashes[j]);
      if (j != info->fields_count - 1) { fprintf(stdout, ", "); }
    }
    fprintf(stdout, "},\n");
  }
  fprintf(stdout, "\n");

  fprintf(stdout, "literals:\n");
  for (u32 i = 0; i < r->literals_count; i++) {
    fprintf(stdout, "[%u | %u] = ", i, kit_var_hash(&r->literals[i]));
    kit_var_print(&r->literals[i], stdout);
    fputc('\n', stdout);
  }
}

int
kit_compile(const kitc_info* info, kit_compilation_result* result)
{
  int e = 0;

  kitc_namespace_stack         namespace_stack   = { 0 };
  kitc_literal_table           lit_table         = { 0 };
  kitc_builtin_variables_table builtin_var_table = { 0 };
  kitc_function_table          func_table        = { 0 };
  kitc_struct_table            struct_table      = { 0 };
  kit_compiler                 cc                = { 0 };

  namespace_stack = (kitc_namespace_stack){
    .namespaces  = (char**)kit_xalloc(init_namespaces_capacity, sizeof(char*)),
    .nnamespaces = 0,
    .capacity    = init_namespaces_capacity,
  };
  if (!namespace_stack.namespaces) goto RET;

  /**
   * Compiler's builtin variables and the hooked variables combined.
   */
  u32 total_builtin_variable_count = KIT_ARRLEN(kit_builtins_vars) + info->nhooked_vars;

  u32*             builtin_variable_hashes = (u32*)kit_arnalloc(info->arena, sizeof(u32) * total_builtin_variable_count);
  kit_builtin_var* builtin_variables       = (kit_builtin_var*)kit_arnalloc(info->arena, sizeof(kit_builtin_var) * total_builtin_variable_count);
  if (!builtin_variable_hashes) goto RET;
  if (!builtin_variables) goto RET;

  /**
   * Load up every hash and variable into the arrays.
   * First, the compiler's builtins, and then the hooked variables.
   * This ensures the compiler's definitions are seen earlier
   * than the later ones, preventing overshadowing of primitive types.
   */
  u32 builtin_var_ctr = 0;
  for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_vars); i++, builtin_var_ctr++) {
    builtin_variable_hashes[builtin_var_ctr] = kit_hash(kit_builtins_vars[i].name, strlen(kit_builtins_vars[i].name));
    memcpy(&builtin_variables[builtin_var_ctr], &kit_builtins_vars[i], sizeof(kit_builtin_var));
  }
  for (u32 i = 0; i < info->nhooked_vars; i++, builtin_var_ctr++) {
    builtin_variable_hashes[builtin_var_ctr] = kit_hash(info->hook_vars[i].name, strlen(info->hook_vars[i].name));
    memcpy(&builtin_variables[builtin_var_ctr], &info->hook_vars[i], sizeof(kit_builtin_var));
  }

  lit_table = (kitc_literal_table){
    .literals          = (kit_var*)kit_xalloc(init_literal_capacity, sizeof(kit_var)),
    .literal_hashes    = (u32*)kit_xalloc(init_literal_capacity, sizeof(u32)),
    .literals_count    = 0,
    .literals_capacity = init_literal_capacity,
  };
  if (!lit_table.literals || !lit_table.literal_hashes) {
    e = -1;
    goto RET;
  }

  builtin_var_table = (kitc_builtin_variables_table){
    .builtin_vars       = builtin_variables,
    .builtin_var_hashes = builtin_variable_hashes,
    .builtin_vars_count = total_builtin_variable_count,
  };

  func_table = (kitc_function_table){
    .functions          = kit_xalloc(init_function_capacity, sizeof(kitc_function)),
    .functions_capacity = init_function_capacity,
    .functions_count    = 0,
  };
  if (!func_table.functions) {
    e = -1;
    goto RET;
  }

  struct_table = (kitc_struct_table){
    .structs_count    = 0,
    .structs_capacity = init_structs_capacity,
    .structs          = kit_xalloc(init_structs_capacity, sizeof(kitc_struct_information)),
  };

  cc = (kit_compiler){
    .arena             = info->arena,
    .ast               = info->ast,
    .info              = info,
    .loop              = NULL,
    .ns                = &namespace_stack,
    .lit_table         = &lit_table,
    .builtin_var_table = &builtin_var_table,
    .function_table    = &func_table,
    .struct_table      = &struct_table,
    .next_vreg         = KIT_REG_GENERAL_BEGIN,
    .next_global       = 0,
    .instructions      = (kit_ins*)kit_xalloc(sizeof(kit_ins), init_code_capacity),
    .ninstructions     = 0,
    .cinstructions     = init_code_capacity,
  };
  if (!cc.instructions) return -1;
  for (u32 i = 0; i < init_code_capacity; i++) { cc.instructions[i] = (kit_ins){ .opcode = EIR_OPCODE_NOP }; }

  scope_push(&cc);

  e = defer_push_scope(&cc);
  if (e < 0) goto RET;

  /**
   * Generate constructors for all builtin && hooked
   * structures.
   */
  e = compile_builtin_structures(&cc);
  if (e < 0) goto RET;

  e = compile(&cc, info->root_node);
  if (e < 0) goto RET;

  defer_pop_scope(&cc);

  if (!cc.info->feature_set.disable_register_allocation_i_know_what_im_doing) era_register_allocation_pass(&cc);
  cc.ninstructions = label_pass(cc.arena, cc.instructions, cc.ninstructions, cc.next_label);

  for (u32 i = 0; i < func_table.functions_count; i++) {
    kitc_function* f = &func_table.functions[i];

    u32      ninstructions = cc.ninstructions;
    kit_ins* instructions  = cc.instructions;
    u32      next_label    = cc.next_label;
    u32      next_vreg     = cc.next_vreg;

    cc.ninstructions = f->code_count;
    cc.instructions  = f->code;
    cc.next_label    = f->labels_used;
    cc.next_vreg     = (kit_vreg_t)f->vregs_used;

    if (!cc.info->feature_set.disable_register_allocation_i_know_what_im_doing) era_register_allocation_pass(&cc);
    f->code_count = label_pass(cc.arena, f->code, f->code_count, f->labels_used);

    cc.ninstructions = ninstructions;
    cc.instructions  = instructions;
    cc.next_label    = next_label;
    cc.next_vreg     = (kit_vreg_t)next_vreg;
  }

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

    result->names_hashes = (u32*)kit_xalloc(strings_count, sizeof(u32));
    if (!result->names_hashes) {
      e = -1;
      goto RET;
    }

    result->names = (char**)kit_xalloc(strings_count, sizeof(char*));
    if (!result->names) {
      e = -1;
      goto RET;
    }

    for (u32 i = 0; i < strings_count; i++) {
      result->names[i] = kit_strdup(strings[i]);
      if (!result->names[i]) {
        e = -1;
        goto RET;
      }
    }
  }

  /* everything done. dump the assembly and return. */
  if (info->dump_assembly) { dump_asm(result); }

  kit_xfree((void**)&namespace_stack.namespaces);
  // scope_pop(&cc);

  return e;

RET: /* Seperate from successful return path. We free everything here, indiscriminately. */
  while (cc.defer_stack) defer_pop_scope(&cc);
  for (u32 i = 0; i < struct_table.structs_count; i++) {
    free((void*)struct_table.structs[i].field_names);
    free(struct_table.structs[i].field_hashes);

    memset(&struct_table.structs[i], 0, sizeof(kitc_struct_information));
  }
  free(struct_table.structs);
  memset(&struct_table, 0, sizeof(struct_table));

  free((void*)namespace_stack.namespaces);
  memset(&namespace_stack, 0, sizeof(namespace_stack));

  free(lit_table.literal_hashes);
  free(lit_table.literals);
  memset(&lit_table, 0, sizeof(lit_table));

  for (u32 i = 0; i < func_table.functions_count; i++) {
    // arg_slots is arena allocated
    free(func_table.functions[i].code);
  }
  free(func_table.functions);
  memset(&func_table, 0, sizeof(func_table));

  free(cc.instructions);
  return e ? e : -1;
}