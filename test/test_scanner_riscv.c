#include <stdio.h>
#include <stdlib.h>

// Module under test
#include "../arch/riscv/scanner_riscv.c"

#include "../dbm.h"
#include "../scanner_common.h"

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

void test_riscv_copy16()
{
	uint16_t w[5] = {0};
	uint16_t *write_p = w;

	uint16_t r[5] = {0x1234, 0x0000, 0xB00B, 0xFFFF, 0x89AB};
	uint16_t *read_address = r;

	riscv_copy16(&write_p, read_address);

	TEST_ASSERT_EQUAL_PTR(&w[1], write_p);
	TEST_ASSERT_EQUAL_PTR(&r[0], read_address);
	TEST_ASSERT_EQUAL_HEX16(0x1234, w[0]);

	read_address++;
	riscv_copy16(&write_p, read_address);
	read_address++;
	riscv_copy16(&write_p, read_address);
	read_address++;
	riscv_copy16(&write_p, read_address);
	read_address++;

	TEST_ASSERT_EQUAL_PTR(&w[4], write_p);
	TEST_ASSERT_EQUAL_PTR(&r[4], read_address);
	TEST_ASSERT_EQUAL_HEX16(0, w[4]);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(r, w, 4);
}

void test_riscv_copy32()
{
	uint16_t w[5] = {0};
	uint16_t *write_p = w;

	uint16_t r[5] = {0x1234, 0x0000, 0xB00B, 0xFFFF, 0x89AB};
	uint16_t *read_address = r;

	riscv_copy32(&write_p, read_address);

	TEST_ASSERT_EQUAL_PTR(&w[2], write_p);
	TEST_ASSERT_EQUAL_PTR(&r[0], read_address);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(r, w, 2);

	read_address += 2;
	riscv_copy32(&write_p, read_address);
	read_address += 2;

	TEST_ASSERT_EQUAL_PTR(&w[4], write_p);
	TEST_ASSERT_EQUAL_PTR(&r[4], read_address);
	TEST_ASSERT_EQUAL_HEX16(0, w[4]);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(r, w, 4);
}

void test_riscv_copy64()
{
	uint16_t w[9] = {0};
	uint16_t *write_p = w;

	uint16_t r[9] = {0x1234, 0x0000, 0xB00B, 0xFFFF, 
					 0x89AB, 0x1111, 0x100F, 0x0069, 0xFF00};
	uint16_t *read_address = r;

	riscv_copy64(&write_p, read_address);

	TEST_ASSERT_EQUAL_PTR(&w[4], write_p);
	TEST_ASSERT_EQUAL_PTR(&r[0], read_address);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(r, w, 4);

	read_address += 4;
	riscv_copy64(&write_p, read_address);
	read_address += 4;

	TEST_ASSERT_EQUAL_PTR(&w[8], write_p);
	TEST_ASSERT_EQUAL_PTR(&r[8], read_address);
	TEST_ASSERT_EQUAL_HEX16(0, w[8]);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(r, w, 8);
}

void test_riscv_check_cb_type()
{
	int offset;
	enum reg r;

	// Test register condition (true if x8, x9, ... x14, x15)
	offset = 2;
	r = x7;
	TEST_ASSERT_FALSE(riscv_check_cb_type(offset, r));

	r = x8;
	TEST_ASSERT_TRUE(riscv_check_cb_type(offset, r));

	r = x15;
	TEST_ASSERT_TRUE(riscv_check_cb_type(offset, r));

	r = x16;
	TEST_ASSERT_FALSE(riscv_check_cb_type(offset, r));

	r = x0;
	TEST_ASSERT_FALSE(riscv_check_cb_type(offset, r));

	// Test offset value condition (true if offset < 256 and offset >= -256)
	r = x8;
	offset = -257;
	TEST_ASSERT_FALSE(riscv_check_cb_type(offset, r));

	offset = -256;
	TEST_ASSERT_TRUE(riscv_check_cb_type(offset, r));

	offset = 255;
	TEST_ASSERT_TRUE(riscv_check_cb_type(offset, r));

	offset = 256;
	TEST_ASSERT_FALSE(riscv_check_cb_type(offset, r));
}

void test_riscv_check_cj_type()
{
	int offset;

	// Test offset calue condition (true if offset < 2048 and offset >= -2048)
	offset = -2049;
	TEST_ASSERT_FALSE(riscv_check_cj_type(offset));

	offset = -2048;
	TEST_ASSERT_TRUE(riscv_check_cj_type(offset));

	offset = 2047;
	TEST_ASSERT_TRUE(riscv_check_cj_type(offset));

	offset = 2048;
	TEST_ASSERT_FALSE(riscv_check_cj_type(offset));
}

