#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>
#include <inc/arm.h>

uint32_t ttb[4096] __attribute__((aligned(16 * 1024)));

void kern_init()
{
	// Setup identity mapping
	for (uint32_t i = 0; i < 4096; i++) {
		uintptr_t va = i * 1024 * 1024;
		uintptr_t pa = va;
		ttb[i] = pa | 2;
	}
	wdacr(0xFFFFFFFFU);
	wttbr0((uint32_t)&ttb);
	wttbr1((uint32_t)&ttb);
	wttbcr(0);
	wcr(rcr() | 1);

	cprintf("Test printf: %d=0x%x\n", 32, 32);
	cprintf("Another line\n");
}

void raise() {for(;;);}