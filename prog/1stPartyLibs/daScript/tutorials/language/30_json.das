// Tutorial 30: JSON
//
// This tutorial covers:
//   - Parsing JSON strings with read_json
//   - Building JSON values with JV constructors
//   - Writing JSON back to strings with write_json
//   - Safe access operators: ?[], ?., ??
//   - is / as variant checks on JSON values
//   - Struct serialization with JV and from_JV
//   - Enum serialization
//   - Modifying JSON values
//   - Field annotations: optional, embed, unescape, enum_as_int
//   - The %json~ reader macro for inline JSON
//   - Writer settings: no_trailing_zeros, no_empty_arrays
//   - Broken JSON repair with try_fixing_broken_json
//   - sprint_json for serializing any value to JSON
//   - sscan_json for deserializing JSON directly into structs
//   - Class serialization
//
// requires:  daslib/json, daslib/json_boost
// Run: daslang.exe tutorials/language/30_json.das

options gen2
options persistent_heap
options rtti

require daslib/json_boost

// === Parsing JSON ===
// read_json takes a string and returns a JsonValue? pointer.
// If parsing fails, the error string is set and null is returned.

def parsing_basics() {
    print("=== Parsing basics ===\n")

    var error : string
    var js = read_json("\{ \"name\": \"Alice\", \"age\": 30, \"active\": true \}", error)
    if (js != null) {
        print("parsed OK\n")
    } else {
        print("parse error: {error}\n")
    }

    // JsValue is a variant with these cases:
    //   _object  — table<string; JsonValue?>
    //   _array   — array<JsonValue?>
    //   _string  — string
    //   _number  — double (floating point)
    //   _longint — int64  (integer)
    //   _bool    — bool
    //   _null    — void?
    print("is object: {js is _object}\n")
    // output: is object: true
}

// === Building JSON with JV ===
// JV() constructors wrap native values into JsonValue?.
// JVNull() creates a JSON null.

def building_json() {
    print("\n=== Building JSON ===\n")

    var str_val = JV("hello")        // _string
    var int_val = JV(42)             // _longint
    var flt_val = JV(3.14)           // _number
    var bool_val = JV(true)          // _bool
    var null_val = JVNull()          // _null

    print("string: {write_json(str_val)}\n")
    // output: string: "hello"
    print("int: {write_json(int_val)}\n")
    // output: int: 42
    print("float: {write_json(flt_val)}\n")
    // output: float: 3.14
    print("bool: {write_json(bool_val)}\n")
    // output: bool: true
    print("null: {write_json(null_val)}\n")
    // output: null: null

    // Multi-value JV creates an array:
    var arr = JV(1, "two", true)
    print("array: {write_json(arr)}\n")
    // output: array: [1,"two",true]
}

// === Writing JSON ===
// write_json converts a JsonValue? back to a string.
// It handles null pointers gracefully.

def writing_json() {
    print("\n=== Writing JSON ===\n")

    var error : string
    var js = read_json("[1, 2, 3]", error)
    print("compact: {write_json(js)}\n")
    // output: compact: [1,2,3]

    // null pointer writes as "null"
    var empty : JsonValue?
    print("null ptr: {write_json(empty)}\n")
    // output: null ptr: null
}

// === Safe access operators ===
// json_boost provides ?., ?[], and ?? for convenient access.
// These never crash — they return null/default on missing keys.

def safe_access() {
    print("\n=== Safe access ===\n")

    var error : string
    var js = read_json("\{ \"user\": \{ \"name\": \"Bob\", \"scores\": [10, 20, 30] \} \}", error)

    // ?. accesses object fields by name
    let name = js?.user?.name ?? "unknown"
    print("name: {name}\n")
    // output: name: Bob

    // ?[] accesses by string key or integer index
    let first_score = js?["user"]?["scores"]?[0] ?? 0
    print("first score: {first_score}\n")
    // output: first score: 10

    // ?? provides a default when the value is null or missing
    let missing = js?.user?.email ?? "no email"
    print("missing: {missing}\n")
    // output: missing: no email

    // Chained access on deeply missing paths returns default
    let deep_missing = js?.a?.b?.c ?? -1
    print("deep missing: {deep_missing}\n")
    // output: deep missing: -1

    // null pointer is safe
    var nothing : JsonValue?
    let safe_val = nothing?.foo ?? "safe"
    print("null ?.: {safe_val}\n")
    // output: null ?.: safe
}

