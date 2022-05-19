	.text
	.align 2
	.global handleSWI
	.type handleSWI, %function
handleSWI:
	sub		sp, sp, #60
	stm		sp, {r0-r14}^
	sub		sp, sp, #8              
	mrs		r4, SPSR
	str		r4, [sp, #4]
	str		lr, [sp, #0]

	mov		r0, sp 
  ldr		r1, [lr, #-4]
	add		sp, sp, #68
  bl		enterKernel

	.text
	.align 2
	.global userMode
	.type userMode, %function
userMode:
	ldr		r4, [r0, #4]
	ldr		lr, [r0, #0]
	add		r0, r0, #8
	msr		SPSR_cxsf, r4
	ldm		r0, {r0-r14}^

	movs	pc, lr