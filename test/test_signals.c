#include <stdio.h>
#include <asm/unistd.h>
#include <string.h>
#include <ucontext.h>

// Module under test
#include "../pie/pie-riscv-decoder.h"
#include "../signals.c"

#include "unity/unity.h"

// DEBUG: Turn off debug
#define DEBUG
#ifdef DEBUG
	#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
	#define debug(...)
#endif

/* 
 * NOTE: Test for RISC-V and only compiles for target platform due to included
 * assembler files.
 */

void setUp(void) {}
void tearDown(void) {}

//dbm_global global_data;
//__thread dbm_thread *current_thread;

void test_inst_size()
{
	riscv_instruction inst;

	// ADDI is 32 bit
	inst = RISCV_ADDI;
	TEST_ASSERT_EQUAL(INST_32BIT, inst_size(inst, NULL));

	// C.ADDI is 16 bit
	inst = RISCV_C_ADDI;
	TEST_ASSERT_EQUAL(INST_16BIT, inst_size(inst, NULL));

	// C.SDSP is the last compressed (16 bit) instruction in the enum
	inst = RISCV_C_SDSP;
	TEST_ASSERT_EQUAL(INST_16BIT, inst_size(inst, NULL));

	// LUI is the first 32 bit instruction in the enum
	inst = RISCV_LUI;
	TEST_ASSERT_EQUAL(INST_32BIT, inst_size(inst, NULL));
}

void test_write_trap()
{
	uint16_t w[3] = {0};
	void *write_p = w;

	write_trap(SIGNAL_TRAP_IB);
	TEST_ASSERT_EQUAL_HEX32(RISCV_SRET_CODE, *(uint32_t *)w);
	TEST_ASSERT_EQUAL_PTR(&w[2], write_p);

	w[0] = 0;
	w[1] = 0;
	write_p = w;
	write_trap(SIGNAL_TRAP_DB);
	TEST_ASSERT_EQUAL_HEX32(RISCV_MRET_CODE, *(uint32_t *)w);
	TEST_ASSERT_EQUAL_PTR(&w[2], write_p);
}

void test_direct_branch()
{
	uint16_t w[3] = {0};
	uint16_t *write_p = w;

	uint16_t w_exp[3] = {0};
	uint16_t *write_p_exp = w_exp;

	// Tested function
	direct_branch(&write_p, (uint64_t)write_p + 100, NULL);

	// Expexted behaviour
	riscv_branch_imm_helper(&write_p_exp, (uint64_t)write_p_exp + 100, false);

	TEST_ASSERT_EQUAL_HEX16_ARRAY(w, w_exp, 3);
	TEST_ASSERT_EQUAL_PTR(&w[1], write_p);
}

void test_restore_ihl_inst()
{
	uint16_t w[3] = {0};
	void *write_p = w;

	uint16_t w_exp[3] = {0};
	void *write_p_exp = w_exp;

	// Tested function
	restore_ihl_inst(write_p);

	// Expexted behaviour
	riscv_jalr((uint16_t **)&write_p_exp, x0, x10, 0);

	TEST_ASSERT_EQUAL_HEX16_ARRAY(w, w_exp, 3);
	TEST_ASSERT_EQUAL_PTR(w, write_p);
}

void test_install_system_sig_handlers()
{
	struct sigaction orgact, act;
	sigaction(UNLINK_SIGNAL, NULL, &orgact);

	install_system_sig_handlers();

	// Get current signal handler and install the old handler
	sigaction(UNLINK_SIGNAL, &orgact, &act);
	if (act.sa_sigaction == signal_trampoline)
		TEST_PASS();
	else
		TEST_FAIL_MESSAGE("`signal_trampoline` was not the signal handler.");
}

