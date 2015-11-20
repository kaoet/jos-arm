#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>

static inline void wdacr(uint32_t value) {
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(value));
}

static inline void wttbr0(uint32_t value) {
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(value));
}

static inline void wttbr1(uint32_t value) {
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r"(value));
}

static inline void wttbcr(uint32_t value) {
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r"(value));
}

static inline void wcr(uint32_t value) {
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(value));
}

static inline uint32_t rcr() {
	uint32_t value;
	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(value));
	return value;
}

uint32_t ttb[4096] __attribute__((aligned(16 * 1024)));

void main()
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