// === is / as variant checks ===
// json_boost rewrites is/as on JsonValue? to check the underlying
// JsValue variant. This works even through the pointer.

def variant_checks() {
    print("\n=== Variant checks ===\n")

    var js = JV("hello")
    print("is string: {js is _string}\n")
    // output: is string: true
    print("is number: {js is _number}\n")
    // output: is number: false
    print("as string: {js as _string}\n")
    // output: as string: hello

    var jn = JV(42)
    print("is longint: {jn is _longint}\n")
    // output: is longint: true
}

// === Struct serialization ===
// json_boost provides generic JV() and from_JV() that convert
// daslang structs to/from JSON objects using compile-time reflection.

struct Player {
    name : string
    hp : int
    speed : float
}

def struct_serialization() {
    print("\n=== Struct serialization ===\n")

    // struct → JSON
    let p = Player(name = "Hero", hp = 100, speed = 5.5)
    var js = JV(p)
    print("json: {write_json(js)}\n")
    // output: json: {"name":"Hero","hp":100,"speed":5.5}

    // JSON → struct
    var error : string
    var parsed = read_json("\{ \"name\": \"Mage\", \"hp\": 80, \"speed\": 3.2 \}", error)
    var p2 = from_JV(parsed, type<Player>)
    print("name: {p2.name}, hp: {p2.hp}\n")
    // output: name: Mage, hp: 80

    // Roundtrip
    var rt = from_JV(JV(p), type<Player>)
    print("roundtrip name: {rt.name}\n")
    // output: roundtrip name: Hero
}

// === Enum serialization ===
// Enums serialize as strings by default.

enum Weapon {
    sword
    bow
    staff
}

def enum_serialization() {
    print("\n=== Enum serialization ===\n")

    var js = JV(Weapon.bow)
    print("weapon json: {write_json(js)}\n")
    // output: weapon json: "bow"

    var w = from_JV(JV("staff"), type<Weapon>)
    print("weapon: {w}\n")
    // output: weapon: staff
}

// === Modifying JSON values ===
// You can manipulate JSON trees by building them incrementally.

def modifying_json() {
    print("\n=== Modifying JSON ===\n")

    // Build an object by hand
    var tab : table<string; JsonValue?>
    tab |> insert("name", JV("Alice"))
    tab |> insert("age", JV(30))
    var obj = JV(tab)
    print("built: {write_json(obj)}\n")

    // Build an array by hand
    var items : array<JsonValue?>
    items |> push(JV(1))
    items |> push(JV("two"))
    items |> push(JV(true))
    var arr = JV(items)
    print("array: {write_json(arr)}\n")
    // output: array: [1,"two",true]
}

// === Reader macro %json~...%% ===
// The %json~ reader macro lets you embed JSON directly in daslang code.
// It parses at compile time and creates a JsonValue? at runtime.

def reader_macro_demo() {
    print("\n=== Reader macro ===\n")

    var settings = %json~
    {
        "resolution": [1920, 1080],
        "fullscreen": true,
        "title": "My Game"
    }
    %%

    let title = settings?.title ?? "untitled"
    let fs = settings?.fullscreen ?? false
    let width = settings?["resolution"]?[0] ?? 0
    print("title: {title}, fullscreen: {fs}, width: {width}\n")
    // output: title: My Game, fullscreen: true, width: 1920
}

// === Writer settings ===
// Global settings control write_json behavior:
//   set_no_trailing_zeros(true) — omit .000000 from round floats
//   set_no_empty_arrays(true)   — skip empty array fields in objects

def writer_settings() {
    print("\n=== Writer settings ===\n")

    let old_tz = set_no_trailing_zeros(true)

    var js = JV(1.0lf)
    print("no trailing zeros: {write_json(js)}\n")
    // output: no trailing zeros: 1

    var js2 = JV(3.14lf)
    print("with fraction: {write_json(js2)}\n")
    // output: with fraction: 3.14

    set_no_trailing_zeros(old_tz)
}

// === Tables and arrays ===
// JV and from_JV work with table<string;T> and array<T>.

def collections_demo() {
    print("\n=== Collections ===\n")

    // array roundtrip
    var arr <- array<int>(10, 20, 30)
    var js_arr = JV(arr)
    print("array: {write_json(js_arr)}\n")
    // output: array: [10,20,30]

    var back = from_JV(js_arr, type<array<int>>)
    print("back[1]: {back[1]}\n")
    // output: back[1]: 20

    // table roundtrip
    var tab <- { "x" => 1, "y" => 2 }
    var js_tab = JV(tab)
    print("table: {write_json(js_tab)}\n")
}

