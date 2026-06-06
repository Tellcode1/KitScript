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

#ifndef KIT_BUILTIN_STRUCT_H
#define KIT_BUILTIN_STRUCT_H

#include "kit.stdafx.h"

typedef struct kit_builtin_struct {
  const char*  name;
  const char** fields;
  u32          fields_count;
} kit_builtin_struct;

static const kit_builtin_struct kit_builtins_structs[] = {
  { .name = "vec2", .fields = (const char*[]){ "x", "y" }, .fields_count = 2 },
  { .name = "vec3", .fields = (const char*[]){ "x", "y", "z" }, .fields_count = 3 },
  { .name = "vec4", .fields = (const char*[]){ "x", "y", "z", "w" }, .fields_count = 4 },

  { .name = "mat3", .fields = (const char*[]){ "x", "y", "z" }, .fields_count = 3 },
  { .name = "mat4", .fields = (const char*[]){ "x", "y", "z", "w" }, .fields_count = 4 },

  // seconds are 0-59
  // minutes are 0-59
  // hours are 0-23
  // days are 1-31
  // wdays are 1-7
  // ydays are 1-366
  // mon is 1-12
  // year is 2026 right now
  { .name = "time::timestamp", .fields = (const char*[]){ "sec", "min", "hour", "day", "wday", "yday", "mon", "year" }, .fields_count = 8 },

  { .name         = "kit::exec_info",
    .fields       = (const char*[]){ "source_code", "entry_point", "optimization_level", "arguments", "command_line_arguments" },
    .fields_count = 5 },
  { .name = "kit::structure", .fields = (const char*[]){ "num_members", "member_names" }, .fields_count = 2 },
  { .name = "kit::constant", .fields = (const char*[]){ "name", "value" }, .fields_count = 2 },
  { .name = "kit::operator", .fields = (const char*[]){ "chr", "is_compound" }, .fields_count = 2 }, // Token value
  { .name = "kit::token", .fields = (const char*[]){ "type", "value" }, .fields_count = 2 },
  { .name         = "kit::compile_info",
    .fields       = (const char*[]){ "ast", "entry_point", "optimization_level", "builtin_structures", "builtin_constants" },
    .fields_count = 5 },

};

#endif // KIT_BUILTIN_STRUCT_H