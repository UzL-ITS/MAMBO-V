.global start_of_dispatcher_s
start_of_dispatcher_s:

gp_tp_context_switch:
        # Switch register values of gp and tp with shadow values
        # param a0: If set, gp and tp are set to mambo context
        ADDI    sp, sp, -16
        SD      t0, 8(sp)
        SD      t1, 0(sp)

        # Check context status
        LW      t0, gp_tp_mambo_ctx
        BEQ     a0, t0, 1f

        XORI    t0, t0, 1                       # Toggle context status
        SW      t0, gp_tp_mambo_ctx, t1
        LD      t0, gp_shadow
        SD      gp, gp_shadow, t1
        MV      gp, t0
        LD      t0, tp_shadow
        SD      tp, tp_shadow, t1
        MV      tp, t0

1:
        LD      t0, 8(sp)
        LD      t1, 0(sp)
        ADDI    sp, sp, 16
        RET

.global push_x1_x31
push_x1_x31:
        /*
         * Push general purpose registers 
         * x1 to x31 exept for x2 (sp), x1 (ra), x10 and x11
         * NOTE: You may want to also push ra before the subroutine.
         */
        # Move sp first, so that compressed instructions can be used
        # (SD replaced with C.SDSP by assembler)
        ADDI    sp, sp, -224
        SD      x3, 216(sp)
        SD      x4, 208(sp)
        SD      x5, 200(sp)
        SD      x6, 192(sp)
        SD      x7, 184(sp)
        SD      x8, 176(sp)
        SD      x9, 168(sp)
        SD      x12, 160(sp)
        SD      x13, 152(sp)
        SD      x14, 144(sp)
        SD      x15, 136(sp)
        SD      x16, 128(sp)
        SD      x17, 120(sp)
        SD      x18, 112(sp)
        SD      x19, 104(sp)
        SD      x20, 96(sp)
        SD      x21, 88(sp)
        SD      x22, 80(sp)
        SD      x23, 72(sp)
        SD      x24, 64(sp)
        SD      x25, 56(sp)
        SD      x26, 48(sp)
        SD      x27, 40(sp)
        SD      x28, 32(sp)
        SD      x29, 24(sp)
        SD      x30, 16(sp)
        SD      x31, 8(sp)
        # Load gp and tp of MAMBO context and store the client gp and tp
        MV      s0, ra
        SD      a0, 0(sp)
        LI      a0, 1
        JAL     gp_tp_context_switch
        LD      a0, 0(sp)
        MV      ra, s0
        LD      s0, 176(sp)
        ADDI    sp, sp, 8
        RET

.global pop_x1_x31
pop_x1_x31:
        /*
         * Pop general purpose registers 
         * x1 to x31 exept for x2 (sp), x1 (ra), x10 and x11
         * NOTE: You may want to also pop ra after the subroutine.
         */
        # Store gp and tp of MAMBO context and load the client gp and tp
        MV s0, ra
        ADDI sp, sp, -8
        SD a0, 0(sp)
        MV a0, zero
        JAL gp_tp_context_switch
        LD a0, 0(sp)
        MV ra, s0
        # LD      x3, 216(sp)
        # LD      x4, 208(sp)
        LD      x5, 200(sp)
        LD      x6, 192(sp)
        LD      x7, 184(sp)
        LD      x8, 176(sp)
        LD      x9, 168(sp)
        LD      x12, 160(sp)
        LD      x13, 152(sp)
        LD      x14, 144(sp)
        LD      x15, 136(sp)
        LD      x16, 128(sp)
        LD      x17, 120(sp)
        LD      x18, 112(sp)
        LD      x19, 104(sp)
        LD      x20, 96(sp)
        LD      x21, 88(sp)
        LD      x22, 80(sp)
        LD      x23, 72(sp)
        LD      x24, 64(sp)
        LD      x25, 56(sp)
        LD      x26, 48(sp)
        LD      x27, 40(sp)
        LD      x28, 32(sp)
        LD      x29, 24(sp)
        LD      x30, 16(sp)
        LD      x31, 8(sp)
        ADDI    sp, sp, 224
        RET

