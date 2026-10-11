options gen2
require dastest/testing_boost
require strings

// Parity property harness for the byte-view string surface.
// Every view extern that has a string counterpart is checked as
//     twin(d, args) == counterpart(string(d), args)
// over NUL-free buffers x an index / needle / offset / cmpLen lattice.
//
// Interior-NUL behaviour is deliberately NOT here: that is where the two surfaces must
// differ by design, and it is pinned in view_binary_safe.das.
//
// Every buffer is assembled at runtime. With all-constant arguments the optimizer folds the
// whole call away, and the property would compare the const-folder against itself.
//
// The view_size INT_MAX guard (view_size in module_builtin_string.cpp) has no runtime test
// here - tripping it needs a >2GB array. It holds by inspection: every view entry sizes
// itself through view_size, which throws before the uint32 truncation can happen.

struct private Case {
    name : string
    bytes : array<uint8>
}

def private add_text(var cases : array<Case>; name, text : string) {
    var b <- to_bytes(text)
    cases |> emplace(Case(name = name, bytes <- b))
}

// high bytes cannot be written as das literals, so they arrive as raw byte values
def private add_raw(var cases : array<Case>; name, head : string; mid : array<int>; tail : string) {
    var b <- to_bytes(head)
    b |> reserve(length(b) + length(mid))
    for (v in mid) {
        b |> push(uint8(v))
    }
    b |> push_from(to_bytes(tail))
    cases |> emplace(Case(name = name, bytes <- b))
}

def private text_cases() : array<Case> {
    var cases : array<Case>
    add_text(cases, "empty", "")
    add_text(cases, "one", "a")
    add_text(cases, "plain", "abcdef")
    add_text(cases, "repeat", "abcabc")
    add_text(cases, "padded", "  ab x ")
    add_text(cases, "spaces", "   ")
    add_text(cases, "all ws", " \t\n\r\f\v")
    add_text(cases, "edge x", "xxabcxx")
    add_text(cases, "ws inside", "a \t b")
    add_raw(cases, "high mid", "ab", [195, 169], "cd")
    add_raw(cases, "high only", "", [255, 254, 128], "")
    add_raw(cases, "high and ws", " ", [200, 9], " ")
    return <- cases
}

def private number_cases() : array<Case> {
    var cases : array<Case>
    add_text(cases, "empty", "")
    add_text(cases, "spaces", "   ")
    add_text(cases, "ws 6set", " \t\n\r\f\v")
    add_text(cases, "int", "123")
    add_text(cases, "lead ws int", "  42xyz")
    add_text(cases, "neg", "-7")
    add_text(cases, "plus", "+5")
    add_text(cases, "garbage", "abc")
    add_text(cases, "lead ws garbage", "  xyz")
    add_text(cases, "overflow", "99999999999999")
    add_text(cases, "byte overflow", "200")
    add_text(cases, "u16 max", "65535")
    add_text(cases, "i64 max", "9223372036854775807")
    add_text(cases, "u64 max", "18446744073709551615")
    add_text(cases, "hex prefixed", "0x1F")
    add_text(cases, "hex bare", "1F")
    add_text(cases, "hex prefix only", "0x")
    add_text(cases, "real", " 2.5e1zz")
    add_text(cases, "real dot", ".5")
    add_text(cases, "real trailing dot", "1.")
    add_text(cases, "real overflow", "1e400")
    add_text(cases, "csv", "1,2,3,4")
    add_raw(cases, "high prefix", "", [200], "42")
    return <- cases
}

// negative / in-range / at-the-boundary / overshooting, for every index-like argument
def private idx_lattice(n : int) : array<int> {
    return <- [-100, -2, -1, 0, 1, 3, n - 1, n, n + 1, 100]
}

def private high_needle() : string {
    return build_string() $(var w) {
        w |> write_char(195)
    }
}

// generic needles plus needles cut from the case itself, so every case gets a needle that
// sits exactly at its start boundary, exactly at its end boundary, and one byte too long
def private needles_for(s : string) : array<string> {
    return <- [
        "", "a", "bc", "zz",
        s, s + "d",
        slice(s, 0, 1), slice(s, -1), slice(s, -2),
        high_needle()
    ]
}

def private check_slice(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    var inscope idx <- idx_lattice(length(d))
    for (a in idx) {
        t |> equal(slice(d, a), slice(s, a), "slice({name},{a})")  // nolint:PERF031 - the parity oracle calls the string counterpart per iteration by design
        for (b in idx) {
            t |> equal(slice(d, a, b), slice(s, a, b), "slice({name},{a},{b})")  // nolint:PERF031 - the parity oracle calls the string counterpart per iteration by design
        }
    }
}

