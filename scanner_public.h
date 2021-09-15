/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo

  Copyright 2013-2016 Cosmin Gorgovan <cosmin at linux-geek dot org>
  Copyright 2015-2017 Guillermo Callaghan <guillermocallaghan at hotmail dot com>
  Copyright 2017 The University of Manchester

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef __SCANNER_PUBLIC_H__
#define __SCANNER_PUBLIC_H__

#include <stdint.h>

#define IMM_LDR 0
#define LDR_REG 1
#define IMM_PROC 1
#define REG_PROC 0

#ifdef __arm__
enum reg {
  r0 = 0,
  r1 = 1,
  r2 = 2,
  r3 = 3,
  r4 = 4,
  r5 = 5,
  r6 = 6,
  r7 = 7,
  r8 = 8, 
  r9 = 9,
  r10 = 10,
  r11 = 11,
  r12 = 12,
  r13 = 13,
  r14 = 14,
  r15 = 15,
  reg_invalid = 16
};

enum reg_alt {
  es = r4, // the first calleE-Saved register - not a standard alias
  sp = r13,
  lr = r14,
  pc = r15
};

#define m_r0 (1 << r0)
#define m_r1 (1 << r1)
#define m_r2 (1 << r2)
#define m_r3 (1 << r3)
#define m_r4 (1 << r4)
#define m_r5 (1 << r5)
#define m_r6 (1 << r6)
#define m_r7 (1 << r7)
#define m_r8 (1 << r8)
#define m_r9 (1 << r9)
#define m_r10 (1 << r10)
#define m_r11 (1 << r11)
#define m_r12 (1 << r12)
#define m_r13 (1 << r13)
#define m_r14 (1 << r14)
#define m_r15 (1 << r15)
#endif // __arm__

#ifdef __aarch64__
enum reg {      // +--------------+
  x0   =   0,   // | X0           |
  x1   =   1,   // | X1           |
  x2   =   2,   // | X2           |
  x3   =   3,   // | X3           |
  x4   =   4,   // | X4           |
  x5   =   5,   // | X5           |
  x6   =   6,   // | X6           |
  x7   =   7,   // | X7           |
  x8   =   8,   // | X8 (XR)      |
  x9   =   9,   // | X9           |
  x10  =  10,   // | X10          |
  x11  =  11,   // | X11          |
  x12  =  12,   // | X12          |
  x13  =  13,   // | X13          |
  x14  =  14,   // | X14          |
  x15  =  15,   // | X15          |
  x16  =  16,   // | X16 (IP0)    |
  x17  =  17,   // | X17 (IP1)    |
  x18  =  18,   // | X18 (PR)     |
  x19  =  19,   // | X19          |
  x20  =  20,   // | X20          |
  x21  =  21,   // | X21          |
  x22  =  22,   // | X22          |
  x23  =  23,   // | X23          |
  x24  =  24,   // | X24          |
  x25  =  25,   // | X25          |
  x26  =  26,   // | X26          |
  x27  =  27,   // | X27          |
  x28  =  28,   // | X28          |
  x29  =  29,   // | X29 (FP)     |
  x30  =  30,   // | X30 (LR)     |
  x31  =  31,   // | X31 (SP/XZR) |
  reg_invalid = 32
};              // +--------------+

enum reg_alt {
  xr   =  x8,   // Designated Indirect Result Location Parameter
  ip0  =  x16,  // Intra-Procedure Call temporary registers
  ip1  =  x17,  // Intra-Procedure Call temporary registers
  pr   =  x18,  // Platform Register
  es   =  x19,  // the first calleE-Saved register - not a standard alias
  fp   =  x29,  // Frame Pointer
  lr   =  x30,  // Link register
  sp   =  x31,  // Stack Pointer
  xzr  =  x31,  // Zero Register
};

