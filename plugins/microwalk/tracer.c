#ifdef PLUGINS_NEW
// Currently only for RISC-V targets
#ifdef __riscv

#include "tracer.h"
#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include <inttypes.h>
#include <string.h>

#include "trace_writer_wrapper.h"
#include "../../plugins.h"

#ifdef DEBUG
	#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
	#define debug(...)
#endif

#define INTERESTING_IMG_NR 2

// Tracer options
const bool enable_stack_allocation_tracking = true;

// Strings
char notify_testcase_start_name[] = "PinNotifyTestcaseStart";
char notify_testcase_end_name[] = "PinNotifyTestcaseEnd";
char notify_stack_pointer_name[] = "PinNotifyStackPointer";
char *interesting_images[INTERESTING_IMG_NR] = { // Must be absolute paths
	"/home/root/wrapper",
	"/usr/lib/libcrypto.so.1.1"
};
const int interesting_images_count = INTERESTING_IMG_NR;


TraceEntry *entry_buffer_next;
TraceEntry *entry_buffer_end;

TraceWriter *trace_writer;

int main_thread_id = -1;
int is_intresting = true;

int tracer_pre_thread_handler(mambo_context *ctx)
{
	// Only trace main thread, first thread is main thread
	if (main_thread_id == -1) {
		main_thread_id == ctx->thread_data->tid;

		entry_buffer_next = TraceWriter_Begin(trace_writer);
		entry_buffer_end = TraceWriter_End(trace_writer);
	} else {
		fprintf(stderr, "Ignoring thread #%d\n", ctx->thread_data->tid);
	}
}

int tracer_post_thread_handler(mambo_context *ctx)
{
	if (ctx->thread_data->tid == main_thread_id)
	{
		TraceWriter_WriteBufferToFile(trace_writer, entry_buffer_next);
		TraceWriter_destroy(trace_writer);
	}
}

int tracer_test_start_pre_fn_handler(mambo_context *ctx) {}

int tracer_test_start_post_fn_handler(mambo_context *ctx)
{
	/*
	 * Because this handler is called directly after another function returned, the
	 * original code does not expect anything in the volatile registers. Therefore
	 * there is no need pushing any function argument registers or using the safe
	 * function call.
	 */

	// Extract testcase ID from previous returned value
	emit_add_sub_i(ctx, reg0, reg0, -42);
	emit_set_reg_ptr(ctx, reg1, &entry_buffer_next);
	
	/* 
	 * void tracer_testcase_start_helper(
	 *     reg0: int testcase_id,
	 *     reg1: TraceEntry** next_entry
	 * )
	 */
	emit_fcall(ctx, tracer_testcase_start_helper);

	debug("    PinNotifyTestcaseStart() instrumented.\n");
}

int tracer_test_end_pre_fn_handler(mambo_context *ctx) 
{
	/*
	 * Because this handler is called directly after another function returned, the
	 * original code does not expect anything in the volatile registers. Therefore
	 * there is no need pushing any function argument registers or using the safe
	 * function call.
	 */

	emit_set_reg_ptr(ctx, reg0, &entry_buffer_next);

	/* 
	 * void tracer_testcase_end_helper(
	 *     reg0: TraceEntry** next_entry
	 * )
	 */
	emit_fcall(ctx, tracer_testcase_end_helper);

	debug("    PinNotifyTestcaseEnd() instrumented.\n");
}

int tracer_test_end_post_fn_handler(mambo_context *ctx) {}

int tracer_sp_notify_pre_fn_handler(mambo_context *ctx)
{
	emit_set_reg_ptr(ctx, reg2, &entry_buffer_next);

	/* 
	 * Param 0 and 1 unchanged from original call to PinNotifyStackPointer and there is
	 * no need to backup them because the result of PinNotifyStackPointer is ignored
	 * anyways.
	 * void tracer_sp_notify_helper(
	 * 		reg0: uintptr_t stack_pointer_min,
	 * 		reg1: uintptr_t stack_pointer_max,
	 * 		reg2: TraceEntry **next_entry
	 * )
	 */
	emit_fcall(ctx, tracer_sp_notify_helper);

	debug("    PinNotifyStackPointer() instrumented.\n");
}

