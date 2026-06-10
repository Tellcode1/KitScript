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

#ifndef KIT_CC_H
#define KIT_CC_H

#include "kit.arena.h"
#include "kit.bstructs.h"
#include "kit.bvar.h"
#include "kit.cerr.h"
#include "kit.ir.h"
#include "kit.stdafx.h"
#include "kit.var.h"

#define KIT_OBJ_AS_INFO(obj) ((kitc_variable_information*)((obj)->data))
#define KIT_VAR_AS_INFO(var) ((kitc_variable_information*)((var)->val.s->data))

#define KIT_OBJ_AS_STRUCT_INFO(obj) ((kitc_struct_information*)((obj)->data))
#define KIT_VAR_AS_STRUCT_INFO(var) ((kitc_struct_information*)((var)->val.s->data))

struct kit_ast;

typedef struct kitc_feature_set {
  bool disable_noop_stripping;
  bool disable_dead_branch_elimination;
  bool disable_constant_propagation;
  bool disable_constant_folding;
  bool disable_local_copy_propagation;
  bool disable_redundant_move_elimination;
  bool disable_function_inlining;
  bool disable_dead_move_forwarding;
  bool disable_dead_store_elimination;
  bool disable_redundant_jump_elimination;

  /**
   * Disable register allocation, leaving the virtual registers as is.
   * Your code WILL crash if it contains more than 256 virtual registers,
   * which is not a large limit!
   * This switch is provided for debugging purposes, particularly
   * where register allocations breaks working code.
   *
   * Register allocation is not very expensive, and is expected
   * to be on.
   * You have been warned.
   */
  bool disable_register_allocation_i_know_what_im_doing;
} kitc_feature_set;

typedef struct kitc_info {
  kit_arena* arena; // Must not be NULL

  struct kit_ast* ast; // Must not be NULL

  /**
   * Must be a valid node.
   * Need node have the ROOT type, anything works.
   */
  int root_node;

  // If NULL, main is used.
  const char* custom_entry_point;

  /**
   * Include additional structures defined during
   * compilation stage.
   * NOT checked for duplicates.
   * Must not collide with any other structures, if it does,
   * the order LOCAL > GLOBAL > BUILTIN is used.
   */
  const kit_builtin_struct* hook_structs;
  u32                       nhooked_structs;

  /**
   * Same deal as structures.
   * NOT checked for duplicates.
   */
  const kit_builtin_var* hook_vars;
  u32                    nhooked_vars;

  int opt_level; // 0 or 1/2/3

  /**
   * Dump the generated IR ("assembly") to stdout
   * (compiled object will be returned as usual!).
   */
  bool dump_assembly;

  kitc_feature_set feature_set;
} kitc_info;

/**
 * Loop location structure.
 * Filled in before while and for loops are compiled
 * so break and continue statements within them know
 * where to go.
 *
 * Both "label"s are the label IDs, generated during
 * compile time.
 * The defer_depth is the depth in the kitc_defer_scope
 * struct, used to determine which defer scopes to clear
 * on break/continue and return statements inside loops.
 */
typedef struct kitc_loop_location {
  u32 continue_label; // Where continue goes
  u32 break_label;    // Where break goes
  u32 defer_depth;
} kitc_loop_location;

/**
 * The namespace list, in order.
 * Used by the compiler to generate the correct absolute
 * name for variables.
 *
 * For instance, we have two namespaces, big and small.
 * small is within big.
 * So like: namespace big { namespace small { let guy; } }
 * The namespace stack will be ["big", "small"]
 * The compiler will then generate the absolute name:
 * "big::small::guy"
 * This identifier is then hashed, and the result is used
 * as if it were a normal variable ID.
 */
typedef struct kitc_namespace_stack {
  char** namespaces;
  u32    nnamespaces;
  u32    capacity;
} kitc_namespace_stack;

typedef struct kitc_variable_information {
  kit_filespan span; // Span at where the variable (name) is.
  int          reg;
  u32          name_hash;
  int          initializer;   // initializer provided during creation.
  int          current_value; // <0 if no value currently (void)
  u32          times_loaded;  // How many times this variable has been loaded.
  bool         is_const;
} kitc_variable_information;

/**
 * Data deposit for a structure.
 */
typedef struct kitc_struct_information {
  char** field_names; // Array allocated. Strings arena allocated.
  u32*   field_hashes;
  u32    fields_count;
  u32    field_capacity;
  u32    name_hash;
  char*  name; // Arena allocated.
} kitc_struct_information;

/**
 * Stored information about all structures.
 * Filled in as the compiler runs.
 */
typedef struct kitc_struct_table {
  kitc_struct_information* structs;
  u32                      structs_count;
  u32                      structs_capacity;
} kitc_struct_table;

/**
 * Stored information about all literal variables
 * Filled in as the compiler runs.
 *
 * Serialized, and used by the executor to reduce
 * memory usage.
 */
