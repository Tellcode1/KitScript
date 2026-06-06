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

#ifndef KIT_BYTECODE_STREAM_READ_WRITE_HELP_H
#define KIT_BYTECODE_STREAM_READ_WRITE_HELP_H

#include "kit.cc.h"
#include "kit.ir.h"
#include "kit.stdafx.h"

#include <assert.h>
#include <stdio.h>

#define KIT_FILE_MAGIC 0xF5F6F7F8

// See BYTECODE FORMAT section of README.

#define KIT_GEN_READ_FUNCTION(type)                                                                                                                  \
  static inline type kit_read_##type(const u8** _ip)                                                                                                 \
  {                                                                                                                                                  \
    *(_ip) = (const u8*)kit_align_ptr((void*)*(_ip), sizeof(type)); /* Align IP so we don't get a SIGBUS */                                          \
    type v = 0;                                                                                                                                      \
    memcpy(&v, *_ip, sizeof(type)); /* memcpy from ip, faster and safer than dereferencing */                                                        \
    *(_ip) += sizeof(type);         /* Move ip forward*/                                                                                             \
    return v;                                                                                                                                        \
  }
KIT_GEN_READ_FUNCTION(u64)
KIT_GEN_READ_FUNCTION(u32)
KIT_GEN_READ_FUNCTION(u16)
KIT_GEN_READ_FUNCTION(u8)

typedef enum kit_file_read_error {
  KIT_FILE_READ_SUCCESS                    = 0,
  KIT_FILE_READ_ERR_INVALID_MAGIC          = -1,
  KIT_FILE_READ_ERR_ROOT_ALLOCATION_FAILED = -2,
  KIT_FILE_READ_ERR_INVALID_FILE           = -3,
} kit_file_read_error;

void    kit_emit_ins(kit_compiler* cc, kit_ins ins);
kit_ins kit_read_ins(const u8** ip);

kit_file_read_error kit_file_load(kit_compilation_result* r, void** root_allocation, FILE* f);

u32  kit_file_bytes_required(const kit_compilation_result* r);
void kit_file_write(const kit_compilation_result* r, FILE* f);

u8* kit_ins_serialize(kit_ins* ins, u32 nins);

#endif // KIT_BYTECODE_STREAM_READ_WRITE_HELP_H