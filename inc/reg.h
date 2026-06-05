#ifndef E_REGISTERS_H
#define E_REGISTERS_H

#define E_REG_ARG_COUNT 16

/* e_preg_t must be large enough to store this amount (includes special registers). */
#define E_REG_COUNT 128

/* 1 byte on disk */
#define E_REG_DISK_SIZE (1)

typedef enum e_regs {
  /* argument vector, 16 arguments are passed thru registers, rest thru the stack. */
  E_REG_ARG0    = 0,
  E_REG_ARG_END = E_REG_ARG0 + E_REG_ARG_COUNT - 1,

  E_REG_SP  = E_REG_ARG_END + 1, /* stack pointer */
  E_REG_IP  = E_REG_SP + 1,      /* instruction pointer (next instruction to be executed) */
  E_REG_NIL = E_REG_IP + 1,      /* nil register, always holds null */

  E_REG_GENERAL_BEGIN = 19,
  E_REG_GENERAL_END   = E_REG_COUNT - 1,
} e_regs;

#endif