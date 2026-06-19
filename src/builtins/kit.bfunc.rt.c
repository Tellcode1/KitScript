#include "../../inc/kit.bfunc.rt.h"

#include "../../inc/kit.arena.h"
#include "../../inc/kit.ast.h"
#include "../../inc/kit.cc.h"
#include "../../inc/kit.cerr.h"
#include "../../inc/kit.exec.h"
#include "../../inc/kit.list.h"
#include "../../inc/kit.perr.h"
#include "../../inc/kit.stdafx.h"
#include "../../inc/kit.strint.h"
#include "../../inc/kit.struct.h"
#include "../../inc/kit.sysexpose.h"
#include "../../inc/kit.var.h"

static inline int
find_func(const char* name, const kit_compilation_result* r, kitc_function* out)
{
  u32 hash = kit_hash(name, strlen(name));
  for (u32 i = 0; i < r->functions_count; i++) {
    if (r->functions[i].name_hash == hash) {
      *out = r->functions[i];
      return 0;
    }
  }
  return -1;
}

kit_ecode
kit_builtins_rt_compile_and_exec(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  kit_var ret = KIT_NULLVAR;

  kit_vm fork_vm = { 0 };

  kit_ast                ast      = { 0, .root = -1 };
  kit_parser             parser   = { 0 };
  kit_token*             tokens   = NULL;
  u32                    ntoks    = 0;
  kit_arena              arena    = { 0 };
  kit_compilation_result compiled = { 0 };
  int                    e        = 0;

  kit_str_interner interner = { 0 };
  if (kit_str_interner_init(4, &interner)) goto RET;

  e = kit_arena_init(1, &arena);
  if (e) {
    kit_xerror("kit::exec: Failed to initialize arena\n");
    goto RET;
  }

  const int    save_argc = kit_argc;
  char** const save_argv = kit_argv;

#define strhash(s) (kit_hash(s, strlen(s)))

  const kit_struct* st = KIT_VAR_AS_STRUCT(&args[0]);

  const char* code               = KIT_VAR_AS_STRING(kit_struct_get_member(strhash("source_code"), st))->s;
  const char* entry_point        = KIT_VAR_AS_STRING(kit_struct_get_member(strhash("entry_point"), st))->s;
  int         optimization_level = kit_struct_get_member(strhash("optimization_level"), st)->val.i;

  kit_list* arguments     = KIT_VAR_AS_LIST(kit_struct_get_member(strhash("arguments"), st));
  kit_list* cmd_arguments = KIT_VAR_AS_LIST(kit_struct_get_member(strhash("command_line_arguments"), st));

  kit_argv = (char**)kit_arnalloc(&arena, (cmd_arguments->size + 3) * sizeof(char*));

  // +2 because we have to expose the ./build/eexec FILE
  // too
  kit_argc    = 2 + (int)cmd_arguments->size;
  kit_argv[0] = "<RT compiler>";
  kit_argv[1] = "<RT compiled code>";
  for (int i = 0; i < cmd_arguments->size; i++) {
    const char* cmd_arg = KIT_VAR_AS_STRING(&cmd_arguments->vars[i])->s;
    kit_argv[2 + i]     = kit_arnstrdup(&arena, cmd_arg);
  }
  kit_argv[kit_argc] = NULL;

  e = kit_tokenize(code, "<RT compiled>", &interner, &tokens, &ntoks);
  if (e) {
    kit_xerror("kit::exec: Failed to tokenize source code\n");
    goto RET;
  }

  e = kit_ast_init(&interner, &ast);
  if (e) {
    kit_xerror("kit::exec: AST initialization failed\n");
    goto RET;
  }

  e = kit_parser_init(tokens, ntoks, &ast, &parser);
  if (e) {
    kit_xerror("kit::exec: AST initialization failed\n");
    goto RET;
  }

  e = kit_parse(&parser);
  if (e) {
    kit_xerror("kit::exec: AST parsing failed\n");
    goto RET;
  }

  kitc_feature_set fset = { 0 };

  kitc_info info = {
    .arena              = &arena,
    .ast                = &ast,
    .root_node          = ast.root,
    .custom_entry_point = NULL,
    .opt_level          = optimization_level,
    .feature_set        = fset,
  };

  e = kit_compile(&info, &compiled);
  if (e) {
    kit_xerror("kit::exec: Compilation failed\n");
    goto RET;
  }

  kitc_function entry_func = { 0 };
  if (find_func(entry_point, &compiled, &entry_func)) {
    kit_xerror("kit::exec: Entry point not in code\n");
    goto RET;
  }

  e = kit_vm_fork(entry_func.name_hash, vm, &fork_vm);
  if (e) {
    kit_xerror("kit::exec: VM Fork failed\n");
    goto RET;
  }

  kit_exec_info exec_info = {
    .args            = NULL,
    .nargs           = 0,
    .literals        = compiled.literals,
    .literals_hashes = compiled.literals_hashes,
    .funcs           = compiled.functions,
    .code            = compiled.instructions,
    .code_count      = compiled.instructions_count,
    .nliterals       = compiled.literals_count,
    .nfuncs          = compiled.functions_count,
    .nextern_funcs   = 0,
    .extern_funcs    = NULL,
    .names           = (const char**)compiled.names,
    .names_hashes    = compiled.names_hashes,
    .nnames          = compiled.names_count,
    .nstructs        = compiled.structs_count,
    .structs         = compiled.structs,
  };
  kit_ecode err = kit_exec(vm, &exec_info, &ret); // Global variable initialization.

  exec_info.code       = entry_func.code;
  exec_info.code_count = entry_func.code_count;

  exec_info.nargs = arguments->size;
  exec_info.args  = arguments->vars;

  /* Execute main function. */
  err = kit_exec(vm, &exec_info, &ret);
  if (err) { kit_xerror("kit::compile_and_exec(): Function returned error: %s\n", kit_ecode_str(err)); }

  kit_argc = save_argc;
  kit_argv = save_argv;

RET:
  if (ntoks > 0 && tokens) kit_freetoks(tokens, ntoks);
  kit_ast_free(&ast);
  kit_compilation_result_free(&compiled);
  kit_str_interner_free(&interner);
  kit_arena_free(&arena);
  kit_vm_free(&fork_vm);
  // kit_stack_free(&stack);
  *result = ret;
  return KIT_OK;
}

kit_ecode
kit_builtins_rt_tokenize(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_rt_ast_init(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_rt_ast_free(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_rt_parse(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = KIT_NULLVAR;
  return KIT_OK;
}

kit_ecode
kit_builtins_rt_compile(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  *result = KIT_NULLVAR;
  return KIT_OK;
}
