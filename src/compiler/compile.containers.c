#include "../../inc/kit.ast.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.reg.h"
#include "../../inc/kit.rwhelp.h"
#include "compile_routines.h"
#include "vreg.h"

RETURNS_ERRCODE int
compile_list(kit_compiler* cc, int node)
{
  u32 nelems = KIT_GET_NODE(cc->ast, node)->list.nelems;

  kit_vreg_t dst = vreg_alloc(cc);

  kit_vreg_t* elems = kit_arnalloc(cc->arena, sizeof(kit_vreg_t) * nelems);

  for (u32 i = 0; i < nelems; i++) {
    int elem_node = KIT_GET_NODE(cc->ast, node)->list.elems[i];

    elems[i] = compile(cc, elem_node);
    if (elems[i] < 0) return elems[i];
  }

  for (u32 i = 0; i < nelems; i++) {
    /* Move at most 16 elements from their registers to our argument list. */
    if (i < KIT_REG_ARG_COUNT) kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = KIT_REG_ARG0 + i, .src = elems[i] } });
    /* else push them to the stack */
    else kit_emit_ins(cc, (kit_ins){ .push = { .opcode = KIT_IR_OPCODE_PUSH, .reg = elems[i] } });
  }

  kit_emit_ins(cc, (kit_ins){ .mk_list = { .opcode = KIT_IR_OPCODE_MK_LIST, .dst = dst, .nelems = nelems } });

  /* remove spilled elements from the stack */
  kit_vreg_t tmp = vreg_alloc(cc);
  for (u32 i = KIT_REG_ARG_COUNT; i < nelems; i++) {
    /* pop repeatedly into tmp */
    kit_emit_ins(cc, (kit_ins){ .pop = { .opcode = KIT_IR_OPCODE_POP, .reg = tmp } });
  }

  return dst;
}

RETURNS_ERRCODE int
compile_map(kit_compiler* cc, int node)
{
  u32 npairs = KIT_GET_NODE(cc->ast, node)->map.npairs;

  kit_vreg_t dst = vreg_alloc(cc);

  kit_vreg_t* pairs = kit_arnalloc(cc->arena, npairs * 2ULL * sizeof(kit_vreg_t));

  for (u32 i = 0; i < npairs; i++) {
    int key = KIT_GET_NODE(cc->ast, node)->map.keys[i];
    int val = KIT_GET_NODE(cc->ast, node)->map.values[i];

    kit_vreg_t k = compile(cc, key);
    if (k < 0) return k;

    kit_vreg_t v = compile(cc, val);
    if (v < 0) return v;

    pairs[((size_t)i * 2)]     = k;
    pairs[((size_t)i * 2) + 1] = v;
  }

  for (u32 i = 0; i < npairs; i++) {
    /* copy the first 8 pairs (16 registers) and spill the rest to the stack */
    kit_vreg_t k = pairs[((size_t)i * 2)];
    kit_vreg_t v = pairs[((size_t)i * 2) + 1];

    if (i < (KIT_REG_ARG_COUNT / 2)) {
      /* copy KV pairs into our argument vector */
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = KIT_REG_ARG0 + (i * 2), .src = k } });
      kit_emit_ins(cc, (kit_ins){ .mov = { .opcode = KIT_IR_OPCODE_MOV, .dst = KIT_REG_ARG0 + (i * 2) + 1, .src = v } });
    } else {
      /* spill! */
      kit_emit_ins(cc, (kit_ins){ .push = { .opcode = KIT_IR_OPCODE_PUSH, .reg = k } });
      kit_emit_ins(cc, (kit_ins){ .push = { .opcode = KIT_IR_OPCODE_PUSH, .reg = v } });
    }
  }

  kit_emit_ins(cc, (kit_ins){ .mk_map = { .opcode = KIT_IR_OPCODE_MK_MAP, .dst = dst, .npairs = npairs } });

  /* Cleanup the stack. */
  kit_vreg_t tmp = vreg_alloc(cc);
  for (u32 i = (KIT_REG_ARG_COUNT / 2); i < npairs; i++) {
    /* pop repeatedly into tmp */
    kit_emit_ins(cc, (kit_ins){ .pop = { .opcode = KIT_IR_OPCODE_POP, .reg = tmp } });
    kit_emit_ins(cc, (kit_ins){ .pop = { .opcode = KIT_IR_OPCODE_POP, .reg = tmp } });
  }

  return dst;
}