int tracer_sp_notify_post_fn_handler(mambo_context *ctx) {}

int tracer_malloc_pre_fn_handler(mambo_context *ctx)
{
	emit_push(ctx, (1 << reg0));
	emit_set_reg_ptr(ctx, reg1, &entry_buffer_next);

	/* 
	 * void tracer_entry_helper_alloc_param(
	 * 		reg0: uint64_t size,
	 * 		reg1: TraceEntry **next_entry 
	 * )
	 */
	emit_fcall(ctx, tracer_entry_helper_alloc_param);
	
	emit_pop(ctx, (1 << reg0));
}

int tracer_malloc_post_fn_handler(mambo_context *ctx)
{
	emit_push(ctx, (1 << reg0));
	emit_set_reg_ptr(ctx, reg1, &entry_buffer_next);

	/*
	 * void tracer_entry_helper_alloc_return(
	 * 		reg0: uintptr_t memory_address, 
	 * 		reg1: TraceEntry **next_entry
	 * )
	 */
	emit_fcall(ctx, tracer_entry_helper_alloc_return);

	emit_pop(ctx, (1 << reg0));

	debug("    malloc() instrumented.\n");
}

int tracer_calloc_pre_fn_handler(mambo_context *ctx)
{
	emit_push(ctx, (1 << reg0) | (1 << reg1));
	emit_set_reg_ptr(ctx, reg2, &entry_buffer_next);

	/* 
	 * void tracer_entry_helper_calloc_param(
	 * 		reg0: uint64_t count, 
	 * 		reg1: uint64_t size, 
	 * 		reg2: TraceEntry **next_entry
	 * )
	 */
	emit_fcall(ctx, tracer_entry_helper_calloc_param);
	
	emit_pop(ctx, (1 << reg0) | (1 << reg1));
}

int tracer_calloc_post_fn_handler(mambo_context *ctx)
{
	tracer_malloc_post_fn_handler(ctx);

	debug("    calloc() instrumented.\n");
}

int tracer_realloc_pre_fn_handler(mambo_context *ctx)
{
	emit_push(ctx, (1 << reg0) | (1 << reg1));
	emit_mov(ctx, reg0, reg1);
	emit_set_reg_ptr(ctx, reg1, &entry_buffer_next);

	/* 
	 * void tracer_entry_helper_alloc_param(
	 * 		reg0: uint64_t size, 
	 * 		reg1: TraceEntry **next_entry
	 * )
	 */
	emit_fcall(ctx, tracer_entry_helper_alloc_param);
	
	emit_pop(ctx, (1 << reg0) | (1 << reg1));
}

int tracer_realloc_post_fn_handler(mambo_context *ctx)
{
	tracer_malloc_post_fn_handler(ctx);

	debug("    realloc() instrumented.\n");
}

int trace_free_pre_fn_handler(mambo_context *ctx)
{
	emit_push(ctx, (1 << reg0));
	emit_set_reg_ptr(ctx, reg1, &entry_buffer_next);

	/* 
	 * void tracer_entry_helper_free_param(
	 * 		reg0: uintptr_t memory_address,
	 * 		reg1: TraceEntry **next_entry
	 * )
	 */
	emit_fcall(ctx, tracer_entry_helper_alloc_param);
	
	emit_pop(ctx, (1 << reg0));

	debug("    free() instrumented.\n");
}

int trace_free_post_fn_handler(mambo_context *ctx) {}


int tracer_vm_op_handler(mambo_context *ctx)
{
	// Initialize the tracer write as early as possible
	if (trace_writer == NULL) {
		trace_writer = TraceWriter_new("");
		TraceWriter_InitPrefixMode("");
	}

	if (mambo_get_vm_op(ctx) == VM_MAP) {
		void *start_address;
		void *end_address;
		char *filename;
		int ret = get_image_info_by_addr(
			(uintptr_t)mambo_get_vm_addr(ctx), &start_address, &end_address, &filename
		);
		// Check if it is a new image
		if (ret < 0 || start_address != mambo_get_vm_addr(ctx))
			return 0;

		// Check if current image is in interesting images list
		int intr = false;
		for (int i = 0; i < interesting_images_count; i++) {
			if (strcmp(interesting_images[i], filename) == 0) {
				intr = true;
				break;
			}
		}

		TraceWriter_WriteImageLoadData(intr, (uint64_t)start_address, (uint64_t)end_address, filename);

		debug("[tracer] LOADED IMAGE: At address 0x%016lx - 0x%016lx, interesting  %d    in image %s\n", start_address, end_address, intr, filename);
	}
}