push_x1_x31_full:
        /*
         * Push general purpose registers. x1 to x31 saved apart from x2 (sp) and 
         * x1 (ra) in ascending order.
         * WARNING: Must be paired with `pop_x1_x31_full` not `pop_x1_x31`!
         * NOTE: You may want to also push ra before the subroutine.
         */
        # Move sp first, so that compressed instructions can be used
        # (SD replaced with C.SDSP by assembler)
        ADDI    sp, sp, -232
        SD      x3, 0(sp)
        SD      x4, 8(sp)
        SD      x5, 16(sp)
        SD      x6, 24(sp)
        SD      x7, 32(sp)
        SD      x8, 40(sp)
        SD      x9, 48(sp)
        SD      x10, 56(sp)
        SD      x11, 64(sp)
        SD      x12, 72(sp)
        SD      x13, 80(sp)
        SD      x14, 88(sp)
        SD      x15, 96(sp)
        SD      x16, 104(sp)
        SD      x17, 112(sp)
        SD      x18, 120(sp)
        SD      x19, 128(sp)
        SD      x20, 136(sp)
        SD      x21, 144(sp)
        SD      x22, 152(sp)
        SD      x23, 160(sp)
        SD      x24, 168(sp)
        SD      x25, 176(sp)
        SD      x26, 184(sp)
        SD      x27, 192(sp)
        SD      x28, 200(sp)
        SD      x29, 208(sp)
        SD      x30, 216(sp)
        SD      x31, 224(sp)
        # Load gp and tp of MAMBO context and store the client gp and tp
        MV      s0, ra
        LI      a0, 1
        JAL     gp_tp_context_switch
        LD      a0, 56(sp)
        MV      ra, s0
        LD      s0, 40(sp)
        RET

pop_x1_x31_full:
        /*
         * Pop general purpose registers. x1 to x31 restored apart from x2 (sp) and 
         * x1 (ra) in ascending order.
         * WARNING: Must be paired with `push_x1_x31_full` not `push_x1_x31`!
         * NOTE: You may want to also pop ra after the subroutine.
         */
        # Store gp and tp of MAMBO context and load the client gp and tp
        MV s0, ra
        MV a0, zero
        JAL gp_tp_context_switch
        MV ra, s0
        # LD      x3, 0(sp)
        # LD      x4, 8(sp)
        LD      x5, 16(sp)
        LD      x6, 24(sp)
        LD      x7, 32(sp)
        LD      x8, 40(sp)
        LD      x9, 48(sp)
        LD      x10, 56(sp)
        LD      x11, 64(sp)
        LD      x12, 72(sp)
        LD      x13, 80(sp)
        LD      x14, 88(sp)
        LD      x15, 96(sp)
        LD      x16, 104(sp)
        LD      x17, 112(sp)
        LD      x18, 120(sp)
        LD      x19, 128(sp)
        LD      x20, 136(sp)
        LD      x21, 144(sp)
        LD      x22, 152(sp)
        LD      x23, 160(sp)
        LD      x24, 168(sp)
        LD      x25, 176(sp)
        LD      x26, 184(sp)
        LD      x27, 192(sp)
        LD      x28, 200(sp)
        LD      x29, 208(sp)
        LD      x30, 216(sp)
        LD      x31, 224(sp)
        ADDI    sp, sp, 232
        RET

.global push_fp_regs
push_fp_regs:
        /*
         * Push all floating point registers (FLEN=64) and stores fcsr to x27 (s11).
         */
        ADDI    sp, sp, -256
        FSD     f0, 248(sp)
        FSD     f1, 240(sp)
        FSD     f2, 232(sp)
        FSD     f3, 224(sp)
        FSD     f4, 216(sp)
        FSD     f5, 208(sp)
        FSD     f6, 200(sp)
        FSD     f7, 192(sp)
        FSD     f8, 184(sp)
        FSD     f9, 176(sp)
        FSD     f10, 168(sp)
        FSD     f11, 160(sp)
        FSD     f12, 152(sp)
        FSD     f13, 144(sp)
        FSD     f14, 136(sp)
        FSD     f15, 128(sp)
        FSD     f16, 120(sp)
        FSD     f17, 112(sp)
        FSD     f18, 104(sp)
        FSD     f19, 96(sp)
        FSD     f20, 88(sp)
        FSD     f21, 80(sp)
        FSD     f22, 72(sp)
        FSD     f23, 64(sp)
        FSD     f24, 56(sp)
        FSD     f25, 48(sp)
        FSD     f26, 40(sp)
        FSD     f27, 32(sp)
        FSD     f28, 24(sp)
        FSD     f29, 16(sp)
        FSD     f30, 8(sp)
        FSD     f31, 0(sp)
        FRCSR   s11
        RET

