.global start_of_dispatcher_s
start_of_dispatcher_s:

.global push_x1_x31
push_x1_x31:
	/*
	 * Push general purpose registers 
	 * x1 to x31 exept for x10 and x11
	 */
	# Move sp first, so that compressed instructions can be used
	# (SD replaced with C.SDSP by assembler)
	ADDI	sp, sp, -232
	SD	x1, 224(sp)
	SD	x2, 216(sp)
	SD	x3, 208(sp)
	SD	x4, 200(sp)
	SD	x5, 192(sp)
	SD	x6, 184(sp)
	SD	x7, 176(sp)
	SD	x8, 168(sp)
	SD	x9, 160(sp)
	SD	x12, 152(sp)
	SD	x13, 144(sp)
	SD	x14, 136(sp)
	SD	x15, 128(sp)
	SD	x16, 120(sp)
	SD	x17, 112(sp)
	SD	x18, 104(sp)
	SD	x19, 96(sp)
	SD	x20, 88(sp)
	SD	x21, 80(sp)
	SD	x22, 72(sp)
	SD	x23, 64(sp)
	SD	x24, 56(sp)
	SD	x25, 48(sp)
	SD	x26, 40(sp)
	SD	x27, 32(sp)
	SD	x28, 24(sp)
	SD	x29, 16(sp)
	SD	x30, 8(sp)
	SD	x31, 0(sp)
	RET

.global pop_x1_x31
pop_x1_x31:
	/*
	 * Pop general purpose registers 
	 * x1 to x31 exept for x10 and x11
	 */
	LD	x1, 224(sp)
	LD	x2, 216(sp)
	LD	x3, 208(sp)
	LD	x4, 200(sp)
	LD	x5, 192(sp)
	LD	x6, 184(sp)
	LD	x7, 176(sp)
	LD	x8, 168(sp)
	LD	x9, 160(sp)
	LD	x12, 152(sp)
	LD	x13, 144(sp)
	LD	x14, 136(sp)
	LD	x15, 128(sp)
	LD	x16, 120(sp)
	LD	x17, 112(sp)
	LD	x18, 104(sp)
	LD	x19, 96(sp)
	LD	x20, 88(sp)
	LD	x21, 80(sp)
	LD	x22, 72(sp)
	LD	x23, 64(sp)
	LD	x24, 56(sp)
	LD	x25, 48(sp)
	LD	x26, 40(sp)
	LD	x27, 32(sp)
	LD	x28, 24(sp)
	LD	x29, 16(sp)
	LD	x30, 8(sp)
	LD	x31, 0(sp)
	ADDI	sp, sp, 232
	RET

.global dispatcher_trampoline
dispatcher_trampoline:
	# PUSH all general purpose registers but x10, x11
	# x0 and x11 are pushed by the exit stub
	C.ADDI 	sp, -16
	SD	x10, 8(sp)
	JAL 	push_x1_x31

	ADDI	x12, sp, 232
	LD	x13, disp_thread_data

	CALL	dispatcher_addr		# Call far-away function

	JAL	pop_x1_x31
	LD	x10, 0(sp)		# param0 = next_addr 
	LD	x11, 8(sp)		# param1 = old param0
	C.ADDI	sp, 16
	
	J	checked_cc_return

dispatcher_addr: .quad dispatcher

.global disp_thread_data
disp_thread_data: .quad 0

.global checked_cc_return
checked_cc_return:
	# TODO: Complete

.global end_of_dispatcher_s
end_of_dispatcher_s: