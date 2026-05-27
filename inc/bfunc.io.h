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

#ifndef E_IO_BUILTIN_FUNCTIONS_H
#define E_IO_BUILTIN_FUNCTIONS_H

#include "var.h"

char* e_read_full_line(FILE* f);

e_var eb_io_read(e_var* args, u32 nargs);
e_var eb_io_write(e_var* args, u32 nargs);
e_var eb_io_seek(e_var* args, u32 nargs);
e_var eb_io_ptell(e_var* args, u32 nargs);
e_var eb_io_error(e_var* args, u32 nargs);
e_var eb_io_flush(e_var* args, u32 nargs);
e_var eb_io_readln(e_var* args, u32 nargs);
e_var eb_io_println(e_var* args, u32 nargs);
e_var eb_io_print(e_var* args, u32 nargs);
e_var eb_io_type(e_var* args, u32 nargs);
e_var eb_io_at_eof(e_var* args, u32 nargs);
e_var eb_io_open(e_var* args, u32 nargs);
e_var eb_io_close(e_var* args, u32 nargs);
e_var eb_io_getc(e_var* args, u32 nargs);
e_var eb_io_putc(e_var* args, u32 nargs);
e_var eb_io_listdir(e_var* args, u32 nargs);
e_var eb_io_exists(e_var* args, u32 nargs);
e_var eb_io_mkdir(e_var* args, u32 nargs);

#endif // E_IO_BUILTIN_FUNCTIONS_H