#define m_x0 (1 << x0)
#define m_x1 (1 << x1)
#define m_x2 (1 << x2)
#define m_x3 (1 << x3)
#define m_x4 (1 << x4)
#define m_x5 (1 << x5)
#define m_x6 (1 << x6)
#define m_x7 (1 << x7)
#define m_x8 (1 << x8)
#define m_x9 (1 << x9)
#define m_x10 (1 << x10)
#define m_x11 (1 << x11)
#define m_x12 (1 << x12)
#define m_x13 (1 << x13)
#define m_x14 (1 << x14)
#define m_x15 (1 << x15)
#define m_x16 (1 << x16)
#define m_x17 (1 << x17)
#define m_x18 (1 << x18)
#define m_x19 (1 << x19)
#define m_x20 (1 << x20)
#define m_x21 (1 << x21)
#define m_x22 (1 << x22)
#define m_x23 (1 << x23)
#define m_x24 (1 << x24)
#define m_x25 (1 << x25)
#define m_x26 (1 << x26)
#define m_x27 (1 << x27)
#define m_x28 (1 << x28)
#define m_x29 (1 << x29)
#define m_x30 (1 << x30)
#define m_x31 (1 << x31)

#define m_xr  (1 << xr)
#define m_ip0 (1 << ip0)
#define m_ip1 (1 << ip1)
#define m_pr  (1 << pr)
#define m_xzr (1 << xzr)
#endif

#if defined(__arm__) || defined(__aarch64__)
enum reg_portable {
  reg0 = 0,
  reg1 = 1,
  reg2 = 2,
  reg3 = 3,
  reg4 = 4,
  reg5 = 5,
  reg6 = 6,
  reg7 = 7,
  reg8 = 8,
  reg9 = 9,
  reg10 = 10,
  reg11 = 11,
  reg12 = 12
};

typedef enum arm_cond_codes {
  EQ = 0,
  NE = 1,
  CS = 2,
  CC = 3,
  MI = 4,
  PL = 5,
  VS = 6,
  VC = 7,
  HI = 8,
  LS = 9,
  GE = 10,
  LT = 11,
  GT = 12,
  LE = 13,
  AL = 14,
  ALT = 15
} mambo_cond;

enum shift_type {
  LSL = 0,
  LSR = 1,
  ASR = 2,
  ROR = 3
};

extern enum arm_cond_codes arm_inverse_cond_code[];

#define invert_cond(cond) ((cond) ^ 1)

#define arm_cond_push_reg(cond, reg) \
  arm_str_cond(&write_p, cond, IMM_LDR, reg, sp, 4, 1, 0, 1); \
  write_p++;

#define arm_cond_pop_reg(cond, reg) \
  arm_ldr_cond(&write_p, cond, IMM_LDR, reg, sp, 4, 0, 1, 0); \
  write_p++;

#define arm_push_reg(reg) \
  arm_str(&write_p, IMM_LDR, reg, sp, 4, 1, 0, 1); \
  write_p++;

#define arm_pop_reg(reg) \
  arm_ldr(&write_p, IMM_LDR, reg, sp, 4, 0, 1, 0); \
  write_p++;

#define arm_push_regs(regs) \
  arm_stm(&write_p, sp, regs, 1, 0, 1, 0); \
  write_p++;

#define arm_pop_regs(regs) \
  if ((regs) & (1 << sp)) { \
    arm_ldm(&write_p, sp, regs, 0, 1, 0, 0); \
  } else { \
    arm_ldm(&write_p, sp, regs, 0, 1, 1, 0); \
  } \
  write_p++;

/*
 * PUSH PAIR
 * STP Xt1, Xt2, [SP]!
 */
#define a64_push_pair_reg(Xt1, Xt2) \
  a64_LDP_STP(&write_p, 2, 0, 3, 0, -2, Xt2, sp, Xt1); \
  write_p++;

/*
 * POP PAIR
 * LDP Xt1, Xt2, [SP], #16
 */
#define a64_pop_pair_reg(Xt1, Xt2) \
  a64_LDP_STP(&write_p, 2, 0, 1, 1, 2, Xt2, sp, Xt1); \
  write_p++;

