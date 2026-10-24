#include "../../inc/kit.bfunc.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.rwhelp.h"
#include "codegraph/codegraph.h"
#include "compile_routines.h"
#include "compiler.tree.h"
#include "defer.h"
#include "peephole.optimizers.h"
#include "scope.h"
#include "tables.h"

kit_vreg_t
compile_function_definition(kit_compiler* cc, int node)
{
  kit_compiler fork = { 0 };
  // kit_filespan function_span = KIT_GET_NODE(cc->ast, node)->func.span;
  const char* function_name = KIT_GET_NODE(cc->ast, node)->func.name;
  char*       full          = qualify_name(cc, function_name);

  const u32 function_hash = kit_hash(full, strlen(full));

  int e = 0;

  /* Ensure it doesn't already exist */
  const kitc_function_table* func_table = cc->function_table;
  for (u32 i = 0; i < func_table->functions_count; i++) {
    kitc_function* func = &func_table->functions[i];
    if (func->name_hash == function_hash) {
      cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Function \"%s\" redefined [function definition]\n", function_name);
      e = -1;
      goto ERR;
    }
  }

  /* Ensure we aren't overriding a builtin function */
  for (u32 i = 0; i < KIT_ARRLEN(kit_builtins_funcs); i++) {
    const kit_builtin_func* func      = &kit_builtins_funcs[i];
    u32                     func_hash = kit_hash(func->name, strlen(func->name));
    if (func_hash == function_hash) {
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
      kit_emit_ins(&fork, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = dst, .src = src } });
    }
    /* argument on stack, mov it to our register */
    else {
      kit_emit_ins(&fork, (kit_ins){ .pop = { .opcode = KIT_IR_OPCODE_POP, .reg = dst } });
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

    for (u32 pass = 0; pass < 8; pass++) {
      if (kit_arena_reset(&codegraph_arena) < 0) return -1;
      codegraph cfg = { 0 };

      e = codegraph_init(&codegraph_arena, &fork, &cfg);
      if (e < 0) goto ERR;

      e = codegraph_build_successor_list(&fork, &cfg);
      if (e < 0) return e;
      e = codegraph_block_level_liveliness_analysis(&fork, &cfg);
      if (e < 0) return e;
      e = codegraph_instruction_level_liveliness_analysis(&fork, &cfg);
      if (e < 0) return e;
      e = codegraph_domination_analysis(&fork, &cfg);
      if (e < 0) return e;
      e = codegraph_loop_analysis(&fork, &cfg);
      if (e < 0) return e;

      // codegraph_preliminary_dead_store_elimination(&fork, &cfg);

      if (!fork.info->feature_set.disable_local_copy_propagation) {
        if (codegraph_local_copy_propagation(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      }
      if (!fork.info->feature_set.disable_dead_store_elimination) {
        if (codegraph_local_dead_store_elimination(&fork, &cfg)) codegraph_rebuild(&fork, &cfg);
      }
      // if (!fork.info->feature_set.disable_dead_store_elimination) {
      //   if (codegraph_dead_store_elimination(&fork, &cfg)) codegraph_rebuild(&fork, &cfg);
      // }
      if (!fork.info->feature_set.disable_constant_propagation) {
        if (codegraph_local_constant_propagation(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      }
      // if (!fork.info->feature_set.disable_constant_folding) {
      //   if (codegraph_constant_folding(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      // }
      // if (!fork.info->feature_set.disable_redundant_move_elimination) {
      //   if (codegraph_redundant_move_elimination(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      // }
      if (!fork.info->feature_set.disable_dead_branch_elimination) {
        if (codegraph_eliminate_unreachable_code(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      }
      if (codegraph_argument_vector_move_coalescing(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }
      if (fork.info->opt_level >= 3 && codegraph_loop_invariant_code_motion(&fork, &cfg)) { codegraph_rebuild(&fork, &cfg); }

      /* codegraph invalidated after these calls */
      if (!fork.info->feature_set.disable_redundant_jump_elimination) { remove_jmp_where_it_would_fallthrough(&fork); }
      if (!fork.info->feature_set.disable_noop_stripping) { strip_noops(&fork); }

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
    .name_hash   = function_hash,
    .nargs       = nargs,
    .vregs_used  = fork.next_vreg,
    .labels_used = fork.next_label,
  };

  e = append_function_entry(cc->arena, cc->function_table, &f);
  if (e < 0) goto ERR;

  kit_vreg_t tmp = vreg_alloc(cc);
  kit_emit_ins(cc, (kit_ins){ .loadfn = { .opcode = KIT_IR_OPCODE_LOADFN, .dst = tmp, .id = function_hash } });

  return tmp;

ERR:
  compiler_free_fork_entirely(&fork);
  return e == 0 ? -1 : e;
}