// === Broken JSON repair ===
// try_fixing_broken_json attempts to fix common issues from LLM output:
//   - String concatenation: "hello" + "world" → "helloworld"
//   - Trailing commas: [1, 2, ] → [1, 2]
//   - Nested quotes: "she said "hi"" → "she said \"hi\""

def broken_json_demo() {
    print("\n=== Broken JSON repair ===\n")

    let bad = "\{ \"msg\": \"hello\", \}"
    let fixed = try_fixing_broken_json(bad)
    var error : string
    var js = read_json(fixed, error)
    if (js != null) {
        let msg = js?.msg ?? ""
        print("repaired: {msg}\n")
        // output: repaired: hello
    }
}

// === Tuples and variants ===
// Tuples serialize as objects with "_0", "_1", ... keys.
// Variants serialize with the active variant name as key.

variant Shape {
    circle : float
    rect : float2
}

def tuples_and_variants() {
    print("\n=== Tuples and variants ===\n")

    // Tuple
    let t = (42, "hello", true)
    var js_t = JV(t)
    print("tuple: {write_json(js_t)}\n")
    // output: tuple: {"_0":42,"_1":"hello","_2":true}

    // Variant
    let s = Shape(circle = 5.0)
    var js_s = JV(s)
    print("variant: {write_json(js_s)}\n")
    // output: variant: {"circle":5}
}

// === Vector types ===
// float2/3/4 and int2/3/4 serialize as objects with x/y/z/w keys.

def vectors_demo() {
    print("\n=== Vectors ===\n")

    var js = JV(float3(1.0, 2.0, 3.0))
    print("float3: {write_json(js)}\n")
    // output: float3: {"x":1,"y":2,"z":3}

    var v = from_JV(js, type<float3>)
    print("back: {v}\n")
    // output: back: (1, 2, 3)
}

// === sprint_json ===
// sprint_json is a builtin function that serializes any daslang value
// directly to a JSON string, without going through JsonValue?.
// It handles structs, classes, variants, tuples, tables, arrays, enums,
// pointers, and all basic types.

variant Payload {
    code : int
    message : float
}

struct Record {
    id : int
    tag : string
    notes : string
    data : Payload
    values : array<int>
    meta : table<string; int>
    coords : tuple<int; float>
    ptr : void?
}

def sprint_json_demo() {
    print("\n=== sprint_json ===\n")

    // sprint_json serializes any value to JSON
    // Second argument controls human-readable formatting
    var r <- Record(uninitialized
        id = 1,
        tag = "test",
        data = Payload(uninitialized code = 42),
        values = [1, 2, 3],
        meta <- { "x" => 10 },
        coords = (7, 3.14),
        ptr = null
    )
    let compact = sprint_json(r, false)
    print("compact: {compact}\n")

    let pretty = sprint_json(r, true)
    print("pretty:\n{pretty}\n")

    // Works with simple values too
    print("int: {sprint_json(42, false)}\n")
    // output: int: 42
    print("string: {sprint_json("hello", false)}\n")
    // output: string: "hello"
    print("array: {sprint_json([10, 20, 30], false)}\n")
    // output: array: [10,20,30]
}

// === Field annotations ===
// Struct field annotations control how sprint_json serializes fields:
//   @optional    — skip the field if it has a default/empty value
//   @embed       — embed a string field as raw JSON (no extra quotes)
//   @unescape    — don't escape special characters in the string
//   @enum_as_int — serialize an enum as its integer value, not a string
//   @rename="x"  — use "x" as the JSON key instead of the field name

enum Priority {
    low
    medium
    high
}

struct AnnotatedConfig {
    name : string
    @optional debug : bool              // omitted when false
    @optional tags : array<string>      // omitted when empty
    @embed raw_data : string            // embedded as raw JSON
    @unescape raw_path : string         // no escaping of special chars
    pri : Priority                      // serialized as string by default
    @enum_as_int level : Priority       // serialized as integer
    @rename = "type" _type : string     // appears as "type" in JSON
}

