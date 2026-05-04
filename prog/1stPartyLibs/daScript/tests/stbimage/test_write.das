options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage
require daslib/safe_addr
require daslib/fio
require dastest/testing_boost public

def make_rgba_pixels(var pixels : uint8[16]) {
    for (i in range(16)) {
        pixels[i] = uint8(i * 16)
    }
}

def make_rgb_pixels(var pixels : uint8[12]) {
    for (i in range(12)) {
        pixels[i] = uint8(i * 20)
    }
}

// ---- write to file + read back ----

[test]
def test_write_png_roundtrip(t : T?) {
    t |> run("write PNG to file and read back") <| @(tt : T?) {
        var pixels : uint8[16]
        make_rgba_pixels(pixels)
        let fname = "{get_das_root()}/tests/stbimage/_test_output.png"
        var ok = stbi_write_png(fname, 2, 2, 4, safe_addr(pixels[0]), 8)
        tt |> success(ok != 0, "write_png succeeded")
        // read back
        var x, y, comp : int
        var loaded = stbi_load(fname, safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(loaded != null, "read back succeeded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        stbi_image_free(loaded)
        remove(fname)
    }
}

[test]
def test_write_bmp_roundtrip(t : T?) {
    t |> run("write BMP to file and read back") <| @(tt : T?) {
        var pixels : uint8[12]
        make_rgb_pixels(pixels)
        let fname = "{get_das_root()}/tests/stbimage/_test_output.bmp"
        var ok = stbi_write_bmp(fname, 2, 2, 3, safe_addr(pixels[0]))
        tt |> success(ok != 0, "write_bmp succeeded")
        var x, y, comp : int
        var loaded = stbi_load(fname, safe_addr(x), safe_addr(y), safe_addr(comp), 0)
        tt |> success(loaded != null, "read back succeeded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        stbi_image_free(loaded)
        remove(fname)
    }
}

[test]
def test_write_tga_roundtrip(t : T?) {
    t |> run("write TGA to file and read back") <| @(tt : T?) {
        var pixels : uint8[16]
        make_rgba_pixels(pixels)
        let fname = "{get_das_root()}/tests/stbimage/_test_output.tga"
        var ok = stbi_write_tga(fname, 2, 2, 4, safe_addr(pixels[0]))
        tt |> success(ok != 0, "write_tga succeeded")
        var x, y, comp : int
        var loaded = stbi_load(fname, safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(loaded != null, "read back succeeded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        stbi_image_free(loaded)
        remove(fname)
    }
}

[test]
def test_write_jpg_roundtrip(t : T?) {
    t |> run("write JPG to file and read back") <| @(tt : T?) {
        var pixels : uint8[12]
        make_rgb_pixels(pixels)
        let fname = "{get_das_root()}/tests/stbimage/_test_output.jpg"
        var ok = stbi_write_jpg(fname, 2, 2, 3, safe_addr(pixels[0]), 90)
        tt |> success(ok != 0, "write_jpg succeeded")
        var x, y, comp : int
        var loaded = stbi_load(fname, safe_addr(x), safe_addr(y), safe_addr(comp), 0)
        tt |> success(loaded != null, "read back succeeded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        stbi_image_free(loaded)
        remove(fname)
    }
}

// ---- write to memory (block API) ----

[test]
def test_write_png_to_memory(t : T?) {
    t |> run("write PNG to memory produces valid PNG") <| @(tt : T?) {
        var pixels : uint8[16]
        make_rgba_pixels(pixels)
        stbi_write_png_to_memory(2, 2, 4, safe_addr(pixels[0]), 8) <| $(data : array<uint8>#) {
            tt |> success(length(data) > 8, "got PNG data")
            // PNG magic: 137 80 78 71 13 10 26 10
            tt |> equal(int(data[0]), 137)
            tt |> equal(int(data[1]), 80)
            tt |> equal(int(data[2]), 78)
            tt |> equal(int(data[3]), 71)
        }
    }
}

[test]
def test_write_bmp_to_memory(t : T?) {
    t |> run("write BMP to memory produces valid BMP") <| @(tt : T?) {
        var pixels : uint8[12]
        make_rgb_pixels(pixels)
        stbi_write_bmp_to_memory(2, 2, 3, safe_addr(pixels[0])) <| $(data : array<uint8>#) {
            tt |> success(length(data) > 14, "got BMP data")
            // BMP magic: 'B' 'M'
            tt |> equal(int(data[0]), 66)
            tt |> equal(int(data[1]), 77)
        }
    }
}

[test]
def test_write_jpg_to_memory(t : T?) {
    t |> run("write JPG to memory produces valid JPEG") <| @(tt : T?) {
        var pixels : uint8[12]
        make_rgb_pixels(pixels)
        stbi_write_jpg_to_memory(2, 2, 3, safe_addr(pixels[0]), 90) <| $(data : array<uint8>#) {
            tt |> success(length(data) > 2, "got JPEG data")
            // JPEG magic: FF D8
            tt |> equal(int(data[0]), 255)
            tt |> equal(int(data[1]), 216)
        }
    }
}

[test]
def test_write_tga_to_memory(t : T?) {
    t |> run("write TGA to memory produces data") <| @(tt : T?) {
        var pixels : uint8[16]
        make_rgba_pixels(pixels)
        stbi_write_tga_to_memory(2, 2, 4, safe_addr(pixels[0])) <| $(data : array<uint8>#) {
            tt |> success(length(data) > 0, "got TGA data")
        }
    }
}

// ---- write-to-memory roundtrip (encode then decode) ----

[test]
def test_write_read_roundtrip_memory(t : T?) {
    t |> run("PNG encode to memory then decode back matches") <| @(tt : T?) {
        var pixels : uint8[16]
        make_rgba_pixels(pixels)
        stbi_write_png_to_memory(2, 2, 4, safe_addr(pixels[0]), 8) <| $(data : array<uint8>#) {
            var x, y, comp : int
            var loaded = unsafe(stbi_load_from_memory(addr(data[0]), length(data), safe_addr(x), safe_addr(y), safe_addr(comp), 4))
            tt |> success(loaded != null, "decoded from memory")
            tt |> equal(x, 2)
            tt |> equal(y, 2)
            tt |> equal(comp, 4)
            // verify pixel data matches (PNG is lossless)
            if (loaded != null) {
                for (i in range(16)) {
                    tt |> equal(int(unsafe(loaded[i])), int(pixels[i]))
                }
            }
            stbi_image_free(loaded)
        }
    }
}