void test_riscv_check_uj_type()
{
	int offset;

	// Test offset calue condition (true if offset < 0x100000 and offset >= -0x100000)
	offset = -0x100001;
	TEST_ASSERT_FALSE(riscv_check_uj_type(offset));

	offset = -0x100000;
	TEST_ASSERT_TRUE(riscv_check_uj_type(offset));

	offset = 0x0FFFFF;
	TEST_ASSERT_TRUE(riscv_check_uj_type(offset));

	offset = 0x100000;
	TEST_ASSERT_FALSE(riscv_check_uj_type(offset));
}

void test_riscv_check_sb_type()
{
	int offset;

	// Test offset calue condition (true if offset < 0x1000 and offset >= -0x1000)
	offset = -0x1001;
	TEST_ASSERT_FALSE(riscv_check_sb_type(offset));

	offset = -0x1000;
	TEST_ASSERT_TRUE(riscv_check_sb_type(offset));

	offset = 0x0FFF;
	TEST_ASSERT_TRUE(riscv_check_sb_type(offset));

	offset = 0x1000;
	TEST_ASSERT_FALSE(riscv_check_sb_type(offset));
}

void test_riscv_calc_j_imm()
{
	unsigned int rawimm, value;

	rawimm = 0;
	riscv_calc_j_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0, value);

	rawimm = 0xFFFFF;
	riscv_calc_j_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0x1FFFFE, value);

	rawimm = -1;
	riscv_calc_j_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0x1FFFFE, value);

	rawimm = 0x80100;
	riscv_calc_j_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0x100800, value);

	// Test with some random values
	rawimm = 0x48CA3;
	riscv_calc_j_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xA348C, value);

	rawimm = 0x450CF;
	riscv_calc_j_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xCF450, value);

	rawimm = 0x84605;
	riscv_calc_j_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0x105046, value);
}

void test_riscv_calc_cj_imm()
{
	unsigned int rawimm, value;

	rawimm = 0;
	riscv_calc_cj_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0, value);

	rawimm = 0x7FF;
	riscv_calc_cj_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xFFE, value);

	rawimm = -1;
	riscv_calc_cj_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xFFE, value);

	rawimm = 0x5AE;
	riscv_calc_cj_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xB4E, value);

	// Test with some random values
	rawimm = 0x3F0;
	riscv_calc_cj_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0x7D0, value);

	rawimm = 0x230;
	riscv_calc_cj_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xD0, value);

	rawimm = 0x29A;
	riscv_calc_cj_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0x19A, value);
}

void test_riscv_calc_b_imm()
{
	unsigned int rawimmhi, rawimmlo, value;

	rawimmhi = rawimmlo = 0;
	riscv_calc_b_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0, value);

	rawimmhi = 0x7F;
	rawimmlo = 0x1F;
	riscv_calc_b_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x1FFE, value);

	rawimmhi = -1;
	rawimmlo = -1;
	riscv_calc_b_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x1FFE, value);

	rawimmhi = 0x40;
	rawimmlo = 0x1E;
	riscv_calc_b_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x101E, value);

	// Test with some random values
	rawimmhi = 0x5;
	rawimmlo = 0x19;
	riscv_calc_b_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x8B8, value);

	rawimmhi = 0x44;
	rawimmlo = 0x19;
	riscv_calc_b_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x1898, value);

	rawimmhi = 0xF;
	rawimmlo = 0x1E;
	riscv_calc_b_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x1FE, value);
}

void test_riscv_calc_cb_imm()
{
	unsigned int rawimmhi, rawimmlo, value;

	rawimmhi = rawimmlo = 0;
	riscv_calc_cb_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0, value);

	rawimmhi = 0x7;
	rawimmlo = 0x1F;
	riscv_calc_cb_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x1FE, value);

	rawimmhi = -1;
	rawimmlo = -1;
	riscv_calc_cb_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x1FE, value);

	rawimmhi = 0x4;
	rawimmlo = 0x19;
	riscv_calc_cb_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x1E0, value);

	// Test with some random values
	rawimmhi = 0x3;
	rawimmlo = 0x19;
	riscv_calc_cb_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0xF8, value);

	rawimmhi = 0x7;
	rawimmlo = 0xD;
	riscv_calc_cb_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0x17C, value);

	rawimmhi = 0x0;
	rawimmlo = 0x17;
	riscv_calc_cb_imm(rawimmhi, rawimmlo, &value);
	TEST_ASSERT_EQUAL_HEX(0xA6, value);
}

void test_riscv_calc_u_imm()
{
	unsigned int rawimm, value;

	rawimm = 0;
	riscv_calc_u_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0, value);

	rawimm = 0xFFFFF;
	riscv_calc_u_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xFFFFF000, value);

	rawimm = -1;
	riscv_calc_u_imm(rawimm, &value);
	TEST_ASSERT_EQUAL_HEX(0xFFFFF000, value);
}

