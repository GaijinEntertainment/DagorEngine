options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage/stbimage_boost
require daslib/safe_addr
require strings
require daslib/fio
require dastest/testing_boost public

def test_image_path(name : string) : string {
    return "{get_das_root()}/tests/stbimage/{name}"
}

// ---- struct queries ----

[test]
def test_default_image_invalid(t : T?) {
    t |> run("freshly declared Image is not valid") <| @(tt : T?) {
        var img : Image
        tt |> success(!img.valid(), "default image not valid")
    }
}

[test]
def test_valid_after_load(t : T?) {
    t |> run("Image is valid after load") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load(test_image_path("test.png"))
        tt |> success(ok, "load ok: {error}")
        tt |> success(img.valid(), "image is valid")
    }
}

[test]
def test_stride(t : T?) {
    t |> run("stride of 64x64 RGBA uint8 image") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        tt |> equal(img.stride(), 64 * 4 * 1)
    }
}

[test]
def test_stride_hdr(t : T?) {
    t |> run("stride of 64x64 HDR image") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        tt |> equal(img.stride(), 64 * 4 * 4)
    }
}

[test]
def test_has_alpha(t : T?) {
    t |> run("has_alpha for RGBA and RGB") <| @(tt : T?) {
        var img4 : Image
        img4.load(test_image_path("test.png"))
        tt |> success(img4.has_alpha(), "4ch has alpha")
        var img3 : Image
        img3.load(test_image_path("test.jpg"), 3)
        tt |> success(!img3.has_alpha(), "3ch no alpha")
    }
}

[test]
def test_pixel_size(t : T?) {
    t |> run("pixel_size for uint8 and float") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        tt |> equal(img.pixel_size(), 4)
        var hdr : Image
        hdr.load_hdr(test_image_path("test.hdr"), 4)
        tt |> equal(hdr.pixel_size(), 16)
    }
}

[test]
def test_is_hdr_query(t : T?) {
    t |> run("is_hdr query on uint8 and float images") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        tt |> success(!img.is_hdr(), "uint8 not HDR")
        var hdr : Image
        hdr.load_hdr(test_image_path("test.hdr"), 4)
        tt |> success(hdr.is_hdr(), "float is HDR")
    }
}

[test]
def test_data_size(t : T?) {
    t |> run("data_size matches expected") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        tt |> equal(img.data_size(), 64 * 64 * 4 * 1)
    }
}

// ---- load from file ----

[test]
def test_load_png(t : T?) {
    t |> run("load PNG from file") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load(test_image_path("test.png"))
        tt |> success(ok, "load ok: {error}")
        tt |> equal(img.width, 64)
        tt |> equal(img.height, 64)
        tt |> equal(img.channels, 4)
        tt |> equal(img.bpc, 1)
    }
}

[test]
def test_load_bmp(t : T?) {
    t |> run("load BMP from file") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load(test_image_path("test.bmp"))
        tt |> success(ok, "load ok: {error}")
        tt |> equal(img.width, 64)
        tt |> equal(img.height, 64)
        tt |> equal(img.bpc, 1)
    }
}

[test]
def test_load_tga(t : T?) {
    t |> run("load TGA from file") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load(test_image_path("test.tga"))
        tt |> success(ok, "load ok: {error}")
        tt |> equal(img.width, 64)
        tt |> equal(img.height, 64)
        tt |> equal(img.bpc, 1)
    }
}

[test]
def test_load_jpg(t : T?) {
    t |> run("load JPG from file") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load(test_image_path("test.jpg"), 3)
        tt |> success(ok, "load ok: {error}")
        tt |> equal(img.width, 64)
        tt |> equal(img.height, 64)
        tt |> equal(img.channels, 3)
        tt |> equal(img.bpc, 1)
    }
}

[test]
def test_load_hdr_file(t : T?) {
    t |> run("load HDR from file") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load_hdr(test_image_path("test.hdr"), 4)
        tt |> success(ok, "load ok: {error}")
        tt |> equal(img.width, 64)
        tt |> equal(img.height, 64)
        tt |> equal(img.bpc, 4)
    }
}

