#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Module under test
#include "../arch/riscv/dispatcher_riscv.h"

#include "../dbm.h"
#include "../../scanner_common.h"

#include "unity/unity.h"

// DEBUG: Turn off debug
#define DEBUG
#ifdef DEBUG
	#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
	#define debug(...)
#endif

void setUp(void) {}
void tearDown(void) {}

void test_insert_cond_exit_branch()
{
	uint16_t w[3] = {0};
	uint16_t *write_p = w;
	uint16_t w_exp[3] = {0};
	uint16_t *write_p_exp = w_exp;
	dbm_code_cache_meta bb_meta = {0};
	mambo_cond cond = {x10, x11, LT};

	bb_meta.exit_branch_type = uncond_imm_riscv;
	insert_cond_exit_branch(&bb_meta, &write_p, &cond);
	TEST_ASSERT_EQUAL_PTR(w, write_p);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 3);

	write_p = w;
	write_p_exp = w_exp;
	bb_meta.exit_branch_type = uncond_reg_riscv;
	insert_cond_exit_branch(&bb_meta, &write_p, &cond);
	TEST_ASSERT_EQUAL_PTR(w, write_p);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 3);

	write_p = w;
	write_p_exp = w_exp;
	bb_meta.exit_branch_type = cond_imm_riscv;
	insert_cond_exit_branch(&bb_meta, &write_p, &cond);
	TEST_ASSERT_EQUAL_PTR(&w[2], write_p);
	riscv_b_cond_helper(&write_p_exp, (uint64_t)write_p_exp + 8, &cond);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 3);

	write_p = w;
	write_p_exp = w_exp;
	memset(write_p, 0, 6);
	memset(write_p_exp, 0, 6);
	cond.cond = NE;
	cond.r2 = x0;
	bb_meta.exit_branch_type = cond_imm_riscv;
	insert_cond_exit_branch(&bb_meta, &write_p, &cond);
	TEST_ASSERT_EQUAL_PTR(&w[1], write_p); // Expext 16 bit instruction
	riscv_b_cond_helper(&write_p_exp, (uint64_t)write_p_exp + 8, &cond);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 3);
}

void test_dispatcher_riscv()
{
	TEST_IGNORE_MESSAGE("Test not implemented yet.");
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_insert_cond_exit_branch);
	RUN_TEST(test_dispatcher_riscv);
	return UNITY_END();
}