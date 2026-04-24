options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage
require math
require daslib/safe_addr
require dastest/testing_boost public

def test_image_path(name : string) : string {
    return "{get_das_root()}/tests/stbimage/{name}"
}

// gradient pattern: pixel(x,y) = R=x*4, G=y*4, B=128, A=255
def verify_pixel_exact(tt : T?; pixels : uint8?; stride : int; px, py, expected_r, expected_g, expected_b, expected_a : int) {
    let idx = (py * 64 + px) * stride
    tt |> equal(int(unsafe(pixels[idx + 0])), expected_r)
    tt |> equal(int(unsafe(pixels[idx + 1])), expected_g)
    tt |> equal(int(unsafe(pixels[idx + 2])), expected_b)
    if (stride == 4) {
        tt |> equal(int(unsafe(pixels[idx + 3])), expected_a)
    }
}

def verify_pixel_lossy(tt : T?; pixels : uint8?; stride : int; px, py, expected_r, expected_g, expected_b : int; tolerance : int) {
    let idx = (py * 64 + px) * stride
    let r = int(unsafe(pixels[idx + 0]))
    let g = int(unsafe(pixels[idx + 1]))
    let b = int(unsafe(pixels[idx + 2]))
    tt |> success(abs(r - expected_r) <= tolerance, "R within tolerance")
    tt |> success(abs(g - expected_g) <= tolerance, "G within tolerance")
    tt |> success(abs(b - expected_b) <= tolerance, "B within tolerance")
}

// ---- stbi_load from file ----

[test]
def test_load_png(t : T?) {
    t |> run("load PNG from file with pixel verification") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_load(test_image_path("test.png"), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "pixels loaded")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        tt |> equal(comp, 4)
        // corner pixels: (0,0)=(0,0,128,255), (63,0)=(252,0,128,255), (0,63)=(0,252,128,255), (63,63)=(252,252,128,255)
        verify_pixel_exact(tt, pixels, 4, 0, 0, 0, 0, 128, 255)
        verify_pixel_exact(tt, pixels, 4, 63, 0, 252, 0, 128, 255)
        verify_pixel_exact(tt, pixels, 4, 0, 63, 0, 252, 128, 255)
        verify_pixel_exact(tt, pixels, 4, 63, 63, 252, 252, 128, 255)
        // mid pixel: (32,32)=(128,128,128,255)
        verify_pixel_exact(tt, pixels, 4, 32, 32, 128, 128, 128, 255)
        stbi_image_free(pixels)
    }
}

[test]
def test_load_bmp(t : T?) {
    t |> run("load BMP from file with pixel verification") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_load(test_image_path("test.bmp"), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "pixels loaded")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        verify_pixel_exact(tt, pixels, 4, 0, 0, 0, 0, 128, 255)
        verify_pixel_exact(tt, pixels, 4, 63, 63, 252, 252, 128, 255)
        verify_pixel_exact(tt, pixels, 4, 32, 32, 128, 128, 128, 255)
        stbi_image_free(pixels)
    }
}

[test]
def test_load_tga(t : T?) {
    t |> run("load TGA from file with pixel verification") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_load(test_image_path("test.tga"), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "pixels loaded")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        verify_pixel_exact(tt, pixels, 4, 0, 0, 0, 0, 128, 255)
        verify_pixel_exact(tt, pixels, 4, 63, 63, 252, 252, 128, 255)
        verify_pixel_exact(tt, pixels, 4, 32, 32, 128, 128, 128, 255)
        stbi_image_free(pixels)
    }
}

[test]
def test_load_jpg(t : T?) {
    t |> run("load JPG from file with lossy pixel verification") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_load(test_image_path("test.jpg"), safe_addr(x), safe_addr(y), safe_addr(comp), 3)
        tt |> success(pixels != null, "pixels loaded")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        tt |> equal(comp, 3)
        // JPEG is lossy — allow tolerance of 10
        verify_pixel_lossy(tt, pixels, 3, 0, 0, 0, 0, 128, 10)
        verify_pixel_lossy(tt, pixels, 3, 63, 63, 252, 252, 128, 10)
        verify_pixel_lossy(tt, pixels, 3, 32, 32, 128, 128, 128, 10)
        stbi_image_free(pixels)
    }
}

// ---- stbi_info from file ----

[test]
def test_info_png(t : T?) {
    t |> run("info PNG from file") <| @(tt : T?) {
        var x, y, comp : int
        var ok = stbi_info(test_image_path("test.png"), safe_addr(x), safe_addr(y), safe_addr(comp))
        tt |> success(ok != 0, "info ok")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        tt |> equal(comp, 4)
    }
}

