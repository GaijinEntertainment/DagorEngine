// Tutorial 41: Serialization (archive)
//
// This tutorial covers:
//   - Binary serialization with daslib/archive
//   - Saving and loading primitive types
//   - Serializing structs automatically
//   - Serializing arrays and tables
//   - Serializing variants and tuples
//   - Using mem_archive_save / mem_archive_load
//   - Round-trip serialization pattern
//   - Custom serialize overloads for non-default formats
//
// Run: daslang.exe tutorials/language/41_serialization.das

options gen2
options no_unused_function_arguments = false

require daslib/archive
require math

// ============================================================
// Section 1: Basic serialization of primitives
// ============================================================
// The archive module provides serialize() generics that handle
// reading and writing binary data. An Archive struct combines
// a stream + direction (reading/writing).

def demo_primitives() {
    print("=== primitive serialization ===\n")
    // Save values to binary
    var x = 42
    var y = 3.14
    var s = "hello"
    var data <- mem_archive_save(x)
    print("  int data size: {length(data)} bytes\n")
    // Load it back
    var loaded_x : int
    mem_archive_load(data, loaded_x)
    print("  loaded int: {loaded_x}\n")

    // Float
    var fdata <- mem_archive_save(y)
    print("  float data size: {length(fdata)} bytes\n")
    var loaded_y : float
    mem_archive_load(fdata, loaded_y)
    print("  loaded float: {loaded_y}\n")

    // String
    var sdata <- mem_archive_save(s)
    print("  string data size: {length(sdata)} bytes\n")
    var loaded_s : string
    mem_archive_load(sdata, loaded_s)
    print("  loaded string: \"{loaded_s}\"\n")
}

// ============================================================
// Section 2: Struct serialization
// ============================================================
// Structs are serialized field by field automatically.

struct Player {
    name : string
    hp : int
    x : float
    y : float
}

def demo_struct() {
    print("\n=== struct serialization ===\n")
    var player = Player(name = "Hero", hp = 100, x = 1.5, y = 2.5)
    print("  original: {player.name}, hp={player.hp}, pos=({player.x}, {player.y})\n")

    // Save
    var data <- mem_archive_save(player)
    print("  serialized size: {length(data)} bytes\n")

    // Load into a new struct
    var loaded : Player
    mem_archive_load(data, loaded)
    print("  loaded:   {loaded.name}, hp={loaded.hp}, pos=({loaded.x}, {loaded.y})\n")
}

// ============================================================
// Section 3: Array serialization
// ============================================================
// Dynamic arrays serialize length + elements.

def demo_arrays() {
    print("\n=== array serialization ===\n")
    var scores <- [10, 20, 30, 40, 50]
    print("  original scores: ")
    for (s in scores) {
        print("{s} ")
    }
    print("\n")

    var data <- mem_archive_save(scores)
    print("  serialized size: {length(data)} bytes\n")

    var loaded : array<int>
    mem_archive_load(data, loaded)
    print("  loaded scores:   ")
    for (s in loaded) {
        print("{s} ")
    }
    print("\n")
}

// ============================================================
// Section 4: Table serialization
// ============================================================
// Tables serialize length + key-value pairs.

def demo_tables() {
    print("\n=== table serialization ===\n")
    var inventory : table<string; int>
    inventory |> insert("sword", 1)
    inventory |> insert("potion", 5)
    inventory |> insert("arrow", 20)

    var data <- mem_archive_save(inventory)
    print("  serialized size: {length(data)} bytes\n")

    var loaded : table<string; int>
    mem_archive_load(data, loaded)
    print("  loaded items:\n")
    // Note: table iteration order may vary
    for (k, v in keys(loaded), values(loaded)) {
        print("    {k}: {v}\n")
    }
}

// ============================================================
// Section 5: Tuple serialization
// ============================================================

def demo_tuples() {
    print("\n=== tuple serialization ===\n")
    var pair : tuple<name : string; score : int>
    pair.name = "Alice"
    pair.score = 99

    var data <- mem_archive_save(pair)
    print("  tuple data size: {length(data)} bytes\n")

    var loaded : tuple<name : string; score : int>
    mem_archive_load(data, loaded)
    print("  loaded: name=\"{loaded.name}\", score={loaded.score}\n")
}

