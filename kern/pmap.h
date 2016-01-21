#pragma once
#include <inc/types.h>
#include <inc/memlayout.h>

#define ALLOC_ZERO 1
void mem_init();
uintptr_t mmio_map_region(physaddr_t pa, size_t size);

struct mem_region
{
	struct mem_region *next;
	int refn;
};

int region_insert(pde_t *pgdir, struct mem_region *rg, uintptr_t va, int perm);
void region_remove(pde_t *pgdir, uintptr_t va);
struct mem_region* 
region_lookup(pde_t *pgdir, uintptr_t va, pte_t **pte_store);
void tlb_invalidate(pde_t* pgdir, uintptr_t va);