void test_deliver_signals()
{
	// Setup test environment
	dbm_thread *thread_data;
	if (!allocate_thread_data(&thread_data)) {
		TEST_FAIL_MESSAGE("Failed to allocate initial thread data");
	}
	current_thread = thread_data;

	self_signal sig, sig_exp;
	global_data.exit_group = 0;
	current_thread->pending_signals[SIGILL] = 0;
	current_thread->is_signal_pending = 0;

	// Test no signal pending
	sig.pid = 0;
	sig.tid = 0;
	sig.signo = 0;
	sig_exp.pid = syscall(__NR_getpid);
	sig_exp.tid = syscall(__NR_gettid);
	sig_exp.signo = SIGILL;

	TEST_ASSERT_EQUAL(0, deliver_signals(0, &sig));
	TEST_ASSERT_EQUAL(0, current_thread->is_signal_pending);
	TEST_ASSERT_EQUAL(0, current_thread->pending_signals[SIGILL]);
	TEST_ASSERT_EQUAL(0, sig.pid);
	TEST_ASSERT_EQUAL(0, sig.tid);
	TEST_ASSERT_EQUAL(0, sig.signo);

	// Test 2 signals pending
	current_thread->pending_signals[SIGILL] = 2;
	current_thread->is_signal_pending = 2;

	TEST_ASSERT_EQUAL(1, deliver_signals(0, &sig));
	TEST_ASSERT_EQUAL(1, current_thread->is_signal_pending);
	TEST_ASSERT_EQUAL(1, current_thread->pending_signals[SIGILL]);
	TEST_ASSERT_EQUAL(sig_exp.pid, sig.pid);
	TEST_ASSERT_EQUAL(sig_exp.tid, sig.tid);
	TEST_ASSERT_EQUAL(sig_exp.signo, sig.signo);

	TEST_ASSERT_EQUAL(1, deliver_signals(0, &sig));
	TEST_ASSERT_EQUAL(0, current_thread->is_signal_pending);
	TEST_ASSERT_EQUAL(0, current_thread->pending_signals[SIGILL]);
	TEST_ASSERT_EQUAL(sig_exp.pid, sig.pid);
	TEST_ASSERT_EQUAL(sig_exp.tid, sig.tid);
	TEST_ASSERT_EQUAL(sig_exp.signo, sig.signo);

	sig.pid = 0;
	sig.tid = 0;
	sig.signo = 0;
	TEST_ASSERT_EQUAL(0, deliver_signals(0, &sig));
	TEST_ASSERT_EQUAL(0, current_thread->is_signal_pending);
	TEST_ASSERT_EQUAL(0, current_thread->pending_signals[SIGILL]);
	TEST_ASSERT_EQUAL(0, sig.pid);
	TEST_ASSERT_EQUAL(0, sig.tid);
	TEST_ASSERT_EQUAL(0, sig.signo);
}

void test_riscv_sret()
{
	uint16_t w[3] = {0};
	uint16_t *write_p = w;

	riscv_sret(&write_p);
	TEST_ASSERT_EQUAL_HEX32(0x10200073, *(uint32_t *)w);
	TEST_ASSERT_EQUAL_HEX16(0, w[2]);
	TEST_ASSERT_EQUAL_PTR(w, write_p);
}

void test_riscv_mret()
{
	uint16_t w[3] = {0};
	uint16_t *write_p = w;

	riscv_mret(&write_p);
	TEST_ASSERT_EQUAL_HEX32(0x30200073, *(uint32_t *)w);
	TEST_ASSERT_EQUAL_HEX16(0, w[2]);
	TEST_ASSERT_EQUAL_PTR(w, write_p);
}

