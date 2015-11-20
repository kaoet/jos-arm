.global _start
_start:
	ldr sp, bootstack
	bl main
spin:
	b spin
bootstack:
	.word 0x80000