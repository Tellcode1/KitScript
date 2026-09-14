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

// Compiler front end

#include "../inc/kit.arena.h"
#include "../inc/kit.ast.h"
#include "../inc/kit.cc.h"
#include "../inc/kit.exec.h"
#include "../inc/kit.lex.h"
#include "../inc/kit.library.h"
#include "../inc/kit.rwhelp.h"
#include "../inc/kit.stdafx.h"
#include "../inc/kit.strint.h"

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int
create_and_cat(const char* s, size_t* to_capacity, char** to_ptr)
{
  if (!*to_ptr) {
    size_t new_cap = strlen(s) + 1;
    *to_ptr        = calloc(1, new_cap);
    *to_capacity   = new_cap;

    memset(*to_ptr, 0, new_cap);
  }
  if (!*to_ptr) return -1;

  const size_t s_len  = strlen(s);
  const size_t to_len = strlen(*to_ptr);

  const size_t new_size = (s_len + to_len + 1);

  if (new_size >= *to_capacity) {
    const size_t new_cap = MAX(new_size, *to_capacity * 2);
    char*        new_to  = realloc(*to_ptr, new_cap); // to_len+1 -> to_len+s_len+1
    if (!new_to) return -1;

    *to_ptr      = new_to;
    *to_capacity = new_cap;
  }

  kit_strlcat(*to_ptr, s, *to_capacity);

  return 0;
}

static inline char*
read_file_arena_better(FILE* f)
{
  char tmp[128];

  const size_t init_capacity = 128;
  size_t       capacity      = init_capacity;

  char* buffer = calloc(1, init_capacity);
  if (!buffer) return NULL;

  while (fgets(tmp, sizeof(tmp), f)) {
    int e = create_and_cat(tmp, &capacity, &buffer);
    if (e) {
      free(buffer);
      return NULL;
    }
  }

  return buffer;
}

#define print_err(...)                                                                                                                               \
  do {                                                                                                                                               \
    fputs("[KitC] ", stderr);                                                                                                                        \
    fprintf(stderr, __VA_ARGS__);                                                                                                                    \
  } while (0)

const char* help_str = "KitC: The compiler for the Kit scripting language\n"
                       "";