void test_unlink_indirect_branch()
{
	uint16_t w[8] = {0};
	void *write_p = w;

	// Test trap instruction already written
	*(uint32_t *)w = 0x10200073;
	TEST_ASSERT_FALSE(unlink_indirect_branch(NULL, (void **)&write_p));
	TEST_ASSERT_EQUAL_PTR(w, write_p);

	*(uint32_t *)w = 0x30200073;
	TEST_ASSERT_FALSE(unlink_indirect_branch(NULL, (void **)&write_p));
	TEST_ASSERT_EQUAL_PTR(w, write_p);

	// Test some other situations
	w[0] = 0x87aa;	// MV a5, a0
	w[1] = 0x3823;	// SD a1, -304(s0)
	w[2] = 0xecb4;
	w[3] = 0x0793;	// LI a5, 64
	w[4] = 0x0400;
	w[5] = 0x0067;	// JALR x0, 0(x0)
	w[6] = 0x0000;
	TEST_ASSERT_TRUE(unlink_indirect_branch(NULL, (void **)&write_p));
	TEST_ASSERT_EQUAL_PTR(&w[7], write_p);
	uint16_t w_exp[8] = {0x87aa, 0x3823, 0xecb4, 0x0793, 0x0400};
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 5);
	TEST_ASSERT_EQUAL_HEX32(0x10200073, *(uint32_t *)&w[5]); // SRET for indirect jump
}

void test_get_direct_branch_exit_trap_sz()
{
	dbm_code_cache_meta bb_meta;

	bb_meta.exit_branch_type = uncond_imm_riscv;
	TEST_ASSERT_EQUAL(4, get_direct_branch_exit_trap_sz(&bb_meta, 0));

	bb_meta.exit_branch_type = cond_imm_riscv;
	bb_meta.branch_cache_status = 0;
	TEST_ASSERT_EQUAL(8, get_direct_branch_exit_trap_sz(&bb_meta, 0));
	bb_meta.branch_cache_status = FALLTHROUGH_LINKED;
	TEST_ASSERT_EQUAL(8, get_direct_branch_exit_trap_sz(&bb_meta, 0));
	bb_meta.branch_cache_status = BRANCH_LINKED;
	TEST_ASSERT_EQUAL(8, get_direct_branch_exit_trap_sz(&bb_meta, 0));
	bb_meta.branch_cache_status = BOTH_LINKED;
	TEST_ASSERT_EQUAL(12, get_direct_branch_exit_trap_sz(&bb_meta, 0));
}

void test_unlink_direct_branch()
{
	dbm_code_cache_meta bb_meta;
	uint16_t w[8] = {1,2,3,4,5,6,7,8};
	void *write_p = w;

	uint16_t w_exp[8] = {1,2,3,4,5,6,7,8};

	bb_meta.exit_branch_type = uncond_imm_riscv;
	bb_meta.branch_cache_status = 0;
	bb_meta.exit_branch_addr = write_p;
	memset(bb_meta.saved_exit, 0, MAX_SAVED_EXIT_SZ);
	TEST_ASSERT_FALSE(unlink_direct_branch(&bb_meta, &write_p, 0, (uintptr_t)write_p + 16));
	TEST_ASSERT_EQUAL_PTR(w, write_p);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w, w_exp, 8);

	TEST_ASSERT_TRUE(unlink_direct_branch(&bb_meta, &write_p, 0, (uintptr_t)write_p));
	TEST_ASSERT_EQUAL_PTR(w, write_p);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w, w_exp, 8);

	bb_meta.branch_cache_status = FALLTHROUGH_LINKED;
	*(uint32_t *)w_exp = 0x30200073;
	uint16_t saved_exp[6] = {1, 2, 0, 0, 0, 0};
	TEST_ASSERT_TRUE(unlink_direct_branch(&bb_meta, &write_p, 0, (uintptr_t)write_p));
	TEST_ASSERT_EQUAL_PTR(&w[2], write_p);
	TEST_ASSERT_EQUAL_HEX32_ARRAY(w_exp, w, 4);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(saved_exp, bb_meta.saved_exit, 6);

	write_p = w;
	bb_meta.exit_branch_type = cond_imm_riscv;
	TEST_ASSERT_FALSE(unlink_direct_branch(&bb_meta, &write_p, 0, (uintptr_t)write_p));
	TEST_ASSERT_EQUAL_PTR(w, write_p);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w, w_exp, 8);

	write_p = w;
	w[0] = 1;
	w[1] = 2;
	w[2] = 3;
	w[3] = 4;
	bb_meta.exit_branch_type = cond_imm_riscv;
	*(uint32_t *)w_exp = 0x30200073;
	*((uint32_t *)w_exp + 1) = 0x30200073;
	saved_exp[2] = 3;
	saved_exp[3] = 4;
	TEST_ASSERT_TRUE(unlink_direct_branch(&bb_meta, &write_p, 0, (uintptr_t)write_p));
	TEST_ASSERT_EQUAL_PTR(&w[4], write_p);
	TEST_ASSERT_EQUAL_HEX32_ARRAY(w_exp, w, 4);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(saved_exp, bb_meta.saved_exit, 6);

	bb_meta.branch_cache_status = BOTH_LINKED;
	write_p = w;
	w[0] = 1;
	w[1] = 2;
	w[2] = 3;
	w[3] = 4;
	w[4] = 5;
	w[5] = 6;
	*((uint32_t *)w_exp + 2) = 0x30200073;
	saved_exp[4] = 5;
	saved_exp[5] = 6;
	TEST_ASSERT_TRUE(unlink_direct_branch(&bb_meta, &write_p, 0, (uintptr_t)write_p));
	TEST_ASSERT_EQUAL_PTR(&w[6], write_p);
	TEST_ASSERT_EQUAL_HEX32_ARRAY(w_exp, w, 4);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(saved_exp, bb_meta.saved_exit, 6);
}

