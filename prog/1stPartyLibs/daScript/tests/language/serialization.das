options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
options persistent_heap

// todo: resize(array<Color>) is becoming
// unsafe under coverage.
options no_coverage

require dastest/testing_boost public
require daslib/archive
require strings
require math

struct Player {
    name : string
    score : int
    alive : bool
}

// Custom serialize: Color stores floats but serializes as 3 bytes
struct Color {
    r : float = 0.0
    g : float = 0.0
    b : float = 0.0
}

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

[test]
def test_primitive_round_trip(t : T?) {
    t |> run("int round trip") @(t : T?) {
        var val = 42
        var buf <- mem_archive_save(val)
        var loaded = 0
        mem_archive_load(buf, loaded)
        t |> equal(loaded, 42)
    }
    t |> run("float round trip") @(t : T?) {
        var val = 3.14
        var buf <- mem_archive_save(val)
        var loaded = 0.0
        mem_archive_load(buf, loaded)
        t |> equal(loaded, 3.14)
    }
    t |> run("string round trip") @(t : T?) {
        var val = "hello"
        var buf <- mem_archive_save(val)
        var loaded = ""
        mem_archive_load(buf, loaded)
        t |> equal(loaded, "hello")
    }
}

[test]
def test_struct_round_trip(t : T?) {
    t |> run("Player struct") @(t : T?) {
        var p = Player(name = "hero", score = 100, alive = true)
        var buf <- mem_archive_save(p)
        var loaded : Player
        mem_archive_load(buf, loaded)
        t |> equal(loaded.name, "hero")
        t |> equal(loaded.score, 100)
        t |> equal(loaded.alive, true)
    }
}

[test]
def test_array_round_trip(t : T?) {
    t |> run("int array") @(t : T?) {
        var arr <- [10, 20, 30]
        var buf <- mem_archive_save(arr)
        var loaded : array<int>
        mem_archive_load(buf, loaded)
        t |> equal(length(loaded), 3)
        t |> equal(loaded[0], 10)
        t |> equal(loaded[1], 20)
        t |> equal(loaded[2], 30)
    }
}

[test]
def test_table_round_trip(t : T?) {
    t |> run("string-int table") @(t : T?) {
        var tab : table<string; int>
        tab |> insert("a", 1)
        tab |> insert("b", 2)
        var buf <- mem_archive_save(tab)
        var loaded : table<string; int>
        mem_archive_load(buf, loaded)
        t |> equal(key_exists(loaded, "a"), true)
        t |> equal(key_exists(loaded, "b"), true)
        unsafe {
            t |> equal(loaded["a"], 1)
            t |> equal(loaded["b"], 2)
        }
    }
}

[test]
def test_tuple_round_trip(t : T?) {
    t |> run("tuple int,string") @(t : T?) {
        var tup = (42, "answer")
        var buf <- mem_archive_save(tup)
        var loaded : tuple<int; string>
        mem_archive_load(buf, loaded)
        t |> equal(loaded._0, 42)
        t |> equal(loaded._1, "answer")
    }
}

[test]
def test_manual_archive(t : T?) {
    t |> run("MemSerializer write then read") @(t : T?) {
        // Write phase
        var writer = new MemSerializer()
        var warch = Archive(reading = false, stream = writer)
        var name = "test"
        var score = 42
        warch |> serialize(name)
        warch |> serialize(score)

        // Read phase
        var data <- writer->extractData()
        var reader = new MemSerializer(data)
        var rarch = Archive(reading = true, stream = reader)
        var rname = ""
        var rscore = 0
        rarch |> serialize(rname)
        rarch |> serialize(rscore)
        t |> equal(rname, "test")
        t |> equal(rscore, 42)
    }
}

[test]
def test_custom_serialize(t : T?) {
    t |> run("Color packs to 3 bytes") @(t : T?) {
        var c = Color(r = 1.0, g = 0.5, b = 0.0)
        var writer = new MemSerializer()
        var warch = Archive(reading = false, stream = writer)
        warch |> serialize(c)
        var data <- writer->extractData()
        // 3 bytes, not 12 (3 floats × 4)
        t |> equal(length(data), 3)
        // r=1.0 -> 255, g=0.5 -> 127, b=0.0 -> 0
        t |> equal(data[0], uint8(0xff))
        t |> equal(data[1], uint8(0x7f))
        t |> equal(data[2], uint8(0x0))
    }
    t |> run("Color round trip") @(t : T?) {
        var c = Color(r = 1.0, g = 0.0, b = 1.0)
        var writer = new MemSerializer()
        var warch = Archive(reading = false, stream = writer)
        warch |> serialize(c)
        var data <- writer->extractData()
        var reader = new MemSerializer(data)
        var rarch = Archive(reading = true, stream = reader)
        var loaded = Color()
        rarch |> serialize(loaded)
        t |> equal(loaded.r, 1.0)
        t |> equal(loaded.g, 0.0)
        t |> equal(loaded.b, 1.0)
    }
    t |> run("Color array uses custom serialize") @(t : T?) {
        unsafe {
            var colors <- [Color(r = 1.0, g = 0.0, b = 0.0), Color(r = 0.0, g = 1.0, b = 0.0)]
            var writer = new MemSerializer()
            var warch = Archive(reading = false, stream = writer)
            warch |> serialize(colors)
            var data <- writer->extractData()
            // 4 bytes array length + 2 × 3 bytes = 10
            t |> equal(length(data), 10)
            var reader = new MemSerializer(data)
            var rarch = Archive(reading = true, stream = reader)
            var loaded : array<Color>
            rarch |> serialize(loaded)
            t |> equal(length(loaded), 2)
            t |> equal(loaded[0].r, 1.0)
            t |> equal(loaded[0].g, 0.0)
            t |> equal(loaded[1].g, 1.0)
            t |> equal(loaded[1].b, 0.0)
        }
    }
}
