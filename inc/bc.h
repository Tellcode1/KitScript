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

#ifndef E_BYTECODE_H
#define E_BYTECODE_H

/**
 * TODO: Revise the entire documentation.
 * All of it is severly outdated!
 */

#include "stdafx.h"

typedef enum e_opcode_bck {
  /**
   * A 1 byte no operation instruction, used for alignment and all.
   */
  E_OPCODE_NOOP,

  /**
   * Arithmetic binary operators.
   * Operands are stored in the stack, the top being the RHS and under it the LHS.
   * Operands are popped off, and result is pushed to the stack, nullvar if operation
   * wasn't valid, say, 1 + "invalid", pushes nullvar.
   *
   * Stack state before: [..., left, right]
   * Stack state after: [..., result]
   *
   * Instruction signature: OPCODE
   */
  E_OPCODE_ADD,
  E_OPCODE_SUB,
  E_OPCODE_MUL,
  E_OPCODE_DIV,
  E_OPCODE_MOD,
  E_OPCODE_EXP,
  E_OPCODE_AND,  // Boolean AND
  E_OPCODE_OR,   // Boolean OR
  E_OPCODE_BAND, // Bitwise AND
  E_OPCODE_BOR,  // Bitwise OR
  E_OPCODE_XOR,  // Bitwise XOR (no boolean equivalent!)
  E_OPCODE_EQL,  // Equality
  E_OPCODE_NEQ,  // Inequality
  E_OPCODE_LT,
  E_OPCODE_LTE,
  E_OPCODE_GT,
  E_OPCODE_GTE,

  /**
   * Arithmetic unary operators.
   * RHS must be on the stack.
   * RHS is popped off the stack and result is pushed.
   *
   * Stack state before: [..., right]
   * Stack state after: [..., result]
   *
   * Instruction signature: OPCODE
   */
  E_OPCODE_BNOT, // Bitwise NOT
  E_OPCODE_NEG,  // Negate (-x)
  E_OPCODE_NOT,  // Boolean NOT

  /**
   * Increment or Decrement the top of the stack
   * Modifies stack in place.
   * Only integers or floats may be incremented, otherwise
   * serves as a NOOP, though this behaviour is undefined.
   *
   * Stack state before: [..., RHS]
   * Stack state after: [..., RHS] [STATE UNCHANGED]
   *
   * Instruction signature: INC/DEC [Variable ID]
   */
  E_OPCODE_INC,
  E_OPCODE_DEC,

  /**
   * Call a function, be it builtin, external or user defined.
   * Arguments are pushed to stack, in correct order.
   * If function does not return a value explicitly, it will return nullvar. Always.
   *
   * Stack state before: [... arg0, arg1, arg2]
   * Stack state after: [... return value], return value may be nullvar.
   * Instruction signature: CALL [Function ID : u32] [Number of arguments: u16]
   */
  E_OPCODE_CALL,

  /**
   * Return from a function or procedure. Optionally, an explicit value can be returned to the
   * caller, otherwise nullvar is returned.
   *
   * Stack state before: [... if have_return_value: return value]
   * Stack state after: Stack invalidated, it no longer exists.
   * Instruction signature: RETURN [have_return_value : u8]
   */
  E_OPCODE_RETURN,

  /**
   * Push a literal from the literal table with the hash specified.
   *
   * State stack before: [...]
   * Stack state after: [..., deep copy of literal]
   * Instruction signature: LITERAL [hash : u32]
   */
  E_OPCODE_LITERAL,

  /**
   * Load a shallow copy of a variable (that was INIT'd) from the map using its ID.
   * nullvar if no such variable exists.
   *
   * State stack before: [...]
   * Stack state after: [..., shallow copy of variable]
   * Instruction signature: LOAD [Variable ID : u32]
   */
  E_OPCODE_LOAD,

  /**
   * Pop the top of the stack.
   *
   * State stack before: [..., top]
   * Stack state after: [...]
   * Instruction signature: POP
   */
  E_OPCODE_POP,

  /**
   * Duplicate (shallow copy!) the top of the stack.
   *
   * State stack before: [..., top]
   * Stack state after: [..., top, shallow copy of top]
   * Instruction signature: DUP
   */
  E_OPCODE_DUP,

  /**
   * Store the top of the stack (shallow copy) to the variable ID's slot.
   * Stored value (top of stack) is *not* popped, to support chained assignments.
   *
   * Stack state before: [slot, ..., value]
   * Stack state after: [*slot= shallow copy of value, ..., value]
   * Instruction signature: ASSIGN [Variable ID : u32]
   */
  E_OPCODE_ASSIGN,

  /**
   * Assign to a member of a struct/container.
   *
   * The modified base structure is pushed, and the assigned value is preserved.
   * Only the index is popped.
   * (See the Stack state delta, it will make sense)
   *
   * It's complicated like this since we need to write back structures like
   * vec2s to their slots. After  the writeback, the modified base structures are popped
   * with instructions by the compiler (since we don't know where the vec2 originates from at runtime).
   *
   * Stack state before: [..., base, index, value]
   * Stack state after: [... value, base]
   * Instruction signature: INDEX_ASSIGN [Variable ID : u32]
   */
  E_OPCODE_INDEX_ASSIGN,

  /**
   * Index into a structure/list/string/map and push the result to the stack
   * without popping off base and index.
   * Used for index assigns, mostly.
   *
   * Stack state before: [..., base, index]
   * Stack state after: [... base, index, indexed]
   * Instruction signature: INDEX_ASSIGN
   */
  E_OPCODE_INDEX_PEEK,

  /**
   * Push a variable to the stack, and set its ID on the variable table.
   * The initial value of the variable will be nullvar.
   * INIT'd variables are popped when the frame they were INIT'd in is popped.
   *
   * Stack state before: [...]
   * Stack state after: [... variable (*variable = nullvar)]
   * Instruction signature: INIT [Variable ID : u32]
   */
  E_OPCODE_INIT,

  /**
   * Pack elements into a single list.
   * The list is pushed to the stack. Elements are popped off.
   * nelems is allowed to be 0.
   *
   * Stack state before: [... elem0, elem1, elem2]
   * Stack state after: [... list]
   * Instruction signature: MK_LIST [num_elems : u32]
   */
  E_OPCODE_MK_LIST,

  /**
   * Pack key value pairs into a map.
   * The map is pushed to the stack.
   * npairs is allowed to be 0.
   * All key value pairs are released from the stack.
   * Key/Value pairs are pushed in forward order:
   * for (pair) {
   *   compile(pair.key)
   *   compile(pair.value)
   * }
   * And is popped in reverse order (value first, key next).
   * Order of KV pairs does not matter in maps, as they
   * do not have any particular order.
   *
   * Stack state before: [... pair0.key, pair0.value, pair1.key, pair1.value]
   * Stack state after: [... map]
   * Instruction signature: MK_MAP [npairs : u32]
   */
  E_OPCODE_MK_MAP,

  /**
   * Index into an list, string or structure.
   * The returned value is not an lvalue, you can not modify it and expect
   * modifications to propogate across all copies.
   * If index is out of range, returns nullvar.
   *
   * Stack state before: [... base, index]
   * Stack state after: [... value (*value = base[index])]
   * Instruction signature: INDEX
   */
  E_OPCODE_INDEX,

  /**
   * A label with an ID. JMPs can refer to this ID to ensure
   * byte code changes do not require a retracking.
   * Serves as a NOOP during runtime.
   * One operand is used, as the label ID. All jumps to this label must have the
   * same label ID.
   *
   * Instruction signature: LABEL [labelID : u32]
   */
  E_OPCODE_LABEL,

  /**
   * Jump to the specified label.
   * Target (operand) is the offset in the instruction stream to jump to.
   * Out of (current) stream jumps are not allowed.
   * Only CALL is allowed to access other instruction streams.
   *
   * If target is out of range, the interpreter prints an error and returns
   * from the executing function with nullvar (not the entire program!).
   * During compilation, JMP initially has the label ID as its operand
   * This is later patched (when the label is defined) to be the target instruction index.
   *
   * Stack state before: [...]
   * Stack state after: [...]
   * Instruction signature: JMP [Target : u32]
   */
  E_OPCODE_JMP,

  /**
   * Jump if top of stack is equal to the value under it.
   * For more information, refer to JMP.
   * If a == b, (comparison is done through e_var_equal), jump to the target.
   * Conditions are popped.
   *
   * Stack state before: [..., a, b]
   * Stack state after: [...]
   * Instruction signature: JE [Target : u32]
   */
  E_OPCODE_JE,

  /**
   * Jump if top of stack is not equal to the value under it.
   * For more information, refer to JMP.
   * If a != b, (comparison is done through e_var_equal), jump to the target.
   * Conditions are popped.
   *
   * Stack state before: [..., a, b]
   * Stack state after: [...]
   * Instruction signature: JNE [Target : u32]
   */
  E_OPCODE_JNE,

  /**
   * Jump if top of stack is equal to zero (only booleans/chars/integers).
   * For more information, refer to JMP.
   * If e_cast_to_int(stack top) == 0, then jump to the target.
   *
   * Stack state before: [..., a]
   * Stack state after: [...]
   * Instruction signature: JZ [Target : u32]
   */
  E_OPCODE_JZ,

  /**
   * Jump if top of stack is not equal to zero (only booleans/chars/integers).
   * For more information, refer to JMP.
   * If e_cast_to_int(stack top) != 0, then jump to the target.
   *
   * Stack state before: [..., a]
   * Stack state after: [...]
   * Instruction signature: JNZ [Target : u32]
   */
  E_OPCODE_JNZ,

  /**
   * Push/pop the stack frame.
   * Push stores the current stack top (offset)and variable map to the stack.
   * Pop causes the VM to pop all variables that will go out of scope
   * and set the stack top and variable map to the stored values.
   *
   * The (pushed) state can be restored by calling E_OPCODE_POP_FRAME
   *
   * No operands are used.
   *
   * Stack state before: [...]
   * Stack state after: [...]
   * Instruction signature: PUSH_FRAME/POP_FRAME
   */
  E_OPCODE_PUSH_FRAME,
  E_OPCODE_POP_FRAME,

  /**
   * Make a struct with a set number of members
   * MK_STRUCT [nMembers : u32]
   */
  E_OPCODE_MK_STRUCT,

  /**
   * Access a member by name from a variable.
   * Usage(noattr) MEMBER_ACCESS [MemberNameHashed : u32], Top = Namespace/Struct
   */
  E_OPCODE_MEMBER_ACCESS,

  /**
   * Assign to a member of a struct by name.
   *  MEMBER_ACCESS [MemberNameHashed : u32], Top=Value, Top-1 = Namespace/Struct
   */
  E_OPCODE_MEMBER_ASSIGN,

  /**
   * If the top of the stack is equal to false, print an error and bail out.
   *
   * Stack state before: [..., assertion]
   * Stack state after: [...]
   * Instruction signature: ASSERT [Error string literal ID : u32]
   */
  E_OPCODE_ASSERT,

  /**
   * A special opcode for making a structure (with specified hash)
   * and filling it one by one with values from the stack.
   * Optionally, this function can directly copy the arguments from the
   * current function's argument list.
   * Stack mode (default) moves the structure members from the stack to our structure.
   *
   * Stack state before [Stack mode]: [..., member0, member1, member2, ...]
   * Stack state after: [..., structure]
   * Instruction signature: STRUCT_CONSTRUCT [Structure ID : u32] [Copy from argument list : u8]
   */
  E_OPCODE_STRUCT_CONSTRUCT,

  /**
   * Exit the program with the code specified in 1st operand.
   * Usage: HALT [exit code : u32]
   */
  E_OPCODE_HALT,

  /* E_OPCODE num */
  E_OPCODE_COUNT,
} e_opcode_bck;
typedef u8 e_opcode;

