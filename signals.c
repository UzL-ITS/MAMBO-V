/*
  This file is part of MAMBO, a low-overhead dynamic binary modification tool:
      https://github.com/beehive-lab/mambo

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

#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <string.h>

#include "dbm.h"
#include "scanner_common.h"
#ifdef __arm__
#include "pie/pie-thumb-encoder.h"
#include "pie/pie-thumb-decoder.h"
#include "pie/pie-thumb-field-decoder.h"
#include "pie/pie-arm-encoder.h"
#include "pie/pie-arm-decoder.h"
#include "pie/pie-arm-field-decoder.h"
#endif
#ifdef __aarch64__
#include "pie/pie-a64-encoder.h"
#include "pie/pie-a64-decoder.h"
#include "pie/pie-a64-field-decoder.h"
#endif
#ifdef DBM_ARCH_RISCV64
#include "pie/pie-riscv-encoder.h"
#include "pie/pie-riscv-decoder.h"
#include "pie/pie-riscv-field-decoder.h"
#endif

#ifdef DEBUG
  #define debug(...) log("signals", __VA_ARGS__)
#else
  #define debug(...)
#endif

#define self_send_signal_offset        ((uintptr_t)send_self_signal - (uintptr_t)&start_of_dispatcher_s)
#define syscall_wrapper_svc_offset     ((uintptr_t)syscall_wrapper_svc - (uintptr_t)&start_of_dispatcher_s)

#define SIGNAL_TRAP_IB (0x94)
#define SIGNAL_TRAP_DB (0x95)
#ifdef DBM_ARCH_RISCV64
  #define RISCV_SRET_CODE 0x10200073
  #define RISCV_MRET_CODE 0x30200073
#endif

#ifdef __arm__
  #define pc_field uc_mcontext.arm_pc
  #define sp_field uc_mcontext.arm_sp
#elif __aarch64__
  #define pc_field uc_mcontext.pc
  #define sp_field uc_mcontext.sp
#elif DBM_ARCH_RISCV64
  #define pc_field uc_mcontext.__gregs[REG_PC]
  #define sp_field uc_mcontext.__gregs[REG_SP]
#endif

typedef struct {
  uintptr_t pid;
  uintptr_t tid;
  uintptr_t signo;
} self_signal;

void install_system_sig_handlers() {
  struct sigaction act;
  act.sa_sigaction = signal_trampoline;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  int ret = sigaction(UNLINK_SIGNAL, &act, NULL);
  assert(ret == 0);
}

int deliver_signals(uintptr_t spc, self_signal *s) {
  uint64_t sigmask;

  if (global_data.exit_group) {
    thread_abort(current_thread);
  }

  int ret = syscall(__NR_rt_sigprocmask, 0, NULL, &sigmask, sizeof(sigmask));
  assert (ret == 0);

  for (int i = 0; i < _NSIG; i++) {
    if ((sigmask & (1 << i)) == 0
        && atomic_decrement_if_positive_i32(&current_thread->pending_signals[i], 1) >= 0) {
      s->pid = syscall(__NR_getpid);
      s->tid = syscall(__NR_gettid);
      s->signo = i;
      atomic_increment_u32(&current_thread->is_signal_pending, -1);
      return 1;
    }
  }

  return 0;
}

typedef int (*inst_decoder)(void *);
#ifdef __arm__
  #define inst_size(inst, is_thumb) (((is_thumb) && ((inst) < THUMB_ADC32)) ? 2 : 4)
  #define write_trap(code)  if (is_thumb) { \
                              thumb_udf16((uint16_t **)&write_p, (code)); \
                              write_p += 2; \
                            } else { \
                              arm_udf((uint32_t **)&write_p, (code) >> 4, (code) & 0xF); \
                              write_p += 4; \
                            }
  #define TRAP_INST_TYPE ((is_thumb) ? THUMB_UDF16 : ARM_UDF)
#elif __aarch64__d
  #define inst_size(inst, is_thumb) (4)
  #define write_trap(code) a64_HVC((uint32_t **)&write_p, (code)); write_p += 4;
  #define TRAP_INST_TYPE (A64_HVC)
#elif DBM_ARCH_RISCV64
  #define inst_size(inst, is_thumb) (inst <= 39 ? INST_16BIT : INST_32BIT)
  #define write_trap(code) if (code == SIGNAL_TRAP_IB) { \
                             riscv_sret((uint16_t **)&write_p); /* SRET for indirect branch */ \
                           } else { \
                             riscv_mret((uint16_t **)&write_p); /* MRET for direct branch */ \
                           } \
                           write_p += INST_32BIT
  #define TRAP_INST_TYPE (RISCV_INVALID) // The decoder does not know the SRET or MRET instructions
