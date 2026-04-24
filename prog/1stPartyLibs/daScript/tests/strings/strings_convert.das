options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer
require daslib/regex

def test_conversions(t : T?; good_string : string; good_result : auto(TT); out_of_range_string : string) {
    var errC : ConversionResult
    var ofs : int

    var res = TT(good_string, errC, ofs)// as is
    t |> equal(good_result, res)
    t |> equal(ConversionResult.ok, errC)
    t |> equal(length(good_string), ofs)

    res = TT("  {good_string}  ", errC, ofs) // with spaces
    t |> equal(good_result, res)
    t |> equal(ConversionResult.ok, errC)
    t |> equal(length(good_string) + 2, ofs)

    res = TT("", errC, ofs) // empty string
    t |> equal(TT(), res)
    t |> equal(ConversionResult.invalid_argument, errC)
    t |> equal(0, ofs)

    res = TT("random goo", errC, ofs) // not a number
    t |> equal(TT(), res)
    t |> equal(ConversionResult.invalid_argument, errC)
    t |> equal(0, ofs)

    res = TT(out_of_range_string, errC, ofs) // out of range
    t |> equal(TT(), res)
    t |> equal(ConversionResult.out_of_range, errC)
    t |> equal(0, ofs)
}

def test_hex_conversions(t : T?; good_string : string; good_result : auto(TT); out_of_range_string : string) {
    var errC : ConversionResult
    var ofs : int

    var res = TT(good_string, errC, ofs, true)// as is
    t |> equal(good_result, res)
    t |> equal(ConversionResult.ok, errC)
    t |> equal(length(good_string), ofs)

    res = TT("  {good_string}  ", errC, ofs, true)// with spaces
    t |> equal(good_result, res)
    t |> equal(ConversionResult.ok, errC)
    t |> equal(length(good_string) + 2, ofs)

    res = TT("", errC, ofs, true)// empty string
    t |> equal(TT(), res)
    t |> equal(ConversionResult.invalid_argument, errC)
    t |> equal(0, ofs)

    res = TT("random goo", errC, ofs, true)// not a number
    t |> equal(TT(), res)
    t |> equal(ConversionResult.invalid_argument, errC)
    t |> equal(0, ofs)

    res = TT(out_of_range_string, errC, ofs, true)// out of range
    t |> equal(TT(), res)
    t |> equal(ConversionResult.out_of_range, errC)
    t |> equal(0, ofs)
}


[test]
def test_convert_with_errors(t : T?) {
    test_conversions(t, "13", int8(13), "128")
    test_hex_conversions(t, "13", int8(0x13), "0x100")

    test_conversions(t, "13", uint8(13), "256")
    test_hex_conversions(t, "13", uint8(0x13), "0x100")

    test_conversions(t, "13", int16(13), "32768")
    test_hex_conversions(t, "13", int16(0x13), "0x10000")

    test_conversions(t, "13", uint16(13), "65536")
    test_hex_conversions(t, "13", uint16(0x13), "0x10000")

    test_conversions(t, "13", int(13), "2147483648")
    test_hex_conversions(t, "13", int(0x13), "0x100000000")

    test_conversions(t, "13", uint(13), "4294967296")
    test_hex_conversions(t, "13", uint(0x13), "0x100000000")

    test_conversions(t, "13", int64(13), "9223372036854775808")
    test_hex_conversions(t, "13", int64(0x13), "0x10000000000000000")

    test_conversions(t, "13", uint64(13), "18446744073709551616")
    test_hex_conversions(t, "13", uint64(0x13), "0x10000000000000000")

    test_conversions(t, "13", float(13.), "1e39")
    test_conversions(t, "13", double(13.lf), "1e309")
}