void test_riscv_push_helper()
{
	uint16_t w[7] = {0};
	uint16_t *write_p = w;

	riscv_push_helper(&write_p, x9);
	riscv_push_helper(&write_p, x31);

	TEST_ASSERT_EQUAL_HEX16(0x1161, w[0]); // C.ADDI sp, -8
	TEST_ASSERT_EQUAL_HEX32(0x00913023, *(uint32_t*)&w[1]); // SD x9, 0(sp)

	TEST_ASSERT_EQUAL_HEX16(0x1161, w[3]); // C.ADDI sp, -8
	TEST_ASSERT_EQUAL_HEX32(0x01F13023, *(uint32_t*)&w[4]); // SD x31, 0(sp)
	
	TEST_ASSERT_EQUAL_PTR(0, w[6]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[6], write_p);
}

void test_riscv_pop_helper()
{
	uint16_t w[7] = {0};
	uint16_t *write_p = w;

	riscv_pop_helper(&write_p, x9);
	riscv_pop_helper(&write_p, x31);

	TEST_ASSERT_EQUAL_HEX32(0x00013483, *(uint32_t*)&w[0]); // LD x9, 0(sp)
	TEST_ASSERT_EQUAL_HEX16(0x0121, w[2]); // C.ADDI sp, 8
	
	TEST_ASSERT_EQUAL_HEX32(0x00013F83, *(uint32_t*)&w[3]); // LD x31, 0(sp)
	TEST_ASSERT_EQUAL_HEX16(0x0121, w[5]); // C.ADDI sp, 8
	
	TEST_ASSERT_EQUAL(0, w[6]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[6], write_p);
}

void test_riscv_branch_imm_helper()
{
	uint16_t w[7] = {0};
	uint16_t *write_p = w;

	int ret;

	ret = riscv_branch_imm_helper(&write_p, (uint64_t)write_p + 2046, false);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_branch_imm_helper(&write_p, (uint64_t)write_p - 2048, true);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_branch_imm_helper(&write_p, (uint64_t)write_p + 1048575, false);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	ret = riscv_branch_imm_helper(&write_p, (uint64_t)write_p + 2048, true);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_branch_imm_helper(&write_p, (uint64_t)write_p + 1048576, false);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	ret = riscv_branch_imm_helper(&write_p, (uint64_t)write_p - 1048576, false);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL_HEX16(0xAFFD, w[0]); // C.J 2046
	TEST_ASSERT_EQUAL_HEX16(0x3001, w[1]); // C.JAL -2048
	TEST_ASSERT_EQUAL_HEX32(0x001000EF, *(uint32_t*)&w[2]); // JAL ra, 2048
	TEST_ASSERT_EQUAL_HEX32(0x8000006F, *(uint32_t*)&w[4]); // JAL x0, -1048576
	TEST_ASSERT_EQUAL(0, w[6]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[6], write_p);
}

void test_riscv_copy_to_reg_32bits()
{
	uint16_t w[23] = {0};
	uint16_t *write_p = w;

	riscv_copy_to_reg_32bits(&write_p, x12, 0);
	riscv_copy_to_reg_32bits(&write_p, x12, 17);
	riscv_copy_to_reg_32bits(&write_p, x0, 17);
	riscv_copy_to_reg_32bits(&write_p, x12, -17);
	riscv_copy_to_reg_32bits(&write_p, x12, 0x1000);
	riscv_copy_to_reg_32bits(&write_p, x12, 0xFFFFFFFF);
	riscv_copy_to_reg_32bits(&write_p, x12, 0x12345678);
	riscv_copy_to_reg_32bits(&write_p, x12, 0x12345000);
	riscv_copy_to_reg_32bits(&write_p, x12, -0x12345678);

	TEST_ASSERT_EQUAL_HEX32(0x00000613, *(uint32_t*)&w[0]); // ADDI x12, x0, 0
	TEST_ASSERT_EQUAL_HEX32(0x01100613, *(uint32_t*)&w[2]); // ADDI x12, x0, 17
	TEST_ASSERT_EQUAL_HEX32(0x01100013, *(uint32_t*)&w[4]); // ADDI x0, x0, 17
	TEST_ASSERT_EQUAL_HEX32(0xFEF00613, *(uint32_t*)&w[6]); // ADDI x12, x0, -17
	TEST_ASSERT_EQUAL_HEX32(0x00001637, *(uint32_t*)&w[8]); // LUI x12, 1
	TEST_ASSERT_EQUAL_HEX32(0xFFF00613, *(uint32_t*)&w[10]); // ADDI x12, x0, -1
	TEST_ASSERT_EQUAL_HEX32(0x12345637, *(uint32_t*)&w[12]); // LUI x12, 0x12345
	TEST_ASSERT_EQUAL_HEX32(0x67860613, *(uint32_t*)&w[14]); // ADDI x12, x12, 0x678
	TEST_ASSERT_EQUAL_HEX32(0x12345637, *(uint32_t*)&w[16]); // LUI x12, 0x12345
	TEST_ASSERT_EQUAL_HEX32(0xEDCBB637, *(uint32_t*)&w[18]); // LUI x12, 0xEDCBB
	TEST_ASSERT_EQUAL_HEX32(0x98860613, *(uint32_t*)&w[20]); // ADDI x12, x12, -1656
	TEST_ASSERT_EQUAL(0, w[22]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[22], write_p);
}

