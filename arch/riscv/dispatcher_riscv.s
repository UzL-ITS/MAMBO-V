.global start_of_dispatcher_s
start_of_dispatcher_s:

gp_tp_context_switch:
        /* Switch register values of gp and tp with shadow values
         * param a0: If set, gp and tp are set to mambo context
         */
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

.global push_volatile
push_volatile:
        /*
         * Push volatile registers: x5-x7, x12-x18, x28-x31
         * (x10 and x11 are expected to be pushed by the translation before)
         */
        # Move sp first, so that compressed instructions can be used
        # (SD replaced with C.SDSP by assembler)
        ADDI    sp, sp, -112
        SD      x5, 104(sp)
        SD      x6, 96(sp)
        SD      x7, 88(sp)
        SD      x12, 80(sp)
        SD      x13, 72(sp)
        SD      x14, 64(sp)
        SD      x15, 56(sp)
        SD      x16, 48(sp)
        SD      x17, 40(sp)
        SD      x28, 32(sp)
        SD      x29, 24(sp)
        SD      x30, 16(sp)
        SD      x31, 8(sp)
        # Load gp and tp of MAMBO context and store the client gp and tp
        MV      t3, ra
        SD      a0, 0(sp)
        LI      a0, 1
        JAL     gp_tp_context_switch
        LD      a0, 0(sp)
        MV      ra, t3
        LD      t3, 32(sp)
        ADDI    sp, sp, 8
        RET

.global pop_volatile
pop_volatile:
        /*
         * Pop volatile registers: x5-x7, x12-x18, x28-x31
         * (x10 and x11 are expected to be popped by the translation later)
         */
        # Store gp and tp of MAMBO context and load the client gp and tp
        MV t3, ra
        ADDI sp, sp, -8
        SD a0, 0(sp)
        MV a0, zero
        JAL gp_tp_context_switch
        LD a0, 0(sp)
        MV ra, t3
        LD      x5, 104(sp)
        LD      x6, 96(sp)
        LD      x7, 88(sp)
        LD      x12, 80(sp)
        LD      x13, 72(sp)
        LD      x14, 64(sp)
        LD      x15, 56(sp)
        LD      x16, 48(sp)
        LD      x17, 40(sp)
        LD      x28, 32(sp)
        LD      x29, 24(sp)
        LD      x30, 16(sp)
        LD      x31, 8(sp)
        ADDI    sp, sp, 112
        RET

push_volatile_syscall:
        /*
         * Push volatile registers: x5-x7, x10-x18, x28-x31 and x8 (non-volatile)
         * WARNING: Must be paired with `pop_volatile_syscall`!
         * NOTE: This routine is specifically for use in `syscall_wrapper`, therefore
         *      x8 is also pushed.
         */
        # Move sp first, so that compressed instructions can be used
        # (SD replaced with C.SDSP by assembler)
        ADDI    sp, sp, -136
        SD      x5, 0(sp)
        SD      x6, 8(sp)
        SD      x7, 16(sp)
        SD      x8, 24(sp)
        SD      x10, 32(sp)
        SD      x11, 40(sp)
        SD      x12, 48(sp)
        SD      x13, 56(sp)
        SD      x14, 64(sp)
        SD      x15, 72(sp)
        SD      x16, 80(sp)
        SD      x17, 88(sp)
        SD      x18, 96(sp)
        SD      x28, 104(sp)
        SD      x29, 112(sp)
        SD      x30, 120(sp)
        SD      x31, 128(sp)
        # Load gp and tp of MAMBO context and store the client gp and tp
        MV      t3, ra
        LI      a0, 1
        JAL     gp_tp_context_switch
        LD      a0, 32(sp)
        MV      ra, t3
        LD      t3, 104(sp)
        RET

pop_volatile_syscall:
        /*
         * Pop volatile registers: x5-x7, x10-x18, x28-x31 and x8 (non-volatile)
         * WARNING: Must be paired with `push_volatile_syscall`!
         * NOTE: This routine is specifically for use in `syscall_wrapper`, therefore
         *      x8 is also popped.
         */
        # Store gp and tp of MAMBO context and load the client gp and tp
        MV t3, ra
        MV a0, zero
        JAL gp_tp_context_switch
        MV ra, t3
        LD      x5, 0(sp)
        LD      x6, 8(sp)
        LD      x7, 16(sp)
        LD      x8, 24(sp)
        LD      x10, 32(sp)
        LD      x11, 40(sp)
        LD      x12, 48(sp)
        LD      x13, 56(sp)
        LD      x14, 64(sp)
        LD      x15, 72(sp)
        LD      x16, 80(sp)
        LD      x17, 88(sp)
        LD      x18, 96(sp)
        LD      x28, 104(sp)
        LD      x29, 112(sp)
        LD      x30, 120(sp)
        LD      x31, 128(sp)
        ADDI    sp, sp, 136
        RET

