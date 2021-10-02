/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo

  Copyright 2013-2016 Cosmin Gorgovan <cosmin at linux-geek dot org>
  Copyright 2017 The University of Manchester

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

# These helpers are executed from .text and are not copied to the code cache

#ifdef __arm__
.syntax unified
#endif

#ifdef DBM_ARCH_RISCV64
.global push_gen_regs
push_gen_regs:
  /*
   * Push general purpose registers 
   * x1 to x31 exept for x2 (sp), x1 (ra), x10 and x11
   * NOTE: You may want to also push ra before the subroutine.
   */
  # Move sp first, so that compressed instructions can be used
  # (SD replaced with C.SDSP by assembler)
  ADDI sp, sp, -216
  SD x3, 208(sp)
  SD x4, 200(sp)
  SD x5, 192(sp)
  SD x6, 184(sp)
  SD x7, 176(sp)
  SD x8, 168(sp)
  SD x9, 160(sp)
  SD x12, 152(sp)
  SD x13, 144(sp)
  SD x14, 136(sp)
  SD x15, 128(sp)
  SD x16, 120(sp)
  SD x17, 112(sp)
  SD x18, 104(sp)
  SD x19, 96(sp)
  SD x20, 88(sp)
  SD x21, 80(sp)
  SD x22, 72(sp)
  SD x23, 64(sp)
  SD x24, 56(sp)
  SD x25, 48(sp)
  SD x26, 40(sp)
  SD x27, 32(sp)
  SD x28, 24(sp)
  SD x29, 16(sp)
  SD x30, 8(sp)
  SD x31, 0(sp)
  # Load gp and tp of MAMBO context
  LD gp, gp_shadow_ptr
  LD gp, 0(gp)
  LD tp, tp_shadow_ptr
  LD tp, 0(tp)
  RET

.global pop_gen_regs
pop_gen_regs:
  /*
   * Pop general purpose registers 
   * x1 to x31 exept for x2 (sp), x1 (ra), x10 and x11
   * NOTE: You may want to also pop ra after the subroutine.
   */
  # Store gp and tp of MAMBO context
  LD t0, gp_shadow_ptr
  SD gp, 0(t0)
  LD t0, tp_shadow_ptr
  SD tp, 0(t0)
  LD x3, 208(sp)
  LD x4, 200(sp)
  LD x5, 192(sp)
  LD x6, 184(sp)
  LD x7, 176(sp)
  LD x8, 168(sp)
  LD x9, 160(sp)
  LD x12, 152(sp)
  LD x13, 144(sp)
  LD x14, 136(sp)
  LD x15, 128(sp)
  LD x16, 120(sp)
  LD x17, 112(sp)
  LD x18, 104(sp)
  LD x19, 96(sp)
  LD x20, 88(sp)
  LD x21, 80(sp)
  LD x22, 72(sp)
  LD x23, 64(sp)
  LD x24, 56(sp)
  LD x25, 48(sp)
  LD x26, 40(sp)
  LD x27, 32(sp)
  LD x28, 24(sp)
  LD x29, 16(sp)
  LD x30, 8(sp)
  LD x31, 0(sp)
  ADDI sp, sp, 216
  RET
#endif

.global dbm_client_entry
.func dbm_client_entry
.type dbm_client_entry, %function

#ifdef __arm__
.code 32
dbm_client_entry:
  MOV SP, R1
  MOV LR, R0
  MOV R0, #0
  MOV R1, #0
  MOV R2, #0
  MOV R3, #0
  BLX LR
  BX LR
#endif // __arm__

