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
#include "strint.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char* argv[])
{
  bool                 tokenizer_only     = false;
  bool                 ast_only           = false;
  bool                 print_memory_usage = false;
  int                  root               = -1;
  e_ast                ast                = { 0, .root = -1 };
  e_token*             tokens             = nullptr;
  u32                  ntoks              = 0;
  e_compilation_result compiled           = { 0 };
  FILE*                f                  = NULL;
  char*                contents           = NULL;
  e_str_interner       interner           = { 0 };
  e_arena              arena              = { 0 };
  int                  e                  = 0;

  if (e_refdobj_pool_init(16, &ge_pool)) goto ret;

  if (e_str_interner_init(256, &interner)) goto ret;

  const char* out = NULL;
  const char* in  = NULL;
  for (int i = 1; i < argc; i++) {
    const char* opt = argv[i];

    if (*opt == '-') { opt++; }
    if (*opt == '-') { opt++; }

    if (strcmp(opt, "o") == 0) {
      assert(i + 1 < argc);
      out = argv[i + 1];
      i++; // consumed path!
    } else if (strcmp(opt, "tokens") == 0) {
      tokenizer_only = true;
    } else if (strcmp(opt, "ast") == 0) {
      ast_only = true;
    } else if (strcmp(opt, "mem") == 0) {
      print_memory_usage = true;
    } else {
      in = argv[i];
    }
  }

  contents = read_file(in, nullptr);
  if (contents == nullptr) {
    fprintf(stderr, "ec: Failed to load input file: %s\n", strerror(errno));
    goto ret;
  }

  e = e_arena_init(1, &arena);
  if (e) {
    /* TODO Add flag to reducce memory allocations? */
    fprintf(stderr, "ec: Failed to initialize arena\n");
    goto ret;
  }

  e = e_tokenize(contents, in, &interner, &tokens, &ntoks);
  if (e) {
    fprintf(stderr, "ec: Failed to tokenize input string\n");
    goto ret;
  }

  if (tokenizer_only) {
    for (u32 i = 0; i < ntoks; i++) {
      printf("%s", e_token_type_to_string(tokens[i].type));
      if (i != ntoks - 1) { fputs(" ", stdout); }
    }
    fputc('\n', stdout);
  }

  e = e_ast_init(tokens, ntoks, &interner, &ast);
  if (e) {
    fprintf(stderr, "ec: AST initialization failed\n");
    goto ret;
  }

  // for (u32 i = 0; i < ntoks; i++) { printf("[%s:%i:%i]\n", tokens[i].span.file, tokens[i].span.line, tokens[i].span.col); }

  e = e_ast_parse(&ast, &root);
  if (root < 0 || e) {
    fprintf(stderr, "ec: AST parsing failed\n");
    goto ret;
  }

  if (ast_only) {
    // TODO: Add AST printing / visualization
    return 0;
  }

  ecc_info info = {
    .arena              = &arena,
    .ast                = &ast,
    .root               = root,
    .custom_entry_point = nullptr,
    .opt_level          = 3,
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
    size_t        goob = 0;
    e_arena_page* next = arena.current;
    while (next) {
      goob++;
      next = next->next;
    }
    fprintf(stderr, "%zu pages were allocated (%zu bytes)\n", goob, goob * (size_t)E_PAGE_SIZE);
  }

ret:
  if (contents) free(contents);
  if (ntoks > 0 && tokens) e_freetoks(tokens, ntoks);
  e_ast_free(&ast);
  e_compilation_result_free(&compiled);
  e_refdobj_pool_free(&ge_pool);
  if (f) fclose(f);
  e_str_interner_free(&interner);
  e_arena_free(&arena);
  return e;
}