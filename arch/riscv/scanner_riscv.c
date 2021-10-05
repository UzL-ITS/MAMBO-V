#ifdef DBM_ARCH_RISCV64

#include <assert.h>
#include <stdio.h>

#include "../../dbm.h"
#include "../../scanner_common.h"

#include "../../pie/pie-riscv-decoder.h"
#include "../../pie/pie-riscv-encoder.h"
#include "../../pie/pie-riscv-field-decoder.h"

#include "../../api/helpers.h"

/*
 * Instead of the 32 NOP instruction, two C.NOP instructions are placed. If this double
 * NOP replaced by an instruction it does not matter whether the replacment is a 32 bit
 * 16 bit instruction. In case of a 16 bit instruction one C.NOP is left and the code
 * stays functional.
 */
#define NOP_INSTRUCTION 0x00010001		// 2x C.NOP
#define C_NOP_INSTRUCTION 0x0001		// C.NOP

#define MIN_FSPACE 68

#ifdef MODULE_ONLY
	// TODO: rather mock these functions
	#define record_cc_link(...)
	#define allocate_bb(...) 0
#endif

#ifdef DEBUG
	#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
	#define debug(...)
#endif

#define riscv_copy16(write_p, read_address) *((*write_p)++) = *read_address
#define riscv_copy32(write_p, read_address) **(uint32_t**)write_p = \
	*(uint32_t *)read_address; *write_p += 2
#define riscv_copy64(write_p, read_address) **(uint64_t**)write_p = \
	*(uint64_t *)read_address; *write_p += 4

#define riscv_check_cb_type(off, rs1) (rs1 >= x8 && rs1 <= x15 && off < 256 \
	&& off >= -256)
#define riscv_check_cj_type(off) (off < 2048 && off >= -2048)
#define riscv_check_uj_type(off) (off < 0x100000 && off >= -0x100000)
#define riscv_check_sb_type(off) (off < 0x1000 && off >= -0x1000)

#define riscv_calc_j_imm(rawimm, value) *value = (rawimm & (1 << 19)) << 1 \
												| (rawimm & (0x3FF << 9)) >> 8 \
												| (rawimm & (1 << 8)) << 3 \
												| (rawimm & (0xFF)) << 12
#define riscv_calc_cj_imm(rawimm, value) *value = (rawimm & (1 << 10)) << 1 \
												| (rawimm & (1 << 9)) >> 5 \
												| (rawimm & (3 << 7)) << 1 \
												| (rawimm & (1 << 6)) << 4 \
												| (rawimm & (1 << 5)) << 1 \
												| (rawimm & (1 << 4)) << 3 \
												| rawimm & (7 << 1) \
												| (rawimm & 1) << 5
#define riscv_calc_b_imm(rawimmhi, rawimmlo, value) *value = (rawimmhi & (1 << 6)) << 6 \
														   | (rawimmhi & 0x3F) << 5 \
														   | (rawimmlo & (0xF << 1)) \
														   | (rawimmlo & 1) << 11
#define riscv_calc_cb_imm(rawimmhi, rawimmlo, value) *value = (rawimmhi & (1 << 2)) << 6 \
															| (rawimmhi & 3) << 3 \
															| (rawimmlo & (3 << 3)) << 3 \
															| (rawimmlo & (3 << 1)) \
															| (rawimmlo & 1) << 5
#define riscv_calc_u_imm(rawimm, value) *value = (rawimm & 0xFFFFF) << 12

void riscv_push_helper(uint16_t **write_p, enum reg reg)
{
	/*
	 * Push register
	 * 					+-------------------------------+
	 * 					|	C.ADDI	sp, -8				|
	 *					|	C.SDSP	reg, 0				|
	 * 					+-------------------------------+
	 * [Size: 4 B]
	 */
	riscv_c_addi(write_p, sp, 1, 24);
	(*write_p)++;
	riscv_c_sdsp(write_p, reg, 0);
	(*write_p)++;
}

void riscv_pop_helper(uint16_t **write_p, enum reg reg)
{
	/*
	 * Pop register
	 * 					+-------------------------------+
	 * 					|	C.LDSP	reg, 0				|
	 * 					|	C.ADDI	sp, 8				|
	 * 					+-------------------------------+
	 * [Size: 4 B]
	 */
	riscv_c_ldsp(write_p, reg, 0, 0);
	(*write_p)++;
	riscv_c_addi(write_p, sp, 0, 8);
	(*write_p)++;
}

int riscv_branch_imm_helper(uint16_t **write_p, uint64_t target, bool link)
{
	int64_t offset = target - (uint64_t)*write_p;
	if((offset & 1) != 0)
		return -1;
	// Write compressed instruction if possible
	if (riscv_check_cj_type(offset)) {
		// Format offset to: offset[11|4|9:8|10|6|7|3:1|5]
		unsigned int imm = ((offset & (1 << 11)) >> 1)
							| ((offset & (1 << 4)) << 5)
							| ((offset & (3 << 8)) >> 1)
							| ((offset & (1 << 10)) >> 4)
							| ((offset & (1 << 6)) >> 1)
							| ((offset & (1 << 7)) >> 3)
							| (offset & (7 << 1))
							| ((offset & (1 << 5)) >> 5);
		if (link)
			// C.JAL offset
			riscv_c_jal(write_p, imm);
		else
			// C.J offset
			riscv_c_j(write_p, imm);
		(*write_p)++;
	}
	// Check if target in range (+-1 MiB)
	else if (riscv_check_uj_type(offset)) {
		// Format offset to: offset[20|10:1|11|19:12]
		unsigned int imm = ((offset & (1 << 20)) >> 1)
						   | ((offset & (0x3FF << 1)) << 8)
						   | ((offset & (1 << 11)) >> 3)
						   | ((offset & (0xFF << 12)) >> 12);
		// zero or ra (x0 or x1) are used as link registers (x0 means no link)
		// JAL link, offset
		riscv_jal(write_p, link, imm);
		*write_p += 2;
	} else {
		return -1;
	}
	return 0;
}

int riscv_large_jump_helper(uint16_t **write_p, uint64_t target, bool link, enum reg tr)
{
	/*
	 * Large relative jump
	 *					+-------------------------------+
	 *					|	AUIPC	tr, offset[31:12] + offset[11]
	 *					|	JALR	link, offset[11:0](tr)
	 *					+-------------------------------+
	 * 
	 * //! WARNING: Inserted code overrides register tr!
	 * 
	 * [Size: 8 B]
	 */
	int64_t offset = target - (uint64_t)*write_p;

	if((offset & 1) != 0)
		return -1;

	/*
	 * NOTE: The upper boundary is 0x7ffff7ff because 12th bit is sort of a second sign
	 * bit. 0x7ffff800 offset and above cannot be achieved by AUIPC + 12 bit signed
	 * offset. It also causes the lower boundary to be -0x80000800 because with AUIPC
	 * up to 0x80000000 can be subtracted and an additional 0x800 can be subtracted
	 * with the jump offset.
	 */
	if (offset >= -0x80000800L && offset < 0x7ffff800L) {
		int immhi = (offset >> 12) + ((offset >> 11) & 1);
		int immlo = offset & 0xFFF;
		// AUIPC tr, immhi
		riscv_auipc(write_p, tr, immhi);
		*write_p += 2;
		// zero or ra (x0 or x1) are used as link registers (x0 means no link)
		// JALR link, immlo(rt)
		riscv_jalr(write_p, link, tr, immlo);
		*write_p += 2;
	} else {
		return -2;
	}
	return 0;
}

