# Save all General-Purpose(GP) registers to context.
# struct context *base = &ctx_task;
# base->ra = ra;
# ......
# These GP registers to be saved don't include gp
# and tp, because they are not caller-saved or
# callee-saved. These two registers are often used
# for special purpose. For example, in RVOS, 'tp'
# (aka "thread pointer") is used to store hartid,
# which is a global value and would not be changed
# during context-switch.
.macro reg_save base
	sw ra, 4(\base)
	sw t0, 16+4(\base)
	sw t1, 20+4(\base)
	sw t2, 24+4(\base)
	sw s0, 28+4(\base)
	sw s1, 32+4(\base)
	sw a0, 36+4(\base)
	sw a1, 40+4(\base)
	sw a2, 44+4(\base)
	sw a3, 48+4(\base)
	sw a4, 52+4(\base)
	sw a5, 56+4(\base)
	sw a6, 60+4(\base)
	sw a7, 64+4(\base)
	sw s2, 68+4(\base)
	sw s3, 72+4(\base)
	sw s4, 76+4(\base)
	sw s5, 80+4(\base)
	sw s6, 84+4(\base)
	sw s7, 88+4(\base)
	sw s8, 92+4(\base)
	sw s9, 96+4(\base)
	sw s10, 100+4(\base)
	sw s11, 104+4(\base)
	sw t3, 108+4(\base)
	sw t4, 112+4(\base)
	sw t5, 116+4(\base)
	sw t6, 120+4(\base)
.endm

# restore all General-Purpose(GP) registers from the context
# except gp & tp.
# struct context *base = &ctx_task;
# ra = base->ra;
# ......
.macro reg_restore base
	lw ra, 4(\base)
	lw t0, 16+4(\base)
	lw t1, 20+4(\base)
	lw t2, 24+4(\base)
	lw s0, 28+4(\base)
	lw s1, 32+4(\base)
	lw a0, 36+4(\base)
	lw a1, 40+4(\base)
	lw a2, 44+4(\base)
	lw a3, 48+4(\base)
	lw a4, 52+4(\base)
	lw a5, 56+4(\base)
	lw a6, 60+4(\base)
	lw a7, 64+4(\base)
	lw s2, 68+4(\base)
	lw s3, 72+4(\base)
	lw s4, 76+4(\base)
	lw s5, 80+4(\base)
	lw s6, 84+4(\base)
	lw s7, 88+4(\base)
	lw s8, 92+4(\base)
	lw s9, 96+4(\base)
	lw s10, 100+4(\base)
	lw s11, 104+4(\base)
	lw t3, 108+4(\base)
	lw t4, 112+4(\base)
	lw t5, 116+4(\base)
	lw t6, 120+4(\base)
.endm

# Something to note about save/restore:
# - We use mscratch to hold a pointer to context of current task
# - We use t6 as the 'base' for reg_save/reg_restore, because it is the
#   very bottom register (x31) and would not be overwritten during loading.
#   Note: CSRs(mscratch) can not be used as 'base' due to load/restore
#   instruction only accept general purpose registers.

.text

# interrupts and exceptions while in machine mode come here.
.globl trap_vector
# the trap vector base address must always be aligned on a 4-byte boundary
.align 4
trap_vector:
	# save context(registers).

    addi sp, sp, -128
	reg_save sp
	csrr a0, mstatus
    andi a0, a0, 8
    bnez a0, save_mpie
    li   a0, 0x80
save_mpie:
    sw a0,   8(sp)
	csrr	a0, mepc
	sw a0,   0(sp)
	csrrw	sp, mscratch, sp	# swap t6 and mscratch
	# call the C trap handler in trap.c
	#csrr	a0, mepc
	#addi    a0, a0, 4

    call arch_irq_enter
	# call the C trap handler in trap.c
	csrr	a0, mepc
	csrr	a1, mcause
	call	trap_handler

	# trap_handler will return the return address via a0.
	csrw	mepc, a0
	call arch_irq_leave

	call arch_need_swap
	beqz a0, exit
	call arch_need_swap_clean

	call arch_get_to_sp
	lw a1, (a0)
	csrrw	a1, mscratch, a1  # swap to thread

    call pup_sched_swap_done_cb

exit:
	csrrw	sp, mscratch, sp	# swap t6 and mscratch
	li    t0, 0x1800

    csrw  mstatus, t0
    lw a0,   8(sp)
    csrs mstatus, a0
	
    lw a0,   0(sp)
	csrw	mepc, a0

	reg_restore sp
	addi sp, sp, 128

	# return to whatever we were doing before trap.
	mret

# void switch_to(struct context *next);
# a0: pointer to the context of the next task
.globl switch_to
.align 4
switch_to:
	csrrw	t6, mscratch, t6	# swap t6 and mscratch
	beqz	t6, 1f			# Note: the first time switch_to() is
	                                # called, mscratch is initialized as zero
					# (in sched_init()), which makes t6 zero,
					# and that's the special case we have to
					# handle with t6
	reg_save t6			# save context of prev task

	# Save the actual t6 register, which we swapped into
	# mscratch
	mv	t5, t6		# t5 points to the context of current task
	csrr	t6, mscratch	# read t6 back from mscratch
	sw	t6, 120(t5)	# save t6 with t5 as base

1:
	# switch mscratch to point to the context of the next task
	csrw	mscratch, a0

	# Restore all GP registers
	# Use t6 to point to the context of the new task
	mv	t6, a0
	reg_restore t6

	# Do actual context switching.
	ret

.end

