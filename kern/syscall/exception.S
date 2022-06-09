.macro INTERRUPT_HANDLER irq
.if \irq
	sub		lr, lr, #4
.endif

@ store user context on stack
	sub		sp, sp, #60
	stm		sp, {r0-r14}^
	sub		sp, sp, #8              
	mrs		r4, SPSR
	str		r4, [sp, #4]
	str		lr, [sp, #0]
	
	mov		r0, sp 
	bl		trap		@ save user context to its trapframe

.if \irq
	bl		getIrqStatus  @ r0 now holds IRQ status
.else
	ldr		lr, [sp, #0]	@ restore lr
	ldr		r0, [lr, #-4] @ r0 now holds SWI code
.endif

	add		sp, sp, #68		@ restore stack

.if \irq
	@ switch to svc mode
	mrs		r4, CPSR
	add		r4, r4, #1
	msr		CPSR, r4
.endif

	bl		enterKernel

	@ restore kernel context
	ldm		sp, {r4-r11, lr}
	add		sp, sp, #36
	bx		lr
.endm


	.text
	.align 2
	.global handleIRQ
	.type handleIRQ, %function
handleIRQ:
	INTERRUPT_HANDLER 1


	.text
	.align 2
	.global handleSWI
	.type handleSWI, %function
handleSWI:
	INTERRUPT_HANDLER 0
	

	.text
	.align 2
	.global userMode
	.type userMode, %function
userMode:
	sub		sp, sp, #36
	stm		sp, {r4-r11, lr}

	ldr		r4, [r0, #4]
	ldr		lr, [r0, #0]
	add		r0, r0, #8
	msr		SPSR_cxsf, r4
	ldm		r0, {r0-r14}^

	movs	pc, lr
