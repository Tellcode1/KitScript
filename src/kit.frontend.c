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
#include "../inc/kit.lex.h"
#include "../inc/kit.pool.h"
#include "../inc/kit.rwhelp.h"
#include "../inc/kit.stdafx.h"
#include "../inc/kit.strint.h"

#include <assert.h>
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
    fputs("[ec] ", stderr);                                                                                                                          \
    fprintf(stderr, __VA_ARGS__);                                                                                                                    \
  } while (0)

int
main(int argc, char* argv[])
{
  bool                   verbose             = false;
  bool                   tokenizer_only      = false;
  bool                   ast_only            = false;
  bool                   print_memory_usage  = false;
  kit_ast                ast                 = { 0, .root = -1 };
  kit_compilation_result compiled            = { 0 };
  FILE*                  f                   = NULL;
  kit_str_interner       interner            = { 0 };
  kit_arena              arena               = { 0 };
  int                    e                   = 0;
  int                    opt_level           = 0;
  ecc_feature_set        compiler_option_set = { 0 };

  e = kit_arena_init(1, &arena);
  if (e) {
    /* TODO Add flag to reducce memory allocations? */
    print_err("Failed to initialize arena\n");
    goto ret;
  }

  if (kit_refdobj_pool_init(16, &ge_pool)) goto ret;

  if (kit_str_interner_init(256, &interner)) goto ret;

  char** files  = (char**)kit_arnalloc(&arena, argc * sizeof(char*));
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

    if (*opt == '-') { opt++; }
    if (*opt == '-') { opt++; }

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
    }

    else {
      files[nfiles++] = kit_arnstrdup(&arena, argv[i]);
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

  ecc_info info = {
    .arena              = &arena,
    .ast                = &ast,
    .root_node          = ast.root,
    .custom_entry_point = NULL,
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

  if (print_memory_usage) {
    size_t          accumulator = 0;
    kit_arena_page* next        = arena.current;
    while (next) {
      accumulator++;
      next = next->next;
    }
    fprintf(stderr, "%zu pages were allocated (%zu bytes)\n", accumulator, accumulator * (size_t)KIT_PAGE_SIZE);
  }

ret:
  kit_ast_free(&ast);
  kit_compilation_result_free(&compiled);
  kit_refdobj_pool_free(&ge_pool);
  kit_str_interner_free(&interner);
  kit_arena_free(&arena);
  return e;
}
