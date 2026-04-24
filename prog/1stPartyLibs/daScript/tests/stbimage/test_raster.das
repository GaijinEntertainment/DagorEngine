options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage/stbimage_boost
require daslib/safe_addr
require dastest/testing_boost public

def test_image_path(name : string) : string {
    return "{get_das_root()}/tests/stbimage/{name}"
}

// helper: fill image bytes with position-based pattern
// each pixel's first byte = (x + y * width) & 0xFF
def fill_pattern(var img : Image) {
    let px = img.channels * img.bpc
    for (y in range(img.height)) {
        for (x in range(img.width)) {
            let idx = (y * img.width + x) * px
            img.bytes[idx] = uint8((x + y * img.width) & int(0xFF))
            // set remaining bytes to a fixed value so we can detect corruption
            for (b in range(1, px)) {
                img.bytes[idx + b] = uint8(0xAB)
            }
        }
    }
}

// helper: verify pattern matches expected pixel mapping after transforms
def verify_equal(tt : T?; a : Image; b : Image) {
    tt |> equal(length(a.bytes), length(b.bytes))
    var mismatch = 0
    for (i in range(length(a.bytes))) {
        if (a.bytes[i] != b.bytes[i]) {
            mismatch++
        }
    }
    tt |> equal(mismatch, 0)
}

// ========================================================
// flip_horizontal — test every pixel size dispatch path
// ========================================================

// px=1: 1ch, bpc=1 (grey uint8)
[test]
def test_flip_h_px1(t : T?) {
    t |> run("flip_horizontal px=1 (grey uint8)") <| @(tt : T?) {
        var img <- make_image(4, 2, 1, 1)
        fill_pattern(img)
        // pixel(0,0)=0, pixel(3,0)=3
        tt |> equal(int(img.bytes[0]), 0)
        tt |> equal(int(img.bytes[3]), 3)
        var flipped <- img.flip_horizontal()
        // after flip: pixel(0,0) should be old pixel(3,0)=3
        tt |> equal(int(flipped.bytes[0]), 3)
        tt |> equal(int(flipped.bytes[3]), 0)
        // roundtrip
        var restored <- flipped.flip_horizontal()
        verify_equal(tt, img, restored)
    }
}

// px=2: 2ch, bpc=1 (grey+alpha uint8)
[test]
def test_flip_h_px2(t : T?) {
    t |> run("flip_horizontal px=2 (grey+alpha uint8)") <| @(tt : T?) {
        var img <- make_image(4, 2, 2, 1)
        fill_pattern(img)
        // pixel(0,0) first byte=0, pixel(3,0) first byte=3
        tt |> equal(int(img.bytes[0]), 0)
        tt |> equal(int(img.bytes[6]), 3)  // 3*2=6
        var flipped <- img.flip_horizontal()
        tt |> equal(int(flipped.bytes[0]), 3)
        tt |> equal(int(flipped.bytes[1]), int(uint8(0xAB)))  // second byte preserved
        var restored <- flipped.flip_horizontal()
        verify_equal(tt, img, restored)
    }
}

// px=3: 3ch, bpc=1 (RGB uint8)
[test]
def test_flip_h_px3(t : T?) {
    t |> run("flip_horizontal px=3 (RGB uint8)") <| @(tt : T?) {
        var img <- make_image(4, 2, 3, 1)
        fill_pattern(img)
        var flipped <- img.flip_horizontal()
        // pixel(0,0) of flipped = pixel(3,0) of original
        tt |> equal(int(flipped.bytes[0]), 3)
        tt |> equal(int(flipped.bytes[1]), int(uint8(0xAB)))
        tt |> equal(int(flipped.bytes[2]), int(uint8(0xAB)))
        var restored <- flipped.flip_horizontal()
        verify_equal(tt, img, restored)
    }
}

// px=4: 4ch, bpc=1 (RGBA uint8)
[test]
def test_flip_h_px4(t : T?) {
    t |> run("flip_horizontal px=4 (RGBA uint8)") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        // gradient: pixel(0,0) R=0, pixel(63,0) R=252
        var flipped <- img.flip_horizontal()
        tt |> equal(int(flipped.bytes[0]), 252)
        tt |> equal(int(flipped.bytes[63 * 4]), 0)
        var restored <- flipped.flip_horizontal()
        verify_equal(tt, img, restored)
    }
}

