options gen2
require dastest/testing_boost
require daslib/fio

// Regression: file IO past 2 GiB. The pre-fix rail computed byte counts as int32
// `length * sizeof`, which wraps negative past INT_MAX; the C runtime sign-extends that to
// a huge size_t and streams out-of-bounds heap garbage into the file (or back over the heap
// on read) with no diagnostic. Now: long_fwrite/long_fread carry int64 counts end to end,
// the int32 fwrite/fread PANIC past INT_MAX, and ftell/fseek use the 64-bit C forms (plain
// ftell/fseek take `long` = 32-bit on Windows).
//
// Costs ~2.2 GB of RAM and of disk for a few seconds, so it only runs when DAS_BIG_IO=1 —
// the dasLLAMA image suite exercises the same rail end-to-end on real >2 GiB weight planes.

let BIG = int64(0x80000000) + 65536l   // just past the int32 wrap
let PROBE_STEP = int64(0x2000000)      // spot-check every 32 MiB — full compare would double the cost

// portable probe path: temp_directory + path_join (cwd as a last resort — never a bare "/name")
def private big_io_path(name : string) : string {
    var terr : string
    let tmp = temp_directory(terr)
    return path_join(empty(tmp) ? "." : tmp, name)
}

[test]
def test_big_write_read(t : T?) {
    if (get_env_variable("DAS_BIG_IO") != "1") {
        t |> skip("big-IO regression needs DAS_BIG_IO=1 (2.2 GB RAM + disk)")
        return
    }
    t |> run("long_fwrite/long_fread past 2 GiB") @@(t : T?) {
        let path = big_io_path("das_big_io_test.bin")
        var buf : array<uint8>
        buf |> resize(BIG)
        for (k in range64(0l, BIG / PROBE_STEP)) {
            buf[k * PROBE_STEP] = uint8(k + 1l)
        }
        buf[BIG - 1l] = uint8(0xAB)
        fopen(path, "wb") $(f) {
            t |> success(f != null)
            t |> equal(f |> long_fwrite(buf), BIG)
        }
        t |> equal(int64(stat(path).size), BIG)   // pre-fix: garbage-padded file, wrong size
        for (k in range64(0l, BIG / PROBE_STEP)) {
            buf[k * PROBE_STEP] = uint8(0)
        }
        buf[BIG - 1l] = uint8(0)
        fopen(path, "rb") $(f) {
            t |> success(f != null)
            t |> equal(f |> long_fread(buf), BIG)
        }
        for (k in range64(0l, BIG / PROBE_STEP)) {
            t |> equal(buf[k * PROBE_STEP], uint8(k + 1l))
        }
        t |> equal(buf[BIG - 1l], uint8(0xAB))
        delete buf
        t |> success(remove(path))   // a stuck multi-GiB temp file poisons later runs
    }
    t |> run("int32 fwrite/fread panic past 2 GiB") @@(t : T?) {
        let path = big_io_path("das_big_io_panic.bin")
        var buf : array<uint8>
        buf |> resize(BIG)
        var wrote_panicked = false
        fopen(path, "wb") $(f) {
            t |> success(f != null)
            try {
                f |> fwrite(buf)
            } recover {
                wrote_panicked = true
            }
        }
        t |> success(wrote_panicked)
        var read_panicked = false
        fopen(path, "rb") $(f) {
            t |> success(f != null)
            try {
                f |> fread(buf)
            } recover {
                read_panicked = true
            }
        }
        t |> success(read_panicked)
        delete buf
        t |> success(remove(path))   // a stuck multi-GiB temp file poisons later runs
    }
    t |> run("fseek/ftell past 2 GiB") @@(t : T?) {
        let path = big_io_path("das_big_io_seek.bin")
        let pos = int64(0xC0000000)   // 3 GiB — sparse on every filesystem we test on
        fopen(path, "wb") $(f) {
            t |> success(f != null)
            fseek(f, pos, seek_set)
            f |> fwrite(uint8(7))
            t |> equal(ftell(f), pos + 1l)   // pre-fix on Windows: 32-bit ftell truncates
        }
        t |> equal(int64(stat(path).size), pos + 1l)
        t |> success(remove(path))   // a stuck multi-GiB temp file poisons later runs
    }
}
