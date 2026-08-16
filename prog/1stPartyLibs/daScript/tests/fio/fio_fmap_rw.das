options gen2
require dastest/testing_boost
require daslib/fio

// fmap_open_rw is the writable SHARED twin of fmap_open (PAGE_READWRITE / PROT_WRITE): writes
// through the mapping go back to the file via the page cache. Contract tests — write-through
// persistence, read parity with the read-only twin, decline shapes.

def private rw_pattern(i : int) : uint8 => uint8((i * 11 + 5) % 256)

[test]
def test_fmap_rw(t : T?) {
    t |> run("writes through the mapping persist to the file") @(t : T?) {
        var terr : string
        let tmp = temp_directory(terr)   // portable (no /tmp on Windows); cwd as a last resort
        let fname = path_join(empty(tmp) ? "." : tmp, "fio_fmap_rw_{ref_time_ticks()}.bin")
        let n = 65536
        var payload : array<uint8>
        payload |> resize(n)
        for (i in range(n)) {
            payload[i] = rw_pattern(i)
        }
        var wrote = false
        fopen(fname, "wb") $(f) {
            if (f != null) {
                fwrite(f, payload)
                wrote = true
            }
        }
        t |> success(wrote, "wrote the probe file")
        // map writable: verify content, then flip a sparse set of bytes through the mapping
        var msize = 0ul
        var base : void?
        unsafe {
            base = fmap_open_rw(fname, addr(msize))
        }
        t |> success(base != null, "fmap_open_rw maps")
        return if (base == null)
        t |> equal(uint64(n), msize, "mapped size is the file size")
        var bp = unsafe(reinterpret<uint8?>(base))
        var intact = true
        for (i in range(n)) {
            if (unsafe(bp[i]) != rw_pattern(i)) {
                intact = false
                break
            }
        }
        t |> success(intact, "mapped content matches what was written")
        var at = 0
        while (at < n) {
            unsafe {
                bp[at] = uint8(255 - int(rw_pattern(at)))
            }
            at += 4099
        }
        unsafe {
            fmap_close(base, msize)
        }
        // reopen READ-ONLY: the flips must have landed in the file itself
        var persisted = false
        fopen(fname, "rb") $(f) {
            if (f == null) {
                return
            }
            fmap(f) $(var bytes : array<uint8>#) {
                var ok = long_length(bytes) == int64(n)
                if (ok) {
                    for (i in range(n)) {
                        let want = i % 4099 == 0 ? uint8(255 - int(rw_pattern(i))) : rw_pattern(i)
                        if (bytes[i] != want) {
                            ok = false
                            break
                        }
                    }
                }
                persisted = ok
            }
        }
        t |> success(persisted, "writes persisted to the file after unmap")
        delete payload
        remove(fname)
    }
    t |> run("decline shapes: missing and empty files map as null") @(t : T?) {
        var msize = 1ul
        var base : void?
        unsafe {
            base = fmap_open_rw("no_such_fio_fmap_rw_probe.bin", addr(msize))
        }
        t |> success(base == null && msize == 0ul, "missing file declines with null, size 0")
        var terr : string
        let tmp = temp_directory(terr)
        let fname = path_join(empty(tmp) ? "." : tmp, "fio_fmap_rw_empty_{ref_time_ticks()}.bin")
        fopen(fname, "wb") $(f) {
            feint("{f != null}")   // an empty file: created, nothing written
        }
        unsafe {
            base = fmap_open_rw(fname, addr(msize))
        }
        t |> success(base == null, "empty file declines with null (mmap(0) is invalid)")
        remove(fname)
    }
}