int tracer_pre_bb_handler(mambo_context *ctx)
{
	void *start_address;
	void *end_address;
	char *filename;
	get_image_info_by_addr((uintptr_t)mambo_get_source_addr(ctx), &start_address, &end_address, &filename);

	// Check if current image is in interesting images list
	is_intresting = false;
	for (int i = 0; i < interesting_images_count; i++) {
		if (strcmp(interesting_images[i], filename) == 0) {
			is_intresting = true;
			break;
		}
	}
}

int tracer_pre_inst_handler(mambo_context *ctx) 
{
	// Abort instrumentation if source address is in an interesting image to save time
	// (following MicroWalks procedure)
	if (!is_intresting)
		return 0;

	mambo_branch_type branch_type = mambo_get_branch_type(ctx);

	// Loads and Stores
	if (branch_type == BRANCH_NONE && mambo_is_load_or_store(ctx)) {
		tracer_write_start_trace_instrumentation(ctx, 132);

		debug("[tracer] Instrument load or store\n");
		emit_set_reg_ptr(ctx, reg0, &entry_buffer_next);
		emit_set_reg_ptr(ctx, reg1, ctx->code.read_address);
		mambo_calc_ld_st_addr(ctx, reg2);
		emit_set_reg(ctx, reg3, mambo_get_ld_st_size(ctx));
		
		if (mambo_is_load(ctx))
			/*
			 * void tracer_entry_helper_read(
			 *     reg0: TraceWriter **next_entry, 
			 *     reg1: uintptr_t instruction_address, 
			 *     reg2: uintptr_t memory_address, 
			 *     reg3: uint32_t size
			 * )
			 */
			emit_safe_fcall(ctx, tracer_entry_helper_read, 4);
		else
			/*
			 * void tracer_entry_helper_write(
			 *     reg0: TraceWriter **next_entry, 
			 *     reg1: uintptr_t instruction_address, 
			 *     reg2: uintptr_t memory_address, 
			 *     reg3: uint32_t size
			 * )
			 */
			emit_safe_fcall(ctx, tracer_entry_helper_write, 4);

		tracer_write_end_trace_instrumentation(ctx);

	} else if (branch_type == (BRANCH_DIRECT | BRANCH_COND)) {
		tracer_write_start_trace_instrumentation(ctx, 152);

		debug("[tracer] Instrument conditional branches\n");
		// Handle all conditional branches
		uint64_t offset;

		emit_push(ctx, (1 << reg4));
		if (mambo_get_inst_len(ctx) == INST_16BIT) {
			enum reg rs1;
			unsigned int immhi, immlo;

			riscv_c_beqz_decode_fields(ctx->code.read_address, &rs1, &immhi, &immlo);
			riscv_calc_cb_imm(immhi, immlo, offset);
			offset = sign_extend64(9, offset);

		} else {
			enum reg rs1, rs2;
			unsigned int immhi, immlo;

			riscv_beq_decode_fields(ctx->code.read_address, &rs1, &rs2, &immhi, &immlo);
			riscv_calc_b_imm(immhi, immlo, offset);
			offset = sign_extend64(13, offset);
		}

		// Do taken detection first before potentially overwriting the source registers
		mambo_cond cond = mambo_get_cond(ctx);
		// Insert NOP on case 16 bit branch gets inserted
		*((uint16_t *)ctx->code.write_p + 1) = 0x0001;
		if (emit_branch_cond(ctx, mambo_get_cc_addr(ctx) + 8, cond) == INST_16BIT)
			mambo_set_cc_addr(ctx, mambo_get_cc_addr(ctx) + INST_16BIT);
		emit_riscv_c_li(ctx, reg3, 0, 0);
		emit_riscv_c_j(ctx, 4);
		emit_riscv_c_li(ctx, reg3, 0, 1);

		emit_set_reg_ptr(ctx, reg0, &entry_buffer_next);
		emit_set_reg_ptr(ctx, reg1, ctx->code.read_address);
		emit_set_reg(ctx, reg2, (uintptr_t)ctx->code.read_address + offset);
		emit_set_reg(ctx, reg4, TraceEntryFlags_BranchTypeJump);

		/*
		 * void tracer_entry_helper_branch(
		 *     reg0: TraceEntry **next_entry, 
		 *     reg1: uintptr_t source_address, 
		 *     reg2: uintptr_t target_address, 
		 *     reg3: uint8_t taken, 
		 *     reg4: uint8_t type
		 * )
		 */
		emit_safe_fcall(ctx, tracer_entry_helper_branch, 5);
		emit_pop(ctx, (1 << reg4));

		tracer_write_end_trace_instrumentation(ctx);

	} else if (branch_type & BRANCH_DIRECT) {
		tracer_write_start_trace_instrumentation(ctx, 148);

		debug("[tracer] Instrument direct jumps\n");
		// Handle JAL, C.JAL and C.J (and call)
		uint64_t offset;
		enum reg rd;
		unsigned int imm;

		if (mambo_get_inst_len(ctx) == INST_16BIT)  { //C.JAL, C.J
			riscv_c_jal_decode_fields(ctx->code.read_address, &imm);
			riscv_calc_cj_imm(imm, offset);
			offset = sign_extend64(12, offset);
		} else { // JAL
			riscv_jal_decode_fields(ctx->code.read_address, &rd, &imm);
			riscv_calc_j_imm(imm, offset);
			offset = sign_extend64(21, offset);
		}

		emit_set_reg_ptr(ctx, reg0, &entry_buffer_next);
		emit_set_reg_ptr(ctx, reg1, ctx->code.read_address);
		emit_set_reg(ctx, reg2, (uintptr_t)ctx->code.read_address + offset);
		emit_set_reg(ctx, reg3, TraceEntryFlags_BranchTypeJump);

		/*
		 * void tracer_entry_helper_jump(
		 *     reg0: TraceEntry **next_entry, 
		 *     reg1: uintptr_t source_address, 
		 *     reg2: uintptr_t target_address, 
		 *     reg3: uint8_t type
		 * )
		 */
		emit_safe_fcall(ctx, tracer_entry_helper_jump, 4);

		tracer_write_end_trace_instrumentation(ctx);

	} else if (branch_type & BRANCH_INDIRECT) {
		tracer_write_start_trace_instrumentation(ctx, 130);

		debug("[tracer] Instrument indirect jumps\n");
		// Handle JALR, C.JALR and C.JR (and ret)
		enum reg rd, rs1;
		unsigned int imm;

		if (mambo_get_inst_len(ctx) == INST_16BIT) { // C.JALR, C.JR
			riscv_c_jalr_decode_fields(ctx->code.read_address, &rs1);
			emit_mov(ctx, reg2, rs1);
		} else { // JALR
			uint64_t offset;

			riscv_jalr_decode_fields(ctx->code.read_address, &rd, &rs1, &imm);
			offset = sign_extend64(12, imm);

			emit_mov(ctx, reg2, rs1);
			emit_add_sub_i(ctx, reg2, reg2, offset);
		}

		emit_set_reg_ptr(ctx, reg0, &entry_buffer_next);
		emit_set_reg_ptr(ctx, reg1, ctx->code.read_address);
		emit_set_reg(ctx, reg3, TraceEntryFlags_BranchTypeJump);

		/*
		 * void tracer_entry_helper_jump(
		 *     reg0: TraceEntry **next_entry, 
		 *     reg1: uintptr_t source_address, 
		 *     reg2: uintptr_t target_address, 
		 *     reg3: uint8_t type
		 * )
		 */
		emit_safe_fcall(ctx, tracer_entry_helper_jump, 4);

		tracer_write_end_trace_instrumentation(ctx);
	}
}

