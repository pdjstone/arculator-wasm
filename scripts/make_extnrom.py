#!/usr/bin/env python3
"""Build an Archimedes extension ROM image (e.g. roms/arcrom_ext)
from RISC OS module files (",ffa").

The layout follows extnrom_load() in ArcEm's arch/extnrom.c:

  0x00  Extended expansion card identity (8 bytes, product type 0x0087)
  0x08  Interrupt status pointers (8 zero bytes)
  0x10  Chunk directory: one 8-byte entry per chunk, then a zero word
        - a device description chunk (OS identity byte 0xF5)
        - one RISC OS module chunk (OS identity byte 0x81) per file
  ....  Description string, then each module preceded by the
        undocumented size prefix word (module size + 4)
  end   16-byte extension ROM footer: ROM size, checksum, "ExtnROM0"

The checksum is the 32-bit sum of every word except the last three.

See also the PRM chapter on expansion cards and extension ROMs:
https://www.riscos.com/support/developers/prm/excards.html

Usage (run by "make roms/arcrom_ext"):
  scripts/make_extnrom.py -o roms/arcrom_ext \\
      src/hostfs/hostfs,ffa src/hostfs/hostfsfiler,ffa
"""

import argparse
import struct
import sys

PRODUCT_TYPE_EXTENSION_ROM = 0x0087
MANUFACTURER_CODE = 0x0000
COUNTRY_CODE = 0x00

DESCRIPTION_STRING = b"ArcEm Support"

OS_ID_BYTE_RISCOS_MODULE = 0x81
OS_ID_BYTE_DEVICE_DESCR = 0xF5

MAXIMUM_FILE_SIZE = 0xFFFFFF


def round_up(x, n):
    return (x + n - 1) & ~(n - 1)


def build_rom(modules, description=DESCRIPTION_STRING):
    """Return the ROM image for a list of module byte strings."""
    if not modules:
        raise ValueError("no modules given")
    for m in modules:
        if len(m) > MAXIMUM_FILE_SIZE:
            raise ValueError("module too large for a chunk directory entry")

    # (os_id_byte, size, data, size_prefix)
    chunks = [(OS_ID_BYTE_DEVICE_DESCR, len(description) + 1,
               description + b"\0", False)]
    chunks += [(OS_ID_BYTE_RISCOS_MODULE, len(m), m, True) for m in modules]

    # Header (16 bytes), chunk directory, terminator word
    data_start = 16 + 8 * len(chunks) + 4

    # Place the chunk data
    placed = []  # (offset of data, chunk)
    pos = data_start
    for chunk in chunks:
        _, size, _, prefix = chunk
        if prefix:
            pos += 4
        placed.append((pos, chunk))
        pos += round_up(size, 4)

    rom_size = round_up(pos + 16, 0x10000)
    rom = bytearray(rom_size)

    # Extended expansion card identity: Acorn conformant, no interrupts;
    # flags 0x03 = 8-bit wide, interrupt status pointers and chunk
    # directory present
    struct.pack_into("<II", rom, 0,
                     0x00 | (0x03 << 8) | (0x00 << 16) |
                     ((PRODUCT_TYPE_EXTENSION_ROM & 0xFF) << 24),
                     (PRODUCT_TYPE_EXTENSION_ROM >> 8) |
                     (MANUFACTURER_CODE << 8) | (COUNTRY_CODE << 24))
    # Interrupt status pointers are left as zero

    entry = 16
    for offset, (os_id, size, data, prefix) in placed:
        struct.pack_into("<II", rom, entry, os_id | (size << 8), offset)
        entry += 8
        if prefix:
            # Size prefix includes the size word itself
            struct.pack_into("<I", rom, offset - 4, size + 4)
        rom[offset:offset + len(data)] = data
    # Chunk directory terminator is already zero

    # Footer: ROM size, checksum, "ExtnROM0"
    struct.pack_into("<I", rom, rom_size - 16, rom_size)
    words = struct.unpack_from("<%dI" % (rom_size // 4 - 3), rom)
    struct.pack_into("<I", rom, rom_size - 12, sum(words) & 0xFFFFFFFF)
    rom[rom_size - 8:] = b"ExtnROM0"

    return bytes(rom)


def main():
    ap = argparse.ArgumentParser(
        description="Build an extension ROM image from RISC OS modules.")
    ap.add_argument("modules", nargs="+",
                    help="module files, in the order they go in the ROM")
    ap.add_argument("-o", "--output", required=True, help="ROM image to write")
    ap.add_argument("--max-size", type=lambda s: int(s, 0), default=0x10000,
                    help="fail if the ROM is larger than this "
                         "(default 0x10000, the size Arculator loads)")
    args = ap.parse_args()

    modules = []
    for path in args.modules:
        with open(path, "rb") as f:
            modules.append(f.read())

    rom = build_rom(modules)
    if len(rom) > args.max_size:
        sys.exit("ROM is %d bytes, more than the maximum of %d"
                 % (len(rom), args.max_size))

    with open(args.output, "wb") as f:
        f.write(rom)
    print("Wrote %s: %d bytes, %d modules" % (args.output, len(rom), len(modules)))


if __name__ == "__main__":
    main()
