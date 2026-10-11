options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/json_boost
require strings

def parse(t : T?; text : string) : JsonValue? {
    var error : string
    var js = read_json(text, error)
    t |> success(js != null, "parsed {text}")
    t |> equal(error, "")
    return js
}

def repeat_digit(digit : int; count : int) : string {
    return build_string() $(var w) {
        for (_ in range(count)) {
            write_char(w, digit)
        }
    }
}

[test]
def test_integers_within_range(t : T?) {
    t |> run("int64 max") @(t : T?) {
        var js = parse(t, "9223372036854775807")
        t |> equal(js is _longint, true)
        t |> equal(js as _longint, 9223372036854775807l)
    }
    t |> run("int64 min") @(t : T?) {
        var js = parse(t, "-9223372036854775808")
        t |> equal(js is _longint, true)
        t |> equal(js as _longint, -9223372036854775807l - 1l)
    }
    t |> run("nineteen digits below the limit") @(t : T?) {
        var js = parse(t, "9223372036854775806")
        t |> equal(js is _longint, true)
        t |> equal(js as _longint, 9223372036854775806l)
    }
    t |> run("leading zeros do not push past the limit") @(t : T?) {
        var js = parse(t, "000000000000000000000042")
        t |> equal(js is _longint, true)
        t |> equal(js as _longint, 42l)
    }
    t |> run("zero") @(t : T?) {
        var js = parse(t, "0")
        t |> equal(js is _longint, true)
        t |> equal(js as _longint, 0l)
    }
    t |> run("negative zero") @(t : T?) {
        var js = parse(t, "-0")
        t |> equal(js is _longint, true)
        t |> equal(js as _longint, 0l)
    }
}

[test]
def test_integers_out_of_range_become_numbers(t : T?) {
    t |> run("int64 max plus one") @(t : T?) {
        var js = parse(t, "9223372036854775808")
        t |> equal(js is _number, true)
        t |> success(js as _number > 9.223e18lf, "magnitude and sign are preserved")
    }
    t |> run("uint64 max") @(t : T?) {
        var js = parse(t, "18446744073709551615")
        t |> equal(js is _number, true)
        t |> success(js as _number > 1.8e19lf, "magnitude and sign are preserved")
    }
    t |> run("int64 min minus one") @(t : T?) {
        var js = parse(t, "-9223372036854775809")
        t |> equal(js is _number, true)
        t |> success(js as _number < -9.223e18lf, "magnitude and sign are preserved")
    }
    t |> run("far past uint64") @(t : T?) {
        var js = parse(t, "99999999999999999999999999")
        t |> equal(js is _number, true)
        t |> success(js as _number > 9.9e25lf, "magnitude and sign are preserved")
    }
    t |> run("out of range inside a document") @(t : T?) {
        var js = parse(t, "\{ \"id\" : 18446744073709551615 \}")
        t |> equal(js?["id"] is _number, true)
    }
}

[test]
def test_integers_past_double_range_error(t : T?) {
    t |> run("four hundred digits is a parse error, not a panic") @(t : T?) {
        var error : string
        var js = read_json(repeat_digit('9', 400), error)
        t |> equal(js == null, true)
        t |> success(!empty(error), "the lexer reports the out-of-range number")
    }
    t |> run("negative four hundred digits") @(t : T?) {
        var error : string
        var js = read_json("-" + repeat_digit('9', 400), error)
        t |> equal(js == null, true)
        t |> success(!empty(error), "the lexer reports the out-of-range number")
    }
}

def expect_number_error(t : T?; text : string) {
    var error : string
    var js = read_json(text, error)
    t |> success(js == null, "{text} is rejected")
    t |> success(!empty(error), "{text} reports an error instead of panicking")
}

def expect_number_error_matching(t : T?; text, fragment : string) {
    var error : string
    var js = read_json(text, error)
    t |> success(js == null, "{text} is rejected")
    t |> success(error |> find(fragment) >= 0, "{text}: got <{error}>")
}

[test]
def test_floats_past_double_range_error(t : T?) {
    t |> run("exponent overflow") @(t : T?) {
        expect_number_error(t, "1e400")
        expect_number_error(t, "-1e999")
        expect_number_error(t, "2.5e308")
    }
    t |> run("exponent underflow") @(t : T?) {
        expect_number_error(t, "1e-400")
    }
    t |> run("inside a document") @(t : T?) {
        expect_number_error(t, "\{ \"ratio\" : 1e400 \}")
        expect_number_error(t, "[1, 1e400]")
    }
}

[test]
def test_floats_within_range_still_parse(t : T?) {
    t |> run("ordinary and extreme representable values") @(t : T?) {
        for (text in ["3.14", "-0.5", "1.5e10", "2.5E-3", "1.7e308", "-1.7e308", "1e-320", "0.0"]) {
            var js = parse(t, text)
            t |> success(js is _number, "{text} is a number")
        }
    }
}

[test]
def test_number_error_messages_distinguish_invalid_from_range(t : T?) {
    t |> run("truncated exponents are invalid, not out of range") @(t : T?) {
        for (text in ["1e", "1e+", "1.5e-", "[1e]"]) {
            expect_number_error_matching(t, text, "is invalid")
        }
    }
    t |> run("true overflow stays out of range") @(t : T?) {
        for (text in ["1e400", "1e-400", "2.5e308"]) {
            expect_number_error_matching(t, text, "is out of range")
        }
    }
}

[test]
def test_leading_plus_is_not_a_number(t : T?) {
    t |> run("RFC 8259 has no leading plus") @(t : T?) {
        expect_number_error(t, "+5")
        expect_number_error(t, "+3.14")
        expect_number_error(t, "[+1]")
    }
}
