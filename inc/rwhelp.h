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

#ifndef E_BYTECODE_STREAM_READ_WRITE_HELP_H
#define E_BYTECODE_STREAM_READ_WRITE_HELP_H

#include "bc.h"
#include "cc.h"
#include "stdafx.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define E_FILE_MAGIC 0xF5F6F7F8

// See BYTECODE FORMAT section of README.

#define E_GEN_READ_FUNCTION(type)                                                                                                                    \
  static inline type e_read_##type(const u8** _ip)                                                                                                   \
  {                                                                                                                                                  \
    *(_ip) = (const u8*)e_align_ptr((void*)*(_ip), sizeof(type)); /* Align IP so we don't get a SIGBUS */                                            \
    type v = 0;                                                                                                                                      \
    memcpy(&v, *_ip, sizeof(type)); /* memcpy from ip, faster and safer than dereferencing */                                                        \
    *(_ip) += sizeof(type);         /* Move ip forward*/                                                                                             \
    return v;                                                                                                                                        \
  }
E_GEN_READ_FUNCTION(u32)
E_GEN_READ_FUNCTION(u16)
E_GEN_READ_FUNCTION(u8)

#define E_GEN_EMIT_FN(type)                                                                                                                          \
  static inline void e_emit_##type(e_compiler* cc, type value)                                                                                       \
  {                                                                                                                                                  \
    if (cc->num_bytes_emitted + sizeof(type) > cc->emit_capacity)                                                                                    \
      ecc_stream_resize(cc, MAX(cc->num_bytes_emitted + sizeof(type), (size_t)cc->emit_capacity * 2));                                               \
    void* unaligned = (uchar*)cc->emit + cc->num_bytes_emitted;                                                                                      \
    void* aligned   = e_align_ptr((uchar*)cc->emit + cc->num_bytes_emitted, sizeof(type)); /* Align pointer so we don't get a SIGBUS */              \
    memset(unaligned, E_OPCODE_NOOP, (uchar*)aligned - (uchar*)unaligned);                 /* Fill the memory we're skipping over with NOOPs */      \
    memcpy((type*)aligned, &value, sizeof(value));                                         /* Emit our value */                                      \
    cc->num_bytes_emitted += ((uchar*)aligned - (uchar*)unaligned);                        /* Move over alignment padding */                         \
    cc->num_bytes_emitted += sizeof(type);                                                 /* Move over size */                                      \
  }
E_GEN_EMIT_FN(u32)
E_GEN_EMIT_FN(u64)
E_GEN_EMIT_FN(u16)
E_GEN_EMIT_FN(u8)

typedef enum e_file_read_error {
  E_FILE_READ_SUCCESS                    = 0,
  E_FILE_READ_ERR_INVALID_MAGIC          = -1,
  E_FILE_READ_ERR_ROOT_ALLOCATION_FAILED = -2,
  E_FILE_READ_ERR_INVALID_FILE           = -3,
} e_file_read_error;

static inline void
e_emit_instruction(e_compiler* cc, e_opcode opcode)
{ e_emit_u8(cc, opcode); }

e_ins e_read_ins(const u8** ip);

e_file_read_error e_file_load(e_compilation_result* r, void** root_allocation, FILE* f);

u32  e_file_bytes_required(const e_compilation_result* r);
void e_file_write(const e_compilation_result* r, FILE* f);

#endif // E_BYTECODE_STREAM_READ_WRITE_HELP_H