void test_riscv_copy_to_reg_64bits()
{
	uint16_t w[28] = {0};
	uint16_t *write_p = w;

	riscv_copy_to_reg_64bits(&write_p, x5, 0);
	riscv_copy_to_reg_64bits(&write_p, x5, 0x1234567823456789);
	riscv_copy_to_reg_64bits(&write_p, x5, -0x1234567812345678);

	TEST_ASSERT_EQUAL_HEX32(0xA029, w[0]); // C.J .+10
	TEST_ASSERT_EQUAL_HEX64(0, *(uint64_t*)&w[1]); // .qword 0
	TEST_ASSERT_EQUAL_HEX32(0x00000297, *(uint32_t*)&w[5]); // AUIPC x5, 0
	TEST_ASSERT_EQUAL_HEX32(0xFF82B283, *(uint32_t*)&w[7]); // LD x8, -8(x5)
	TEST_ASSERT_EQUAL_HEX16(0xA029, w[9]); // C.J .+10
	TEST_ASSERT_EQUAL_HEX64(0x1234567823456789, *(uint64_t*)&w[10]); // .qword 0x1234567823456789
	TEST_ASSERT_EQUAL_HEX32(0x00000297, *(uint32_t*)&w[14]); // AUIPC x5, 0
	TEST_ASSERT_EQUAL_HEX32(0xFF82B283, *(uint32_t*)&w[16]); // LD x8, -8(x5)
	TEST_ASSERT_EQUAL_HEX16(0xA029, w[18]); // C.J .+10
	TEST_ASSERT_EQUAL_HEX64(0xEDCBA987EDCBA988, *(uint64_t*)&w[19]); // .qword -0x123457823456789
	TEST_ASSERT_EQUAL_HEX32(0x00000297, *(uint32_t*)&w[23]); // AUIPC x5, 0
	TEST_ASSERT_EQUAL_HEX32(0xFF82B283, *(uint32_t*)&w[25]); // LD x8, -8(x5)
	TEST_ASSERT_EQUAL(0, w[27]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[27], write_p);
}

void test_riscv_b_cond_helper()
{
	uint16_t w[31] = {0};
	uint16_t *write_p = w;

	mambo_cond cond = {x8, x0, EQ};

	int ret;

	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p - 258, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p - 256, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 254, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 256, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	cond.cond = NE;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p - 258, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p - 256, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 254, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 256, &cond);
	TEST_ASSERT_EQUAL(0, ret);

	cond.r1 = x1;
	cond.r2 = x2;
	cond.cond = EQ;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 42, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	cond.cond = NE;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 42, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	cond.cond = LT;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 42, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	cond.cond = GE;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 42, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	cond.cond = LTU;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 42, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	cond.cond = GEU;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 42, &cond);
	TEST_ASSERT_EQUAL(0, ret);

	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 4094, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 4096, &cond);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p - 4096, &cond);
	TEST_ASSERT_EQUAL(0, ret);
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p - 4098, &cond);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	cond.cond = GE;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 17, &cond);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	cond.cond = AL;
	ret = riscv_b_cond_helper(&write_p, (uint64_t)write_p + 4098, &cond);

	TEST_ASSERT_EQUAL_HEX32(0xEE040FE3, *(uint32_t*)&w[0]); // BEQ x8, x0, -258
	TEST_ASSERT_EQUAL_HEX16(0xD001, w[2]); // C.BEQZ x8, -256
	TEST_ASSERT_EQUAL_HEX16(0xCC7D, w[3]); // C.BEQZ x8, 254
	TEST_ASSERT_EQUAL_HEX32(0x10040063, *(uint32_t*)&w[4]); // BEQ x8, x0, 256
	TEST_ASSERT_EQUAL_HEX32(0xEE041FE3, *(uint32_t*)&w[6]); // BNE x8, x0, -258
	TEST_ASSERT_EQUAL_HEX16(0xF001, w[8]); // C.BNEZ x8, -256
	TEST_ASSERT_EQUAL_HEX16(0xEC7D, w[9]); // C.BNEZ x8, 254
	TEST_ASSERT_EQUAL_HEX32(0x10041063, *(uint32_t*)&w[10]); // BNE x8, x0, 256

	TEST_ASSERT_EQUAL_HEX32(0x2208563, *(uint32_t*)&w[12]); // BEQ x1, x2, 42
	TEST_ASSERT_EQUAL_HEX32(0x2209563, *(uint32_t*)&w[14]); // BNE x1, x2, 42
	TEST_ASSERT_EQUAL_HEX32(0x220C563, *(uint32_t*)&w[16]); // BLT x1, x2, 42
	TEST_ASSERT_EQUAL_HEX32(0x220D563, *(uint32_t*)&w[18]); // BGE x1, x2, 42
	TEST_ASSERT_EQUAL_HEX32(0x220E563, *(uint32_t*)&w[20]); // BLTU x1, x2, 42
	TEST_ASSERT_EQUAL_HEX32(0x220F563, *(uint32_t*)&w[22]); // BGEU x1, x2, 42

	TEST_ASSERT_EQUAL_HEX32(0x7E20FFE3, *(uint32_t*)&w[24]); // BGEU x1, x2, 4094
	TEST_ASSERT_EQUAL_HEX32(0x8020F063, *(uint32_t*)&w[26]); // BGEU x1, x2, -4096
	
	uint16_t w_exp[2] = {0};
	uint16_t *write_p_exp = w_exp;
	riscv_branch_imm_helper(&write_p_exp, (uint64_t)w_exp + 4098, false);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[28], 2);
	
	TEST_ASSERT_EQUAL(0, w[30]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[30], write_p);
}