void test_unlink_fragment()
{
	dbm_thread *thread_data;
	dbm_code_cache_meta bb_meta;
	if (!allocate_thread_data(&thread_data)) {
		TEST_FAIL_MESSAGE("Failed to allocate initial thread data");
	}
	current_thread = thread_data;

	uint16_t w[6] = {0x0067, 0x0000};
	void *write_p = w;

	// Unlink indirect branch
	bb_meta.exit_branch_type = uncond_reg_riscv;
	bb_meta.exit_branch_addr = write_p;
	current_thread->code_cache_meta[0] = bb_meta;
	w[0] = 0x87aa;	// MV a5, a0
	w[1] = 0x3823;	// SD a1, -304(s0)
	w[2] = 0xecb4;
	w[3] = 0x0793;	// LI a5, 64
	w[4] = 0x0400;
	w[5] = 0x0067;	// JALR x0, 0(x0)
	w[6] = 0x0000;
	unlink_fragment(0, (uintptr_t)write_p);
	TEST_ASSERT_EQUAL_PTR(&w[0], write_p);
	uint16_t w_exp[8] = {0x87aa, 0x3823, 0xecb4, 0x0793, 0x0400};
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 5);
	TEST_ASSERT_EQUAL_HEX32(0x10200073, *(uint32_t *)&w[5]);

	// Unlink direct branch
	write_p = w;
	bb_meta.exit_branch_addr = write_p;
	bb_meta.exit_branch_type = cond_imm_riscv;
	bb_meta.branch_cache_status = BOTH_LINKED;
	memset(bb_meta.saved_exit, 0, MAX_SAVED_EXIT_SZ);
	current_thread->code_cache_meta[0] = bb_meta;
	*(uint32_t *)w_exp = 0x30200073;
	*((uint32_t *)w_exp + 1) = 0x30200073;
	*((uint32_t *)w_exp + 2) = 0x30200073;
	uint16_t saved_exp[6] = {0x87aa, 0x3823, 0xecb4, 0x0793, 0x0400, 0x0073};
	unlink_fragment(0, (uintptr_t)write_p);
	TEST_ASSERT_EQUAL_PTR(&w[0], write_p);
	TEST_ASSERT_EQUAL_HEX32_ARRAY(w_exp, w, 3);
	bb_meta = current_thread->code_cache_meta[0];
	TEST_ASSERT_EQUAL_HEX16_ARRAY(saved_exp, bb_meta.saved_exit, 6);
}