static int
compile_to_obj(bool dont_write_file, int argc, char* argv[], kit_arena* arena, kit_compilation_result* result)
{
  bool                   verbose             = false;
  bool                   tokenizer_only      = false;
  bool                   ast_only            = false;
  bool                   print_memory_usage  = false;
  bool                   dump_asm            = false;
  kit_ast                ast                 = { 0, .root = -1 };
  kit_compilation_result compiled            = { 0 };
  FILE*                  f                   = NULL;
  kit_str_interner       interner            = { 0 };
  int                    e                   = 0;
  int                    opt_level           = 0;
  kitc_feature_set       compiler_option_set = { 0 };

  if (kit_str_interner_init(256, &interner)) goto ret;

  char** files  = (char**)kit_arnalloc(arena, argc * sizeof(char*));
  u32    nfiles = 0;

  /* set every option disable to false */
  memset(&compiler_option_set, 0, sizeof(compiler_option_set));

  const char* out = NULL;
  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];
    if (strcmp(opt, "-") == 0) {
      files[nfiles++] = "-"; // "read from stdin"
      continue;
    }

    bool isnt_option = true;
    if (*opt == '-') {
      isnt_option = false;
      opt++;
    }
    if (*opt == '-') {
      isnt_option = false;
      opt++;
    }

    if (strcmp(opt, "o") == 0) {
      if (i + 1 >= argc) {
        print_err("Expected output file name, got EOF. Assuming stdout\n");
        out = NULL;
        continue;
      }
      out = argv[i + 1];
      i++; // consumed path!
    } else if (strcmp(opt, "v") == 0 || strcmp(opt, "verbose") == 0) {
      verbose = true;
    } else if (*opt == 'O') {
      opt++;
      opt_level = *opt - '0';
      if (opt_level < 0 || opt_level > 3) {
        print_err("Expected optimization level to be between 0 and 3. Assuming 0\n");
        opt_level = 0;
      }
    } else if (strcmp(opt, "tokens") == 0) {
      tokenizer_only = true;
    } else if (strcmp(opt, "ast") == 0) {
      ast_only = true;
    } else if (strcmp(opt, "mem") == 0) {
      print_memory_usage = true;
    } else if (strcmp(opt, "dump") == 0 || strcmp(opt, "dump-asm") == 0) {
      dump_asm = true;
    }

    /* compiler option flags */
    else if (strcmp(opt, "no-noop-strip") == 0) {
      compiler_option_set.disable_noop_stripping = true;
    } else if (strcmp(opt, "no-dead-branch-elimination") == 0 || strcmp(opt, "no-DBE") == 0) {
      compiler_option_set.disable_dead_branch_elimination = true;
    } else if (strcmp(opt, "no-constant-propagation") == 0) {
      compiler_option_set.disable_constant_propagation = true;
    } else if (strcmp(opt, "no-constant-folding") == 0) {
      compiler_option_set.disable_constant_folding = true;
    } else if (strcmp(opt, "no-local-copy-propagation") == 0) {
      compiler_option_set.disable_local_copy_propagation = true;
    } else if (strcmp(opt, "no-redundant-move-elimination") == 0) {
      compiler_option_set.disable_redundant_move_elimination = true;
    } else if (strcmp(opt, "no-inline") == 0) {
      compiler_option_set.disable_function_inlining = true;
    } else if (strcmp(opt, "no-dead-move-forwarding") == 0) {
      compiler_option_set.disable_dead_move_forwarding = true;
    } else if (strcmp(opt, "no-dead-store-elimination") == 0 || strcmp(opt, "no-DSE") == 0) {
      compiler_option_set.disable_dead_store_elimination = true;
    } else if (strcmp(opt, "no-redundant-jump-elimination") == 0) {
      compiler_option_set.disable_redundant_jump_elimination = true;
    } else if (strcmp(opt, "no-register-allocation-i-know-what-im-doing") == 0) {
      compiler_option_set.disable_register_allocation_i_know_what_im_doing = true;
    }

    /* providing library, we have no use for that here */
    else if (strcmp(opt, "lib") == 0 || strcmp(opt, "l") == 0) {
      i++;
      continue;
    }

    else if (isnt_option) {
      files[nfiles++] = kit_arnstrdup(arena, argv[i]);
    }
  }

  if (nfiles == 0) {
    print_err("No source files given\n");
    goto ret;
  }

  e = kit_ast_init(&interner, &ast);
  if (e) {
    print_err("AST initialization failed\n");
    goto ret;
  }

  // Feed into the AST
  for (u32 fi = 0; fi < nfiles; fi++) {
    kit_parser parser   = { 0 };
    kit_token* tokens   = NULL;
    u32        ntoks    = 0;
    char*      contents = NULL;

    const char* in = files[fi];

    if (verbose) fprintf(stderr, "Opening %s: ", in);

    f = NULL;

    if (strcmp(in, "-") == 0) {
      f = stdin;
    } else {
      f = fopen(in, "rb");
    }

    if (f == NULL) {
      if (verbose) fprintf(stderr, "error!\n"); // Follow up the "Opening %s: " message.
      print_err("Failed to open %s: %s\n", in, strerror(errno));
      e = -1;
      goto ERR;
    }

    if (verbose) fprintf(stderr, "success\n");

    contents = read_file_arena_better(f);
    if (contents == NULL) {
      print_err("Failed to load input file: %s. Next.\n", strerror(errno));
      e = -1;
      goto ERR;
    }

    if (verbose) fprintf(stderr, "Tokenizing %s: ", in);

    const char* expose_file = in;
    if (strcmp(in, "-") == 0) { expose_file = "stdin"; }

    e = kit_tokenize(contents, expose_file, &interner, &tokens, &ntoks);
    if (e) {
      if (verbose) fprintf(stderr, "error!\n"); // Follow up the "Tokenizing %s: " message.
      print_err("Failed to tokenize input string\n");
      goto ERR;
    }

    if (verbose) fprintf(stderr, "success\n");

    if (tokenizer_only) {
      for (u32 i = 0; i < ntoks; i++) {
        printf("%s", kit_token_type_to_string(tokens[i].type));
        if (i != ntoks - 1) { fputs(" ", stdout); }
      }
      fputc('\n', stdout);
      goto ERR;
    }

    e = kit_parser_init(tokens, ntoks, &ast, &parser);
    if (e) {
      print_err("Parser initialization failed\n");
      goto ERR;
    }

    if (verbose) fprintf(stderr, "Parsing to AST %s: ", in);

    e = kit_parse(&parser);
    if (e) {
      print_err("Parsing failed\n");
      goto ERR;
    }

    if (verbose) fprintf(stderr, "success\n");

    if (tokens) kit_freetoks(tokens, ntoks);
    kit_parser_free(&parser);
    if (f) fclose(f); // clang-tidy complains that even if f is stdin, we need to fclose it?
    free(contents);
    continue;
  ERR:
    if (tokens) kit_freetoks(tokens, ntoks);
    kit_parser_free(&parser);
    if (f) fclose(f);
    free(contents);
    goto ret;
  }

  if (ast_only) {
    // TODO: Add AST printing / visualization
    return 0;
  }

  kitc_info info = {
    .arena              = arena,
    .ast                = &ast,
    .root_node          = ast.root,
    .custom_entry_point = NULL,
    .dump_assembly      = dump_asm,
    .opt_level          = opt_level,
    .feature_set        = compiler_option_set,
  };

  if (verbose) fprintf(stderr, "Compiling AST: ");

  e = kit_compile(&info, &compiled);
  if (e) {
    print_err("Compilation failed\n");
    goto ret;
  }

  if (verbose) fprintf(stderr, "success\n");

  if (!dump_asm && !dont_write_file) {
    /* only print result if we didn't dump the assembly */

    if (out && strcmp(out, "-") != 0) { // out is valid and out is not stdout
      f = fopen(out, "wb");
      if (!f) {
        print_err("Failed to open out file: %s\n", strerror(errno));
        goto ret;
      }
      kit_file_write(&compiled, f);
      if (verbose) fprintf(stderr, "Wrote compilation result to: %s\n", out);
      fclose(f);
      f = NULL;
    } else {
      kit_file_write(&compiled, stdout);
      if (verbose) fprintf(stderr, "Wrote compilation result to stdout\n");
    }
  }

  if (print_memory_usage) {
    size_t          accumulator = 0;
    kit_arena_page* next        = arena->current;
    while (next) {
      accumulator++;
      next = next->next;
    }
    fprintf(stderr, "%zu pages were allocated (%zu bytes)\n", accumulator, accumulator * (size_t)KIT_PAGE_SIZE);
  }

