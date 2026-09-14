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

#include "../../inc/kit.cc.h"

#include "../../inc/kit.arena.h"
#include "../../inc/kit.ast.h"
#include "../../inc/kit.bstructs.h"
#include "../../inc/kit.bvar.h"
#include "../../inc/kit.cerr.h"
#include "../../inc/kit.ir.h"
#include "../../inc/kit.pool.h"
#include "../../inc/kit.reg.h"
#include "../../inc/kit.regalloc.h"
#include "../../inc/kit.rwhelp.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.strint.h"
#include "../../inc/kit.var.h"
#include "compile_routines.h"
#include "constants.h"
#include "defer.h"
#include "dump.asm.h"
#include "scope.h"
#include "tables.h"
#include "vreg.h"

#include <assert.h>
#include <error.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct val_t;

static u32 label_pass(kit_arena* arena, kit_ins* instructions, u32 ninstructions, u32 label_count);

/**
 * Add the jump to the label's stream.
 * opcode is needed because there are multiple
 * jump instructions (JMP,JE,JNE,JZ,JNZ,etc.)
 */
int
emit_and_record_jmp(kit_compiler* cc, kit_ir_opcode opcode, kit_vreg_t condition, u32 label_id)
{
  switch (opcode) {
    case KIT_IR_OPCODE_JMP: kit_emit_ins(cc, (kit_ins){ .jmp = { .opcode = KIT_IR_OPCODE_JMP, .target = label_id } }); break;
    case KIT_IR_OPCODE_JZ: kit_emit_ins(cc, (kit_ins){ .jz = { .opcode = KIT_IR_OPCODE_JZ, .condition = condition, .target = label_id } }); break;
    case KIT_IR_OPCODE_JNZ: kit_emit_ins(cc, (kit_ins){ .jnz = { .opcode = KIT_IR_OPCODE_JNZ, .condition = condition, .target = label_id } }); break;

    default: return -1;
  }

  return 0;
}

RETURNS_ERRCODE int
compiler_make_fork(const kit_compiler* old_c, kit_compiler* new_c)
{
  *new_c = (kit_compiler){
    .arena             = old_c->arena,
    .ast               = old_c->ast,
    .info              = old_c->info,
    .loop              = old_c->loop,
    .lit_table         = old_c->lit_table,
    .builtin_var_table = old_c->builtin_var_table,
    .function_table    = old_c->function_table,
    .struct_table      = old_c->struct_table,
    .next_label        = old_c->next_label,
    .next_global       = old_c->next_global,
    .next_vreg         = KIT_REG_GENERAL_BEGIN, // Seperate register for each function
    .scope             = old_c->scope,
    .ns                = old_c->ns,
    .stack             = old_c->stack,
    .instructions      = (kit_ins*)kit_xalloc(sizeof(kit_ins), init_code_capacity),
    .ninstructions     = 0,
    .cinstructions     = init_code_capacity,
  };
  for (u32 i = 0; i < init_code_capacity; i++) { new_c->instructions[i] = (kit_ins){ .opcode = KIT_IR_OPCODE_NOP }; }
  scope_push(new_c);
  return new_c->instructions ? 0 : -1;
}

void
compiler_join_fork(kit_compiler* copy, kit_compiler* cc)
{
  /* The tables are stored on the main compile function stack. Their address SHOULD NOT change. */
  if (cc->lit_table != copy->lit_table || cc->builtin_var_table != copy->builtin_var_table || cc->function_table != copy->function_table
      || cc->ns != copy->ns) {
    puts("Compiler structure corrupted");
    // abort(); How remove this ?
  }

  scope_pop(copy);

  /* Ensure we don't ever get two labels in different streams with the same ID */
  cc->next_label  = copy->next_label;
  cc->next_global = copy->next_global;

  /* Can't modify builtin variable count */
}

/**
 * for error paths. Free everything owned by this fork.
 */
void
compiler_free_fork_entirely(kit_compiler* cc)
{
  free(cc->instructions);
  while (cc->defer_stack) defer_pop_scope(cc);
  memset(cc, 0, sizeof *cc);
}

