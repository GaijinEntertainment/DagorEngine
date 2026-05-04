options gen2
options persistent_heap
options no_unused_block_arguments = false
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/json_boost
require daslib/strings_boost

def parse(json_str : string; t : T?) : JsonValue? {
    var error : string
    var js = read_json(json_str, error)
    t |> success(js != null)
    t |> equal(error, "")
    return js
}

def parse_expect_fail(json_str : string; t : T?) {
    var error : string
    var js = read_json(json_str, error)
    t |> equal(js == null, true)
    t |> equal(error != "", true)
}

// ============================================================
//  Parsing: scalars
// ============================================================

[test]
def test_parse_scalars(t : T?) {
    t |> run("null") @(t : T?) {
        var js = parse("null", t)
        t |> equal(js is _null, true)
    }
    t |> run("true") @(t : T?) {
        var js = parse("true", t)
        t |> equal(js is _bool, true)
        t |> equal(js as _bool, true)
    }
    t |> run("false") @(t : T?) {
        var js = parse("false", t)
        t |> equal(js is _bool, true)
        t |> equal(js as _bool, false)
    }
    t |> run("integer") @(t : T?) {
        var js = parse("42", t)
        t |> equal(js is _longint, true)
        t |> equal(int(js as _longint), 42)
    }
    t |> run("negative integer") @(t : T?) {
        var js = parse("-7", t)
        t |> equal(js is _longint, true)
        t |> equal(int(js as _longint), -7)
    }
    t |> run("zero") @(t : T?) {
        var js = parse("0", t)
        t |> equal(js is _longint, true)
        t |> equal(int(js as _longint), 0)
    }
    t |> run("float") @(t : T?) {
        var js = parse("3.14", t)
        t |> equal(js is _number, true)
        t |> equal(js as _number > 3.13lf && js as _number < 3.15lf, true)
    }
    t |> run("negative float") @(t : T?) {
        var js = parse("-0.5", t)
        t |> equal(js is _number, true)
        t |> equal(js as _number < -0.49lf && js as _number > -0.51lf, true)
    }
    t |> run("scientific notation") @(t : T?) {
        var js = parse("1.5e10", t)
        t |> equal(js is _number, true)
        t |> equal(js as _number > 1.4e10lf, true)
    }
    t |> run("scientific E notation") @(t : T?) {
        var js = parse("2.5E-3", t)
        t |> equal(js is _number, true)
        t |> equal(js as _number > 0.0024lf && js as _number < 0.0026lf, true)
    }
    t |> run("string") @(t : T?) {
        var js = parse("\"hello\"", t)
        t |> equal(js is _string, true)
        t |> equal(js as _string, "hello")
    }
    t |> run("empty string") @(t : T?) {
        var js = parse("\"\"", t)
        t |> equal(js is _string, true)
        t |> equal(js as _string, "")
    }
}

// ============================================================
//  Parsing: string escapes
// ============================================================

[test]
def test_parse_string_escapes(t : T?) {
    t |> run("escaped quote") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"a\\\"b\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "a\"b")
    }
    t |> run("escaped backslash") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"a\\\\b\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "a\\b")
    }
    t |> run("escaped newline") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"a\\nb\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "a\nb")
    }
    t |> run("escaped tab") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"a\\tb\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "a\tb")
    }
    t |> run("escaped slash") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"a\\/b\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "a/b")
    }
    t |> run("unicode escape basic ASCII") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"\\u0041\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "A")
    }
    t |> run("unicode escape space") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"\\u0020\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, " ")
    }
    t |> run("unicode escape uppercase hex") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"\\u004F\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "O")
    }
    t |> run("unicode escape lowercase hex") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"\\u006f\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "o")
    }
    t |> run("multiple escapes in one string") @(t : T?) {
        let input = build_string() $(var w) {
            write(w, "\"\\t\\n\\r\"")
        }
        var js = parse(input, t)
        t |> equal(js as _string, "\t\n\r")
    }
}

// ============================================================
//  Parsing: arrays
// ============================================================

