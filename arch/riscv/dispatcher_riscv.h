#ifndef __DISPATCHER_RISCV_H__
#define __DISPATCHER_RISCV_H__
#ifdef DBM_ARCH_RISCV64

#include "../../dbm.h"
#include "../../scanner_common.h"

/**
 * Insert code cache basic block exit. 
 * Insert branch to skip next instruction (jump write_p + 4) depending on the given 
 * condition.
 * @param bb_meta Basic block meta data.
 * @param write_p Pointer to the writing location (branch_exit_address).
 * @param cond Condition for jumping over the next instruction.
 */
void insert_cond_exit_branch(dbm_code_cache_meta *bb_meta, uint16_t **write_p, 
	mambo_cond *cond);

/**
 * Link and insert (conditional) jumps to next basic block.
 * @param thread_data Thread data of current thread.
 * @param source_index Source block index.
 * @param exit_type Exit type of current block.
 * @param target Branch target in original code space.
 * @param block_address Address of target block where \c target is instrumented.
 */
void dispatcher_riscv(dbm_thread *thread_data, uint32_t source_index, 
	branch_type exit_type, uintptr_t target, uintptr_t block_address);

#endif
#endif