[test]
def test_info_bmp(t : T?) {
    t |> run("info BMP from file") <| @(tt : T?) {
        var x, y, comp : int
        var ok = stbi_info(test_image_path("test.bmp"), safe_addr(x), safe_addr(y), safe_addr(comp))
        tt |> success(ok != 0, "info ok")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
    }
}

[test]
def test_info_jpg(t : T?) {
    t |> run("info JPG from file") <| @(tt : T?) {
        var x, y, comp : int
        var ok = stbi_info(test_image_path("test.jpg"), safe_addr(x), safe_addr(y), safe_addr(comp))
        tt |> success(ok != 0, "info ok")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        tt |> equal(comp, 3)
    }
}

// ---- stbi_load nonexistent file ----

[test]
def test_load_nonexistent(t : T?) {
    t |> run("load nonexistent file returns null") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_load(test_image_path("does_not_exist.png"), safe_addr(x), safe_addr(y), safe_addr(comp), 0)
        tt |> success(pixels == null, "null for missing file")
    }
}

// ---- HDR file ----

[test]
def test_is_hdr_file(t : T?) {
    t |> run("HDR file detected as HDR") <| @(tt : T?) {
        var ok = stbi_is_hdr(test_image_path("test.hdr"))
        tt |> success(ok != 0, "is HDR")
    }
}

[test]
def test_is_not_hdr_png(t : T?) {
    t |> run("PNG file not detected as HDR") <| @(tt : T?) {
        var ok = stbi_is_hdr(test_image_path("test.png"))
        tt |> equal(ok, 0)
    }
}

[test]
def test_loadf_hdr(t : T?) {
    t |> run("loadf HDR from file with pixel verification") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_loadf(test_image_path("test.hdr"), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "float pixels loaded")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        // pixel(0,0): R=0/64~0, G=0/64~0, B=0.5, A=1.0
        tt |> success(unsafe(pixels[0]) < 0.05f, "R near 0")
        tt |> success(unsafe(pixels[1]) < 0.05f, "G near 0")
        tt |> success(unsafe(pixels[2]) > 0.3f && unsafe(pixels[2]) < 0.7f, "B near 0.5")
        tt |> success(unsafe(pixels[3]) > 0.9f, "A near 1.0")
        // pixel(63,63): R=63/64~0.98, G=63/64~0.98, B=0.5, A=1.0
        let last = (63 * 64 + 63) * 4
        tt |> success(unsafe(pixels[last + 0]) > 0.9f, "last R near 1.0")
        tt |> success(unsafe(pixels[last + 1]) > 0.9f, "last G near 1.0")
        tt |> success(unsafe(pixels[last + 2]) > 0.3f && unsafe(pixels[last + 2]) < 0.7f, "last B near 0.5")
        stbi_image_free(pixels)
    }
}

// ---- stbi_load_16 from file ----

[test]
def test_load_16_png(t : T?) {
    t |> run("load_16 PNG from file") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_load_16(test_image_path("test.png"), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "16-bit pixels loaded")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        stbi_image_free(pixels)
    }
}

// ---- stbi_is_16_bit from file ----

[test]
def test_is_16_bit_file(t : T?) {
    t |> run("8-bit PNG not detected as 16-bit") <| @(tt : T?) {
        var ok = stbi_is_16_bit(test_image_path("test.png"))
        tt |> equal(ok, 0)
    }
}

// ---- loadf from non-HDR file ----

[test]
def test_loadf_png(t : T?) {
    t |> run("loadf PNG converts to float") <| @(tt : T?) {
        var x, y, comp : int
        var pixels = stbi_loadf(test_image_path("test.png"), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "float pixels loaded")
        tt |> equal(x, 64)
        tt |> equal(y, 64)
        // pixel(32,32): uint8 values are (128,128,128) → stbi_loadf applies gamma 2.2 → ~0.217 linear
        let mid = (32 * 64 + 32) * 4
        tt |> success(unsafe(pixels[mid + 0]) > 0.15f && unsafe(pixels[mid + 0]) < 0.30f, "mid R near 0.22")
        tt |> success(unsafe(pixels[mid + 1]) > 0.15f && unsafe(pixels[mid + 1]) < 0.30f, "mid G near 0.22")
        tt |> success(unsafe(pixels[mid + 2]) > 0.15f && unsafe(pixels[mid + 2]) < 0.30f, "mid B near 0.22")
        stbi_image_free(pixels)
    }
}
