"""Execute the production v3 command path's PIC24 instructions.

This deliberately small interpreter covers framing/guards/latch order, not
silicon timing, UART hardware, NVM physics, or the reset entry path. CC0.
"""
from pathlib import Path
import re
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "v3" / "ds30loader.s"

class CommandMachine:
    aliases = dict(DOERASE=1, WBUFPTR=2, WCNT=3, WADDR2=4, WADDR=5,
                   ERASEPAGE=6, RESIDENT=8, LENGTH=9, STORED=12, WCMD=13, WCRC=14)

    def __init__(self):
        self.code, self.labels = [], {}
        for line in SOURCE.read_text().splitlines():
            line = line.split(";", 1)[0].strip()
            if not line or line.startswith("."):
                continue
            if ":" in line:
                label, line = line.split(":", 1)
                self.labels[label] = len(self.code)
                line = line.strip()
            if line:
                op, _, args = line.partition(" ")
                self.code.append((op.lower(), [s.strip() for s in args.split(",")] if args else []))
        self.reg = [0] * 16
        self.mem = bytearray(512)
        self.page = 0
        self.flash, self.latches = {}, {}
        self.operations = []
        self.equal = self.less = False

    def regno(self, name):
        return self.aliases[name] if name in self.aliases else int(name[1:])

    def value(self, operand, byte=False):
        if operand.startswith("#"):
            literal = operand[1:]
            return {"buffer": 0, "ROWSIZE": 64, "BLCHECKST": 0xA780}.get(literal,
                    ord(literal[1]) if literal.startswith("'") else
                    int(literal, 0) if re.fullmatch(r"0x[0-9A-Fa-f]+|[0-9]+", literal) else 0)
        if operand.startswith("["):
            name = operand.strip("[]").replace("++", "")
            reg = self.regno(name)
            result = self.mem[self.reg[reg]]
            if "++" in operand:
                self.reg[reg] += 1 if byte else 2
            return result
        if operand == "TBLPAG": return self.page
        return self.reg[self.regno(operand)] & (0xFF if byte else 0xFFFF)

    def store(self, operand, value, byte=False):
        value &= 0xFF if byte else 0xFFFF
        if operand == "TBLPAG":
            self.page = value
        elif operand.startswith("["):
            reg = self.regno(operand.strip("[]").replace("++", ""))
            self.mem[self.reg[reg]] = value
            if "++" in operand: self.reg[reg] += 1 if byte else 2
        else:
            reg = self.regno(operand)
            self.reg[reg] = (self.reg[reg] & 0xFF00 | value) if byte else value

    def execute(self, packet):
        self.input = iter(packet)
        pc = self.labels["main"]
        for _ in range(10000):
            op, a = self.code[pc]
            pc += 1
            byte = op.endswith(".b")
            base = op.removesuffix(".b")
            if base == "rcall":
                if a[0] == "Receive":
                    value = next(self.input)
                    self.reg[0] = value
                    self.reg[14] = (self.reg[14] + value) & 0xFFFF
                elif a[0] == "Send":
                    return self.reg[0]
                elif a[0] == "Write":
                    self.operations.append((self.reg[0], self.page, self.reg[4]))
                    if self.reg[0] == 0x4001:
                        self.flash.update(self.latches)
                    self.latches = {}
                else: raise AssertionError(a)
            elif base == "mov": self.store(a[1], self.value(a[0], byte), byte)
            elif base == "clr": self.store(a[0], 0, byte)
            elif base in ("cp", "cp0"):
                lhs = self.value(a[0], byte)
                rhs = self.value(a[1], byte) if base == "cp" else 0
                self.equal, self.less = lhs == rhs, lhs < rhs
            elif base == "bra":
                if len(a) == 1: pc = self.labels[a[0]]
                elif {"z": self.equal, "nz": not self.equal,
                      "geu": not self.less, "gtu": not self.less and not self.equal}[a[0]]:
                    pc = self.labels[a[1]]
            elif base in ("inc", "dec"):
                result = (self.value(a[0]) + (1 if base == "inc" else -1)) & 0xFFFF
                self.store(a[-1], result)
                self.equal = result == 0
            elif base in ("and", "ior", "sl"):
                lhs, rhs = self.value(a[0]), self.value(a[1])
                result = lhs & rhs if base == "and" else lhs | rhs if base == "ior" else lhs << rhs
                self.store(a[-1], result)
                self.equal = (result & 0xFFFF) == 0
            elif base == "btsc":
                # The NVM hook models a successful, completed operation.
                if a[0] != "NVMCON": raise AssertionError(a)
                pc += 1
            elif base.startswith("tbl"):
                write = base.startswith("tblw")
                high = "h" in base
                pointer = a[1] if write else a[0]
                reg = self.regno(pointer.strip("[]").replace("++", ""))
                address = self.reg[reg]
                key = (address & ~1, 2 if high else address & 1)
                if write:
                    value = self.value(a[0], byte)
                    if byte: self.latches[key] = value
                    # Erase dummy TBLWTL doesn't commit a row.
                else:
                    self.store(a[1], self.flash.get(key, 0xFF), byte)
                if "++" in pointer: self.reg[reg] += 1 if byte else 2
            else:
                raise AssertionError((op, a))
        raise AssertionError("command did not terminate")

    def command(self, address=0x200, cmd=2, size=193, bad=False):
        data = [address >> 16 & 255, address >> 8 & 255, address & 255, cmd, size]
        if size:
            data += [(i * 13) & 255 for i in range(size - 1)]
            data += [(-sum(data) & 255) ^ bool(bad)]
        return self.execute(data)

