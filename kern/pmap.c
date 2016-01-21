#include <inc/types.h>
#include <inc/memlayout.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/error.h>
#include <kern/pmap.h>

// Memory mapping when booting.
// map [0, 16MiB) to [KERNBASE, KERNBASE + 16MiB)
#define PADDR(kva) ((uintptr_t)(kva) - KERNBASE)
#define KADDR(pa) ((uintptr_t)(pa) + KERNBASE)

extern uint8_t bootstack[];
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
#ifdef VERSATILE_PB
    [MCONSOLE >> 20] = 0x10100002,
#else
	[MCONSOLE >> 20] = 0x20200002,
#endif
    [(KSTACKTOP - 1) >> 20] = PADDR(bootstack) + 2,
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
struct mem_region regions[MAX_REGION], *free_regions;

static void check_free_regions();
static void check_region_alloc(void);
static void check_kern_pgdir(void);
static physaddr_t check_va2pa(pde_t *pgdir, uintptr_t va);
static void check_region(void);
static void check_region_installed_pgdir(void);

static inline struct mem_region *pa2region(physaddr_t pa)
{
	return &regions[pa / MEM_UNIT];
}

static inline physaddr_t region2pa(struct mem_region *r)
{
	return MEM_UNIT * (r - regions);
}

static inline uintptr_t region2kva(struct mem_region *r) {
    return region2pa(r) + KERNBASE;
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

static void set_domain(int did, int priv) {
    int clear_bit = ~(11 << (2 * did));
    int new_priv = priv << (2 * did);
    asm("mrc p15, 0, r0, c3, c0, 0\n"
        "and r0, r0, %0\n"
        "orr r0, r0, %1\n"
        "mcr p15, 0, r0, c3, c0, 0\n" 
        : 
        : "r"(clear_bit), "r"(new_priv)
        : "r0");
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
		kern_pgdir[PDX(addr)] = PADDR(addr) | PDE_ENTRY_1M | PDE_NONE_U;
		kern_pgdir[PDX(PADDR(addr))] = 0;
	}
	
	// update console permission
	kern_pgdir[PDX(MCONSOLE)] |= PDE_ENTRY_1M | PDE_NONE_U;
	
	// map kernel stack
	kern_pgdir[PDX(KSTACKTOP - 1)] = PADDR(bootstack) | PDE_ENTRY_1M | PDE_NONE_U;
	set_domain(0, DOMAIN_CLIENT);
	
	check_free_regions();
	check_region_alloc();
	check_region();
	check_kern_pgdir();
	check_region_installed_pgdir();
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
	    *pde = region2pa(new) | PDE_ENTRY;
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
        *pte = (pa + off) | PTE_ENTRY_SMALL | PTE_NONE_U;
    }
}

uintptr_t mmio_map_region(physaddr_t pa, size_t size)
{
	static uintptr_t base = MMIOBASE;
	size = ROUNDUP(size, PGSIZE);
	if (base + size > MMIOLIM)
		return -E_NO_MEM;

	boot_map_region(kern_pgdir, base, size, pa);
	uintptr_t old_base = base;
	base += size;
	return old_base;
}

struct mem_region* 
region_lookup(pde_t *pgdir, uintptr_t va, pte_t **pte_store)
{
	pte_t* ppte = pgdir_walk(pgdir, va, 0);
	if (NULL == ppte) {
		return NULL;
	}
	if (!(*ppte & PTE_P)) {
	    return NULL;
	}
	struct mem_region* rg = pa2region(*ppte);
	if (pte_store) {
		*pte_store = ppte;
	}
	return rg;
}

int
region_insert(pde_t *pgdir, struct mem_region *rg, uintptr_t va, int perm)
{
	// Fill this function in
	pte_t* ppte = pgdir_walk(pgdir, va, 1);
	if (NULL == ppte) {
		return -E_NO_MEM;
	}
	rg->refn ++;
	if (*ppte & PTE_P) {
		region_remove(pgdir, va);
	}
	*ppte = region2pa(rg) | PTE_ENTRY_SMALL | perm;
	
	tlb_invalidate(pgdir, va);
	
	return 0;
}