// ============================================================
// Section 6: Variant serialization
// ============================================================
// Variants serialize the active variant index + the active value.

variant Value {
    i : int
    f : float
    s : string
}

def demo_variants() {
    print("\n=== variant serialization ===\n")
    unsafe {
        // Construct variants with the correct active field
        var v1 = Value(i = 42)
        var v2 = Value(s = "text")

        var data1 <- mem_archive_save(v1)
        var data2 <- mem_archive_save(v2)

        // Load them back
        var loaded1, loaded2 : Value
        mem_archive_load(data1, loaded1)
        mem_archive_load(data2, loaded2)

        // Check which variant is active using 'is'
        if (loaded1 is i) {
            print("  loaded1: int = {loaded1.i}\n")
        }
        if (loaded2 is s) {
            print("  loaded2: string = \"{loaded2.s}\"\n")
        }
    }
}

// ============================================================
// Section 7: Complex nested structs
// ============================================================
// Serialization is recursive — structs containing arrays,
// tables, and other structs are handled automatically.

struct Inventory {
    items : array<string>
    counts : table<string; int>
}

struct GameState {
    player : Player
    inventory : Inventory
    level : int
}

def demo_nested() {
    print("\n=== nested struct serialization ===\n")
    var state : GameState
    state.player = Player(name = "Knight", hp = 80, x = 10.0, y = 20.0)
    state.level = 3
    state.inventory.items |> push("sword")
    state.inventory.items |> push("shield")
    state.inventory.counts |> insert("sword", 1)
    state.inventory.counts |> insert("shield", 1)

    var data <- mem_archive_save(state)
    print("  game state size: {length(data)} bytes\n")

    var loaded : GameState
    mem_archive_load(data, loaded)
    print("  player: {loaded.player.name}, hp={loaded.player.hp}\n")
    print("  level: {loaded.level}\n")
    print("  items: ")
    for (item in loaded.inventory.items) {
        print("{item} ")
    }
    print("\n")
}

// ============================================================
// Section 8: Manual Archive usage
// ============================================================
// For full control, create Archive + MemSerializer manually.

def demo_manual_archive() {
    print("\n=== manual archive usage ===\n")
    // Writing phase
    var writer = new MemSerializer()
    var warch = Archive(reading = false, stream = writer)
    var name = "save_001"
    var score = 9999
    var tags <- ["rpg", "fantasy"]
    warch |> serialize(name)
    warch |> serialize(score)
    warch |> serialize(tags)
    var data <- writer->extractData()
    print("  wrote {length(data)} bytes\n")

    // Reading phase
    var reader = new MemSerializer(data)
    var rarch = Archive(reading = true, stream = reader)
    var r_name : string
    var r_score : int
    var r_tags : array<string>
    rarch |> serialize(r_name)
    rarch |> serialize(r_score)
    rarch |> serialize(r_tags)
    print("  read: name=\"{r_name}\", score={r_score}, tags=[")
    for (tag in r_tags) {
        print("{tag} ")
    }
    print("]\n")
}

// ============================================================
// Section 9: Custom serialize for non-default format
// ============================================================
// You can override the serialization of any type by defining:
//   def serialize(var arch : Archive; var val : YourType&)
// This overload is more specific than the generic `[expect_any_struct]`
// version, so it wins — as long as the `serialize` call happens within
// the module that defines the overload (or one that requires it).
//
// Note: `mem_archive_save` / `mem_archive_load` resolve `serialize` in the
// archive module's scope, so they won't find overloads defined in user code.
// Instead, create the Archive manually — this is straightforward and makes
// the overload dispatch explicit.
//
// Use case: a Color struct stores r/g/b as floats (0..1) internally,
// but we want compact serialization as 3 bytes (0..255).

struct Color {
    r : float = 0.0
    g : float = 0.0
    b : float = 0.0
}

