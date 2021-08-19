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

void insert_cond_exit_branch(dbm_code_cache_meta *bb_meta, uint16_t **write_p, 
	mambo_cond *cond)
{
	switch (bb_meta->exit_branch_type) {
	case uncond_imm_riscv:
	case uncond_reg_riscv:
		return;
	case cond_imm_riscv:
		/*
		 * Overwrite code written by riscv_branch_jump_cond
	 	 * 				+-------------------------------+
	 	 * 	 	NEW	 |- |	B(cond)	rs1, rs2, .+8		|	previously NOP
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
		riscv_b_cond_helper(write_p, (uint64_t)*write_p + 8, cond);
		break;

	default:
		fprintf(stderr, "insert_cond_exit_branch(): unknown branch type\n");
		while(1);
	}
}

void dispatcher_riscv(dbm_thread *thread_data, uint32_t source_index, 
	branch_type exit_type, uintptr_t target, uintptr_t block_address)
{
	uint16_t *branch_addr;
	bool is_taken;
	uintptr_t other_target;
	bool other_target_in_cache;
	mambo_cond cond;

	branch_addr = thread_data->code_cache_meta[source_index].exit_branch_addr;

	switch (exit_type) {
	#ifdef DBM_LINK_UNCOND_IMM
	case uncond_imm_riscv:
		/*
		 * Overwrite code written by riscv_branch_jump_cond to jump directly to the
		 * target block.
	 	 * 				+-------------------------------+
	 	 * 	 	NEW		|	JAL		x0, block_address+4	|	previously NOP
	 	 * 			##	|	NOP							|	(everything else unchanged)
	 	 * 			##	|								|
	 	 * 			##	|	PUSH	x10, x11			|
	 	 * 			##	|	LI		x11, basic_block 	|
	 	 * 			##	|								|
	 	 * 			##	|	B(cond)	branch_target:		|
	 	 * 			##	|								|
	 	 * 			##	|	LI		x10, read_address+len
	 	 * 			##	|	JAL		DISPATCHER			|
	 	 * 			##	|								|
	 	 * 			##	| branch_target:				|
	 	 * 			##	|	LI		x10, target			|
	 	 * 			##	|	JAL		DISPATCHER			|
	 	 * 				+-------------------------------+
		 * 
		 * ## dead code
	 	 */
		riscv_cc_branch(thread_data, branch_addr, block_address + 4);
		__clear_cache((void *)branch_addr, (void *)branch_addr + 4 + 1);
		thread_data->code_cache_meta[source_index].branch_cache_status = BRANCH_LINKED;
		break;
	#endif
	#ifdef DBM_LINK_COND_IMM
    case cond_imm_riscv:
		is_taken = 
			(target == thread_data->code_cache_meta[source_index].branch_taken_addr);

		// Link target if not linked yet
		if (thread_data->code_cache_meta[source_index].branch_cache_status == 0) {
			if (is_taken)
				other_target = 
					thread_data->code_cache_meta[source_index].branch_skipped_addr;
			else
				other_target = 
					thread_data->code_cache_meta[source_index].branch_taken_addr;
			other_target = cc_lookup(thread_data, other_target);
			other_target_in_cache = (other_target != UINT_MAX);

			// Configure skip condition
			cond = thread_data->code_cache_meta[source_index].branch_condition;
			if (is_taken)
				cond.cond = invert_cond(cond.cond);
			insert_cond_exit_branch(&thread_data->code_cache_meta[source_index], 
				&branch_addr, &cond);
			
			thread_data->code_cache_meta[source_index].branch_cache_status =
				(is_taken ? BRANCH_LINKED : FALLTHROUGH_LINKED);
		} else {
			branch_addr += 4;
			other_target_in_cache = false;
			thread_data->code_cache_meta[source_index].branch_cache_status |= BOTH_LINKED;
		}

		/*
		 * Overwrite secound NOP from riscv_branch_jump_cond with branch to block address
	 	 * 				+-------------------------------+
	 	 * 		**	 |- |	B(cond)	rs1, rs2, .+8		|
	 	 * 		NEW	 |	|	JAL		block_address+4		|	previously NOP
	 	 * 			 |	|								|
	 	 * 			 -> |	PUSH	x10, x11			|	(everything else unchanged)
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
		 * 
		 * ** if conditional exit
	 	 */
		riscv_cc_branch(thread_data, branch_addr, block_address + 4);

		if (other_target_in_cache) {
			/*
			* Overwrite code written by riscv_branch_jump_cond to jump to the other 
			* target. Both targets are linked now, so the remaining code (jumping to
			* the dispatcher) becomes obsolete.
			* 			+-------------------------------+
			* 	**	 |- |	B(cond)	rs1, rs2, .+8		|
			* 		 |	|	JAL		block_address+4		|
			* 		 |	|								|
			* 	NEW	 -> |	JAL		other_target+4		|	previously PUSH	x10, x11
			* 		##	|	LI		x11, basic_block 	|	(everything else unchanged)
			* 		##	|								|
			* 		##	|	B(cond)	branch_target:		|
			* 		##	|								|
			* 		##	|	LI		x10, read_address+len
			* 		##	|	JAL		DISPATCHER			|
			* 		##	|								|
			* 		##	| branch_target:				|
			* 		##	|	LI		x10, target			|
			* 		##	|	JAL		DISPATCHER			|
			* 			+-------------------------------+
			*
			* ** if conditional exit
			* ## dead code
			*/ 
			riscv_cc_branch(thread_data, branch_addr, other_target + 4);
			thread_data->code_cache_meta[source_index].branch_cache_status |= BOTH_LINKED;
		}

		__clear_cache(
			(void *)thread_data->code_cache_meta[source_index].exit_branch_addr, 
			(void *)branch_addr);
		break;
  	#endif
	}
}