[test]
def test_load_16_file(t : T?) {
    t |> run("load 16-bit from file") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load_16(test_image_path("test.png"))
        tt |> success(ok, "load ok: {error}")
        tt |> equal(img.width, 64)
        tt |> equal(img.height, 64)
        tt |> equal(img.bpc, 2)
    }
}

[test]
def test_load_requested_channels(t : T?) {
    t |> run("load with requested_channels changes channel count") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load(test_image_path("test.png"), 3)
        tt |> success(ok, "load ok: {error}")
        tt |> equal(img.channels, 3)
    }
}

[test]
def test_load_failure(t : T?) {
    t |> run("load nonexistent file returns error") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.load(test_image_path("does_not_exist.png"))
        tt |> success(!ok, "load should fail")
        tt |> success(length(error) > 0, "error message not empty")
        tt |> success(!img.valid(), "image not valid after failure")
    }
}

// ---- load from memory ----

[test]
def test_load_from_memory(t : T?) {
    t |> run("load from memory buffer") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var png_bytes : array<uint8>
        let (enc_ok, enc_err) = img.encode("png", png_bytes)
        tt |> success(enc_ok, "encode ok: {enc_err}")
        var img2 : Image
        let (ok, error) = img2.load_from_memory(png_bytes)
        tt |> success(ok, "load_from_memory ok: {error}")
        tt |> equal(img2.width, 64)
        tt |> equal(img2.height, 64)
        tt |> equal(img2.bpc, 1)
    }
}

[test]
def test_load_from_memory_failure(t : T?) {
    t |> run("load from garbage memory fails") <| @(tt : T?) {
        var garbage : array<uint8>
        garbage |> resize(16)
        for (i in range(16)) {
            garbage[i] = uint8(i)
        }
        var img : Image
        let (ok, error) = img.load_from_memory(garbage)
        tt |> success(!ok, "should fail")
        tt |> success(length(error) > 0, "error not empty")
    }
}

[test]
def test_load_16_from_memory(t : T?) {
    t |> run("load 16-bit from memory") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var png_bytes : array<uint8>
        img.encode("png", png_bytes)
        var img2 : Image
        let (ok, error) = img2.load_16_from_memory(png_bytes)
        tt |> success(ok, "load_16_from_memory ok: {error}")
        tt |> equal(img2.bpc, 2)
    }
}

[test]
def test_load_from_memory_requested_channels(t : T?) {
    t |> run("load from memory with requested channels") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var png_bytes : array<uint8>
        img.encode("png", png_bytes)
        var img2 : Image
        let (ok, error) = img2.load_from_memory(png_bytes, 3)
        tt |> success(ok, "ok: {error}")
        tt |> equal(img2.channels, 3)
    }
}

[test]
def test_load_hdr_from_memory(t : T?) {
    t |> run("load HDR from memory buffer") <| @(tt : T?) {
        // load HDR file to get raw float data, encode as HDR to memory, then load_hdr_from_memory
        var hdr : Image
        let (ok1, err1) = hdr.load_hdr(test_image_path("test.hdr"), 4)
        tt |> success(ok1, "initial load_hdr ok: {err1}")
        // read the raw .hdr file bytes
        var file_bytes : array<uint8>
        fopen(test_image_path("test.hdr"), "rb") <| $(f) {
            if (f != null) {
                fmap(f) <| $(var data : array<uint8>#) {
                    file_bytes := data
                }
            }
        }
        tt |> success(length(file_bytes) > 0, "read .hdr file bytes")
        var img2 : Image
        let (ok2, err2) = img2.load_hdr_from_memory(file_bytes)
        tt |> success(ok2, "load_hdr_from_memory ok: {err2}")
        tt |> equal(img2.bpc, 4)
        tt |> equal(img2.width, 64)
        tt |> equal(img2.height, 64)
        tt |> success(img2.channels > 0, "has channels")
    }
}

