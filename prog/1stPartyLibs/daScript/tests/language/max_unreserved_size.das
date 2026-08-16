options gen2
options max_unreserved_size = 65536

require dastest/testing_boost
require strings
require daslib/archive

let LIMIT = 65536

struct Empty {}

struct BigPayload {
    samples : array<uint8>
}

// The unreserved-resize guard: a resize that has to GROW past max_unreserved_size bytes
// panics; a prior exact reserve means the resize never grows, so it passes at any size.
// The 64 KB option above keeps every case cheap; the shipped default is 64 MB.

[test]
def test_unreserved_resize_panics(t : T?) {
    t |> run("bare growing resize past the threshold panics") @(t : T?) {
        var failed = false
        try {
            var a : array<uint8>
            a |> resize(LIMIT + 1)
        } recover {
            failed = true
        }
        t |> success(failed, "resize over the byte threshold must panic")
    }
    t |> run("resize_no_init is the same growth path") @(t : T?) {
        var failed = false
        try {
            var a : array<uint8>
            a |> resize_no_init(LIMIT + 1)
        } recover {
            failed = true
        }
        t |> success(failed, "resize_no_init over the threshold must panic")
    }
    t |> run("the int64 overload is the same growth path") @(t : T?) {
        var failed = false
        try {
            var a : array<uint8>
            a |> resize(int64(LIMIT) + 1l)
        } recover {
            failed = true
        }
        t |> success(failed, "int64 resize over the threshold must panic")
    }
    t |> run("stride counts toward the byte threshold") @(t : T?) {
        var failed = false
        try {
            var a : array<int>
            a |> resize(LIMIT / 4 + 1)  // one int past the threshold: it counts bytes, not elements
        } recover {
            failed = true
        }
        t |> success(failed, "the guard measures newSize * stride")
    }
}

[test]
def test_reserved_and_small_resizes_pass(t : T?) {
    t |> run("reserve-then-resize passes at any size, capacity exact") @(t : T?) {
        var a : array<uint8>
        a |> reserve(200000)
        a |> resize(200000)
        t |> equal(length(a), 200000)
        t |> equal(capacity(a), 200000)
    }
    t |> run("below the threshold resize passes, capacity pow2") @(t : T?) {
        var a : array<int>
        a |> resize(1000)               // 4000 bytes, well under the 64 KB threshold
        t |> equal(capacity(a), 1024)
    }
    t |> run("ensure_capacity append crosses the threshold without panic") @(t : T?) {
        var a : array<uint8>
        var size = 0
        while (size < 200000) {
            size += 30000
            a |> ensure_capacity(size)
            a |> resize(size)
        }
        t |> equal(length(a), 210000)
        t |> success(capacity(a) >= 210000)
    }
    t |> run("zero-stride elements never trip the guard") @(t : T?) {
        var a : array<Empty>
        a |> resize(1000000)            // 0 bytes total: the stride sub-guard skips
        t |> equal(length(a), 1000000)
    }
    // upstream also covers to_bytes(string) reserving exactly; that builtin is past our
    // vendored base, so the case is dropped until the next daScript sync brings it in
    t |> run("binary_load reserves exactly past the threshold") @(t : T?) {
        var src : BigPayload
        src.samples |> reserve(200000)
        src.samples |> resize(200000)
        var dst : BigPayload
        binary_save(src) <| $(data) {
            binary_load(dst, data)
        }
        t |> equal(length(dst.samples), 200000)
        delete src
        delete dst
    }
    t |> run("MemSerializer.write appends past the threshold") @(t : T?) {
        var src : BigPayload
        src.samples |> reserve(200000)
        src.samples |> resize(200000)
        var bytes <- mem_archive_save(src)
        t |> success(length(bytes) > 200000, "the archive carries the payload")
        var back : BigPayload
        mem_archive_load(bytes, back)
        t |> equal(length(back.samples), 200000)
        delete src
        delete back
        delete bytes
    }
    t |> run("shrink never panics regardless of size") @(t : T?) {
        var a : array<uint8>
        a |> reserve(200000)
        a |> resize(200000)
        a |> resize(100000)             // shrink of an over-threshold array is not a growth
        t |> equal(length(a), 100000)
        a |> pop()
        t |> equal(length(a), 99999)
    }
}

[test]
def test_max_unreserved_size_api(t : T?) {
    t |> run("option, getter, and runtime setter agree") @(t : T?) {
        t |> equal(max_unreserved_size(), uint64(LIMIT))
        set_max_unreserved_size(uint64(LIMIT * 16))
        t |> equal(max_unreserved_size(), uint64(LIMIT * 16))
        var ok : array<uint8>
        ok |> resize(100000)            // under the raised limit now
        t |> equal(length(ok), 100000)
        set_max_unreserved_size(uint64(LIMIT))
        var failed = false
        try {
            var a : array<uint8>
            a |> resize(100000)
        } recover {
            failed = true
        }
        t |> success(failed, "lowering the limit back re-arms the guard")
    }
}