// px=6: 3ch, bpc=2 (RGB uint16)
[test]
def test_flip_h_px6(t : T?) {
    t |> run("flip_horizontal px=6 (RGB uint16)") <| @(tt : T?) {
        var img <- make_image(4, 2, 3, 2)
        fill_pattern(img)
        var flipped <- img.flip_horizontal()
        // pixel(0,0) of flipped = pixel(3,0) of original, first byte = 3
        tt |> equal(int(flipped.bytes[0]), 3)
        var restored <- flipped.flip_horizontal()
        verify_equal(tt, img, restored)
    }
}

// px=8: 4ch, bpc=2 (RGBA uint16)
[test]
def test_flip_h_px8(t : T?) {
    t |> run("flip_horizontal px=8 (RGBA uint16)") <| @(tt : T?) {
        var img <- make_image(4, 2, 4, 2)
        fill_pattern(img)
        var flipped <- img.flip_horizontal()
        tt |> equal(int(flipped.bytes[0]), 3)
        var restored <- flipped.flip_horizontal()
        verify_equal(tt, img, restored)
    }
}

// px=12: 3ch, bpc=4 (RGB float)
[test]
def test_flip_h_px12(t : T?) {
    t |> run("flip_horizontal px=12 (RGB float)") <| @(tt : T?) {
        var img <- make_image(4, 2, 3, 4)
        fill_pattern(img)
        var flipped <- img.flip_horizontal()
        tt |> equal(int(flipped.bytes[0]), 3)
        var restored <- flipped.flip_horizontal()
        verify_equal(tt, img, restored)
    }
}

// px=16: 4ch, bpc=4 (RGBA float)
[test]
def test_flip_h_px16(t : T?) {
    t |> run("flip_horizontal px=16 (RGBA float)") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        var flipped <- img.flip_horizontal()
        tt |> equal(flipped.bpc, 4)
        // pixel(0,0) of flipped = pixel(63,0) of original: R=63/64~0.98
        flipped |> with_pixels() <| $(var pixels : array<float>#) {
            tt |> success(pixels[0] > 0.9f, "flipped(0,0) R near 1.0")
        }
        var restored <- flipped.flip_horizontal()
        // restore should match original
        restored |> with_pixels() <| $(var rpx : array<float>#) {
            tt |> success(rpx[0] < 0.05f, "restored(0,0) R near 0")
        }
    }
}

// ========================================================
// blit — test every pixel size dispatch path
// ========================================================

// px=1: 1ch, bpc=1
[test]
def test_blit_px1(t : T?) {
    t |> run("blit px=1 (grey uint8)") <| @(tt : T?) {
        var dst <- make_image(8, 8, 1, 1)
        var src <- make_image(3, 3, 1, 1)
        for (i in range(9)) {
            src.bytes[i] = uint8(200)
        }
        dst.blit(src, 2, 2)
        // pixel(2,2) of dst = 200
        tt |> equal(int(dst.bytes[2 * 8 + 2]), 200)
        // pixel(4,4) of dst = 200 (still inside 3x3)
        tt |> equal(int(dst.bytes[4 * 8 + 4]), 200)
        // pixel(5,2) of dst = 0 (outside src)
        tt |> equal(int(dst.bytes[2 * 8 + 5]), 0)
    }
}

// px=2: 2ch, bpc=1
[test]
def test_blit_px2(t : T?) {
    t |> run("blit px=2 (grey+alpha uint8)") <| @(tt : T?) {
        var dst <- make_image(8, 8, 2, 1)
        var src <- make_image(2, 2, 2, 1)
        for (i in range(2 * 2)) {
            src.bytes[i * 2 + 0] = uint8(100)
            src.bytes[i * 2 + 1] = uint8(255)
        }
        dst.blit(src, 3, 3)
        let idx = (3 * 8 + 3) * 2
        tt |> equal(int(dst.bytes[idx + 0]), 100)
        tt |> equal(int(dst.bytes[idx + 1]), 255)
        // outside
        let idx2 = (3 * 8 + 2) * 2
        tt |> equal(int(dst.bytes[idx2 + 0]), 0)
    }
}

// px=3: 3ch, bpc=1
[test]
def test_blit_px3(t : T?) {
    t |> run("blit px=3 (RGB uint8)") <| @(tt : T?) {
        var dst <- make_image(16, 16, 3, 1)
        var src <- make_image(4, 4, 3, 1)
        for (i in range(4 * 4)) {
            src.bytes[i * 3 + 0] = uint8(200)
            src.bytes[i * 3 + 1] = uint8(100)
            src.bytes[i * 3 + 2] = uint8(50)
        }
        dst.blit(src, 5, 5)
        let idx = (5 * 16 + 5) * 3
        tt |> equal(int(dst.bytes[idx + 0]), 200)
        tt |> equal(int(dst.bytes[idx + 1]), 100)
        tt |> equal(int(dst.bytes[idx + 2]), 50)
        let idx2 = (5 * 16 + 4) * 3
        tt |> equal(int(dst.bytes[idx2 + 0]), 0)
    }
}

