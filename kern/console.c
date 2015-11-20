#include <inc/types.h>

struct uart {
	uint32_t dr;
	uint32_t whatever[5];
	uint32_t fr;
};

static struct uart * const uart0 = (struct uart*)0x101f1000;

void cputchar(char c) {
	while (uart0->fr & 0x20)
		continue;
	uart0->dr = c;
}