#ifdef __aarch64__
dbm_client_entry:
  MOV SP, X1
  STP XZR, XZR, [SP, #-16]!
  BR X0
#endif

#ifdef DBM_ARCH_RISCV64
dbm_client_entry:
  MV sp, a1
  LD t0, gp_shadow_ptr
  SD gp, 0(t0)
  LD t0, tp_shadow_ptr
  SD tp, 0(t0)
  ADDI sp, sp, -16      # The scanner inserted code to pop a0 and a1, so push zeros before
  SD zero, 0(sp)
  SD zero, 8(sp)
  JR a0
#endif
.endfunc

# R0 - new SP
.global th_enter
.func   th_enter
.type   th_enter, %function

#ifdef __arm__
.thumb_func
th_enter:
  MOV SP, R0
  STR R1, [SP, #56]
  POP {R0-R12, R14}
  POP {PC}
#endif

#ifdef __aarch64__
th_enter:
  MOV SP, X0
  LDP  X4,  X5, [SP,  #16]
  LDP  X6,  X7, [SP,  #32]
  LDP  X8,  X9, [SP,  #48]
  LDP X10, X11, [SP,  #64]
  LDP X12, X13, [SP,  #80]
  LDP X14, X15, [SP,  #96]
  LDP X16, X17, [SP, #112]
  LDP X18, X19, [SP, #128]
  LDP X20, X21, [SP, #144]
  LDP X22, X23, [SP, #160]
  LDP X24, X25, [SP, #176]
  LDP X26, X27, [SP, #192]
  LDR X28,      [SP, #208]
  LDP X29, X30, [SP, #224]
  LDP  X2,  X3, [SP], #240

  BR X1
#endif

#ifdef DBM_ARCH_RISCV64
th_enter:
  MV sp, a0
  # Load saved parent registers from child stack excluding sp (x2), a0 (x10) and a1 (x11).
  # a0 and a1 are popped by the scanner.
  LD x1, 0(sp)
  LD x3, 16(sp)
  LD x4, 24(sp)
  LD x5, 32(sp)
  LD x6, 40(sp)
  LD x7, 48(sp)
  LD x8, 56(sp)
  LD x9, 64(sp)
  LD x12, 88(sp)
  LD x13, 96(sp)
  LD x14, 104(sp)
  LD x15, 112(sp)
  LD x16, 120(sp)
  LD x17, 128(sp)
  LD x18, 136(sp)
  LD x19, 144(sp)
  LD x20, 152(sp)
  LD x21, 160(sp)
  LD x22, 168(sp)
  LD x23, 176(sp)
  LD x24, 184(sp)
  LD x25, 192(sp)
  LD x26, 200(sp)
  LD x27, 208(sp)
  LD x28, 216(sp)
  LD x29, 224(sp)
  LD x30, 232(sp)
  LD x31, 240(sp)
  ADDI sp, sp, 248
  JR a1
#endif
.endfunc

.global new_thread_trampoline
.func
.type   new_thread_trampoline, %function
new_thread_trampoline:
#ifdef __arm__
  PUSH {R4-R12, LR}
  MOV R1, SP
  B dbm_start_thread_pth
#elif __aarch64__
  STP X19, X20, [SP, #-96]!
  STP X21, X22, [SP, #16]
  STP X23, X24, [SP, #32]
  STP X25, X26, [SP, #48]
  STP X27, X28, [SP, #64]
  STP X29, X30, [SP, #80]
  MOV X1, SP
  B dbm_start_thread_pth
#elif DBM_ARCH_RISCV64
  # Save registers as part of the standard calling convention
  JAL push_gen_regs
  MV a1, sp
  J dbm_start_thread_pth
#endif
.endfunc

.global return_with_sp
.func
.type   return_with_sp, %function
return_with_sp:
#ifdef __arm__
  MOV SP, R0
  POP {R4-R12, PC}
#elif __aarch64__
  MOV SP, X0
  LDP X21, X22, [SP, #16]
  LDP X23, X24, [SP, #32]
  LDP X25, X26, [SP, #48]
  LDP X27, X28, [SP, #64]
  LDP X29, X30, [SP, #80]
  LDP X19, X20, [SP], #96
  RET
#elif DBM_ARCH_RISCV64
  MV sp, a0
  # Restore registers as part of the standard calling convention
  JAL pop_gen_regs
  RET
#endif
.endfunc

.global raw_syscall
.func   raw_syscall
.type   raw_syscall, %function

raw_syscall:
#ifdef __arm__
  MOV R12, SP
  PUSH {R4 - R7}
  MOV R7, R0
  MOV R0, R1
  MOV R1, R2
  MOV R2, R3
  LDM R12, {R3 - R6}
  SVC 0
  POP {R4 - R7}
  BX LR
#endif
#ifdef __aarch64__
  MOV W8, W0
  MOV X0, X1
  MOV X1, X2
  MOV X2, X3
  MOV X3, X4
  MOV X4, X5
  MOV X5, X6
  MOV X6, X7
  SVC 0
  RET
#endif
#ifdef DBM_ARCH_RISCV64
  MV a7, a0     # Set syscall number
  MV a0, a1
  MV a1, a2
  MV a2, a3
  MV a3, a4
  MV a4, a5
  MV a5, a6
  ECALL
  RET
#endif
.endfunc

.global signal_trampoline
.func signal_trampoline
.type signal_trampoline, %function

signal_trampoline:
#ifdef __arm__
  SUB SP, SP, #4
  PUSH {r0-r3, r9, r12, lr}
  BL signal_dispatcher
  CBZ R0, sigret
  STR R0, [SP, #28]
  POP {r0-r3, r9, r12, lr}
  POP {PC}
sigret:
  ADD SP, SP, #32
  MOV R7, #173
  SVC 0
#endif
#ifdef __aarch64__
  STP  X2,  X3, [SP, #-176]!
  STP  X4,  X5, [SP, #16]
  STP  X6,  X7, [SP, #32]
  STP  X8,  X9, [SP, #48]
  STP X10, X11, [SP, #64]
  STP X12, X13, [SP, #80]
  STP X14, X15, [SP, #96]
  STP X16, X17, [SP, #112]
  STP X18, X29, [SP, #128]
  STR X30,      [SP, #144]
  STP  X0,  X1, [SP, #160]

  BL signal_dispatcher

  LDP  X4,  X5, [SP, #16]
  LDP  X6,  X7, [SP, #32]
  LDP  X8,  X9, [SP, #48]
  LDP X10, X11, [SP, #64]
  LDP X12, X13, [SP, #80]
  LDP X14, X15, [SP, #96]
  LDP X16, X17, [SP, #112]
  LDP X18, X29, [SP, #128]
  LDR X30,      [SP, #144]
  LDP  X2,  X3, [SP], #160

  CBZ X0, sigret

  BR X0
sigret:
  ADD SP, SP, #16
  MOV X8, #139
  SVC 0
#endif
#ifdef DBM_ARCH_RISCV64
  C.ADDI sp, -24
  SD ra, 0(sp)
  SD a0, 8(sp)
  SD a1, 16(sp)
  JAL push_gen_regs
  JAL push_fp_regs
  JAL signal_dispatcher
  JAL pop_fp_regs
  JAL pop_gen_regs
  LD ra, 0(sp)
  C.ADDI sp, 8

  BEQZ a0, sigret

  JR a0
sigret:
  C.ADDI sp, 16
  LI a7, 137
  ECALL
#endif
.endfunc

.global atomic_increment_u64
.func atomic_increment_u64
.type atomic_increment_u64, %function

atomic_increment_u64:
#ifdef __arm__
  // R0 - ptr, R2 inc (low), R3, inc (high)
  PUSH {R4, R5}

retry:
  LDREXD R4, R5, [R0]
  ADDS R4, R2
  ADC R5, R3
  STREXD R1, R4, R5, [R0]
  CMP R1, #0
  BNE retry

  MOV R0, R4
  MOV R1, R5
  POP {R4, R5}
  BX LR

#elif __aarch64__
  LDXR X2, [X0]
  ADD X2, X2, X1
  STXR W3, X2, [X0]
  CBNZ W3, atomic_increment_u64
  MOV X0, X2
  RET

#elif DBM_ARCH_RISCV64
  AMOADD.D zero, a1, (a0)
  LD a0, 0(a0)
  RET

#endif
.endfunc

.global atomic_increment_u32
.func atomic_increment_u32
.type atomic_increment_u32, %function

atomic_increment_u32:
#ifdef __arm__
  LDREX R2, [R0]
  ADD R2, R1
  STREX R3, R2, [R0]
  CMP R3, #0
  BNE atomic_increment_u32
  MOV R0, R2
  BX LR

#elif __aarch64__
  LDXR W2, [X0]
  ADD W2, W2, W1
  STXR W3, W2, [X0]
  CBNZ W3, atomic_increment_u32
  MOV W0, W2
  RET

#elif DBM_ARCH_RISCV64
  AMOADD.W zero, a1, (a0)
  LW a0, 0(a0)
  RET

#endif
.endfunc

.global atomic_decrement_if_positive_i32
.func atomic_decrement_if_positive_i32
.type atomic_decrement_if_positive_i32, %function

atomic_decrement_if_positive_i32:
#ifdef __arm__
  LDREX R2, [R0]
  CMP R2, R1
  BLT abort
  SUB R2, R2, R1
  STREX R3, R2, [R0]
  CMP R3, #0
  BNE atomic_decrement_if_positive_i32
  MOV R0, R2
  BX LR
abort:
  CLREX
  MOV R0, #-1
  BX LR

#elif __aarch64__
  LDXR W2, [X0]
  CMP W2, W1
  BLT abort
  SUB W2, W2, W1
  STXR W3, W2, [X0]
  CBNZ W3, atomic_decrement_if_positive_i32
  MOV W0, W2
  RET
abort:
  CLREX
  MOV W0, #-1
  RET

#elif DBM_ARCH_RISCV64
  LR.W.aq t1, (a0)
  BLEZ t1, abort
  SUB t1, t1, a1
  SC.W.rl t2, t1, (a0)
  BNEZ t2, atomic_decrement_if_positive_i32
  MV a0, t1
  RET
abort:
  SC.W zero, t1, (a0)
  LI a0, -1
  RET

#endif
.endfunc

#ifdef DBM_ARCH_RISCV64
push_all_but_params:
  /*
   * Push all registers but the function argument registers and sp and ra (x3-x9, x18-x31).
   */
  # Move sp first, so that compressed instructions can be used
  # (SD replaced with C.SDSP by assembler)
  ADDI    sp, sp, -168
  SD      x3, 0(sp)
  SD      x4, 8(sp)
  SD      x5, 16(sp)
  SD      x6, 24(sp)
  SD      x7, 32(sp)
  SD      x8, 40(sp)
  SD      x9, 48(sp)
  SD      x18, 56(sp)
  SD      x19, 64(sp)
  SD      x20, 72(sp)
  SD      x21, 80(sp)
  SD      x22, 88(sp)
  SD      x23, 96(sp)
  SD      x24, 104(sp)
  SD      x25, 112(sp)
  SD      x26, 120(sp)
  SD      x27, 128(sp)
  SD      x28, 136(sp)
  SD      x29, 144(sp)
  SD      x30, 152(sp)
  SD      x31, 160(sp)
  # Load gp of MAMBO context
  LD gp, gp_shadow_ptr
  LD gp, 0(gp)
  LD tp, tp_shadow_ptr
  LD tp, 0(tp)
  RET

pop_all_but_params:
  /*
   * Pop all registers but the function argument registers and sp and ra (x3-x9, x18-x31).
   */
  # Store gp of MAMBO context
  LD t0, gp_shadow_ptr
  SD gp, 0(t0)
  LD t0, tp_shadow_ptr
  SD tp, 0(t0)
  LD      x3, 0(sp)
  LD      x4, 8(sp)
  LD      x5, 16(sp)
  LD      x6, 24(sp)
  LD      x7, 32(sp)
  LD      x8, 40(sp)
  LD      x9, 48(sp)
  LD      x18, 56(sp)
  LD      x19, 64(sp)
  LD      x20, 72(sp)
  LD      x21, 80(sp)
  LD      x22, 88(sp)
  LD      x23, 96(sp)
  LD      x24, 104(sp)
  LD      x25, 112(sp)
  LD      x26, 120(sp)
  LD      x27, 128(sp)
  LD      x28, 136(sp)
  LD      x29, 144(sp)
  LD      x30, 152(sp)
  LD      x31, 160(sp)
  ADDI    sp, sp, 168
  RET
#endif

.global safe_fcall_trampoline
.func safe_fcall_trampoline
.type safe_fcall_trampoline, %function

safe_fcall_trampoline:
#ifdef __arm__
  PUSH {R5-R7, R9, R12, LR}
  VPUSH {d16-d31}
  VPUSH {d0-d7}

  MOV R7, SP
  BIC R6, R7, #7
  MOV SP, R6

  MRS R5, CPSR
  VMRS R6, FPSCR

  BLX R4

  MOV SP, R7

  MSR CPSR, R5
  VMSR FPSCR, R6

  VPOP {d0-d7}
  VPOP {d16-d31}
  POP {R5-R7, R9, R12, PC}

#elif __aarch64__
  STP X8,  X9,  [SP, #-128]!
  STP X10, X11, [SP, #16]
  STP X12, X13, [SP, #32]
  STP X14, X15, [SP, #48]
  STP X16, X17, [SP, #64]
  STP X18, X19, [SP, #80]
  STP X20, X21, [SP, #96]
  STP X29, X30, [SP, #112]

  MRS X19, NZCV
  MRS X20, FPCR
  MRS X21, FPSR

  BL push_neon

  BLR X8

  BL pop_neon

  MSR NZCV, X19
  MSR FPCR, X20
  MSR FPSR, X21

  LDP X10, X11, [SP, #16]
  LDP X12, X13, [SP, #32]
  LDP X14, X15, [SP, #48]
  LDP X16, X17, [SP, #64]
  LDP X18, X19, [SP, #80]
  LDP X20, X21, [SP, #96]
  LDP X29, X30, [SP, #112]
  LDP X8,  X9,  [SP], #128

  RET

#elif DBM_ARCH_RISCV64
  C.ADDI sp, -8
  SD ra, 0(sp)
  JAL push_all_but_params
  JAL push_fp_regs
  JALR x18          # function pointer
  JAL pop_fp_regs
  JAL pop_all_but_params
  LD ra, 0(sp)
  C.ADDI sp, 8
  RET
#endif

.endfunc

.global __try_memcpy_error
.type __try_memcpy_error, %function
.global __try_memcpy
.type __try_memcpy, %function

__try_memcpy:
#ifdef __arm__
  LDRB R3, [R1], #1
  STRB R3, [R0], #1
  SUB R2, #1
  CBZ R2, __try_memcpy_ret
  B __try_memcpy
__try_memcpy_ret:
  MOV R0, #0
  BX LR

__try_memcpy_error:
  MOV R0, #-1
  BX LR
#elif __aarch64__
  LDRB W3, [X1], #1
  STRB W3, [X0], #1
  SUB X2, X2, #1
  CBNZ X2, __try_memcpy
  MOV X0, #0
  RET

__try_memcpy_error:
  MOV X0, #-1
  RET
#elif DBM_ARCH_RISCV64
  LB t1, 0(a1)
  SB t1, 0(a0)
  C.ADDI a0, 1
  C.ADDI a1, 1
  ADDI a2, a2, -1
  BNEZ a2, __try_memcpy_error
  LI a0, 0
  RET

__try_memcpy_error:
  LI a0, -1
  RET
#endif
