#!/usr/bin/env python3

import sys
import os
import subprocess

qemu = 'qemu-system-arm'
qemu_options = ['-machine', 'versatilepb', '-cpu', 'cortex-a8', '-m', '256']
qemu_options += ['-serial', 'mon:stdio', '-no-reboot', '-gdb', 'tcp::1234']
qemu_options += ['-kernel', 'build/kern/kernel']

def help_message():
	print(sys.argv[0], '[compile|qemu|qemu-gdb]')

if len(sys.argv) < 2:
	help_message()
	exit(1)

if sys.argv[1] == 'compile':
	try: os.mkdir('build')
	except: pass
	os.chdir('build')
	subprocess.call(['cmake', '..'])
	subprocess.call(['make'])
elif sys.argv[1] == 'qemu':
	subprocess.call([qemu] + qemu_options)
elif sys.argv[1] == 'qemu-gdb':
	subprocess.call([qemu] + qemu_options + ['-S'])
else:
	help_message()
	exit(1)