void
region_remove(pde_t *pgdir, uintptr_t va)
{
	// Fill this function in
	pte_t* ppte;
	struct mem_region* rg = region_lookup(pgdir, va, &ppte);
	if (NULL == rg) {
		return;
	}
	region_decref(rg);
	if(ppte) {
		*ppte = 0;
	}
	tlb_invalidate(pgdir, va);
}

static physaddr_t
check_va2pa(pde_t *pgdir, uintptr_t va)
{
	pte_t *p;

	pgdir = &pgdir[PDX(va)];
	if (!(*pgdir & PDE_P))
		return ~0;
	
	if ((*pgdir & PDE_ENTRY_1M) == PDE_ENTRY_1M) {
		return (physaddr_t) (((*pgdir) & 0xFFF00000) + (va & 0xFFFFF));
	} else if ((*pgdir & PDE_ENTRY_16M) == PDE_ENTRY_16M){
	    return (physaddr_t) (((*pgdir) & 0xFF000000) + (va & 0xFFFFFF));
	} else {
		p = (pte_t*) KADDR(PDE_ADDR(*pgdir));
		if (!(p[PTX(va)] & PTE_P))
			return ~0;
		pte_t pte = p[PTX(va)];
		if ((pte & PTE_ENTRY_SMALL) == PTE_ENTRY_SMALL) {
		    return PTE_SMALL_ADDR(p[PTX(va)]) + (va & 0xFFF);
		} else {
		    return PTE_LARGE_ADDR(p[PTX(va)]) + (va & 0xFFFF);
		}
	}
	panic("unreachable area.\n");
	return ~0;
}


static void
check_free_regions()
{
    struct mem_region *rg;
    int count = 0;
	assert( NULL != free_regions);
    
    for (rg = free_regions; rg; rg = rg->next) {
        assert(rg->refn == 0);
        count ++;
    }
    cprintf("check_free_regions() succeeded!\n");
}

static void
check_region_alloc(void)
{
	struct mem_region *pp, *pp0, *pp1, *pp2;
	int nfree;
	struct mem_region *fl;
	char *c;
	int i;

	// check number of free regions
	for (pp = free_regions, nfree = 0; pp; pp = pp->next)
		++nfree;

	// should be able to allocate three regions
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = region_alloc(0)));
	assert((pp1 = region_alloc(0)));
	assert((pp2 = region_alloc(0)));

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(region2pa(pp0) < MAX_REGION*MEM_UNIT);
	assert(region2pa(pp1) < MAX_REGION*MEM_UNIT);
	assert(region2pa(pp2) < MAX_REGION*MEM_UNIT);

	// temporarily steal the rest of the free regions
	fl = free_regions;
    free_regions = 0;

	// should be no free memory
	assert(!region_alloc(0));

	// free and re-allocate?
	region_free(pp0);
	region_free(pp1);
	region_free(pp2);
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = region_alloc(0)));
	assert((pp1 = region_alloc(0)));
	assert((pp2 = region_alloc(0)));
	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(!region_alloc(0));
	
	// test flags
	memset((void*)region2kva(pp0), 1, PGSIZE);
	region_free(pp0);
	assert((pp = region_alloc(ALLOC_ZERO)));
	assert(pp && pp0 == pp);
	c = (char*)region2kva(pp);
	for (i = 0; i < PGSIZE; i++)
		assert(c[i] == 0);

	// give free list back
	free_regions = fl;

	// free the regions we took
	region_free(pp0);
	region_free(pp1);
	region_free(pp2);

	// number of free regions should be the same
	for (pp = free_regions; pp; pp = pp->next)
		--nfree;
	assert(nfree == 0);
	
	cprintf("check_region_alloc() succeeded!\n");
}