[test]
def test_convert(t : T?) {

    let a8 <- array<uint8>(uint8('A'), uint8('B'), uint8('C'))
    t |> equal(string(a8), "ABC")
    t |> run("fuzz string from array") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let len = fake |> random_uint % 10u
            var arr <- [for (q in urange(len)); uint8(fake |> random_int % 256)]
            t |> accept(string(arr))
            delete arr
        }
    }

    t |> equal(to_char('A'), "A")
    t |> run("fuzz to_char") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(to_char(fake |> random_int))
        }
    }

    t |> equal(int("13"), 13)
    t |> run("fuzz int") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(int(fake |> any_int))
        }
        fuzz() {
            t |> accept(int(fake |> any_string))
        }
    }

    t |> equal(int64("12345678999"), 12345678999l)
    t |> equal(int64("-123456789999"), -123456789999l)
    t |> run("fuzz int64") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(int64(fake |> any_int))
        }
        fuzz() {
            t |> accept(int64(fake |> any_string))
        }
    }

    t |> equal(uint("13"), 13u)
    t |> run("fuzz uint") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(uint(fake |> any_uint))
        }
        fuzz() {
            t |> accept(uint(fake |> any_string))
        }
    }

    t |> equal(uint64("12345678999"), 12345678999ul)
    t |> run("fuzz uint64") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(uint64(fake |> any_int))
        }
        fuzz() {
            t |> accept(uint64(fake |> any_string))
        }
    }

    t |> equal(float("13"), 13.)
    t |> equal(float("13."), 13.)
    t |> equal(float("13.54"), 13.54)
    t |> equal(float("-13"), -13.)
    t |> equal(float("-13."), -13.)
    t |> equal(float("-13.54"), -13.54)
    t |> run("fuzz float") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(float(fake |> any_float))
        }
        fuzz() {
            t |> accept(float(fake |> any_string))
        }
    }

    t |> equal(double("13"), 13.lf)
    t |> equal(double("13."), 13.lf)
    t |> equal(double("13.54"), 13.54lf)
    t |> equal(double("-13"), -13.lf)
    t |> equal(double("-13."), -13.lf)
    t |> equal(double("-13.54"), -13.54lf)
    t |> run("fuzz double") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(double(fake |> any_float))
        }
        fuzz() {
            t |> accept(double(fake |> any_string))
        }
    }

    t |> equal(to_int("13", true), 19)
    t |> run("fuzz to_int") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(to_int(fake |> any_hex, true))
        }
        fuzz() {
            t |> accept(to_int(fake |> any_string, true))
        }
    }

    t |> equal(to_uint("13", true), 0x13)
    t |> run("fuzz to_uint") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(to_uint(fake |> any_hex, true))
        }
        fuzz() {
            t |> accept(to_uint(fake |> any_string, true))
        }
    }

    t |> equal(to_int64("13", true), 19l)
    t |> run("fuzz to_int64") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(to_int64(fake |> any_hex, true))
        }
        fuzz() {
            t |> accept(to_int64(fake |> any_string, true))
        }
    }

    t |> equal(to_uint64("13", true), 0x13ul)
    t |> run("fuzz to_uint") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(to_uint64(fake |> any_hex, true))
        }
        fuzz() {
            t |> accept(to_uint64(fake |> any_string, true))
        }
    }

    t |> equal(to_float("13"), 13.)
    t |> equal(to_float("13."), 13.)
    t |> equal(to_float("13.54"), 13.54)
    t |> equal(to_float("-13"), -13.)
    t |> equal(to_float("-13."), -13.)
    t |> equal(to_float("-13.54"), -13.54)
    t |> run("fuzz float") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(to_float(fake |> any_float))
        }
        fuzz() {
            t |> accept(to_float(fake |> any_string))
        }
    }

    t |> equal(to_double("13"), 13.lf)
    t |> equal(to_double("13."), 13.lf)
    t |> equal(to_double("13.54"), 13.54lf)
    t |> equal(to_double("-13"), -13.lf)
    t |> equal(to_double("-13."), -13.lf)
    t |> equal(to_double("-13.54"), -13.54lf)
    t |> run("fuzz double") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            t |> accept(to_double(fake |> any_float))
        }
        fuzz() {
            t |> accept(to_double(fake |> any_string))
        }
    }
}
