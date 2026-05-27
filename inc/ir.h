#ifndef ESL_IR_H
#define ESL_IR_H

#include "stdafx.h"

typedef int ereg_t;

typedef enum eir_opcode {
  EIR_OPCODE_NOP,

  /**
   * Load a constant from the constant table
   * LOADK [dst] [which]
   */
  EIR_OPCODE_LOADK,

  /**
   * Move values between two registers.
   * The source register is still valid (so this is more of a copy).
   * MOV [dst] [src]
   */
  EIR_OPCODE_MOV,

  /**
   * Move a global variable to a local register.
   * GETG [dst] [globalReg]
   */
  EIR_OPCODE_GETG,

  /**
   * Assign to a global register from a local register.
   * SETG [dstGlobalReg] [localReg]
   */
  EIR_OPCODE_SETG,

  /**
   * Move values between two global registers.
   * MOVG [dstGReg] [srcGReg]
   */
  EIR_OPCODE_MOVG,

  /**
   * Add two registers and place them in a third.
   * Source registers are still valid.
   * ADD [dst] [a] [b]
   * dst = a+b
   */
  EIR_OPCODE_ADD,
  EIR_OPCODE_SUB,
  EIR_OPCODE_MUL,
  EIR_OPCODE_DIV,
  EIR_OPCODE_MOD,
  EIR_OPCODE_EXP,
  EIR_OPCODE_AND,
  EIR_OPCODE_OR,
  EIR_OPCODE_BAND,
  EIR_OPCODE_BOR,
  EIR_OPCODE_XOR,
  EIR_OPCODE_EQL,
  EIR_OPCODE_NEQ,
  EIR_OPCODE_LT,
  EIR_OPCODE_LTE,
  EIR_OPCODE_GT,
  EIR_OPCODE_GTE,

  /**
   * Unary operators.
   * NEG [dst] [src]
   */
  EIR_OPCODE_BNOT,
  EIR_OPCODE_NEG,
  EIR_OPCODE_NOT,

  /* Increment a register. INC [reg] */
  EIR_OPCODE_INC,
  /* Decrement a register. DEC [reg] */
  EIR_OPCODE_DEC,

  /**
   * Return from a function with a value
   * RET [ReturnValueReg]
   */
  EIR_OPCODE_RET,

  /**
   * Initialize a list with given elements.
   * 2nd argument must be the first register of a register array
   * if number of elements is greater than 0.
   *
   * MK_LIST [dstReg] [numElems]
   */
  EIR_OPCODE_MK_LIST,

  /**
   * Pack key value pairs into a map.
   * npairs is allowed to be 0.
   * Implicitly reads from the argument vector.
   *
   * MK_MAP [dst] [nPairs]
   */
  EIR_OPCODE_MK_MAP,

  /**
   * INDEX [dst] [base] [index]
   */
  EIR_OPCODE_INDEX,

  /* INDEX_ASSIGN [value] [base] [index] */
  EIR_OPCODE_INDEX_ASSIGN,

  /* MEMBER_ACCESS [dst] [base] [member ID : u32] */
  EIR_OPCODE_MEMBER_ACCESS,
  /* MEMBER_ACCESS [value] [base] [member ID : u32] */
  EIR_OPCODE_MEMBER_ASSIGN,

  /**
   * MK_STRUCT [dst] [ID]
   * Implicitly reads from the argument vector.
   */
  EIR_OPCODE_MK_STRUCT,

  /* CALL [retValueReg] [functionID] [numArgs] */
  EIR_OPCODE_CALL,

  EIR_OPCODE_LABEL,
  EIR_OPCODE_JMP,
  EIR_OPCODE_JZ,
  EIR_OPCODE_JNZ,

  /* Push a register to the stack [regID] */
  EIR_OPCODE_PUSH,
  /* Pop a register from the stack [regID]*/
  EIR_OPCODE_POP,
} eir_opcode;

/**
 * An instruction loaded into memory.
 * They are tightly packed on disk.
 */
typedef union e_ins {
  eir_opcode opcode;
  struct eir_ins_binop {
    eir_opcode opcode;
    u32        dst;
    u32        a;
    u32        b;
  } binop;
  struct eir_ins_unop {
    eir_opcode opcode;
    u32        dst;
    u32        a;
  } unop;
  struct eir_ins_loadk {
    eir_opcode opcode;
    u32        dst;
    u32        id;
  } loadk;
  struct eir_ins_mov {
    eir_opcode opcode;
    u32        dst;
    u32        src;
  } mov, movg, getg, setg;
  struct eir_ins_ret {
    eir_opcode opcode;
    u32        return_value;
  } ret;
  struct eir_ins_mk_list {
    eir_opcode opcode;
    u32        dst;
    u32        nelems;
  } mk_list;
  struct eir_ins_mk_map {
    eir_opcode opcode;
    u32        dst;
    u32        npairs;
  } mk_map;
  struct eir_ins_index {
    eir_opcode opcode;
    u32        dst;
    u32        base;
    u32        index;
  } index;
  struct eir_ins_index_assign {
    eir_opcode opcode;
    u32        value;
    u32        base;
    u32        index;
  } index_assign;
  struct eir_ins_call {
    eir_opcode opcode;
    u32        dst;
    u32        function_id;
    u32        nargs;
  } call;
  struct eir_ins_jmp {
    eir_opcode opcode;
    u32        target;
  } jmp;
  struct eir_ins_cjmp { // conditional jump
    eir_opcode opcode;
    u32        target;
    u32        condition;
  } cj, jz, jnz, je, jne;
  struct eir_ins_label {
    eir_opcode opcode;
    u32        id;
  } label;
  struct eir_ins_member_access {
    eir_opcode opcode;
    u32        dst;
    u32        base;
    u32        member_id;
  } member_access;
  struct eir_ins_member_assign {
    eir_opcode opcode;
    u32        value;
    u32        base;
    u32        member_id;
  } member_assign;
  struct eir_ins_mk_struct {
    eir_opcode opcode;
    u32        dst;
    u32        struct_id;
  } mk_struct;
  struct eir_ins_push {
    eir_opcode opcode;
    u32        reg;
  } push, pop;
} e_ins;

#endif // ESL_IR_H