.global push_fp_volatile
push_fp_volatile:
        /*
         * Push volatile floating point registers (FLEN=64) and stores fcsr to x27 (s11).
         */
        ADDI    sp, sp, -104
        FSD     f8, 96(sp)
        FSD     f9, 88(sp)
        FSD     f18, 80(sp)
        FSD     f19, 72(sp)
        FSD     f20, 64(sp)
        FSD     f21, 56(sp)
        FSD     f22, 48(sp)
        FSD     f23, 40(sp)
        FSD     f24, 32(sp)
        FSD     f25, 24(sp)
        FSD     f26, 16(sp)
        FSD     f27, 8(sp)
        SD      s11, 0(sp)
        FRCSR   s11
        RET

.global pop_fp_volatile
pop_fp_volatile:
        /*
         * Pop volatile floating point registers (FLEN=64) and restores fcsr from x27 (s11).
         */
        FSCSR   s11
        LD      s11, 0(sp)
        FLD     f8, 96(sp)
        FLD     f9, 88(sp)
        FLD     f18, 80(sp)
        FLD     f19, 72(sp)
        FLD     f20, 64(sp)
        FLD     f21, 56(sp)
        FLD     f22, 48(sp)
        FLD     f23, 40(sp)
        FLD     f24, 32(sp)
        FLD     f25, 24(sp)
        FLD     f26, 16(sp)
        FLD     f27, 8(sp)
        ADDI    sp, sp, 104
        RET

.global dispatcher_trampoline
dispatcher_trampoline:
        # PUSH all general purpose registers but x10, x11
        # x10 and x11 are pushed by the exit stub
        LD      x12, 0(sp)              # Restore temp jump register
        C.ADDI  sp, -16                 # 24 byte allocated
        SD      ra, 0(sp)
        SD      x10, 16(sp)
        JAL     push_volatile
        JAL     push_fp_volatile

        ADDI    x12, sp, 216            # param2: *next_addr (TCP)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data

        LD      x14, dispatcher_addr    # Call very far-away function
        JALR    ra, 0(x14)

        JAL     pop_fp_volatile
        JAL     pop_volatile
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
        JAL     push_volatile_syscall
        JAL     push_fp_volatile

        # Call pre syscall handler
        MV      x10, x17                # param0: syscall_no
        ADDI    x11, sp, 136            # param1: *args
        MV      x12, x8	                # param2: *next_inst (x8 set by scanner)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data
        
        LD      x18, syscall_handler_pre_addr   # Call very far-away function
        JALR    ra, 0(x18)

        BEQZ    x10, s_w_r

        # Load syscall parameter registers
        LD      x10, 136(sp)
        LD      x11, 144(sp)
        LD      x12, 152(sp)
        LD      x13, 160(sp)
        LD      x14, 168(sp)
        LD      x15, 176(sp)
        LD      x16, 184(sp)
        # Also load syscall register because it may have changed (debug output)
        LD      x17, 192(sp)

        # Balance the stack on rt_sigreturn, which doesn't return here anymore
        LI      x28, 139
        BNE     x17, x28, svc
        ADDI    sp, sp, (16 + 136 + 104)      # Additional 16 because scanner pushed x1 and x8

svc:
        # Syscall
        ECALL

syscall_wrapper_svc:
        # Call post syscall handler
        ADDI    x11, sp, 136            # param1: *args
        SD      x10, 0(x11)
        MV      x10, x17                # param0: syscall_no
        MV      x12, x8	                # param2: *next_inst (x8 set by scanner)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data

        LD      x18, syscall_handler_post_addr  # Call very far-away function
        JALR    ra, 0(x18)

s_w_r:
        JAL     pop_fp_volatile
        JAL     pop_volatile_syscall
        LD      x1, 16(sp)              # Restore x1 pushed by scanner
        LD      x8, 8(sp)               # Restore x8 pushed by scanner
        SD      x10, 16(sp)
        SD      x11, 8(sp)
        LD      x10, 0(sp)              # param0: TCP (x1 set by scanner)
        LD      x11, -96(sp)            # param1: SPC (x8 set by scanner)
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
        JAL     push_volatile
        JAL     push_fp_volatile

        LI      x12, 0xd6db             # param2: sigmask TODO: Why should SCP be 0xd6db?
        BEQ     x10, x12, .             # loop if SPC (target) == 0xd6db

        LD      x18, deliver_signals_addr       # Call very far-away function
        JALR    ra, 0(x18)

        JAL     pop_fp_volatile
        JAL     pop_volatile
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