kit_vreg_t
compile_function_call(kit_compiler* cc, int node)
{
  // kit_filespan function_span = KIT_GET_NODE(cc->ast, node)->common.span;
  int  func_node = KIT_GET_NODE(cc->ast, node)->call.func;
  u32  nargs     = KIT_GET_NODE(cc->ast, node)->call.nargs;
  int* args      = KIT_GET_NODE(cc->ast, node)->call.args;

  int e = 0;

  kit_vreg_t compiled_function = compile(cc, func_node);
  if (compiled_function < 0) {
    cerror(KIT_GET_NODE(cc->ast, func_node)->common.span, "Failed to compile function LHS [function call]\n");
    return compiled_function;
  }

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
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = KIT_REG_ARG0 + i, .src = arg_registers[i] } });
    } else {
      /* Spill the rest of the arguments on the stack */
      kit_emit_ins(cc, (kit_ins){ .push = { .opcode = KIT_IR_OPCODE_PUSH, .reg = arg_registers[i] } });
    }
  }

  kit_vreg_t dst = vreg_alloc(cc);
  kit_emit_ins(cc, (kit_ins){ .call = { .opcode = KIT_IR_OPCODE_CALL, .dst = dst, .reg = compiled_function, .nargs = nargs } });

  /* No need to cleanup spilled arguments from stack, the called function picks up the arguments from the stack */

  return dst;
}

kit_vreg_t
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
  kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = KIT_IR_OPCODE_RET, .return_value = nilvar } });

  // if (!cc->info->feature_set.disable_register_allocation_i_know_what_im_doing) era_register_allocation_pass(cc);
  // cc->ninstructions = label_pass(cc->arena, cc->instructions, cc->ninstructions, cc->next_label);

  return 0; // Done!
}

kit_vreg_t
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

kit_vreg_t
compile_function(kit_compiler* cc, int node)
{
  u32  nstmts = KIT_GET_NODE(cc->ast, node)->func.nstmts;
  int* stmts  = KIT_GET_NODE(cc->ast, node)->func.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    if (compile(cc, stmts[i]) < 0) {
      cerror(KIT_GET_NODE(cc->ast, stmts[i])->common.span, "Failed to compile statement [function]\n");
      return -1;
    }

    /**
     * OPTIMIZATION: If we're in the function stream,
     * and a node pushes a value to the stack that we do not want,
     * pop it.
     */
    // if (cc->info->opt_level >= 1) pop_value_if_pushes(cc, stmts[i]);
  }

  kit_emit_ins(cc, (kit_ins){ .ret = { .opcode = KIT_IR_OPCODE_RET, .return_value = KIT_REG_NIL } });
  return 0;
}

kit_vreg_t
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