/*
 * PUSH REGISTER
 * STR reg, [SP, #-16]!
 */
#define a64_push_reg(reg) \
  a64_LDR_STR_immed(&write_p, 3, 0, 0, -16, 3, sp, reg); \
  write_p++;

/*
 * POP REGISTER
 * LDR reg, [SP], #16
 */
#define a64_pop_reg(reg) \
  a64_LDR_STR_immed(&write_p, 3, 0, 1, 16, 1, sp, reg); \
  write_p++;

void copy_to_reg_16bit(uint16_t **write_p, enum reg reg, uint32_t value);
void copy_to_reg_32bit(uint16_t **write_p, enum reg reg, uint32_t value);
void a64_copy_to_reg_64bits(uint32_t **write_p, enum reg reg, uint64_t value);

void thumb_push_regs(uint16_t **write_p, uint32_t regs);
void thumb_pop_regs(uint16_t **write_p, uint32_t regs);
void arm_copy_to_reg_16bit(uint32_t **write_p, enum reg reg, uint32_t value);
void arm_cond_copy_to_reg_16bit(uint32_t **write_p, enum arm_cond_codes cond, enum reg reg, uint32_t value);
void arm_copy_to_reg_32bit(uint32_t **write_p, enum reg reg, uint32_t value);
void arm_cond_copy_to_reg_32bit(uint32_t **write_p, enum arm_cond_codes cond, enum reg reg, uint32_t value);
void arm_add_sub_32_bit(uint32_t **write_p, enum reg rd, enum reg rn, int value);
#endif

#ifdef DBM_ARCH_RISCV64
/**
 * RISC-V registers.
 */
enum reg {      // +--------------+
  x0   =   0,   // | x0   (zero)  |
  x1   =   1,   // | x1   (ra)    |
  x2   =   2,   // | x2   (sp)    |
  x3   =   3,   // | x3   (gp)    |
  x4   =   4,   // | x4   (tp)    |
  x5   =   5,   // | x5           |
  x6   =   6,   // | x6           |
  x7   =   7,   // | x7           |
  x8   =   8,   // | x8   (fp)    |
  x9   =   9,   // | x9           |
  x10  =  10,   // | x10          |
  x11  =  11,   // | x11          |
  x12  =  12,   // | x12          |
  x13  =  13,   // | x13          |
  x14  =  14,   // | x14          |
  x15  =  15,   // | x15          |
  x16  =  16,   // | x16          |
  x17  =  17,   // | x17          |
  x18  =  18,   // | x18          |
  x19  =  19,   // | x19          |
  x20  =  20,   // | x20          |
  x21  =  21,   // | x21          |
  x22  =  22,   // | x22          |
  x23  =  23,   // | x23          |
  x24  =  24,   // | x24          |
  x25  =  25,   // | x25          |
  x26  =  26,   // | x26          |
  x27  =  27,   // | x27          |
  x28  =  28,   // | x28          |
  x29  =  29,   // | x29          |
  x30  =  30,   // | x30          |
  x31  =  31,   // | x31          |
  reg_invalid = 32
};              // +--------------+

/**
 * RISC-V alternative register names (ABI names)
 */
enum reg_alt {
  zero  =  x0,  /**< Hardwired Zero Register    */
  ra    =  x1,  /**< Return Address Register    */
  sp    =  x2,  /**< Stack Pointer              */
  gp    =  x3,  /**< Global Pointer             */
  tp    =  x4,  /**< Thread Pointer             */
  fp    =  x8,  /**< Frame Pointer              */
  c_x8  =  0,   /**< Compressed register x8/s0  */
  c_x9  =  1,   /**< Compressed register x9/s1  */
  c_x10 =  2,   /**< Compressed register x10/a0 */
  c_x11 =  3,   /**< Compressed register x11/a1 */
  c_x12 =  4,   /**< Compressed register x12/a2 */
  c_x13 =  5,   /**< Compressed register x13/a3 */
  c_x14 =  6,   /**< Compressed register x14/a4 */
  c_x15 =  7,   /**< Compressed register x15/a5 */
};