typedef struct kitc_literal_table {
  kit_var* literals;
  u32*     literal_hashes;
  u32      literals_count;
  u32      literals_capacity;
} kitc_literal_table;

/**
 * A constant list (upfront, before compilation starts) of
 * all the builtin variables, provided by the kit_compile caller.
 * Shared across all forks of a compiler.
 */
typedef struct kitc_builtin_variables_table {
  const kit_builtin_var* builtin_vars;
  const u32*             builtin_var_hashes;
  u32                    builtin_vars_count;
} kitc_builtin_variables_table;

/**
 * Compiled function structure.
 */
typedef struct kitc_function {
  u32      name_hash;
  u32      nargs;
  u32      code_count;
  u32      vregs_used;
  u32      labels_used;
  kit_ins* code;
} kitc_function;

/**
 * All functions in the resulting binary
 * Shared across all forks of a compiler.
 */
typedef struct kitc_function_table {
  kitc_function* functions;
  u32            functions_count;
  u32            functions_capacity;
} kitc_function_table;

/**
 * A defer statement.
 * eg. defer io::close(fd); io::close(fd) is our expression
 */
typedef struct kitc_defer_entry {
  u32  nexprs; // Can be 0!
  u32  capacity;
  int* exprs;
} kitc_defer_entry;

/**
 * All the defer statements in a scope.
 * Instantiated on function return / scope break.
 */
typedef struct kitc_defer_scope {
  struct kitc_defer_scope* parent;
  kitc_defer_entry*        entries;
  u32                      count;
  u32                      capacity;
} kitc_defer_scope;

typedef struct kitc_var {
  kit_filespan span;
  const char*  name;
  u32          name_hash;
  union {
    kit_vreg_t reg;
    u32        global_id;
  } slot;
  bool             is_const;
  bool             is_global;
  struct kitc_var* next;
} kitc_var;

typedef struct kitc_scope {
  kitc_var*          vars;
  struct kitc_scope* parent;
} kitc_scope;

typedef struct kit_compiler {
  const kitc_info* info;

  kit_arena*      arena;
  struct kit_ast* ast;

  kitc_literal_table*           lit_table;
  kitc_builtin_variables_table* builtin_var_table;
  kitc_function_table*          function_table;
  kitc_defer_scope*             defer_stack;
  kitc_struct_table*            struct_table;

  kitc_scope*           scope;
  kitc_loop_location*   loop;
  kitc_namespace_stack* ns;

  /* Stack for storing information about variables during compilation. */
  struct kit_stackemu* stack;

  kit_ins* instructions;
  u32      ninstructions;
  u32      cinstructions;

  u32        next_label;
  u32        next_global;
  kit_vreg_t next_vreg;
} kit_compiler;

typedef struct kit_compilation_result {
  u32 literals_count;
  u32 functions_count;
  u32 instructions_count;
  u32 names_count;
  u32 structs_count;

  kit_var*                 literals;        // Array allocated by struct, don't free inviduals.
  u32*                     literals_hashes; // Array allocated by struct. Free after use.
  kitc_function*           functions;       // Array allocated by struct. Free after use.
  kit_ins*                 instructions;    // Array allocated by struct. Free after use.
  kitc_struct_information* structs;         // Array (and arrays inside) allocated by struct. Free after use.

  /* Debug symbols. Optional. */
  char** names; // Array && individuals allocated.
  u32*   names_hashes;
} kit_compilation_result;

/**
 * Uses kit_g_obj_pool, Initialize it using kit_refdobj_pool_init().
 * Free later with kit_refdobj_pool_free();
 */
int kit_compile(const kitc_info* info, kit_compilation_result* result);

static inline void
kitc_stream_resize(kit_compiler* cc, u32 new_cap)
{
  if (cc == NULL || new_cap == 0) return;

  kit_ins* newcode = (kit_ins*)realloc(cc->instructions, sizeof(kit_ins) * new_cap);
  if (newcode == NULL) { return; }

  for (u32 i = cc->cinstructions; i < new_cap; i++) { newcode[i] = (kit_ins){ .opcode = EIR_OPCODE_NOP }; }

  cc->instructions  = newcode;
  cc->cinstructions = new_cap;
}

static inline void
kit_compilation_result_free(kit_compilation_result* r)
{
  for (u32 i = 0; i < r->functions_count; i++) { free(r->functions[i].code); }
  for (u32 i = 0; i < r->names_count; i++) { free(r->names[i]); }
  for (u32 i = 0; i < r->structs_count; i++) {
    free(r->structs[i].field_hashes);
    free((void*)r->structs[i].field_names);
  }
  free((void*)r->names);
  free((void*)r->names_hashes);
  free(r->literals);
  free(r->literals_hashes);
  free(r->functions);
  free(r->structs);
  free(r->instructions);
}

#endif // KIT_CC_H