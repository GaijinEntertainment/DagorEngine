options gen2
options rtti
require dastest/testing_boost public
require daslib/json_boost

// Casts of uint8/uint16/uint64-backed enums must zero-extend, not sign-extend.
// Signed-underlying enums must keep their existing sign-extending bit-reinterpret
// semantics (e.g. uint(int8_value=-50) stays 0xffffffce).

// Natural opcode byte — uint8 is the canonical choice for compact packed enums.
enum Opcode : uint8 {
    NOP = 0u8
    PING = 50u8
    HEARTBEAT = 200u8       // > 127, triggers the bug
    SHUTDOWN = 254u8
}

// Like a port id or compact channel — uint16 representable.
enum Channel : uint16 {
    SYS = 100ul
    USER = 5000ul
    BROADCAST = 60000ul     // > 32767, triggers the bug
}

// uint32-backed — the WalkEnumeration (32-bit) path also needed the zero-extend fix.
enum Mask32 : uint {
    NONE = 0u
    LOW = 0x1u
    HIGH_BIT = 0x80000000u  // > INT32_MAX, exercises the WalkEnumeration (32-bit) fix
}

// uint64-backed — exercises the EnumStub64u binding (cast helpers, ==/!= policies, isSameType
// gating for tEnumeration64). The HIGH_BIT value has the int64 sign bit set, so any leftover
// sign-vs-unsigned mismatch in the lookup paths surfaces here.
enum Wide64 : uint64 {
    LOW = 100ul
    HIGH_BIT = 0x8000000000000000ul   // 2^63 — top bit set
}

// Signed counterparts — behaviour preserved. Hex literals avoid the unary-minus form
// (daslang's lint pass treats `-N` as an expression, not an integer constant, in enum values).
enum Signed8 : int8 {
    LOW = 50
    NEGATIVE = 0xce              // int8 bit-reinterpret = -50
}

enum Signed16 : int16 {
    LOW = 1000l
    NEGATIVE = 0xfc18l           // int16 bit-reinterpret = -1000
}

[test]
def test_unsigned_underlying_zero_extends(t : T?) {
    t |> run("uint8-backed enum cast preserves byte value") @(t : T?) {
        t |> equal(uint(Opcode.HEARTBEAT), 200u)
        t |> equal(int(Opcode.HEARTBEAT), 200)
        t |> equal(int64(Opcode.SHUTDOWN), 254l)
        t |> equal(uint64(Opcode.SHUTDOWN), 254ul)
    }
    t |> run("uint16-backed enum cast preserves half-word value") @(t : T?) {
        t |> equal(uint(Channel.BROADCAST), 60000u)
        t |> equal(int(Channel.BROADCAST), 60000)
        t |> equal(int64(Channel.BROADCAST), 60000l)
    }
    t |> run("uint64-backed enum cast preserves bit pattern (top bit set)") @(t : T?) {
        t |> equal(uint64(Wide64.HIGH_BIT), 0x8000000000000000ul)
        // int64(uint64-enum) is a bit-reinterpret in daslang convention — top bit set → INT64_MIN
        // (written as -INT64_MAX - 1 because INT64_MIN as a literal overflows the int64 parser).
        t |> equal(int64(Wide64.HIGH_BIT), -9223372036854775807l - 1l)
        t |> equal(uint64(Wide64.LOW), 100ul)
    }
}

[test]
def test_signed_underlying_unchanged(t : T?) {
    t |> run("int8-backed enum cast keeps sign-reinterpret") @(t : T?) {
        t |> equal(int(Signed8.NEGATIVE), -50)
        t |> equal(uint(Signed8.NEGATIVE), 0xffffffceu)
    }
    t |> run("int16-backed enum cast keeps sign-reinterpret") @(t : T?) {
        t |> equal(int(Signed16.NEGATIVE), -1000)
    }
}

