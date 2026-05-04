options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage/stbimage_boost
require math
require dastest/testing_boost public

// ---- fill_rect (4-channel RGBA) ----

[test]
def test_fill_rect_rgba(t : T?) {
    t |> run("fill_rect fills correct region with RGBA color") <| @(tt : T?) {
        var img = make_image(10, 10, 4)
        // zero out
        for (b in img.bytes) { b = uint8(0); }
        let color = 0xAABBCCDDu
        img.fill_rect(2, 3, 4, 2, color)
        // check filled region
        for (y in range(3, 5)) {
            for (x in range(2, 6)) {
                let idx = (y * 10 + x) * 4
                let px = uint(img.bytes[idx]) | (uint(img.bytes[idx + 1]) << 8u) | (uint(img.bytes[idx + 2]) << 16u) | (uint(img.bytes[idx + 3]) << 24u)
                tt |> equal(px, color)
            }
        }
        // check outside is still zero
        let idx_outside = (0 * 10 + 0) * 4
        tt |> equal(int(img.bytes[idx_outside]), 0)
    }
}

[test]
def test_fill_rect_clips(t : T?) {
    t |> run("fill_rect clips to image bounds") <| @(tt : T?) {
        var img = make_image(5, 5, 4)
        for (b in img.bytes) { b = uint8(0); }
        // fill extending beyond bounds
        img.fill_rect(-1, -1, 3, 3, 0xFFFFFFFFu)
        // pixel at (0,0) should be filled
        let idx00 = 0
        tt |> equal(int(img.bytes[idx00]), 255)
        // pixel at (2,2) should NOT be filled (clip: -1+3=2, so x range is 0..1, y range is 0..1)
        let idx22 = (2 * 5 + 2) * 4
        tt |> equal(int(img.bytes[idx22]), 0)
    }
}

// ---- fill_rect (1-channel) ----

[test]
def test_fill_rect_grey(t : T?) {
    t |> run("fill_rect fills 1-channel image") <| @(tt : T?) {
        var img = make_image(8, 8, 1)
        for (b in img.bytes) { b = uint8(0); }
        img.fill_rect(1, 1, 3, 3, uint8(128))
        tt |> equal(int(img.bytes[1 * 8 + 1]), 128)
        tt |> equal(int(img.bytes[1 * 8 + 4]), 0)
        tt |> equal(int(img.bytes[0 * 8 + 0]), 0)
    }
}

// ---- blit_alpha ----

[test]
def test_blit_alpha_full(t : T?) {
    t |> run("blit_alpha with 255 alpha overwrites color") <| @(tt : T?) {
        var dst = make_image(4, 4, 4)
        for (b in dst.bytes) { b = uint8(0); }
        var src = make_image(2, 2, 1)
        // full alpha
        for (b in src.bytes) { b = uint8(255); }
        dst.blit_alpha(src, 0, 0, 1, 1, 2, 2, 255, 128, 64)
        // check pixel at (1,1)
        let idx = (1 * 4 + 1) * 4
        tt |> equal(int(dst.bytes[idx + 0]), 255)
        tt |> equal(int(dst.bytes[idx + 1]), 128)
        tt |> equal(int(dst.bytes[idx + 2]), 64)
        tt |> equal(int(dst.bytes[idx + 3]), 255)
    }
}

[test]
def test_blit_alpha_zero(t : T?) {
    t |> run("blit_alpha with 0 alpha leaves dest unchanged") <| @(tt : T?) {
        var dst = make_image(4, 4, 4)
        // fill with known color
        for (i in range(4 * 4)) {
            let idx = i * 4
            dst.bytes[idx + 0] = uint8(100)
            dst.bytes[idx + 1] = uint8(150)
            dst.bytes[idx + 2] = uint8(200)
            dst.bytes[idx + 3] = uint8(255)
        }
        var src = make_image(2, 2, 1)
        for (b in src.bytes) { b = uint8(0); }
        dst.blit_alpha(src, 0, 0, 1, 1, 2, 2, 255, 0, 0)
        // dest should be unchanged
        let idx = (1 * 4 + 1) * 4
        tt |> equal(int(dst.bytes[idx + 0]), 100)
        tt |> equal(int(dst.bytes[idx + 1]), 150)
        tt |> equal(int(dst.bytes[idx + 2]), 200)
    }
}

[test]
def test_blit_alpha_blend(t : T?) {
    t |> run("blit_alpha with 128 alpha blends correctly") <| @(tt : T?) {
        var dst = make_image(4, 4, 4)
        for (i in range(4 * 4)) {
            let idx = i * 4
            dst.bytes[idx + 0] = uint8(0)
            dst.bytes[idx + 1] = uint8(0)
            dst.bytes[idx + 2] = uint8(0)
            dst.bytes[idx + 3] = uint8(255)
        }
        var src = make_image(2, 2, 1)
        for (b in src.bytes) { b = uint8(128); }
        dst.blit_alpha(src, 0, 0, 0, 0, 2, 2, 255, 255, 255)
        // expected: (128 * 255 + 127 * 0 + 127) / 255 = 128
        let idx = 0
        let val = int(dst.bytes[idx])
        tt |> success(val >= 127 && val <= 129, "blended value ~128, got {val}")
    }
}

[test]
def test_blit_alpha_clips(t : T?) {
    t |> run("blit_alpha clips to both src and dst bounds") <| @(tt : T?) {
        var dst = make_image(4, 4, 4)
        for (b in dst.bytes) { b = uint8(0); }
        var src = make_image(2, 2, 1)
        for (b in src.bytes) { b = uint8(255); }
        // blit with negative dest offset — should clip
        dst.blit_alpha(src, 0, 0, -1, -1, 2, 2, 255, 255, 255)
        // only pixel (0,0) should be affected (from src[1,1])
        let idx00 = 0
        tt |> equal(int(dst.bytes[idx00]), 255)
        // pixel (1,0) should be untouched
        let idx10 = 1 * 4
        tt |> equal(int(dst.bytes[idx10]), 0)
    }
}

