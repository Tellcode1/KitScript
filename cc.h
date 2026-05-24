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

#ifndef E_CC_H
#define E_CC_H

#include "arena.h"
#include "bstructs.h"
#include "bvar.h"
#include "cerr.h"
#include "stdafx.h"
#include "var.h"

#define E_OBJ_AS_INFO(obj) ((ecc_variable_information*)((obj)->data))
#define E_VAR_AS_INFO(var) ((ecc_variable_information*)((var)->val.s->data))

#define E_OBJ_AS_STRUCT_INFO(obj) ((ecc_struct_information*)((obj)->data))
#define E_VAR_AS_STRUCT_INFO(var) ((ecc_struct_information*)((var)->val.s->data))

struct e_ast;

typedef struct ecc_info {
  e_arena* arena; // Must not be NULL

  struct e_ast* ast; // Must not be NULL

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
  const e_builtin_struct* hook_structs;
  u32                     nhooked_structs;

  /**
   * Same deal as structures.
   * NOT checked for duplicates.
   */
  const e_builtin_var* hook_vars;
  u32                  nhooked_vars;

  int opt_level; // 0 or 1/2/3
} ecc_info;

/**
 * Loop location structure.
 * Filled in before while and for loops are compiled
 * so break and continue statements within them know
 * where to go.
 *
 * Both "label"s are the label IDs, generated during
 * compile time.
 * The defer_depth is the depth in the ecc_defer_scope
 * struct, used to determine which defer scopes to clear
 * on break/continue and return statements inside loops.
 */
typedef struct ecc_loop_location {
  u32 continue_label; // Where continue goes
  u32 break_label;    // Where break goes
  u32 defer_depth;
} ecc_loop_location;

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
typedef struct ecc_namespace_stack {
  char** namespaces;
  u32    nnamespaces;
  u32    capacity;
} ecc_namespace_stack;

typedef struct ecc_variable_information {
  e_filespan span; // Span at where the variable (name) is.
  u32        name_hash;
  int        initializer;   // initializer provided during creation.
  int        current_value; // <0 if no value currently (void)
  bool       is_const;
} ecc_variable_information;

/**
 * Data deposit for a structure.
 */
typedef struct ecc_struct_information {
  char** field_names; // Array allocated. Strings arena allocated.
  u32*   field_hashes;
  u32    fields_count;
  u32    field_capacity;
  char*  name; // Arena allocated.
} ecc_struct_information;

/**
 * Stored information about all structures.
 * Filled in as the compiler runs.
 */
typedef struct ecc_struct_table {
  ecc_struct_information* structs;
  u32                     structs_count;
  u32                     structs_capacity;
} ecc_struct_table;

/**
 * Stored information about all literal variables
 * Filled in as the compiler runs.
 *
 * Serialized, and used by the executor to reduce
 * memory usage.
 */
typedef struct ecc_literal_table {
  e_var* literals;
  u32*   literal_hashes;
  u32    literals_count;
  u32    literals_capacity;
} ecc_literal_table;

/**
 * A constant list (upfront, before compilation starts) of
 * all the builtin variables, provided by the e_compile caller.
 * Shared across all forks of a compiler.
 */
typedef struct ecc_builtin_variables_table {
  const e_builtin_var* builtin_vars;
  const u32*           builtin_var_hashes;
  u32                  builtin_vars_count;
} ecc_builtin_variables_table;

/**
 * Compiled function structure.
 */
typedef struct ecc_function {
  u32  name_hash;
  u32  nargs;
  u32  code_size;
  u8*  code;
  u32* arg_slots; /* The ID of the arguments, in order. */
} ecc_function;

/**
 * All functions in the resulting binary
 * Shared across all forks of a compiler.
 */
typedef struct ecc_function_table {
  ecc_function* functions;
  u32           functions_count;
  u32           functions_capacity;
} ecc_function_table;

/**
 * A defer statement.
 * eg. defer io::close(fd); io::close(fd) is our expression
 */
typedef struct ecc_defer_entry {
  u32  nexprs; // Can be 0!
  u32  capacity;
  int* exprs;
} ecc_defer_entry;