// ---- save to file ----

[test]
def test_save_png(t : T?) {
    t |> run("save as PNG and load back") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        let out_path = test_image_path("_test_out.png")
        let (ok, error) = img.save(out_path)
        tt |> success(ok, "save ok: {error}")
        var img2 : Image
        let (ok2, error2) = img2.load(out_path)
        tt |> success(ok2, "reload ok: {error2}")
        tt |> equal(img2.width, 64)
        tt |> equal(img2.height, 64)
        fio::remove(out_path)
    }
}

[test]
def test_save_bmp(t : T?) {
    t |> run("save as BMP and load back") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        let out_path = test_image_path("_test_out.bmp")
        let (ok, error) = img.save(out_path)
        tt |> success(ok, "save ok: {error}")
        var img2 : Image
        let (ok2, error2) = img2.load(out_path)
        tt |> success(ok2, "reload ok: {error2}")
        tt |> equal(img2.width, 64)
        fio::remove(out_path)
    }
}

[test]
def test_save_tga(t : T?) {
    t |> run("save as TGA and load back") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        let out_path = test_image_path("_test_out.tga")
        let (ok, error) = img.save(out_path)
        tt |> success(ok, "save ok: {error}")
        var img2 : Image
        let (ok2, error2) = img2.load(out_path)
        tt |> success(ok2, "reload ok: {error2}")
        tt |> equal(img2.width, 64)
        fio::remove(out_path)
    }
}

[test]
def test_save_jpg(t : T?) {
    t |> run("save as JPG with default quality") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"), 3)
        let out_path = test_image_path("_test_out.jpg")
        let (ok, error) = img.save(out_path)
        tt |> success(ok, "save ok: {error}")
        var img2 : Image
        let (ok2, error2) = img2.load(out_path, 3)
        tt |> success(ok2, "reload ok: {error2}")
        tt |> equal(img2.width, 64)
        fio::remove(out_path)
    }
}

[test]
def test_save_invalid_image(t : T?) {
    t |> run("save invalid image returns error") <| @(tt : T?) {
        var img : Image
        let (ok, error) = img.save(test_image_path("_test_out.png"))
        tt |> success(!ok, "should fail")
        tt |> success(length(error) > 0, "error not empty")
    }
}

// ---- encode to memory ----

[test]
def test_encode_png(t : T?) {
    t |> run("encode PNG to memory with magic bytes") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var out : array<uint8>
        let (ok, error) = img.encode("png", out)
        tt |> success(ok, "encode ok: {error}")
        tt |> success(length(out) > 8, "has data")
        // PNG magic: 137 80 78 71
        tt |> equal(int(out[0]), 137)
        tt |> equal(int(out[1]), 80)
        tt |> equal(int(out[2]), 78)
        tt |> equal(int(out[3]), 71)
    }
}

[test]
def test_encode_bmp(t : T?) {
    t |> run("encode BMP to memory with magic bytes") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var out : array<uint8>
        let (ok, error) = img.encode("bmp", out)
        tt |> success(ok, "encode ok: {error}")
        // BMP magic: 66 77 ('BM')
        tt |> equal(int(out[0]), 66)
        tt |> equal(int(out[1]), 77)
    }
}

[test]
def test_encode_jpg(t : T?) {
    t |> run("encode JPG to memory with magic bytes") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"), 3)
        var out : array<uint8>
        let (ok, error) = img.encode("jpg", out)
        tt |> success(ok, "encode ok: {error}")
        // JPEG magic: 255 216
        tt |> equal(int(out[0]), 255)
        tt |> equal(int(out[1]), 216)
    }
}

[test]
def test_encode_tga(t : T?) {
    t |> run("encode TGA to memory") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var out : array<uint8>
        let (ok, error) = img.encode("tga", out)
        tt |> success(ok, "encode ok: {error}")
        tt |> success(length(out) > 0, "has data")
    }
}

