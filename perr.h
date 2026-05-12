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

#ifndef E_PROGRAM_ERROR_H
#define E_PROGRAM_ERROR_H

typedef enum e_ecode {
  E_OK           = 0,
  E_EMALLOC      = -1,
  E_EMALFORM     = -2, // Malformed input
  E_EILLINS      = -3, // Illegal instruction
  E_ENONEXISTENT = -4, // Non existent variable / structure.
  E_EUNDEFINED   = -5, // Undefined function
  E_EUNKNOWN     = -6,
  E_EPANIC       = -7,
  E_EBADARG      = -8,  // Bad arguments to executor
  E_ERANGE       = -9,  //  Out of range. JMP, etc.
  E_EASSERT      = -10, // Assertion failed
} e_ecode;

static inline const char*
e_ecode_str(e_ecode e)
{
  switch (e) {
    case E_OK: return "Success";
    case E_EMALLOC: return "Allocation failed, Out of memory";
    case E_EMALFORM: return "Malformed or invalid input";
    case E_EILLINS: return "Illegal instruction";
    case E_ENONEXISTENT: return "Undeclared variable";
    case E_EUNDEFINED: return "Undefined function";
    case E_EUNKNOWN: return "Unknown error";
    case E_EPANIC: return "PANIC was called";
    case E_EBADARG: return "Invalid arguments passed to executor";
    case E_ERANGE: return "JMP out of range";
    case E_EASSERT: return "Assertion failed";
    default: break;
  }
  return "Unknown error code";
}

#endif // E_PROGRAM_ERROR_H