/**
 * All the defer statements in a scope.
 * Instantiated on function return / scope break.
 */
typedef struct ecc_defer_scope {
  struct ecc_defer_scope* parent;
  ecc_defer_entry*        entries;
  u32                     count;
  u32                     capacity;
} ecc_defer_scope;

/**
 * Data a label stores about all of its
 * jumps. Used in e_resolve_labels to quickly
 * Go through and patch up all labels
 * (without traversing the entire stream)!.
 */
typedef struct ecc_label_jumps_table {
  bool defined;

  u32 label_id;

  // The actual offset that all instructions
  // will need to JMP to
  // Filled in later if the jumps precede the label.
  // Safely accessed only after compilation is finished.
  u32 label_offset;

  u32 jumps_count;
  u32 jumps_capacity;

  // Offsets in the instruction stream
  u32* jumps_target_offsets;
} ecc_label_jumps_table;

/**
 * All labels in the stream, and their child jmps.
 *
 *                LABEL0
 *    --------------|-----------------
 *    |             |                |
 *    |             |                |
 *  JMP(Off=3)     JZ(Off=8)        JNZ(Off=16)
 *
 * e_resolve_labels will look at this structure,
 * open up the stream and then:
 *
 * Instructions[3].target = LABEL;
 *
 * Instructions[8].target = LABEL
 *
 * Instructions[16].target = LABEL
 *
 * Needless to say, reduces the work of e_resolve_labels
 * by a LOT! The old algorithm I used was O(n^2)
 * (Walked through the entire list, and for each label,
 *  walked through the list AGAIN, patching up all jumps
 *  one by one as it walks through).
 */
typedef struct ecc_label_table {
  u32                    labels_count;
  u32                    labels_capacity; // Can NOT be 0
  ecc_label_jumps_table* labels;
} ecc_label_table;

typedef struct ecc_frame_table {
  u32* stack;
  u32  capacity;
  u32  top;
} ecc_frame_table;

typedef struct e_compiler {
  const ecc_info* info;

  e_arena*      arena;
  struct e_ast* ast;

  ecc_label_table*             label_table;
  ecc_literal_table*           lit_table;
  ecc_builtin_variables_table* builtin_var_table;
  ecc_function_table*          function_table;
  ecc_defer_scope*             defer_stack;
  ecc_struct_table*            struct_table;

  ecc_loop_location*   loop;
  ecc_namespace_stack* ns;

  /* Stack for storing information about variables during compilation. */
  struct e_stackemu* stack;

  u8* emit;
  u32 num_bytes_emitted;
  u32 emit_capacity;

  u32 next_label;
} e_compiler;

typedef struct e_compilation_result {
  u32 literals_count;
  u32 functions_count;
  u32 instructions_count;
  u32 names_count;
  u32 structs_count;

  e_var*                  literals;        // Array allocated by struct, don't free inviduals.
  u32*                    literals_hashes; // Array allocated by struct. Free after use.
  ecc_function*           functions;       // Array allocated by struct. Free after use.
  u8*                     instructions;    // Array allocated by struct. Free after use.
  ecc_struct_information* structs;         // Array (and arrays inside) allocated by struct. Free after use.

  /* Debug symbols. Optional. */
  char** names; // Array && individuals allocated.
  u32*   names_hashes;
} e_compilation_result;

/**
 * Uses ge_pool, Initialize it using e_refdobj_pool_init().
 * Free later with e_refdobj_pool_free();
 */
int e_compile(const ecc_info* info, e_compilation_result* result);

static inline void
ecc_stream_resize(e_compiler* cc, int new_cap)
{
  if (cc == nullptr || new_cap == 0) return;

  u8* newcode = (u8*)realloc(cc->emit, sizeof(u8) * new_cap);
  if (newcode == nullptr) { return; }

  cc->emit          = newcode;
  cc->emit_capacity = new_cap;
}

static inline void
e_compilation_result_free(e_compilation_result* r)
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

#endif // E_CC_H