// px=4: 4ch, bpc=1
[test]
def test_blit_px4(t : T?) {
    t |> run("blit px=4 (RGBA uint8)") <| @(tt : T?) {
        var dst <- make_image(32, 32, 4, 1)
        var src <- make_image(4, 4, 4, 1)
        for (i in range(4 * 4)) {
            src.bytes[i * 4 + 0] = uint8(255)
            src.bytes[i * 4 + 3] = uint8(255)
        }
        dst.blit(src, 10, 10)
        let idx = (10 * 32 + 10) * 4
        tt |> equal(int(dst.bytes[idx + 0]), 255)
        tt |> equal(int(dst.bytes[idx + 1]), 0)
        tt |> equal(int(dst.bytes[idx + 2]), 0)
        tt |> equal(int(dst.bytes[idx + 3]), 255)
        let idx2 = (10 * 32 + 9) * 4
        tt |> equal(int(dst.bytes[idx2 + 0]), 0)
    }
}

// px=6: 3ch, bpc=2
[test]
def test_blit_px6(t : T?) {
    t |> run("blit px=6 (RGB uint16)") <| @(tt : T?) {
        var dst <- make_image(8, 8, 3, 2)
        var src <- make_image(2, 2, 3, 2)
        // fill all src bytes with 0xCC
        for (i in range(length(src.bytes))) {
            src.bytes[i] = uint8(0xCC)
        }
        dst.blit(src, 1, 1)
        let idx = (1 * 8 + 1) * 6
        tt |> equal(int(dst.bytes[idx]), int(0xCC))
        let idx2 = (1 * 8 + 0) * 6
        tt |> equal(int(dst.bytes[idx2]), 0)
    }
}

// px=8: 4ch, bpc=2
[test]
def test_blit_px8(t : T?) {
    t |> run("blit px=8 (RGBA uint16)") <| @(tt : T?) {
        var dst <- make_image(8, 8, 4, 2)
        var src <- make_image(2, 2, 4, 2)
        for (i in range(length(src.bytes))) {
            src.bytes[i] = uint8(0xDD)
        }
        dst.blit(src, 3, 3)
        let idx = (3 * 8 + 3) * 8
        tt |> equal(int(dst.bytes[idx]), int(0xDD))
        let idx2 = (3 * 8 + 2) * 8
        tt |> equal(int(dst.bytes[idx2]), 0)
    }
}

// px=12: 3ch, bpc=4
[test]
def test_blit_px12(t : T?) {
    t |> run("blit px=12 (RGB float)") <| @(tt : T?) {
        var dst <- make_image(8, 8, 3, 4)
        var src <- make_image(2, 2, 3, 4)
        for (i in range(length(src.bytes))) {
            src.bytes[i] = uint8(0xEE)
        }
        dst.blit(src, 4, 4)
        let idx = (4 * 8 + 4) * 12
        tt |> equal(int(dst.bytes[idx]), int(0xEE))
        let idx2 = (4 * 8 + 3) * 12
        tt |> equal(int(dst.bytes[idx2]), 0)
    }
}

// px=16: 4ch, bpc=4
[test]
def test_blit_px16(t : T?) {
    t |> run("blit px=16 (RGBA float)") <| @(tt : T?) {
        var dst <- make_image(16, 16, 4, 4)
        var src <- make_image(4, 4, 4, 4)
        src |> with_pixels() <| $(var pixels : array<float>#) {
            for (i in range(4 * 4)) {
                pixels[i * 4 + 0] = 0.75f
                pixels[i * 4 + 1] = 0.5f
                pixels[i * 4 + 2] = 0.25f
                pixels[i * 4 + 3] = 1.0f
            }
        }
        dst.blit(src, 2, 2)
        dst |> with_pixels() <| $(var pixels : array<float>#) {
            let idx = (2 * 16 + 2) * 4
            tt |> success(pixels[idx + 0] > 0.7f && pixels[idx + 0] < 0.8f, "R=0.75")
            tt |> success(pixels[idx + 1] > 0.45f && pixels[idx + 1] < 0.55f, "G=0.5")
            let idx2 = (2 * 16 + 1) * 4
            tt |> success(pixels[idx2 + 0] < 0.01f, "untouched pixel is zero")
        }
    }
}

// ========================================================
// blit — clipping and edge cases
// ========================================================

