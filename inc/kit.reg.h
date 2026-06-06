#ifndef KIT_REGISTERS_H
#define KIT_REGISTERS_H

#define KIT_REG_ARG_COUNT 16

/* kit_preg_t must be large enough to store this amount (includes special registers). */
#define KIT_REG_COUNT 128

/* 1 byte on disk */
#define KIT_REG_DISK_SIZE (1)

typedef enum kit_regs {
  /* argument vector, 16 arguments are passed thru registers, rest thru the stack. */
  KIT_REG_ARG0    = 0,
  KIT_REG_ARG_END = KIT_REG_ARG0 + KIT_REG_ARG_COUNT - 1,

  KIT_REG_SP  = KIT_REG_ARG_END + 1, /* stack pointer */
  KIT_REG_IP  = KIT_REG_SP + 1,      /* instruction pointer (next instruction to be executed) */
  KIT_REG_NIL = KIT_REG_IP + 1,      /* nil register, always holds null */

  KIT_REG_GENERAL_BEGIN = 19,
  KIT_REG_GENERAL_END   = KIT_REG_COUNT - 1,
} kit_regs;

#endif