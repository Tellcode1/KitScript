#include "../../inc/kit.cc.h"
#include "../../inc/kit.operate.h"
#include "../../inc/kit.rwhelp.h"
#include "ast.extract.info.h"
#include "compile_routines.h"
#include "lvalue.h"
#include "vreg.h"

void
define_and_emit_label(kit_compiler* cc, u32 label_id)
{
  kit_emit_ins(cc, (kit_ins){ .label = { .opcode = KIT_IR_OPCODE_LABEL, .id = label_id } });
}

static inline kit_ir_opcode
kit_binary_operator_to_opcode(kit_operator op)
{
  switch (op) {
    case KIT_OPERATOR_ADD: return KIT_IR_OPCODE_ADD;
    case KIT_OPERATOR_SUB: return KIT_IR_OPCODE_SUB;
    case KIT_OPERATOR_MUL: return KIT_IR_OPCODE_MUL;
    case KIT_OPERATOR_DIV: return KIT_IR_OPCODE_DIV;
    case KIT_OPERATOR_MOD: return KIT_IR_OPCODE_MOD;
    case KIT_OPERATOR_EXP: return KIT_IR_OPCODE_EXP;
    case KIT_OPERATOR_AND: return KIT_IR_OPCODE_AND;
    case KIT_OPERATOR_OR: return KIT_IR_OPCODE_OR;
    case KIT_OPERATOR_BAND: return KIT_IR_OPCODE_BAND;
    case KIT_OPERATOR_BOR: return KIT_IR_OPCODE_BOR;
    case KIT_OPERATOR_XOR: return KIT_IR_OPCODE_XOR;
    case KIT_OPERATOR_ISEQL: return KIT_IR_OPCODE_EQL;
    case KIT_OPERATOR_ISNEQ: return KIT_IR_OPCODE_NEQ;
    case KIT_OPERATOR_LT: return KIT_IR_OPCODE_LT;
    case KIT_OPERATOR_LTE: return KIT_IR_OPCODE_LTE;
    case KIT_OPERATOR_GT: return KIT_IR_OPCODE_GT;
    case KIT_OPERATOR_GTE: return KIT_IR_OPCODE_GTE;
    case KIT_OPERATOR_NOT: return KIT_IR_OPCODE_NOT;
    case KIT_OPERATOR_BNOT: return KIT_IR_OPCODE_BNOT;
    case KIT_OPERATOR_DEC: return KIT_IR_OPCODE_DEC;
    case KIT_OPERATOR_INC: return KIT_IR_OPCODE_INC;
  }
  return -1;
}

kit_vreg_t
compile_binary_op(kit_compiler* cc, int node)
{
  val_t lv = { 0 };

  bool is_compound = KIT_GET_NODE(cc->ast, node)->binaryop.is_compound;
  int  left        = KIT_GET_NODE(cc->ast, node)->binaryop.left;
  int  right       = KIT_GET_NODE(cc->ast, node)->binaryop.right;

  kit_ir_opcode opcode = kit_binary_operator_to_opcode(KIT_GET_NODE(cc->ast, node)->binaryop.op);
  if (opcode < 0) {
    cerror(KIT_GET_NODE(cc->ast, node)->common.span, "Operator %u can not be used as a binary operator\n", KIT_GET_NODE(cc->ast, node)->binaryop.op);
    goto err;
  }

  /* handle && and || explicitly to support short circuiting */
  if (opcode == KIT_IR_OPCODE_AND || opcode == KIT_IR_OPCODE_OR) {
    u32 short_circuit_label = make_label_id(cc);
    u32 end_label           = make_label_id(cc);

    kit_vreg_t dst = vreg_alloc(cc);
    kit_vreg_t lhs = compile(cc, left);
    if (lhs < 0) goto err;

    if (opcode == KIT_IR_OPCODE_AND) {
      if (emit_and_record_jmp(cc, KIT_IR_OPCODE_JZ, lhs, short_circuit_label) < 0) { return -1; }
    } else {
      if (emit_and_record_jmp(cc, KIT_IR_OPCODE_JNZ, lhs, short_circuit_label) < 0) return -1;
    }

    kit_vreg_t rhs = compile(cc, right);
    if (rhs < 0) goto err;

    kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = dst, .src = rhs } });
    if (emit_and_record_jmp(cc, KIT_IR_OPCODE_JMP, -1, end_label) < 0) return -1;

    define_and_emit_label(cc, short_circuit_label);
    kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = dst, .src = lhs } });

    define_and_emit_label(cc, end_label);
    return dst;
  }

  /* 1++ my favourite expression */
  if (is_compound && !can_make_value(cc->ast, left)) {
    cerror(KIT_GET_NODE(cc->ast, left)->common.span, "Can not assign to left\n");
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

kit_vreg_t
compile_inc_or_dec(kit_compiler* cc, int node)
{
  kit_ir_opcode opcode = -1;
  if (KIT_GET_NODE(cc->ast, node)->unaryop.op == KIT_OPERATOR_INC) {
    opcode = KIT_IR_OPCODE_ADD;
  } else if (KIT_GET_NODE(cc->ast, node)->unaryop.op == KIT_OPERATOR_DEC) {
    opcode = KIT_IR_OPCODE_SUB;
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

kit_vreg_t
compile_unary_op(kit_compiler* cc, int node)
{
  /* Special case, INC and DEC assign to variables directly. */
  kit_operator oper = KIT_GET_NODE(cc->ast, node)->unaryop.op;
  if (oper == KIT_OPERATOR_INC || oper == KIT_OPERATOR_DEC) { return compile_inc_or_dec(cc, node); }

  int  right       = KIT_GET_NODE(cc->ast, node)->unaryop.right;
  bool is_compound = KIT_GET_NODE(cc->ast, node)->unaryop.is_compound;

  /* +x */
  if (oper == KIT_OPERATOR_ADD && !is_compound) return compile(cc, right);

  kit_ir_opcode opcode = -1;

  switch (KIT_GET_NODE(cc->ast, node)->unaryop.op) {
    case KIT_OPERATOR_NOT: opcode = KIT_IR_OPCODE_NOT; break;
    case KIT_OPERATOR_BNOT: opcode = KIT_IR_OPCODE_BNOT; break;
    case KIT_OPERATOR_SUB: opcode = KIT_IR_OPCODE_NEG; break;
    case KIT_OPERATOR_ADD: opcode = KIT_IR_OPCODE_NOP; break;
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
