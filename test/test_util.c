#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Module under test
#include "../util.h"

#include "../dbm.h"

#include "unity/unity.h"

// DEBUG: Turn off debug
#define DEBUG
#ifdef DEBUG
	#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
	#define debug(...)
#endif

void setUp(void) 
{
#if !(defined(__arm__) || defined(__aarch64__) || __riscv_xlen == 64)
	TEST_IGNORE_MESSAGE("This test only executes on a target platform (arm, aarch64 "
		"or RV64GC)");
#endif
}
void tearDown(void) {}

void test_atomic_increment_u32()
{
	uint32_t i = 0;
	TEST_ASSERT_EQUAL_UINT32(1, atomic_increment_u32(&i, 1));
	TEST_ASSERT_EQUAL_UINT32(1, i);

	TEST_ASSERT_EQUAL_UINT32(3, atomic_increment_u32(&i, 2));
	TEST_ASSERT_EQUAL_UINT32(3, i);

	// uint32 overflow test
	i = UINT32_MAX;
	TEST_ASSERT_EQUAL_UINT32(0, atomic_increment_u32(&i, 1));
	TEST_ASSERT_EQUAL_UINT32(0, i);
}

void test_atomic_increment_u64()
{
	uint64_t i = 0;
	TEST_ASSERT_EQUAL_UINT64(1, atomic_increment_u64(&i, 1));
	TEST_ASSERT_EQUAL_UINT64(1, i);

	TEST_ASSERT_EQUAL_UINT64(3, atomic_increment_u64(&i, 2));
	TEST_ASSERT_EQUAL_UINT64(3, i);

	// uint64 overflow test
	i = UINT64_MAX;
	TEST_ASSERT_EQUAL_UINT64(0, atomic_increment_u64(&i, 1));
	TEST_ASSERT_EQUAL_UINT64(0, i);
}

void test_atomic_decrement_if_positive_i32()
{
	int32_t i = 1;
	TEST_ASSERT_EQUAL_INT32(0, atomic_decrement_if_positive_i32(&i, 1));
	TEST_ASSERT_EQUAL_INT32(0, i);

	TEST_ASSERT_EQUAL_INT32(-1, atomic_decrement_if_positive_i32(&i, 1));
	TEST_ASSERT_EQUAL_INT32(0, i);

	i = -420;
	TEST_ASSERT_EQUAL_INT32(-1, atomic_decrement_if_positive_i32(&i, 1));
	TEST_ASSERT_EQUAL_INT32(-420, i);
}

void test_atomic_increment_i32()
{
	int32_t i = 0;
	TEST_ASSERT_EQUAL_INT32(1, atomic_increment_i32(&i, 1));
	TEST_ASSERT_EQUAL_INT32(1, i);

	i = -17;
	TEST_ASSERT_EQUAL_INT32(-16, atomic_increment_i32(&i, 1));
	TEST_ASSERT_EQUAL_INT32(-16, i);

	// int32 overflow test
	i = INT32_MAX;
	TEST_ASSERT_EQUAL_INT32(INT32_MIN, atomic_increment_i32(&i, 1));
	TEST_ASSERT_EQUAL_INT32(INT32_MIN, i);
}

void test_atomic_increment_i64()
{
	int64_t i = 0;
	TEST_ASSERT_EQUAL_INT64(1, atomic_increment_i64(&i, 1));
	TEST_ASSERT_EQUAL_INT64(1, i);

	i = -17;
	TEST_ASSERT_EQUAL_INT64(-16, atomic_increment_i64(&i, 1));
	TEST_ASSERT_EQUAL_INT64(-16, i);

	// int64 overflow test
	i = INT64_MAX;
	TEST_ASSERT_EQUAL_INT64(INT64_MIN, atomic_increment_i64(&i, 1));
	TEST_ASSERT_EQUAL_INT64(INT64_MIN, i);
}

void test_atomic_increment_uptr()
{
	uintptr_t i = 0;
	TEST_ASSERT_EQUAL_PTR(1, atomic_increment_uptr(&i, 1));
	TEST_ASSERT_EQUAL_PTR(1, i);

	// uintptr overflow test
	i = UINTPTR_MAX;
	TEST_ASSERT_EQUAL_INT64(0, atomic_increment_uptr(&i, 1));
	TEST_ASSERT_EQUAL_INT64(0, i);
}

void test_atomic_increment_int()
{
	int i = 0;
	TEST_ASSERT_EQUAL_INT(1, atomic_increment_int(&i, 1));
	TEST_ASSERT_EQUAL_INT(1, i);

	i = -17;
	TEST_ASSERT_EQUAL_INT(-16, atomic_increment_int(&i, 1));
	TEST_ASSERT_EQUAL_INT(-16, i);

	// int overflow test
	i = INT_MAX;
	TEST_ASSERT_EQUAL_INT(INT_MIN, atomic_increment_int(&i, 1));
	TEST_ASSERT_EQUAL_INT(INT_MIN, i);
}

void test_atomic_increment_uint()
{
	unsigned int i = 0;
	TEST_ASSERT_EQUAL_UINT(1, atomic_increment_uint(&i, 1));
	TEST_ASSERT_EQUAL_UINT(1, i);

	TEST_ASSERT_EQUAL_UINT(3, atomic_increment_uint(&i, 2));
	TEST_ASSERT_EQUAL_UINT(3, i);

	// uint overflow test
	i = UINT_MAX;
	TEST_ASSERT_EQUAL_UINT(0, atomic_increment_uint(&i, 1));
	TEST_ASSERT_EQUAL_UINT(0, i);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_atomic_increment_u32);
	RUN_TEST(test_atomic_increment_u64);
	RUN_TEST(test_atomic_decrement_if_positive_i32);
	RUN_TEST(test_atomic_increment_i32);
	RUN_TEST(test_atomic_increment_i64);
	RUN_TEST(test_atomic_increment_uptr);
	RUN_TEST(test_atomic_increment_int);
	RUN_TEST(test_atomic_increment_uint);
	return UNITY_END();
}