[test]
def test_parse_arrays(t : T?) {
    t |> run("empty array") @(t : T?) {
        var js = parse("[]", t)
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 0)
    }
    t |> run("array of ints") @(t : T?) {
        var js = parse("[1, 2, 3]", t)
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 3)
    }
    t |> run("nested array") @(t : T?) {
        var js = parse("[[1, 2], [3, 4]]", t)
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 2)
        let inner = (js as _array)[0]
        t |> equal(inner is _array, true)
        t |> equal(length(inner as _array), 2)
    }
    t |> run("mixed types array") @(t : T?) {
        var js = parse("[1, \"two\", true, null, 3.14]", t)
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 5)
        t |> equal((js as _array)[0] is _longint, true)
        t |> equal((js as _array)[1] is _string, true)
        t |> equal((js as _array)[2] is _bool, true)
        t |> equal((js as _array)[3] is _null, true)
        t |> equal((js as _array)[4] is _number, true)
    }
    t |> run("single element array") @(t : T?) {
        var js = parse("[42]", t)
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 1)
    }
}

// ============================================================
//  Parsing: objects
// ============================================================

[test]
def test_parse_objects(t : T?) {
    t |> run("empty object") @(t : T?) {
        var js = parse("\{\}", t)
        t |> equal(js is _object, true)
        t |> equal(length(js as _object), 0)
    }
    t |> run("single key object") @(t : T?) {
        let input = "\{ \"name\": \"john\" \}"
        var js = parse(input, t)
        t |> equal(js is _object, true)
        t |> equal(js?.name ?? "", "john")
    }
    t |> run("nested objects") @(t : T?) {
        let input = "\{ \"a\": \{ \"b\": 1 \} \}"
        var js = parse(input, t)
        t |> equal(js is _object, true)
        t |> equal(js?.a?.b ?? -1, 1)
    }
    t |> run("object with array value") @(t : T?) {
        let input = "\{ \"items\": [1, 2, 3] \}"
        var js = parse(input, t)
        t |> equal(js is _object, true)
        t |> equal(js?["items"] is _array, true)
    }
}

// ============================================================
//  Parsing: error handling
// ============================================================

[test]
def test_parse_errors(t : T?) {
    t |> run("empty input") @(t : T?) {
        var error : string
        var js = read_json("", error)
        t |> equal(js == null, true)
    }
    t |> run("just whitespace") @(t : T?) {
        var error : string
        var js = read_json("   ", error)
        t |> equal(js == null, true)
    }
    t |> run("unterminated string") @(t : T?) {
        parse_expect_fail("\"hello", t)
    }
    t |> run("unterminated array") @(t : T?) {
        parse_expect_fail("[1, 2", t)
    }
    t |> run("unterminated object") @(t : T?) {
        parse_expect_fail("\{ \"a\": 1", t)
    }
    t |> run("missing colon in object") @(t : T?) {
        parse_expect_fail("\{ \"a\" 1 \}", t)
    }
    t |> run("comma instead of colon") @(t : T?) {
        parse_expect_fail("\{ \"a\", 1 \}", t)
    }
    t |> run("invalid token") @(t : T?) {
        parse_expect_fail("undefined", t)
    }
    t |> run("trailing comma in array rejected") @(t : T?) {
        parse_expect_fail("[1, 2, ]", t)
    }
    t |> run("duplicate keys rejected by default") @(t : T?) {
        let input = "\{ \"a\": 1, \"a\": 2 \}"
        parse_expect_fail(input, t)
    }
    t |> run("duplicate keys allowed when set") @(t : T?) {
        let old = set_allow_duplicate_keys(true)
        let input = "\{ \"a\": 1, \"a\": 2 \}"
        var js = parse(input, t)
        t |> equal(js?.a ?? -1, 2)
        set_allow_duplicate_keys(old)
    }
}

// ============================================================
//  JV constructors (json.das)
// ============================================================

