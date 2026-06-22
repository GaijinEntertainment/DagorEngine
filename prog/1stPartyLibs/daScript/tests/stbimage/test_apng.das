options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage
require strings
require daslib/safe_addr
require daslib/fio
require dastest/testing_boost public

// Tiny chunk parser. Walks the PNG byte stream once, populating a count
// per chunk type plus the body of acTL (the only chunk we backpatch).
struct ChunkSummary {
    actl_body  : array<uint8>    // 8 bytes: num_frames(u32 BE) + num_plays(u32 BE)
    n_ihdr     : int
    n_actl     : int
    n_fctl     : int
    n_idat     : int
    n_fdat     : int
    n_iend     : int
    bad        : bool             // signature mismatch or truncated stream
}

def be_u32_at(b0, b1, b2, b3 : uint8) : uint {
    return ((uint(b0) << 24u) | (uint(b1) << 16u) | (uint(b2) << 8u) | uint(b3))
}

def parse_png_chunks(path : string) : ChunkSummary {
    var out : ChunkSummary
    var arr : array<uint8>
    // Read file as raw bytes — fread(path : string) returns a das_string that
    // peek_data truncates at the first NUL. Use sized fread(f, buf) instead.
    let sz = int(stat(path).size)
    if (sz < 8) {
        out.bad = true
        return <- out
    }
    arr |> resize(sz)
    fopen(path, "rb") $(f) {
        if (f != null) {
            fread(f, arr)
        }
    }
    let n = length(arr)
    // Signature: 137 80 78 71 13 10 26 10
    if (n < 8 || int(arr[0]) != 137 || int(arr[1]) != 80 || int(arr[2]) != 78 || int(arr[3]) != 71) {
        out.bad = true
        return <- out
    }
    var p = 8
    while (p + 8 <= n) {
        let len = int(be_u32_at(arr[p + 0], arr[p + 1], arr[p + 2], arr[p + 3]))
        let t0 = arr[p + 4]; let t1 = arr[p + 5]; let t2 = arr[p + 6]; let t3 = arr[p + 7]
        let body_start = p + 8
        let crc_start  = body_start + len
        if (crc_start + 4 > n) {
            out.bad = true
            return <- out
        }
        // Cheap 4-char dispatch — compare bytes directly. IHDR/acTL/fcTL/IDAT/fdAT/IEND.
        if (t0 == 73u8 && t1 == 72u8 && t2 == 68u8 && t3 == 82u8) {
            out.n_ihdr++
        } elif (t0 == 97u8 && t1 == 99u8 && t2 == 84u8 && t3 == 76u8) {
            out.n_actl++
            if (len == 8) {
                out.actl_body |> resize(8)
                for (i in range(8)) {
                    out.actl_body[i] = arr[body_start + i]
                }
            }
        } elif (t0 == 102u8 && t1 == 99u8 && t2 == 84u8 && t3 == 76u8) {
            out.n_fctl++
        } elif (t0 == 73u8 && t1 == 68u8 && t2 == 65u8 && t3 == 84u8) {
            out.n_idat++
        } elif (t0 == 102u8 && t1 == 100u8 && t2 == 65u8 && t3 == 84u8) {
            out.n_fdat++
        } elif (t0 == 73u8 && t1 == 69u8 && t2 == 78u8 && t3 == 68u8) {
            out.n_iend++
            return <- out
        }
        p = crc_start + 4
    }
    return <- out
}

def make_gradient_frame(var pixels : array<uint8>; w : int; h : int; frame_idx : int) {
    pixels |> resize(w * h * 4)
    for (y in range(h)) {
        for (x in range(w)) {
            let i = (y * w + x) * 4
            pixels[i + 0] = uint8((x + frame_idx * 16) & int(0xFF))
            pixels[i + 1] = uint8(y & int(0xFF))
            pixels[i + 2] = uint8(frame_idx * 64)
            pixels[i + 3] = 0xFFu8
        }
    }
}