void test_riscv_save_regs()
{
	uint16_t w[115] = {0};
	uint16_t *write_p = w;

	/* HACK: Mocking internal functions is a nightmare, therefore riscv_save_regs's 
	 * result is compared with the result of the internal function called with the
	 * expected parameters. Most likely, this test needs to be rewritten when the
	 * implementation of riscv_save_regs changes.
	 */
	riscv_save_regs(&write_p, 1); // x0
	riscv_save_regs(&write_p, 0x80000000); // x31
	riscv_save_regs(&write_p, 0x8002022); // x27, x13, x5, x1
	riscv_save_regs(&write_p, -1u); // x0-x31

	uint16_t w_exp[96] = {0};
	uint16_t *write_p_exp = w_exp;

	riscv_push_helper(&write_p_exp, x0);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 3);

	write_p_exp = w_exp;
	riscv_push_helper(&write_p_exp, x31);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[3], 3);

	write_p_exp = w_exp;
	riscv_push_helper(&write_p_exp, x1);
	riscv_push_helper(&write_p_exp, x5);
	riscv_push_helper(&write_p_exp, x13);
	riscv_push_helper(&write_p_exp, x27);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[6], 12);

	write_p_exp = w_exp;
	riscv_push_helper(&write_p_exp, x0);
	riscv_push_helper(&write_p_exp, x1);
	riscv_push_helper(&write_p_exp, x2);
	riscv_push_helper(&write_p_exp, x3);
	riscv_push_helper(&write_p_exp, x4);
	riscv_push_helper(&write_p_exp, x5);
	riscv_push_helper(&write_p_exp, x6);
	riscv_push_helper(&write_p_exp, x7);
	riscv_push_helper(&write_p_exp, x8);
	riscv_push_helper(&write_p_exp, x9);
	riscv_push_helper(&write_p_exp, x10);
	riscv_push_helper(&write_p_exp, x11);
	riscv_push_helper(&write_p_exp, x12);
	riscv_push_helper(&write_p_exp, x13);
	riscv_push_helper(&write_p_exp, x14);
	riscv_push_helper(&write_p_exp, x15);
	riscv_push_helper(&write_p_exp, x16);
	riscv_push_helper(&write_p_exp, x17);
	riscv_push_helper(&write_p_exp, x18);
	riscv_push_helper(&write_p_exp, x19);
	riscv_push_helper(&write_p_exp, x20);
	riscv_push_helper(&write_p_exp, x21);
	riscv_push_helper(&write_p_exp, x22);
	riscv_push_helper(&write_p_exp, x23);
	riscv_push_helper(&write_p_exp, x24);
	riscv_push_helper(&write_p_exp, x25);
	riscv_push_helper(&write_p_exp, x26);
	riscv_push_helper(&write_p_exp, x27);
	riscv_push_helper(&write_p_exp, x28);
	riscv_push_helper(&write_p_exp, x29);
	riscv_push_helper(&write_p_exp, x30);
	riscv_push_helper(&write_p_exp, x31);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[18], 96);

	TEST_ASSERT_EQUAL(0, w[114]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[114], write_p);
}

