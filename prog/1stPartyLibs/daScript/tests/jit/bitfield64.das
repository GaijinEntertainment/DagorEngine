options gen2
options indenting = 4

require dastest/testing_boost

bitfield TestFlags64 : uint64 {
    lo
    mid
    hi
}

def get_bitfield64_value() : TestFlags64 {
    return bitfield64(1ul << 63ul)
}

def pass_and_return_bitfield64(f : TestFlags64) : TestFlags64 {
    return f
}

def bitfield64_to_uint64(f : TestFlags64) : uint64 {
    return uint64(f)
}

[test]
def test_bitfield64_high_bit(t : T?) {
    t |> run("bitfield64 high bit constant") @(t : T?) {
        let f = bitfield64(1ul << 63ul)
        t |> equal(uint64(f), 0x8000000000000000ul)
    }
}

[test]
def test_bitfield64_pass_through(t : T?) {
    t |> run("bitfield64 pass through function") @(t : T?) {
        let f = get_bitfield64_value()
        t |> equal(uint64(f), 0x8000000000000000ul)
    }
}

[test]
def test_bitfield64_roundtrip(t : T?) {
    t |> run("bitfield64 roundtrip via function call") @(t : T?) {
        let f = pass_and_return_bitfield64(bitfield64(1ul << 63ul))
        t |> equal(uint64(f), 0x8000000000000000ul)
    }
}

[test]
def test_bitfield64_cast(t : T?) {
    t |> run("bitfield64 to uint64 via function") @(t : T?) {
        let v = bitfield64_to_uint64(bitfield64(1ul << 63ul))
        t |> equal(v, 0x8000000000000000ul)
    }
}

[test]
def test_bitfield64_all_bits(t : T?) {
    t |> run("bitfield64 all named bits") @(t : T?) {
        let f = TestFlags64.lo | TestFlags64.mid | TestFlags64.hi
        t |> equal(uint64(f), 7ul)
    }
}

[test]
def test_bitfield64_var_assign(t : T?) {
    t |> run("bitfield64 var assign high bit") @(t : T?) {
        var f : TestFlags64
        f = bitfield64(1ul << 63ul)
        t |> equal(uint64(f), 0x8000000000000000ul)
    }
}

[test]
def test_bitfield64_orassign(t : T?) {
    t |> run("bitfield64 |= high bit") @(t : T?) {
        var f = TestFlags64.lo
        f |= bitfield64(1ul << 63ul)
        t |> equal(uint64(f), 0x8000000000000001ul)
    }
}