[test]
def test_jv_constructors(t : T?) {
    t |> run("JV string") @(t : T?) {
        var js = JV("hello")
        t |> equal(js is _string, true)
        t |> equal(js as _string, "hello")
    }
    t |> run("JV int") @(t : T?) {
        var js = JV(42)
        t |> equal(js is _longint, true)
        t |> equal(int(js as _longint), 42)
    }
    t |> run("JV float") @(t : T?) {
        var js = JV(3.14)
        t |> equal(js is _number, true)
    }
    t |> run("JV double") @(t : T?) {
        var js = JV(1.0lf)
        t |> equal(js is _number, true)
    }
    t |> run("JV bool true") @(t : T?) {
        var js = JV(true)
        t |> equal(js is _bool, true)
        t |> equal(js as _bool, true)
    }
    t |> run("JV bool false") @(t : T?) {
        var js = JV(false)
        t |> equal(js is _bool, true)
        t |> equal(js as _bool, false)
    }
    t |> run("JVNull") @(t : T?) {
        var js = JVNull()
        t |> equal(js is _null, true)
    }
    t |> run("JV int64") @(t : T?) {
        var js = JV(9876543210l)
        t |> equal(js is _longint, true)
        t |> equal(js as _longint, 9876543210l)
    }
    t |> run("JV uint") @(t : T?) {
        var js = JV(100u)
        t |> equal(js is _longint, true)
    }
}

// ============================================================
//  write_json
// ============================================================

[test]
def test_write_json(t : T?) {
    t |> run("write null") @(t : T?) {
        var js = JVNull()
        t |> equal(write_json(js), "null")
    }
    t |> run("write true") @(t : T?) {
        var js = JV(true)
        t |> equal(write_json(js), "true")
    }
    t |> run("write false") @(t : T?) {
        var js = JV(false)
        t |> equal(write_json(js), "false")
    }
    t |> run("write int") @(t : T?) {
        var js = JV(42)
        t |> equal(write_json(js), "42")
    }
    t |> run("write string") @(t : T?) {
        var js = JV("hello")
        t |> equal(write_json(js), "\"hello\"")
    }
    t |> run("write empty array") @(t : T?) {
        var arr : array<JsonValue?>
        var js = JV(arr)
        t |> equal(write_json(js), "[]")
    }
    t |> run("write empty object") @(t : T?) {
        var tab : table<string; JsonValue?>
        var js = JV(tab)
        t |> equal(write_json(js), "\{\}")
    }
    t |> run("write null pointer") @(t : T?) {
        var js : JsonValue?
        t |> equal(write_json(js), "null")
    }
    t |> run("no trailing zeros") @(t : T?) {
        let old = set_no_trailing_zeros(true)
        var js = JV(1.0lf)
        let written = write_json(js)
        t |> equal(written, "1")
        set_no_trailing_zeros(old)
    }
    t |> run("roundtrip string with escapes") @(t : T?) {
        var js = JV("line1\nline2\ttab")
        let written = write_json(js)
        var error : string
        var parsed = read_json(written, error)
        t |> equal(parsed as _string, "line1\nline2\ttab")
    }
}

// ============================================================
//  roundtrip: parse → write → parse
// ============================================================

[test]
def test_roundtrip(t : T?) {
    t |> run("roundtrip scalars") @(t : T?) {
        let inputs = array<string>("42", "3.14", "true", "false", "null", "\"hello\"")
        for (input in inputs) {
            var error : string
            var js1 = read_json(input, error)
            let written = write_json(js1)
            var js2 = read_json(written, error)
            t |> equal(write_json(js2), written)
        }
    }
    t |> run("roundtrip object") @(t : T?) {
        let input = "\{ \"x\": 1, \"y\": \"test\" \}"
        var error : string
        var js1 = read_json(input, error)
        let written = write_json(js1)
        var js2 = read_json(written, error)
        t |> equal(js2?.x ?? -1, 1)
        t |> equal(js2?.y ?? "", "test")
    }
    t |> run("roundtrip nested") @(t : T?) {
        let input = "\{ \"a\": [1, 2], \"b\": \{ \"c\": true \} \}"
        var error : string
        var js1 = read_json(input, error)
        let written = write_json(js1)
        var js2 = read_json(written, error)
        t |> equal(js2?.b?.c ?? false, true)
    }
}