void test_riscv_restore_regs()
{
	uint16_t w[115] = {0};
	uint16_t *write_p = w;

	// HACK: Same procedure as in test_riscv_save_regs
	riscv_restore_regs(&write_p, 1); // x0
	riscv_restore_regs(&write_p, 0x80000000); // x31
	riscv_restore_regs(&write_p, 0x8002022); // x27, x13, x5, x1
	riscv_restore_regs(&write_p, -1u); // x0-x31

	uint16_t w_exp[96] = {0};
	uint16_t *write_p_exp = w_exp;

	riscv_pop_helper(&write_p_exp, x0);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 3);

	write_p_exp = w_exp;
	riscv_pop_helper(&write_p_exp, x31);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[3], 3);

	write_p_exp = w_exp;
	riscv_pop_helper(&write_p_exp, x27);
	riscv_pop_helper(&write_p_exp, x13);
	riscv_pop_helper(&write_p_exp, x5);
	riscv_pop_helper(&write_p_exp, x1);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[6], 12);

	write_p_exp = w_exp;
	riscv_pop_helper(&write_p_exp, x31);
	riscv_pop_helper(&write_p_exp, x30);
	riscv_pop_helper(&write_p_exp, x29);
	riscv_pop_helper(&write_p_exp, x28);
	riscv_pop_helper(&write_p_exp, x27);
	riscv_pop_helper(&write_p_exp, x26);
	riscv_pop_helper(&write_p_exp, x25);
	riscv_pop_helper(&write_p_exp, x24);
	riscv_pop_helper(&write_p_exp, x23);
	riscv_pop_helper(&write_p_exp, x22);
	riscv_pop_helper(&write_p_exp, x21);
	riscv_pop_helper(&write_p_exp, x20);
	riscv_pop_helper(&write_p_exp, x19);
	riscv_pop_helper(&write_p_exp, x18);
	riscv_pop_helper(&write_p_exp, x17);
	riscv_pop_helper(&write_p_exp, x16);
	riscv_pop_helper(&write_p_exp, x15);
	riscv_pop_helper(&write_p_exp, x14);
	riscv_pop_helper(&write_p_exp, x13);
	riscv_pop_helper(&write_p_exp, x12);
	riscv_pop_helper(&write_p_exp, x11);
	riscv_pop_helper(&write_p_exp, x10);
	riscv_pop_helper(&write_p_exp, x9);
	riscv_pop_helper(&write_p_exp, x8);
	riscv_pop_helper(&write_p_exp, x7);
	riscv_pop_helper(&write_p_exp, x6);
	riscv_pop_helper(&write_p_exp, x5);
	riscv_pop_helper(&write_p_exp, x4);
	riscv_pop_helper(&write_p_exp, x3);
	riscv_pop_helper(&write_p_exp, x2);
	riscv_pop_helper(&write_p_exp, x1);
	riscv_pop_helper(&write_p_exp, x0);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[18], 96);

	TEST_ASSERT_EQUAL(0, w[114]);

	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[114], write_p);
}

void test_riscv_branch_jump()
{
	// HACK: Same handling of internal function calls as in test_riscv_save_regs
	uint16_t w[37] = {0};
	uint16_t *write_p = w;

	dbm_thread *thread_data = malloc(sizeof(dbm_thread));
	thread_data->dispatcher_addr = (uint64_t)write_p + 5000;

	riscv_branch_jump(thread_data, &write_p, 42, (uint64_t)write_p + 125, 
		REPLACE_TARGET | INSERT_BRANCH);
	riscv_branch_jump(thread_data, &write_p, 42, (uint64_t)write_p + 124, 
		INSERT_BRANCH);
	riscv_branch_jump(thread_data, &write_p, 42, (uint64_t)write_p + 124, 
		REPLACE_TARGET);
	riscv_branch_jump(thread_data, &write_p, 42, (uint64_t)write_p + 124, 0);

	TEST_ASSERT_EQUAL(0, w[26]);
	// Check pointer
	TEST_ASSERT_EQUAL_PTR(&w[26], write_p);

	uint16_t w_exp[13] = {0};
	uint16_t *write_p_exp = w_exp;

	riscv_copy_to_reg_64bits(&write_p_exp, x10, (uint64_t)&w[0] + 125);
	riscv_copy_to_reg_32bits(&write_p_exp, x11, 42);
	riscv_branch_imm_helper(&write_p_exp, (uint64_t)w_exp + 5000, false);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 13);

	write_p_exp = w_exp;
	riscv_copy_to_reg_32bits(&write_p_exp, x11, 42);
	riscv_branch_imm_helper(&write_p_exp, (uint64_t)w_exp + 5000 - 26, false);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[13], 4);

	write_p_exp = w_exp;
	riscv_copy_to_reg_64bits(&write_p_exp, x10, (uint64_t)&w[17] + 124);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, &w[17], 9);

	free(thread_data);
}

void test_riscv_get_inst_length()
{
	TEST_ASSERT_EQUAL(INST_16BIT, riscv_get_inst_length(RISCV_C_ILLEGAL));
	TEST_ASSERT_EQUAL(INST_16BIT, riscv_get_inst_length(RISCV_C_ADDI4SPN));
	TEST_ASSERT_EQUAL(INST_16BIT, riscv_get_inst_length(RISCV_C_ADDI));
	TEST_ASSERT_EQUAL(INST_16BIT, riscv_get_inst_length(RISCV_C_SDSP));

	TEST_ASSERT_EQUAL(INST_32BIT, riscv_get_inst_length(RISCV_LUI));
	TEST_ASSERT_EQUAL(INST_32BIT, riscv_get_inst_length(RISCV_SD));
	TEST_ASSERT_EQUAL(INST_32BIT, riscv_get_inst_length(RISCV_FMV_D_X));
	TEST_ASSERT_EQUAL(INST_32BIT, riscv_get_inst_length(RISCV_INVALID));
}