[test]
def test_blit_clipping(t : T?) {
    t |> run("blit clips at destination bounds") <| @(tt : T?) {
        var dst <- make_image(8, 8, 4, 1)
        var src <- make_image(4, 4, 4, 1)
        for (i in range(4 * 4)) {
            src.bytes[i * 4 + 0] = uint8(200)
            src.bytes[i * 4 + 3] = uint8(255)
        }
        // blit at (6,6) — only 2x2 pixels should land
        dst.blit(src, 6, 6)
        let idx = (6 * 8 + 6) * 4
        tt |> equal(int(dst.bytes[idx + 0]), 200)
        let idx2 = (7 * 8 + 7) * 4
        tt |> equal(int(dst.bytes[idx2 + 0]), 200)
        var count = 0
        for (i in range(8 * 8)) {
            if (dst.bytes[i * 4] != uint8(0)) {
                count++
            }
        }
        tt |> equal(count, 4)  // 2x2 region
    }
}

[test]
def test_blit_negative_offset(t : T?) {
    t |> run("blit with negative offset clips source") <| @(tt : T?) {
        var dst <- make_image(8, 8, 4, 1)
        var src <- make_image(4, 4, 4, 1)
        for (i in range(4 * 4)) {
            src.bytes[i * 4 + 0] = uint8(150)
            src.bytes[i * 4 + 3] = uint8(255)
        }
        // blit at (-2,-2) — only bottom-right 2x2 of src should land at dst(0,0)
        dst.blit(src, -2, -2)
        tt |> equal(int(dst.bytes[0]), 150)
        let idx2 = (1 * 8 + 1) * 4
        tt |> equal(int(dst.bytes[idx2]), 150)
        let idx3 = (0 * 8 + 2) * 4
        tt |> equal(int(dst.bytes[idx3]), 0)
    }
}

// ========================================================
// crop — core tests + HDR
// ========================================================

[test]
def test_crop(t : T?) {
    t |> run("crop extracts sub-region with pixel verification") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var cropped <- img.crop(10, 20, 16, 16)
        tt |> equal(cropped.width, 16)
        tt |> equal(cropped.height, 16)
        tt |> equal(cropped.channels, 4)
        tt |> equal(cropped.bpc, 1)
        // pixel(0,0) of cropped = pixel(10,20) of original = R=10*4=40, G=20*4=80, B=128, A=255
        tt |> equal(int(cropped.bytes[0]), 40)
        tt |> equal(int(cropped.bytes[1]), 80)
        tt |> equal(int(cropped.bytes[2]), 128)
        tt |> equal(int(cropped.bytes[3]), 255)
    }
}

[test]
def test_crop_clamp(t : T?) {
    t |> run("crop clamps to image bounds") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var cropped <- img.crop(60, 60, 16, 16)
        tt |> equal(cropped.width, 4)
        tt |> equal(cropped.height, 4)
    }
}

[test]
def test_crop_out_of_bounds(t : T?) {
    t |> run("crop fully outside returns invalid image") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        let cropped <- img.crop(100, 100, 10, 10)
        tt |> success(!cropped.valid(), "cropped out of bounds is invalid")
    }
}

[test]
def test_crop_full(t : T?) {
    t |> run("crop of full image preserves all pixels") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        let cropped <- img.crop(0, 0, 64, 64)
        tt |> equal(cropped.width, 64)
        tt |> equal(cropped.height, 64)
        verify_equal(tt, img, cropped)
    }
}

[test]
def test_crop_hdr(t : T?) {
    t |> run("crop HDR image preserves bpc and pixels") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        var cropped <- img.crop(10, 10, 16, 16)
        tt |> equal(cropped.width, 16)
        tt |> equal(cropped.height, 16)
        tt |> equal(cropped.bpc, 4)
        cropped |> with_pixels() <| $(var pixels : array<float>#) {
            tt |> success(pixels[0] > 0.1f && pixels[0] < 0.25f, "R near 0.156")
            tt |> success(pixels[1] > 0.1f && pixels[1] < 0.25f, "G near 0.156")
        }
    }
}

[test]
def test_crop_grey(t : T?) {
    t |> run("crop on 1-channel image") <| @(tt : T?) {
        var img <- make_image(8, 8, 1, 1)
        fill_pattern(img)
        var cropped <- img.crop(2, 2, 4, 4)
        tt |> equal(cropped.width, 4)
        tt |> equal(cropped.height, 4)
        tt |> equal(cropped.channels, 1)
        // pixel(0,0) of cropped = pixel(2,2) of original = (2 + 2*8) & 0xFF = 18
        tt |> equal(int(cropped.bytes[0]), 18)
    }
}