#endif

#ifdef DBM_ARCH_RISCV64
static void riscv_sret (uint16_t **address)
{
	uint32_t inst = RISCV_SRET_CODE;
	*(*address + 1) = (uint16_t)(inst >> 16);
	*(*address) = (uint16_t)(inst & 0xffff);
}

static void riscv_mret (uint16_t **address)
{
	uint32_t inst = RISCV_MRET_CODE;
	*(*address + 1) = (uint16_t)(inst >> 16);
	*(*address) = (uint16_t)(inst & 0xffff);
}
#endif

/**
 * Search for the indirect basic block exit and replace it with a trap to cause 
 * UNLINK_SIGNAL.
 * @param bb_meta Basic-Block meta data. (used by arm only)
 * @param o_write_p Starting point of search for exit instruction.
 */
bool unlink_indirect_branch(dbm_code_cache_meta *bb_meta, void **o_write_p) {
  int br_inst_type, trap_inst_type;
  inst_decoder decoder;
  void *write_p = *o_write_p;
  bool is_thumb = false;
#ifdef __arm__
  if (bb_meta->exit_branch_type == uncond_reg_thumb) {
    is_thumb = true;
    br_inst_type = THUMB_BX16;
    decoder = (inst_decoder)thumb_decode;
  } else if (bb_meta->exit_branch_type == uncond_reg_arm) {
    br_inst_type = ARM_BX;
    decoder = (inst_decoder)arm_decode;
  }
#elif __aarch64__
  br_inst_type = A64_BR;
  decoder = (inst_decoder)a64_decode;
#elif DBM_ARCH_RISCV64
  decoder = (inst_decoder)riscv_decode;
#endif
  trap_inst_type = TRAP_INST_TYPE;

  // Search for br_inst_type instruction
  int inst = decoder(write_p);
#ifndef DBM_ARCH_RISCV64
  while(inst != br_inst_type && inst != trap_inst_type) {
#else
  while(inst != RISCV_C_JR && inst != RISCV_JALR && inst != trap_inst_type) {
#endif
    write_p += inst_size(inst, is_thumb);
    inst = decoder(write_p);
  }

  if (inst == trap_inst_type) {
    return false;
  }

  // Write trap to cause UNLINK_SIGNAL
  write_trap(SIGNAL_TRAP_IB);
  *o_write_p = write_p;
  return true;
}

/**
 * Get exit trap size.
 * @param bb_meta Basic-Block meta data.
 * @param fragment_id Code cache index of basic block.
 */
int get_direct_branch_exit_trap_sz(dbm_code_cache_meta *bb_meta, int fragment_id) {
  int sz;
  switch(bb_meta->exit_branch_type) {
#ifdef __arm__
    case cond_imm_thumb:
    case cbz_thumb:
      sz = (bb_meta->branch_cache_status & BOTH_LINKED) ? 10 : 6;
      break;
    case cond_imm_arm:
      sz = (bb_meta->branch_cache_status & BOTH_LINKED) ? 8 : 4;
      break;
#elif __aarch64__
    case uncond_imm_a64:
      sz = 4;
      break;
    case cond_imm_a64:
    case cbz_a64:
    case tbz_a64:
      if (fragment_id >= CODE_CACHE_SIZE) {
        // a single branch is inserted for a conditional exit in a trace
        // however a second branch may follow for an early exit to an existing trace
        sz = 8;
      } else {
        sz = (bb_meta->branch_cache_status & BOTH_LINKED) ? 12 : 8;
      }
      break;
#elif DBM_ARCH_RISCV64
    case uncond_imm_riscv:
      sz = 4;
      break;
    case cond_imm_riscv:
      // TODO: [traces] 8 for trace fragment
      sz = (bb_meta->branch_cache_status & BOTH_LINKED) ? 12 : 8;
      break;
#endif
    default:
      while(1);
  }
  return sz;
}

/**
 * Rewrite basic block exit code with traps for direct branches.
 * @param bb_meta Basic-Block meta data.
 * @param o_write_p Trap write location.
 * @param fragment_id Code cache index of basic block.
 * @param pc Start address of current translated basic block (TPC).
 */
bool unlink_direct_branch(dbm_code_cache_meta *bb_meta, void **o_write_p, int fragment_id, uintptr_t pc) {
  int offset = 0;
  bool is_thumb = false;
  void *write_p = *o_write_p;

  offset = get_direct_branch_exit_trap_sz(bb_meta, fragment_id);

  if (pc < ((uintptr_t)bb_meta->exit_branch_addr + offset)) {
    if (bb_meta->branch_cache_status != 0) {
      inst_decoder decoder;

#ifdef __arm__
      is_thumb = (bb_meta->exit_branch_type == cond_imm_thumb) || (bb_meta->exit_branch_type == cbz_thumb);
      if (is_thumb) {
        decoder = (inst_decoder)thumb_decode;
      } else {
        decoder = (inst_decoder)arm_decode;
      }
#elif __aarch64__
      decoder = (inst_decoder)a64_decode;
#elif DBM_ARCH_RISCV64
      decoder = (inst_decoder)riscv_decode;
#endif
      int inst = decoder(write_p);
      if (inst == TRAP_INST_TYPE) {
        return false;
      }
      memcpy(&bb_meta->saved_exit, write_p, offset);
      for (int i = 0; i < offset; i += inst_size(TRAP_INST_TYPE, is_thumb)) {
        write_trap(SIGNAL_TRAP_DB);
      }
    } // if (bb_meta->branch_cache_status != 0)
  } else {
    /* It's already setting up a call to the dispatcher. Ensure that the
       fragment is not supposed to be linked */
    assert((bb_meta->branch_cache_status & BOTH_LINKED) == 0);
    return false;
  }

  *o_write_p = write_p;
  return true;
}

/**
 * Unlink fragment with given ID.
 * @param fragment_id Code cache index of basic block.
 * @param pc Start address of current translated basic block (TPC).
 */
void unlink_fragment(int fragment_id, uintptr_t pc) {
  dbm_code_cache_meta *bb_meta;

#ifdef DBM_TRACES
  // Skip over trace fragments with elided unconditional branches
  branch_type type;

  do {
    bb_meta = &current_thread->code_cache_meta[fragment_id];
    type = bb_meta->exit_branch_type;
    fragment_id++;
  }
  #ifdef __arm__
  while ((type == uncond_imm_arm || type == uncond_imm_thumb ||
          type == uncond_blxi_thumb || type == uncond_blxi_arm) &&
  #elif __aarch64__
  while (type == uncond_imm_a64 &&
  #elif DBM_ARCH_RISCV64
  // TODO: [traces] while(type == uncond_imm_riscv &&
  #endif
         (bb_meta->branch_cache_status & BOTH_LINKED) == 0 &&
         fragment_id >= CODE_CACHE_SIZE &&
         fragment_id < current_thread->active_trace.id);

  fragment_id--;
  // If the fragment isn't installed, make sure it's active
  if (fragment_id >= current_thread->trace_id) {
    assert(current_thread->active_trace.active);
  }
#else
  bb_meta = &current_thread->code_cache_meta[fragment_id];
#endif // DBM_TRACES

#ifdef __aarch64__
  // we don't try to unlink trace exits, we unlink the fragment they jump to
  if (bb_meta->exit_branch_type == trace_exit) {
    fragment_id = addr_to_fragment_id(current_thread, bb_meta->branch_taken_addr);
    bb_meta = &current_thread->code_cache_meta[fragment_id];
    pc = bb_meta->tpc;
  }
#elif DBM_ARCH_RISCV64
  // TODO: [traces] Handle trace exit
#endif

  void *write_p = bb_meta->exit_branch_addr;
  void *start_addr = write_p;

#ifdef __arm__
  if (bb_meta->exit_branch_type == uncond_reg_thumb ||
      bb_meta->exit_branch_type == uncond_reg_arm) {
#elif __aarch64__
  if (bb_meta->exit_branch_type == uncond_branch_reg) {
#elif DBM_ARCH_RISCV64
  if (bb_meta->exit_branch_type == uncond_reg_riscv) {
#endif
    if (!unlink_indirect_branch(bb_meta, &write_p)) {
      return;
    }
  } else if (bb_meta->branch_cache_status != 0) {
    if (!unlink_direct_branch(bb_meta, &write_p, fragment_id, pc)) {
      return;
    }
  }

  __clear_cache(start_addr, write_p);
}

/**
 * Pop registers from stack at `send_self_signal` and set PC to SPC.
 * @param cont Signal context containing register values.
 */
void translate_delayed_signal_frame(ucontext_t *cont) {
  uintptr_t *sp = (uintptr_t *)cont->sp_field;
#ifdef __arm__
  /*
         r7
         r1
         r2
         PID
         TID
         SIGNO
         R0
         TPC
         SPC
  */
  cont->uc_mcontext.arm_r7 = sp[0];
  cont->uc_mcontext.arm_r1 = sp[1];
  cont->uc_mcontext.arm_r2 = sp[2];
  cont->uc_mcontext.arm_r0 = sp[6];
  cont->uc_mcontext.arm_pc = sp[8];

  sp += 9;
#elif __aarch64__
  /*
    TPC, SPC
    X2, X8
    X0, X1
  */
  cont->uc_mcontext.regs[x8] = sp[3];
  cont->uc_mcontext.regs[x2] = sp[2];
  cont->uc_mcontext.pc = sp[1];
  cont->uc_mcontext.regs[x0] = sp[4];
  cont->uc_mcontext.regs[x1] = sp[5];
  sp += 6;
#elif DBM_ARCH_RISCV64
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
  cont->context_reg(17) = sp[0];
  cont->context_reg(12) = sp[3];
  cont->context_reg(11) = sp[4];
  cont->context_reg(10) = sp[5];
  cont->pc_field = sp[2];
  sp += 6;
#endif

  cont->sp_field = (uintptr_t)sp;
}

void translate_svc_frame(ucontext_t *cont) {
  uintptr_t *sp = (uintptr_t *)cont->sp_field;
#ifdef __arm__
  cont->uc_mcontext.arm_r8  = sp[8];
  cont->uc_mcontext.arm_r9  = sp[9];
  cont->uc_mcontext.arm_r10 = sp[10];
  cont->uc_mcontext.arm_fp  = sp[11];
  cont->uc_mcontext.arm_ip  = sp[12];
  cont->uc_mcontext.arm_lr  = sp[13];
  cont->uc_mcontext.arm_pc  = sp[15];
  sp += 16;
#elif __aarch64__
  #define FPSIMD_SIZE  (0x210)

  assert(cont->uc_mcontext.regs[x8] != __NR_rt_sigreturn);
  struct fpsimd_context *fpstate = (struct fpsimd_context *)&cont->uc_mcontext.__reserved;

  // Set up the FP state first
  assert(fpstate->head.magic == FPSIMD_MAGIC && fpstate->head.size == FPSIMD_SIZE);
  memcpy(fpstate->vregs, sp, sizeof(fpstate->vregs));
  fpstate->fpsr = cont->uc_mcontext.regs[x21];
  fpstate->fpcr = cont->uc_mcontext.regs[x20];
  sp += 512 / sizeof(sp[0]);

  // Now set the general purpose registers  & PSTATE
  cont->uc_mcontext.pstate = cont->uc_mcontext.regs[x19];
  for (int r = 9; r <= 21; r++) {
    cont->uc_mcontext.regs[r] = sp[r];
  }
  cont->uc_mcontext.pc = sp[23];
  cont->uc_mcontext.regs[x29] = sp[24];
  cont->uc_mcontext.regs[x30] = sp[25];
  sp += 26;
#elif DBM_ARCH_RISCV64
  sp += 256 / sizeof(sp[0]);
  for (int x = 5; x <= 7; x++) {
    cont->context_reg(x) = sp[x-5];
  }
  for (int x = 12; x <= 15; x++) { // restore x28 - x31
    cont->context_reg(x) = sp[x];
  }
  cont->pc_field = sp[3]; // Get SPC from x8
  cont->context_reg(x8) = sp[16];
  cont->context_reg(x1) = sp[17];
  sp += 32;
  debug("context restored sp: 0x%lx\n", sp);
  debug("context pc: 0x%lx\n", cont->pc_field);
#endif
  cont->sp_field = (uintptr_t)sp;
}

#if defined(__arm__) || defined(__aarch64__)
#define PSTATE_N (1 << 31)
#define PSTATE_Z (1 << 30)
#define PSTATE_C (1 << 29)
#define PSTATE_V (1 << 28)
bool interpret_condition(uint32_t pstate, mambo_cond cond) {
  assert(cond >= 0 && cond <= 0xF);
  bool state = true;
  switch (cond >> 1) {
    case 0:
      state = pstate & PSTATE_Z;
      break;
    case 1:
      state = pstate & PSTATE_C;
      break;
    case 2:
      state = pstate & PSTATE_N;
      break;
    case 3:
      state = pstate & PSTATE_V;
      break;
    case 4:
      state = (pstate & PSTATE_C) && ((pstate & PSTATE_Z) == 0);
      break;
    case 5:
      state = ((pstate & PSTATE_N) ? true : false) == ((pstate & PSTATE_V) ? true : false);
      break;
    case 6:
      state = ((pstate & PSTATE_N) ? true : false) == ((pstate & PSTATE_V) ? true : false);
      state = state && ((pstate & PSTATE_Z) == 0);
      break;
    case 7:
      state = true;
      break;
  }

  state = state ? true : false;

  if (cond < 14 && (cond & 1)) {
    state = !state;
  }

  return state;
}
#elif DBM_ARCH_RISCV64
/**
 * Test if condition in conditional branch instruction is true or false.
 * @param context Userlevel context containing register values. @see ucontext_t
 * @param write_p Location of the linked conditional branch instruction.
 * @return Whether conditional branch is would be taken or not.
 */
bool interpret_condition(ucontext_t *context, uintptr_t write_p)
{
  mambo_cond cond;
  riscv_get_mambo_cond(riscv_decode((uint16_t *)write_p), (uint16_t *)write_p, &cond, NULL);

  // Load values to compare
  int v1 = (cond.r1 == x0) ? 0 : context->uc_mcontext.__gregs[cond.r1];
  int v2 = (cond.r2 == x0) ? 0 : context->uc_mcontext.__gregs[cond.r2];

  // Check if condition is true
  switch (cond.cond) {
  case EQ:
    return (v1 == v2);
    break;
  case NE:
    return (v1 != v2);
    break;
  case LT:
    return (v1 < v2);
    break;
  case GE:
    return (v1 >= v2);
    break;
  case LTU:
    return ((unsigned int)v1 < (unsigned int)v2);
    break;
  case GEU:
    return ((unsigned int)v1 >= (unsigned int)v2);
    break;
  
  default:
    return true;
  }
}
#endif

#ifdef __aarch64__
bool interpret_cbz(ucontext_t *cont, dbm_code_cache_meta *bb_meta) {
  int reg = (bb_meta->rn) & 0x1F;
  uint64_t val = cont->uc_mcontext.regs[reg];
  if (bb_meta->rn & (1 << 5)) {
    val &= 0xFFFFFFFF;
  }

  return (val == 0) ^ (bb_meta->branch_condition);
}

bool interpret_tbz(ucontext_t *cont, dbm_code_cache_meta *bb_meta) {
  int reg = (bb_meta->rn) & 0x1F;
  int bit = (bb_meta->rn) >> 5;
  bool is_taken = (cont->uc_mcontext.regs[reg] & (1 << bit)) == 0;

  return is_taken ^ bb_meta->branch_condition;
}
#endif

#ifdef __arm__
  #define direct_branch(write_p, target, cond)  if (is_thumb) { \
                                                  thumb_b32_helper((write_p), (target)) \
                                                } else { \
                                                  arm_b32_helper((write_p), (target), cond) \
                                                }
#elif __aarch64__
  #define direct_branch(write_p, target, cond)  a64_b_helper((write_p), (target) + 4)
#elif DBM_ARCH_RISCV64
  #define direct_branch(write_p, target, cond) riscv_branch_imm_helper(write_p, target, false)
#endif

/**
 * Restore saved basic block exit.
 * @param thread_data Thread data.
 * @param fragment_id Code cache index of basic block.
 * @param o_write_p Start location of the basic block exit.
 */
#ifdef __arm__
void restore_exit(dbm_thread *thread_data, int fragment_id, void **o_write_p, bool is_thumb) {
#elif defined(__aarch64__) || defined(DBM_ARCH_RISCV64)
void restore_exit(dbm_thread *thread_data, int fragment_id, void **o_write_p) {
#endif
  void *write_p = *o_write_p;
  dbm_code_cache_meta *bb_meta = &thread_data->code_cache_meta[fragment_id];

  int restore_sz = get_direct_branch_exit_trap_sz(bb_meta, fragment_id);
  memcpy(write_p, &bb_meta->saved_exit, restore_sz);
  write_p += restore_sz;

  *o_write_p = write_p;
}

/**
 * Restore scratch registers.
 * @param cont Signal context containing register values.
 */
void restore_ihl_regs(ucontext_t *cont) {
  uintptr_t *sp = (uintptr_t *)cont->sp_field;

#ifdef __arm__
  cont->context_reg(5) = sp[0];
  cont->context_reg(6) = sp[1];
#elif __aarch64__
  cont->context_reg(0) = sp[0];
  cont->context_reg(1) = sp[1];
#elif DBM_ARCH_RISCV64
  cont->context_reg(10) = sp[0];
  cont->context_reg(11) = sp[1];
#endif
  sp += 2;

  cont->sp_field = (uintptr_t)sp;
}

/**
 * Call dispatcher routine.
 * @param thread_data Thread data.
 * @param cont Signal context.
 * @param target Target address.
 */
void sigret_dispatcher_call(dbm_thread *thread_data, ucontext_t *cont, uintptr_t target) {
  uintptr_t *sp = (uintptr_t *)cont->context_sp;

#if defined(__arm__) || defined(__aarch64__)
#ifdef __arm__
  sp -= DISP_SP_OFFSET / 4;
#elif __aarch64__
  sp -= 2;
#endif
  sp[0] = cont->context_reg(0);
  sp[1] = cont->context_reg(1);
#ifdef __arm__
  sp[2] = cont->context_reg(2);
  sp[3] = cont->context_reg(3);
#endif
// Set dispatcher parameters (target, source_index)
  cont->context_reg(0) = target;
  cont->context_reg(1) = 0;
  cont->context_pc = thread_data->dispatcher_addr;
#ifdef __arm__
  cont->context_reg(3) = cont->context_sp;
  cont->uc_mcontext.arm_cpsr &= ~CPSR_T;
#endif
#elif DBM_ARCH_RISCV64
  // Save current registers a0 and a1
  sp -= 2;
  sp[0] = cont->context_reg(10);
  sp[1] = cont->context_reg(11);
  // Set dispatcher parameters (target, source_index)
  cont->context_reg(10) = target;
  cont->context_reg(11) = 0;
  // Call dispatcher on signal return
  cont->context_pc = thread_data->dispatcher_addr;
#endif

  // Update stack pointer
  cont->context_sp = (uintptr_t)sp;
}

#ifdef __arm__
  #define restore_ihl_inst(addr)  if (is_thumb) { \
                                    thumb_bx16((uint16_t **)&addr, r6); \
                                    __clear_cache((void *)addr, (void *)addr + 2); \
                                  } else { \
                                    arm_bx((uint32_t **)&addr, r6); \
                                    __clear_cache((void *)addr, (void *)addr + 4); \
                                  }

#elif __aarch64__
  #define restore_ihl_inst(addr) a64_BR((uint32_t **)&addr, x0); \
                                __clear_cache((void *)addr, (void *)addr + 4);

#elif DBM_ARCH_RISCV64
  #define restore_ihl_inst(addr) riscv_jalr((uint16_t **)&addr, x0, x10, 0); \
                                 __clear_cache((void *)addr, (void *)addr + INST_32BIT)
#endif

/* If type == indirect && pc >= exit, read the pc and deliver the signal */
/* If pc < <type specific>, unlink the fragment and resume execution */
uintptr_t signal_dispatcher(int i, siginfo_t *info, void *context) {
  uintptr_t handler = 0;
  bool deliver_now = false;

  assert(i >= 0 && i < _NSIG);
  ucontext_t *cont = (ucontext_t *)context;

  uintptr_t pc = (uintptr_t)cont->pc_field;
  uintptr_t cc_start = (uintptr_t)&current_thread->code_cache->blocks[trampolines_size_bbs];
  uintptr_t cc_end = cc_start + MAX_BRANCH_RANGE;

  debug("Signal trap at %p: 0x%x\n", (uint32_t *)pc, *(uint32_t *)pc);

  if (global_data.exit_group > 0) {
    if (pc >= cc_start && pc < cc_end) {
      int fragment_id = addr_to_fragment_id(current_thread, (uintptr_t)pc);
      dbm_code_cache_meta *bb_meta = &current_thread->code_cache_meta[fragment_id];
      if (pc >= (uintptr_t)bb_meta->exit_branch_addr) {
        thread_abort(current_thread);
      }
      unlink_fragment(fragment_id, pc);
    }
    atomic_increment_u32(&current_thread->is_signal_pending, 1);
    return 0;
  }

  if (pc == ((uintptr_t)current_thread->code_cache + self_send_signal_offset)) {
    translate_delayed_signal_frame(cont);
    deliver_now = true;
  } else if (pc == ((uintptr_t)current_thread->code_cache + syscall_wrapper_svc_offset)) {
    translate_svc_frame(cont);
    deliver_now = true;
  }

  if (deliver_now) {
    handler = lookup_or_scan(current_thread, global_data.signal_handlers[i], NULL);
    return handler;
  }

  if (pc >= cc_start && pc < cc_end) {
    int fragment_id = addr_to_fragment_id(current_thread, (uintptr_t)pc);
    dbm_code_cache_meta *bb_meta = &current_thread->code_cache_meta[fragment_id];

    if (pc >= (uintptr_t)bb_meta->exit_branch_addr) {
      void *write_p;

      if (i == UNLINK_SIGNAL) {
        uint32_t imm;
#ifdef __arm__
        bool is_thumb = cont->uc_mcontext.arm_cpsr & CPSR_T;
        if (is_thumb) {
          thumb_udf16_decode_fields((uint16_t *)pc, &imm);
        } else {
          uint32_t imm12, imm4;
          arm_udf_decode_fields((uint32_t *)pc, &imm12, &imm4);
          imm = (imm12 << 4) | imm4;
        }
#elif __aarch64__
        a64_HVC_decode_fields((uint32_t *)pc, &imm);
#elif DBM_ARCH_RISCV64
        if (*(uint32_t *)pc == RISCV_SRET_CODE)
          imm = SIGNAL_TRAP_IB;
        else if (*(uint32_t *)pc == RISCV_MRET_CODE)
          imm = SIGNAL_TRAP_DB;
        else
          imm = 0;
#endif
        if (imm == SIGNAL_TRAP_IB) {
          restore_ihl_inst(pc);

          int rn = current_thread->code_cache_meta[fragment_id].rn;
          uintptr_t target;
#ifdef __arm__
          unsigned long *regs = &cont->uc_mcontext.arm_r0;
          target = regs[rn];
#elif __aarch64__
          target = cont->uc_mcontext.regs[rn];
#elif DBM_ARCH_RISCV64
          target = cont->uc_mcontext.__gregs[rn];
#endif
          restore_ihl_regs(cont);
          sigret_dispatcher_call(current_thread, cont, target);
          return 0;
        } else if (imm == SIGNAL_TRAP_DB) {
          write_p = bb_meta->exit_branch_addr;
          void *start_addr = write_p;
#ifdef __arm__
          restore_exit(current_thread, fragment_id, &write_p, is_thumb);
#elif defined(__aarch64__) || defined(DBM_ARCH_RISCV64)
          restore_exit(current_thread, fragment_id, &write_p);
#endif
          __clear_cache(start_addr, write_p);

          bool is_taken;
          switch(bb_meta->exit_branch_type) {
#ifdef __arm__
            case cond_imm_thumb:
            case cond_imm_arm:
              is_taken = interpret_condition(cont->uc_mcontext.arm_cpsr, bb_meta->branch_condition);
              break;
            case cbz_thumb: {
              unsigned long *regs = &cont->uc_mcontext.arm_r0;
              is_taken = regs[bb_meta->rn] == 0;
              break;
            }
#elif __aarch64__
            case uncond_imm_a64:
              is_taken = true;
              break;
            case cond_imm_a64:
              is_taken = interpret_condition(cont->uc_mcontext.pstate, bb_meta->branch_condition);
              break;
            case cbz_a64:
              is_taken = interpret_cbz(cont, bb_meta);
              break;
            case tbz_a64:
              is_taken = interpret_tbz(cont, bb_meta);
              break;
#elif DBM_ARCH_RISCV64
            case uncond_imm_riscv:
            case uncond_reg_riscv:
              is_taken = true;
              break;
            case cond_imm_riscv:
              is_taken = interpret_condition(cont, (uintptr_t)write_p);
              break;
#endif
            default:
              fprintf(stderr, "Signal: interpreting of %d fragments not implemented\n", bb_meta->exit_branch_type);
              while(1);
          }

          // Set up *sigreturn* to the dispatcher
          sigret_dispatcher_call(current_thread, cont,
                                 is_taken ? bb_meta->branch_taken_addr : bb_meta->branch_skipped_addr);
          return 0;
        } else {
          fprintf(stderr, "Error: unknown MAMBO trap code\n");
          while(1);
        }
      } // i == UNLINK_SIGNAL
    } // if (pc >= (uintptr_t)bb_meta->exit_branch_addr)
    unlink_fragment(fragment_id, pc);
  }

  /* Call the handlers of synchronous signals immediately
     The SPC of the instruction is unknown, so sigreturning to addresses derived
     from the PC value in the signal frame is not supported.
     We mangle the PC in the context to hopefully trap such attempts.
  */
  if (i == SIGSEGV || i == SIGBUS || i == SIGFPE || i == SIGTRAP || i == SIGILL || i == SIGSYS) {
    handler = global_data.signal_handlers[i];

    if (pc < cc_start || pc >= cc_end) {
      fprintf(stderr, "Synchronous signal (%d) outside the code cache\n", i);
      while(1);
    }

    // Check if the application actually has a handler installed for the signal used by MAMBO
    if (handler == (uintptr_t)SIG_IGN || handler == (uintptr_t)SIG_DFL) {
      assert(i == UNLINK_SIGNAL);

      // Remove this handler
      struct sigaction act;
      act.sa_sigaction = (void *)handler;
      sigemptyset(&act.sa_mask);
      int ret = sigaction(i, &act, NULL);
      assert(ret == 0);

      // sigreturn so the same signal is raised again without an installed signal handler
      return 0;
    }

    cont->pc_field = 0;
    handler = lookup_or_scan(current_thread, handler, NULL);
    return handler;
  }

  atomic_increment_int(&current_thread->pending_signals[i], 1);
  atomic_increment_u32(&current_thread->is_signal_pending, 1);

  return handler;
}
