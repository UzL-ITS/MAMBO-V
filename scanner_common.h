/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo

  Copyright 2013-2016 Cosmin Gorgovan <cosmin at linux-geek dot org>

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

#ifndef __SCANNER_COMMON_H__
#define __SCANNER_COMMON_H__

#include "scanner_public.h"
#ifdef DBM_ARCH_RISCV64
#include "pie/pie-riscv-decoder.h"
#endif

#define SETUP (1 << 0)
#define REPLACE_TARGET (1 << 1)
#define INSERT_BRANCH (1 << 2)
#define LATE_APP_SP (1 << 3)

#ifdef __arm__
  #define APP_SP (r3)
  #define DISP_SP_OFFSET (28)
  #define DISP_RES_WORDS (3)
#endif

#ifdef __arm__
void thumb_cc_branch(dbm_thread *thread_data, uint16_t *write_p, uint32_t dest_addr);
void thumb_b16_cond_helper(uint16_t *write_p, uint32_t dest_addr, mambo_cond cond);
void thumb_b32_helper(uint16_t *write_p, uint32_t dest_addr);
void thumb_b32_cond_helper(uint16_t **write_p, uint32_t dest_addr, enum arm_cond_codes condition);
void thumb_bl32_helper(uint16_t *write_p, uint32_t dest_addr);
void thumb_blx32_helper(uint16_t *write_p, uint32_t dest_addr);
void thumb_b_bl_helper(uint16_t *write_p, uint32_t dest_addr, bool link, bool to_arm);
void thumb_adjust_b_bl_target(dbm_thread *thread_data, uint16_t *write_p, uint32_t dest_addr);
void thumb_encode_cond_imm_branch(dbm_thread *thread_data,
                                       uint16_t **o_write_p,
                                       int basic_block,
                                       uint32_t address_taken,
                                       uint32_t address_skipped,
                                       enum arm_cond_codes condition,
                                       bool taken_in_cache,
                                       bool skipped_in_cache,
                                       bool update);
void thumb_encode_cbz_branch(dbm_thread *thread_data,
                                  uint32_t rn,
                                  uint16_t **o_write_p,
                                  int basic_block,
                                  uint32_t address_taken,
                                  uint32_t address_skipped,
                                  bool taken_in_cache,
                                  bool skipped_in_cache,
                                  bool update);

void arm_cc_branch(dbm_thread *thread_data, uint32_t *write_p, uint32_t target, uint32_t cond);
void arm_b32_helper(uint32_t *write_p, uint32_t target, uint32_t cond);
void arm_branch_helper(uint32_t *write_p, uint32_t target, bool link, uint32_t cond);
int thumb_cbz_cbnz_helper(uint16_t *write_p, uint32_t target, enum reg reg, bool cbz);
void arm_adjust_b_bl_target(uint32_t *write_p, uint32_t dest_addr);
void branch_save_context(dbm_thread *thread_data, uint16_t **o_write_p, bool late_app_sp);
void arm_inline_hash_lookup(dbm_thread *thread_data, uint32_t **o_write_p, int basic_block, int r_target);
void thumb_inline_hash_lookup(dbm_thread *thread_data, uint16_t **o_write_p, int basic_block, int r_target);
#endif

#ifdef __aarch64__
void a64_branch_helper(uint32_t *write_p, uint64_t target, bool link);
void a64_b_helper(uint32_t *write_p, uint64_t target);
void a64_bl_helper(uint32_t *write_p, uint64_t target);
void a64_b_cond_helper(uint32_t *write_p, uint64_t target, mambo_cond cond);
int a64_cbz_cbnz_helper(uint32_t *write_p, bool cbnz, uint64_t target, uint32_t sf, uint32_t rt);
void a64_cbz_helper(uint32_t *write_p, uint64_t target, uint32_t sf, uint32_t rt);
void a64_cbnz_helper(uint32_t *write_p, uint64_t target, uint32_t sf, uint32_t rt);
void a64_tbz_tbnz_helper(uint32_t *write_p, bool is_tbnz,
                         uint64_t target, enum reg reg, uint32_t bit);
void a64_tbz_helper(uint32_t *write_p, uint64_t target, enum reg reg, uint32_t bit);
void a64_tbnz_helper(uint32_t *write_p, uint64_t target, enum reg reg, uint32_t bit);
void a64_cc_branch(dbm_thread *thread_data, uint32_t *write_p, uint64_t target);
void a64_inline_hash_lookup(dbm_thread *thread_data, int basic_block, uint32_t **o_write_p,
                            uint32_t *read_address, enum reg rn, bool link, bool set_meta);
#endif

#ifdef DBM_ARCH_RISCV64
/** 
 * Write code to push a register to the stack.
 * @param write_p Pointer to the writing location.
 * @param reg Register to push.
 */
void riscv_push_helper(uint16_t **write_p, enum reg reg);

/** 
 * Write code to pop a register to the stack.
 * @param write_p Pointer to the writing location.
 * @param reg Register to pop.
 */
void riscv_pop_helper(uint16_t **write_p, enum reg reg);

/**
 * Write code to branch/jump to a target (JAL). 
 * Depending on the offset, the compressed instruction will be used if possible.
 * @param write_p Pointer to the writing location.
 * @param target Target address.
 * @param link Store address of the next instruction in the link register \c ra .
 * @return 0 if executed successfully, else non-zero.
 */
int riscv_branch_imm_helper(uint16_t **write_p, uint64_t target, bool link);

/**
 * Write code to branch to a target and link it in the code cache.
 * @param thread_data Thread data of current thread.
 * @param write_p Pointer to the writing location.
 * @param target Target address.
 */
void riscv_cc_branch(dbm_thread *thread_data, uint16_t *write_p, uint64_t target);