[test]
def test_encode_invalid_format(t : T?) {
    t |> run("encode unsupported format returns error") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var out : array<uint8>
        let (ok, error) = img.encode("gif", out)
        tt |> success(!ok, "should fail")
        tt |> success(length(error) > 0, "error not empty")
    }
}

// ---- pixel access: with_pixels ----

[test]
def test_with_pixels_uint8(t : T?) {
    t |> run("with_pixels uint8 reads corner pixels") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        img |> with_pixels() <| $(var pixels : array<uint8>#) {
            // pixel(0,0) = R=0, G=0, B=128, A=255
            tt |> equal(int(pixels[0]), 0)
            tt |> equal(int(pixels[1]), 0)
            tt |> equal(int(pixels[2]), 128)
            tt |> equal(int(pixels[3]), 255)
            // pixel(63,63) = R=252, G=252, B=128, A=255
            let last = (63 * 64 + 63) * 4
            tt |> equal(int(pixels[last + 0]), 252)
            tt |> equal(int(pixels[last + 1]), 252)
            tt |> equal(int(pixels[last + 2]), 128)
        }
    }
}

[test]
def test_with_pixels_float(t : T?) {
    t |> run("with_pixels float reads HDR data") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        img |> with_pixels() <| $(var pixels : array<float>#) {
            // pixel(0,0): R~0, G~0, B~0.5, A~1.0
            tt |> success(pixels[0] < 0.05f, "R near 0")
            tt |> success(pixels[1] < 0.05f, "G near 0")
            tt |> success(pixels[2] > 0.3f && pixels[2] < 0.7f, "B near 0.5")
            tt |> success(pixels[3] > 0.9f, "A near 1.0")
        }
    }
}

[test]
def test_with_pixels_16(t : T?) {
    t |> run("with_pixels uint16 reads 16-bit data") <| @(tt : T?) {
        var img : Image
        img.load_16(test_image_path("test.png"))
        img |> with_pixels() <| $(var pixels : array<uint16>#) {
            // pixel(0,0) in 16-bit: R=0, G=0
            tt |> equal(int(pixels[0]), 0)
            tt |> equal(int(pixels[1]), 0)
            // B channel should be non-zero (128 scaled to 16-bit)
            tt |> success(int(pixels[2]) > 0, "B non-zero in 16-bit")
        }
    }
}

[test]
def test_with_pixels_modify(t : T?) {
    t |> run("mutable with_pixels modifies pixel data") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        img |> with_pixels() <| $(var pixels : array<uint8>#) {
            pixels[0] = uint8(42)
        }
        // verify change persisted
        img |> with_pixels() <| $(var pixels : array<uint8>#) {
            tt |> equal(int(pixels[0]), 42)
        }
    }
}

[test]
def test_with_pixels_named_f(t : T?) {
    t |> run("with_pixels_f explicit name on HDR") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        img |> with_pixels_f() <| $(var pixels : array<float>#) {
            tt |> success(pixels[3] > 0.9f, "alpha near 1.0")
        }
    }
}

[test]
def test_with_pixels_named_16(t : T?) {
    t |> run("with_pixels_16 explicit name") <| @(tt : T?) {
        var img : Image
        img.load_16(test_image_path("test.png"))
        img |> with_pixels_16() <| $(var pixels : array<uint16>#) {
            tt |> success(length(pixels) == 64 * 64 * 4, "correct length")
        }
    }
}

// ---- pixel access: with_row ----

[test]
def test_with_row(t : T?) {
    t |> run("with_row reads first row") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        img |> with_row(0) <| $(var row : array<uint8>#) {
            tt |> equal(length(row), 64 * 4)
            // first pixel row 0: R=0, G=0
            tt |> equal(int(row[0]), 0)
            tt |> equal(int(row[1]), 0)
        }
    }
}

