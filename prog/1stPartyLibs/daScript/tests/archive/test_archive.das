options gen2
require strings
require daslib/archive
require dastest/testing_boost public

variant FooBar {
    a : float
    b : string
}

[test]
def test_save_and_load_variant(t : T?) {
    // save
    var reference = fixed_array(FooBar(a = 13.), FooBar(b = "hello"))
    var data <- mem_archive_save(reference)
    t |> equal(reference[0] as a, 13.)
    t |> equal(reference[1] as b, "hello")
    // load
    var test : FooBar[2]
    data |> mem_archive_load(test)
    delete data
    t |> equal(test[0] as a, 13.)
    t |> equal(test[1] as b, "hello")
}

[test]
def test_save_and_load_mem_table(t : T?) {
    // save
    var reference <- {
        "one" => 1,
        "two" => 2
    }
    var data <- mem_archive_save(reference)
    // load
    var test : table<string; int>
    data |> mem_archive_load(test)
    delete data
    t |> equal(length(test), length(reference))
    for (k in keys(reference)) {
        t |> equal(reference?[k] ?? -1, test?[k] ?? -100500)
    }
}

[test]
def test_save_and_load_mem_array(t : T?) {
    // save
    var reference <- [for (tt in range(10)); tt + 1]
    var data <- mem_archive_save(reference)
    // load
    var test : array<int>
    data |> mem_archive_load(test)
    delete data
    t |> equal(length(test), length(reference))
    for (i, j in test, reference) {
        t |> equal(i, j)
    }
}

[test]
def test_save_and_load_mem_dim(t : T?) {
    // save
    var reference = fixed_array<int>(1, 2, 3, 4, 5)
    var data <- mem_archive_save(reference)
    // load
    var test : int[5]
    data |> mem_archive_load(test)
    delete data
    for (i, j in test, range(5)) {
        t |> equal(i, j + 1)
    }
}

struct Foo {
    a : float
    b : string
}

[test]
def test_save_and_load_struct(t : T?) {
    // save
    var reference = Foo(a = 13., b = "hello")
    var data <- mem_archive_save(reference)
    t |> equal(reference.a, 13.)
    t |> equal(reference.b, "hello")
    // load
    var test : Foo
    data |> mem_archive_load(test)
    delete data
    t |> equal(test.a, 13.)
    t |> equal(test.b, "hello")
}

[test]
def test_save_and_load_tuple(t : T?) {
    // save
    var reference =  (13., "hello")
    var data <- mem_archive_save(reference)
    t |> equal(reference._0, 13.)
    t |> equal(reference._1, "hello")
    // load
    var test : tuple<float; string>
    data |> mem_archive_load(test)
    delete data
    t |> equal(test._0, 13.)
    t |> equal(test._1, "hello")
}

[test]
def test_save_and_load_mem_archive(t : T?) {
    // save
    var reference = 13ul
    var data <- mem_archive_save(reference)
    t |> equal(reference, 13ul)
    t |> equal(length(data), typeinfo sizeof(reference))
    // load
    var test : uint64
    data |> mem_archive_load(test)
    delete data
    t |> equal(reference, 13ul)
    t |> equal(test, 13ul)
}


def test_save_and_load_string(t : T?; ref : string) {
    // save
    var reference = ref
    var data <- mem_archive_save(reference)
    t |> equal(reference, ref)
    t |> equal(length(data), typeinfo sizeof(type<int>) + length(ref))
    // load
    var test : string
    data |> mem_archive_load(test)
    delete data
    t |> equal(reference, ref)
    t |> equal(test, ref)
}

[test]
def test_save_and_load_string(t : T?) {
    test_save_and_load_string(t, "13ul")
}

[test]
def test_save_and_load_empty_string(t : T?) {
    test_save_and_load_string(t, "")
}

[test]
def test_save_and_load_dim2d(t : T?) {
    t |> run("int 2D fixed array") @(t : T?) {
        var reference : int[3][2]
        reference[0][0] = 42
        reference[0][1] = 7
        reference[2][1] = 99
        var data <- mem_archive_save(reference)
        var test : int[3][2]
        data |> mem_archive_load(test)
        delete data
        t |> equal(test[0][0], 42)
        t |> equal(test[0][1], 7)
        t |> equal(test[1][0], 0)
        t |> equal(test[2][1], 99)
    }
    t |> run("string 2D fixed array") @(t : T?) {
        var reference : string[2][3]
        reference[0][0] = "hello"
        reference[1][2] = "world"
        var data <- mem_archive_save(reference)
        var test : string[2][3]
        data |> mem_archive_load(test)
        delete data
        t |> equal(test[0][0], "hello")
        t |> equal(test[0][1], "")
        t |> equal(test[1][2], "world")
    }
}