[test]
def test_apng_basic(t : T?) {
    t |> run("APNG: encode 4 frames, parse chunks, decode frame 0") <| @(tt : T?) {
        let fname = "{get_das_root()}/tests/stbimage/_test_output.apng"
        let W = 32; let H = 32
        var writer = unsafe(stbi_apng_begin(fname, W, H, 4))
        tt |> success(writer != null, "stbi_apng_begin succeeded")
        return if (writer == null)
        for (frame in range(4)) {
            var pixels : array<uint8>
            make_gradient_frame(pixels, W, H, frame)
            var ok : int
            unsafe {
                ok = stbi_apng_frame(writer, addr(pixels[0]), W * 4, 100)
            }
            tt |> equal(ok, 1)
        }
        let end_ok = unsafe(stbi_apng_end(writer))
        tt |> equal(end_ok, 1)

        // Frame 0 decodes as a normal PNG (default-image fallback)
        var x = 0; var y = 0; var comp = 0
        var loaded = stbi_load(fname, safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(loaded != null, "frame 0 decodes as PNG")
        if (loaded != null) {
            tt |> equal(x, W)
            tt |> equal(y, H)
            stbi_image_free(loaded)
        }

        // Chunk parse: 1 IHDR, 1 acTL with num_frames=4, 4 fcTL, 1 IDAT, 3 fdAT, 1 IEND
        let chunks = parse_png_chunks(fname)
        tt |> success(!chunks.bad, "chunk parse: signature ok, stream not truncated")
        tt |> equal(chunks.n_ihdr, 1)
        tt |> equal(chunks.n_actl, 1)
        tt |> equal(chunks.n_fctl, 4)
        tt |> equal(chunks.n_idat, 1)
        tt |> equal(chunks.n_fdat, 3)
        tt |> equal(chunks.n_iend, 1)
        // acTL backpatch: num_frames == 4
        tt |> equal(length(chunks.actl_body), 8)
        if (length(chunks.actl_body) == 8) {
            let nf = be_u32_at(chunks.actl_body[0], chunks.actl_body[1], chunks.actl_body[2], chunks.actl_body[3])
            tt |> equal(int(nf), 4)
        }
        remove(fname)
    }
}

[test]
def test_apng_rgb_channels(t : T?) {
    t |> run("APNG: 3-channel RGB encode works") <| @(tt : T?) {
        let fname = "{get_das_root()}/tests/stbimage/_test_output_rgb.apng"
        let W = 8; let H = 8
        var writer = unsafe(stbi_apng_begin(fname, W, H, 3))
        tt |> success(writer != null, "begin RGB succeeded")
        return if (writer == null)
        var pixels : array<uint8>
        pixels |> resize(W * H * 3)
        for (i in range(W * H * 3)) {
            pixels[i] = uint8(i & int(0xFF))
        }
        unsafe {
            stbi_apng_frame(writer, addr(pixels[0]), W * 3, 50)
            stbi_apng_frame(writer, addr(pixels[0]), W * 3, 50)
        }
        let end_ok = unsafe(stbi_apng_end(writer))
        tt |> equal(end_ok, 1)
        let chunks = parse_png_chunks(fname)
        tt |> equal(chunks.n_ihdr, 1)
        tt |> equal(chunks.n_idat, 1)
        tt |> equal(chunks.n_fdat, 1)
        remove(fname)
    }
}

[test]
def test_apng_begin_bad_filename(t : T?) {
    t |> run("APNG: begin with unwritable path returns null") <| @(tt : T?) {
        // A path that fopen("wb") refuses: the path IS a directory.
        let writer = unsafe(stbi_apng_begin(get_das_root(), 16, 16, 4))
        tt |> success(writer == null, "begin on bad path returns null, no crash")
    }
}

[test]
def test_apng_drop_accounting(t : T?) {
    t |> run("APNG: submitted == emitted + dropped (worker race-tolerant invariant)") <| @(tt : T?) {
        let fname = "{get_das_root()}/tests/stbimage/_test_output_overflow.apng"
        // Big-ish frame so deflate takes noticeable time vs. enqueue speed.
        let W = 512; let H = 512
        var writer = unsafe(stbi_apng_begin(fname, W, H, 4))
        if (writer == null) {
            tt |> success(false, "begin failed")
            return
        }
        var pixels : array<uint8>
        pixels |> resize(W * H * 4)
        // Random-ish content so deflate has actual work to do
        for (i in range(W * H * 4)) {
            pixels[i] = uint8((i * 1103515245 + 12345) & int(0xFF))
        }
        // Submit 40 frames as fast as possible — queue cap is 4, so we expect drops.
        for (_i in range(40)) {
            unsafe {
                stbi_apng_frame(writer, addr(pixels[0]), W * 4, 33)
            }
        }
        let dropped_before_end = unsafe(stbi_apng_dropped(writer))
        let end_ok = unsafe(stbi_apng_end(writer))
        tt |> equal(end_ok, 1)
        // File is valid even when many frames dropped — verify chunk parse succeeds.
        let chunks = parse_png_chunks(fname)
        tt |> success(!chunks.bad, "file is valid APNG even after drops")
        tt |> equal(chunks.n_ihdr, 1)
        tt |> equal(chunks.n_iend, 1)
        // Submitter/worker accounting: every submitted frame either lands as an
        // fcTL chunk or counts as a drop. drop_count is only updated by enqueue
        // (no more after the 40th submit completes), and end() drains the queue,
        // so this invariant is race-free even though the worker is concurrent.
        tt |> equal(dropped_before_end + chunks.n_fctl, 40)
        // num_frames in acTL must equal n_fctl in file
        tt |> equal(length(chunks.actl_body), 8)
        if (length(chunks.actl_body) == 8) {
            let nf = int(be_u32_at(chunks.actl_body[0], chunks.actl_body[1], chunks.actl_body[2], chunks.actl_body[3]))
            tt |> equal(nf, chunks.n_fctl)
        }
        remove(fname)
    }
}

[test]
def test_apng_double_writers(t : T?) {
    t |> run("APNG: two parallel writers — no shared state") <| @(tt : T?) {
        let path1 = "{get_das_root()}/tests/stbimage/_test_output_p1.apng"
        let path2 = "{get_das_root()}/tests/stbimage/_test_output_p2.apng"
        var w1 = unsafe(stbi_apng_begin(path1, 32, 32, 4))
        var w2 = unsafe(stbi_apng_begin(path2, 64, 64, 4))
        tt |> success(w1 != null && w2 != null, "both writers opened")
        if (w1 == null || w2 == null) {
            return
        }
        var px32 : array<uint8>; px32 |> resize(32 * 32 * 4)
        var px64 : array<uint8>; px64 |> resize(64 * 64 * 4)
        for (_frame in range(2)) {
            unsafe {
                stbi_apng_frame(w1, addr(px32[0]), 32 * 4, 100)
                stbi_apng_frame(w2, addr(px64[0]), 64 * 4, 100)
            }
        }
        let ok1 = unsafe(stbi_apng_end(w1))
        let ok2 = unsafe(stbi_apng_end(w2))
        tt |> equal(ok1, 1)
        tt |> equal(ok2, 1)
        // Both files should be valid APNGs of their respective sizes
        var x = 0; var y = 0; var comp = 0
        var l1 = stbi_load(path1, safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(l1 != null, "path1 decodes")
        if (l1 != null) {
            tt |> equal(x, 32)
            tt |> equal(y, 32)
            stbi_image_free(l1)
        }
        x = 0; y = 0; comp = 0
        var l2 = stbi_load(path2, safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(l2 != null, "path2 decodes")
        if (l2 != null) {
            tt |> equal(x, 64)
            tt |> equal(y, 64)
            stbi_image_free(l2)
        }
        remove(path1)
        remove(path2)
    }
}

[test]
def test_apng_dropped_zero_when_keeping_up(t : T?) {
    t |> run("APNG: drop counter stays at 0 for small, slow stream") <| @(tt : T?) {
        let fname = "{get_das_root()}/tests/stbimage/_test_output_slow.apng"
        let W = 16; let H = 16
        var writer = unsafe(stbi_apng_begin(fname, W, H, 4))
        if (writer == null) {
            tt |> success(false, "begin failed")
            return
        }
        var pixels : array<uint8>
        pixels |> resize(W * H * 4)
        for (_frame in range(3)) {
            unsafe {
                stbi_apng_frame(writer, addr(pixels[0]), W * 4, 100)
            }
            sleep(50u)   // ms — give worker time to drain
        }
        let dropped = unsafe(stbi_apng_dropped(writer))
        tt |> equal(dropped, 0)
        unsafe {
            stbi_apng_end(writer)
        }
        remove(fname)
    }
}
