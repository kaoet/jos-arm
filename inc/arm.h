#pragma once

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