[test]
def test_with_row_last(t : T?) {
    t |> run("with_row reads last row") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        img |> with_row(63) <| $(var row : array<uint8>#) {
            tt |> equal(length(row), 64 * 4)
            // last row, first pixel: R=0, G=252
            tt |> equal(int(row[0]), 0)
            tt |> equal(int(row[1]), 252)
        }
    }
}

[test]
def test_with_row_float(t : T?) {
    t |> run("with_row float on HDR image") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        img |> with_row(0) <| $(var row : array<float>#) {
            tt |> equal(length(row), 64 * 4)
            tt |> success(row[2] > 0.3f && row[2] < 0.7f, "B near 0.5")
        }
    }
}

[test]
def test_with_row_modify(t : T?) {
    t |> run("mutable with_row modifies row data") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        img |> with_row(0) <| $(var row : array<uint8>#) {
            row[0] = uint8(99)
        }
        img |> with_row(0) <| $(var row : array<uint8>#) {
            tt |> equal(int(row[0]), 99)
        }
    }
}

// ---- resize ----

[test]
def test_resize_downscale(t : T?) {
    t |> run("resize 64x64 to 32x32") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var small <- img.resize(32, 32)
        tt |> equal(small.width, 32)
        tt |> equal(small.height, 32)
        tt |> equal(small.channels, 4)
        tt |> equal(small.bpc, 1)
        tt |> success(small.valid(), "resized is valid")
    }
}

[test]
def test_resize_upscale(t : T?) {
    t |> run("resize 64x64 to 128x128") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var big <- img.resize(128, 128)
        tt |> equal(big.width, 128)
        tt |> equal(big.height, 128)
        tt |> equal(big.channels, 4)
    }
}

[test]
def test_resize_uniform_color(t : T?) {
    t |> run("resize uniform white stays white") <| @(tt : T?) {
        var img : Image
        img.width = 2
        img.height = 2
        img.channels = 4
        img.bpc = 1
        img.bytes |> resize(2 * 2 * 4)
        for (i in range(16)) {
            img.bytes[i] = uint8(255)
        }
        var big <- img.resize(4, 4)
        big |> with_pixels() <| $(var pixels : array<uint8>#) {
            for (i in range(length(pixels))) {
                tt |> equal(int(pixels[i]), 255)
            }
        }
    }
}

[test]
def test_resize_with_filter(t : T?) {
    t |> run("resize with POINT_SAMPLE filter") <| @(tt : T?) {
        var img : Image
        img.width = 1
        img.height = 1
        img.channels = 4
        img.bpc = 1
        img.bytes |> resize(4)
        img.bytes[0] = uint8(255)
        img.bytes[1] = uint8(0)
        img.bytes[2] = uint8(0)
        img.bytes[3] = uint8(255)
        var big <- img.resize(2, 2, stbir_filter.STBIR_FILTER_POINT_SAMPLE)
        big |> with_pixels() <| $(var pixels : array<uint8>#) {
            for (p in range(4)) {
                tt |> equal(int(pixels[p * 4 + 0]), 255)
                tt |> equal(int(pixels[p * 4 + 1]), 0)
                tt |> equal(int(pixels[p * 4 + 2]), 0)
                tt |> equal(int(pixels[p * 4 + 3]), 255)
            }
        }
    }
}

[test]
def test_resize_hdr(t : T?) {
    t |> run("resize HDR preserves bpc=4") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        var small <- img.resize(32, 32)
        tt |> equal(small.bpc, 4)
        tt |> equal(small.width, 32)
        tt |> equal(small.height, 32)
        tt |> success(small.valid(), "resized HDR is valid")
    }
}

// ---- flip vertical ----

[test]
def test_flip_vertical(t : T?) {
    t |> run("flip vertical reverses rows") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var flipped <- img.flip_vertical()
        // original pixel(0,0) = R=0,G=0 should now be at row 63
        flipped |> with_row(63) <| $(var row : array<uint8>#) {
            tt |> equal(int(row[0]), 0)
            tt |> equal(int(row[1]), 0)
            tt |> equal(int(row[2]), 128)
        }
        // original pixel(0,63) = R=0,G=252 should now be at row 0
        flipped |> with_row(0) <| $(var row : array<uint8>#) {
            tt |> equal(int(row[0]), 0)
            tt |> equal(int(row[1]), 252)
        }
    }
}

