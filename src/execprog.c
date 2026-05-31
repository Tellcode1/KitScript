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

#include "../inc/bfunc.h"
#include "../inc/bfunc.time.h"
#include "../inc/cc.h"
#include "../inc/exec.h"
#include "../inc/perr.h"
#include "../inc/pool.h"
#include "../inc/rwhelp.h"
#include "../inc/stdafx.h"
#include "../inc/sysexpose.h"
#include "../inc/var.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define print_err(...)                                                                                                                               \
  do {                                                                                                                                               \
    fputs("[eexec] ", stderr);                                                                                                                       \
    fprintf(stderr, __VA_ARGS__);                                                                                                                    \
  } while (0)

static inline int
find_func(const char* name, u32 nfuncs, const ecc_function* funcs, ecc_function* out)
{
  u32 hash = e_hash(name, strlen(name));
  for (u32 i = 0; i < nfuncs; i++) {
    if (funcs[i].name_hash == hash) {
      *out = funcs[i];
      return 0;
    }
  }
  return -1;
}

static const char* const help = "eexec: [-e/r] [--entry ENTRYPOINT] <FILE/->\n"
                                "The following options can be used:\n"
                                "-e/error Interpret integral return values from the script as error, and exit with the error code\n"
                                "-r/return Print return value\n"
                                "-entry [FUNCTION_NAME] Call the specified entry point instead of \'main\'\n"
                                "-h/help Print this help message and exit\n"
                                "- If the - switch is used, program will be read from stdin";

int
main(int argc, char* argv[])
{
  // assert(argc == 2);
  e_var v = { .type = E_VARTYPE_NULL };

  const char* entry_point = "main";

  bool wants_to_print_return_value     = false;
  bool interpret_return_value_as_error = false;
  bool run_from_stdin                  = false;

  FILE* f = NULL;

  void*                root_allocation = NULL;
  e_compilation_result r               = { 0 };

  e_var time_now    = E_NULLVAR;
  e_var time_as_str = E_NULLVAR;

  int e = 0;

  e_argv = argv;
  e_argc = argc;

  const char* file = NULL;
  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];
    if (strcmp(opt, "-") == 0) {
      run_from_stdin = true;
      continue;
    }

    if (*opt == '-') opt++;
    if (*opt == '-') opt++;

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
    } else if (!file) { // interpret first non option string as file
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

  e = e_file_load(&r, &root_allocation, f);
  if (e) {
    print_err("Failed to parse input file: 0x%x\n", e);
    goto RET;
  }

  ecc_function entry_point_func;
  e = find_func(entry_point, r.functions_count, r.functions, &entry_point_func);
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

  e = e_refdobj_pool_init(init_branches, &ge_pool);
  if (e) goto RET;

  e_var gvars[128];

  e_exec_info info = {
    .gvars           = gvars,
    .args            = NULL,
    .nargs           = 0,
    .literals        = r.literals,
    .literals_hashes = r.literals_hashes,
    .funcs           = r.functions,
    .code            = r.instructions,
    .code_count      = r.instructions_count,
    .nliterals       = r.literals_count,
    .nfuncs          = r.functions_count,
    .nextern_funcs   = 0,
    .extern_funcs    = NULL,
    .names           = (const char**)r.names,
    .names_hashes    = r.names_hashes,
    .nnames          = r.names_count,
    .structs         = r.structs,
    .nstructs        = r.structs_count,
  };

  /* Initialize PRNG subroutine */
  time_now    = eb_time_now(NULL, 0);
  time_as_str = eb_cast_string(&time_now, 1);

  eb_rand_seed(&time_as_str, 1);

  // e = e_stack_push_frame(&stack);
  // if (e) return e;

  /**
   * Global variable initialization
   */
  e = e_exec(&info, &v);
  if (e) {
    print_err("Program initialization failed: %s\n", e_ecode_str(e));
    goto RET;
  }

  // e = e_stack_push_frame(&stack);
  // if (e) return e;

  info.code       = entry_point_func.code;
  info.code_count = entry_point_func.code_count;
  info.nargs      = 0;

  /* Execute main function. */
  e = e_exec(&info, &v);
  if (e) {
    print_err("Entry point execution failed: %s\n", e_ecode_str(e));
    goto RET;
  }

  // e_stack_pop_frame(&stack);
  // e_stack_pop_frame(&stack);

  if (wants_to_print_return_value) { eb_println(&v, 1); }

  if (interpret_return_value_as_error) {
    if (v.type == E_VARTYPE_INT) e = v.val.i;
    else if (v.type == E_VARTYPE_NULL) e = 0;
    else e = -1;
  }

RET:
  e_var_release(&time_as_str);
  e_var_release(&time_now);

  if (!run_from_stdin && f) fclose(f);
  e_var_release(&v);
  e_refdobj_pool_free(&ge_pool);

  /* No need to free anything else */
  free(root_allocation);

  return e;
}