void test_translate_delayed_signal_frame()
{
	/*
	* Stack to rebuild:
	*              ┌───────┐
	*        sp -> │ x17   │
	*              │ TCP   │
	*              │ SPC   │
	*              │ x12   │
	*              │ x11   │
	*              │ x10   │
	*              │.......│
	*/
	ucontext_t context;
	mcontext_t uc_mcontext;
	long unsigned int stack[6] = {17, 0, 1, 12, 11, 10};

	uc_mcontext.__gregs[REG_SP] = (long unsigned int)stack;
	context.uc_mcontext = uc_mcontext;
	
	translate_delayed_signal_frame(&context);
	uc_mcontext = context.uc_mcontext;
	TEST_ASSERT_EQUAL(17, uc_mcontext.__gregs[17]);
	TEST_ASSERT_EQUAL(12, uc_mcontext.__gregs[12]);
	TEST_ASSERT_EQUAL(11, uc_mcontext.__gregs[11]);
	TEST_ASSERT_EQUAL(10, uc_mcontext.__gregs[10]);
	TEST_ASSERT_EQUAL(1, uc_mcontext.__gregs[REG_PC]);
	TEST_ASSERT_EQUAL(stack + 6, uc_mcontext.__gregs[REG_SP]);
}

void test_translate_svc_frame()
{
	/*
	* Stack to rebuild:
	*              ┌───────┐
	*        sp -> │ x1    │
	*              │ x2    │
	*              │ x3    │
	*              │ .     │
	*              │ .     │
	*              │ .     │
	*              │ x29   │
	*              │ x30   │
	*              │ x31   │
	*              │.......│
	*/
	ucontext_t context;
	mcontext_t uc_mcontext;
	long unsigned int stack[31] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 
		10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
		30, 31};

	uc_mcontext.__gregs[REG_SP] = (long unsigned int)stack;
	context.uc_mcontext = uc_mcontext;

	translate_svc_frame(&context);
	uc_mcontext = context.uc_mcontext;
	TEST_ASSERT_EQUAL(1, uc_mcontext.__gregs[1]);
	TEST_ASSERT_EQUAL_UINT64_ARRAY(stack + 2, uc_mcontext.__gregs + 3, 29);
	TEST_ASSERT_EQUAL(10, uc_mcontext.__gregs[REG_PC]);
	TEST_ASSERT_EQUAL_PTR(stack + 31, uc_mcontext.__gregs[REG_SP]);
}

void test_interpret_condition()
{
	ucontext_t context;
	mcontext_t uc_mcontext;
	mambo_cond cond;

	uint16_t w[2];
	uintptr_t write_p = (uintptr_t)w;
	
	// Test equal
	*(uint32_t *)w = 0x06b50263; // BEQ x10, x11, .+100
	uc_mcontext.__gregs[10] = 17;
	uc_mcontext.__gregs[11] = 17;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_TRUE(interpret_condition(&context, write_p));
	uc_mcontext.__gregs[11] = 255;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_FALSE(interpret_condition(&context, write_p));

	// Test not equal
	*(uint32_t *)w = 0x07411263; // BNE x2, x20, .+100
	uc_mcontext.__gregs[2] = 17;
	uc_mcontext.__gregs[20] = 17;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_FALSE(interpret_condition(&context, write_p));
	uc_mcontext.__gregs[2] = 255;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_TRUE(interpret_condition(&context, write_p));

	// Test lower then
	*(uint32_t *)w = 0x07ff4263; // BLT x30, x31, .+100
	uc_mcontext.__gregs[30] = 15;
	uc_mcontext.__gregs[31] = 30;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_TRUE(interpret_condition(&context, write_p));
	uc_mcontext.__gregs[31] = 10;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_FALSE(interpret_condition(&context, write_p));

	// Test greater equal
	*(uint32_t *)w = 0x06f0d263; // BGE x1, x15, .+100
	uc_mcontext.__gregs[1] = 2;
	uc_mcontext.__gregs[15] = 1;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_TRUE(interpret_condition(&context, write_p));
	uc_mcontext.__gregs[15] = 3;
	context.uc_mcontext = uc_mcontext;
	TEST_ASSERT_FALSE(interpret_condition(&context, write_p));
}

