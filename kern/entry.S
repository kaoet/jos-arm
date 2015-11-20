.global _start
_start:
	ldr sp, bootstack
	bl kern_init
spin:
	b spin
bootstack:
	.word 0x80000