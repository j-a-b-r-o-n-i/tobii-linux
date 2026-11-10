#!/usr/bin/env python3
"""Clear the executable-stack flag on 64-bit ELF shared libraries.

glibc 2.41 and later refuse to dlopen() a library that asks for an executable
stack. Tobii's Stream Engine 3.0.4 asks for one without needing it, which stops
Tobii Config from loading it. This rewrites the PT_GNU_STACK program header
from RWE to RW in place, the same as `patchelf --clear-execstack`.

Usage: scripts/clear_execstack.py <library.so> [...]
"""
import struct
import sys

PT_GNU_STACK = 0x6474E551
PF_X = 1

for path in sys.argv[1:]:
    with open(path, "r+b") as f:
        ident = f.read(16)
        if ident[:4] != b"\x7fELF" or ident[4] != 2 or ident[5] != 1:
            sys.exit(f"{path}: not a 64-bit little-endian ELF file")
        f.seek(0x20)
        (phoff,) = struct.unpack("<Q", f.read(8))
        f.seek(0x36)
        phentsize, phnum = struct.unpack("<HH", f.read(4))
        for i in range(phnum):
            at = phoff + i * phentsize
            f.seek(at)
            p_type, p_flags = struct.unpack("<II", f.read(8))
            if p_type == PT_GNU_STACK:
                if p_flags & PF_X:
                    f.seek(at + 4)
                    f.write(struct.pack("<I", p_flags & ~PF_X))
                    print(f"{path}: executable stack cleared")
                else:
                    print(f"{path}: already clear")
                break
        else:
            print(f"{path}: no PT_GNU_STACK header")