.global pop_fp_regs
pop_fp_regs:
        /*
         * Pop all floating point registers (FLEN=64) and restores fcsr from x27 (s11).
         */
        FSCSR   s11
        FLD     f0, 248(sp)
        FLD     f1, 240(sp)
        FLD     f2, 232(sp)
        FLD     f3, 224(sp)
        FLD     f4, 216(sp)
        FLD     f5, 208(sp)
        FLD     f6, 200(sp)
        FLD     f7, 192(sp)
        FLD     f8, 184(sp)
        FLD     f9, 176(sp)
        FLD     f10, 168(sp)
        FLD     f11, 160(sp)
        FLD     f12, 152(sp)
        FLD     f13, 144(sp)
        FLD     f14, 136(sp)
        FLD     f15, 128(sp)
        FLD     f16, 120(sp)
        FLD     f17, 112(sp)
        FLD     f18, 104(sp)
        FLD     f19, 96(sp)
        FLD     f20, 88(sp)
        FLD     f21, 80(sp)
        FLD     f22, 72(sp)
        FLD     f23, 64(sp)
        FLD     f24, 56(sp)
        FLD     f25, 48(sp)
        FLD     f26, 40(sp)
        FLD     f27, 32(sp)
        FLD     f28, 24(sp)
        FLD     f29, 16(sp)
        FLD     f30, 8(sp)
        FLD     f31, 0(sp)
        ADDI    sp, sp, 256
        RET

.global dispatcher_trampoline
dispatcher_trampoline:
        # PUSH all general purpose registers but x10, x11
        # x10 and x11 are pushed by the exit stub
        LD      x12, 0(sp)              # Restore temp jump register
        C.ADDI  sp, -16                 # 24 byte allocated
        SD      ra, 0(sp)
        SD      x10, 16(sp)
        JAL     push_x1_x31
        JAL     push_fp_regs

        ADDI    x12, sp, 480            # param2: *next_addr (TCP)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data

        LD      x18, dispatcher_addr    # Call very far-away function
        JALR    ra, 0(x18)

        JAL     pop_fp_regs
        JAL     pop_x1_x31
        LD      ra, 0(sp)
        LD      x10, 8(sp)              # param0: TCP (next_addr)
        LD      x11, 16(sp)             # param1: SPC (target)
        C.ADDI  sp, 24
        
        J       checked_cc_return

dispatcher_addr: .dword dispatcher

.global disp_thread_data
disp_thread_data: .dword 0

.global syscall_wrapper
.global syscall_wrapper_svc
syscall_wrapper:
        LD      x9, 0(sp)              # Restore temp jump register
        SD      ra, 0(sp)
        JAL     push_x1_x31_full
        JAL     push_fp_regs

        # Call pre syscall handler
        MV      x10, x17                # param0: syscall_no
        ADDI    x11, sp, 312            # param1: *args
        MV      x12, x8	                # param2: *next_inst (x8 set by scanner)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data
        
        LD      x18, syscall_handler_pre_addr   # Call very far-away function
        JALR    ra, 0(x18)

        BEQZ    x10, s_w_r

        # Load syscall parameter registers
        LD      x10, 312(sp)
        LD      x11, 320(sp)
        LD      x12, 328(sp)
        LD      x13, 336(sp)
        LD      x14, 344(sp)
        LD      x15, 352(sp)
        LD      x16, 360(sp)
        # Also load syscall register because it may have changed (debug output)
        LD      x17, 368(sp)

        # Balance the stack on rt_sigreturn, which doesn't return here anymore
        LI      x28, 139
        BNE     x17, x28, svc
        ADDI    sp, sp, (16 + 232 + 256)      # Additional 16 because scanner pushed x1 and x8

svc:
        # Syscall
        ECALL