void riscv_cc_branch(dbm_thread *thread_data, uint16_t *write_p, uint64_t target)
{
	riscv_branch_imm_helper(&write_p, target, false);

	record_cc_link(thread_data, (uintptr_t)write_p, target);
}

int riscv_b_cond_helper(uint16_t **write_p, uint64_t target, mambo_cond *cond)
{
	int64_t offset = target - (uint64_t)*write_p;

	enum reg r1 = cond->r1;
	enum reg r2 = cond->r2;

	if((offset & 1) != 0)
		return -1;

	// Handle pseudo condition "always" (aka no condition)
	if (cond->cond == AL) {
		return riscv_branch_imm_helper(write_p, target, cond->r1);
	}

	// Write compressed instruction if possible
	if ((cond->cond == EQ || cond->cond == NE) && r2 == x0 && riscv_check_cb_type(offset, r1)) {
		unsigned int immlo = ((offset >> 5) & 1)
						   | (offset & (3 << 1))
						   | ((offset & (3 << 6)) >> 3);
		unsigned int immhi = ((offset >> 3) & 3) | ((offset >> 6) & 4);

		switch (cond->cond) {
		case EQ:
			// C.BEQZ r1, offset (only lower 3 bits of r1 are written)
			riscv_c_beqz(write_p, r1, immhi, immlo);
			break;
		case NE:
			// C.BNEZ r1, offset (only lower 3 bits of r1 are written)
			riscv_c_bnez(write_p, r1, immhi, immlo);
			break;

		default:
			return -1;
			break;
		}
		(*write_p)++;
	}

	// Offsets up to +-4 KiB are possible
	else if (riscv_check_sb_type(offset)) {
		unsigned int immlo = ((offset >> 11) & 1) | (offset & (0xF << 1));
		unsigned int immhi = ((offset >> 6) & 64) | ((offset >> 5) & 0x3F);

		switch (cond->cond) {
		case EQ:
			// C.BEQ r1, r2, offset
			riscv_beq(write_p, r1, r2, immhi, immlo);
			break;
		case NE:
			// C.BNE r1, r2, offset
			riscv_bne(write_p, r1, r2, immhi, immlo);
			break;
		case LT:
			// C.BLT r1, r2, offset
			riscv_blt(write_p, r1, r2, immhi, immlo);
			break;
		case GE:
			// C.BGE r1, r2, offset
			riscv_bge(write_p, r1, r2, immhi, immlo);
			break;
		case LTU:
			// C.BLTU r1, r2, offset
			riscv_bltu(write_p, r1, r2, immhi, immlo);
			break;
		case GEU:
			// C.BGEU r1, r2, offset
			riscv_bgeu(write_p, r1, r2, immhi, immlo);
			break;

		default:
			return -1;
			break;
		}
		*write_p += 2;
	}
	else {
		return -1;
	}
	return 0;
}

int riscv_bez_helper(uint16_t **write_p, enum reg reg, uint64_t target)
{
	mambo_cond cond = {reg, x0, EQ};
	return riscv_b_cond_helper(write_p, target, &cond);
}

int riscv_bnez_helper(uint16_t **write_p, enum reg reg, uint64_t target)
{
	mambo_cond cond = {reg, x0, NE};
	return riscv_b_cond_helper(write_p, target, &cond);
}

void riscv_save_regs(uint16_t **write_p, uint32_t reg_mask)
{
	enum reg reg = 0;
	while (reg_mask > 0) {
		if (reg_mask & 1) {
			riscv_push_helper(write_p, reg);
		}
		reg_mask >>= 1;
		reg++;
	}
}

void riscv_restore_regs(uint16_t **write_p, uint32_t reg_mask)
{
	enum reg reg = 31;
	while (reg_mask > 0) {
		if (reg_mask & 0x80000000)
			riscv_pop_helper(write_p, reg);
		reg_mask <<= 1;
		reg--;
	}
}

void riscv_copy_to_reg_32bits(uint16_t **write_p, enum reg reg, uint32_t value)
{
	/* NOTE: Can be optimized for size by using compressed instructions but it
	 * will cost translation performance.
	 */

	uint32_t lo = value & 0xFFF;
	uint32_t hi = (value >> 12) & 0xFFFFF;

	/*
	 * Load 32 bit immediate value
	 * 					+-------------------------------+
	 * 				**	|	LUI		reg, hi				|
	 * 					|	ADDI	reg, reg, lo		|
	 * 					+-------------------------------+
	 * 
	 * ** if value has more than 12 bits
	 * 
	 * [Size: 4-8 B]
	 */
	bool upper_needed = (int32_t)value > 0x7FF || (int32_t)value < -0x800;
	if (upper_needed) {
		if (lo >> 11 == 1)
			hi++;
		riscv_lui(write_p, reg, hi);
		*write_p += 2;
	}

	if (lo > 0 && upper_needed) {
		riscv_addi(write_p, reg, reg, lo);
		*write_p += 2;
	} else if (!upper_needed) {
		riscv_addi(write_p, reg, x0, lo);
		*write_p += 2;
	}
}

void riscv_copy_to_reg_64bits(uint16_t **write_p, enum reg reg, uint64_t value)
{
	/*
	 * Write 64 bit value to memory and load it into the register.
	 * It is not possible to load a 64 bit immediate with LUI and ADDI. Instead, this
	 * function writes code similar to a compiler by writing the immediate to
	 * memory and loading it to a register from there.
	 * 
	 * 					+-------------------------------+
	 * 					|	C.J		.+10				|
	 * 					|								|
	 * 					|	.dword value				|
	 * 					|								|
	 * 					|	AUIPC	reg, 0				|
	 * 					|	LD		reg, -8(reg)		|
	 * 					+-------------------------------+
	 * [Size: 18 B]
	 */
	riscv_c_j(write_p, 10);
	(*write_p)++;
	
	// .dword value
	riscv_copy64(write_p, &value);

	riscv_auipc(write_p, reg, 0);
	*write_p += 2;
	riscv_ld(write_p, reg, reg, -8);
	*write_p += 2;
}

void riscv_branch_jump(dbm_thread *thread_data, uint16_t **write_p, int basic_block,
	uint64_t target, uint32_t flags)
{
	/*
	 *					+-------------------------------+
	 *				**	|	LI		x10, target			|	dispatcher: target
	 *				##	|	LI		x11, basic_block	|	dispatcher: source_index
	 *				##	|	J		DISPATCHER			|	Large jump
	 *					+-------------------------------+
	 *
	 * ** if REPLACE_TARGET
	 * ## if INSERT_BRANCH
	 * 
	 * //! WARNING: Inserted code may override registers x10, x11 and x12!
	 * 
	 * [Size: 0-30 B]
	 */
	debug("riscv_branch_jump: RISC-V branch target: 0x%lx\n", target);

	if (flags & REPLACE_TARGET) {
		riscv_copy_to_reg_64bits(write_p, x10, target);
	}

	if (flags & INSERT_BRANCH) {
		riscv_copy_to_reg_32bits(write_p, x11, basic_block);
		riscv_large_jump_helper(write_p, thread_data->dispatcher_addr, false, x12);
	}
}

/**
 * Get instuction length of instruction in bytes.
 * @param inst Instruction.
 * @return Instruction length in bytes.
 */
static int riscv_get_inst_length(riscv_instruction inst)
{
	if (inst <= 39)
		return INST_16BIT;
	else
		return INST_32BIT;
}

void riscv_branch_jump_cond(dbm_thread *thread_data, uint16_t **write_p,
	int basic_block, uint64_t target, uint16_t *read_address, mambo_cond *cond,
	int len)
{
	/*
	 * //! Dont change any registers before the conditional branch!
	 * 					+-------------------------------+
	 * 					|	NOP							|	Space for lookup jump
	 * 					|	NOP							|		inserted by dispatcher
	 * 					|								|		(see dispatcher_riscv.c)
	 * 					|	PUSH	x10, x11, x12		|	(Pseudo instruction)
	 * 					|								|
	 * 					|	B(cond)	branch_target		|
	 * 					|								|
	 * 					|	LI		x10, read_address+len	dispatcher: target
	 * 					|	C.J		branch				|
	 * 					|								|
	 * 					| branch_target:				|
	 * 					|	LI		x10, target			|	dispatcher: target
	 * 					|								|
	 * 					| branch:						|
	 * 					|	LI		x11, basic_block	|	dispatcher: source_index
	 * 					|	J		DISPATCHER			|	Long jump
	 * 					+-------------------------------+
	 * [Size: 78 B]
	 */
	uint16_t *cond_branch, *branch_disp;

	debug("riscv_branch_jump_cond: RISC-V branch target: 0x%lx\n", target);

	// NOP
	**(uint32_t **)write_p = NOP_INSTRUCTION;
	*write_p += 2;
	// NOP
	**(uint32_t **)write_p = NOP_INSTRUCTION;
	*write_p += 2;

	// PUSH x10, x11, x12
	riscv_save_regs(write_p, (m_x10 | m_x11 | m_x12));

	// B(cond) branch_target (added later)
	cond_branch = *write_p;
	// Write NOPs in case of 16 bit instruction inserted
	**(uint32_t **)write_p = NOP_INSTRUCTION;
	*write_p += 2;

	// LI x10, read_address+len
	riscv_copy_to_reg_64bits(write_p, x10, (uint64_t)read_address + len);
	// C.J branch (added later)
	branch_disp = *write_p;
	(*write_p)++;

	// branch_target:
	// Insert "B(cond) branch_target" at cond_branch
	riscv_b_cond_helper(&cond_branch, (uint64_t)*write_p, cond);
	// LI x10, target
	riscv_copy_to_reg_64bits(write_p, x10, target);

	// branch:
	// Insert "C.J branch" at branch_disp
	riscv_branch_imm_helper(&branch_disp, (uint64_t)*write_p, false);
	// LI x11, basic_block
	riscv_copy_to_reg_32bits(write_p, x11, basic_block);
	// J DISPATCHER
	riscv_large_jump_helper(write_p, thread_data->dispatcher_addr, false, x12);
}

void riscv_check_free_space(dbm_thread *thread_data, uint16_t **write_p,
	uint16_t **data_p, uint32_t size, int cur_block)
{
	int basic_block;

	if ((((uint64_t)*write_p) + size) >= (uint64_t)*data_p) {
		basic_block = allocate_bb(thread_data);
		thread_data->code_cache_meta[basic_block].actual_id = cur_block;
		if ((uint16_t *)&thread_data->code_cache->blocks[basic_block] != *data_p) {
			riscv_branch_imm_helper(write_p,
				(uint64_t)&thread_data->code_cache->blocks[basic_block], false);
			*write_p = (uint16_t *)&thread_data->code_cache->blocks[basic_block];
		}
		*data_p = (uint16_t *)&thread_data->code_cache->blocks[basic_block];
		*data_p += BASIC_BLOCK_SIZE;
	}
}

void pass1_riscv(uint16_t *read_address, branch_type *bb_type)
{
	*bb_type = unknown;

	while (*bb_type == unknown) {
		riscv_instruction inst = riscv_decode(read_address);

		switch (inst) {
		case RISCV_JAL:
			*bb_type = uncond_imm_riscv;
			break;
		case RISCV_JALR:
			*bb_type = uncond_reg_riscv;
			break;
		case RISCV_C_J:
			*bb_type = uncond_imm_riscv;
			break;
		case RISCV_C_JR:
			*bb_type = uncond_reg_riscv;
			break;
		case RISCV_C_JAL:
			*bb_type = uncond_imm_riscv;
			break;
		case RISCV_C_JALR:
			*bb_type = uncond_reg_riscv;
			break;
		case RISCV_C_BEQZ:
		case RISCV_C_BNEZ:
		case RISCV_BEQ:
		case RISCV_BNE:
		case RISCV_BLT:
		case RISCV_BGE:
		case RISCV_BLTU:
		case RISCV_BGEU:
			*bb_type = cond_imm_riscv;
			break;
		case RISCV_INVALID:
		case RISCV_C_ILLEGAL:
			return;
		}
		read_address += riscv_get_inst_length(inst);
	}
}

int riscv_get_mambo_cond(riscv_instruction inst, uint16_t *read_address, 
	mambo_cond *cond, uint64_t *target)
{
	unsigned int immhi, immlo;

	switch (inst) {
	case RISCV_C_BEQZ:
		cond->cond = EQ;
		riscv_c_beqz_decode_fields(read_address, &cond->r1, &immhi, &immlo);
		cond->r2 = x0;
		break;
	case RISCV_C_BNEZ:
		cond->cond = NE;
		riscv_c_bnez_decode_fields(read_address, &cond->r1, &immhi, &immlo);
		cond->r2 = x0;
		break;
	case RISCV_BEQ:
		cond->cond = EQ;
		riscv_beq_decode_fields(read_address, &cond->r1, &cond->r2, &immhi, &immlo);
		break;
	case RISCV_BNE:
		cond->cond = NE;
		riscv_bne_decode_fields(read_address, &cond->r1, &cond->r2, &immhi, &immlo);
		break;
	case RISCV_BLT:
		cond->cond = LT;
		riscv_blt_decode_fields(read_address, &cond->r1, &cond->r2, &immhi, &immlo);
		break;
	case RISCV_BGE:
		cond->cond = GE;
		riscv_bge_decode_fields(read_address, &cond->r1, &cond->r2, &immhi, &immlo);
		break;
	case RISCV_BLTU:
		cond->cond = LTU;
		riscv_bltu_decode_fields(read_address, &cond->r1, &cond->r2, &immhi, &immlo);
		break;
	case RISCV_BGEU:
		cond->cond = GEU;
		riscv_bgeu_decode_fields(read_address, &cond->r1, &cond->r2, &immhi, &immlo);
		break;
	default:
		return -1;
	}

	if (target != NULL) {
		int64_t offset;
		if (riscv_get_inst_length(inst) == INST_32BIT) {
			offset = (immhi & 64) << 6
				| (immhi & 0x2F) << 5
				| (immlo & 0x1E)
				| (immlo & 1) << 11;
			offset = sign_extend64(13, offset);
		} else {
			offset = (immhi & 4) << 6
				| (immhi & 3) << 3
				| (immlo & 0x18) << 3
				| (immlo & 6)
				| (immlo & 1) << 5;
			offset = sign_extend64(9, offset);
		}
		*target = (uint64_t)read_address + offset;
	}
	return 0;
}