void test_riscv_branch_jump_cond()
{
	TEST_IGNORE_MESSAGE("Test not implemented yet.");
}

void test_riscv_check_free_space()
{
	TEST_IGNORE_MESSAGE("Test not implemented yet.");
}

void test_pass1_riscv()
{
	TEST_IGNORE_MESSAGE("Test not implemented yet.");
}

void test_riscv_get_mambo_cond()
{
	uint16_t r[5] = {
		0xDDDD, 			// BEQZ 	x11, .-0x42
		0x4963, 0x4111, 	// BLT 		x2, x17, .+1042
		0x8113, 0x011f		// ADDI		x2, x31, 17
	};
	uint16_t *read_address = r;

	mambo_cond cond = {0};
	uint64_t target = 0;
	int ret;

	ret = riscv_get_mambo_cond(RISCV_C_BEQZ, read_address, &cond, NULL);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0, target);
	TEST_ASSERT_EQUAL(c_x11, cond.r1);
	TEST_ASSERT_EQUAL(x0, cond.r2);
	TEST_ASSERT_EQUAL(EQ, cond.cond);

	ret = riscv_get_mambo_cond(RISCV_C_BEQZ, read_address, &cond, &target);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(-0x42, target - (uint64_t)read_address);
	TEST_ASSERT_EQUAL(c_x11, cond.r1);
	TEST_ASSERT_EQUAL(x0, cond.r2);
	TEST_ASSERT_EQUAL(EQ, cond.cond);
	read_address++;
	
	ret = riscv_get_mambo_cond(RISCV_BLT, read_address, &cond, &target);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1042, target - (uint64_t)read_address);
	TEST_ASSERT_EQUAL(x2, cond.r1);
	TEST_ASSERT_EQUAL(x17, cond.r2);
	TEST_ASSERT_EQUAL(LT, cond.cond);
	read_address += 2;
	
	ret = riscv_get_mambo_cond(RISCV_ADDI, read_address, &cond, &target);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	// Unchanged
	TEST_ASSERT_EQUAL(1042, target - (uint64_t)(read_address - 2));
	TEST_ASSERT_EQUAL(x2, cond.r1);
	TEST_ASSERT_EQUAL(x17, cond.r2);
	TEST_ASSERT_EQUAL(LT, cond.cond);
}

void test_riscv_scanner_deliver_callbacks()
{
	TEST_IGNORE_MESSAGE("Test not implemented yet.");
}