def annotations_demo() {
    print("\n=== Field annotations ===\n")

    // Field annotations are recognized by sprint_json
    var c <- AnnotatedConfig(uninitialized
        name = "app",
        debug = false,
        raw_data = "[1,2,3]",
        raw_path = "C:\\Users\\test",
        pri = Priority.high,
        level = Priority.medium,
        _type = "widget"
    )
    let json_str = sprint_json(c, false)
    print("annotated: {json_str}\n")
    // @optional: debug (false) and tags (empty) are skipped
    // @embed: raw_data appears as raw JSON [1,2,3], not quoted "[1,2,3]"
    // @unescape: raw_path preserves backslashes as-is
    // @enum_as_int: level is 1 (integer), pri is "high" (string)
    // @rename: _type field appears as "type" in the JSON output

    // sprint_json with human-readable formatting
    let pretty = sprint_json(c, true)
    print("pretty:\n{pretty}\n")
}

// === @rename annotation ===
// Use @rename="json_key" when the JSON key is a daScript reserved word
// or doesn't follow daScript naming conventions. The field name in code
// uses a safe prefix (e.g. _type), but serializes as the desired key.
// Works with sprint_json, JV, and from_JV.

struct ApiResponse {
    @rename = "type" _type : string     // "type" is awkward as a daScript identifier
    @rename = "class" _class : int      // "class" collides with daScript keyword
    value : float
}

def rename_demo() {
    print("\n=== @rename annotation ===\n")

    // sprint_json uses the renamed key
    var resp = ApiResponse(_type = "widget", _class = 3, value = 1.5)
    let json_str = sprint_json(resp, false)
    print("sprint_json: {json_str}\n")
    // output: sprint_json: \{"type":"widget","class":3,"value":1.500000\}

    // JV also respects @rename
    var js = JV(resp)
    print("via JV: {write_json(js)}\n")

    // from_JV maps the renamed keys back to struct fields
    var error : string
    var parsed = read_json("\{ \"type\": \"button\", \"class\": 5, \"value\": 2.0 \}", error)
    var result = from_JV(parsed, type<ApiResponse>)
    print("from_JV: _type={result._type}, _class={result._class}\n")
    // output: from_JV: _type=button, _class=5
}

// === sscan_json ===
// sscan_json is the reverse of sprint_json — it parses a JSON string
// directly into a struct, without going through JsonValue?.
// It uses RTTI to match JSON keys to struct fields and supports
// all the same types as sprint_json: structs, pointers, arrays,
// tables, tuples, variants, enums, bitfields, vector types, and
// all basic types. It also respects @rename field annotations.

struct Config {
    host : string
    port : int
    @rename _type : string
    tags : array<string>
}

def sscan_json_demo() {
    print("\n=== sscan_json ===\n")

    // Parse JSON directly into a struct
    var cfg : Config
    let ok = sscan_json("\{\"host\":\"localhost\",\"port\":8080,\"type\":\"server\",\"tags\":[\"prod\",\"v2\"]\}", cfg)
    print("ok: {ok}\n")
    // output: ok: true
    print("host: {cfg.host}, port: {cfg.port}\n")
    // output: host: localhost, port: 8080
    print("type: {cfg._type}\n")
    // output: type: server
    print("tags: {cfg.tags[0]}, {cfg.tags[1]}\n")
    // output: tags: prod, v2

    // Round-trip: sprint_json -> sscan_json
    var src = Player(name = "Hero", hp = 100, speed = 5.5)
    let json = sprint_json(src, false)
    var dst : Player
    sscan_json(json, dst)
    print("round-trip: {dst.name}, hp={dst.hp}\n")
    // output: round-trip: Hero, hp=100

    // Unknown keys are ignored, missing fields keep defaults
    var partial : Config
    sscan_json("\{\"host\":\"example.com\",\"unknown\":true\}", partial)
    print("partial: host={partial.host}, port={partial.port}\n")
    // output: partial: host=example.com, port=0
}

// === Class serialization ===
// Both JV/from_JV and sprint_json work with classes.
// Classes serialize their fields just like structs.

class Animal {
    species : string
    legs : int
}

def class_serialization() {
    print("\n=== Class serialization ===\n")

    var a = new Animal(species = "cat", legs = 4)
    let json_str = sprint_json(*a, false)
    print("class: {json_str}\n")

    // JV also works with class instances
    var js = JV(*a)
    print("via JV: {write_json(js)}\n")

    unsafe {
        delete a
    }
}

[export]
def main() {
    parsing_basics()
    building_json()
    writing_json()
    safe_access()
    variant_checks()
    struct_serialization()
    enum_serialization()
    modifying_json()
    reader_macro_demo()
    writer_settings()
    collections_demo()
    broken_json_demo()
    tuples_and_variants()
    vectors_demo()
    sprint_json_demo()
    annotations_demo()
    rename_demo()
    sscan_json_demo()
    class_serialization()
}
