.global start_of_dispatcher_s
start_of_dispatcher_s:

.global push_x1_x31
push_x1_x31:
        /*
         * Push general purpose registers 
         * x1 to x31 exept for x2 (sp), x1 (ra), x10 and x11
         * NOTE: You may want to also push ra before the subroutine.
         */
        # Move sp first, so that compressed instructions can be used
        # (SD replaced with C.SDSP by assembler)
        ADDI    sp, sp, -216
        SD      x3, 208(sp)
        SD      x4, 200(sp)
        SD      x5, 192(sp)
        SD      x6, 184(sp)
        SD      x7, 176(sp)
        SD      x8, 168(sp)
        SD      x9, 160(sp)
        SD      x12, 152(sp)
        SD      x13, 144(sp)
        SD      x14, 136(sp)
        SD      x15, 128(sp)
        SD      x16, 120(sp)
        SD      x17, 112(sp)
        SD      x18, 104(sp)
        SD      x19, 96(sp)
        SD      x20, 88(sp)
        SD      x21, 80(sp)
        SD      x22, 72(sp)
        SD      x23, 64(sp)
        SD      x24, 56(sp)
        SD      x25, 48(sp)
        SD      x26, 40(sp)
        SD      x27, 32(sp)
        SD      x28, 24(sp)
        SD      x29, 16(sp)
        SD      x30, 8(sp)
        SD      x31, 0(sp)
        RET

.global pop_x1_x31
pop_x1_x31:
        /*
         * Pop general purpose registers 
         * x1 to x31 exept for x2 (sp), x1 (ra), x10 and x11
         * NOTE: You may want to also pop ra after the subroutine.
         */
        LD      x3, 208(sp)
        LD      x4, 200(sp)
        LD      x5, 192(sp)
        LD      x6, 184(sp)
        LD      x7, 176(sp)
        LD      x8, 168(sp)
        LD      x9, 160(sp)
        LD      x12, 152(sp)
        LD      x13, 144(sp)
        LD      x14, 136(sp)
        LD      x15, 128(sp)
        LD      x16, 120(sp)
        LD      x17, 112(sp)
        LD      x18, 104(sp)
        LD      x19, 96(sp)
        LD      x20, 88(sp)
        LD      x21, 80(sp)
        LD      x22, 72(sp)
        LD      x23, 64(sp)
        LD      x24, 56(sp)
        LD      x25, 48(sp)
        LD      x26, 40(sp)
        LD      x27, 32(sp)
        LD      x28, 24(sp)
        LD      x29, 16(sp)
        LD      x30, 8(sp)
        LD      x31, 0(sp)
        ADDI    sp, sp, 216
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
        RET

pop_x1_x31_full:
        /*
         * Pop general purpose registers. x1 to x31 restored apart from x2 (sp) and 
         * x1 (ra) in ascending order.
         * WARNING: Must be paired with `push_x1_x31_full` not `push_x1_x31`!
         * NOTE: You may want to also pop ra after the subroutine.
         */
        LD      x3, 0(sp)
        LD      x4, 8(sp)
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

.global dispatcher_trampoline
dispatcher_trampoline:
        # PUSH all general purpose registers but x10, x11
        # x10 and x11 are pushed by the exit stub
        C.ADDI  sp, -24
        SD      ra, 0(sp)
        SD      x10, 16(sp)
        JAL     push_x1_x31

        ADDI    x12, sp, 224            # param2: *next_addr (TCP)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data

        LD      x18, dispatcher_addr    # Call very far-away function
        JALR    ra, 0(x18)

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
        JAL     push_x1_x31_full

        # Call pre syscall handler
        MV      x10, x17                # param0: syscall_no
        ADDI    x11, sp, 56             # param1: *args
        MV      x12, x8	                # param2: *next_inst (x8 set by scanner)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data
        
        LD      x18, syscall_handler_pre_addr   # Call very far-away function
        JALR    ra, 0(x18)

        BEQZ    x10, s_w_r

        # Load syscall parameter registers
        LD      x10, 56(sp)
        LD      x11, 64(sp)
        LD      x12, 72(sp)
        LD      x13, 80(sp)
        LD      x14, 88(sp)
        LD      x15, 96(sp)
        LD      x16, 104(sp)

        # Balance the stack on rt_sigreturn, which doesn't return here anymore
        LI      x28, 139
        BNE     x17, x28, svc
        ADDI    sp, sp, (16 + 232)      # Additional 16 because scanner pushed x1 and x8

svc:
        # Syscall
        ECALL

syscall_wrapper_svc:
        # Call post syscall handler
        ADDI    x11, sp, 56             # param1: *args
        SD      x10, 0(x11)
        MV      x10, x17                # param0: syscall_no
        MV      x12, x8	                # param2: *next_inst (x8 set by scanner)
        LD      x13, disp_thread_data   # param3: dbm_thread *thread_data

        LD      x18, syscall_handler_post_addr  # Call very far-away function
        JALR    ra, 0(x18)

s_w_r:
        JAL     pop_x1_x31_full
        LD      x8, 0(sp)               # Restore x8 pushed by scanner
        LD      x1, 8(sp)               # Restore x1 pushed by scanner
        SD      x10, 8(sp)
        SD      x11, 0(sp)
        LD      x10, -232(sp)           # param0: TCP (x1 set by scanner)
        LD      x11, -192(sp)           # param1: SPC (x8 set by scanner)

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

        LI      x12, 0xd6db             # param2: sigmask TODO: Why should SCP be 0xd6db?
        BEQ     x10, x12, .             # loop if SPC (target) == 0xd6db

        LD      x18, deliver_signals_addr       # Call very far-away function
        JALR    ra, 0(x18)

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

.global end_of_dispatcher_s
end_of_dispatcher_s:
