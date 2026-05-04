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

#ifndef E_MATH_STRUCTURES_H
#define E_MATH_STRUCTURES_H

#include "stdafx.h"

/* Vector of 2 elements, aligned to 16 bytes */
typedef double e_vec2[2] ALIGNAS(16);
/* Vector of 3 elements, aligned to 16 bytes */
typedef double e_vec3[3] ALIGNAS(16);
/* Vector of 4 elements, aligned to 16 bytes */
typedef double e_vec4[4] ALIGNAS(16);

/* Intialize a vector2 from another (vec2 or larger) */
#define E_VEC2_INIT(old) { (old)[0], (old)[1] }
/* Intialize a vector3 from another (vec3 or larger) */
#define E_VEC3_INIT(old) { (old)[0], (old)[1], (old)[2] }
/* Intialize a vector4 from another */
#define E_VEC4_INIT(old) { (old)[0], (old)[1], (old)[2], (old)[3] }

/**
 * mat3 and mat4 are ref counted, to avoid bloating the
 * variable size. Variables are stored linearly in the stack
 * and any increase in size massively reduces performance.
 */
typedef struct e_mat3 {
  double m[3][3];
} e_mat3;
typedef struct e_mat4 {
  double m[4][4];
} e_mat4;

#endif // E_MATH_STRUCTURES_H