[test]
def test_save_and_load_dim3d(t : T?) {
    t |> run("int 3D fixed array") @(t : T?) {
        var reference : int[2][3][4]
        reference[0][0][0] = 1
        reference[1][2][3] = 42
        var data <- mem_archive_save(reference)
        var test : int[2][3][4]
        data |> mem_archive_load(test)
        delete data
        t |> equal(test[0][0][0], 1)
        t |> equal(test[1][2][3], 42)
        t |> equal(test[0][1][2], 0)
    }
}

struct DimTestStruct {
    values : float[2][3]
    name : string
}

[test]
def test_save_and_load_struct_with_dim(t : T?) {
    t |> run("struct containing 2D fixed array") @(t : T?) {
        var reference = DimTestStruct(name = "test")
        reference.values[0][0] = 1.0
        reference.values[1][2] = 3.14
        var data <- mem_archive_save(reference)
        var test : DimTestStruct
        data |> mem_archive_load(test)
        delete data
        t |> equal(test.name, "test")
        t |> equal(test.values[0][0], 1.0)
        t |> equal(test.values[1][2], 3.14)
        t |> equal(test.values[0][1], 0.0)
    }
}

[test]
def test_save_and_load_manual(t : T?) {
    var reference = 13
    // saving
    var mems = new MemSerializer()
    var sarch = Archive(reading = false, stream = mems)
    sarch |> serialize(reference)
    var data <- mems->extractData()
    t |> equal(length(data), typeinfo sizeof(reference))
    t |> equal(reference, 13)
    // restoring
    var test : int
    var memr = new MemSerializer(data)
    var rarch = Archive(reading = true, stream = memr)
    rarch |> serialize(test)
    if (!memr->OK()) {
        t->fatal(memr->getLastError())
    }
    delete data
    // compare
    t |> equal(reference, test)
    t |> equal(test, 13)
    t |> equal(reference, 13)
}

// ─── Bulk serialization tests (is_raw path) ───

struct RawPod {
    x : int
    y : int
    z : int
}

struct NonRawStruct {
    value : int
    name : string
}

[test]
def test_bulk_array_raw_pod(t : T?) {
    t |> run("array<int> bulk path") @(t : T?) {
        var reference <- [for (i in range(1000)); i * 7]
        var data <- mem_archive_save(reference)
        // bulk: 4 bytes length + 1000*4 bytes data = 4004
        t |> equal(length(data), 4 + 1000 * typeinfo sizeof(type<int>))
        var test : array<int>
        data |> mem_archive_load(test)
        delete data
        t |> equal(length(test), 1000)
        for (i in range(1000)) {
            t |> equal(test[i], i * 7)
        }
    }
    t |> run("array<float> bulk path") @(t : T?) {
        var reference : array<float>
        reference |> reserve(500)
        for (i in range(500)) {
            reference |> push(float(i) * 0.5)
        }
        var data <- mem_archive_save(reference)
        t |> equal(length(data), 4 + 500 * typeinfo sizeof(type<float>))
        var test : array<float>
        data |> mem_archive_load(test)
        delete data
        t |> equal(length(test), 500)
        t |> equal(test[0], 0.0)
        t |> equal(test[499], 249.5)
    }
    t |> run("array<RawPod> element-wise path (struct, not workhorse)") @(t : T?) {
        var reference : array<RawPod>
        reference |> reserve(100)
        for (i in range(100)) {
            reference |> push(RawPod(x = i, y = i * 2, z = i * 3))
        }
        var data <- mem_archive_save(reference)
        var test : array<RawPod>
        data |> mem_archive_load(test)
        delete data
        t |> equal(length(test), 100)
        t |> equal(test[0].x, 0)
        t |> equal(test[99].y, 198)
        t |> equal(test[50].z, 150)
    }
}

