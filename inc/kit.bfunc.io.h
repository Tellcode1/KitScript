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

#ifndef KIT_IO_BUILTIN_FUNCTIONS_H
#define KIT_IO_BUILTIN_FUNCTIONS_H

#include "kit.var.h"

char* kit_read_full_line(FILE* f);

kit_var kit_builtins_io_read(kit_var* args, u32 nargs);
kit_var kit_builtins_io_write(kit_var* args, u32 nargs);
kit_var kit_builtins_io_seek(kit_var* args, u32 nargs);
kit_var kit_builtins_io_ptell(kit_var* args, u32 nargs);
kit_var kit_builtins_io_error(kit_var* args, u32 nargs);
kit_var kit_builtins_io_flush(kit_var* args, u32 nargs);
kit_var kit_builtins_io_readln(kit_var* args, u32 nargs);
kit_var kit_builtins_io_println(kit_var* args, u32 nargs);
kit_var kit_builtins_io_print(kit_var* args, u32 nargs);
kit_var kit_builtins_io_type(kit_var* args, u32 nargs);
kit_var kit_builtins_io_at_eof(kit_var* args, u32 nargs);
kit_var kit_builtins_io_open(kit_var* args, u32 nargs);
kit_var kit_builtins_io_close(kit_var* args, u32 nargs);
kit_var kit_builtins_io_getc(kit_var* args, u32 nargs);
kit_var kit_builtins_io_putc(kit_var* args, u32 nargs);
kit_var kit_builtins_io_listdir(kit_var* args, u32 nargs);
kit_var kit_builtins_io_exists(kit_var* args, u32 nargs);
kit_var kit_builtins_io_mkdir(kit_var* args, u32 nargs);

#endif // KIT_IO_BUILTIN_FUNCTIONS_H