/**
 * Deliver callbacks to registered plugins.
 * @param thread_data Thread data of current thread.
 * @param cb_id Callback ID.
 * @param o_read_address Pointer to the current reading location.
 * @param inst RISC-V instruction at \c read_address .
 * @param o_write_p Pointer to the writing location.
 * @param o_data_p Pointer to the end of the basic block space.
 * @param basic_block Index of current block.
 * @param type Type of the code cache block.
 * @param allow_write Allow plugin to add instumentation code.
 * @param stop Ponter to scanner loop stop.
 * @return Intructions have been replaced by plugin.
 */
bool riscv_scanner_deliver_callbacks(dbm_thread *thread_data, mambo_cb_idx cb_id, 
	uint16_t **o_read_address, riscv_instruction inst, uint16_t **o_write_p, 
	uint16_t **o_data_p, int basic_block, cc_type type, bool allow_write, bool *stop) 
{
	bool replaced = false;
#ifdef PLUGINS_NEW
	if (global_data.free_plugin > 0) {
		uint16_t *write_p = *o_write_p;
		uint16_t *data_p = *o_data_p;
		uint16_t *read_address = *o_read_address;

		mambo_cond cond = {0, 0, AL};
		riscv_get_mambo_cond(inst, read_address, &cond, NULL);

		mambo_context ctx;
		set_mambo_context_code(&ctx, thread_data, cb_id, type, basic_block, 
			RISCV64_INST, inst, cond, read_address, write_p, data_p, stop);
		
		for (int i = 0; i < global_data.free_plugin; i++) {
			if (global_data.plugins[i].cbs[cb_id] != NULL) {
				ctx.code.write_p = write_p;
				ctx.code.data_p = data_p;
				ctx.plugin_id = i;
				ctx.code.replace = false;
				ctx.code.available_regs = ctx.code.pushed_regs;
				global_data.plugins[i].cbs[cb_id](&ctx);
				if (allow_write) {
					if (replaced && (write_p != ctx.code.write_p || ctx.code.replace)) {
						fprintf(stderr, "MAMBO API WARNING: plugin %d added code for "
							"overridden instruction (%p).\n", i, read_address);
					}
					if (ctx.code.replace) {
						if (cb_id == PRE_INST_C) {
							replaced = true;
						} else {
						fprintf(stderr, "MAMBO API WARNING: plugin %d set replace_inst "
							"for a disallowed event (at %p).\n", i, read_address);
						}
					}
					assert(count_bits(ctx.code.pushed_regs) == 
						ctx.code.plugin_pushed_reg_count);
					if (allow_write && ctx.code.pushed_regs) {
						emit_pop(&ctx, ctx.code.pushed_regs);
					}
					write_p = ctx.code.write_p;
					data_p = ctx.code.data_p;
					riscv_check_free_space(thread_data, &write_p, &data_p, MIN_FSPACE, 
						basic_block);
				} else {
					assert(ctx.code.write_p == write_p);
					assert(ctx.code.data_p == data_p);
				}
			}
		}

		if (cb_id == PRE_BB_C) {
			watched_functions_t *wf = &global_data.watched_functions;
			for (int i = 0; i < wf->funcp_count; i++) {
				if (read_address == wf->funcps[i].addr) {
					_function_callback_wrapper(&ctx, wf->funcps[i].func);
					if (ctx.code.replace) {
						read_address = ctx.code.read_address;
					}
					write_p = ctx.code.write_p;
					data_p = ctx.code.data_p;
					riscv_check_free_space(thread_data, &write_p, &data_p, MIN_FSPACE, 
						basic_block);
				}
			}
		}

		*o_write_p = write_p;
		*o_data_p = data_p;
		*o_read_address = read_address;
	}
#endif
	return replaced;
}

void riscv_inline_hash_lookup(dbm_thread *thread_data, int basic_block,
	uint16_t **write_p, uint16_t *read_address, enum reg rn, uint32_t offset, 
	enum reg link, bool set_meta, int len)
{
	/*
	 * Indirect Branch Lookup
	 * 
	 * 					+-------------------------------+
	 * 					|	PUSH	x10, x11, x12		|	(Pseudo instruction)
	 * 				**	|	ADDI	x11, rn, offset		|	x11 = rn + offset
	 * 				##	|	LI		link, read_address+len	len is 2 or 4
	 * 					|	LI		x10, &hash_table	|
	 * 					|	LI		x_tmp, HASH_MASK<<1	|	HASH_MASK = 0x7FFFF
	 * 					|	AND		x_tmp, x_tmp, rn	|
	 * 					|	C.SLLI	x_tmp, 3			|	SLL 3 + SLL 1 (from mask)
	 * 					|	C.ADD	x10, x_tmp			|		= SLL 4 for 16 byte
	 * 					|								|		struct size
	 * 					| lin_probing:					|
	 * 					|	C.LD	x_tmp, 0(x10)		|
	 * 					|	C.ADDI	x10, 16				|
	 * 					|	C.BEQZ	x_tmp, not_found	|
	 * 					|	BNE		x_tmp, rn, lin_probing
	 * 					|	LD		x10, -8(x10)		|	load code_cache_address
	 * 					|	POP		x12					|	(Pseudo instruction)
	 * 					|	C.JR	x10					|
	 * 					|								|
	 * 					| not_found:					|
	 * 					|	C.MV	x10, rn				|	dispatcher: target
	 * 					|	LI		x11, basic_block	|	dispatcher: source_index
	 * 					|	J		DISPATCHER			|	Large jump
	 * 					+-------------------------------+
	 * 
	 * ** if rn is x10, x11, or return address register (if JALR or C.JALR), 
	 * 	  or offset != 0
	 * ## for JALR or C.JALR
	 * 
	 * [Size: 80-104 B]
	 */

	uint16_t *lin_probing;
	uint16_t *branch_to_not_found;
	enum reg x_spc, x_tmp;
	bool use_x12 = false;

	if ((rn == x10) || (rn == x11) || (rn == link) || offset != 0) {
		x_spc = x11;
		x_tmp = x12;
		use_x12 = true;
	} else {
		x_spc = rn;
		x_tmp = x11;
	}

	if (set_meta) {
		thread_data->code_cache_meta[basic_block].rn = x_spc;
	}

	// PUSH x10, x11, x12
	riscv_save_regs(write_p, (m_x10 | m_x11 | m_x12));

	if (use_x12) {
		//ADDI x11, rn, offset
		riscv_addi(write_p, x_spc, rn, offset);
		*write_p += 2;
	}

