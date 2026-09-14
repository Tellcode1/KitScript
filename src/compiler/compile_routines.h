#ifndef KIT_COMPILER_ROUTINES_H
#define KIT_COMPILER_ROUTINES_H

#include "../../inc/kit.cc.h"
#include "../../inc/kit.ir.h"
#include "../../inc/kit.stdafx.h"

RETURNS_ERRCODE int emit_and_record_jmp(kit_compiler* cc, kit_ir_opcode opcode, kit_vreg_t condition, u32 label_id);
void                define_and_emit_label(kit_compiler* cc, u32 label_id);

RETURNS_ERRCODE int collect_struct_declerations(kit_compiler* cc, int* stmts, u32 nstmts, kitc_struct_information* deposit);

/* Return register ID of result, -1 on error */
RETURNS_ERRCODE kit_vreg_t compile_statement_list(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_and_push_literal_variable(kit_compiler* cc, const kit_var* v);
RETURNS_ERRCODE kit_vreg_t compile_literal(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_list(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_map(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_function_definition(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_binary_op(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_unary_op(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_index(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_function_call(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_if_statement(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_while_statement(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_for_statement(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_ranged_for_statement(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_member_access(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_member_assign(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_assign(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_index_assign(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_return(kit_compiler* cc, int node);
// RETURNS_ERRCODE ereg_t compile_struct_constructor(kit_compiler* fork, kit_filespan span, const kitc_struct_information* struc);
RETURNS_ERRCODE kit_vreg_t compile_struct_decleration(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_variable_decleration(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_variable_load(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_namespace_decleration(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_builtin_structure(kit_compiler* cc, const kit_builtin_struct* b);
RETURNS_ERRCODE kit_vreg_t compile_builtin_structures(kit_compiler* cc);
RETURNS_ERRCODE kit_vreg_t compile_function(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_root(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile_struct_constructor(kit_compiler* fork, kit_filespan span, const kitc_struct_information* struc);
RETURNS_ERRCODE kit_vreg_t compile_struct_fill(kit_compiler* cc, int node);
RETURNS_ERRCODE kit_vreg_t compile(kit_compiler* cc, int node);

#endif // KIT_COMPILER_ROUTINES_H