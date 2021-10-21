/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo

  Copyright 2013-2016 Cosmin Gorgovan <cosmin at linux-geek dot org>
  Copyright 2017-2020 The University of Manchester

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

#ifndef __API_HELPERS_H__
#define __API_HELPERS_H__

typedef struct {
  void *loc;
} mambo_branch;

#define LSL 0
#define LSR 1
#define ASR 2

#ifdef __arm__
  #define MAX_FCALL_ARGS 4
#elif __aarch64__ || DBM_ARCH_RISCV64
  #define MAX_FCALL_ARGS 8
#endif

#ifdef DBM_ARCH_RISCV64
  #define PARAM_REGS_OFFSET 10
#else
  #define PARAM_REGS_OFFSET 0
#endif

/**
 * Write code to increment a counter.
 * @param ctx MAMBO context.
 * @param counter Counter to increment.
 * @param incr Incrementation size.
 */
void emit_counter64_incr(mambo_context *ctx, void *counter, unsigned incr);

/**
 * Write code to push registers.
 * @param ctx MAMBO context.
 * @param regs Register mask of registers to push. Is bit 1 set, `x1` will be pushed.
 */
void emit_push(mambo_context *ctx, uint32_t regs);

/**
 * Write code to pop registers.
 * @param ctx MAMBO context.
 * @param regs Register mask of registers to pop. Is bit 1 set, `x1` will be popped.
 */
void emit_pop(mambo_context *ctx, uint32_t regs);

/**
 * Write code to load an intermediate value to a register.
 * @param ctx MAMBO context.
 * @param reg Register to load the value in.
 * @param value Value to load.
 */
void emit_set_reg(mambo_context *ctx, enum reg reg, uintptr_t value);

/**
 * Write code to call a function.
 * @warning Using this function for inserting a function call can cause unexpected
 *  behavior when inserted at an unsuitable location. Be sure the inserted function
 *  can mess with volatile registers or consider ::emit_safe_fcall.
 * @param ctx MAMBO context.
 * @param function_ptr Raw function pointer.
 */
void emit_fcall(mambo_context *ctx, void *function_ptr);

/**
 * Write code to call a function safely without breaking the instrumented client.
 * This function can insert a function call everywhere.
 * @param ctx MAMBO context.
 * @param function_ptr Pointer to function to call.
 * @param argno Number of arguments prepared for the function.
 * @return 0 if executed successfully, else non-zero (`argno` exceeded maximum).
 */
int emit_safe_fcall(mambo_context *ctx, void *function_ptr, int argno);

/**
 * Write code to call a function safely without breaking the instrumented client.
 * This function can insert a function call everywhere.
 * @param ctx MAMBO context.
 * @param fptr Pointer to function to call.
 * @param argno Number of arguments prepared for the function.
 * @param ... Arguments to write to registers.
 * @return 0 if executed successfully, else non-zero (`argno` invalid).
 */
int emit_safe_fcall_static_args(mambo_context *ctx, void *fptr, int argno, ...);
int emit_indirect_branch_by_spc(mambo_context *ctx, enum reg reg);

/**
 * Write code to copy the value from the source register to the destination register.
 * @param ctx MAMBO context.
 * @param rd Destination register.
 * @param rn Source register.
 */
void emit_mov(mambo_context *ctx, enum reg rd, enum reg rn);

/**
 * Write code to add a signed value to a register.
 * @param ctx MAMBO context.
 * @param rd Destination register.
 * @param rn Source register.
 * @param offset Signed immediate value to add to the value in the source register.
 * @return 0 if executed successfully, else non-zero (`offset` out of range).
 */
int emit_add_sub_i(mambo_context *ctx, int rd, int rn, int offset);

/**
 * Write code to add shifted register.
 * @param ctx MAMBO context.
 * @param rd Destination register.
 * @param rn First source register.
 * @param rm Secound source register. If positive, `rm` is added to `rn`, if negative, 
 *  `abs(rm)` is subtracted from `rn`.
 * @param shift_type Shift type to be applied to the secound source register.
 * @param shift Shift amount.
 * @return 0 if executed successfully, else non-zero (invalid shift amount or type).
 */
int emit_add_sub_shift(mambo_context *ctx, int rd, int rn, int rm,
                       unsigned int shift_type, unsigned int shift);

/**
 * Write code to add or substract two registers.
 * @param ctx MAMBO context.
 * @param rd Destination register.
 * @param rn First source register.
 * @param rm Secound source register. If positive, `rm` is added to `rn`, if negative, 
 *  `abs(rm)` is subtracted from `rn`.
 * @return 0 if executed successfully, else non-zero.
 */
int emit_add_sub(mambo_context *ctx, int rd, int rn, int rm);


/**
 * Write code to branch to a static target.
 * @param ctx MAMBO context.
 * @param target Branch target.
 * @return 0 if executed successfully, else non-zero.
 */
int emit_branch(mambo_context *ctx, void *target);

