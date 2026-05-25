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

#ifndef E_OPTIMIZATION_PASSES_H
#define E_OPTIMIZATION_PASSES_H

#include "stdafx.h"
#include "var.h"

struct e_token;
struct e_ast;
struct e_compiler;

typedef enum eopt_stage {
  // Before anything happens to the source file.
  // Source file is directly given to the pass.
  EOPT_STAGE_SOURCE,
  // Immediately after lexer
  // Token list is given to the pass.
  EOPT_STAGE_LEXER,
  // After AST formation
  // Root node is given to the pass
  EOPT_STAGE_AST,
  // Called on a function definition node
  // before it is compiled
  // Function definition node is given to the pass.
  EOPT_STAGE_BEFORE_FUNCTION,
  // Called on a bytecode stream
  // after the compiler is done with that specific function
  // Function stream is given to the pass.
  EOPT_STAGE_FUNCTION,
  // Called after full compilation
  // Root bytecode stream + all functions are given to pass.
  EOPT_STAGE_BYTECODE_STREAM,
} eopt_stage;

typedef union eopt_data {
  char* source_file; // EOPT_STAGE_SOURCE

  struct {
    struct e_token* tokens;
    u32             ntokens;
  } lexed; // EOPT_STAGE_LEXER

  struct e_ast* ast; // EOPT_STAGE_AST, ast has root node tracked

  struct {
    struct e_ast* ast;           // EOPT_STAGE_AST
    int           func_def_node; // EOPT_STAGE_BEFORE_FUNCTION
  } func_def;

  struct {
    struct e_compiler* cc; // cc contains the bytecode stream. cc can be a fork.
  } bytecode_stream;       // EOPT_STAGE_FUNCTION | EOPT_STAGE_BYTECODE_STREAM
} eopt_data;

typedef struct eopt_var_info {
  const char* name;
  e_var       curr_value; // Current value if it could be parsed, E_VARTYPE_NULL if it isn't representible as a constant.
  u32         depth;      // Stack depth
  bool        used;
} eopt_var_info;

// Function that is called for optimization. Value in data can be modified.
typedef int (*eopt_pass_fn)(eopt_stage stage, eopt_data* data);

typedef struct eopt_pass {
  const char*  name;  // Name of pass
  eopt_stage   stage; // Stage at which it wants to be called
  int          minimum_opt_level;
  eopt_pass_fn fp; // Must not be NULL.
} eopt_pass;

/**
 * Constant fold expressions.
 * AKA. simplify constant expressions that can be evaluated
 * at compile time.
 *
 * Given a binary operator node, 1 + 2. This pass converts the entire leaf
 * into a single node with integer literal value 3.
 */
int eopt_constant_fold(eopt_stage stage, eopt_data* data);

/**
 * Remove unused variable decleration nodes.
 * This is quite expensive (have to recursively walk the AST),
 * and doesn't reduce stack usage by much (No one usually leaves unused variables).
 */
int eopt_unused_vars(eopt_stage stage, eopt_data* data);

/**
 * Call the builtin function at compile time, if possible
 * and place the evaluated output as a literal.
 * Very good performance gains, little expense.
 */
int eopt_eval_const_builtin(eopt_stage stage, eopt_data* data);

int eopt_var_track(struct e_ast* ast, int root);

static const eopt_pass eopt_passes[] = {
  { .name = "Constant Folding", .stage = EOPT_STAGE_AST, .minimum_opt_level = 1, .fp = eopt_constant_fold },
  // { .name = "Remove Unused Variables", .stage = EOPT_STAGE_AST, .minimum_opt_level = 2, .fp = eopt_unused_vars },
  // { .name = "Evaluate Constant Builtin Functions", .stage = EOPT_STAGE_AST, .minimum_opt_level = 1, .fp = eopt_eval_const_builtin },
};

#endif // E_OPTIMIZATION_PASSES_H