// check region_insert, region_remove, &c
static void
check_region(void)
{
	struct mem_region *pp, *pp0, *pp1, *pp2;
	struct mem_region *fl;
	pte_t *ptep, *ptep1;
	uintptr_t va;
	uintptr_t mm1, mm2;
	int i;

	// should be able to allocate three regions
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = region_alloc(0)));
	assert((pp1 = region_alloc(0)));
	assert((pp2 = region_alloc(0)));

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free regions
	fl = free_regions;
	free_regions = 0;

	// should be no free memory
	assert(!region_alloc(0));

	// there is no page allocated at address 0
	assert(region_lookup(kern_pgdir, 0x0, &ptep) == NULL);

	// there is no free memory, so we can't allocate a page table
	assert(region_insert(kern_pgdir, pp1, 0x0, PTE_NONE_U) < 0);

	// free pp0 and try again: pp0 should be used for page table
	region_free(pp0);
	assert(region_insert(kern_pgdir, pp1, 0x0, PTE_NONE_U) == 0);
	assert(PTE_SMALL_ADDR(kern_pgdir[0]) == region2pa(pp0));
	assert(check_va2pa(kern_pgdir, 0x0) == region2pa(pp1));
	assert(pp1->refn == 1);
	assert(pp0->refn == 1);

	// should be able to map pp2 at PGSIZE because pp0 is already allocated for page table
	assert(region_insert(kern_pgdir, pp2,  PGSIZE, PTE_NONE_U) == 0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == region2pa(pp2));
	assert(pp2->refn == 1);

	// should be no free memory
	assert(!region_alloc(0));

	// should be able to map pp2 at PGSIZE because it's already there
	assert(region_insert(kern_pgdir, pp2,  PGSIZE, PTE_NONE_U) == 0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == region2pa(pp2));
	assert(pp2->refn == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in region_insert
	assert(!region_alloc(0));

	// check that pgdir_walk returns a pointer to the pte
	ptep = (pte_t *) KADDR(PTE_SMALL_ADDR(kern_pgdir[PDX(PGSIZE)]));
	assert(pgdir_walk(kern_pgdir, PGSIZE, 0) == ptep+PTX(PGSIZE));

	// should be able to change permissions too.
	assert(region_insert(kern_pgdir, pp2,  PGSIZE, PTE_RW_U) == 0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == region2pa(pp2));
	assert(pp2->refn == 1);
	assert((*pgdir_walk(kern_pgdir,  PGSIZE, 0) & PTE_RW_U) == PTE_RW_U);

	// should be able to remap with fewer permissions
	assert(region_insert(kern_pgdir, pp2,  PGSIZE, PTE_NONE_U) == 0);
	assert(*pgdir_walk(kern_pgdir,  PGSIZE, 0) & PTE_NONE_U);
	assert((*pgdir_walk(kern_pgdir,  PGSIZE, 0) & PTE_RW_U) != PTE_RW_U);

	// should not be able to map at PTSIZE because need free page for page table
	assert(region_insert(kern_pgdir, pp0,  PTSIZE, PTE_NONE_U) < 0);

	// insert pp1 at PGSIZE (replacing pp2)
	assert(region_insert(kern_pgdir, pp1,  PGSIZE, PTE_NONE_U) == 0);
	assert((*pgdir_walk(kern_pgdir,  PGSIZE, 0) & PTE_RW_U) != PTE_RW_U);

	// should have pp1 at both 0 and PGSIZE, pp2 nowhere, ...
	assert(check_va2pa(kern_pgdir, 0) == region2pa(pp1));
	assert(check_va2pa(kern_pgdir, PGSIZE) == region2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->refn == 2);
	assert(pp2->refn == 0);

	// pp2 should be returned by region_alloc
	assert((pp = region_alloc(0)) && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at PGSIZE
	region_remove(kern_pgdir, 0x0);
	assert(check_va2pa(kern_pgdir, 0x0) == ~0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == region2pa(pp1));
	assert(pp1->refn == 1);
	assert(pp2->refn == 0);

	// test re-inserting pp1 at PGSIZE
	assert(region_insert(kern_pgdir, pp1,  PGSIZE, 0) == 0);
	assert(pp1->refn);
	assert(pp1->next == NULL);

	// unmapping pp1 at PGSIZE should free it
	region_remove(kern_pgdir,  PGSIZE);
	assert(check_va2pa(kern_pgdir, 0x0) == ~0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == ~0);
	assert(pp1->refn == 0);
	assert(pp2->refn == 0);

	// so it should be returned by region_alloc
	assert((pp = region_alloc(0)) && pp == pp1);

	// should be no free memory
	assert(!region_alloc(0));

	// forcibly take pp0 back
	assert(PTE_SMALL_ADDR(kern_pgdir[0]) == region2pa(pp0));
	kern_pgdir[0] = 0;
	assert(pp0->refn == 1);
	pp0->refn = 0;

	// check pointer arithmetic in pgdir_walk
	region_free(pp0);
	va = (PGSIZE * NPDENTRIES + PGSIZE);
	ptep = pgdir_walk(kern_pgdir, va, 1);
	ptep1 = (pte_t *) KADDR(PTE_SMALL_ADDR(kern_pgdir[PDX(va)]));
	assert(ptep == ptep1 + PTX(va));
	kern_pgdir[PDX(va)] = 0;
	pp0->refn = 0;

	// check that new page tables get cleared
	memset((void*)region2kva(pp0), 0xFF, PGSIZE);
	region_free(pp0);
	pgdir_walk(kern_pgdir, 0x0, 1);
	ptep = (pte_t *) region2kva(pp0);
	for(i=0; i<NPTENTRIES; i++)
		assert((ptep[i] & PTE_P) == 0);
	kern_pgdir[0] = 0;
	pp0->refn = 0;
	
	// give free list back
	free_regions = fl;

	// free the regions we took
	region_free(pp0);
	region_free(pp1);
	region_free(pp2);

	// test mmio_map_region
	mm1 = (uintptr_t) mmio_map_region(0, 4097);
	mm2 = (uintptr_t) mmio_map_region(0, 4096);
	// check that they're in the right region
	assert(mm1 >= MMIOBASE && mm1 + 8096 < MMIOLIM);
	assert(mm2 >= MMIOBASE && mm2 + 8096 < MMIOLIM);
	// check that they're page-aligned
	assert(mm1 % PGSIZE == 0 && mm2 % PGSIZE == 0);
	// check that they don't overlap
	assert(mm1 + 8096 <= mm2);
	// check page mappings
	assert(check_va2pa(kern_pgdir, mm1) == 0);
	assert(check_va2pa(kern_pgdir, mm1+PGSIZE) == PGSIZE);
	assert(check_va2pa(kern_pgdir, mm2) == 0);
	assert(check_va2pa(kern_pgdir, mm2+PGSIZE) == ~0);
	// check permissions
	assert((*pgdir_walk(kern_pgdir,  mm1, 0) & (PTE_NONE_U)) == PTE_NONE_U);
	assert(PTE_RW_U != (*pgdir_walk(kern_pgdir,  mm1, 0) & PTE_RW_U));
	// clear the mappings
	*pgdir_walk(kern_pgdir,  mm1, 0) = 0;
	*pgdir_walk(kern_pgdir,  mm1 + PGSIZE, 0) = 0;
	*pgdir_walk(kern_pgdir,  mm2, 0) = 0;
	
    cprintf("check_region() succeeded!\n");
}


static void
check_kern_pgdir(void)
{
	uint32_t i;
	pde_t *pgdir;

	pgdir = kern_pgdir;

    /* I comment this because it's not convenient for arm architecture
       to expose pgdir to users.
	   // check regions array
	n = ROUNDUP(nregions*sizeof(struct mem_region), PGSIZE);
	for (i = 0; i < n; i += PGSIZE)
		assert(check_va2pa(pgdir, Uregions + i) == PADDR(regions) + i);
	*/
	
	/*
	// check envs array (new test for lab 3)
	n = ROUNDUP(NENV*sizeof(struct Env), PGSIZE);
	for (i = 0; i < n; i += PGSIZE)
		assert(check_va2pa(pgdir, UENVS + i) == PADDR(envs) + i);

    */
    
    
	// check phys mem
	for (i = 0; i < MAX_REGION * PGSIZE; i += PGSIZE)
		assert(check_va2pa(pgdir, KERNBASE + i) == i);

    /*
	// check kernel stack
	// (updated in lab 4 to check per-CPU kernel stacks)
	for (n = 0; n < NCPU; n++) {
		uint32_t base = KSTACKTOP - (KSTKSIZE + KSTKGAP) * (n + 1);
		for (i = 0; i < KSTKSIZE; i += PGSIZE)
			assert(check_va2pa(pgdir, base + KSTKGAP + i)
				== PADDR(percpu_kstacks[n]) + i);
		for (i = 0; i < KSTKGAP; i += PGSIZE)
			assert(check_va2pa(pgdir, base + i) == ~0);
	}
    */
    
	// check PDE permissions
	for (i = 0; i < NPDENTRIES; i++) {
		switch (i) {
		case PDX(KSTACKTOP-1):
		//comment for lab3 case PDX(UENVS):
		case PDX(MMIOBASE):
		case PDX(MCONSOLE):
			assert(pgdir[i] & PDE_P);
			break;
		default:
			if (i >= PDX(KERNBASE)) {
				assert(pgdir[i] & PTE_P);
				assert(PDE_NONE_U == (pgdir[i] & PDE_NONE_U));
			} else {
				assert(pgdir[i] == 0);
			}
			break;
		}
	}
	cprintf("check_kern_pgdir() succeeded!\n");
}

// check region_insert, region_remove, &c, with an installed kern_pgdir
static void
check_region_installed_pgdir(void)
{
	struct mem_region *pp0, *pp1, *pp2;

	// check that we can read and write installed regions
	pp1 = pp2 = 0;
	assert((pp0 = region_alloc(0)));
	assert((pp1 = region_alloc(0)));
	assert((pp2 = region_alloc(0)));
	region_free(pp0);
	memset((void*)region2kva(pp1), 1, PGSIZE);
	memset((void*)region2kva(pp2), 2, PGSIZE);
	region_insert(kern_pgdir, pp1,  PGSIZE, PTE_NONE_U);
	assert(pp1->refn == 1);
	assert(*(uint32_t *)PGSIZE == 0x01010101U);
	region_insert(kern_pgdir, pp2,  PGSIZE, PTE_NONE_U);
	assert(*(uint32_t *)PGSIZE == 0x02020202U);
	assert(pp2->refn == 1);
	assert(pp1->refn == 0);
	*(uint32_t *)PGSIZE = 0x03030303U;
	assert(*(uint32_t *)region2kva(pp2) == 0x03030303U);
	region_remove(kern_pgdir,  PGSIZE);
	assert(pp2->refn == 0);

	// forcibly take pp0 back
	assert(PTE_SMALL_ADDR(kern_pgdir[0]) == region2pa(pp0));
	kern_pgdir[0] = 0;
	assert(pp0->refn == 1);
	pp0->refn = 0;

	// free the regions we took
	region_free(pp0);

	cprintf("check_page_installed_pgdir() succeeded!\n");
}

void tlb_invalidate(pde_t* pgdir, uintptr_t va) {
    asm("mcr p15, 0, %0, c8,c7, 1"
        :
        : "r"(va)
        :);
}