static const u8 static_assert__e_opcode_must_have_less_than_256_entries__[E_OPCODE_COUNT <= 256 ? 1 : -1] = { 0 };

/**
 * An instruction loaded into memory.
 * They are tightly packed on disk.
 */
typedef struct e_ins {
  u8 opcode;
  union {
    u32 init;
    u32 load;
    u32 halt;
    u32 jmp; // , jz, jnz, je, jne
    u32 assign;
    u32 mk_list, mk_map;
    u32 index_assign;
    u32 label;
    u32 literal, assertion; // assert refers to the literal in which error string is stored.
    u8  has_return_value;   // return
    struct {
      u16 nargs;
      u32 hash;
    } call;
    u32 mk_struct;
    struct {
      u32 id;
      u8  arg_mode;
    } struct_construct;
    u32 member;
  } v;
} e_ins;

static inline const char*
e_opcode_to_str(e_opcode_bck op)
{
  switch (op) {
    case E_OPCODE_NOOP: return "NOOP";
    case E_OPCODE_ADD: return "ADD";
    case E_OPCODE_SUB: return "SUB";
    case E_OPCODE_MUL: return "MUL";
    case E_OPCODE_DIV: return "DIV";
    case E_OPCODE_MOD: return "MOD";
    case E_OPCODE_EXP: return "EXP";
    case E_OPCODE_AND: return "AND";
    case E_OPCODE_OR: return "OR";
    case E_OPCODE_NOT: return "NOT";
    case E_OPCODE_BAND: return "BAND";
    case E_OPCODE_BOR: return "BOR";
    case E_OPCODE_XOR: return "XOR";
    case E_OPCODE_BNOT: return "BNOT";
    case E_OPCODE_EQL: return "EQL";
    case E_OPCODE_NEQ: return "NEQ";
    case E_OPCODE_LT: return "LT";
    case E_OPCODE_LTE: return "LTE";
    case E_OPCODE_GT: return "GT";
    case E_OPCODE_GTE: return "GTE";
    case E_OPCODE_NEG: return "NEG";
    case E_OPCODE_INC: return "INC";
    case E_OPCODE_DEC: return "DEC";
    case E_OPCODE_CALL: return "CALL";
    case E_OPCODE_RETURN: return "RETURN";
    case E_OPCODE_LITERAL: return "LITERAL";
    case E_OPCODE_LOAD: return "LOAD";
    case E_OPCODE_POP: return "POP";
    case E_OPCODE_DUP: return "DUP";
    case E_OPCODE_ASSIGN: return "ASSIGN";
    case E_OPCODE_INDEX_ASSIGN: return "INDEX_ASSIGN";
    case E_OPCODE_INIT: return "INIT";
    case E_OPCODE_MK_LIST: return "MK_LIST";
    case E_OPCODE_MK_MAP: return "MK_MAP";
    case E_OPCODE_INDEX: return "INDEX";
    case E_OPCODE_LABEL: return "LABEL";
    case E_OPCODE_JMP: return "JMP";
    case E_OPCODE_JE: return "JE";
    case E_OPCODE_JNE: return "JNE";
    case E_OPCODE_JZ: return "JZ";
    case E_OPCODE_JNZ: return "JNZ";
    case E_OPCODE_PUSH_FRAME: return "PUSH_FRAME";
    case E_OPCODE_POP_FRAME: return "POP_FRAME";
    case E_OPCODE_MK_STRUCT: return "MK_STRUCT";
    case E_OPCODE_MEMBER_ACCESS: return "MEMBER_ACCESS";
    case E_OPCODE_MEMBER_ASSIGN: return "MEMBER_ASSIGN";
    case E_OPCODE_HALT: return "HALT";
    case E_OPCODE_COUNT: return "COUNT";
    case E_OPCODE_INDEX_PEEK: return "INDEX_PEEK";
    case E_OPCODE_ASSERT: return "ASSERT";
    case E_OPCODE_STRUCT_CONSTRUCT: return "STRUCT_CONSTRUCT";
  }
  return "UNKNOWN";
}

#endif // E_IR_H
