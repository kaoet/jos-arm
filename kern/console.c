#include <inc/types.h>
#include <kern/pmap.h>
#include <inc/config.h>

struct uart {
	uint32_t dr;
	uint32_t whatever[5];
	uint32_t fr;
};

static struct uart *uart0 = (struct uart*)
(MCONSOLE +
#ifdef VERSATILE_PB
	0xF1000
#else
	0x01000
#endif
);

void console_init()
{
#ifdef VERSATILE_PB
	uart0 = (struct uart *)mmio_map_region(0x101F1000, 4 * 1024);
#else
	uart0 = (struct uart *)mmio_map_region(0x20201000, 4 * 1024);
#endif
}

int iscons(int fdnum)
{
	return 1;
}

void cputchar(int c)
{
	while (uart0->fr & 0x20)
		continue;
	uart0->dr = c;
}

int getchar()
{
	while (uart0->fr & 0x10)
		continue;
	return uart0->dr;
}
