options gen2
options rtti
options no_unused_block_arguments = false
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/json_boost
require daslib/strings_boost

// ============================================================
//  sprint_json: basic types
// ============================================================

[test]
def test_sprint_basic_types(t : T?) {
    t |> run("int") @(t : T?) {
        t |> equal(sprint_json(42, false), "42")
    }
    t |> run("uint") @(t : T?) {
        t |> equal(sprint_json(0xFFu, false), "255")
    }
    t |> run("int64") @(t : T?) {
        t |> equal(sprint_json(123456789l, false), "123456789")
    }
    t |> run("float") @(t : T?) {
        // sprint_json formats floats in its own way
        let s = sprint_json(3.14, false)
        t |> equal(s != "", true)
    }
    t |> run("double") @(t : T?) {
        let s = sprint_json(3.14lf, false)
        t |> equal(s != "", true)
    }
    t |> run("bool true") @(t : T?) {
        t |> equal(sprint_json(true, false), "true")
    }
    t |> run("bool false") @(t : T?) {
        t |> equal(sprint_json(false, false), "false")
    }
    t |> run("string") @(t : T?) {
        t |> equal(sprint_json("hello", false), "\"hello\"")
    }
    t |> run("string with escapes") @(t : T?) {
        t |> equal(sprint_json("a\nb", false), "\"a\\nb\"")
    }
}

// ============================================================
//  sprint_json: containers
// ============================================================

[test]
def test_sprint_containers(t : T?) {
    // NOTE: assertions use parsed JSON instead of exact string matching
    // due to JIT bug with sprint_json compact mode — see test_sprint_jit_compact_bug.
    t |> run("array") @(t : T?) {
        let json = sprint_json([1, 2, 3], false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js != null, true)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        let json = sprint_json(a, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js != null, true)
    }
    t |> run("table") @(t : T?) {
        var tab <- { "x" => 1 }
        let json = sprint_json(tab, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.x ?? -1, 1)
    }
    t |> run("empty table") @(t : T?) {
        var tab : table<string; int>
        let json = sprint_json(tab, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js != null, true)
    }
    t |> run("tuple") @(t : T?) {
        let s = sprint_json((42, 3.14), false)
        // tuples serialize with _0, _1 keys
        t |> equal(s != "", true)
    }
}

// ============================================================
//  sprint_json: structs
// ============================================================

struct SimpleStruct {
    x : int
    name : string
}

[test]
def test_sprint_struct(t : T?) {
    t |> run("basic struct") @(t : T?) {
        var s = SimpleStruct(x = 42, name = "hello")
        let json = sprint_json(s, false)
        // verify fields are present
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.x ?? 0, 42)
        t |> equal(js?.name ?? "", "hello")
    }
}

// ============================================================
//  sprint_json: enums
// ============================================================

enum Color {
    red
    green
    blue
}

[test]
def test_sprint_enum(t : T?) {
    t |> run("enum as string") @(t : T?) {
        t |> equal(sprint_json(Color.green, false), "\"green\"")
    }
}

// ============================================================
//  sprint_json: variants
// ============================================================

variant TestPayload {
    code : int
    msg : string
}

[test]
def test_sprint_variant(t : T?) {
    t |> run("variant int") @(t : T?) {
        let v = TestPayload(code = 99)
        let json = sprint_json(v, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.code ?? 0, 99)
    }
    t |> run("variant string") @(t : T?) {
        let v = TestPayload(msg = "hi")
        let json = sprint_json(v, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.msg ?? "", "hi")
    }
}

// ============================================================
//  sprint_json: classes
// ============================================================

class TestAnimal {
    species : string
    legs : int
}

[test]
def test_sprint_class(t : T?) {
    t |> run("class fields") @(t : T?) {
        var a = new TestAnimal(species = "cat", legs = 4)
        let json = sprint_json(*a, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.species ?? "", "cat")
        t |> equal(js?.legs ?? 0, 4)
        unsafe { delete a; }
    }
}

// ============================================================
//  sprint_json: null pointer
// ============================================================

[test]
def test_sprint_null(t : T?) {
    t |> run("null void pointer") @(t : T?) {
        var p : void?
        t |> equal(sprint_json(p, false), "null")
    }
}

// ============================================================
//  sprint_json: human-readable formatting
// ============================================================

[test]
def test_sprint_pretty(t : T?) {
    if (jit_enabled()) return ;
    t |> run("pretty has newlines") @(t : T?) {
        var s = SimpleStruct(x = 1, name = "a")
        let json = sprint_json(s, true)
        // human-readable format has newlines
        t |> equal(find(json, "\n") >= 0, true)
    }
    t |> run("compact has no newlines") @(t : T?) {
        var s = SimpleStruct(x = 1, name = "a")
        let json = sprint_json(s, false)
        // NOTE: Under JIT, sprint_json(struct, false) may produce pretty-printed
        // output for certain struct layouts.  SimpleStruct appears unaffected,
        // but if this fails under JIT, it is the same known bug documented in
        // test_sprint_jit_compact_bug below.
        t |> equal(find(json, "\n") >= 0, false)
    }
}

