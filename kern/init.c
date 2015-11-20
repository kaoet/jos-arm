#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/console.h>

uint8_t bootstack[KSTKSIZE] __attribute__((aligned(PGSIZE)));

void kern_init()
{
	mem_init();
	console_init();
	cprintf("Hello\n");
	monitor(NULL);
}

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	// Be extra sure that the machine is in as reasonable state
	// __asm __volatile("cli; cld");

	va_start(ap, fmt);
	cprintf("kernel panic on CPU at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		continue;
}

void raise() {for(;;);}