// ============================================================
//  json_boost: safe access operators
// ============================================================

[test]
def test_safe_access(t : T?) {
    t |> run("?[] on object") @(t : T?) {
        let input = "\{ \"x\": 10 \}"
        var error : string
        var js = read_json(input, error)
        t |> equal(js?["x"] ?? -1, 10)
    }
    t |> run("?[] missing key returns null") @(t : T?) {
        let input = "\{ \"x\": 10 \}"
        var error : string
        var js = read_json(input, error)
        t |> equal(js?["missing"] is _null, true)
    }
    t |> run("?. on object") @(t : T?) {
        let input = "\{ \"name\": \"test\" \}"
        var error : string
        var js = read_json(input, error)
        t |> equal(js?.name ?? "", "test")
    }
    t |> run("?[] on array") @(t : T?) {
        var error : string
        var js = read_json("[10, 20, 30]", error)
        t |> equal(js?[0] ?? -1, 10)
        t |> equal(js?[1] ?? -1, 20)
        t |> equal(js?[2] ?? -1, 30)
    }
    t |> run("?[] out of bounds returns null") @(t : T?) {
        var error : string
        var js = read_json("[1]", error)
        t |> equal(js?[99] is _null, true)
    }
    t |> run("?? with default on missing") @(t : T?) {
        let input = "\{ \"x\": 10 \}"
        var error : string
        var js = read_json(input, error)
        t |> equal(js?.missing ?? 42, 42)
    }
    t |> run("?? coercion int from longint") @(t : T?) {
        var js = JV(100)
        t |> equal(js ?? -1, 100)
    }
    t |> run("?? coercion float from number") @(t : T?) {
        var js = JV(3.14)
        t |> equal(js ?? 0.0 > 3.0, true)
    }
    t |> run("?? bool default") @(t : T?) {
        var js = JVNull()
        t |> equal(js ?? true, true)
    }
    t |> run("?? string default") @(t : T?) {
        var js = JVNull()
        t |> equal(js ?? "fallback", "fallback")
    }
    t |> run("chained safe access") @(t : T?) {
        let input = "\{ \"a\": \{ \"b\": \{ \"c\": 42 \} \} \}"
        var error : string
        var js = read_json(input, error)
        t |> equal(js?.a?.b?.c ?? -1, 42)
    }
    t |> run("chained safe access missing middle") @(t : T?) {
        let input = "\{ \"a\": 1 \}"
        var error : string
        var js = read_json(input, error)
        t |> equal(js?.a?.b?.c ?? -1, -1)
    }
    t |> run("null pointer safe access") @(t : T?) {
        var js : JsonValue?
        t |> equal(js?.anything ?? "nope", "nope")
        t |> equal(js?[0] ?? -1, -1)
    }
}

// ============================================================
//  json_boost: from_JV conversions
// ============================================================

struct TestStruct {
    name : string
    value : int
    score : float
}

def operator ==(a, b : TestStruct) {
    return a.name == b.name && a.value == b.value && a.score == b.score
}

[test]
def test_from_jv(t : T?) {
    t |> run("from_JV int") @(t : T?) {
        var js = JV(42)
        t |> equal(from_JV(js, type<int>), 42)
    }
    t |> run("from_JV float") @(t : T?) {
        var js = JV(3.14)
        t |> equal(from_JV(js, type<float>) > 3.0, true)
    }
    t |> run("from_JV string") @(t : T?) {
        var js = JV("hello")
        t |> equal(from_JV(js, type<string>), "hello")
    }
    t |> run("from_JV bool") @(t : T?) {
        var js = JV(true)
        t |> equal(from_JV(js, type<bool>), true)
    }
    t |> run("from_JV null returns default") @(t : T?) {
        var js : JsonValue?
        t |> equal(from_JV(js, type<int>), 0)
        t |> equal(from_JV(js, type<string>), "")
        t |> equal(from_JV(js, type<bool>), false)
    }
    t |> run("from_JV struct") @(t : T?) {
        let input = "\{ \"name\": \"test\", \"value\": 42, \"score\": 3.14 \}"
        var error : string
        var js = read_json(input, error)
        var result = from_JV(js, type<TestStruct>)
        t |> equal(result.name, "test")
        t |> equal(result.value, 42)
        t |> equal(result.score > 3.0, true)
    }
    t |> run("from_JV array") @(t : T?) {
        var error : string
        var js = read_json("[1, 2, 3]", error)
        var result = from_JV(js, type<array<int>>)
        t |> equal(length(result), 3)
        t |> equal(result[0], 1)
        t |> equal(result[1], 2)
        t |> equal(result[2], 3)
    }
}

