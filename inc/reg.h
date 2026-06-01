#ifndef E_REGISTERS_H
#define E_REGISTERS_H

#define E_REG_ARG_COUNT 16
#define E_REG_COUNT 128

typedef enum e_regs {
  E_REG_ARG0  = 0,
  E_REG_ARG1  = 1,
  E_REG_ARG2  = 2,
  E_REG_ARG3  = 3,
  E_REG_ARG4  = 4,
  E_REG_ARG5  = 5,
  E_REG_ARG6  = 6,
  E_REG_ARG7  = 7,
  E_REG_ARG8  = 8,
  E_REG_ARG9  = 9,
  E_REG_ARG10 = 10,
  E_REG_ARG11 = 11,
  E_REG_ARG12 = 12,
  E_REG_ARG13 = 13,
  E_REG_ARG14 = 14,
  E_REG_ARG15 = 15,

  E_REG_SP   = 16, /* stack pointer */
  E_REG_IP   = 17, /* instruction pointer (next instruction to be executed) */
  E_REG_TMP1 = 18,
  E_REG_TMP2 = 19,

  E_REG_GENERAL_BEGIN = 20,
  E_REG_GENERAL_END   = 255
} e_regs;

#endif