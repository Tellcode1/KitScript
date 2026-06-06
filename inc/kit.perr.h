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

#ifndef KIT_PROGRAM_ERROR_H
#define KIT_PROGRAM_ERROR_H

typedef enum kit_ecode {
  KIT_OK           = 0,
  KIT_EMALLOC      = -1,
  KIT_EMALFORM     = -2, // Malformed input
  KIT_EILLINS      = -3, // Illegal instruction
  KIT_ENONEXISTENT = -4, // Non existent variable / structure.
  KIT_EUNDEFINED   = -5, // Undefined function
  KIT_EUNKNOWN     = -6,
  KIT_EPANIC       = -7,
  KIT_EBADARG      = -8,  // Bad arguments to executor
  KIT_ERANGE       = -9,  //  Out of range. JMP, etc.
  KIT_EASSERT      = -10, // Assertion failed
} kit_ecode;

static inline const char*
kit_ecode_str(kit_ecode e)
{
  switch (e) {
    case KIT_OK: return "Success";
    case KIT_EMALLOC: return "Allocation failed, Out of memory";
    case KIT_EMALFORM: return "Malformed or invalid input";
    case KIT_EILLINS: return "Illegal instruction";
    case KIT_ENONEXISTENT: return "Undeclared variable";
    case KIT_EUNDEFINED: return "Undefined function";
    case KIT_EUNKNOWN: return "Unknown error";
    case KIT_EPANIC: return "PANIC was called";
    case KIT_EBADARG: return "Invalid arguments passed to executor";
    case KIT_ERANGE: return "JMP out of range";
    case KIT_EASSERT: return "Assertion failed";
    default: break;
  }
  return "Unknown error code";
}

#endif // KIT_PROGRAM_ERROR_H