/**
 * Write code to conditionally branch to a target.
 * @param write_p Pointer to the writing location.
 * @param target Target address.
 * @param cond Condition struct.
 * @see mambo_cond
 * @return 0 if executed successfully, else non-zero.
 */
int riscv_b_cond_helper(uint16_t **write_p, uint64_t target, mambo_cond *cond);

/**
 * Write code to branch to a target if register is 0.
 * @param write_p Pointer to the writing location.
 * @param reg Register to compare with 0.
 * @param target Target address. 
 * @see riscv_cond_codes
 * @return 0 if executed successfully, else non-zero.
 */
int riscv_bez_helper(uint16_t **write_p, enum reg reg, uint64_t target);

/**
 * Write code to branch to a target if register is not 0.
 * @param write_p Pointer to the writing location.
 * @param reg Register to compare with 0.
 * @param target Target address. 
 * @see riscv_cond_codes
 * @return 0 if executed successfully, else non-zero.
 */
int riscv_bnez_helper(uint16_t **write_p, enum reg reg, uint64_t target);

/**
 * Write code to save registers on the stack.
 * @param write_p Pointer to the writing location.
 * @param reg_mask Mask of registers to save.
 * @see riscv_restore_regs
 */
void riscv_save_regs(uint16_t **write_p, uint32_t reg_mask);

/**
 * Write code to restore registers from the stack.
 * @param write_p Pointer to the writing location.
 * @param reg_mask Mask of registers to restore.
 * @see riscv_save_regs
 */
void riscv_restore_regs(uint16_t **write_p, uint32_t reg_mask);

/**
 * Write code to branch to a target by calling the dispatcher on it.
 * @note The registers x10 and x11 could be overwritten, it is recomended to save 
 * them before calling this function. Other functions may handle scratch registers 
 * itself but this function does not!
 * @param thread_data Thread data of current thread.
 * @param write_p Pointer to the writing location.
 * @param basic_block Index of current block.
 * @param target Original target address if the branch/jump.
 * @param flags Flags ( \c REPLACE_TARGET , \c INSERT_BRANCH )
 */
void riscv_branch_jump(dbm_thread *thread_data, uint16_t **write_p, int basic_block,
	uint64_t target, uint32_t flags);

/**
 * Write code to conditionally branch to a target by calling the dispatcher on it.
 * @param thread_data Thread data of current thread.
 * @param write_p Pointer to the writing location.
 * @param basic_block Index of current block.
 * @param target Original target address if the branch/jump.
 * @param cond Condition struct.
 * @param len Length of the original instruction.
 * @see mambo_cond
 * @see riscv_instruction
 */
void riscv_branch_jump_cond(dbm_thread *thread_data, uint16_t **write_p, 
	int basic_block, uint64_t target, uint16_t *read_address, mambo_cond *cond,
	int len);

/**
 * Check if size bytes can be written to current block. If not, create a new block 
 * and branch to it.
 * @param thread_data Thread data of current thread.
 * @param write_p Pointer to the writing location.
 * @param data Pointer to the end of the basic block space.
 * @param size Size of planned write.
 * @param cur_block Index of current block.
 */
void riscv_check_free_space(dbm_thread *thread_data, uint16_t **write_p, 
	uint16_t **data_p, uint32_t size, int cur_block);

/**
 * Read RISC-V code until branch that should be instrumented.
 * @param read_address Pointer to the current reading location.
 * @param bb_type Exit type of basic block.
 * @see branch_type
 */
void pass1_riscv(uint16_t *read_address, branch_type *bb_type);

/**
 * Insert a inline hash lookup. If the hash table contains the target a jump to the 
 * corresponding target address otherwise call the dispatcher to scan the code 
 * at the target location.
 * @param thread_data Thread data of current thread.
 * @param basic_block Index of current block.
 * @param write_p Pointer to the writing location.
 * @param read_address Pointer to the current reading location.
 * @param rn Jump target register.
 * @param offset Target register offset (added to register value).
 * @param link Save the address of the next instruction to \c ra .
 * @param set_meta Save jump target register to code cache meta data.
 * @param len Length of the original control flow instruction at \c read_adress .
 */
void riscv_inline_hash_lookup(dbm_thread *thread_data, int basic_block,
	uint16_t **write_p, uint16_t *read_address, enum reg rn, uint32_t offset, 
	bool link, bool set_meta, int len);

/**
 * Scan next basic block until basic block exit instruction (jump or branch).
 * @param thread_data Thread data of current thread.
 * @param read_address Pointer to the current reading location.
 * @param basic_block Index of current block.
 * @param type Type of the code cache block.
 * @param write_p Pointer to the writing location.
 */
size_t scan_riscv(dbm_thread *thread_data, uint16_t *read_address, int basic_block,
	cc_type type, uint16_t *write_p);

/**
 * Extract \c mambo_cond data from raw branch instruction.
 * @param inst Instruction to expect at read address.
 * @param read_address Address to read the code from.
 * @param cond Pointer to \c mambo_cond where the results are stored.
 * @param target Pointer where to save the target address of the branch. (Can be
 * 		NULL if the target address should not be saved).
 * @return 0 if data extracted successfully, non-zero if the given instruction is not
 * 		a branch instruction B[EQ | NE | LT{U} | GE{U}] or [C.BEQZ | C.BNEZ].
 * @see mambo_cond
 */
int riscv_get_mambo_cond(riscv_instruction inst, uint16_t *read_address, 
	mambo_cond *cond, uint64_t *target);
#endif

extern void inline_hash_lookup();
extern void end_of_inline_hash_lookup();
extern void inline_hash_lookup_get_addr();
extern void inline_hash_lookup_data();
#endif
