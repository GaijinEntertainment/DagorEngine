options gen2
options _function_length = 0   // test bodies enumerate the fixture arms
require dastest/testing_boost
require daslib/fio

// dwrite is the cache-bypassing sequential appender (bounce bands re-block any caller size to
// the sector alignment NO_BUFFERING-class paths demand; full bands flush on a writer thread).
// These are mechanics tests — round-trip content, the append-only + close-truncate contract,
// the band/commit producer path, stat counters — not timing.

def private dwrite_pattern(i : int) : uint8 => uint8((i * 7 + 13) % 256)

def private make_probe_path(tag : string) : string {
    var terr : string
    let tmp = temp_directory(terr)   // portable (no /tmp on Windows); cwd as a last resort
    return path_join(empty(tmp) ? "." : tmp, "fio_dwrite_{tag}_{ref_time_ticks()}.bin")
}

def private verify_file_bytes(t : T?; fname : string; total : int; tag : string) {
    var checked = false
    fopen(fname, "rb") $(f) {
        if (f == null) {
            return
        }
        fmap(f) $(var bytes : array<uint8>#) {
            var intact = long_length(bytes) == int64(total)
            t |> success(intact, "{tag}: file is exactly the appended size")
            if (intact) {
                for (i in range(total)) {
                    if (bytes[i] != dwrite_pattern(i)) {
                        intact = false
                        break
                    }
                }
            }
            t |> success(intact, "{tag}: content round-trips byte-for-byte")
            checked = true
        }
    }
    t |> success(checked, "{tag}: mapped the written file back")
}