// Runtime path — defeats const folding so we exercise the SimNode side.
[test]
def test_runtime_iteration(t : T?) {
    t |> run("runtime: for-loop sum of uint8 enums") @(t : T?) {
        var arr : array<Opcode>
        arr |> push(Opcode.PING)
        arr |> push(Opcode.HEARTBEAT)
        var sum = 0u
        for (op in arr) {
            sum += uint(op)
        }
        t |> equal(sum, 250u)               // 50 + 200
    }
    t |> run("runtime: uint64 enum cast through array iteration (top bit set)") @(t : T?) {
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

// Equality on uint-backed enums — the isSameType tightening must NOT remove the ==/!= operator.
[test]
def test_unsigned_enum_equality(t : T?) {
    t |> run("uint8 enum == operator works") @(t : T?) {
        let a = Opcode.HEARTBEAT
        let b = Opcode.HEARTBEAT
        let c = Opcode.PING
        t |> equal(a == b, true)
        t |> equal(a == c, false)
        t |> equal(a != c, true)
    }
    t |> run("uint16 enum == operator works") @(t : T?) {
        let a = Channel.BROADCAST
        let b = Channel.BROADCAST
        let c = Channel.SYS
        t |> equal(a == b, true)
        t |> equal(a != c, true)
    }
    t |> run("uint32 enum == operator works") @(t : T?) {
        let a = Mask32.HIGH_BIT
        let b = Mask32.HIGH_BIT
        let c = Mask32.LOW
        t |> equal(a == b, true)
        t |> equal(a != c, true)
    }
    t |> run("uint64 enum == operator works (top bit set)") @(t : T?) {
        let a = Wide64.HIGH_BIT
        let b = Wide64.HIGH_BIT
        let c = Wide64.LOW
        t |> equal(a == b, true)
        t |> equal(a == c, false)
        t |> equal(a != c, true)
    }
}

// Struct field access — exercises SimNode_GetField lowering.
struct Pkt {
    op : Opcode
}

struct Pkt64 {
    w : Wide64
}

[test]
def test_struct_field_cast(t : T?) {
    t |> run("struct field of uint8 enum, cast to uint") @(t : T?) {
        let p = Pkt(op = Opcode.HEARTBEAT)
        t |> equal(uint(p.op), 200u)
    }
    t |> run("struct field of uint64 enum, cast to uint64") @(t : T?) {
        let p = Pkt64(w = Wide64.HIGH_BIT)
        t |> equal(uint64(p.w), 0x8000000000000000ul)
    }
}

// String interpolation goes through WalkEnumeration8 / Enum() value-name lookup.
[test]
def test_string_interpolation_names(t : T?) {
    t |> run("uint8 enum interpolates value name") @(t : T?) {
        t |> equal("{Opcode.HEARTBEAT}", "HEARTBEAT")
    }
    t |> run("uint16 enum interpolates value name") @(t : T?) {
        t |> equal("{Channel.BROADCAST}", "BROADCAST")
    }
    t |> run("uint32 enum interpolates value name (high bit set)") @(t : T?) {
        t |> equal("{Mask32.HIGH_BIT}", "HIGH_BIT")
    }
    t |> run("uint64 enum interpolates value name (top bit set)") @(t : T?) {
        t |> equal("{Wide64.HIGH_BIT}", "HIGH_BIT")
    }
}

// JSON serialization of a struct containing a uint-backed enum field — exercises
// json_print.cpp's WalkEnumeration*/Enum() value-name lookup. Before the fix the
// uint8 byte 0xC8 was read as int8(-56) and the lookup against the int64-stored
// field value (200) missed, producing {"op":} with an empty value.
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
def test_sprint_json_unsigned_enum_field(t : T?) {
    t |> run("sprint_json: uint8 enum HEARTBEAT renders by name") @(t : T?) {
        let p = PktU8(op = Opcode.HEARTBEAT)
        t |> equal(sprint_json(p, false), "\{\"op\":\"HEARTBEAT\"\}")
    }
    t |> run("sprint_json: uint16 enum BROADCAST renders by name") @(t : T?) {
        let p = PktU16(ch = Channel.BROADCAST)
        t |> equal(sprint_json(p, false), "\{\"ch\":\"BROADCAST\"\}")
    }
    t |> run("sprint_json: uint32 enum HIGH_BIT renders by name") @(t : T?) {
        let p = PktU32(mk = Mask32.HIGH_BIT)
        t |> equal(sprint_json(p, false), "\{\"mk\":\"HIGH_BIT\"\}")
    }
    t |> run("sprint_json: uint64 enum HIGH_BIT renders by name (top bit set)") @(t : T?) {
        let p = PktU64(w = Wide64.HIGH_BIT)
        t |> equal(sprint_json(p, false), "\{\"w\":\"HIGH_BIT\"\}")
    }
}
