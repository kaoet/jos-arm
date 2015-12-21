#include <inc/types.h>
#include <inc/memlayout.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <kern/pmap.h>

// Memory mapping when booting.
// map [0, 16MiB) to [KERNBASE, KERNBASE + 16MiB)
pde_t kern_pgdir[4096] __attribute__((aligned(16 * 1024))) = {
	[0x0] = 0x00000002,
	[0x1] = 0x00100002,
	[0x2] = 0x00200002,
	[0x3] = 0x00300002,
	[0x4] = 0x00400002,
	[0x5] = 0x00500002,
	[0x6] = 0x00600002,
	[0x7] = 0x00700002,
	[0x8] = 0x00800002,
	[0x9] = 0x00900002,
	[0xa] = 0x00a00002,
	[0xb] = 0x00b00002,
	[0xc] = 0x00c00002,
	[0xd] = 0x00d00002,
	[0xe] = 0x00e00002,
	[0xf] = 0x00f00002,
	[0xf00] = 0x00000002,
	[0xf01] = 0x00100002,
	[0xf02] = 0x00200002,
	[0xf03] = 0x00300002,
	[0xf04] = 0x00400002,
	[0xf05] = 0x00500002,
	[0xf06] = 0x00600002,
	[0xf07] = 0x00700002,
	[0xf08] = 0x00800002,
	[0xf09] = 0x00900002,
	[0xf0a] = 0x00a00002,
	[0xf0b] = 0x00b00002,
	[0xf0c] = 0x00c00002,
	[0xf0d] = 0x00d00002,
	[0xf0e] = 0x00e00002,
	[0xf0f] = 0x00f00002,
};

#define TOTAL_PHYS_MEM (256 * 1024 * 1024)
#define MEM_UNIT (16 * 1024)
#define MAX_REGION (TOTAL_PHYS_MEM / MEM_UNIT)
struct mem_region
{
	struct mem_region *next;
	int refn;
} regions[MAX_REGION], *free_regions;

#define PADDR(kva) ((uintptr_t)(kva) - KERNBASE)
#define KADDR(pa) ((uintptr_t)(pa) + KERNBASE)

static inline struct mem_region *pa2region(physaddr_t pa)
{
	return &regions[pa / MEM_UNIT];
}

static inline physaddr_t region2pa(struct mem_region *r)
{
	return MEM_UNIT * (r - regions);
}

void region_init()
{
	extern char end[];
	for (physaddr_t addr = 0; addr < TOTAL_PHYS_MEM; addr += MEM_UNIT) {
		struct mem_region *r = pa2region(addr);
		if (0x100000 <= addr && addr < PADDR(end)) {
			r->refn = 1;
		} else {
			r->refn = 0;
			r->next = free_regions;
			free_regions = r;
		}
	}
}

// allocate a mem_region
struct mem_region *region_alloc(int alloc_flags)
{
	if (free_regions == NULL)
	    return NULL;

	struct mem_region *ret = free_regions;
	free_regions = ret->next;
	ret->next = NULL;
	
	if (alloc_flags & ALLOC_ZERO)
	    memset((void *)KADDR(region2pa(ret)), 0, MEM_UNIT);
	return ret;
}

void region_free(struct mem_region *r)
{
	assert(r->refn == 0);
	assert(r->next == NULL);
	r->next = free_regions;
	free_regions = r;
}

void region_decref(struct mem_region* r)
{
	if (--r->refn == 0)
		region_free(r);
}

void mem_init()
{
	region_init();

	// map physical memory
	for (uintptr_t addr = KERNBASE; addr != 0; addr += PTSIZE) {
		kern_pgdir[PDX(addr)] = PADDR(addr) | 2;
		kern_pgdir[PDX(PADDR(addr))] = 0;
	}
}

pte_t * pgdir_walk(pde_t *pgdir, uintptr_t va, bool create)
{
	pde_t *pde = &pgdir[PDX(va)];
	if (!(*pde & 3)) {
	    if (!create) {
	        return NULL;
	    }
	    struct mem_region *new = region_alloc(ALLOC_ZERO);
	    if (new == NULL) {
	        return NULL;
	    }
	    new->refn++;
	    *pde = region2pa(new) | 1;
	}
	
	pte_t *pgtbl = (pte_t *)KADDR(PDE_ADDR(*pde));
	return &pgtbl[PTX(va)];
}

static void boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa)
{
	assert(va % PGSIZE == 0);
	assert(pa % PGSIZE == 0);
    for (uintptr_t off = 0; off < size; off += PGSIZE) {
        pte_t *pte = pgdir_walk(pgdir, off + va, true);
        if (pte == NULL) {
            panic("boot_map_region out of memory\n");
        }
        *pte = (pa + off) | 2;
    }
}

uintptr_t mmio_map_region(physaddr_t pa, size_t size)
{
	static uintptr_t base = MMIOBASE;
	size = ROUNDUP(size, PGSIZE);
	if (base + size > MMIOLIM)
		panic("mmio_map_region overflow\n");

	boot_map_region(kern_pgdir, base, size, pa);
	uintptr_t old_base = base;
	base += size;
	return old_base;
}