// ---- detailed rast_blit_alpha correctness ----

[test]
def test_blit_alpha_detailed(t : T?) {
    t |> run("blit_alpha blend math is exact for key alpha values") <| @(tt : T?) {
        // test blending onto a known background with specific alpha values
        // formula: out = (alpha * src + (255 - alpha) * dst) / 255  (exact integer)
        let test_alphas = fixed_array(0, 1, 64, 127, 128, 200, 254, 255)
        let src_r = 220; let src_g = 100; let src_b = 30
        let dst_r = 40;  let dst_g = 180; let dst_b = 250
        for (a in test_alphas) {
            var dst = make_image(1, 1, 4)
            dst.bytes[0] = uint8(dst_r)
            dst.bytes[1] = uint8(dst_g)
            dst.bytes[2] = uint8(dst_b)
            dst.bytes[3] = uint8(200)
            var src = make_image(1, 1, 1)
            src.bytes[0] = uint8(a)
            dst.blit_alpha(src, 0, 0, 0, 0, 1, 1, src_r, src_g, src_b)
            if (a == 0) {
                // zero alpha: dest unchanged
                tt |> equal(int(dst.bytes[0]), dst_r)
                tt |> equal(int(dst.bytes[1]), dst_g)
                tt |> equal(int(dst.bytes[2]), dst_b)
                tt |> equal(int(dst.bytes[3]), 200)
            } elif (a == 255) {
                // full alpha: exact source color
                tt |> equal(int(dst.bytes[0]), src_r)
                tt |> equal(int(dst.bytes[1]), src_g)
                tt |> equal(int(dst.bytes[2]), src_b)
                tt |> equal(int(dst.bytes[3]), 255)
            } else {
                // blended: verify within ±1 of exact formula
                let expect_r = (a * src_r + (255 - a) * dst_r + 127) / 255
                let expect_g = (a * src_g + (255 - a) * dst_g + 127) / 255
                let expect_b = (a * src_b + (255 - a) * dst_b + 127) / 255
                let got_r = int(dst.bytes[0])
                let got_g = int(dst.bytes[1])
                let got_b = int(dst.bytes[2])
                tt |> success(abs(got_r - expect_r) <= 1, "R alpha={a}: got {got_r}, expected {expect_r}")
                tt |> success(abs(got_g - expect_g) <= 1, "G alpha={a}: got {got_g}, expected {expect_g}")
                tt |> success(abs(got_b - expect_b) <= 1, "B alpha={a}: got {got_b}, expected {expect_b}")
                // alpha channel: min(dst_a + src_a, 255)
                let expect_a = min(200 + a, 255)
                tt |> equal(int(dst.bytes[3]), expect_a)
            }
        }
    }
    t |> run("blit_alpha sub-rect positions are correct") <| @(tt : T?) {
        // 4x4 dest, 4x4 src alpha — blit a 2x2 sub-rect from src(1,1) to dst(2,2)
        var dst = make_image(4, 4, 4)
        for (b in dst.bytes) { b = uint8(0); }
        var src = make_image(4, 4, 1)
        for (b in src.bytes) { b = uint8(0); }
        // set src pixels at (1,1), (2,1), (1,2), (2,2) to 255
        src.bytes[1 * 4 + 1] = uint8(255)
        src.bytes[1 * 4 + 2] = uint8(255)
        src.bytes[2 * 4 + 1] = uint8(255)
        src.bytes[2 * 4 + 2] = uint8(255)
        dst.blit_alpha(src, 1, 1, 2, 2, 2, 2, 128, 64, 32)
        // dst pixels at (2,2), (3,2), (2,3), (3,3) should be colored
        for (dy in range(4)) {
            for (dx in range(4)) {
                let idx = (dy * 4 + dx) * 4
                if (dx >= 2 && dy >= 2) {
                    tt |> equal(int(dst.bytes[idx + 0]), 128)
                    tt |> equal(int(dst.bytes[idx + 1]), 64)
                    tt |> equal(int(dst.bytes[idx + 2]), 32)
                } else {
                    tt |> equal(int(dst.bytes[idx + 0]), 0)
                }
            }
        }
    }
    t |> run("blit_alpha preserves dest outside blitted region") <| @(tt : T?) {
        var dst = make_image(4, 4, 4)
        // fill entire dest with known color
        for (i in range(4 * 4)) {
            let idx = i * 4
            dst.bytes[idx + 0] = uint8(11)
            dst.bytes[idx + 1] = uint8(22)
            dst.bytes[idx + 2] = uint8(33)
            dst.bytes[idx + 3] = uint8(44)
        }
        var src = make_image(1, 1, 1)
        src.bytes[0] = uint8(255)
        dst.blit_alpha(src, 0, 0, 1, 1, 1, 1, 200, 100, 50)
        // only (1,1) changed
        let idx_changed = (1 * 4 + 1) * 4
        tt |> equal(int(dst.bytes[idx_changed + 0]), 200)
        // check corners are untouched
        for (corner in fixed_array(int2(0, 0), int2(3, 0), int2(0, 3), int2(3, 3))) {
            let idx = (corner.y * 4 + corner.x) * 4
            tt |> equal(int(dst.bytes[idx + 0]), 11)
            tt |> equal(int(dst.bytes[idx + 1]), 22)
            tt |> equal(int(dst.bytes[idx + 2]), 33)
            tt |> equal(int(dst.bytes[idx + 3]), 44)
        }
    }
}
