#include <inc/memlayout.h>

.global _start
_start:
	ldr r0, =0xFFFFFFFF
	ldr r1, =(kern_pgdir - KERNBASE)
	mov r2, #0
	mcr p15, 0, r0, c3, c0, 0 // set domain access
	mcr p15, 0, r1, c2, c0, 0 // ttb r0
	mcr p15, 0, r1, c2, c0, 1 // ttb r1
	mcr p15, 0, r2, c2, c0, 2 // ttb cr

	mrc p15, 0, r0, c1, c0, 0 // read control register
	orr r0, r0, #1 // turn on mmu
	mcr p15, 0, r0, c1, c0, 0 // write control register

	ldr lr, =high_addr
	bx lr

high_addr:
	ldr sp, =KSTACKTOP
	bl kern_init
spin:
	b spin