def private check_chop(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    let n = length(d)
    var inscope idx <- idx_lattice(n)
    var inscope lens <- [-1, 0, 1, 3, n, n + 1, 100]
    for (a in idx) {
        for (l in lens) {
            t |> equal(chop(d, a, l), chop(s, a, l), "chop({name},{a},{l})")  // nolint:PERF031 - the parity oracle calls the string counterpart per iteration by design
        }
    }
}

def private check_find(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    var inscope needles <- needles_for(s)
    var inscope starts <- idx_lattice(length(d))
    for (nd in needles) {
        t |> equal(find(d, nd), find(s, nd), "find({name},[{nd}])")
        t |> equal(rfind(d, nd), rfind(s, nd), "rfind({name},[{nd}])")
        for (st in starts) {
            t |> equal(find(d, nd, st), find(s, nd, st), "find({name},[{nd}],{st})")
            t |> equal(rfind(d, nd, st), rfind(s, nd, st), "rfind({name},[{nd}],{st})")
        }
    }
}

def private check_find_char(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    // -61 is byte 0xC3 read as a signed char, 195 is the same byte unsigned: both sides match
    // the unsigned reading, so -61 must miss and 195 must hit
    var inscope chars <- [-61, -1, 0, 9, 32, 'a', 'c', 'x', 'z', 128, 195, 200, 255, 256, 1000]
    var inscope starts <- idx_lattice(length(d))
    for (ch in chars) {
        t |> equal(find(d, ch), find(s, ch), "find_char({name},{ch})")
        for (st in starts) {
            t |> equal(find(d, ch, st), find(s, ch, st), "find_char({name},{ch},{st})")
        }
    }
}

def private check_starts_with(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    var inscope needles <- needles_for(s)
    var inscope lens <- [0u, 1u, 3u, 100u]
    for (cmp in needles) {
        t |> equal(starts_with(d, cmp), starts_with(s, cmp), "starts_with({name},[{cmp}])")
        t |> equal(ends_with(d, cmp), ends_with(s, cmp), "ends_with({name},[{cmp}])")
        for (cl in lens) {
            t |> equal(starts_with(d, cmp, cl), starts_with(s, cmp, cl), "starts_with({name},[{cmp}],{cl})")
        }
    }
}

def private check_starts_with_at(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    var inscope needles <- needles_for(s)
    var inscope offs <- idx_lattice(length(d))
    var inscope lens <- [0u, 1u, 3u, 100u]
    for (cmp in needles) {
        for (off in offs) {
            t |> equal(starts_with(d, off, cmp), starts_with(s, off, cmp), "starts_with({name},{off},[{cmp}])")
            for (cl in lens) {
                t |> equal(starts_with(d, off, cmp, cl), starts_with(s, off, cmp, cl),
                    "starts_with({name},{off},[{cmp}],{cl})")
            }
        }
    }
}

def private check_strip_trim(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    t |> equal(strip(d), strip(s), "strip({name})")
    t |> equal(strip_left(d), strip_left(s), "strip_left({name})")
    t |> equal(strip_right(d), strip_right(s), "strip_right({name})")
    t |> equal(trim(d), trim(s), "trim({name})")
    t |> equal(ltrim(d), ltrim(s), "ltrim({name})")
    t |> equal(rtrim(d), rtrim(s), "rtrim({name})")
    var inscope sets <- ["", "x", "xy", " ", "abc", " \t\n\r\f\v"]
    for (cs in sets) {
        t |> equal(rtrim(d, cs), rtrim(s, cs), "rtrim({name},[{cs}])")
    }
}

def private check_skip_white_space(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    var inscope offs <- idx_lattice(length(d))
    for (off in offs) {
        t |> equal(skip_white_space(d, off), skip_white_space(s, off), "skip_white_space({name},{off})")
    }
}

// write_string has no string counterpart to compare against; its oracle is slice, which the
// checks above have already tied to the string surface
def private check_write(t : T?; name : string; d : array<uint8>) {
    let whole = build_string() $(var w) {
        w |> write_string(d)
    }
    t |> equal(whole, string(d), "write_string({name}) == string(d)")
    var inscope idx <- idx_lattice(length(d))
    for (a in idx) {
        for (b in idx) {
            let part = build_string() $(var w) {
                w |> write_string(d, a, b)
            }
            t |> equal(part, slice(d, a, b), "write_string({name},{a},{b}) == slice")
        }
    }
}

