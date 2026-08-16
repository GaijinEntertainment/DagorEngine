options gen2
require dastest/testing_boost
require daslib/fio

// prefetch_map is ADVISORY readahead — these are contract tests (acceptance + intact reads +
// degenerate-argument declines), not timing: the OS is free to prefetch lazily or not at all.

[test]
def test_prefetch_map(t : T?) {
    t |> run("prefetch over a real mapping is accepted and reads stay intact") @(t : T?) {
        var terr : string
        let tmp = temp_directory(terr)   // portable (no /tmp on Windows); cwd as a last resort
        let fname = path_join(empty(tmp) ? "." : tmp, "fio_prefetch_{ref_time_ticks()}.bin")
        let n = 65536
        var payload : array<uint8>
        payload |> resize(n)
        for (i in range(n)) {
            payload[i] = uint8(i % 251)
        }
        var wrote = false
        fopen(fname, "wb") $(f) {
            if (f != null) {
                fwrite(f, payload)
                wrote = true
            }
        }
        t |> success(wrote, "wrote the probe file")
        var mapped = false
        fopen(fname, "rb") $(f) {
            if (f == null) {
                return
            }
            fmap(f) $(var bytes : array<uint8>#) {
                mapped = true
                var armed = false
                unsafe {
                    armed = prefetch_map(addr<void?>(bytes[0]), uint64(long_length(bytes)))
                }
                t |> success(armed, "the OS accepted the advisory prefetch")
                var intact = length(bytes) == n
                for (i in range(n)) {
                    if (bytes[i] != uint8(i % 251)) {
                        intact = false
                        break
                    }
                }
                t |> success(intact, "mapped content intact after prefetch")
            }
        }
        t |> success(mapped, "mapped the probe file")
        delete payload
        remove(fname)
    }
    t |> run("degenerate arguments decline instead of crashing") @(t : T?) {
        unsafe {
            t |> success(!prefetch_map(null, 0ul), "null base + zero bytes declines")
        }
    }
}