/**
 * Write code to conditionally branch to a static target.
 * @param ctx MAMBO context.
 * @param target Branch target.
 * @param cond Branch condition.
 * @retval >=0: Size of the branch inserted in bytes (usually 2 or 4 bytes).
 * @retval -1: Branch or jump not written.
 */
int emit_branch_cond(mambo_context *ctx, void *target, mambo_cond cond);

/**
 * Write code to branch to a static target if value of reg is zero (or not).
 * @param ctx MAMBO context.
 * @param target Branch target.
 * @param reg Register to test.
 * @param is_cbz True for branch if register value is zero, false for branch if not zero.
 * @return 0 if executed successfully, else non-zero.
 */
int emit_branch_cbz_cbnz(mambo_context *ctx, void *target, enum reg reg, bool is_cbz);

/**
 * Write code to branch to a static target if value of reg is zero.
 * @param ctx MAMBO context.
 * @param target Branch target.
 * @param reg Register to test.
 * @return 0 if executed successfully, else non-zero.
 */
int emit_branch_cbz(mambo_context *ctx, void *target, enum reg reg);

/**
 * Write code to branch to a static target if value of reg is not zero.
 * @param ctx MAMBO context.
 * @param target Branch target.
 * @param reg Register to test.
 * @return 0 if executed successfully, else non-zero.
 */
int emit_branch_cbnz(mambo_context *ctx, void *target, enum reg reg);

int mambo_reserve_branch(mambo_context *ctx, mambo_branch *br);
int mambo_reserve_branch_cbz(mambo_context *ctx, mambo_branch *br);

int emit_local_branch_cond(mambo_context *ctx, mambo_branch *br, mambo_cond cond);
int emit_local_branch(mambo_context *ctx, mambo_branch *br);
int emit_local_fcall(mambo_context *ctx, mambo_branch *br);
int emit_local_branch_cbz_cbnz(mambo_context *ctx, mambo_branch *br, enum reg reg, bool is_cbz);
int emit_local_branch_cbz(mambo_context *ctx, mambo_branch *br, enum reg reg);
int emit_local_branch_cbnz(mambo_context *ctx, mambo_branch *br, enum reg reg);

static inline void emit_set_reg_ptr(mambo_context *ctx, enum reg reg, void *ptr) {
  emit_set_reg(ctx, reg, (uintptr_t)ptr);
}

#ifdef __arm__
#define ROR 3
void emit_thumb_push_cpsr(mambo_context *ctx, enum reg reg);
void emit_arm_push_cpsr(mambo_context *ctx, enum reg reg);
void emit_thumb_pop_cpsr(mambo_context *ctx, enum reg reg);
void emit_arm_pop_cpsr(mambo_context *ctx, enum reg reg);
void emit_thumb_copy_to_reg_32bit(mambo_context *ctx, enum reg reg, uint32_t value);
void emit_arm_copy_to_reg_32bit(mambo_context *ctx, enum reg reg, uint32_t value);
void emit_thumb_b16_cond(void *write_p, void *target, mambo_cond cond);
void emit_thumb_push(mambo_context *ctx, uint32_t regs);
void emit_arm_push(mambo_context *ctx, uint32_t regs);
void emit_thumb_pop(mambo_context *ctx, uint32_t regs);
void emit_arm_pop(mambo_context *ctx, uint32_t regs);
void emit_thumb_fcall(mambo_context *ctx, void *function_ptr);
void emit_arm_fcall(mambo_context *ctx, void *function_ptr);
static inline int emit_arm_add_sub_shift(mambo_context *ctx, int rd, int rn, int rm,
                                         unsigned int shift_type, unsigned int shift);
static inline int emit_thumb_add_sub_shift(mambo_context *ctx, int rd, int rn, int rm,
                                           unsigned int shift_type, unsigned int shift);
static inline int emit_arm_add_sub(mambo_context *ctx, int rd, int rn, int rm);
static inline int emit_thumb_add_sub(mambo_context *ctx, int rd, int rn, int rm);
#endif

#ifdef __aarch64__
void emit_a64_push(mambo_context *ctx, uint32_t regs);
void emit_a64_pop(mambo_context *ctx, uint32_t regs);
static inline int emit_a64_add_sub_shift(mambo_context *ctx, int rd, int rn, int rm,
                                   unsigned int shift_type, unsigned int shift);
static inline int emit_a64_add_sub(mambo_context *ctx, int rd, int rn, int rm);
int emit_a64_add_sub_ext(mambo_context *ctx, int rd, int rn, int rm, int ext_option, int shift);
#endif

#ifdef DBM_ARCH_RISCV64
/**
 * Write RISC-V code to push registers.
 * @param ctx MAMBO context.
 * @param regs Register mask of registers to push. Is bit 1 set, `x1` will be pushed.
 */
void emit_riscv_push(mambo_context *ctx, uint32_t regs);

/**
 * Write RISC-V code to pop registers.
 * @param ctx MAMBO context.
 * @param regs Register mask of registers to pop. Is bit 1 set, `x1` will be popped.
 */
void emit_riscv_pop(mambo_context *ctx, uint32_t regs);
#endif

#endif