class V3Commands(unittest.TestCase):
    def test_every_length(self):
        for size in range(256):
            with self.subTest(size=size):
                vm = CommandMachine()
                self.assertEqual(vm.command(size=size), ord("K" if size == 193 else "U"))
                self.assertEqual(len(vm.operations), int(size == 193))
                self.assertTrue(all(value == 0 for value in vm.mem[193:]))

    def test_addresses(self):
        vm = CommandMachine()
        for address in range(0xA800, 0x10000, 128):
            self.assertEqual(vm.command(address), ord("P"))
        for address in (0x10000, 0x10200, 0x800000, 0xFF0000, 0xFFFF80):
            self.assertEqual(vm.command(address), ord("P"))
        for offset in range(1, 128):
            self.assertEqual(vm.command(0x200 + offset), ord("P"))
        self.assertFalse(vm.operations)
        for address in (0, 0x200, 0xA700, 0xA780):
            self.assertEqual(vm.command(address), ord("K"))

    def test_reset_vector(self):
        vm = CommandMachine()
        self.assertEqual(vm.command(0), ord("K"))
        self.assertEqual(list(vm.mem[:6]), [4, 0, 0xA8, 0, 0, 0])

    def test_erase_binding_and_error_recovery(self):
        vm = CommandMachine()
        self.assertEqual(vm.command(0x400, 1, 1), ord("K"))
        self.assertEqual(vm.command(0x800), ord("P"))
        self.assertFalse(vm.operations)
        self.assertEqual(vm.command(0x400), ord("K"))
        self.assertEqual([x[0] for x in vm.operations], [0x4001])
        vm.operations.clear()
        self.assertEqual(vm.command(0x400, 1, 1), ord("K"))
        self.assertEqual(vm.command(0x480), ord("K"))
        self.assertEqual([x[0] for x in vm.operations], [0x4042, 0x4001])
        self.assertEqual(vm.command(0x400, 1, 1), ord("K"))
        self.assertEqual(vm.command(0x400, bad=True), ord("N"))
        self.assertEqual(vm.reg[1], 0)
        self.assertEqual(vm.command(cmd=0x82), ord("U"))
        self.assertEqual(vm.command(0x480, 1, 1), ord("P"))

if __name__ == "__main__":
    unittest.main()