int tracer_post_inst_handler(mambo_context *ctx)
{
	// Abort instrumentation if source address is in an interesting image to save time
	// (following MicroWalks procedure)
	if (!is_intresting || !enable_stack_allocation_tracking)
		return 0;

	mambo_branch_type branch_type = mambo_get_branch_type(ctx);
	enum reg rd = 0;
	if (!mambo_is_load_or_store(ctx) && branch_type == BRANCH_NONE) {
		/* 
		 * Explict memory operations (load and stores) and conditional branches are
		 * already handled in pre instruction handler. At this point, the current 
		 * instruction is either encoded in the R, I, U or J format where the rd field
		 * is always at the same location, or it is a compressed instruction. The only
		 * compressed instructions that can modifiy the stack pointer are CI- and 
		 * CR-type instructions also having the rd field always at the same location.
		 */
		if (mambo_get_inst_len(ctx) == INST_32BIT) {
			unsigned int rs1, imm;
			riscv_ld_decode_fields(mambo_get_source_addr(ctx), &rd, &rs1, &imm);

		} else { // 16 bit instructions
			switch(mambo_get_inst(ctx)) {
			case RISCV_C_LI: // CI
			case RISCV_C_ADDI: // CI
			case RISCV_C_ADDIW: // CI
			case RISCV_C_ADDI16SP: // CI
			case RISCV_C_SLLI: // CI
			case RISCV_C_MV:  // CR
			case RISCV_C_ADD: { // CR
				unsigned rs2;
				riscv_c_add_decode_fields(mambo_get_source_addr(ctx), &rd, &rs2);
			}
			}
		}

		if (rd == sp) {
			tracer_write_start_trace_instrumentation(ctx, 124);

			debug("[tracer] Instrument stack pointer modification\n");
			emit_set_reg_ptr(ctx, reg0, &entry_buffer_next);
			emit_set_reg_ptr(ctx, reg1, ctx->code.read_address);
			emit_add_sub_i(ctx, reg2, sp, ctx->code.plugin_pushed_reg_count * sizeof(uintptr_t));
			emit_set_reg(ctx, reg3, TraceEntryFlags_StackIsOther);

			/* void tracer_entry_helper_stack_mod(
				* 		reg0: TraceEntry **next_entry, 
				* 		reg1: uintptr_t instruction_address,
				* 		reg2: uintptr_t new_stack_pointer, 
				* 		reg3: uint8_t flags
				* )
				*/
			emit_safe_fcall(ctx, tracer_entry_helper_stack_mod, 4);

			tracer_write_end_trace_instrumentation(ctx);
		}
	}
}