void test_restore_ihl_regs()
{
	ucontext_t context;
	mcontext_t uc_mcontext;

	long unsigned int stack[2];

	uc_mcontext.__gregs[REG_SP] = (long unsigned int)stack;
	context.uc_mcontext = uc_mcontext;

	stack[0] = 0x1234567812345678;
	stack[1] = 0x8765432187654321;

	restore_ihl_regs(&context);
	TEST_ASSERT_EQUAL_HEX64(0x1234567812345678, context.uc_mcontext.__gregs[10]);
	TEST_ASSERT_EQUAL_HEX64(0x8765432187654321, context.uc_mcontext.__gregs[11]);
	TEST_ASSERT_EQUAL_PTR(stack + 2, context.uc_mcontext.__gregs[REG_SP]);
}

void test_restore_exit()
{
	dbm_thread *thread_data;
	dbm_code_cache_meta bb_meta;
	if (!allocate_thread_data(&thread_data)) {
		TEST_FAIL_MESSAGE("Failed to allocate initial thread data");
	}

	uint16_t w[6];
	void *write_p = w;

	uint16_t w_exp[6] = {0, 1, 2, 3, 4, 5};

	bb_meta.exit_branch_type = cond_imm_riscv;
	bb_meta.branch_cache_status = BOTH_LINKED;
	memcpy(bb_meta.saved_exit, w_exp, 12);
	thread_data->code_cache_meta[0] = bb_meta;
	
	restore_exit(thread_data, 0, &write_p);
	TEST_ASSERT_EQUAL_PTR(&w[6], write_p);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 6);
}

void test_sigret_dispatcher_call()
{
	dbm_thread *thread_data;
	if (!allocate_thread_data(&thread_data)) {
		TEST_FAIL_MESSAGE("Failed to allocate initial thread data");
	}
	thread_data->dispatcher_addr = 0x1234;

	ucontext_t context;
	mcontext_t uc_mcontext;
	long unsigned int stack[2];

	uc_mcontext.__gregs[REG_SP] = (long unsigned int)&stack[2];
	uc_mcontext.__gregs[10] = 72;
	uc_mcontext.__gregs[11] = 85469;
	context.uc_mcontext = uc_mcontext;

	sigret_dispatcher_call(thread_data, &context, 128);
	uc_mcontext = context.uc_mcontext;
	TEST_ASSERT_EQUAL_PTR(stack, uc_mcontext.__gregs[REG_SP]);
	TEST_ASSERT_EQUAL(128, uc_mcontext.__gregs[10]);
	TEST_ASSERT_EQUAL(0, uc_mcontext.__gregs[11]);
	TEST_ASSERT_EQUAL(0x1234, uc_mcontext.__gregs[REG_PC]);
	long unsigned int stack_exp[2] = {72, 85469};
	TEST_ASSERT_EQUAL_UINT64_ARRAY(stack_exp, stack, 2);
}

void test_signal_dispatcher()
{
	TEST_IGNORE_MESSAGE("Test not implemented yet.");
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_inst_size);
	RUN_TEST(test_write_trap);
	RUN_TEST(test_direct_branch);
	RUN_TEST(test_restore_ihl_inst);
	RUN_TEST(test_install_system_sig_handlers);
	RUN_TEST(test_deliver_signals);
	RUN_TEST(test_riscv_sret);
	RUN_TEST(test_riscv_mret);
	RUN_TEST(test_unlink_indirect_branch);
	RUN_TEST(test_get_direct_branch_exit_trap_sz);
	RUN_TEST(test_unlink_direct_branch);
	RUN_TEST(test_unlink_fragment);
	RUN_TEST(test_translate_delayed_signal_frame);
	RUN_TEST(test_translate_svc_frame);
	RUN_TEST(test_interpret_condition);
	RUN_TEST(test_restore_ihl_regs);
	RUN_TEST(test_restore_exit);
	RUN_TEST(test_sigret_dispatcher_call);
	RUN_TEST(test_signal_dispatcher);
	return UNITY_END();
}