// ============================================================
//  json_boost: JV for complex types
// ============================================================

[test]
def test_jv_complex(t : T?) {
    t |> run("JV struct roundtrip") @(t : T?) {
        let s = TestStruct(name = "test", value = 42, score = 1.5)
        var js = JV(s)
        var result = from_JV(js, type<TestStruct>)
        t |> equal(result, s)
    }
    t |> run("JV array") @(t : T?) {
        var arr <- array<int>(1, 2, 3)
        var js = JV(arr)
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 3)
    }
    t |> run("JV table") @(t : T?) {
        var tab <- {"one" => 1, "two" => 2}
        var js = JV(tab)
        t |> equal(js is _object, true)
    }
    t |> run("JV tuple") @(t : T?) {
        let tup = (1, "two", 3.0)
        var js = JV(tup)
        t |> equal(js is _object, true)
    }
    t |> run("JV vector float2") @(t : T?) {
        var js = JV(float2(1.0, 2.0))
        t |> equal(js is _object, true)
        t |> equal(js?.x ?? 0.0, 1.0)
        t |> equal(js?.y ?? 0.0, 2.0)
    }
    t |> run("JV vector float3") @(t : T?) {
        var js = JV(float3(1.0, 2.0, 3.0))
        t |> equal(js?.z ?? 0.0, 3.0)
    }
    t |> run("JV multi-value shorthand") @(t : T?) {
        var js = JV(1, "two", true)
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 3)
    }
}

// ============================================================
//  json_boost: enum conversions
// ============================================================

enum Color {
    red
    green
    blue
}

[test]
def test_enum_json(t : T?) {
    t |> run("enum to JV") @(t : T?) {
        var js = JV(Color.green)
        t |> equal(js is _string, true)
        t |> equal(js as _string, "green")
    }
    t |> run("enum from JV string") @(t : T?) {
        var js = JV("blue")
        t |> equal(from_JV(js, type<Color>), Color.blue)
    }
    t |> run("enum roundtrip") @(t : T?) {
        var js = JV(Color.red)
        t |> equal(from_JV(js, type<Color>), Color.red)
    }
}

// ============================================================
//  json reader macro (%json~...%%)
// ============================================================

[test]
def test_json_reader_macro(t : T?) {
    t |> run("simple object literal") @(t : T?) {
        var js = %json~
        {
            "name": "test",
            "count": 42
        }
        %%
        t |> equal(js is _object, true)
        t |> equal(js?.name ?? "", "test")
        t |> equal(js?.count ?? -1, 42)
    }
    t |> run("array literal") @(t : T?) {
        var js = %json~ [1, 2, 3] %%
        t |> equal(js is _array, true)
        t |> equal(length(js as _array), 3)
    }
    t |> run("nested literal") @(t : T?) {
        var js = %json~
        {
            "items": [1, 2],
            "nested": { "ok": true }
        }
        %%
        t |> equal(js?.nested?.ok ?? false, true)
        t |> equal(js?["items"]?[0] ?? -1, 1)
    }
}

// ============================================================
//  settings: no_trailing_zeros, no_empty_arrays
// ============================================================