__attribute__((constructor)) void branch_count_init_plugin() 
{
	mambo_context *ctx = mambo_register_plugin();
	assert(ctx != NULL);

	// Register target wrapper function callbacks
	mambo_register_function_cb(ctx, notify_testcase_start_name, 
		&tracer_test_start_pre_fn_handler, &tracer_test_start_post_fn_handler, 1);
	mambo_register_function_cb(ctx, notify_testcase_end_name, 
		&tracer_test_end_pre_fn_handler, &tracer_test_end_post_fn_handler, 0);
	mambo_register_function_cb(ctx, notify_stack_pointer_name, 
		&tracer_sp_notify_pre_fn_handler, &tracer_sp_notify_post_fn_handler, 2);

	// Register allocation functions from libc
	mambo_register_function_cb(ctx, "malloc", &tracer_malloc_pre_fn_handler, &tracer_malloc_post_fn_handler, 1);
	mambo_register_function_cb(ctx, "calloc", &tracer_calloc_pre_fn_handler, &tracer_calloc_post_fn_handler, 2);
	mambo_register_function_cb(ctx, "realloc", &tracer_realloc_pre_fn_handler, &tracer_realloc_post_fn_handler, 2);
	mambo_register_function_cb(ctx, "free", &trace_free_pre_fn_handler, &trace_free_post_fn_handler, 1);
	
	mambo_register_pre_thread_cb(ctx, &tracer_pre_thread_handler);
	mambo_register_post_thread_cb(ctx, &tracer_post_thread_handler);
	mambo_register_pre_inst_cb(ctx, &tracer_pre_inst_handler);
	mambo_register_post_inst_cb(ctx, &tracer_post_inst_handler);

	mambo_register_pre_basic_block_cb(ctx, &tracer_pre_bb_handler);
	mambo_register_vm_op_cb(ctx, &tracer_vm_op_handler);

	setlocale(LC_NUMERIC, "");
}

