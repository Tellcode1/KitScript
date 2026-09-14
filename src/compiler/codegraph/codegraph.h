#ifndef KIT_COMPILER_CODEGRAPH_H
#define KIT_COMPILER_CODEGRAPH_H

#include "../../../inc/kit.ir.h"
#include "../../../inc/kit.stdafx.h"

struct kit_arena;
struct kit_compiler;
union kit_ins;

typedef struct codeblock {
  u32 start; /* first instruction (inclusive) */
  u32 end;   /* last instruction (inclusive) */

  u32 idom; /* The immediate dominater */

  /* Block can really only have 2 successors max (JZ and JNZ, namely the condition failed branch and the condition pass branch) */
  u32 nsuccessors;
  u32 npredecessors;

  u32  successors[2];
  u32* predecessors;

  bool* defines;
  bool* uses;

  /* block level liveliness analysis. Arena allocated */
  bool* live_in;  /* [nvregs] */
  bool* live_out; /* [nvregs] */

  /* Which codeblocks dominate this block */
  bool* dominators; /* [nblocks] */

} codeblock;

typedef struct loop {
  u32 header; /* loop header index */

  u32  nlatches;
  u32* latches; /* loop latch (connector) indices */

  bool* is_block_member; /* which blocks are members of this loop [nblocks] */
  bool* is_ins_member;   /* which instructions are members of this loop [ninstructions] */
} blockloop;

/* our CFG */
typedef struct codegraph {
  struct kit_arena* arena;

  codeblock* blocks; /* arena allocated  */
  u32        nblocks;

  blockloop* loops;
  u32        nloops;

  /* instruction level liveliness analysis. Arena allocated */
  bool** ins_live_out; /* [ninstructions][nvregs] */
  bool** ins_live_in;  /* [ninstructions][nvregs] */

  u32 entry_block; /* index of the entry codeblock */

  u32 nvregs;
} codegraph;

ATTR_NODISCARD int codegraph_init(struct kit_arena* a, struct kit_compiler* cc, codegraph* dst);
void               codegraph_free(struct kit_compiler* cc, codegraph* graph);
int                codegraph_rebuild(struct kit_compiler* cc, codegraph* cfg);

u32  get_destination_reg(const union kit_ins* i);
void set_destination_reg(union kit_ins* i, u32 value);
u32  get_source_registers(const union kit_ins* i, u32 sources[32]);

u32 next_real_ins(struct kit_compiler* cc, u32 start);

void opt_inline_function_calls(struct kit_compiler* cc);

bool is_instruction_impure(kit_ir_opcode op);
bool is_instruction_noop(kit_ir_opcode opcode);

int codegraph_block_level_liveliness_analysis(struct kit_compiler* cc, codegraph* dst);
int codegraph_instruction_level_liveliness_analysis(struct kit_compiler* cc, codegraph* dst);
int codegraph_domination_analysis(struct kit_compiler* cc, codegraph* cfg);
int codegraph_loop_analysis(struct kit_compiler* cc, codegraph* cfg);
int codegraph_build_successor_list(struct kit_compiler* cc, codegraph* dst);

bool codegraph_dead_store_elimination(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_constant_folding(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_preliminary_dead_store_elimination(struct kit_compiler* cc, const codegraph* cfg);
bool codegraph_redundant_move_elimination(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_eliminate_unreachable_code(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_local_constant_propagation(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_local_copy_propagation(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_local_dead_store_elimination(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_loop_invariant_code_motion(struct kit_compiler* cc, codegraph* cfg);
bool codegraph_argument_vector_move_coalescing(struct kit_compiler* cc, codegraph* cfg);

/**
 * Remove any functions that are not used throught all the functions in cc.
 * This function is only expected to be called on the root stream.
 */
bool codegraph_root_unused_function_elimination(struct kit_compiler* cc, codegraph* cfg);

#endif // KIT_COMPILER_CODEGRAPH_H