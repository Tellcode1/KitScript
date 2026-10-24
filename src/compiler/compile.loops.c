#include "../../inc/kit.cc.h"
#include "../../inc/kit.rwhelp.h"
#include "compile_routines.h"
#include "defer.h"
#include "scope.h"

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
kit_vreg_t
compile_while_statement(kit_compiler* cc, int node)
{
  int e = 0;

  /* Ensure we don't overwrite it... */
  kitc_loop_location* last = cc->loop;

  /* Computes the condition and jumps to the end label (breaks) if condition is false */
  const u32 pre_condition_label = make_label_id(cc);

  /* After the while loop, with one POP_VARIABLES to ensure we always pop our variables. */
  const u32 end_label = make_label_id(cc);

  define_and_emit_label(cc, pre_condition_label);

  scope_push(cc);

  /**
   * Push frame for the stack
   */
  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  /* CONDITION */
  kit_vreg_t cond = compile(cc, KIT_GET_NODE(cc->ast, node)->while_stmt.condition);
  if (cond < 0) goto ERR;

  // Break out of loop if condition is false.
  e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JZ, cond, end_label);
  if (e < 0) goto ERR;

  /* Append a loop entry to our compiler. */
  kitc_loop_location loop = {
    .continue_label = pre_condition_label,
    .break_label    = end_label,
    .defer_depth    = defer_get_current_depth(cc),
  };

  cc->loop = &loop;

  // WHILE BODY
  const int* while_body = KIT_GET_NODE(cc->ast, node)->while_stmt.stmts;
  for (u32 i = 0; i < KIT_GET_NODE(cc->ast, node)->while_stmt.nstmts; i++) {
    e = compile(cc, while_body[i]);
    if (e < 0) goto ERR;
  }

  e = defer_emit_current_scope(cc);
  if (e < 0) goto ERR;

  /* Jump to condition, body is done executing */
  e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, pre_condition_label);
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
  cc->loop = last;
  return e ? e : -1;
}

kit_vreg_t
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
    cerror(KIT_GET_NODE(cc->ast, cond)->common.span, "Failed to compile condition [for statement]\n");
    goto ERR;
  }

  // JZ END_LABEL
  e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JZ, cond_reg, end_label);
  if (e < 0) goto ERR;

  e = defer_push_scope(cc);
  if (e < 0) goto ERR;

  u32 old_depth = cc->loop->defer_depth;
  if (cc->loop) cc->loop->defer_depth = defer_get_current_depth(cc);

  // BODY
  u32        nstmts = KIT_GET_NODE(cc->ast, node)->for_stmt.nstmts;
  const int* stmts  = KIT_GET_NODE(cc->ast, node)->for_stmt.stmts;
  for (u32 i = 0; i < nstmts; i++) {
    e = compile(cc, stmts[i]);
    if (e < 0) {
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
  e = emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, top_label);
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

kit_vreg_t
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

  kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = iterator_reg, .src = iterator_start_value } });

  /* calc = stop < start [for i in 10..0]*/
  kit_emit_ins(
      cc,
      (kit_ins){
          .binop = {
              .opcode = KIT_IR_OPCODE_LT, .dst = iterator_increment_amount_calculation, .a = iterator_stop_value, .b = iterator_start_value } });

  /* jnz calc reverse_direction */
  kit_emit_ins(
      cc,
      (kit_ins){
          .jnz = { .opcode = KIT_IR_OPCODE_JNZ, .target = reverse_iterator_direction_label, .condition = iterator_increment_amount_calculation } });

  /* jnz failed, normal iterator direction */
  kit_emit_ins(cc, (kit_ins){ .movi = { .opcode = KIT_IR_OPCODE_MOVI, .dst = iterator_increment_amount, .value = 1 } });
  kit_emit_ins(cc, (kit_ins){ .jmp = { .opcode = KIT_IR_OPCODE_JMP, .target = end_iterator_direction_calculation_label } });

  define_and_emit_label(cc, reverse_iterator_direction_label);

  /* reverse iterator direction */
  kit_emit_ins(cc, (kit_ins){ .movi = { .opcode = KIT_IR_OPCODE_MOVI, .dst = iterator_increment_amount, .value = -1 } });

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
  kit_emit_ins(cc, (kit_ins){ .binop = { .opcode = KIT_IR_OPCODE_EQL, .dst = condition_check, .a = iterator_reg, .b = iterator_stop_value } });
  if (emit_and_record_jmp(cc, KIT_IR_OPCODE_JNZ, condition_check, end_label) < 0) goto ERR;

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
  kit_emit_ins(cc, (kit_ins){ .binop = { .opcode = KIT_IR_OPCODE_ADD, .dst = iterator_reg, .a = iterator_reg, .b = iterator_increment_amount } });

  if (emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, start_label) < 0) goto ERR;

  define_and_emit_label(cc, end_label);

  scope_pop(cc);
  defer_pop_scope(cc);

  cc->loop = old_loop;

  return 0;

ERR:
  cc->loop = old_loop;
  return -1;
}
