options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/strings_convert

def expect_ok_value(t : T?; r : auto(R) const; expected : auto(V)) {
    t |> equal(true, is_ok(r))
    if (is_ok(r)) {
        t |> equal(expected, unwrap(r))
    }
}

def expect_err_kind(t : T?; r : auto(R) const; expected : ConversionError) {
    t |> equal(false, is_ok(r))
    if (is_err(r)) {
        t |> equal(expected, unwrap_err(r))
    }
}

[test]
def test_try_to_int_basic(t : T?) {
    expect_ok_value(t, try_to_int("13"), 13)
    expect_ok_value(t, try_to_int("-13"), -13)
    expect_ok_value(t, try_to_int("0"), 0)
    expect_ok_value(t, try_to_int("  13"), 13)
    expect_err_kind(t, try_to_int("13  "), ConversionError.trailing_garbage)
    expect_err_kind(t, try_to_int("13abc"), ConversionError.trailing_garbage)
    expect_err_kind(t, try_to_int(""), ConversionError.invalid_argument)
    expect_err_kind(t, try_to_int("   "), ConversionError.invalid_argument)
    expect_err_kind(t, try_to_int("abc"), ConversionError.invalid_argument)
    expect_err_kind(t, try_to_int("1.5"), ConversionError.trailing_garbage)
    expect_err_kind(t, try_to_int("9999999999999999999"), ConversionError.out_of_range)
}

[test]
def test_try_to_int_hex(t : T?) {
    expect_ok_value(t, try_to_int("13", true), int(0x13))
    expect_ok_value(t, try_to_int("0x1F", true), int(0x1F))
    expect_ok_value(t, try_to_int("FF", true), int(0xFF))
    expect_err_kind(t, try_to_int("0x1F", false), ConversionError.trailing_garbage)
    expect_err_kind(t, try_to_int("xyz", true), ConversionError.invalid_argument)
}

[test]
def test_try_to_int8(t : T?) {
    expect_ok_value(t, try_to_int8("13"), int8(13))
    expect_ok_value(t, try_to_int8("-128"), int8(-128))
    expect_ok_value(t, try_to_int8("127"), int8(127))
    expect_err_kind(t, try_to_int8("128"), ConversionError.out_of_range)
    expect_err_kind(t, try_to_int8(""), ConversionError.invalid_argument)
    expect_err_kind(t, try_to_int8("13x"), ConversionError.trailing_garbage)
}

[test]
def test_try_to_uint8(t : T?) {
    expect_ok_value(t, try_to_uint8("13"), uint8(13))
    expect_ok_value(t, try_to_uint8("255"), uint8(255))
    expect_err_kind(t, try_to_uint8("256"), ConversionError.out_of_range)
    expect_err_kind(t, try_to_uint8("-1"), ConversionError.invalid_argument)
}

[test]
def test_try_to_int16(t : T?) {
    expect_ok_value(t, try_to_int16("13"), int16(13))
    expect_ok_value(t, try_to_int16("-32768"), int16(-32768))
    expect_ok_value(t, try_to_int16("32767"), int16(32767))
    expect_err_kind(t, try_to_int16("32768"), ConversionError.out_of_range)
}

[test]
def test_try_to_uint16(t : T?) {
    expect_ok_value(t, try_to_uint16("13"), uint16(13))
    expect_ok_value(t, try_to_uint16("65535"), uint16(65535))
    expect_err_kind(t, try_to_uint16("65536"), ConversionError.out_of_range)
}

[test]
def test_try_to_uint(t : T?) {
    expect_ok_value(t, try_to_uint("13"), 13u)
    expect_ok_value(t, try_to_uint("4294967295"), 4294967295u)
    expect_err_kind(t, try_to_uint("4294967296"), ConversionError.out_of_range)
    expect_err_kind(t, try_to_uint("-1"), ConversionError.invalid_argument)
}

[test]
def test_try_to_int64(t : T?) {
    expect_ok_value(t, try_to_int64("12345678999"), 12345678999l)
    expect_ok_value(t, try_to_int64("-12345678999"), -12345678999l)
    expect_err_kind(t, try_to_int64("99999999999999999999"), ConversionError.out_of_range)
}

[test]
def test_try_to_uint64(t : T?) {
    expect_ok_value(t, try_to_uint64("12345678999"), 12345678999ul)
    expect_ok_value(t, try_to_uint64("18446744073709551615"), 18446744073709551615ul)
    expect_err_kind(t, try_to_uint64("18446744073709551616"), ConversionError.out_of_range)
    expect_err_kind(t, try_to_uint64("-1"), ConversionError.invalid_argument)
}

[test]
def test_try_to_float_basic(t : T?) {
    expect_ok_value(t, try_to_float("13"), 13.f)
    expect_ok_value(t, try_to_float("13.5"), 13.5f)
    expect_ok_value(t, try_to_float("-13."), -13.f)
    expect_ok_value(t, try_to_float("  -13.54"), -13.54f)
    expect_err_kind(t, try_to_float(""), ConversionError.invalid_argument)
    expect_err_kind(t, try_to_float("abc"), ConversionError.invalid_argument)
    expect_err_kind(t, try_to_float("13x"), ConversionError.trailing_garbage)
    expect_err_kind(t, try_to_float("13.5  "), ConversionError.trailing_garbage)
    expect_err_kind(t, try_to_float("1e39"), ConversionError.out_of_range)
}

[test]
def test_try_to_double_basic(t : T?) {
    expect_ok_value(t, try_to_double("13"), 13.lf)
    expect_ok_value(t, try_to_double("13.54"), 13.54lf)
    expect_ok_value(t, try_to_double("-13.54"), -13.54lf)
    expect_err_kind(t, try_to_double(""), ConversionError.invalid_argument)
    expect_err_kind(t, try_to_double("13x"), ConversionError.trailing_garbage)
    expect_err_kind(t, try_to_double("1e309"), ConversionError.out_of_range)
}

[test]
def test_try_to_round_trip(t : T?) {
    let i8 = int8(-42)
    expect_ok_value(t, try_to_int8(string(i8)), i8)
    let u8 = uint8(200)
    expect_ok_value(t, try_to_uint8(string(u8)), u8)
    let i16 = int16(-12345)
    expect_ok_value(t, try_to_int16(string(i16)), i16)
    let u16 = uint16(54321)
    expect_ok_value(t, try_to_uint16(string(u16)), u16)
    let i32 = -1234567
    expect_ok_value(t, try_to_int(string(i32)), i32)
    let u32 = 1234567u
    expect_ok_value(t, try_to_uint(string(u32)), u32)
    let i64 = -123456789012345l
    expect_ok_value(t, try_to_int64(string(i64)), i64)
    let u64 = 123456789012345ul
    expect_ok_value(t, try_to_uint64(string(u64)), u64)
    let f = 3.5f
    expect_ok_value(t, try_to_float(string(f)), f)
    let d = 3.5lf
    expect_ok_value(t, try_to_double(string(d)), d)
}