[test]
def test_flip_vertical_roundtrip(t : T?) {
    t |> run("double flip restores original") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var flipped <- img.flip_vertical()
        var restored <- flipped.flip_vertical()
        for (i in range(16)) {
            tt |> equal(int(img.bytes[i]), int(restored.bytes[i]))
        }
    }
}

[test]
def test_flip_vertical_hdr(t : T?) {
    t |> run("flip vertical on HDR preserves bpc") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        var flipped <- img.flip_vertical()
        tt |> equal(flipped.bpc, 4)
        tt |> equal(flipped.width, 64)
        tt |> equal(flipped.height, 64)
    }
}

// ---- standalone functions ----

[test]
def test_load_image_convenience(t : T?) {
    t |> run("load_image convenience returns valid Image") <| @(tt : T?) {
        var img <- load_image(test_image_path("test.png"))
        tt |> success(img.valid(), "loaded and valid")
        tt |> equal(img.width, 64)
    }
}

[test]
def test_load_image_with_error(t : T?) {
    t |> run("load_image with error out-param") <| @(tt : T?) {
        var error : string
        var img <- load_image(test_image_path("does_not_exist.png"), error)
        tt |> success(!img.valid(), "not valid")
        tt |> success(length(error) > 0, "error populated")
    }
}

[test]
def test_image_info(t : T?) {
    t |> run("image_info returns dimensions") <| @(tt : T?) {
        let info = image_info(test_image_path("test.png"))
        tt |> equal(info.w, 64)
        tt |> equal(info.h, 64)
        tt |> equal(info.comp, 4)
    }
}

[test]
def test_is_hdr_standalone(t : T?) {
    t |> run("is_hdr standalone function") <| @(tt : T?) {
        tt |> success(stbimage_boost::is_hdr(test_image_path("test.hdr")), "HDR detected")
        tt |> success(!stbimage_boost::is_hdr(test_image_path("test.png")), "PNG not HDR")
    }
}

[test]
def test_is_16_bit_standalone(t : T?) {
    t |> run("is_16_bit standalone function") <| @(tt : T?) {
        tt |> success(!stbimage_boost::is_16_bit(test_image_path("test.png")), "8-bit PNG not 16-bit")
    }
}

[test]
def test_is_hdr_from_memory_standalone(t : T?) {
    t |> run("is_hdr_from_memory on PNG bytes") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var png_bytes : array<uint8>
        img.encode("png", png_bytes)
        tt |> success(!is_hdr_from_memory(png_bytes), "PNG bytes not HDR")
    }
}

// ---- roundtrip integrity ----

[test]
def test_save_load_pixel_roundtrip(t : T?) {
    t |> run("save PNG then load back preserves pixels") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        let out_path = test_image_path("_test_roundtrip.png")
        img.save(out_path)
        var img2 : Image
        img2.load(out_path)
        tt |> equal(length(img.bytes), length(img2.bytes))
        var mismatch = 0
        for (i in range(length(img.bytes))) {
            if (img.bytes[i] != img2.bytes[i]) {
                mismatch++
            }
        }
        tt |> equal(mismatch, 0)
        fio::remove(out_path)
    }
}

[test]
def test_encode_decode_roundtrip(t : T?) {
    t |> run("encode to PNG memory then decode preserves pixels") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var png_bytes : array<uint8>
        img.encode("png", png_bytes)
        var img2 : Image
        img2.load_from_memory(png_bytes)
        tt |> equal(img.width, img2.width)
        tt |> equal(img.height, img2.height)
        tt |> equal(img.channels, img2.channels)
        var mismatch = 0
        for (i in range(length(img.bytes))) {
            if (img.bytes[i] != img2.bytes[i]) {
                mismatch++
            }
        }
        tt |> equal(mismatch, 0)
    }
}