// Custom serialize — packs each channel into a uint8 on write,
// unpacks on read.  This replaces the default field-by-field serializer.
def serialize(var arch : Archive; var c : Color&) {
    if (arch.reading) {
        var rb, gb, bb : uint8
        arch |> serialize_raw(rb)
        arch |> serialize_raw(gb)
        arch |> serialize_raw(bb)
        c.r = float(rb) / 255.0
        c.g = float(gb) / 255.0
        c.b = float(bb) / 255.0
    } else {
        var rb = uint8(clamp(c.r * 255.0, 0.0, 255.0))
        var gb = uint8(clamp(c.g * 255.0, 0.0, 255.0))
        var bb = uint8(clamp(c.b * 255.0, 0.0, 255.0))
        arch |> serialize_raw(rb)
        arch |> serialize_raw(gb)
        arch |> serialize_raw(bb)
    }
}

def demo_custom_serialize() {
    print("\n=== custom serialize ===\n")

    // Create a color
    var c = Color(r = 1.0, g = 0.5, b = 0.0)
    print("  original: r={c.r}, g={c.g}, b={c.b}\n")

    // Save — uses our custom serializer
    // We use Archive directly so that `serialize` resolves in our module,
    // where it finds the custom Color overload.
    var writer = new MemSerializer()
    var warch = Archive(reading = false, stream = writer)
    warch |> serialize(c)
    var data <- writer->extractData()
    print("  serialized size: {length(data)} bytes (3 instead of 12)\n")

    // Load — uses our custom deserializer
    var reader = new MemSerializer(data)
    var rarch = Archive(reading = true, stream = reader)
    var loaded = Color()
    rarch |> serialize(loaded)
    print("  loaded: r={loaded.r}, g={loaded.g}, b={loaded.b}\n")

    // Verify the packed bytes are correct
    // r=1.0 -> 255, g=0.5 -> 128 (rounding), b=0.0 -> 0
    print("  bytes: [{data[0]}, {data[1]}, {data[2]}]\n")

    // Array of Colors — must also serialize manually to use custom overload.
    // The custom `serialize` for Color is picked up by the generic array
    // serializer when the call happens in this module's scope.
    var colors <- [Color(r = 1.0, g = 0.0, b = 0.0), Color(r = 0.0, g = 1.0, b = 0.0), Color(r = 0.0, g = 0.0, b = 1.0)]
    var writer2 = new MemSerializer()
    var warch2 = Archive(reading = false, stream = writer2)
    warch2 |> serialize(colors)
    var adata <- writer2->extractData()
    print("  3 colors data size: {length(adata)} bytes (4 len + 3*3 = 13)\n")

    var reader2 = new MemSerializer(adata)
    var rarch2 = Archive(reading = true, stream = reader2)
    var loaded_colors : array<Color>
    rarch2 |> serialize(loaded_colors)
    for (lc in loaded_colors) {
        print("  color: r={lc.r}, g={lc.g}, b={lc.b}\n")
    }
}

[export]
def main() {
    demo_primitives()
    demo_struct()
    demo_arrays()
    demo_tables()
    demo_tuples()
    demo_variants()
    demo_nested()
    demo_manual_archive()
    demo_custom_serialize()
    print("done\n")
}

// output:
// === primitive serialization ===
//   int data size: 4 bytes
//   loaded int: 42
//   float data size: 4 bytes
//   loaded float: 3.14
//   string data size: 9 bytes
//   loaded string: "hello"
//
// === struct serialization ===
//   original: Hero, hp=100, pos=(1.5, 2.5)
//   serialized size: 20 bytes
//   loaded:   Hero, hp=100, pos=(1.5, 2.5)
//
// === array serialization ===
//   original scores: 10 20 30 40 50
//   serialized size: 24 bytes
//   loaded scores:   10 20 30 40 50
//
// === table serialization ===
//   serialized size: 44 bytes
//   loaded items:
//     sword: 1
//     potion: 5
//     arrow: 20
//
// === tuple serialization ===
//   tuple data size: 13 bytes
//   loaded: name="Alice", score=99
//
// === variant serialization ===
//   loaded1: int = 42
//   loaded2: string = "text"
//
// === nested struct serialization ===
//   game state size: 80 bytes
//   player: Knight, hp=80
//   level: 3
//   items: sword shield
//
// === manual archive usage ===
//   wrote 38 bytes
//   read: name="save_001", score=9999, tags=[rpg fantasy ]
// done
