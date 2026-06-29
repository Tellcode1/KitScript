#ifndef KIT_IR_H
#define KIT_IR_H

#include "kit.stdafx.h"

typedef int kit_vreg_t;
typedef u32 kit_reg_t; /* register when it is in memory, u8 on disk */

typedef enum kit_ir_opcode_bits {
  KIT_IR_OPCODE_NOP = 0,

  /**
   * Move values between two registers.
   * The source register is still valid (so this is more of a copy).
   * MOV [dst] [src]
   */
  KIT_IR_OPCODE_MOV,

  /**
   * Move an integer into the register specified.
   * MOVI [dst] [value]
   */
  KIT_IR_OPCODE_MOVI,

  /**
   * Move a floating point number into the register specified.
   * MOVF [dst] [value]
   */
  KIT_IR_OPCODE_MOVF,

  /**
   * Move a global variable to a local register.
   * GETG [dst] [globalReg]
   */
  KIT_IR_OPCODE_GETG,

  /**
   * Assign to a global register from a local register.
   * SETG [dstGlobalReg] [localReg]
   */
  KIT_IR_OPCODE_SETG,

  /**
   * Move values between two global registers.
   * MOVG [dstGReg] [srcGReg]
   */
  KIT_IR_OPCODE_MOVG,

  /**
   * Add two registers and place them in a third.
   * Source registers are still valid.
   * ADD [dst] [a] [b]
   * dst = a+b
   */
  KIT_IR_OPCODE_ADD,
  KIT_IR_OPCODE_SUB,
  KIT_IR_OPCODE_MUL,
  KIT_IR_OPCODE_DIV,
  KIT_IR_OPCODE_MOD,
  KIT_IR_OPCODE_EXP,
  KIT_IR_OPCODE_AND,
  KIT_IR_OPCODE_OR,
  KIT_IR_OPCODE_BAND, /* Bitwise AND */
  KIT_IR_OPCODE_BOR,  /* Bitwise OR */
  KIT_IR_OPCODE_XOR,
  KIT_IR_OPCODE_EQL,
  KIT_IR_OPCODE_NEQ,
  KIT_IR_OPCODE_LT,
  KIT_IR_OPCODE_LTE,
  KIT_IR_OPCODE_GT,
  KIT_IR_OPCODE_GTE,

  /**
   * Unary operators.
   * NEG [dst] [src]
   */
  KIT_IR_OPCODE_BNOT,
  KIT_IR_OPCODE_NEG,
  KIT_IR_OPCODE_NOT,

  /* Increment a register. INC [reg] */
  KIT_IR_OPCODE_INC,
  /* Decrement a register. DEC [reg] */
  KIT_IR_OPCODE_DEC,

  /**
   * Return from a function with a value
   * RET [ReturnValueReg]
   */
  KIT_IR_OPCODE_RET,

  /**
   * Initialize a list with given elements.
   * 2nd argument must be the first register of a register array
   * if number of elements is greater than 0.
   *
   * MK_LIST [dstReg] [numElems]
   */
  KIT_IR_OPCODE_MK_LIST,

  /**
   * Pack key value pairs into a map.
   * npairs is allowed to be 0.
   * Implicitly reads from the argument vector.
   *
   * MK_MAP [dst] [nPairs]
   */
  KIT_IR_OPCODE_MK_MAP,

  /**
   * INDEX [dst] [base] [index]
   */
  KIT_IR_OPCODE_INDEX,

  /* INDEX_ASSIGN [value] [base] [index] */
  KIT_IR_OPCODE_INDEX_ASSIGN,

  /* MEMBER_ACCESS [dst] [base] [member ID : u32] */
  KIT_IR_OPCODE_MEMBER_ACCESS,
  /* MEMBER_ACCESS [value] [base] [member ID : u32] */
  KIT_IR_OPCODE_MEMBER_ASSIGN,

  /**
   * MK_STRUCT [dst] [ID]
   * Implicitly reads from the argument vector.
   */
  KIT_IR_OPCODE_MK_STRUCT,

  /* CALL [retValueReg] [functionID] [numArgs] */
  KIT_IR_OPCODE_CALL,

  KIT_IR_OPCODE_LABEL,
  KIT_IR_OPCODE_JMP,
  KIT_IR_OPCODE_JZ,
  KIT_IR_OPCODE_JNZ,

  /* Push a register to the stack [regID] */
  KIT_IR_OPCODE_PUSH,
  /* Pop a register from the stack [regID]*/
  KIT_IR_OPCODE_POP,

  /**
   * Load a constant from the constant table
   * LOADK [dst] [which]
   */
  KIT_IR_OPCODE_LOADK,

  /**
   * Assert that a condition is true, or return
   * and error if it is not.
   */
  KIT_IR_OPCODE_ASSERT,
  KIT_IR_OPCODE_LOADFN,
} kit_ir_opcode_bits;
typedef u8 kit_ir_opcode;

/**
 * An instruction loaded into memory.
 * They are tightly packed on disk.
 */
typedef union kit_ins {
  kit_ir_opcode opcode;
  struct kit_ir_ins_binop {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    kit_reg_t     a;
    kit_reg_t     b;
  } binop;
  struct kit_ir_ins_unop {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    kit_reg_t     a;
  } unop;
  struct kit_ir_ins_loadk {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    u32           id;
  } loadk;
  struct kit_ir_ins_mov {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    kit_reg_t     src;
  } mov, movg, getg, setg;
  struct kit_ir_ins_movi {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    int           value;
  } movi;
  struct kit_ir_ins_movf {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    double        value;
  } movf;
  struct kit_ir_ins_ret {
    kit_ir_opcode opcode;
    kit_reg_t     return_value;
  } ret;
  struct kit_ir_ins_mk_list {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    u32           nelems;
  } mk_list;
  struct kit_ir_ins_mk_map {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    u32           npairs;
  } mk_map;
  struct kit_ir_ins_index {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    kit_reg_t     base;
    kit_reg_t     index;
  } index;
  struct kit_ir_ins_index_assign {
    kit_ir_opcode opcode;
    kit_reg_t     value;
    kit_reg_t     base;
    kit_reg_t     index;
  } index_assign;
  struct kit_ir_ins_call {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    kit_reg_t     reg;
    u32           nargs;
  } call;
  struct kit_ir_ins_loadfn {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    u32           id;
  } loadfn;
  struct kit_ir_ins_jmp {
    kit_ir_opcode opcode;
    u32           target;
  } jmp;
  struct kit_ir_ins_cjmp { // conditional jump
    kit_ir_opcode opcode;
    kit_reg_t     condition;
    u32           target;
  } cj, jz, jnz, je, jne;
  struct kit_ir_ins_label {
    kit_ir_opcode opcode;
    u32           id;
  } label;
  struct kit_ir_ins_member_access {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    kit_reg_t     base;
    u32           member_id;
  } member_access;
  struct kit_ir_ins_member_assign {
    kit_ir_opcode opcode;
    kit_reg_t     value;
    kit_reg_t     base;
    u32           member_id;
  } member_assign;
  struct kit_ir_ins_mk_struct {
    kit_ir_opcode opcode;
    kit_reg_t     dst;
    u32           struct_id;
  } mk_struct;
  struct kit_ir_ins_push {
    kit_ir_opcode opcode;
    kit_reg_t     reg;
  } push, pop;
  struct kit_ir_ins_assert {
    kit_ir_opcode opcode;
    kit_reg_t     cond;
    u32           line_id; /* the constant ID of the line */
  } assertion;
} kit_ins;

#endif // KIT_IR_H