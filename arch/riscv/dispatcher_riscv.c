#include <stdio.h>

#include "../../dbm.h"
#include "../../scanner_common.h"
#include "dispatcher_riscv.h"

#include "../../pie/pie-riscv-encoder.h"

// DEBUG: Turn off debug
#define DEBUG
#ifdef DEBUG
	#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
	#define debug(...)
#endif

void insert_cond_exit_branch(dbm_code_cache_meta *bb_meta, uint32_t **write_p, 
	mambo_cond *cond)
{
	switch(bb_meta->exit_branch_type) {
	case uncond_imm_riscv:
	case uncond_reg_riscv:
		return;
	case cond_imm_riscv:
		/*
		 * Override code written by riscv_branch_jump_cond
	 	 * 				+-------------------------------+
	 	 * 	 		 |- |	B(cond)	rs1, rs2, .+8		|	previously NOP
	 	 * 			 |	|	NOP							|	(everything else unchanged)
	 	 * 			 |	|								|
	 	 * 			 -> |	PUSH	x10, x11			|
	 	 * 				|	LI		x11, basic_block 	|
	 	 * 				|								|
	 	 * 				|	B(cond)	branch_target:		|
	 	 * 				|								|
	 	 * 				|	LI		x10, read_address+len
	 	 * 				|	JAL		DISPATCHER			|
	 	 * 				|								|
	 	 * 				| branch_target:				|
	 	 * 				|	LI		x10, target			|
	 	 * 				|	JAL		DISPATCHER			|
	 	 * 				+-------------------------------+
	 	 */
		riscv_b_cond_helper(write_p, (uint64_t)write_p + 8, cond);

	default:
		fprintf(stderr, "insert_cond_exit_branch(): unknown branch type\n");
		while(1);
	}
}

void dispatcher_aarch64(dbm_thread *thread_data, uint32_t source_index, 
	branch_type exit_type, uintptr_t target, uintptr_t block_address)
{
	
}