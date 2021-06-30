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
void insert_cond_exit_branch(dbm_code_cache_meta *bb_meta, uint32_t **write_p, 
	mambo_cond *cond);

void dispatcher_aarch64(dbm_thread *thread_data, uint32_t source_index, 
	branch_type exit_type, uintptr_t target, uintptr_t block_address);

#endif
#endif