	if (link)
		//LI link, read_address+len
		riscv_copy_to_reg_64bits(write_p, link, (uint64_t)read_address + len);
	
	// LI x10, &hash_table
	riscv_copy_to_reg_64bits(write_p, x10, 
		(uint64_t)&thread_data->entry_address.entries);

	// LI x_tmp, HASH_MASK << 1
	/* Hash mask shifted here because of 16 bit alignment making the least segnificant
	 * bit always 0.
	 */
	riscv_copy_to_reg_32bits(write_p, x_tmp, CODE_CACHE_HASH_SIZE << 1);
	// AND x_tmp, x_tmp, rn
	riscv_and(write_p, x_tmp, x_tmp, x_spc);
	*write_p += 2;

	// C.SLLI x_tmp, 3
	riscv_c_slli(write_p, x_tmp, 0, 3);
	(*write_p)++;
	// C.ADD x10, x_tmp
	riscv_c_add(write_p, x10, x_tmp);
	(*write_p)++;

	// lin_probing:
	lin_probing = *write_p;

	// C.LD x_tmp, 0(x10)
	riscv_c_ld(write_p, x_tmp, x10, 0, 0);
	(*write_p)++;
	// C.ADDI x10, 16
	riscv_c_addi(write_p, x10, 0, 16);
	(*write_p)++;

	// C.BEQZ x_tmp, not_found (added later)
	branch_to_not_found = (*write_p)++;

	// BNE x_tmp, rn, lin_probing
	{
		mambo_cond cond = {x_tmp, x_spc, NE};
		riscv_b_cond_helper(write_p, (uint64_t)lin_probing, &cond);
	}

	// LD x10, -8(x10)
	riscv_ld(write_p, x10, x10, -8);
	*write_p += 2;

	// POP x12
	riscv_pop_helper(write_p, x12);
	
	// C.JR x10
	riscv_c_jr(write_p, x10);
	(*write_p)++;

	// Insert "C.BEQZ x_tmp, not_found" at branch_to_not_found (above)
	riscv_bez_helper(&branch_to_not_found, x_tmp, (uint64_t)(*write_p));

	// C.MV x10, rn
	riscv_c_mv(write_p, x10, x_spc);
	(*write_p)++;

	// LI x11, basic_block
	riscv_copy_to_reg_32bits(write_p, x11, basic_block);

	// J DISPATCHER
	riscv_large_jump_helper(write_p, (uint64_t)thread_data->dispatcher_addr, false, x12);
}