// to_bytes has no string counterpart either - it is the materializing inverse of string(d),
// so its oracle is the round trip: over NUL-free bytes it must reproduce the view exactly
def private check_to_bytes(t : T?; name : string; d : array<uint8>) {
    let s = string(d)
    var inscope b <- to_bytes(s)
    t |> equal(length(b), length(d), "to_bytes({name}) length")
    var same = length(b) == length(d)
    for (x, y in b, d) {
        same = same && x == y
    }
    t |> success(same, "to_bytes({name}) reproduces the view byte for byte")
    t |> equal(string(b), s, "string(to_bytes({name})) round-trips")
}

[test]
def test_parity_slice_and_chop(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_slice(t, c.name, c.bytes)
        check_chop(t, c.name, c.bytes)
    }
}

[test]
def test_parity_find(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_find(t, c.name, c.bytes)
    }
}

[test]
def test_parity_find_char(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_find_char(t, c.name, c.bytes)
    }
}

[test]
def test_parity_starts_and_ends_with(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_starts_with(t, c.name, c.bytes)
        check_starts_with_at(t, c.name, c.bytes)
    }
}

[test]
def test_parity_strip_and_trim(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_strip_trim(t, c.name, c.bytes)
    }
}

[test]
def test_parity_skip_white_space(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_skip_white_space(t, c.name, c.bytes)
    }
}

[test]
def test_parity_write_string(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_write(t, c.name, c.bytes)
    }
}

[test]
def test_parity_to_bytes(t : T?) {
    var inscope cases <- text_cases()
    for (c in cases) {
        check_to_bytes(t, c.name, c.bytes)
    }
}

// ===== parse family =====
// The view forms take offset in AND out; the string forms only write it. Parity is therefore
// at cursor 0: preset the view offset to 0 and compare the value / result / offset triple.
// The value is compared stringified so that one comparison shape serves all ten widths.

def private cmp_parse(t : T?; what, av, bv : string; acr, bcr : ConversionResult; aoff, boff : int) {
    t |> equal(av, bv, "{what} value")
    t |> equal(acr, bcr, "{what} result")
    t |> equal(aoff, boff, "{what} offset")
}

def private par_int8(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = int8(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = int8(s, bcr, boff, hex)
    cmp_parse(t, "int8({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_uint8(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = uint8(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = uint8(s, bcr, boff, hex)
    cmp_parse(t, "uint8({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_int16(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = int16(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = int16(s, bcr, boff, hex)
    cmp_parse(t, "int16({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_uint16(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = uint16(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = uint16(s, bcr, boff, hex)
    cmp_parse(t, "uint16({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_int(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = int(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = int(s, bcr, boff, hex)
    cmp_parse(t, "int({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_uint(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = uint(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = uint(s, bcr, boff, hex)
    cmp_parse(t, "uint({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_int64(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = int64(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = int64(s, bcr, boff, hex)
    cmp_parse(t, "int64({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_uint64(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = uint64(d, acr, aoff, hex)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = uint64(s, bcr, boff, hex)
    cmp_parse(t, "uint64({name},hex={hex})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_float(t : T?; name : string; d : array<uint8>; s : string) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = float(d, acr, aoff)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = float(s, bcr, boff)
    cmp_parse(t, "float({name})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private par_double(t : T?; name : string; d : array<uint8>; s : string) {
    var acr = ConversionResult.ok
    var aoff = 0
    let av = double(d, acr, aoff)
    var bcr = ConversionResult.ok
    var boff = 0
    let bv = double(s, bcr, boff)
    cmp_parse(t, "double({name})", "{av}", "{bv}", acr, bcr, aoff, boff)
}

def private check_parse_ints(t : T?; name : string; d : array<uint8>; s : string; hex : bool) {
    par_int8(t, name, d, s, hex)
    par_uint8(t, name, d, s, hex)
    par_int16(t, name, d, s, hex)
    par_uint16(t, name, d, s, hex)
    par_int(t, name, d, s, hex)
    par_uint(t, name, d, s, hex)
    par_int64(t, name, d, s, hex)
    par_uint64(t, name, d, s, hex)
}

[test]
def test_parity_parse_ints(t : T?) {
    var inscope cases <- number_cases()
    var inscope hexes <- [false, true]
    for (c in cases) {
        let s = string(c.bytes)
        for (hex in hexes) {
            check_parse_ints(t, c.name, c.bytes, s, hex)
        }
    }
}

[test]
def test_parity_parse_reals(t : T?) {
    var inscope cases <- number_cases()
    for (c in cases) {
        let s = string(c.bytes)
        par_float(t, c.name, c.bytes, s)
        par_double(t, c.name, c.bytes, s)
    }
}
