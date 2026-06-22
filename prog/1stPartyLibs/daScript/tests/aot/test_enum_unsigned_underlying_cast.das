options gen2
options rtti
require dastest/testing_boost public
require daslib/json_boost

// AOT mirror of tests/language/enum_unsigned_underlying_cast.das — exercises the same
// uint8/uint16/uint32/uint64-backed enum cast cases through the AOT codegen path. Post-fix the
// generated C++ should call enum8u_to_uint/enum16u_to_uint/enum64u_to_uint (zero-extending)
// rather than the signed helpers.

enum Opcode : uint8 {
    NOP = 0u8
    PING = 50u8
    HEARTBEAT = 200u8
    SHUTDOWN = 254u8
}

enum Channel : uint16 {
    SYS = 100ul
    USER = 5000ul
    BROADCAST = 60000ul
}

enum Mask32 : uint {
    NONE = 0u
    LOW = 0x1u
    HIGH_BIT = 0x80000000u
}

enum Wide64 : uint64 {
    LOW = 100ul
    HIGH_BIT = 0x8000000000000000ul
}

enum Signed8 : int8 {
    LOW = 50
    NEGATIVE = 0xce              // int8 bit-reinterpret = -50
}

enum Signed16 : int16 {
    LOW = 1000l
    NEGATIVE = 0xfc18l           // int16 bit-reinterpret = -1000
}

[test]
def test_aot_unsigned_underlying_zero_extends(t : T?) {
    t |> run("aot: uint8-backed enum cast preserves byte value") @(t : T?) {
        t |> equal(uint(Opcode.HEARTBEAT), 200u)
        t |> equal(int(Opcode.HEARTBEAT), 200)
        t |> equal(int64(Opcode.SHUTDOWN), 254l)
    }
    t |> run("aot: uint16-backed enum cast preserves half-word value") @(t : T?) {
        t |> equal(uint(Channel.BROADCAST), 60000u)
        t |> equal(int(Channel.BROADCAST), 60000)
    }
    t |> run("aot: uint64-backed enum cast preserves bit pattern (top bit set)") @(t : T?) {
        t |> equal(uint64(Wide64.HIGH_BIT), 0x8000000000000000ul)
        t |> equal(int64(Wide64.HIGH_BIT), -9223372036854775807l - 1l)
        t |> equal(uint64(Wide64.LOW), 100ul)
    }
}

[test]
def test_aot_signed_underlying_unchanged(t : T?) {
    t |> run("aot: int8-backed enum cast keeps sign-reinterpret") @(t : T?) {
        t |> equal(int(Signed8.NEGATIVE), -50)
        t |> equal(uint(Signed8.NEGATIVE), 0xffffffceu)
    }
    t |> run("aot: int16-backed enum cast keeps sign-reinterpret") @(t : T?) {
        t |> equal(int(Signed16.NEGATIVE), -1000)
    }
}

[test]
def test_aot_runtime_iteration(t : T?) {
    t |> run("aot: runtime for-loop sum of uint8 enums") @(t : T?) {
        var arr : array<Opcode>
        arr |> push(Opcode.PING)
        arr |> push(Opcode.HEARTBEAT)
        var sum = 0u
        for (op in arr) {
            sum += uint(op)
        }
        t |> equal(sum, 250u)
    }
    t |> run("aot: runtime uint64 enum cast (top bit set)") @(t : T?) {
        var arr : array<Wide64>
        arr |> push(Wide64.LOW)
        arr |> push(Wide64.HIGH_BIT)
        var hit_high = false
        for (w in arr) {
            if (w == Wide64.HIGH_BIT) {
                hit_high = true
                t |> equal(uint64(w), 0x8000000000000000ul)
            }
        }
        t |> equal(hit_high, true)
    }
}

[test]
def test_aot_unsigned_enum_equality(t : T?) {
    t |> run("aot: uint8 enum == operator works") @(t : T?) {
        let a = Opcode.HEARTBEAT
        let c = Opcode.PING
        t |> equal(a == a, true)
        t |> equal(a != c, true)
    }
    t |> run("aot: uint16 enum == operator works") @(t : T?) {
        let a = Channel.BROADCAST
        let c = Channel.SYS
        t |> equal(a == a, true)
        t |> equal(a != c, true)
    }
    t |> run("aot: uint32 enum == operator works") @(t : T?) {
        let a = Mask32.HIGH_BIT
        let c = Mask32.LOW
        t |> equal(a == a, true)
        t |> equal(a != c, true)
    }
    t |> run("aot: uint64 enum == operator works (top bit set)") @(t : T?) {
        let a = Wide64.HIGH_BIT
        let c = Wide64.LOW
        t |> equal(a == a, true)
        t |> equal(a != c, true)
    }
}

struct Pkt {
    op : Opcode
}

struct Pkt64 {
    w : Wide64
}

[test]
def test_aot_struct_field_cast(t : T?) {
    t |> run("aot: struct field of uint8 enum, cast to uint") @(t : T?) {
        let p = Pkt(op = Opcode.HEARTBEAT)
        t |> equal(uint(p.op), 200u)
    }
    t |> run("aot: struct field of uint64 enum, cast to uint64") @(t : T?) {
        let p = Pkt64(w = Wide64.HIGH_BIT)
        t |> equal(uint64(p.w), 0x8000000000000000ul)
    }
}

[test]
def test_aot_string_interpolation_names(t : T?) {
    t |> run("aot: uint8 enum interpolates value name") @(t : T?) {
        t |> equal("{Opcode.HEARTBEAT}", "HEARTBEAT")
    }
    t |> run("aot: uint16 enum interpolates value name") @(t : T?) {
        t |> equal("{Channel.BROADCAST}", "BROADCAST")
    }
    t |> run("aot: uint32 enum interpolates value name (high bit set)") @(t : T?) {
        t |> equal("{Mask32.HIGH_BIT}", "HIGH_BIT")
    }
    t |> run("aot: uint64 enum interpolates value name (top bit set)") @(t : T?) {
        t |> equal("{Wide64.HIGH_BIT}", "HIGH_BIT")
    }
}

struct PktU8 {
    op : Opcode
}

struct PktU16 {
    ch : Channel
}

struct PktU32 {
    mk : Mask32
}

struct PktU64 {
    w : Wide64
}

[test]
def test_aot_sprint_json_unsigned_enum_field(t : T?) {
    t |> run("aot: sprint_json uint8 enum renders by name") @(t : T?) {
        let p = PktU8(op = Opcode.HEARTBEAT)
        t |> equal(sprint_json(p, false), "\{\"op\":\"HEARTBEAT\"\}")
    }
    t |> run("aot: sprint_json uint16 enum renders by name") @(t : T?) {
        let p = PktU16(ch = Channel.BROADCAST)
        t |> equal(sprint_json(p, false), "\{\"ch\":\"BROADCAST\"\}")
    }
    t |> run("aot: sprint_json uint32 enum renders by name") @(t : T?) {
        let p = PktU32(mk = Mask32.HIGH_BIT)
        t |> equal(sprint_json(p, false), "\{\"mk\":\"HIGH_BIT\"\}")
    }
    t |> run("aot: sprint_json uint64 enum renders by name (top bit set)") @(t : T?) {
        let p = PktU64(w = Wide64.HIGH_BIT)
        t |> equal(sprint_json(p, false), "\{\"w\":\"HIGH_BIT\"\}")
    }
}