[test]
def test_bulk_array_non_raw(t : T?) {
    t |> run("array<string> element-wise path") @(t : T?) {
        var reference : array<string>
        reference |> push("hello")
        reference |> push("world")
        reference |> push("foo")
        var data <- mem_archive_save(reference)
        var test : array<string>
        data |> mem_archive_load(test)
        delete data
        t |> equal(length(test), 3)
        t |> equal(test[0], "hello")
        t |> equal(test[1], "world")
        t |> equal(test[2], "foo")
    }
    t |> run("array<NonRawStruct> element-wise path") @(t : T?) {
        var reference : array<NonRawStruct>
        reference |> push(NonRawStruct(value = 1, name = "one"))
        reference |> push(NonRawStruct(value = 2, name = "two"))
        var data <- mem_archive_save(reference)
        var test : array<NonRawStruct>
        data |> mem_archive_load(test)
        delete data
        t |> equal(length(test), 2)
        t |> equal(test[0].value, 1)
        t |> equal(test[0].name, "one")
        t |> equal(test[1].value, 2)
        t |> equal(test[1].name, "two")
    }
}

[test]
def test_bulk_array_empty(t : T?) {
    t |> run("empty array<int>") @(t : T?) {
        var reference : array<int>
        var data <- mem_archive_save(reference)
        t |> equal(length(data), 4)  // just the length (0)
        var test : array<int>
        data |> mem_archive_load(test)
        delete data
        t |> equal(length(test), 0)
    }
}

[test]
def test_bulk_dim_raw_pod(t : T?) {
    t |> run("int[5] bulk path") @(t : T?) {
        var reference = fixed_array(10, 20, 30, 40, 50)
        var data <- mem_archive_save(reference)
        t |> equal(length(data), 5 * typeinfo sizeof(type<int>))
        var test : int[5]
        data |> mem_archive_load(test)
        delete data
        for (i in range(5)) {
            t |> equal(test[i], (i + 1) * 10)
        }
    }
    t |> run("float[3] bulk path") @(t : T?) {
        var reference = fixed_array(1.5, 2.5, 3.5)
        var data <- mem_archive_save(reference)
        t |> equal(length(data), 3 * typeinfo sizeof(type<float>))
        var test : float[3]
        data |> mem_archive_load(test)
        delete data
        t |> equal(test[0], 1.5)
        t |> equal(test[1], 2.5)
        t |> equal(test[2], 3.5)
    }
    t |> run("RawPod[2] element-wise path (struct, not workhorse)") @(t : T?) {
        var reference = fixed_array(RawPod(x = 1, y = 2, z = 3), RawPod(x = 4, y = 5, z = 6))
        var data <- mem_archive_save(reference)
        var test : RawPod[2]
        data |> mem_archive_load(test)
        delete data
        t |> equal(test[0].x, 1)
        t |> equal(test[1].z, 6)
    }
    t |> run("int[2][3] 2D bulk path") @(t : T?) {
        var reference : int[2][3]
        reference[0][0] = 11
        reference[1][2] = 99
        var data <- mem_archive_save(reference)
        t |> equal(length(data), 2 * 3 * typeinfo sizeof(type<int>))
        var test : int[2][3]
        data |> mem_archive_load(test)
        delete data
        t |> equal(test[0][0], 11)
        t |> equal(test[1][2], 99)
        t |> equal(test[0][1], 0)
    }
}

[test]
def test_bulk_dim_non_raw(t : T?) {
    t |> run("string[3] element-wise path") @(t : T?) {
        var reference = fixed_array("a", "bb", "ccc")
        var data <- mem_archive_save(reference)
        var test : string[3]
        data |> mem_archive_load(test)
        delete data
        t |> equal(test[0], "a")
        t |> equal(test[1], "bb")
        t |> equal(test[2], "ccc")
    }
}

[test]
def test_bulk_array_int16(t : T?) {
    // Simulates the SF2 sample_data use case (array<int16>)
    t |> run("array<int16> bulk path") @(t : T?) {
        var reference : array<int16>
        reference |> reserve(10000)
        for (i in range(10000)) {
            reference |> push(int16(i % 32000))
        }
        var data <- mem_archive_save(reference)
        t |> equal(length(data), 4 + 10000 * typeinfo sizeof(type<int16>))
        var test : array<int16>
        data |> mem_archive_load(test)
        delete data
        t |> equal(length(test), 10000)
        t |> equal(int(test[0]), 0)
        t |> equal(int(test[9999]), 9999 % 32000)
        t |> equal(int(test[5000]), 5000)
    }
}
