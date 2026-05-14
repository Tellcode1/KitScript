#include "bfunc.rt.h"

#include "arena.h"
#include "ast.h"
#include "cc.h"
#include "cerr.h"
#include "exec.h"
#include "fn.h"
#include "list.h"
#include "perr.h"
#include "stack.h"
#include "stdafx.h"
#include "strint.h"
#include "struct.h"
#include "sysexpose.h"
#include "var.h"

static inline int
find_func(const char* name, const e_compilation_result* r, e_function* out)
{
  u32 hash = e_hash(name, strlen(name));
  for (u32 i = 0; i < r->functions_count; i++) {
    if (r->functions[i].name_hash == hash) {
      *out = r->functions[i];
      return 0;
    }
  }
  return -1;
}

e_var
eb_rt_compile_and_exec(e_var* args, u32 nargs)
{
  e_var ret = E_NULLVAR;

  e_ast                ast      = { 0, .root = -1 };
  e_parser             parser   = { 0 };
  e_token*             tokens   = nullptr;
  u32                  ntoks    = 0;
  e_arena              arena    = { 0 };
  e_compilation_result compiled = { 0 };
  e_stack              stack    = { 0 };
  int                  e        = 0;

  e_str_interner interner = { 0 };
  if (e_str_interner_init(4, &interner)) goto RET;

  e = e_arena_init(1, &arena);
  if (e) {
    e_xerror("jit::exec: Failed to initialize arena\n");
    goto RET;
  }

  const int    save_argc = e_argc;
  char** const save_argv = e_argv;

#define strhash(s) (e_hash(s, strlen(s)))

  const e_struct* st = E_VAR_AS_STRUCT(&args[0]);

  const char* code               = E_VAR_AS_STRING(e_struct_get_member(strhash("source_code"), st))->s;
  const char* entry_point        = E_VAR_AS_STRING(e_struct_get_member(strhash("entry_point"), st))->s;
  int         optimization_level = e_struct_get_member(strhash("optimization_level"), st)->val.i;

  e_list* arguments     = E_VAR_AS_LIST(e_struct_get_member(strhash("arguments"), st));
  e_list* cmd_arguments = E_VAR_AS_LIST(e_struct_get_member(strhash("command_line_arguments"), st));

  e_argv = (char**)e_arnalloc(&arena, (cmd_arguments->size + 3) * sizeof(char*));

  // +2 because we have to expose the ./build/eexec FILE
  // too
  e_argc    = 2 + (int)cmd_arguments->size;
  e_argv[0] = "<JIT compiler>";
  e_argv[1] = "<JIT compiled code>";
  for (int i = 0; i < cmd_arguments->size; i++) {
    const char* cmd_arg = E_VAR_AS_STRING(&cmd_arguments->vars[i])->s;
    e_argv[2 + i]       = e_arnstrdup(&arena, cmd_arg);
  }
  e_argv[e_argc] = NULL;

  e = e_tokenize(code, "<JIT compiled>", &interner, &tokens, &ntoks);
  if (e) {
    e_xerror("jit::exec: Failed to tokenize source code\n");
    goto RET;
  }

  e = e_ast_init(&interner, &ast);
  if (e) {
    e_xerror("jit::exec: AST initialization failed\n");
    goto RET;
  }

  e = e_parser_init(tokens, ntoks, &ast, &parser);
  if (e) {
    e_xerror("jit::exec: AST initialization failed\n");
    goto RET;
  }

  e = e_parse(&parser);
  if (e) {
    e_xerror("jit::exec: AST parsing failed\n");
    goto RET;
  }

  ecc_info info = {
    .arena              = &arena,
    .ast                = &ast,
    .root_node          = ast.root,
    .custom_entry_point = nullptr,
    .opt_level          = optimization_level,
  };

  e = e_compile(&info, &compiled);
  if (e) {
    e_xerror("jit::exec: Compilation failed\n");
    goto RET;
  }

  e_function entry_func = { 0 };
  if (find_func(entry_point, &compiled, &entry_func)) {
    e_xerror("jit::exec: Entry point not in code\n");
    goto RET;
  }

  const u32 init_stack_capacity    = 32;
  const u32 init_variable_capacity = 8;
  const u32 init_frame_capacity    = 4;

  e = e_stack_init(init_stack_capacity, init_frame_capacity, init_variable_capacity, &stack);
  if (e) {
    e_xerror("jit::exec: Failed to initialize stack\n");
    goto RET;
  }

  e_exec_info exec_info = {
    .args            = NULL,
    .arg_slots       = NULL,
    .nargs           = 0,
    .literals        = compiled.literals,
    .literals_hashes = compiled.literals_hashes,
    .funcs           = compiled.functions,
    .code            = compiled.instructions,
    .code_size       = compiled.instructions_count,
    .nliterals       = compiled.literals_count,
    .nfuncs          = compiled.functions_count,
    .stack           = &stack,
    .nextern_funcs   = 0,
    .extern_funcs    = NULL,
    .names           = (const char**)compiled.names,
    .names_hashes    = compiled.names_hashes,
    .nnames          = compiled.names_count,
    .nstructs        = compiled.structs_count,
    .structs         = compiled.structs,
  };
  e_ecode err = e_exec(&exec_info, &ret); // Global variable initialization.

  exec_info.code      = entry_func.code;
  exec_info.code_size = entry_func.code_size;
  exec_info.arg_slots = entry_func.arg_slots;
  exec_info.stack     = &stack;

  exec_info.nargs = arguments->size;
  exec_info.args  = arguments->vars;

  /* Execute main function. */
  err = e_exec(&exec_info, &ret);
  if (err) { e_xerror("jit::compile_and_exec(): Function returned error: %s\n", e_ecode_str(err)); }

  e_argc = save_argc;
  e_argv = save_argv;

RET:
  if (ntoks > 0 && tokens) e_freetoks(tokens, ntoks);
  e_ast_free(&ast);
  e_compilation_result_free(&compiled);
  e_str_interner_free(&interner);
  e_arena_free(&arena);
  e_stack_free(&stack);
  return ret;
}