RETURNS_ERRCODE kit_vreg_t
compile_struct_fill(kit_compiler* cc, int node)
{
  kit_vreg_t dst = vreg_alloc(cc);

  /* lookup which structure we're filling */
  const char* struct_name = KIT_GET_NODE(cc->ast, node)->struct_fill.struct_name;
  u32         struct_hash = kit_hash(struct_name, strlen(struct_name));

  int* member_names    = KIT_GET_NODE(cc->ast, node)->struct_fill.members;
  int* assigned_values = KIT_GET_NODE(cc->ast, node)->struct_fill.assigned_values;

  u32 nfilled_values = KIT_GET_NODE(cc->ast, node)->struct_fill.nmembers;

  kitc_struct_information* si = NULL;

  for (u32 i = 0; i < cc->struct_table->structs_count; i++) {
    if (cc->struct_table->structs[i].name_hash != struct_hash) continue;

    si = &cc->struct_table->structs[i];
    break;
  }

  if (si == NULL) {
    cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Structure %s undefined [Structure fill]\n", struct_name);
    return -1;
  }

  for (u32 i = 0; i < si->fields_count; i++) {
    const char* field = si->field_names[i];

    u32  idx   = 0;
    bool found = false;
    for (u32 j = 0; j < nfilled_values; j++) {
      const char* member_name = KIT_GET_NODE(cc->ast, member_names[j])->ident.ident;

      if (strcmp(member_name, field) == 0) {
        idx   = j;
        found = true;
        break;
      }
    }

    /* Aren't filling in the value at idx. Move null to it */
    if (!found) {
      /* mov arg+i, nil */
      /* +i and not +idx since i is the member we have to fill */
      if (i < KIT_REG_ARG_COUNT) {
        kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = KIT_REG_ARG0 + i, .src = KIT_REG_NIL } });
      } else {
        kit_emit_ins(cc, (kit_ins){ .push = { .opcode = KIT_IR_OPCODE_PUSH, .reg = KIT_REG_NIL } });
      }
      continue;
    }

    /* We're filling in the value at idx */
    /* Compile and move to argument vector */
    kit_vreg_t compiled = compile(cc, assigned_values[idx]);

    if (i < KIT_REG_ARG_COUNT) {
      /* mov arg+i, compiled */
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = KIT_REG_ARG0 + i, .src = compiled } });
    } else {
      kit_emit_ins(cc, (kit_ins){ .push = { .opcode = KIT_IR_OPCODE_PUSH, .reg = compiled } });
    }
  }

  /* Call the constructor */

  /* Loadfn the struct constructor in a temp register and call it */
  kit_vreg_t tmp = vreg_alloc(cc);
  kit_emit_ins(cc, (kit_ins){ .loadfn = { .opcode = KIT_IR_OPCODE_LOADFN, .dst = tmp, .id = struct_hash } });
  kit_emit_ins(cc, (kit_ins){ .call = { .opcode = KIT_IR_OPCODE_CALL, .dst = dst, .nargs = si->fields_count, .reg = tmp } });

  return dst;
}

/**
 * Load all builtin structures, even if they
 * aren't used. We can not safely say a structure
 * is used or not before compilation.
 * (Compiling the structurs only when they are seems
 *  to work, but will cause issues later, like with
 *  dynamic script execution).
 */
kit_vreg_t
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

/* Register ID on success, <0 on error */
int
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
    case KIT_AST_NODE_STRUCT_FILL: return compile_struct_fill(cc, node);
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
      return emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, target);
    }
    case KIT_AST_NODE_CONTINUE: {
      if (!cc->loop) {
        kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
        cerror(span, "continue used outside a loop\n");
        return -1;
      }

      if (defer_emit_to_depth(cc, cc->loop->defer_depth) < 0) return -1;

      u32 target = cc->loop->continue_label;
      return emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, target);
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

      kit_emit_ins(cc, (kit_ins){ .assertion = { .opcode = KIT_IR_OPCODE_ASSERT, .cond = cond, .line_id = kit_var_hash(&line_str) } });

      return 0;
    }

    default: {
      kit_filespan span = KIT_GET_NODE(cc->ast, node)->common.span;
      cerror(span, "Unknown node type: %i\n", KIT_GET_NODE(cc->ast, node)->common.type);
      return -1;
    }
  }
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

    if (copy[i].opcode == KIT_IR_OPCODE_NOP) continue;

    if (ins->opcode == KIT_IR_OPCODE_LABEL) {
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
      case KIT_IR_OPCODE_JMP:
        // fprintf(stderr, "jmp %u vs %u\n", ins->jmp.target, label_count);
        ins->jmp.target = label_map[ins->jmp.target];
        break;
      case KIT_IR_OPCODE_JZ:
      case KIT_IR_OPCODE_JNZ:
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
  for (u32 i = 0; i < init_code_capacity; i++) { cc.instructions[i] = (kit_ins){ .opcode = KIT_IR_OPCODE_NOP }; }

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
      result->names_hashes[i] = kit_hash(strings[i], strlen(strings[i]));
    }
  }

  /* everything done. dump the assembly and return. */
  if (info->dump_assembly) { kit_dump_asm(result); }

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