ret:
  *result = compiled;
  kit_ast_free(&ast);
  kit_str_interner_free(&interner);
  return e;
}

#undef print_err
#define print_err(...)                                                                                                                               \
  do {                                                                                                                                               \
    fputs("[KitExec] ", stderr);                                                                                                                     \
    fprintf(stderr, __VA_ARGS__);                                                                                                                    \
  } while (0)

static inline int
find_func(const char* name, u32 nfuncs, const kitc_function* funcs, kitc_function* out)
{
  u32 hash = kit_hash(name, strlen(name));
  for (u32 i = 0; i < nfuncs; i++) {
    if (funcs[i].name_hash == hash) {
      *out = funcs[i];
      return 0;
    }
  }
  return -1;
}

static inline int
find_and_load_file(int argc, char* argv[], kit_compilation_result* r)
{
  FILE* f              = NULL;
  int   e              = 0;
  bool  run_from_stdin = false;

  const char* file = NULL;
  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];
    if (strcmp(opt, "-") == 0) {
      run_from_stdin = true;
      continue;
    }

    bool isnt_file = true;
    if (*opt == '-') {
      isnt_file = false;
      opt++;
    }
    if (*opt == '-') {
      isnt_file = false;
      opt++;
    }

    if (strcmp(opt, "lib") == 0) {
      /* skip library option and its name */
      i++;
      continue;
    }

    if (!file && isnt_file) { // interpret first non option string as file
      file = argv[i];
    }
  }

  if (run_from_stdin) {
    f = stdin;
  } else if (file != NULL) {
    f = fopen(file, "rb");
    if (!f) {
      print_err("Failed to open file: %s\n", strerror(errno));
      e = -1;
      goto RET;
    }
  }
  if (!f) {
    print_err("Nothing given to execute\n");
    e = -1;
    goto RET;
  }

  e = kit_file_load(r, f);
  if (e) {
    print_err("Failed to parse input file: 0x%x\n", e);
    goto RET;
  }

RET:
  return e;
}