#define INST_16BIT 2 /** RISC-V compressed instruction length (in bytes). */
#define INST_32BIT 4 /** RISC-V base instruction length (in bytes). */

#define m_x0 (1 << x0)
#define m_x1 (1 << x1)
#define m_x2 (1 << x2)
#define m_x3 (1 << x3)
#define m_x4 (1 << x4)
#define m_x5 (1 << x5)
#define m_x6 (1 << x6)
#define m_x7 (1 << x7)
#define m_x8 (1 << x8)
#define m_x9 (1 << x9)
#define m_x10 (1 << x10)
#define m_x11 (1 << x11)
#define m_x12 (1 << x12)
#define m_x13 (1 << x13)
#define m_x14 (1 << x14)
#define m_x15 (1 << x15)
#define m_x16 (1 << x16)
#define m_x17 (1 << x17)
#define m_x18 (1 << x18)
#define m_x19 (1 << x19)
#define m_x20 (1 << x20)
#define m_x21 (1 << x21)
#define m_x22 (1 << x22)
#define m_x23 (1 << x23)
#define m_x24 (1 << x24)
#define m_x25 (1 << x25)
#define m_x26 (1 << x26)
#define m_x27 (1 << x27)
#define m_x28 (1 << x28)
#define m_x29 (1 << x29)
#define m_x30 (1 << x30)
#define m_x31 (1 << x31)

#define m_zero  (1 << zero)
#define m_ra    (1 << ra)
#define m_sp    (1 << sp)
#define m_gp    (1 << gp)
#define m_tp    (1 << tp)
#define m_fp    (1 << fp)

#define invert_cond(cond) cond ^ 1

/**
 * Write code to copy an immediate value up to 32 bits to a register.
 * @param write_p Pointer to the writing location.
 * @param reg Destination register.
 * @param value Value to store in the register (32 Bits).
 */
void riscv_copy_to_reg_32bits(uint16_t **write_p, enum reg reg, uint32_t value);

/**
 * Write code to copy an immediate value up to 64 bits to a register. 
 * For values that only need 32 bit use riscv_copy_to_reg_32bits() for lower memory
 * footprint.
 * @param write_p Pointer to the writing location.
 * @param reg Destination register.
 * @param value Value to store in the register (64 Bits).
 * @see riscv_copy_to_reg_32bits()
 */
void riscv_copy_to_reg_64bits(uint16_t **write_p, enum reg reg, uint64_t value);

/**
 * RISC-V condition codes.
 */
enum riscv_cond_codes {
  EQ,
  NE,
  LT,
  GE,
  LTU,
  GEU,
  AL    /**< Always/No condition */
};

/**
 * RISC-V conditions.
 */
typedef struct
{
  enum reg r1;                /**< Register 1 to compare with register 2. */
  enum reg r2;                /**< Register 2 to compare with register 1. */
  enum riscv_cond_codes cond; /**< Condition for branching. */
} mambo_cond;

#endif

void init_plugin();

void mambo_memcpy(void *dst, void *src, size_t l);

/**
 * Sign extend value to 64 bit integer.
 * @param bits Size of value (index of sign bit + 1).
 * @param value Value to sign extend.
 * @return Sign extended 64 bit integer.
 */
static inline uint64_t sign_extend64(unsigned int bits, uint64_t value)
{
    uint64_t C = (-1) << (bits - (uint64_t) 1);
    return (value + C) ^ C;
}

/**
 * Sign extend value to 32 bit integer.
 * @param bits Size of value (index of sign bit + 1).
 * @param value Value to sign extend.
 * @return Sign extended 32 bit integer.
 */
static inline int32_t sign_extend32(unsigned int bits, uint32_t value)
{
  uint32_t C = (-1) << (bits - 1);
  return (int32_t)((value + C) ^ C);
}
#endif

