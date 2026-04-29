options gen2
require dastest/testing_boost public
require daslib/bitfield_boost

def test_any_bitfield(t : T?; bf : auto(BitFieldType)) {
    // test [] and []=
    var b : BitFieldType
    for (i in range(typeinfo sizeof(bf) << 3)) {
        t |> equal(b[i], false, "initial bit {i} should be false")
    }
    for (i in range(typeinfo sizeof(bf) << 3)) {
        b[i] = true
        t |> equal(b[i], true, "bit {i} should be true after setting to true")
    }
    // set it to iterating and check 'each'
    for (i in range(typeinfo sizeof(bf) << 3)) {
        b[i] = (i & 1) == 0
    }
    for (i, bit in count(), each_bit(b)) {
        let expected = (i & 1) == 0
        t |> equal(bit, expected, "bit {i} should be {expected} after setting via iteration")
    }
    // test []&&= operator
    for (i in range(typeinfo sizeof(bf) << 3)) {
        b[i] &&= false
        t |> equal(b[i], false, "bit {i} should be false after &&= false")
    }
    for (i in range(typeinfo sizeof(bf) << 3)) {
        b[i] = true
        b[i] &&= true
        t |> equal(b[i], true, "bit {i} should be true after &&= true")
    }
    // test []||= operator
    for (i in range(typeinfo sizeof(bf) << 3)) {
        b[i] ||= true
        t |> equal(b[i], true, "bit {i} should be true after ||= true")
    }
    for (i in range(typeinfo sizeof(bf) << 3)) {
        b[i] = false
        b[i] ||= false
        t |> equal(b[i], false, "bit {i} should be false after ||= false")
    }
    // test []^^= operator
    for (i in range(typeinfo sizeof(bf) << 3)) {
        b[i] = false
        b[i] ^^= true
        t |> equal(b[i], true, "bit {i} should be true after ^^= true")
    }
}

[test]
def test_bitfield_boost(t : T?) {
    t |> run("bitfield") @@(t : T?) {
        test_any_bitfield(t, type<bitfield>)
    }
    t |> run("bitfield8") @@(t : T?) {
        test_any_bitfield(t, type<bitfield8>)
    }
    t |> run("bitfield16") @@(t : T?) {
        test_any_bitfield(t, type<bitfield16>)
    }
    t |> run("bitfield64") @@(t : T?) {
        test_any_bitfield(t, type<bitfield64>)
    }
}