static const char* const help = "kitexec: [-e/r] [--entry ENTRYPOINT] <FILE/->\n"
                                "The following options can be used:\n"
                                "-e/error Interpret integral return values from the script as error, and exit with the error code\n"
                                "-r/return Print return value\n"
                                "-entry [FUNCTION_NAME] Call the specified entry point instead of \'main\'\n"
                                "-h/help Print this help message and exit\n"
                                "- If the - switch is used, program will be read from stdin";

static int
load_library(const char* path, kit_builtin_func** out_funcs, u32* out_count)
{
  void* handle = dlopen(path, RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "dlopen: %s\n", dlerror());
    return -1;
  };

  kit_library_entry_point_fn entry = (kit_library_entry_point_fn)dlsym(handle, "kit_library_entry_point");
  if (!entry) {
    fprintf(stderr, "dlsym: %s\n", dlerror());
    return -1;
  }

  const kit_library_info* mod = entry();

  kit_builtin_func* funcs = kit_xalloc(mod->nexports, sizeof(kit_builtin_func));
  for (u32 i = 0; i < mod->nexports; i++) {
    funcs[i].name = mod->exports[i].name;
    funcs[i].func = mod->exports[i].funcp;
  }

  *out_funcs = funcs;
  *out_count = mod->nexports;
  return 0;
}

static int
execute_obj(kit_compilation_result* obj, int argc, char* argv[])
{
  // assert(argc == 2);
  kit_var v = { .type = KIT_VARTYPE_NULL };

  kit_var gvars[128] = { 0 };
  for (u32 i = 0; i < KIT_ARRLEN(gvars); i++) { gvars[i] = KIT_NULLVAR; };

  const char* entry_point = "main";

  bool wants_to_print_return_value     = false;
  bool interpret_return_value_as_error = false;
  bool run_from_stdin                  = false;

  kit_var time_now    = KIT_NULLVAR;
  kit_var time_as_str = KIT_NULLVAR;

  kit_refdobj_pool object_pool = { 0 };

  kit_builtin_func* library_funcs      = NULL;
  u32               library_func_count = 0;

  int e = 0;

  const char* file = NULL;
  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];
    if (strcmp(opt, "-") == 0) {
      run_from_stdin = true;
      continue;
    }

    bool isnt_file = true;
    if (*opt == '-') {
      isnt_file = false;
      opt++;
    }
    if (*opt == '-') {
      isnt_file = false;
      opt++;
    }

    if (strcmp(opt, "entry") == 0 && i + 1 < argc) {
      entry_point = argv[i + 1];
      i++;
    } else if (strcmp(opt, "h") == 0 || strcmp(opt, "help") == 0) {
      fputs(help, stdout);
      return 0;
    } else if (strcmp(opt, "r") == 0 || strcmp(opt, "return") == 0) {
      wants_to_print_return_value = true;
    } else if (strcmp(opt, "e") == 0 || strcmp(opt, "error") == 0) {
      interpret_return_value_as_error = true;
    }

    else if (strcmp(opt, "lib") == 0) {
      if (i + 1 >= argc || !argv[i + 1]) {
        print_err("Expected library name after -lib/-l\n");
        goto RET;
      }

      i++;

      kit_builtin_func* tmp_lib_funcs      = NULL;
      u32               tmp_lib_func_count = 0;

      if (load_library(argv[i], &tmp_lib_funcs, &tmp_lib_func_count)) {
        print_err("Failed to load library: %s\n", argv[i]);
        goto RET;
      }

      /* merge */
      kit_builtin_func* new_lib_funcs = realloc(library_funcs, sizeof(kit_builtin_func) * (library_func_count + tmp_lib_func_count));
      memcpy(new_lib_funcs + library_func_count, tmp_lib_funcs, sizeof(kit_builtin_func) * tmp_lib_func_count);

      free(tmp_lib_funcs);

      library_func_count += tmp_lib_func_count;
      // fprintf(stderr, "Loaded %s (%u functions)\n", argv[i], builtin_func_count);
    }
  }

  kitc_function entry_point_func;
  e = find_func(entry_point, obj->functions_count, obj->functions, &entry_point_func);
  if (e) {
    print_err("File does not have the entry point '%s'\n", entry_point);
    goto RET;
  }

  /* Maybe add a way to pass integers / strings from the command line as arguments? */
  if (entry_point_func.nargs != 0) {
    print_err("Entry point takes non zero arguments, can not give any\n");
    e = -1;
    goto RET;
  }

  const u32 init_branches = 8;

  e = kit_refdobj_pool_init(init_branches, &object_pool);
  if (e) goto RET;

  kit_exec_info info = {
    .gvars           = gvars,
    .args            = NULL,
    .nargs           = 0,
    .literals        = obj->literals,
    .literals_hashes = obj->literals_hashes,
    .funcs           = obj->functions,
    .code            = obj->instructions,
    .code_count      = obj->instructions_count,
    .nliterals       = obj->literals_count,
    .nfuncs          = obj->functions_count,
    .names           = (const char**)obj->names,
    .names_hashes    = obj->names_hashes,
    .nnames          = obj->names_count,
    .structs         = obj->structs,
    .nstructs        = obj->structs_count,

    .extern_funcs  = library_funcs,
    .nextern_funcs = library_func_count,
  };

  kit_vm vm = { 0 };
  kit_vm_init(argc, (const char**)argv, &object_pool, entry_point_func.name_hash, &vm);

  /* Initialize PRNG subroutine */
  kit_builtins_time_now(&vm, NULL, 0, &time_now);
  kit_builtins_cast_string(&vm, &time_now, 1, &time_as_str);

  kit_var discard;
  kit_builtins_rand_seed(&vm, &time_as_str, 1, &discard);

  // e = kit_stack_push_frame(&stack);
  // if (e) return e;

  /**
   * Global variable initialization
   */
  e = kit_exec(&vm, &info, &v);
  if (e) {
    print_err("Program initialization failed: %s\n", kit_ecode_str(e));
    goto RET;
  }

  // e = kit_stack_push_frame(&stack);
  // if (e) return e;

  info.code       = entry_point_func.code;
  info.code_count = entry_point_func.code_count;
  info.nargs      = 0;

  vm.curr_func_hash  = entry_point_func.name_hash;
  vm.instruction_idx = 0;

  /* Execute main function. */
  e = kit_exec(&vm, &info, &v);
  if (e) {
    print_err("Entry point execution failed: %s\n", kit_ecode_str(e));
    goto RET;
  }

  // kit_stack_pop_frame(&stack);
  // kit_stack_pop_frame(&stack);

  if (wants_to_print_return_value) { kit_builtins_println(NULL, &v, 1, &discard); }

  if (interpret_return_value_as_error) {
    if (v.type == KIT_VARTYPE_INT) e = v.val.i;
    else if (v.type == KIT_VARTYPE_NULL) e = 0;
    else e = -1;
  }

