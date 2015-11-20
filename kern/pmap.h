#pragma once
#include <inc/types.h>

#define ALLOC_ZERO 1
void mem_init();
uintptr_t mmio_map_region(physaddr_t pa, size_t size);