size_t scan_riscv(dbm_thread *thread_data, uint16_t *read_address, int basic_block,
	cc_type type, uint16_t *write_p)
{
	uint16_t *start_scan = read_address;
	bool stop = false;
	uint16_t *start_address;
	uint16_t *data_p;

	if (write_p == NULL) {
		write_p = (uint16_t *) &thread_data->code_cache->blocks[basic_block];
	}

	start_address = write_p;

	if (type == mambo_bb) {
		data_p = write_p + BASIC_BLOCK_SIZE;
	} else { // mambo_trace, mambo_trace_entry
		data_p = (uint16_t *)&thread_data->code_cache->traces + (TRACE_CACHE_SIZE / 4);
		thread_data->code_cache_meta[basic_block].free_b = 0;
	}

	/*
	 * On context switches registers x10 and x11 are used to store the target
	 * address and the Basic Block number respectively. Before
	 * overwriting the values of these two registers they are pushed to the
	 * Stack. This means that at the start of every Basic Block x10 and x11 have
	 * to be popped from the Stack. The same is true for trace entries, however
	 * trace fragments do not need a pop instruction.
	 */
	if (type != mambo_trace) {
		riscv_restore_regs(&write_p, (m_x10 | m_x11));
	}

#ifdef DBM_TRACES
	branch_type bb_type;
	pass1_riscv(read_address, &bb_type);

	if (type == mambo_bb 
		&& bb_type != uncond_reg_riscv
		&& bb_type != unknown) 
	{
		riscv_save_regs(&write_p, (m_x1 | m_x11));

		riscv_copy_to_reg_32bits(&write_p, x11, (int)basic_block);
		// TODO: [traces] Use large jump
		riscv_branch_imm_helper(&write_p, thread_data->trace_head_incr_addr, true);

		riscv_restore_regs(&write_p, (m_x1 | m_x11));
	}
#endif

	riscv_scanner_deliver_callbacks(thread_data, PRE_FRAGMENT_C, &read_address, -1,
		&write_p, &data_p, basic_block, type, true, &stop);

	riscv_scanner_deliver_callbacks(thread_data, PRE_BB_C, &read_address, -1, &write_p,
		&data_p, basic_block, type, true, &stop);

	while(!stop) {
		debug("RV64 scan read_address: %p, w: : %p, bb: %d\n", read_address, write_p, basic_block);
		riscv_instruction inst = riscv_decode(read_address);
		debug("  instruction enum: %d\n", (inst == RISCV_INVALID) ? -1 : inst);
		if (inst >= 40)
			debug("  instruction word: 0x%08x\n", *(uint32_t *)read_address);
		else 
			debug("  instruction word: 0x%04x\n", *read_address);

#ifdef PLUGINS_NEW
		bool skip_inst = riscv_scanner_deliver_callbacks(thread_data, PRE_INST_C, 
			&read_address, inst, &write_p, &data_p, basic_block, type, true, &stop);

		if (!skip_inst) {
#endif

		switch (inst) {
		case RISCV_C_JAL:
		case RISCV_C_J:
		case RISCV_JAL: {
			// Call dispatcher on jump instructions
			//? Hash lookup could speed it up
			enum reg x;
			unsigned int rawimm;
			uint64_t branch_offset;
			uint64_t target;
			int imm_size;

			if (inst == RISCV_JAL) {
				riscv_jal_decode_fields(read_address, &x, &rawimm);
				if (x != 0) {
					riscv_copy_to_reg_64bits(&write_p, x, 
						(uint64_t)read_address + INST_32BIT);
				}
				riscv_calc_j_imm(rawimm, &branch_offset);
				imm_size = 21;
			} else if (inst == RISCV_C_JAL) {
				riscv_c_jal_decode_fields(read_address, &rawimm);
				riscv_copy_to_reg_64bits(&write_p, ra, 
					(uint64_t)read_address + INST_16BIT);
				riscv_calc_cj_imm(rawimm, &branch_offset);
				imm_size = 12;
			} else { // RISCV_C_J
				riscv_c_j_decode_fields(read_address, &rawimm);
				riscv_calc_cj_imm(rawimm, &branch_offset);
				imm_size = 12;
			}

			branch_offset = sign_extend64(imm_size, branch_offset);
			debug("  Branch offset = 0x%x\n", branch_offset);
			target = (uint64_t)read_address + branch_offset;
#ifdef DBM_LINK_UNCOND_IMM
			thread_data->code_cache_meta[basic_block].exit_branch_type = uncond_imm_riscv;
			thread_data->code_cache_meta[basic_block].exit_branch_addr = write_p;
			thread_data->code_cache_meta[basic_block].branch_taken_addr = target;
			*(uint32_t *)write_p = NOP_INSTRUCTION; // Reserves space for linking branch.
			write_p += 2;
#endif
			riscv_save_regs(&write_p, (m_x10 | m_x11 | m_x12));
			riscv_branch_jump(thread_data, &write_p, basic_block, target, 
				(REPLACE_TARGET | INSERT_BRANCH));
			// NOPs + save_regs + branch_jump <= 68 B
			stop = true;
			break;
		}

		case RISCV_BEQ:
		case RISCV_BNE:
		case RISCV_BLT:
		case RISCV_BGE:
		case RISCV_BLTU:
		case RISCV_BGEU: {
			// Call dispatcher on (conditional) branch instructions (imm)
			// Hash table linking and hash lookup is done by the dispatcher when called
			enum reg rs1, rs2;
			unsigned int immhi, immlo;
			uint64_t branch_offset;
			uint64_t target;
			mambo_cond cond;
			branch_type type;

			riscv_beq_decode_fields(read_address, &rs1, &rs2, &immhi, &immlo);
			cond.r1 = rs1;
			cond.r2 = rs2;
			
			switch (inst) {
			case RISCV_BEQ:
				cond.cond = EQ;
				break;
			case RISCV_BNE:
				cond.cond = NE;
				break;
			case RISCV_BLT:
				cond.cond = LT;
				break;
			case RISCV_BGE:
				cond.cond = GE;
				break;
			case RISCV_BLTU:
				cond.cond = LTU;
				break;
			case RISCV_BGEU:
				cond.cond = GEU;
				break;
			}

			riscv_calc_b_imm(immhi, immlo, &branch_offset);
			branch_offset = sign_extend64(13, branch_offset);

			debug("  Branch offset = 0x%x\n", branch_offset);
			target = (uint64_t)read_address + branch_offset;

#ifdef DBM_LINK_COND_IMM
			// Mark this as the beggining of code emulating B(cond)
			thread_data->code_cache_meta[basic_block].exit_branch_type = cond_imm_riscv;
			thread_data->code_cache_meta[basic_block].exit_branch_addr = write_p;
			thread_data->code_cache_meta[basic_block].branch_taken_addr = target;
			thread_data->code_cache_meta[basic_block].branch_skipped_addr = 
				(uint64_t)read_address + INST_32BIT;
			thread_data->code_cache_meta[basic_block].branch_condition = cond;
			thread_data->code_cache_meta[basic_block].branch_cache_status = 0;
#endif

			riscv_check_free_space(thread_data, &write_p, &data_p, 78, basic_block);
			riscv_branch_jump_cond(thread_data, &write_p, basic_block, target, 
				read_address, &cond, INST_32BIT);
			stop = true;
			break;
		}

		case RISCV_C_BEQZ:
		case RISCV_C_BNEZ: {
			// Call dispatcher on compressed (conditional) branch instructions (imm)
			// Hash table linking and hash lookup is done by the dispatcher when called
			enum reg rs1, rs2;
			unsigned int immhi, immlo;
			uint64_t branch_offset;
			uint64_t target;
			mambo_cond cond;
			branch_type type;

			riscv_c_beqz_decode_fields(read_address, &rs1, &immhi, &immlo);
			cond.r1 = rs1 + 8;
			cond.r2 = x0;
			if (inst == RISCV_C_BEQZ) {
				cond.cond = EQ;
			} else {
				cond.cond = NE;
			}

			riscv_calc_cb_imm(immhi, immlo, &branch_offset);
			branch_offset = sign_extend64(9, branch_offset);
			debug("  Branch offset = 0x%x\n", branch_offset);
			target = (uint64_t)read_address + branch_offset;

#ifdef DBM_LINK_COND_IMM
			// Mark this as the beggining of code emulating B(cond)
			thread_data->code_cache_meta[basic_block].exit_branch_type = cond_imm_riscv;
			thread_data->code_cache_meta[basic_block].exit_branch_addr = write_p;
			thread_data->code_cache_meta[basic_block].branch_taken_addr = target;
			thread_data->code_cache_meta[basic_block].branch_skipped_addr = 
				(uint64_t)read_address + INST_16BIT;
			thread_data->code_cache_meta[basic_block].branch_condition = cond;
			thread_data->code_cache_meta[basic_block].branch_cache_status = 0;
#endif

			riscv_check_free_space(thread_data, &write_p, &data_p, 78, basic_block);
			riscv_branch_jump_cond(thread_data, &write_p, basic_block, target, 
				read_address, &cond, INST_16BIT);
			stop = true;
			break;
		}

		case RISCV_AUIPC: {
			/*
			 * AUIPC is used to build pc-relative addresses. The 20 bit offset is
			 * added to the address of the AUIPC instuction and written to a register.
			 * 
			 * AUIPC replaced by: `riscv_copy_to_reg_64bits`. The calculated absolute
			 * address is calculated from the SPC.
			 */
			enum reg rd;
			unsigned int imm;
			uint64_t offset, pc_rel_addr;

			riscv_auipc_decode_fields(read_address, &rd, &imm);
			imm <<= 12;

			offset = sign_extend64(32, imm);
			pc_rel_addr = (uint64_t)read_address + offset;

			riscv_copy_to_reg_64bits(&write_p, rd, pc_rel_addr);
			debug("AUIPC instrumented, continue scan.\n");
			break;
		}

		case RISCV_JALR: {
			// Instrument jump register instructions by doing a hash lookup
			enum reg rd, rs1;
			unsigned int imm12;

			riscv_jalr_decode_fields(read_address, &rd, &rs1, &imm12);

#ifdef DBM_INLINE_HASH
			// Check for 104 bytes (worst case inline hash lookup)
			riscv_check_free_space(thread_data, &write_p, &data_p, 104, basic_block);
#endif

			thread_data->code_cache_meta[basic_block].exit_branch_type = 
				uncond_reg_riscv;
			thread_data->code_cache_meta[basic_block].exit_branch_addr = write_p;
			thread_data->code_cache_meta[basic_block].rn = rs1;

#ifndef DBM_INLINE_HASH
			riscv_save_regs(&write_p, (m_x10 | m_x11 | m_x12));

			riscv_addi(&write_p, x10, rs1, imm12);
			write_p += 2;

			if (rd != x0) {
				riscv_copy_to_reg_64bits(&write_p, rd, (uint64_t)read_address + INST_32BIT);
			}

			riscv_branch_jump(thread_data, &write_p, basic_block, 0, INSERT_BRANCH);
#else
			riscv_inline_hash_lookup(thread_data, basic_block, &write_p, read_address, 
				rs1, (uint32_t)imm12, rd, true, INST_32BIT);
#endif
			stop = true;
			break;
		}

		case RISCV_C_JALR:
		case RISCV_C_JR: {
			// Instrument compressed jump register instructions by doing a hash lookup
			enum reg rs1;

			riscv_c_jr_decode_fields(read_address, &rs1);

#ifdef DBM_INLINE_HASH
			// Check for 104 bytes (worst case inline hash lookup)
			riscv_check_free_space(thread_data, &write_p, &data_p, 104, basic_block);
#endif

			thread_data->code_cache_meta[basic_block].exit_branch_type = 
				uncond_reg_riscv;
			thread_data->code_cache_meta[basic_block].exit_branch_addr = write_p;
			thread_data->code_cache_meta[basic_block].rn = rs1;

#ifndef DBM_INLINE_HASH
			riscv_save_regs(&write_p, (m_x10 | m_x11 | m_x12));

			riscv_c_mv(&write_p, x10, rs1);
			write_p++;

			if (inst == RISCV_C_JALR) {
				riscv_copy_to_reg_64bits(&write_p, ra, (uint64_t)read_address + INST_16BIT);
			}

			riscv_branch_jump(thread_data, &write_p, basic_block, 0, INSERT_BRANCH);
#else
			enum reg rd = inst == RISCV_C_JALR ? ra : x0; 
			riscv_inline_hash_lookup(thread_data, basic_block, &write_p, read_address, 
				rs1, 0, rd, true, INST_16BIT);
#endif
			stop = true;
			break;
		}

		case RISCV_ECALL:
			// Instrument system calls
			debug("Syscall detected\n");
			riscv_save_regs(&write_p, (m_x1 | m_x8 | m_x12)); // Restored by syscall_wrapper
			riscv_copy_to_reg_64bits(&write_p, x8, (uint64_t)read_address + 4);
			riscv_large_jump_helper(&write_p, thread_data->syscall_wrapper_addr, true, x12);
			// x1 and x8 are replaced with x10 and x11 in stack, so they must be popped
			// at this point. x12 was already popped by syscall_wrapper.
			riscv_restore_regs(&write_p, (m_x10 | m_x11));

			riscv_scanner_deliver_callbacks(thread_data, POST_BB_C, &read_address, -1,
				&write_p, &data_p, basic_block, type, false, &stop);
			// Set the correct address for the PRE_BB_C event
			read_address++;
			riscv_scanner_deliver_callbacks(thread_data, POST_BB_C, &read_address, -1,
				&write_p, &data_p, basic_block, type, true, &stop);
			read_address--;
			break;

		case RISCV_LR_W: {
			/*
			 * Original instruction
			 *          +-------------------------------+
			 *          |   LR.W    x, (y)              |
			 *          +-------------------------------+
			 * 
			 * Translate to
			 *          +-------------------------------+
			 *          |   LW      x, 0(y)             |
			 *          |   C.MV    x31, x              |   Save original x in x31 for
			 *          +-------------------------------+       comparison when SC
			 * // TODO: Don't just hope that x31 is unused!
			 */
			unsigned int aq, rl, x, y;

			riscv_lr_d_decode_fields(read_address, &aq, &rl, &x, &y);
			riscv_lw(&write_p, x, y, 0);
			write_p += 2;
			riscv_c_mv(&write_p, x31, x);
			write_p++;
			break;
		}
		case RISCV_LR_D: {
			/*
			 * Original instruction
			 *          +-------------------------------+
			 *          |   LR.D    x, (y)              |
			 *          +-------------------------------+
			 * 
			 * Translate to
			 *          +-------------------------------+
			 *          |   LD      x, 0(y)             |
			 *          |   C.MV    x31, x              |   Save original x in x31 for
			 *          +-------------------------------+       comparison when SC
			 * // TODO: Don't just hope that x31 is unused!
			 */
			unsigned int aq, rl, x, y;

			riscv_lr_d_decode_fields(read_address, &aq, &rl, &x, &y);
			riscv_lw(&write_p, x, y, 0);
			write_p += 2;
			riscv_c_mv(&write_p, x31, x);
			write_p++;
			break;
		}
		case RISCV_SC_W: {
			/*
			 * Original instruction
			 *          +-------------------------------+
			 *          |   SC.W    x, y, (z)           |
			 *          +-------------------------------+
			 * 
			 * Translate to
			 *          +-------------------------------+
			 *          |   LR.W    x, (z)n             |
			 *          |   BNE     x, x31, .+8         |   Ignore SC as if it fails.
			 *          |   SC.W    x, y, (z)           |       Branch to LR is expected
			 *          +-------------------------------+       to follow.
			 * // TODO: Don't just hope that x31 is unused!
			 */
			unsigned int aq, rl, x, z, y;
			riscv_instruction follow_inst = riscv_decode(read_address + 2);

			riscv_sc_d_decode_fields(read_address, &aq, &rl, &x, &y, &z);
			riscv_lr_w(&write_p, 1, 1, x, z);
			write_p += 2;
			riscv_bne(&write_p, x, x31, 0, 8);
			write_p += 2;
			riscv_copy32(&write_p, read_address);
			break;
		}
		case RISCV_SC_D: {
			/*
			 * Original instruction
			 *          +-------------------------------+
			 *          |   SC.D    x, y, (z)           |
			 *          +-------------------------------+
			 * 
			 * Translate to
			 *          +-------------------------------+
			 *          |   LR.D    x, (z)n             |
			 *          |   BNE     x, x31, .+8         |   Ignore SC as if it fails.
			 *          |   SC.D    x, y, (z)           |       Branch to LR is expected
			 *          +-------------------------------+       to follow.
			 * // TODO: Don't just hope that x31 is unused!
			 */
			unsigned int aq, rl, x, z, y;
			riscv_instruction follow_inst = riscv_decode(read_address + 2);

			riscv_sc_d_decode_fields(read_address, &aq, &rl, &x, &y, &z);
			riscv_lr_d(&write_p, 1, 1, x, z);
			write_p += 2;
			riscv_bne(&write_p, x, x31, 0, 8);
			write_p += 2;
			riscv_copy32(&write_p, read_address);
			break;
		}

		// All other instructions can by copied unmodified
		case RISCV_LUI:
		case RISCV_LB:
		case RISCV_LH:
		case RISCV_LW:
		case RISCV_LBU:
		case RISCV_LHU:
		case RISCV_SB:
		case RISCV_SH:
		case RISCV_SW:
		case RISCV_ADDI:
		case RISCV_SLTI:
		case RISCV_SLTIU:
		case RISCV_XORI:
		case RISCV_ORI:
		case RISCV_ANDI:
		case RISCV_SLLI:
		case RISCV_SRLI:
		case RISCV_SRAI:
		case RISCV_ADD:
		case RISCV_SUB:
		case RISCV_SLL:
		case RISCV_SLT:
		case RISCV_SLTU:
		case RISCV_XOR:
		case RISCV_SRL:
		case RISCV_SRA:
		case RISCV_OR:
		case RISCV_AND:
		case RISCV_FENCE:
		case RISCV_EBREAK:
		case RISCV_LWU:
		case RISCV_LD:
		case RISCV_SD:
		case RISCV_ADDIW:
		case RISCV_SLLIW:
		case RISCV_SRLIW:
		case RISCV_SRAIW:
		case RISCV_ADDW:
		case RISCV_SUBW:
		case RISCV_SLLW:
		case RISCV_SRLW:
		case RISCV_SRAW:
		case RISCV_FENCEI:
		case RISCV_CSRRW:
		case RISCV_CSRRS:
		case RISCV_CSRRC:
		case RISCV_CSRRWI:
		case RISCV_CSRRSI:
		case RISCV_CSRRCI:
		case RISCV_MUL:
		case RISCV_MULH:
		case RISCV_MULHSU:
		case RISCV_MULHU:
		case RISCV_DIV:
		case RISCV_DIVU:
		case RISCV_REM:
		case RISCV_REMU:
		case RISCV_MULW:
		case RISCV_DIVW:
		case RISCV_DIVUW:
		case RISCV_REMW:
		case RISCV_REMUW:
		case RISCV_AMOSWAP_W:
		case RISCV_AMOADD_W:
		case RISCV_AMOXOR_W:
		case RISCV_AMOAND_W:
		case RISCV_AMOOR_W:
		case RISCV_AMOMIN_W:
		case RISCV_AMOMAX_W:
		case RISCV_AMOMINU_W:
		case RISCV_AMOMAXU_W:
		case RISCV_AMOSWAP_D:
		case RISCV_AMOADD_D:
		case RISCV_AMOXOR_D:
		case RISCV_AMOAND_D:
		case RISCV_AMOOR_D:
		case RISCV_AMOMIN_D:
		case RISCV_AMOMAX_D:
		case RISCV_AMOMINU_D:
		case RISCV_AMOMAXU_D:
		case RISCV_FLW:
		case RISCV_FSW:
		case RISCV_FMADD_S:
		case RISCV_FMSUB_S:
		case RISCV_FNMSUB_S:
		case RISCV_FNMADD_S:
		case RISCV_FADD_S:
		case RISCV_FSUB_S:
		case RISCV_FMUL_S:
		case RISCV_FDIV_S:
		case RISCV_FSQRT_S:
		case RISCV_FSGNJ_S:
		case RISCV_FSGNJN_S:
		case RISCV_FSGNJX_S:
		case RISCV_FMIN_S:
		case RISCV_FMAX_S:
		case RISCV_FCVT_W_S:
		case RISCV_FCVT_WU_S:
		case RISCV_FMV_X_W:
		case RISCV_FEQ_S:
		case RISCV_FLT_S:
		case RISCV_FLE_S:
		case RISCV_FCLASS_S:
		case RISCV_FCVT_S_W:
		case RISCV_FCVT_S_WU:
		case RISCV_FMV_W_X:
		case RISCV_FCVT_L_S:
		case RISCV_FCVT_LU_S:
		case RISCV_FCVT_S_L:
		case RISCV_FCVT_S_LU:
		case RISCV_FLD:
		case RISCV_FSD:
		case RISCV_FMADD_D:
		case RISCV_FMSUB_D:
		case RISCV_FNMSUB_D:
		case RISCV_FNMADD_D:
		case RISCV_FADD_D:
		case RISCV_FSUB_D:
		case RISCV_FMUL_D:
		case RISCV_FDIV_D:
		case RISCV_FSQRT_D:
		case RISCV_FSGNJ_D:
		case RISCV_FSGNJN_D:
		case RISCV_FSGNJX_D:
		case RISCV_FMIN_D:
		case RISCV_FMAX_D:
		case RISCV_FCVT_S_D:
		case RISCV_FCVT_D_S:
		case RISCV_FEQ_D:
		case RISCV_FLT_D:
		case RISCV_FLE_D:
		case RISCV_FCLASS_D:
		case RISCV_FCVT_W_D:
		case RISCV_FCVT_WU_D:
		case RISCV_FCVT_D_W:
		case RISCV_FCVT_D_WU:
		case RISCV_FCVT_L_D:
		case RISCV_FCVT_LU_D:
		case RISCV_FMV_X_D:
		case RISCV_FCVT_D_L:
		case RISCV_FCVT_D_LU:
		case RISCV_FMV_D_X:
			debug("Instruction copied.\n");
			riscv_copy32(&write_p, read_address);
			break;

		case RISCV_C_ADDI4SPN:
		case RISCV_C_FLD:
		case RISCV_C_LW:
		case RISCV_C_LD:
		case RISCV_C_FSD:
		case RISCV_C_SW:
		case RISCV_C_SD:
		case RISCV_C_NOP:
		case RISCV_C_ADDI:
		case RISCV_C_ADDIW:
		case RISCV_C_LI:
		case RISCV_C_ADDI16SP:
		case RISCV_C_LUI:
		case RISCV_C_SRLI:
		case RISCV_C_SRAI:
		case RISCV_C_ANDI:
		case RISCV_C_SUB:
		case RISCV_C_XOR:
		case RISCV_C_OR:
		case RISCV_C_AND:
		case RISCV_C_SUBW:
		case RISCV_C_ADDW:
		case RISCV_C_SLLI:
		case RISCV_C_FLDSP:
		case RISCV_C_LWSP:
		case RISCV_C_FLWSP:
		case RISCV_C_LDSP:
		case RISCV_C_MV:
		case RISCV_C_EBREAK:
		case RISCV_C_ADD:
		case RISCV_C_FSDSP:
		case RISCV_C_SWSP:
		case RISCV_C_SDSP:
			debug("Instruction copied.\n");
			riscv_copy16(&write_p, read_address);
			break;

		case RISCV_INVALID:
		case RISCV_C_ILLEGAL:
			if (read_address != start_scan) {
				riscv_save_regs(&write_p, (m_x10 | m_x11 | m_x12));
				riscv_branch_jump(thread_data, &write_p, basic_block, 
					(uint64_t)read_address, (REPLACE_TARGET | INSERT_BRANCH));
				stop = true;
				debug("WARNING: Deferred scanning because of unknown instruction at: "
					"%p\n", read_address);
			} else {
				uint32_t raw_inst = 0;
				if (RISCV_INVALID)
					raw_inst = *(uint32_t *)read_address;
				else
					raw_inst += *read_address;

				fprintf(stderr, "\nMAMBO: Unknown RV64 instruction: %d (0x%x) at %p\n"
					"Copying it unmodified, but future problems are possible\n\n",
					inst, raw_inst, read_address);
				
				if (inst == RISCV_INVALID) {
					riscv_copy32(&write_p, read_address);
				} else {
					riscv_copy16(&write_p, read_address);
				}
			}
			break;
		
		default:
			// Panic
			fprintf(stderr, "Unhandled RV64 instruction: %d at %p\n", inst,
				read_address);
			exit(EXIT_FAILURE);
		} // switch (inst)

#ifdef PLUGINS_NEW
		} // if (!skip_inst)
#endif

		if (data_p < write_p) {
			fprintf(stderr, "%d, inst: %p, :write: %p\n", inst, data_p, write_p);
			while(1);
		}

		if (!stop)
			riscv_check_free_space(thread_data, &write_p, &data_p, MIN_FSPACE,
				basic_block);
		
#ifdef PLUGINS_NEW
		riscv_scanner_deliver_callbacks(thread_data, POST_INST_C, &read_address, inst,
			&write_p, &data_p, basic_block, type, !stop, &stop);
#endif

		read_address += riscv_get_inst_length(inst) / 2;
	} // while(!stop)

	return ((write_p - start_address + 1) * sizeof(*write_p));
}

#endif // DBM_ARCH_RISCV64