RET:
  kit_var_free(&object_pool, &time_as_str);
  kit_var_free(&object_pool, &time_now);

  for (u32 i = 0; i < KIT_ARRLEN(gvars); i++) { kit_var_free(&object_pool, &gvars[i]); }

  kit_var_release(&object_pool, &v);
  kit_refdobj_pool_free(&object_pool);

  kit_vm_free(&vm);

  return e;
}

int
main(int argc, char* argv[])
{
  kit_arena arena = { 0 };
  int       e     = kit_arena_init(1, &arena);
  if (e) {
    print_err("Failed to initialize arena\n");
    goto RET;
  }

  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];

    if (*opt == '-') opt++;
    if (*opt == '-') opt++;

    if (strcmp(opt, "c") == 0 || strcmp(opt, "compile") == 0) {
      kit_compilation_result result = { 0 };

      /* Disable don't write to file since we need to output the program in this path */
      e = compile_to_obj(false, argc, argv, &arena, &result);
      if (e) { goto RET; }

      kit_compilation_result_free(&result);
      goto RET;
    }
    if (strcmp(opt, "e") == 0 || strcmp(opt, "exec") == 0) {
      kit_compilation_result obj = { 0 };
      if (find_and_load_file(argc, argv, &obj)) { goto RET; }

      int e = execute_obj(&obj, argc, argv);

      kit_compilation_result_free(&obj);

      goto RET;
    }
    if (strcmp(opt, "cx") == 0 || strcmp(opt, "compile-and-exec") == 0) {
      kit_compilation_result obj = { 0 };

      /* Don't write to file since we're executing the program directly */
      int e = compile_to_obj(true, argc, argv, &arena, &obj);
      if (e) { goto RET; }

      e = execute_obj(&obj, argc, argv);

      kit_compilation_result_free(&obj);

      goto RET;
    }
  }

  fprintf(stderr, "KScript: Nothing to compile (-c/--compile) nor execute (-e/--exec)\n");

RET:
  kit_arena_free(&arena);
  return e;
}