// ============================================================
//  @optional annotation
// ============================================================

struct OptionalFields {
    name : string
    @optional opt_bool : bool
    @optional opt_int : int
    @optional opt_float : float
    @optional opt_string : string
    @optional opt_array : array<int>
    @optional opt_table : table<string; int>
    @optional opt_ptr : void?
}

[test]
def test_annotation_optional(t : T?) {
    t |> run("default values are omitted") @(t : T?) {
        var s <- OptionalFields(uninitialized name = "test")
        let json = sprint_json(s, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        // name is present
        t |> equal(js?.name ?? "", "test")
        // all optional fields with defaults should be absent
        t |> equal(find(json, "opt_bool") < 0, true)
        t |> equal(find(json, "opt_int") < 0, true)
        t |> equal(find(json, "opt_float") < 0, true)
        t |> equal(find(json, "opt_string") < 0, true)
        t |> equal(find(json, "opt_array") < 0, true)
        t |> equal(find(json, "opt_table") < 0, true)
        t |> equal(find(json, "opt_ptr") < 0, true)
    }
    t |> run("non-default values are present") @(t : T?) {
        var s <- OptionalFields(uninitialized
            name = "test",
            opt_bool = true,
            opt_int = 42,
            opt_float = 1.0,
            opt_string = "hi",
            opt_array = [1],
            opt_table <- { "k" => 1 }
        )
        let json = sprint_json(s, false)
        // all optional fields with non-default values should be present
        t |> equal(find(json, "opt_bool") >= 0, true)
        t |> equal(find(json, "opt_int") >= 0, true)
        t |> equal(find(json, "opt_float") >= 0, true)
        t |> equal(find(json, "opt_string") >= 0, true)
        t |> equal(find(json, "opt_array") >= 0, true)
        t |> equal(find(json, "opt_table") >= 0, true)
    }
}

// ============================================================
//  @enum_as_int annotation
// ============================================================

struct EnumFields {
    normal : Color
    @enum_as_int as_int : Color
}

[test]
def test_annotation_enum_as_int(t : T?) {
    t |> run("normal enum is string") @(t : T?) {
        var s = EnumFields(normal = Color.green, as_int = Color.blue)
        let json = sprint_json(s, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.normal ?? "", "green")
    }
    t |> run("enum_as_int is integer") @(t : T?) {
        var s = EnumFields(normal = Color.red, as_int = Color.blue)
        let json = sprint_json(s, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.as_int ?? -1, 2)     // blue = 2
    }
    t |> run("enum_as_int zero") @(t : T?) {
        var s = EnumFields(normal = Color.red, as_int = Color.red)
        let json = sprint_json(s, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.as_int ?? -1, 0)     // red = 0
    }
}

// ============================================================
//  @embed annotation
// ============================================================

struct EmbedFields {
    name : string
    @embed data : string
}

[test]
def test_annotation_embed(t : T?) {
    // NOTE: @embed assertions use parsed JSON (read_json + type checks) instead of
    // exact substring matching because of a JIT bug: sprint_json(struct, false)
    // ignores humanReadable=false for some struct layouts under JIT, producing
    // pretty-printed output with newlines instead of compact JSON.
    // See test_sprint_jit_compact_bug below.
    t |> run("embedded array") @(t : T?) {
        var s = EmbedFields(name = "test", data = "[1,2,3]")
        let json = sprint_json(s, false)
        // data should be embedded as raw JSON, not quoted
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        // verify data is an array [1,2,3], not a string "[1,2,3]"
        t |> equal(js?.data is _array, true)
    }
    t |> run("embedded object") @(t : T?) {
        var s = EmbedFields(name = "test", data = "\{\"x\":1\}")
        let json = sprint_json(s, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        // verify data is an object with key "x", not a string
        t |> equal(js?.data?.x ?? -1, 1)
    }
    t |> run("embedded string is reparsed") @(t : T?) {
        var s = EmbedFields(name = "test", data = "[1,2,3]")
        let json = sprint_json(s, false)
        // the entire output should be valid JSON
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js != null, true)
    }
}

// ============================================================
//  @unescape annotation
// ============================================================

struct UnescapeFields {
    normal : string
    @unescape raw : string
}

[test]
def test_annotation_unescape(t : T?) {
    t |> run("normal string escapes backslash") @(t : T?) {
        var s = UnescapeFields(normal = "C:\\Users\\test", raw = "C:\\Users\\test")
        let json = sprint_json(s, false)
        // normal field should have escaped backslashes: C:\\Users\\test
        t |> equal(find(json, "C:\\\\Users\\\\test") >= 0, true)
    }
    t |> run("unescape preserves raw") @(t : T?) {
        var s = UnescapeFields(normal = "hello", raw = "C:\\Users\\test")
        let json = sprint_json(s, false)
        // raw field should NOT have double backslashes — just single: C:\Users\test
        // Note: @unescape produces non-standard JSON (unescaped backslashes), so
        // read_json cannot parse these values.  We use value-only substring matching
        // which is format-independent (unaffected by JIT pretty-printing spaces).
        t |> equal(find(json, "C:\\Users\\test") >= 0, true)
    }
}

// ============================================================
//  @rename annotation
// ============================================================

struct RenameFields {
    @rename = "type" _type : string
    @rename = "class" _class : int
    normal : bool
}

[test]
def test_annotation_rename(t : T?) {
    t |> run("renamed fields in sprint_json") @(t : T?) {
        var s = RenameFields(_type = "widget", _class = 5, normal = true)
        let json = sprint_json(s, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        // field appears as "type" not "_type"
        let type_val = js?["type"] ?? ""
        t |> equal(type_val, "widget")
        // field appears as "class" not "_class"
        let class_val = js?["class"] ?? 0
        t |> equal(class_val, 5)
        // normal field keeps its name
        t |> equal(js?.normal ?? false, true)
    }
    t |> run("renamed fields in JV") @(t : T?) {
        var s = RenameFields(_type = "box", _class = 3, normal = false)
        var js = JV(s)
        // JV also respects @rename
        let type_val = js?["type"] ?? ""
        t |> equal(type_val, "box")
        let class_val = js?["class"] ?? 0
        t |> equal(class_val, 3)
    }
    t |> run("from_JV with rename") @(t : T?) {
        var error : string
        var js = read_json("\{ \"type\": \"item\", \"class\": 7, \"normal\": true \}", error)
        t |> equal(error, "")
        var s = from_JV(js, type<RenameFields>)
        t |> equal(s._type, "item")
        t |> equal(s._class, 7)
        t |> equal(s.normal, true)
    }
}

// ============================================================
//  @rename without string value (bare annotation)
// ============================================================

struct BareRenameFields {
    @rename _bare : int         // no string value — strips leading underscore, key is "bare"
    normal : string
}

[test]
def test_annotation_rename_bare(t : T?) {
    t |> run("bare @rename strips underscore in sprint_json") @(t : T?) {
        var s = BareRenameFields(_bare = 42, normal = "ok")
        let json = sprint_json(s, false)
        // Validate via parsed JSON — bare @rename strips leading underscore
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        t |> equal(js?.bare ?? -1, 42)
    }
    t |> run("bare @rename strips underscore in JV") @(t : T?) {
        var s = BareRenameFields(_bare = 99, normal = "ok")
        var js = JV(s)
        // key is "bare" (underscore stripped)
        t |> equal(js?.bare ?? 0, 99)
    }
    t |> run("bare @rename from_JV") @(t : T?) {
        var error : string
        var js = read_json("\{ \"bare\": 7, \"normal\": \"yes\" \}", error)
        t |> equal(error, "")
        var s = from_JV(js, type<BareRenameFields>)
        t |> equal(s._bare, 7)
        t |> equal(s.normal, "yes")
    }
}

// ============================================================
//  Combined annotations
// ============================================================

enum TestLevel {
    low
    medium
    high
}

struct CombinedAnnotations {
    @rename = "label" _label : string
    @optional @enum_as_int pri : TestLevel
    @embed payload : string
    @unescape raw : string
}

[test]
def test_combined_annotations(t : T?) {
    // NOTE: @embed assertion uses parsed JSON instead of substring match
    // due to JIT bug with sprint_json compact mode — see test_sprint_jit_compact_bug.
    t |> run("all annotations together") @(t : T?) {
        var s = CombinedAnnotations(
            _label = "test",
            pri = TestLevel.high,
            payload = "[1,2]",
            raw = "a\\b"
        )
        let json = sprint_json(s, false)
        var error : string
        var js = read_json(json, error)
        t |> equal(error, "")
        // @rename: "label" not "_label"
        t |> equal(js?["label"] ?? "", "test")
        // @enum_as_int: high = 2
        t |> equal(js?.pri ?? -1, 2)
        // @embed: payload is a raw JSON array, not a quoted string
        t |> equal(js?.payload is _array, true)
    }
    t |> run("optional on enum does not skip (enum type not checked)") @(t : T?) {
        var s = CombinedAnnotations(
            _label = "test",
            pri = TestLevel.low,        // low = 0
            payload = "[]",
            raw = ""
        )
        let json = sprint_json(s, false)
        // @optional does not recognize enum types in the C++ implementation,
        // so the field is always present even when the value is 0/default
        t |> equal(find(json, "\"pri\"") >= 0, true)
    }
}

[test]
def test_sprint_jit_compact_bug(t : T?) {
    t |> run("compact mode should not contain newlines") @(t : T?) {
        var s = CombinedAnnotations(
            _label = "test",
            pri = TestLevel.high,
            payload = "[1,2]",
            raw = "a\\b"
        )
        let json = sprint_json(s, false)
        let has_newlines = find(json, "\n") >= 0
        t |> equal(has_newlines, false)
    }
}
