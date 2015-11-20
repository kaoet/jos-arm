GCC_PREFIX := arm-none-eabi-
CC := $(GCC_PREFIX)gcc
LD := $(GCC_PREFIX)ld
AS := $(GCC_PREFIX)as
OBJDUMP := $(GCC_PREFIX)objdump

CFLAGS := -nostdlib -fno-builtin -nostdinc -I. -O1
CFLAGS += -DVERSATILE_PB -std=gnu99 -g
CFLAGS += -march=armv6zk -mcpu=arm1176jzf-s -marm -mfpu=vfp -mfloat-abi=hard
LDFLAGS := -nostdinc
LIBS := -lgcc

QEMU := qemu-system-arm
QEMUOPTS := -machine versatilepb -cpu arm1176 -m 256
QEMUOPTS += -nographic -no-reboot -gdb tcp::1234 -kernel target/kernel

SOURCE := kern/entry.S kern/main.c kern/console.c kern/printf.c lib/printfmt.c lib/string.c

target/kernel: $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	$(OBJDUMP) -S $@ > $@.asm

qemu: target/kernel
	$(QEMU) $(QEMUOPTS)

qemu-gdb: target/kernel
	$(QEMU) $(QEMUOPTS) -S

.PHONY: clean
clean:
	- rm target/*