// ---- make_image ----

[test]
def test_make_image(t : T?) {
    t |> run("make_image creates blank image") <| @(tt : T?) {
        var img <- make_image(32, 16, 4)
        tt |> equal(img.width, 32)
        tt |> equal(img.height, 16)
        tt |> equal(img.channels, 4)
        tt |> equal(img.bpc, 1)
        tt |> equal(length(img.bytes), 32 * 16 * 4)
        // all bytes should be zero
        var nonzero = 0
        for (i in range(length(img.bytes))) {
            if (img.bytes[i] != uint8(0)) {
                nonzero++
            }
        }
        tt |> equal(nonzero, 0)
    }
}

[test]
def test_make_image_hdr(t : T?) {
    t |> run("make_image with bpc=4 creates HDR image") <| @(tt : T?) {
        var img <- make_image(8, 8, 3, 4)
        tt |> equal(img.bpc, 4)
        tt |> success(img.is_hdr(), "is HDR")
        tt |> equal(length(img.bytes), 8 * 8 * 3 * 4)
    }
}

// ---- save HDR ----

[test]
def test_save_hdr(t : T?) {
    t |> run("save as HDR and load back") <| @(tt : T?) {
        var img : Image
        img.load_hdr(test_image_path("test.hdr"), 4)
        let out_path = test_image_path("_test_out.hdr")
        let (ok, error) = img.save(out_path)
        tt |> success(ok, "save ok: {error}")
        var img2 : Image
        let (ok2, error2) = img2.load_hdr(out_path, 4)
        tt |> success(ok2, "reload ok: {error2}")
        tt |> equal(img2.width, 64)
        tt |> equal(img2.height, 64)
        tt |> equal(img2.bpc, 4)
        fio::remove(out_path)
    }
}

[test]
def test_save_jpg_quality(t : T?) {
    t |> run("save JPG with explicit quality") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"), 3)
        let out_low = test_image_path("_test_q10.jpg")
        let out_high = test_image_path("_test_q100.jpg")
        let (ok1, err1) = img.save(out_low, 10)
        tt |> success(ok1, "save q10 ok: {err1}")
        let (ok2, err2) = img.save(out_high, 100)
        tt |> success(ok2, "save q100 ok: {err2}")
        // higher quality should produce a larger file
        var sz_low, sz_high : int
        fopen(out_low, "rb") <| $(f) {
            if (f != null) {
                fmap(f) <| $(var data : array<uint8>#) {
                    sz_low = length(data)
                }
            }
        }
        fopen(out_high, "rb") <| $(f) {
            if (f != null) {
                fmap(f) <| $(var data : array<uint8>#) {
                    sz_high = length(data)
                }
            }
        }
        tt |> success(sz_high > sz_low, "q100 ({sz_high}) > q10 ({sz_low})")
        fio::remove(out_low)
        fio::remove(out_high)
    }
}

// ---- is_16_bit_from_memory ----

[test]
def test_is_16_bit_from_memory_standalone(t : T?) {
    t |> run("is_16_bit_from_memory on PNG bytes") <| @(tt : T?) {
        var img : Image
        img.load(test_image_path("test.png"))
        var png_bytes : array<uint8>
        img.encode("png", png_bytes)
        tt |> success(!is_16_bit_from_memory(png_bytes), "8-bit PNG bytes not 16-bit")
    }
}

// ---- make_image with bpc=2 ----

[test]
def test_make_image_16(t : T?) {
    t |> run("make_image with bpc=2 creates 16-bit image") <| @(tt : T?) {
        var img <- make_image(8, 8, 4, 2)
        tt |> equal(img.bpc, 2)
        tt |> equal(img.channels, 4)
        tt |> equal(length(img.bytes), 8 * 8 * 4 * 2)
        tt |> success(img.valid(), "16-bit image valid")
    }
}
