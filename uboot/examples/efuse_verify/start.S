.text 

.globl _back_addr
_back_addr:
        .word 0x01a00000
.globl _stack_start
_stack_start:
        .word 0x02000000
.globl _stack_start1
_stack_start1:
        .word 0x02500000

/* Enable the Neon for hard-float */
        /* Set the CPACR for access to CP10 and CP11, and clear the ASEDIS and D32DIS bits */
        ldr r0, =(0xF << 20)
        mcr p15, 0, r0, c1, c0, 2
 
        /* Set the FPEXC EN bit to enable the NEON MPE */
        mov r3, #0x40000000
        VMSR FPEXC, r3

start:
     mrc     p15, 0, r1, c0, c0, 5     @ Read Multiprocessor Affinity Register
     ands    r1, r1, #0x03             @ Mask off, leaving the CPU ID field
     cmp     r1,#0x00                  @ core0 : 0x00 , others 0x01 , 0x02 ...
     bne     run_secondary

     ldr     r8, _back_addr
     str     lr, [r8]
     add     r8, r8,#4
     str     sp, [r8]

	 /* Enable i-cache and D-cache */
	 /* Because we enable the cache, we have to invalidate all first */
	 mrc p15, 0, r1, c1, c0, 0
	 orr r1, r1, #0x1000
	 orr r1, r1, #0x4
	 dsb

	 mcr p15, 0, r0, c7, c6, 1
	 mcr p15, 0, r0, c7, c5, 0
	 mcr p15, 0, r1, c1, c0, 0
	 isb

     ldr     sp, _stack_start
     bic     sp, sp, #7
     bl	     verifymain

     ldr     r8, _back_addr
     add     r9, r8,#4
     ldr     sp, [r9]
     ldr     r1, [r8]
     mov     r0, #0
     bx      r1

run_secondary:
     ldr   sp, _stack_start1
     MSR   CPSR_c, #0xD3 ; #MODE_SVC:OR:DIS_IRQ:OR:DIS_FIRQ
     MOV   r0, r0
     MOV   r0, r0
     MOV   r0, r0
9:
     b     9b
     MOV   r0, r0
     MOV   r0, r0
     MOV   r0, r0
