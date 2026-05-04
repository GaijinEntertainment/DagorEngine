options gen2
require strings
require dastest/testing_boost public

def so_failUint(val : string) {
    try {
        debug(uint(val), "=")
        assert(false, "can't convert non uint string without exception")
    } recover {}
}

def so_failInt(val : string) {
    try {
        debug(int(val), "=")
        assert(false, "can't convert non int string without exception")
    } recover {}
}

def so_failFloat(val : string) {
    try {
        debug(float(val), "=")
        assert(false, "can't convert non float string without exception")
    } recover {}
}

def so_failDouble(val : string) {
    try {
        debug(double(val), "=")
        assert(false, "can't convert non double string without exception")
    } recover {}
}

def so_failAll(val : string) {
    so_failUint(val)
    so_failInt(val)
    so_failFloat(val)
    so_failDouble(val)
}

[test]
def test_string_ops(t : T?) {
    t |> run("case conversion and reverse") @(t : T?) {
        let s : string = "hello, world!"
        t |> equal(to_upper(s), "HELLO, WORLD!")
        t |> equal(to_lower(to_upper(s)), s)
        t |> equal(reverse(s), "!dlrow ,olleh")
        t |> equal(length(s), 13)
    }
    t |> run("find and starts/ends_with") @(t : T?) {
        let s : string = "hello, world!"
        t |> equal(find(s, "world!"), 7)
        t |> equal(find(s, "world!", 4), 7)
        t |> equal(find(s, "o"), 4)
        t |> equal(find(s, "o", 5), 8)
        t |> equal(find(s, "o", 9), -1)
        t |> equal(find(s, "??"), -1)
        t |> success(ends_with(s, "world!"))
        t |> success(starts_with(s, "hello"))
        t |> success(starts_with(s, ""))
        t |> success(ends_with(s, ""))
        t |> success(starts_with(s, "a") == false)
        t |> success(ends_with(s, "b") == false)
    }
    t |> run("strip and slice") @(t : T?) {
        let s : string = "hello, world!"
        let s2 : string = "   hello, world!   "
        t |> equal(strip(s2), s)
        t |> equal(strip_right(strip_left(s2)), s)
        t |> equal(strip_left(s2), "hello, world!   ")
        t |> equal(strip_right(s2), "   hello, world!")
        t |> equal(slice(s, 7), "world!")
        t |> equal(slice(s, -6), "world!")
        t |> equal(slice(s, 6, -6), " ")
        t |> equal(slice(s, 6, 7), " ")
    }
    t |> run("numeric conversions") @(t : T?) {
        t |> equal(int("11"), 11)
        t |> equal(int("-12"), -12)
        t |> equal(int("-12  "), -12)
        t |> equal(int("  -12"), -12)
        t |> equal(int("  -12  "), -12)
        t |> equal(uint("11"), 11u)
        t |> equal(uint("  12  "), 12u)
        t |> equal(uint("  12"), 12u)
        t |> equal(int64("11"), 11l)
        t |> equal(int64("-12"), -12l)
        t |> equal(int64("-12  "), -12l)
        t |> equal(int64("  -12"), -12l)
        t |> equal(int64("  -12  "), -12l)
        t |> equal(uint64("11"), 11ul)
        t |> equal(uint64("  12  "), 12ul)
        t |> equal(uint64("  12"), 12ul)
        t |> equal(float("-12"), -12.)
        t |> equal(float("1e1"), 10.)
        t |> equal(float("  12"), 12.)
        t |> equal(float("12   "), 12.)
        t |> equal(float("  12   "), 12.)
    }
    t |> run("fail conversions") @(t : T?) {
        so_failAll("")
        so_failAll("abc")
        t |> equal(to_int(""), 0)
        t |> equal(to_float(""), 0.)
        t |> equal(to_int("11abc"), 11)
        t |> equal(to_float("11.1abc"), float(11.1))
    }
    t |> run("escape and multiline") @(t : T?) {
        let a_slash_slash = "ab\\xy"
        t |> equal(length(a_slash_slash), 5)
        let mls = "this is
multi line string"
        t |> equal(mls, "this is\nmulti line string")
    }
    t |> run("fmt formatting") @(t : T?) {
        t |> equal(fmt(":02x", 26), "1a")
        t |> equal(fmt(":02x", 27u), "1b")
        t |> equal(fmt(":02x", 28l), "1c")
        t |> equal(fmt(":02x", 29ul), "1d")
        t |> equal(fmt(":.1f", 1.23), "1.2")
        t |> equal(fmt(":.2f", 1.234), "1.23")
        var failed = false
        try {
            var ft : string
            ft = "blah"
            var x = fmt(ft, 13)
            assert(x == "13")
        } recover {
            failed = true
        }
        t |> success(failed)
    }
    t |> run("build_string and string iterator") @(t : T?) {
        let st = build_string() <| $(var writer : StringBuilderWriter) {
            write(writer, "255 hex is ")
            write(writer, fmt(":2x", 255))
        }
        t |> equal(st, "255 hex is ff")
        var count = 0
        var index = 0
        for (ch in st) {
            if (ch == '5') {
                count ++
            }
            let chAt = character_at(st, index)
            t |> equal(ch, chAt)
            index ++
        }
        t |> equal(count, 2)
    }
    t |> run("build_string large and delete") @(t : T?) {
        var zzs = build_string() <| $(writer) {
            for (zzx in range(100500)) {
                write(writer, zzx)
            }
        }
        unsafe {
            delete_string(zzs)
        }
        t |> success(true)
    }
}
