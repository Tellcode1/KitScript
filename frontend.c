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
#include "opt.h"
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
read_file_arena(e_arena* a, const char* path, u64* size)
{
  FILE* f        = nullptr;
  char* contents = nullptr;
  long  fsize    = 0;

  f = fopen(path, "rb");
  if (f == nullptr) return nullptr;

  if ((bool)fseek(f, 0, SEEK_END)) goto CLEANUP;

  fsize = ftell(f);
  if (fsize <= 0) goto CLEANUP;

  if (size != nullptr) *size = (int)fsize;

  if ((bool)fseek(f, 0, SEEK_SET)) goto CLEANUP;

  contents = (char*)e_arnalloc(a, fsize + 1);
  if (fread(contents, fsize, 1, f) != 1) { goto CLEANUP; }
  contents[fsize] = 0;

  fclose(f);

  return contents;

CLEANUP:
  if (f != nullptr) fclose(f);
  return nullptr;
}

int
main(int argc, char* argv[])
{
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
    fprintf(stderr, "ec: Failed to initialize arena\n");
    goto ret;
  }

  if (e_refdobj_pool_init(16, &ge_pool)) goto ret;

  if (e_str_interner_init(256, &interner)) goto ret;

  char** files  = (char**)e_arnalloc(&arena, argc * sizeof(char*));
  u32    nfiles = 0;

  const char* out = NULL;
  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];

    if (*opt == '-') { opt++; }
    if (*opt == '-') { opt++; }

    if (strcmp(opt, "o") == 0) {
      assert(i + 1 < argc);
      out = argv[i + 1];
      i++; // consumed path!
    } else if (*opt == 'O') {
      opt++;
      opt_level = *opt - '0';
      if (opt_level < 0 || opt_level > 3) {
        fprintf(stderr, "Expected optimization level to be between 0 and 3. Assuming 0\n");
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

  e = e_ast_init(&interner, &ast);
  if (e) {
    fprintf(stderr, "ec: AST initialization failed\n");
    goto ret;
  }

  // Feed into the AST
  for (u32 fi = 0; fi < nfiles; fi++) {
    e_parser parser = { 0 };
    e_token* tokens = nullptr;
    u32      ntoks  = 0;

    const char* in = files[fi];

    char* contents = read_file_arena(&arena, in, nullptr);
    if (contents == nullptr) {
      fprintf(stderr, "ec: Failed to load input file: %s\n", strerror(errno));
      goto next;
    }

    e = e_tokenize(contents, in, &interner, &tokens, &ntoks);
    if (e) {
      fprintf(stderr, "ec: Failed to tokenize input string\n");
      goto next;
    }

    if (tokenizer_only) {
      for (u32 i = 0; i < ntoks; i++) {
        printf("%s", e_token_type_to_string(tokens[i].type));
        if (i != ntoks - 1) { fputs(" ", stdout); }
      }
      fputc('\n', stdout);
      goto next;
    }

    e = e_parser_init(tokens, ntoks, &ast, &parser);
    if (e) {
      fprintf(stderr, "ec: Parser initialization failed\n");
      goto next;
    }

    e = e_parse(&parser);
    if (e) {
      fprintf(stderr, "ec: Parsing failed\n");
      goto next;
    }

    eopt_data ast_opt_data = { .ast = &ast };
    for (u32 i = 0; i < E_ARRLEN(eopt_passes); i++) {
      const eopt_pass* pass = &eopt_passes[i];
      if (pass->stage == EOPT_STAGE_AST && opt_level >= pass->minimum_opt_level) {
        e = pass->fp(EOPT_STAGE_AST, &ast_opt_data);
        if (e) {
          fprintf(stderr, "ec: Optimization pass failed\n");
          goto loop_err;
        }
      }
    }

  next:
    if (ntoks > 0 && tokens) e_freetoks(tokens, ntoks);
    e_parser_free(&parser);
    continue;
  loop_err:
    if (ntoks > 0 && tokens) e_freetoks(tokens, ntoks);
    e_parser_free(&parser);
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

  e = e_compile(&info, &compiled);
  if (e) {
    fprintf(stderr, "ec: Compilation failed\n");
    goto ret;
  }

  if (out) {
    f = fopen(out, "wb");
    if (!f) {
      perror("Failed to open out file");
      goto ret;
    }
    e_file_write(&compiled, f);
  } else {
    e_file_write(&compiled, stdout);
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
  if (f) { fclose(f); }
  e_str_interner_free(&interner);
  e_arena_free(&arena);
  return e;
}