#!/usr/bin/env python3
"""Inspect ARM target artifacts and a linked barrier probe; never runs target code."""
from pathlib import Path
import os
import re
import subprocess

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ.get('DEVKITPRO', '/opt/devkitpro'))
arm = Path(os.environ.get('DEVKITARM', sdk / 'devkitARM'))
bin_dir = arm / 'bin'
cxx = str(bin_dir / 'arm-none-eabi-g++')
readelf = str(bin_dir / 'arm-none-eabi-readelf')
objdump = str(bin_dir / 'arm-none-eabi-objdump')

def output(args):
    return subprocess.check_output(args, cwd=root, text=True)

def check(path):
    attributes = output([readelf, '-A', str(path)])
    arches = re.findall(r'Tag_CPU_arch: (\S+)', attributes)
    if not arches or any(arch != 'v6K' for arch in arches):
        raise RuntimeError(f'{path}: expected ARMv6K, got {arches}')
    if 'Thumb-2' in attributes:
        raise RuntimeError(f'{path}: unexpected Thumb-2 requirement')

# Keep SDK startup/link behavior; change only the barrier implementation that
# the installed 3dsx.specs currently selects for ARMv7.
installed = Path(output([cxx, '-print-file-name=3dsx.specs']).strip()).read_text()
expected = installed.replace('%include <sync-dmb.specs>', '%include <sync-cp15dmb.specs>')
if expected.strip() != (root / 'tools/3dsx-armv6k.specs').read_text().strip():
    raise RuntimeError('SDK 3dsx.specs changed: review the local ARMv6K startup/link specs')
check(root / 'build/3ds-chess.elf')
objects = list((root / 'build/3ds').rglob('*.o'))
if not objects:
    raise RuntimeError('Missing target objects')
for path in objects:
    check(path)
folder = root / 'build/update-090'
folder.mkdir(exist_ok=True)
probe = folder / 'barrier_probe.cpp'
probe.write_text('int main() { asm volatile("bl __sync_synchronize" ::: "r0", "lr", "memory"); return 0; }\n')
elf = folder / 'barrier_probe.elf'
subprocess.run([cxx, '-march=armv6k', '-mtune=mpcore', '-mfloat-abi=hard', '-mtp=soft',
                '-std=gnu++17', '-fno-exceptions', '-fno-rtti', '-specs=tools/3dsx-armv6k.specs',
                str(probe), '-L' + str(sdk / 'libctru/lib'), '-lctru', '-lm', '-o', str(elf)],
               cwd=root, check=True)
check(elf)
disassembly = output([objdump, '-d', str(elf)])
match = re.search(r'<__sync_synchronize(?:_cp15dmb)?>:\n(.*?)(?:\n\n|\Z)', disassembly, re.S)
if not match or not re.search(r'\bmcr\b.*(?:p15|15)', match[1]) or '{5}' not in match[1]:
    raise RuntimeError('Barrier probe does not resolve to the ARMv6K CP15 barrier')
print(f'Target: ELF and {len(objects)} project objects ARMv6K; linked probe uses CP15 MCR barrier, no Thumb-2 requirement. No target execution.')