[test]
def test_dwrite(t : T?) {
    t |> run("odd-size appends round-trip and close truncates the preallocation") @(t : T?) {
        let fname = make_probe_path("odd")
        // preallocate far more than we append — close must cut the file back
        var w = unsafe(dwrite_open(fname, 1048576ul, 8192ul))
        t |> success(w != null, "writer opened")
        return if (w == null)
        // odd sizes on both sides of the band: sub-sector, near-band, multi-band
        var sizes <- [1, 3, 4095, 8192, 65537, 12345, 100000]
        var payload : array<uint8>
        var total = 0
        var ok = true
        for (sz in sizes) {
            payload |> resize(sz)
            for (i in range(sz)) {
                payload[i] = dwrite_pattern(total + i)
            }
            ok = ok && unsafe(dwrite_append(w, unsafe(addr<void?>(payload[0])), uint64(sz)))
            total += sz
        }
        t |> success(ok, "every append accepted")
        // stat accounting: the direct-vs-bounce split is platform-dependent (win32 demands
        // sector-aligned sources, the POSIX arm takes any band-sized run direct) — the portable
        // contract is conservation, plus the sub-sector pieces must stage through the band
        t |> equal(uint64(total), unsafe(dwrite_stat(w, 2)) + unsafe(dwrite_stat(w, 3)), "direct + bounce == appended bytes")
        t |> success(unsafe(dwrite_stat(w, 3)) > 0ul, "sub-sector pieces staged through the band")
        t |> success(unsafe(dwrite_close(w)), "close flushed the tail and truncated")
        verify_file_bytes(t, fname, total, "odd-size appends")
        delete payload
        delete sizes
        remove(fname)
    }
    t |> run("the band/commit producer path crosses band boundaries intact") @(t : T?) {
        let fname = make_probe_path("band")
        var w = unsafe(dwrite_open(fname, 0ul, 8192ul))
        t |> success(w != null, "writer opened")
        return if (w == null)
        // build bytes straight into the writer's own staging band, committing uneven runs so
        // several commits land exactly on a band boundary (the async flush + swap point)
        var total = 0
        var ok = true
        while (total < 50000) {
            var avail = 0ul
            var band = unsafe(dwrite_band(w, unsafe(addr(avail))))
            if (band == null || avail == 0ul) {
                ok = false
                break
            }
            var take = 3000
            if (uint64(take) > avail) {
                take = int(avail)   // top the band off exactly — forces the boundary flush
            }
            var bp = unsafe(reinterpret<uint8?>(band))
            for (i in range(take)) {
                unsafe {
                    bp[i] = dwrite_pattern(total + i)
                }
            }
            ok = ok && unsafe(dwrite_commit(w, uint64(take)))
            total += take
        }
        t |> success(ok, "every band commit accepted")
        t |> success(unsafe(dwrite_close(w)), "close flushed the tail")
        verify_file_bytes(t, fname, total, "band/commit")
        remove(fname)
    }
    t |> run("append and band/commit interleave in order") @(t : T?) {
        let fname = make_probe_path("mixed")
        var w = unsafe(dwrite_open(fname, 0ul, 8192ul))
        t |> success(w != null, "writer opened")
        return if (w == null)
        var payload : array<uint8>
        var total = 0
        var ok = true
        for (round in range(6)) {
            let sz = 5000 + round * 777
            payload |> resize(sz)
            for (i in range(sz)) {
                payload[i] = dwrite_pattern(total + i)
            }
            ok = ok && unsafe(dwrite_append(w, unsafe(addr<void?>(payload[0])), uint64(sz)))
            total += sz
            var avail = 0ul
            var band = unsafe(dwrite_band(w, unsafe(addr(avail))))
            if (band == null || avail < 100ul) {
                ok = false
                break
            }
            var bp = unsafe(reinterpret<uint8?>(band))
            for (i in range(100)) {
                unsafe {
                    bp[i] = dwrite_pattern(total + i)
                }
            }
            ok = ok && unsafe(dwrite_commit(w, 100ul))
            total += 100
        }
        t |> success(ok, "interleaved appends and commits accepted")
        t |> success(unsafe(dwrite_close(w)), "close flushed the tail")
        verify_file_bytes(t, fname, total, "interleaved")
        delete payload
        remove(fname)
    }
    t |> run("degenerate opens decline instead of crashing") @(t : T?) {
        // a directory that cannot exist — open returns null, no throw
        let w = unsafe(dwrite_open("Z:/no/such/dir/ever/fio_dwrite_probe.bin", 0ul, 0ul))
        t |> success(w == null, "impossible path declines with null")
    }
    t |> run("a single append past 2 GiB survives (DASTEST_BIG_IO=1)") @(t : T?) {
        // macOS write(2) REJECTS nbyte > INT_MAX with EINVAL (Linux merely truncates), so an
        // unchunked raw writer dies exactly on a big single append — an 11 GB image plane read
        // as "disk full". A das array caps at int elements (why no unit ever hit this), so the
        // >2 GiB source comes from an fmap of a staged file. ~4.2 GB of disk traffic — opt-in;
        // the M-box bring-ups run it.
        if (empty(get_env_variable("DASTEST_BIG_IO"))) {
            t |> success(true, "skipped — set DASTEST_BIG_IO=1 (writes ~4.2 GB)")
            return
        }
        let half = 1073743872l                  // 1 GiB + 2048: two of these cross INT_MAX
        let big = 2l * half
        let src_name = make_probe_path("bigsrc")
        var chunk : array<uint8>
        chunk |> resize(int(half))
        for (i in range(4096)) {
            chunk[i] = dwrite_pattern(i)
            chunk[int(half) - 1 - i] = dwrite_pattern(i)
        }
        var sw = unsafe(dwrite_open(src_name, uint64(big), 8ul * 1024ul * 1024ul))
        t |> success(sw != null, "stage writer opened")
        if (sw == null) {
            delete chunk
            return
        }
        var staged = unsafe(dwrite_append(sw, unsafe(addr<void?>(chunk[0])), uint64(half)))
        staged = staged && unsafe(dwrite_append(sw, unsafe(addr<void?>(chunk[0])), uint64(half)))
        staged = staged && unsafe(dwrite_close(sw))
        delete chunk
        t |> success(staged, "staged the 2.1 GiB source in sub-2GiB appends")
        if (!staged) {
            remove(src_name)
            return
        }
        // the actual regression: ONE append of the whole >2 GiB mapping
        let dst_name = make_probe_path("bigdst")
        var appended = false
        var closed = false
        fopen(src_name, "rb") $(f) {
            if (f == null) {
                return
            }
            fmap(f) $(var bytes : array<uint8>#) {
                var w = unsafe(dwrite_open(dst_name, uint64(big), 8ul * 1024ul * 1024ul))
                if (w == null) {
                    return
                }
                appended = unsafe(dwrite_append(w, unsafe(addr<void?>(bytes[0])), uint64(big)))
                closed = unsafe(dwrite_close(w))
            }
        }
        t |> success(appended, ">2 GiB single append accepted")
        t |> success(closed, "close flushed and truncated")
        t |> equal(int64(stat(dst_name).size), big)
        var seams_ok = true
        fopen(dst_name, "rb") $(f) {
            if (f == null) {
                seams_ok = false
                return
            }
            fmap(f) $(var bytes : array<uint8>#) {
                // the two halves are identical staged chunks: check both copies' seams
                for (i in range(4096)) {
                    seams_ok = seams_ok && bytes[i] == dwrite_pattern(i)
                    unsafe {
                        let bp = addr(bytes[0])
                        seams_ok = seams_ok && bp[big - 1l - int64(i)] == dwrite_pattern(i)
                        seams_ok = seams_ok && bp[half - 1l - int64(i)] == dwrite_pattern(i)
                        seams_ok = seams_ok && bp[half + int64(i)] == dwrite_pattern(i)
                    }
                }
            }
        }
        t |> success(seams_ok, "all four half-seams intact after the big append")
        remove(src_name)
        remove(dst_name)
    }
}