void test_riscv_inline_hash_lookup()
{
	// HACK: Tests without stubs.
	/*
	 * Test Indirect Branch Lookup
	 * Tested instance:
	 * - rn = ra
	 * - Replaced instruction: RET/C.JR
	 * - &read_address = 0x5550
	 * - HASH_MASK = 0xFF
	 * - dispatcher = 0x6770
	 * - basic_block = 17
	 * 
	 * 		16 bit
	 * 		offset		+-------------------------------+
	 * 		0			|	(PUSH	x10, x11)			|
	 * 					|	 -> riscv_save_regs			|
	 * 		6			|	(PUSH	x12)				|
	 * 					|	 -> riscv_push_helper		|
	 * 		9			|	ADDI	x11, ra, 0			|
	 * 		11			|	(LI		ra, 0x5552)			|
	 * 					|	 -> riscv_copy_to_reg_64bits|
	 * 		20			|	(LI		x10, &entries)		|
	 * 					|	 -> riscv_copy_to_reg_64bits|
	 * 		29			|	(LI		x12, 0x1FE)			|
	 * 					|	 -> riscv_copy_to_reg_32bits|
	 * 		31			|	AND		x12, x12, x11		|
	 * 		33			|	C.SLLI	x12, 3				|
	 * 		34			|	C.ADD	x10, x12			|
	 * 					|								|
	 * 					| lin_probing:					|
	 * 		35			|	C.LD	x12, 0(x10)			|
	 * 		36			|	C.ADDI	x10, 16				|
	 * 		37			|	(C.BEQZ	x12, not_found)		|
	 * 					|	 -> riscv_bez_helper		|
	 * 		38			|	(BNE	x12, x11, lin_probing)
	 * 					|	 -> riscv_b_cond_helper		|
	 * 		40			|	LD		x10, -8(x10)		|
	 * 		42			|	(POP	x12)				|
	 * 					|	 -> riscv_pop_helper		|
	 * 		45			|	C.JR	x10					|
	 * 					|								|
	 * 					| not_found:					|
	 * 		46			|	C.MV	x10, x11			|
	 * 		47			|	(LI		x11, basic_block)	|
	 * 					|	 -> riscv_copy_to_reg_32bits|
	 * 		49			|	(POP	x12)				|
	 * 					|	 -> riscv_pop_helper		|
	 * 		52			|	(JAL	x0, 0x6770)			|
	 * 					|	 -> riscv_branch_imm_helper	|
	 * 					+-------------------------------+
	 */

	uint16_t w[55] = {0};
	uint16_t *write_p = w;
	
	uint16_t *read_address = (uint16_t *)0x5552;

	dbm_thread *thread_data = malloc(sizeof(dbm_thread));
	thread_data->dispatcher_addr = 0x6770;

	riscv_inline_hash_lookup(thread_data, 17, &write_p, read_address, ra, 0, true, true,
		INST_16BIT);

	TEST_ASSERT_EQUAL_HEX16(0, w[54]);
	TEST_ASSERT_EQUAL_PTR(&w[54], write_p);
	TEST_ASSERT_EQUAL(thread_data->code_cache_meta[17].rn, x11);

	uint16_t w_exp[54] = {0};
	uint16_t *write_p_exp = w_exp;
	uint16_t *lin_probing;
	uint16_t *branch_to_not_found;

	riscv_save_regs(&write_p_exp, (m_x10 | m_x11));
	riscv_push_helper(&write_p_exp, x12);
	riscv_addi(&write_p_exp, x11, ra, 0);
	write_p_exp += 2;

	riscv_copy_to_reg_64bits(&write_p_exp, ra, (uint64_t)read_address + 2);
	riscv_copy_to_reg_64bits(&write_p_exp, x10, 
		(uint64_t)&thread_data->entry_address.entries);
	riscv_copy_to_reg_32bits(&write_p_exp, x12, CODE_CACHE_HASH_SIZE << 1);

	riscv_and(&write_p_exp, x12, x12, x11);
	write_p_exp += 2;
	riscv_c_slli(&write_p_exp, x12, 0, 3);
	write_p_exp++;
	riscv_c_add(&write_p_exp, x10, x12);
	write_p_exp++;
	
	lin_probing = write_p_exp;
	riscv_c_ld(&write_p_exp, c_x12, c_x10, 0, 0);
	write_p_exp++;
	riscv_c_addi(&write_p_exp, x10, 0, 16);
	write_p_exp++;
	branch_to_not_found = write_p_exp++;
	{
		mambo_cond cond = {x12, x11, NE};
		riscv_b_cond_helper(&write_p_exp, (uint64_t)lin_probing, &cond);
	}
	riscv_ld(&write_p_exp, x10, x10, -8);
	write_p_exp += 2;
	riscv_pop_helper(&write_p_exp, x12);
	riscv_c_jr(&write_p_exp, x10);
	write_p_exp++;

	riscv_bez_helper(&branch_to_not_found, x12, (uint64_t)write_p_exp);
	riscv_c_mv(&write_p_exp, x10, x11);
	write_p_exp++;
	riscv_copy_to_reg_32bits(&write_p_exp, x11, 17);
	riscv_pop_helper(&write_p_exp, x12);
	riscv_branch_imm_helper(&write_p_exp, 0x6770, false);

	TEST_ASSERT_EQUAL_HEX16_ARRAY(w_exp, w, 54);

	free(thread_data);
}

void test_scan_riscv()
{
	TEST_IGNORE_MESSAGE("Test not implemented yet.");
}


int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_riscv_copy16);
	RUN_TEST(test_riscv_copy32);
	RUN_TEST(test_riscv_copy64);
	RUN_TEST(test_riscv_check_cb_type);
	RUN_TEST(test_riscv_check_cj_type);
	RUN_TEST(test_riscv_check_uj_type);
	RUN_TEST(test_riscv_check_sb_type);
	RUN_TEST(test_riscv_calc_j_imm);
	RUN_TEST(test_riscv_calc_cj_imm);
	RUN_TEST(test_riscv_calc_b_imm);
	RUN_TEST(test_riscv_calc_cb_imm);
	RUN_TEST(test_riscv_calc_u_imm);
	RUN_TEST(test_riscv_push_helper);
	RUN_TEST(test_riscv_pop_helper);
	RUN_TEST(test_riscv_branch_imm_helper);
	RUN_TEST(test_riscv_copy_to_reg_32bits);
	RUN_TEST(test_riscv_copy_to_reg_64bits);
	RUN_TEST(test_riscv_b_cond_helper);
	RUN_TEST(test_riscv_save_regs);
	RUN_TEST(test_riscv_restore_regs);
	RUN_TEST(test_riscv_branch_jump);
	RUN_TEST(test_riscv_get_inst_length);
	RUN_TEST(test_riscv_branch_jump_cond);
	RUN_TEST(test_riscv_check_free_space);
	RUN_TEST(test_pass1_riscv);
	RUN_TEST(test_riscv_get_mambo_cond);
	RUN_TEST(test_riscv_scanner_deliver_callbacks);
	RUN_TEST(test_riscv_inline_hash_lookup);
	RUN_TEST(test_scan_riscv);
	return UNITY_END();
}