syscall_wrapper_svc:
        # Call post syscall handler
        ADDI    x11, sp, 312            # param1: *args
        SD      x10, 0(x11)
        MV      x10, x17                # param0: syscall_no
        MV      x12, x8	                # param2: *next_inst (x8 set by scanner)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data

        LD      x18, syscall_handler_post_addr  # Call very far-away function
        JALR    ra, 0(x18)

s_w_r:
        JAL     pop_fp_regs
        JAL     pop_x1_x31_full
        LD      x1, 16(sp)              # Restore x1 pushed by scanner
        LD      x8, 8(sp)               # Restore x8 pushed by scanner
        SD      x10, 16(sp)
        SD      x11, 8(sp)
        LD      x10, 0(sp)              # param0: TCP (x1 set by scanner)
        LD      x11, -192(sp)           # param1: SPC (x8 set by scanner)
        C.ADDI  sp, 8

        J       checked_cc_return

syscall_handler_pre_addr: .dword syscall_handler_pre
syscall_handler_post_addr: .dword syscall_handler_post

.global checked_cc_return
checked_cc_return:
        C.ADDI  sp, -8
        SD      x12, 0(sp)
        LD      x12, th_is_pending_ptr
        LWU     x12, 0(x12)
        BNEZ    x12, deliver_signals_trampoline
        LD      x12, 0(sp)
        C.ADDI  sp, 8
        JR      x10
deliver_signals_trampoline:
        ADDI    sp, sp, -48
        SD      ra, 0(sp)
        SD      x10, 32(sp)
        SD      x11, 40(sp)
        MV      x10, x11                # param0: SPC (target)
        MV      x11, sp	                # param1: self_signal *
        C.ADDI  x11, 8                  # 0(sp) is saved ra, self_signal starts at 8(sp)
        JAL     push_x1_x31
        JAL     push_fp_regs

        LI      x12, 0xd6db             # param2: sigmask TODO: Why should SCP be 0xd6db?
        BEQ     x10, x12, .             # loop if SPC (target) == 0xd6db

        LD      x18, deliver_signals_addr       # Call very far-away function
        JALR    ra, 0(x18)

        JAL     pop_fp_regs
        JAL     pop_x1_x31
        LD      ra, 0(sp)
        C.ADDI  sp, 8

        /*
         * Stack peek:
         *              ┌───────┐
         *        sp -> │ pid   │ ┐
         *              │ tgid  │ ├ self_signal struct
         *              │ signo │ ┘
         *              │ TCP   ├ Translated target PC (next_addr)
         *              │ SPC   ├ Original target PC (target)
         *              │ x12   │
         *              │ x11   ├ (Pushed by exit stub in scanner)
         *              │ x10   ├ (Pushed by exit stub in scanner)
         *              │.......│
         */

        BEQZ    x10, abort_self_signal

        # Syscall
        LD      x10, 0(sp)              # param0: pid
        LD      x11, 8(sp)              # param1: tgid
        LD      x12, 16(sp)             # param2: signo
        ADDI    sp, sp, 16
        SD      x17, 0(sp)

        /*
         * Stack peek:
         *              ┌───────┐
         *        sp -> │ x17   │
         *              │ TCP   ├ Translated target PC (next_addr)
         *              │ SPC   ├ Original target PC (target)
         *              │ x12   │
         *              │ x11   ├ (Pushed by exit stub in scanner)
         *              │ x10   ├ (Pushed by exit stub in scanner)
         *              │.......│
         */
r:
        LI      x17, 131
        ECALL

.global send_self_signal
send_self_signal:
        LD      x17, 0(sp)
        LD      x12, 24(sp)
        LD      x10, 8(sp)              # Load TCP
        ADDI    sp, sp, 32
        JR      x10

abort_self_signal:
        C.ADDI  sp, 24
        LD      x12, 16(sp)
        LD      x10, 0(sp)
        C.ADDI  sp, 24
        JR      x10

deliver_signals_addr: .dword deliver_signals

.global th_is_pending_ptr
th_is_pending_ptr: .dword 0             # uint32 *

.global gp_tp_mambo_ctx
gp_tp_mambo_ctx: .word 0

.global gp_shadow
gp_shadow: .dword 0

.global tp_shadow
tp_shadow: .dword 0

.global end_of_dispatcher_s
end_of_dispatcher_s:
