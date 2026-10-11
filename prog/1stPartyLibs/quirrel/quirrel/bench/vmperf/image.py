"""Code of a csq.exe build: symbols from its pdb, function bounds, the opcode dispatch block."""
import os
import re
import struct
import subprocess

import build

EXECUTE = "??$Execute@$0A@@SQVM@@QEAA_NAEBUSQObjectPtr@@_J1AEAU1@_KW4ExecutionType@0@@Z"  # Execute<false>


class Image:
    def __init__(self, exe):
        self.exe = exe
        with open(exe, "rb") as f:
            data = f.read()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        self.base = struct.unpack_from("<Q", data, pe + 24 + 24)[0]
        nsec, opt_size = struct.unpack_from("<H", data, pe + 6)[0], struct.unpack_from("<H", data, pe + 20)[0]
        self.sections = {}
        for i in range(nsec):
            off = pe + 24 + opt_size + i * 40
            vsize, va, rsize, raw = struct.unpack_from("<IIII", data, off + 8)
            self.sections[data[off:off + 8].rstrip(b"\0").decode()] = (va, data[raw:raw + min(vsize, rsize)])
        pdata = self.sections[".pdata"][1]
        self.pdata = sorted(struct.unpack_from("<II", pdata, k) for k in range(0, len(pdata) - 11, 12))
        self._symbols = None

    def symbols(self):
        """{rva: public symbol name}"""
        if self._symbols is None:
            out = subprocess.run([os.path.join(build.llvm_bin(), "llvm-pdbutil.exe"), "dump", "-publics",
                                  os.path.splitext(self.exe)[0] + ".pdb"],
                                 capture_output=True, text=True, errors="replace").stdout
            self._symbols, name = {}, None
            text_va = self.sections[".text"][0]
            for line in out.splitlines():
                m = re.search(r"S_PUB32 \[size = \d+\] `(.*)`", line)
                if m:
                    name = m.group(1)
                    continue
                m = re.search(r"addr = 0001:(\d+)", line)
                if m and name:
                    self._symbols[text_va + int(m.group(1))] = name
                    name = None
        return self._symbols

    def function(self, name):
        """(rva, end) of a function; end follows the chained .pdata ranges."""
        rva = next((a for a, n in self.symbols().items() if n == name), None)
        if rva is None:
            raise SystemExit(f"{self.exe}: no symbol {name} in its pdb")
        end = None
        for b, e in self.pdata:
            if b == rva or (end is not None and b == end):
                end = e
        return rva, end

    def disassemble(self, rva, end):
        """[(rva, instruction)] in Intel syntax; an operand that is an address reads @<rva>."""
        out = subprocess.run([os.path.join(build.llvm_bin(), "llvm-objdump.exe"), "-d", "--no-show-raw-insn",
                              "--x86-asm-syntax=intel", f"--start-address={self.base + rva:#x}",
                              f"--stop-address={self.base + end:#x}", self.exe], capture_output=True, text=True).stdout
        res = []
        for line in out.splitlines():
            m = re.match(r"\s*([0-9a-f]+):\s+([^#<]*)", line)
            if m:
                ins = re.sub(r"0x[0-9a-f]{9,}", lambda a: f"@{int(a.group(0), 16) - self.base:#x}", m.group(2))
                res.append((int(m.group(1), 16) - self.base, " ".join(ins.split())))
        return res

    def listing(self, name, path):
        """Write the code of a function with offsets from its start, to diff two builds."""
        rva, end = self.function(name)
        syms = self.symbols()

        def sub(m):
            t = int(m.group(1), 16)
            return f"L{t - rva:05x}" if rva <= t < end else syms.get(t, m.group(0))
        with open(path, "w") as f:
            f.write(f"; {name} rva {rva:#x}, {rva % 64} bytes into its 64-byte line\n")
            for addr, ins in self.disassemble(rva, end):
                f.write(f"{addr - rva:05x}  {re.sub(r'@(0x[0-9a-f]+)', sub, ins)}\n")

    def dispatch(self):
        """The shared dispatch block of Execute<false>: the opcode load, the range check, the jump
        table load and the indirect jmp. A reload of the table base right before the block counts
        as part of it, since handlers enter there. Returns dict(start, length)."""
        rva, end = self.function(EXECUTE)
        asm = self.disassemble(rva, end)
        for i, (_, ins) in enumerate(asm):
            m = re.fullmatch(r"jmp (r\w+)", ins)
            if not m or i < 6:
                continue
            reg = m.group(1)
            add = re.fullmatch(rf"add {reg}, (r\w+)", asm[i - 1][1])
            load = re.fullmatch(rf"movsxd {reg}, dword ptr \[(r\w+) \+ 4\*{reg}\]", asm[i - 2][1])
            if not (add and load and add.group(1) == load.group(1)):
                continue
            lea = f"lea {load.group(1)}, [rip"
            j = i - 4 if asm[i - 3][1].startswith(lea) else i - 3
            ja = re.fullmatch(r"ja @(0x[0-9a-f]+)", asm[j][1])
            if not (ja and asm[j - 1][1].startswith("cmp")):
                continue
            # the range check jumps back to the start of the block (opcode 0 is a no-op)
            start = int(ja.group(1), 16)
            k = next(n for n, (a, _) in enumerate(asm) if a == start)
            if asm[k - 1][1].startswith(lea):
                start = asm[k - 1][0]
            return dict(start=start, length=asm[i + 1][0] - start)
        raise SystemExit(f"{self.exe}: the dispatch block of SQVM::Execute was not found")


def describe_dispatch(d):
    """The bytes of its 64-byte line that the block takes, e.g. "48-83 CROSSES"."""
    offset = d["start"] % 64
    return f"{offset}-{offset + d['length'] - 1}" + (" CROSSES" if offset + d["length"] > 64 else "")