[test]
def test_write_settings(t : T?) {
    t |> run("no_empty_arrays suppresses empty arrays") @(t : T?) {
        let old = set_no_empty_arrays(true)
        let input = "\{ \"items\": [], \"name\": \"test\" \}"
        var error : string
        var js = read_json(input, error)
        let written = write_json(js)
        // "items" should be omitted from output
        t |> equal(written |> find("items") == -1, true)
        t |> equal(written |> find("name") != -1, true)
        set_no_empty_arrays(old)
    }
    t |> run("no_trailing_zeros for floats") @(t : T?) {
        let old = set_no_trailing_zeros(true)
        var js = JV(2.0lf)
        let written = write_json(js)
        t |> equal(written, "2")
        set_no_trailing_zeros(old)
    }
    t |> run("no_trailing_zeros preserves fractional") @(t : T?) {
        let old = set_no_trailing_zeros(true)
        var js = JV(3.14lf)
        let written = write_json(js)
        t |> equal(written |> starts_with("3.14"), true)
        set_no_trailing_zeros(old)
    }
}

// ============================================================
//  is/as variant checks via BetterJsonMacro
// ============================================================

[test]
def test_is_as_variant(t : T?) {
    t |> run("is _string") @(t : T?) {
        var js = JV("hello")
        t |> equal(js is _string, true)
        t |> equal(js is _number, false)
        t |> equal(js is _bool, false)
        t |> equal(js is _null, false)
    }
    t |> run("as _string") @(t : T?) {
        var js = JV("world")
        t |> equal(js as _string, "world")
    }
    t |> run("is _number for double") @(t : T?) {
        var js = JV(1.5lf)
        t |> equal(js is _number, true)
        t |> equal(js is _longint, false)
    }
    t |> run("is _longint for int") @(t : T?) {
        var js = JV(42)
        t |> equal(js is _longint, true)
        t |> equal(js is _number, false)
    }
    t |> run("is _object") @(t : T?) {
        let input = "\{ \"a\": 1 \}"
        var error : string
        var js = read_json(input, error)
        t |> equal(js is _object, true)
    }
    t |> run("is _array") @(t : T?) {
        var error : string
        var js = read_json("[1]", error)
        t |> equal(js is _array, true)
    }
}

// ============================================================
//  try_fixing_broken_json
// ============================================================

[test]
def test_fix_broken_json(t : T?) {
    t |> run("fix trailing comma in object") @(t : T?) {
        let bad = "\{ \"a\": 1, \}"
        let fixed = try_fixing_broken_json(bad)
        var error : string
        var js = read_json(fixed, error)
        t |> success(js != null)
    }
    t |> run("fix trailing comma in array") @(t : T?) {
        let bad = "[1, 2, ]"
        let fixed = try_fixing_broken_json(bad)
        var error : string
        var js = read_json(fixed, error)
        t |> success(js != null)
    }
    t |> run("fix string concatenation") @(t : T?) {
        let bad = build_string() $(var w) {
            write(w, "\{ \"msg\": \"hello\" + \"world\" \}")
        }
        let fixed = try_fixing_broken_json(bad)
        var error : string
        var js = read_json(fixed, error)
        t |> success(js != null)
    }
}

// ============================================================
//  variant serialization
// ============================================================

variant Shape {
    circle : float
    rect : float2
}

def operator ==(a, b : Shape) {
    if ((a is circle) && (b is circle)) {
        return (a as circle) == (b as circle)
    } elif ((a is rect) && (b is rect)) {
        return (a as rect) == (b as rect)
    }
    return false
}

[test]
def test_variant_json(t : T?) {
    t |> run("variant circle roundtrip") @(t : T?) {
        let s = Shape(circle = 5.0)
        var js = JV(s)
        t |> equal(js is _object, true)
        var result = from_JV(js, type<Shape>)
        t |> equal(result, s)
    }
    t |> run("variant rect roundtrip") @(t : T?) {
        let s = Shape(rect = float2(3.0, 4.0))
        var js = JV(s)
        var result = from_JV(js, type<Shape>)
        t |> equal(result, s)
    }
}
