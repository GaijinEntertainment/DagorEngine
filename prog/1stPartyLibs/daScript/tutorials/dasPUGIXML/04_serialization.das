// Tutorial PUGIXML-04: Struct Serialization
//
// This tutorial covers:
//   - Serializing structs to XML with XML() and to_XML()
//   - Deserializing XML to structs with from_XML()
//   - One-step string deserialization with from_XML(string, type)
//   - Roundtrip: struct → XML string → struct
//   - Enum and bitfield serialization
//   - Array, table, and set types
//   - Tuple and variant types
//   - Fixed-size arrays (dim)
//   - Vector types (float2/3/4, int2/3/4)
//   - Field annotations: @rename, @enum_as_int, @unescape
//   - Nested structs
//
// requires: daslib/PUGIXML_boost (re-exports pugixml)
// Run: daslang.exe tutorials/dasPUGIXML/04_serialization.das

options gen2
options persistent_heap
options rtti

require pugixml/PUGIXML_boost

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Basic struct serialization with to_XML / from_XML
// ──────────────────────────────────────────────────────────────────────────
//
// to_XML(value) converts any struct to a complete XML string.
// from_XML(string, type<T>) parses it back in one step.

struct Player {
    name : string
    hp : int
    speed : float
    alive : bool
}

def basic_serialization() {
    print("=== Basic struct serialization ===\n")

    let p = Player(name = "Hero", hp = 100, speed = 5.5, alive = true)

    // Serialize to XML string
    let xml_str = to_XML(p)
    print("XML:\n{xml_str}")
    // output: XML:
    // output: <?xml version="1.0"?>
    // output: <root>
    // output:   <name>Hero</name>
    // output:   <hp>100</hp>
    // output:   <speed>5.5</speed>
    // output:   <alive>true</alive>
    // output: </root>

    // Deserialize back
    var restored = from_XML(xml_str, type<Player>)
    print("name: {restored.name}, hp: {restored.hp}, speed: {restored.speed}\n")
    // output: name: Hero, hp: 100, speed: 5.5
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Custom root element name
// ──────────────────────────────────────────────────────────────────────────
//
// to_XML accepts an optional root_name argument (default is "root").

def custom_root_name() {
    print("\n=== Custom root name ===\n")

    let p = Player(name = "Mage", hp = 80, speed = 3.0, alive = true)
    let xml_str = to_XML(p, "player")
    print(xml_str)
    // output: <?xml version="1.0"?>
    // output: <player>
    // output:   <name>Mage</name>
    // output:   <hp>80</hp>
    // output:   <speed>3</speed>
    // output:   <alive>true</alive>
    // output: </player>
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Nested structs
// ──────────────────────────────────────────────────────────────────────────

struct Address {
    street : string
    city : string
    zip : string
}

struct Person {
    name : string
    age : int
    address : Address
}

def nested_structs() {
    print("\n=== Nested structs ===\n")

    let p = Person(
        name = "Alice",
        age = 30,
        address = Address(street = "123 Main St", city = "Springfield", zip = "62704")
    )

    let xml_str = to_XML(p, "person")
    print(xml_str)

    var restored = from_XML(xml_str, type<Person>)
    print("restored: {restored.name}, {restored.address.city}\n")
    // output: restored: Alice, Springfield
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Enum serialization
// ──────────────────────────────────────────────────────────────────────────
//
// Enums serialize as their name by default. Use @enum_as_int to
// serialize as integer value instead.

enum Weapon {
    sword
    bow
    staff
}

struct Warrior {
    name : string
    weapon : Weapon
    @enum_as_int backup : Weapon
}

def enum_serialization() {
    print("\n=== Enum serialization ===\n")

    let w = Warrior(name = "Knight", weapon = Weapon.sword, backup = Weapon.bow)
    let xml_str = to_XML(w, "warrior")
    print(xml_str)
    // weapon serializes as "sword" (name), backup as "1" (int)

    var restored = from_XML(xml_str, type<Warrior>)
    print("weapon: {restored.weapon}, backup: {restored.backup}\n")
    // output: weapon: sword, backup: bow
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Bitfield serialization
// ──────────────────────────────────────────────────────────────────────────
//
// Bitfields serialize as their unsigned integer value.

bitfield Permissions {
    read
    write
    execute
}

def bitfield_serialization() {
    print("\n=== Bitfield serialization ===\n")

    let perms = Permissions.read | Permissions.execute
    let xml_str = to_XML(perms, "permissions")
    print(xml_str)

    var restored = from_XML(xml_str, type<Permissions>)
    let has_read = uint(restored & Permissions.read) != 0u
    print("read: {has_read}\n")
    // output: read: true
    let has_write = uint(restored & Permissions.write) != 0u
    print("write: {has_write}\n")
    // output: write: false
    let has_execute = uint(restored & Permissions.execute) != 0u
    print("execute: {has_execute}\n")
    // output: execute: true
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Arrays and tables
// ──────────────────────────────────────────────────────────────────────────
//
// array<T> elements become <item> children.
// table<K;V> entries become <entry> children with <_key> and <_val>.
// table<K> (set) entries become <item> children.

struct Inventory {
    items : array<string>
    counts : table<string; int>
}

def arrays_and_tables() {
    print("\n=== Arrays and tables ===\n")

    var inv <- Inventory(
        items <- ["sword", "shield", "potion"],
        counts <- { "sword" => 1, "shield" => 2, "potion" => 5 }
    )

    let xml_str = to_XML(inv, "inventory")
    print(xml_str)

    var restored <- from_XML(xml_str, type<Inventory>)
    print("items: {length(restored.items)}\n")
    // output: items: 3
    for (item in restored.items) {
        print("  {item}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 7 — Tuples and variants
// ──────────────────────────────────────────────────────────────────────────
//
// Tuples: fields become _0, _1, _2, ...
// Variants: active index stored in _variant attribute.

variant Shape {
    circle : float
    rectangle : float2
}

def tuples_and_variants() {
    print("\n=== Tuples and variants ===\n")

    // Tuple
    let t = (42, "hello", 3.14)
    let t_xml = to_XML(t, "tuple")
    print("Tuple:\n{t_xml}")

    var t_back = from_XML(t_xml, type<tuple<int; string; float>>)
    print("restored: ({t_back._0}, {t_back._1}, {t_back._2})\n")
    // output: restored: (42, hello, 3.14)

    // Variant — circle
    let s1 = Shape(circle = 5.0)
    let s1_xml = to_XML(s1, "shape")
    print("Circle:\n{s1_xml}")

    var s1_back = from_XML(s1_xml, type<Shape>)
    print("restored circle: {s1_back as circle}\n")
    // output: restored circle: 5

    // Variant — rectangle
    let s2 = Shape(rectangle = float2(10.0, 20.0))
    let s2_xml = to_XML(s2, "shape")
    print("Rectangle:\n{s2_xml}")

    var s2_back = from_XML(s2_xml, type<Shape>)
    let rect = s2_back as rectangle
    print("restored rectangle: {rect.x} x {rect.y}\n")
    // output: restored rectangle: 10 x 20
}

// ──────────────────────────────────────────────────────────────────────────
// Section 8 — Fixed-size arrays (dim)
// ──────────────────────────────────────────────────────────────────────────
//
// Fixed-size arrays (dim) serialize as sequential <item> children,
// just like dynamic arrays.

struct Matrix2x2 {
    data : int[4]
}

def dim_serialization() {
    print("\n=== Fixed-size arrays (dim) ===\n")

    var m = Matrix2x2(data = fixed_array(1, 0, 0, 1))
    let xml_str = to_XML(m, "matrix")
    print(xml_str)

    var restored = from_XML(xml_str, type<Matrix2x2>)
    print("restored: [{restored.data[0]}, {restored.data[1]}, {restored.data[2]}, {restored.data[3]}]\n")
    // output: restored: [1, 0, 0, 1]
}

// ──────────────────────────────────────────────────────────────────────────
// Section 9 — Vector types
// ──────────────────────────────────────────────────────────────────────────
//
// float2/3/4 and int2/3/4 serialize with x, y, z, w child elements.

def vector_serialization() {
    print("\n=== Vector types ===\n")

    let pos = float3(1.0, 2.5, -3.0)
    let xml_str = to_XML(pos, "position")
    print(xml_str)

    var restored = from_XML(xml_str, type<float3>)
    print("restored: ({restored.x}, {restored.y}, {restored.z})\n")
    // output: restored: (1, 2.5, -3)

    // int2
    let size = int2(1920, 1080)
    let xml2 = to_XML(size, "resolution")
    var restored2 = from_XML(xml2, type<int2>)
    print("resolution: {restored2.x} x {restored2.y}\n")
    // output: resolution: 1920 x 1080
}

// ──────────────────────────────────────────────────────────────────────────
// Section 10 — Field annotations
// ──────────────────────────────────────────────────────────────────────────
//
// @rename  — use a different XML element name for a field
// @enum_as_int — serialize enum as integer (shown in Section 4)
// @unescape — don't escape special characters in string values (for XML entities)

enum Priority {
    low
    medium
    high
}

struct Config {
    @rename = "type" _type : string
    @enum_as_int level : Priority
    name : string
}

def annotations_demo() {
    print("\n=== Field annotations ===\n")

    let c = Config(
        _type = "widget",
        level = Priority.high,
        name = "app"
    )
    let xml_str = to_XML(c, "config")
    print(xml_str)
    // _type field appears as <type>, not <_type>
    // level appears as integer 2, not "high"

    var restored = from_XML(xml_str, type<Config>)
    print("type: {restored._type}\n")
    // output: type: widget
    print("level: {restored.level}\n")
    // output: level: high
    print("name: {restored.name}\n")
    // output: name: app
}

// ──────────────────────────────────────────────────────────────────────────
// Section 11 — Low-level XML() and from_XML(node)
// ──────────────────────────────────────────────────────────────────────────
//
// XML() serializes a value into an existing xml_node's children.
// from_XML(node, type<T>) deserializes from a node.
// These give you control over the document structure.

def low_level_api() {
    print("\n=== Low-level XML() and from_XML(node) ===\n")

    with_doc() <| $(doc) {
        // Build a document with multiple records
        doc |> tag("records") <| $(var records) {
            let p1 = Player(name = "Alice", hp = 100, speed = 5.0, alive = true)
            let p2 = Player(name = "Bob", hp = 80, speed = 3.5, alive = false)

            records |> tag("player") <| $(var node) {
                XML(node, p1)
            }
            records |> tag("player") <| $(var node) {
                XML(node, p2)
            }
        }
        print(to_string(doc))

        // Read them back
        let records = doc.document_element
        records |> for_each_child("player") <| $(node) {
            var p = from_XML(node, type<Player>)
            print("  {p.name}: hp={p.hp}, alive={p.alive}\n")
        }
        // output:   Alice: hp=100, alive=true
        // output:   Bob: hp=80, alive=false
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 12 — Complete roundtrip example
// ──────────────────────────────────────────────────────────────────────────

struct GameState {
    level : int
    score : int
    player : Player
    weapons : array<string>
    position : float3
}

def roundtrip_example() {
    print("\n=== Complete roundtrip ===\n")

    var state <- GameState(
        level = 3,
        score = 42000,
        player = Player(name = "Hero", hp = 95, speed = 6.0, alive = true),
        weapons <- ["sword", "bow", "fireball"],
        position = float3(10.5, 0.0, -3.2)
    )

    // Serialize
    let xml_str = to_XML(state, "save")
    print("Saved XML to string\n")

    // Deserialize
    var loaded <- from_XML(xml_str, type<GameState>)
    print("Level: {loaded.level}, Score: {loaded.score}\n")
    print("Player: {loaded.player.name}, HP: {loaded.player.hp}\n")
    print("Weapons: {length(loaded.weapons)}\n")
    for (w in loaded.weapons) {
        print("  {w}\n")
    }
    print("Position: ({loaded.position.x}, {loaded.position.y}, {loaded.position.z})\n")
}

[export]
def main() {
    basic_serialization()
    custom_root_name()
    nested_structs()
    enum_serialization()
    bitfield_serialization()
    arrays_and_tables()
    tuples_and_variants()
    dim_serialization()
    vector_serialization()
    annotations_demo()
    low_level_api()
    roundtrip_example()
}
