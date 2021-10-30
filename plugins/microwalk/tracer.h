#ifndef __TRACER_H
#define __TRACER_H

#include <inttypes.h>
#include "trace_writer_wrapper.h"
#include "../../plugins.h"

#define riscv_calc_j_imm(rawimm, value) value = (rawimm & (1 << 19)) << 1 \
												| (rawimm & (0x3FF << 9)) >> 8 \
												| (rawimm & (1 << 8)) << 3 \
												| (rawimm & (0xFF)) << 12
#define riscv_calc_cj_imm(rawimm, value) value = (rawimm & (1 << 10)) << 1 \
												| (rawimm & (1 << 9)) >> 5 \
												| (rawimm & (3 << 7)) << 1 \
												| (rawimm & (1 << 6)) << 4 \
												| (rawimm & (1 << 5)) << 1 \
												| (rawimm & (1 << 4)) << 3 \
												| rawimm & (7 << 1) \
												| (rawimm & 1) << 5
#define riscv_calc_b_imm(rawimmhi, rawimmlo, value) value = (rawimmhi & (1 << 6)) << 6 \
														   | (rawimmhi & 0x3F) << 5 \
														   | (rawimmlo & (0xF << 1)) \
														   | (rawimmlo & 1) << 11
#define riscv_calc_cb_imm(rawimmhi, rawimmlo, value) value = (rawimmhi & (1 << 2)) << 6 \
															| (rawimmhi & 3) << 3 \
															| (rawimmlo & (3 << 3)) << 3 \
															| (rawimmlo & (3 << 1)) \
															| (rawimmlo & 1) << 5
#define riscv_calc_u_imm(rawimm, value) value = (rawimm & 0xFFFFF) << 12

/**
 * Emit procedure at the beginning of an instrumentation.
 * @param ctx MAMBO context.
 * @param size Size of the inserted code (bytes).
 */
void tracer_write_start_trace_instrumentation(mambo_context *ctx, int size);

/**
 * Emit procedure at the end of an instrumentation.
 * @param ctx MAMBO context.
 */
void tracer_write_end_trace_instrumentation(mambo_context *ctx);

/**
 * Handles beginning of a trace.
 * A call to this function can be inserted into the original code.
 * @param testcase_id Test case ID.
 * @param next_entry Pointer to next trace entry pointer.
 */
void tracer_testcase_start_helper(int testcase_id, TraceEntry **next_entry);

/**
 * Handles ending of a trace.
 * A call to this function can be inserted into the original code.
 * @param next_entry Pointer to trace next trace entry pointer.
 */
void tracer_testcase_end_helper(TraceEntry **next_entry);

/**
 * Check if buffer is full and write it to file if it is.
 * @param next_entry Pointer to trace next trace entry pointer.
 * @return Pointer to next trace entry.
 */
TraceEntry *tracer_check_buffer_and_store_helper(TraceEntry *next_entry);

/**
 * @param stack_pointer_min Minimum stack pointer address.
 * @param stack_pointer_max Maximum stack pointer address.
 * @param next_entry Pointer to next trace entry pointer.
 */
void tracer_sp_notify_helper(uintptr_t stack_pointer_min, uintptr_t stack_pointer_max, TraceEntry **next_entry);

/**
 * Calls tracer writer function and checks if buffer full.
 * A call to this function can be inserted into the original code.
 * @param next_entry Pointer to next trace entry pointer.
 * @param instruction_address Source address of current instruction.
 * @param memory_address Target address in memory.
 * @param size Size of memory read.
 */
void tracer_entry_helper_read(TraceEntry **next_entry, uintptr_t instruction_address, uintptr_t memory_address, uint32_t size);

/**
 * Calls tracer writer function and checks if buffer full.
 * A call to this function can be inserted into the original code.
 * @param next_entry Pointer to next trace entry pointer.
 * @param instruction_address Source address of current instruction.
 * @param memory_address Target address in memory.
 * @param size Size of memory write.
 */
void tracer_entry_helper_write(TraceEntry **next_entry, uintptr_t instruction_address, uintptr_t memory_address, uint32_t size);

/**
 * Calls tracer writer function and checks if buffer full.
 * A call to this function can be inserted into the original code.
 * @param next_entry Pointer to next trace entry pointer.
 * @param source_address Source address of current instruction.
 * @param target_address Target jump address.
 * @param taken If branch is taken.
 * @param type Branch type.
 */
void tracer_entry_helper_branch(TraceEntry **next_entry, uintptr_t source_address, uintptr_t target_address, uint8_t taken, uint8_t type);

/**
 * Calls tracer writer function and checks if buffer full.
 * A call to this function can be inserted into the original code.
 * @param next_entry Pointer to next trace entry pointer.
 * @param source_address Source address of current instruction.
 * @param target_address Target jump address.
 * @param type Branch type 
 */
void tracer_entry_helper_jump(TraceEntry **next_entry, uintptr_t source_address, uintptr_t target_address, uint8_t type);

/**
 * Calls tracer writer function and checks if buffer full.
 * A call to this function can be inserted into the original code.
 * @param next_entry Pointer to next trace entry pointer.
 * @param instruction_address Source address of current instruction.
 * @param new_stack_pointer New value of the stack pointer.
 * @param flags Stack pointer manipulation flags.
 */
void tracer_entry_helper_stack_mod(TraceEntry **next_entry, uintptr_t instruction_address, uintptr_t new_stack_pointer, uint8_t flags);

#endif