void tracer_write_start_trace_instrumentation(mambo_context *ctx, int size)
{
	riscv_check_free_space(ctx->thread_data, (uint16_t **)&ctx->code.write_p, (uint16_t **)&ctx->code.data_p, size, ctx->code.fragment_id);
	emit_push(ctx, (1 << reg0) | (1 << reg1) | (1 << reg2) | (1 << reg3));
}

void tracer_write_end_trace_instrumentation(mambo_context *ctx)
{
	emit_pop(ctx, (1 << reg0) | (1 << reg1) | (1 << reg2) | (1 << reg3));
}

void tracer_testcase_start_helper(int testcase_id, TraceEntry **next_entry)
{
	TraceWriter_TestcaseStart(trace_writer, testcase_id, *next_entry);
	*next_entry = TraceWriter_Begin(trace_writer);
}

void tracer_testcase_end_helper(TraceEntry **next_entry)
{
	TraceWriter_TestcaseEnd(trace_writer, *next_entry);
	*next_entry = TraceWriter_Begin(trace_writer);
}

TraceEntry *tracer_check_buffer_and_store_helper(TraceEntry *next_entry)
{
	TraceEntry *entry_buffer_end = TraceWriter_End(trace_writer);
	if (next_entry == 0 || entry_buffer_end == 0)
		return next_entry;

	if (TraceWriter_CheckBufferFull(next_entry, entry_buffer_end)) {
		TraceWriter_WriteBufferToFile(trace_writer, entry_buffer_end);
		return TraceWriter_Begin(trace_writer);
	}
	return next_entry;
}

void tracer_sp_notify_helper(uintptr_t stack_pointer_min, uintptr_t stack_pointer_max, TraceEntry **next_entry)
{
	*next_entry = TraceWriter_InsertStackPointerInfoEntry(*next_entry, stack_pointer_min, stack_pointer_max);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_read(TraceEntry **next_entry, uintptr_t instruction_address, uintptr_t memory_address, uint32_t size)
{
	*next_entry = TraceWriter_InsertMemoryReadEntry(*next_entry, instruction_address, memory_address, size);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_write(TraceEntry **next_entry, uintptr_t instruction_address, uintptr_t memory_address, uint32_t size)
{
	*next_entry = TraceWriter_InsertMemoryWriteEntry(*next_entry, instruction_address, memory_address, size);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_branch(TraceEntry **next_entry, uintptr_t source_address, uintptr_t target_address, uint8_t taken, uint8_t type)
{
	*next_entry = TraceWriter_InsertBranchEntry(*next_entry, source_address, target_address, taken, type);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_jump(TraceEntry **next_entry, uintptr_t source_address, uintptr_t target_address, uint8_t type)
{
	*next_entry = TraceWriter_InsertJumpEntry(*next_entry, source_address, target_address, type);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_stack_mod(TraceEntry **next_entry, uintptr_t instruction_address, uintptr_t new_stack_pointer, uint8_t flags)
{
	*next_entry = TraceWriter_InsertStackPointerModificationEntry(*next_entry, instruction_address, new_stack_pointer, flags);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_alloc_param(uint64_t size, TraceEntry **next_entry)
{
	*next_entry = TraceWriter_InsertHeapAllocSizeParameterEntry(*next_entry, size);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_alloc_return(uintptr_t memory_address, TraceEntry **next_entry)
{
	*next_entry = TraceWriter_InsertHeapAllocAddressReturnEntry(*next_entry, memory_address);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_calloc_param(uint64_t count, uint64_t size, TraceEntry **next_entry)
{
	*next_entry = TraceWriter_InsertCallocSizeParameterEntry(*next_entry, count, size);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

void tracer_entry_helper_free_param(uintptr_t memory_address, TraceEntry **next_entry)
{
	*next_entry = TraceWriter_InsertHeapFreeAddressParameterEntry(*next_entry, memory_address);
	*next_entry = tracer_check_buffer_and_store_helper(*next_entry);
}

#endif
#endif
