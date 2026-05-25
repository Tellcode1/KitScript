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

#include "arena.h"
#include "ast.h"
#include "cc.h"
#include "lex.h"
#include "pool.h"
#include "rwhelp.h"
#include "stdafx.h"
#include "strint.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline char*
read_file_arena(e_arena* a, FILE* f, u64* size)
{
  char* contents = nullptr;
  long  fsize    = 0;

  if ((bool)fseek(f, 0, SEEK_END)) goto CLEANUP;

  fsize = ftell(f);
  if (fsize <= 0) goto CLEANUP;

  if (size != nullptr) *size = (int)fsize;

  if ((bool)fseek(f, 0, SEEK_SET)) goto CLEANUP;

  contents = (char*)e_arnalloc(a, fsize + 1);
  if (fread(contents, fsize, 1, f) != 1) { goto CLEANUP; }
  contents[fsize] = 0;

  return contents;

CLEANUP:
  return nullptr;
}

#define print_err(...)                                                                                                                               \
  do {                                                                                                                                               \
    fputs("[ec] ", stderr);                                                                                                                          \
    fprintf(stderr, __VA_ARGS__);                                                                                                                    \
  } while (0)

int
main(int argc, char* argv[])
{
  bool                 verbose            = false;
  bool                 tokenizer_only     = false;
  bool                 ast_only           = false;
  bool                 print_memory_usage = false;
  e_ast                ast                = { 0, .root = -1 };
  e_compilation_result compiled           = { 0 };
  FILE*                f                  = NULL;
  e_str_interner       interner           = { 0 };
  e_arena              arena              = { 0 };
  int                  e                  = 0;
  int                  opt_level          = 0;

  e = e_arena_init(1, &arena);
  if (e) {
    /* TODO Add flag to reducce memory allocations? */
    print_err("Failed to initialize arena\n");
    goto ret;
  }

  if (e_refdobj_pool_init(16, &ge_pool)) goto ret;

  if (e_str_interner_init(256, &interner)) goto ret;

  char** files  = (char**)e_arnalloc(&arena, argc * sizeof(char*));
  u32    nfiles = 0;

  const char* out = NULL;
  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];
    if (strcmp(opt, "-") == 0) {
      files[nfiles++] = "-";
      continue;
    }

    if (*opt == '-') { opt++; }
    if (*opt == '-') { opt++; }

    if (strcmp(opt, "o") == 0) {
      assert(i + 1 < argc);
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
    } else {
      files[nfiles++] = e_arnstrdup(&arena, argv[i]);
    }
  }

  if (nfiles == 0) {
    print_err("No source files given\n");
    goto ret;
  }

  e = e_ast_init(&interner, &ast);
  if (e) {
    print_err("AST initialization failed\n");
    goto ret;
  }

  // Feed into the AST
  for (u32 fi = 0; fi < nfiles; fi++) {
    e_parser parser = { 0 };
    e_token* tokens = nullptr;
    u32      ntoks  = 0;

    const char* in = files[fi];

    if (verbose) fprintf(stderr, "Opening %s: ", in);

    FILE* f = NULL;

    if (strcmp(in, "-") == 0) {
      f = stdin;
    } else {
      f = fopen(in, "rb");
    }

    if (f == nullptr) {
      if (verbose) fprintf(stderr, "error!\n"); // Follow up the "Opening %s: " message.
      print_err("Failed to open %s: %s\n", in, strerror(errno));
      goto ERR;
    }

    if (verbose) fprintf(stderr, "success\n");

    char* contents = read_file_arena(&arena, f, nullptr);
    if (contents == nullptr) {
      print_err("Failed to load input file: %s. Next.\n", strerror(errno));
      goto ERR;
    }

    if (verbose) fprintf(stderr, "Tokenizing %s: ", in);

    e = e_tokenize(contents, in, &interner, &tokens, &ntoks);
    if (e) {
      if (verbose) fprintf(stderr, "error!\n"); // Follow up the "Tokenizing %s: " message.
      print_err("Failed to tokenize input string\n");
      goto ERR;
    }

    if (verbose) fprintf(stderr, "success\n");

    if (tokenizer_only) {
      for (u32 i = 0; i < ntoks; i++) {
        printf("%s", e_token_type_to_string(tokens[i].type));
        if (i != ntoks - 1) { fputs(" ", stdout); }
      }
      fputc('\n', stdout);
      goto ERR;
    }

    e = e_parser_init(tokens, ntoks, &ast, &parser);
    if (e) {
      print_err("Parser initialization failed\n");
      goto ERR;
    }

    if (verbose) fprintf(stderr, "Parsing to AST %s: ", in);

    e = e_parse(&parser);
    if (e) {
      print_err("Parsing failed\n");
      goto ERR;
    }

    if (verbose) fprintf(stderr, "success\n");

  ERR:
    if (ntoks > 0 && tokens) e_freetoks(tokens, ntoks);
    e_parser_free(&parser);
    if (f && f != stdin) fclose(f);
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
    .custom_entry_point = nullptr,
    .opt_level          = opt_level,
  };

  if (verbose) fprintf(stderr, "Compiling AST: ");

  e = e_compile(&info, &compiled);
  if (e) {
    print_err("Compilation failed\n");
    goto ret;
  }

  if (verbose) fprintf(stderr, "success\n");

  if (out) {
    f = fopen(out, "wb");
    if (!f) {
      print_err("Failed to open out file: %s\n", strerror(errno));
      goto ret;
    }
    e_file_write(&compiled, f);
    if (verbose) fprintf(stderr, "Wrote compilation result to: %s\n", out);

  } else {
    e_file_write(&compiled, stdout);
    if (verbose) fprintf(stderr, "Wrote compilation result to stdout\n");
  }

  if (print_memory_usage) {
    size_t        accumulator = 0;
    e_arena_page* next        = arena.current;
    while (next) {
      accumulator++;
      next = next->next;
    }
    fprintf(stderr, "%zu pages were allocated (%zu bytes)\n", accumulator, accumulator * (size_t)E_PAGE_SIZE);
  }

ret:
  e_ast_free(&ast);
  e_compilation_result_free(&compiled);
  e_refdobj_pool_free(&ge_pool);
  e_str_interner_free(&interner);
  e_arena_free(&arena);
  return e;
}
