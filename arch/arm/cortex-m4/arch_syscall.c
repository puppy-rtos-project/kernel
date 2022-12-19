/*
 * Copyright (c) 2022, The Puppy RTOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <puppy.h>
#include <puppy/target.h>

// __attribute__((aligned(4)))
// char _kernel_stack[1024];

/**
 * r0~r3,r12,lr,psr caller regs ; s0~s15
 * r4~r11 called regs ; s16~s31
 * 
 */
static p_ubase_t user_stack_bak;

p_ubase_t get_user_stack(void)
{
    return user_stack_bak;
}

p_ubase_t get_kernel_stack(void)
{
#if 0
    return (p_ubase_t)_kernel_stack + sizeof(_kernel_stack);
#else

    struct _thread_obj *_thread = p_thread_self();
    _thread->kernel_stack = get_user_stack();
    return _thread->kernel_stack;
#endif
}

bool arch_thread_in_kernel(p_obj_t obj)
{
    struct _thread_obj *_thread = obj;
    if(_thread->mode == P_THREAD_MODE_KENL || _thread->kernel_stack)
        return true;
    else
        return false;
}

void backup_user_stack(p_ubase_t stack)
{
    user_stack_bak = stack;
}

void p_svc_exit(void (*entry)(void *parameter), void *param)
{
    printk("p_svc_exit enter...\n");
    while (1);
}

void SVC_Handler(void)
{
    /**
     * r0: syscall number
     * r1-r6: syscall args(r0-r5)
     * r7: syscall api entry
     * r8: svc origin lr
     * r9: sysapi origin lr
     */
    __asm ("mov r8, lr"); /* backup lr */
    __asm ("push {r1-r9}"); /* store args(r0-r5) and r7~r9 */
    /* get syscall_api entry */
    __asm ("bl syscall_get_api");
    __asm ("mov r7, r0");

    /* backup user stack */
    __asm ("mrs r0, psp");
    __asm ("LDR     R9, [R0, #24]"); /* get user lr to r9 */
    __asm ("add r9, #1"); /* todo Odd address, return to tumb mode */
    __asm ("add r0, #36"); /* add 4*8=32 svc automatically pushed to psp ;todo double word alignment */
    __asm ("bl backup_user_stack");

    /* change to kernel stack */
    __asm ("bl get_kernel_stack");
    __asm ("mov r6, r0");
    __asm ("pop {r0-r5}"); /* restore args(r0-r5) */
      /**
     * r0-r5: syscall args
     * r6: kernel stack
     * r7: syscall api entry
     * r8: svc origin lr
     * r9: sysapi origin lr
     */
    __asm ("    STMFD   r6!, {r4-r5}");
    __asm ("push {r0-r3}");

    /* push r12 lr pc psr */
    __asm ("    LDR     r0, =0");
    __asm ("    mov     r1, r9");
    __asm ("    mov     r2, r7");
    __asm ("    LDR     r3, =0x01000000UL");
    __asm ("    STMFD   r6!, {r0-r3}");

    __asm ("pop {r0-r3}");
    /* push r0 r1 r2 r3 */
    __asm ("    STMFD   r6!, {r0-r3}");

    __asm ("    MSR     psp, r6"); /* update stack pointer */
    __asm ("mov lr, r8");  /* revert lr */
    __asm ("pop {r7-r9}"); /* restore r7~r9 */
    __asm ("    ORR     lr, lr, #0x04");
    __asm ("    BX      lr");
}

__attribute__((naked)) void arch_syscall(int sycall_no, ...)
{
    __asm ("push {r4-r7,lr}");
    
    __asm ("push {r0-r3}");
    /* syscall enter, return syscall_no */
    __asm ("bl syscall_enter");
    __asm ("pop {r0-r3}");

    /* r1-r6: syscall args(r0-r5) */
    __asm ("add r7, sp, #20"); /* Remove push regs r4~r7,lr 4*5=20 */
    __asm ("ldmia r7,{r4-r6}");
    
    /**
     * r0: syscall number
     * r1-r6: syscall args(r0-r5)
     */
    __asm ("svc 0");
    
    /* syscall enter, return syscall_no */
    __asm ("bl syscall_leave");
    __asm ("pop {r4, r5}"); /* syscall args(r0-r5) Remove the reserved parameter position */
    __asm ("pop {r4-r7,pc}");
}
