options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage
require daslib/safe_addr
require dastest/testing_boost public
require strings

// Helper: create a tiny 2x2 RGBA PNG in memory via write-to-memory, return the bytes
def make_test_png(var out_data : array<uint8>) {
    var pixels : uint8[16]
    for (i in range(16)) {
        pixels[i] = uint8(i * 16)
    }
    stbi_write_png_to_memory(2, 2, 4, safe_addr(pixels[0]), 8) <| $(data : array<uint8>#) {
        out_data := data
    }
}

// Helper: create a 2x2 RGB BMP in memory
def make_test_bmp(var out_data : array<uint8>) {
    var pixels : uint8[12]
    for (i in range(12)) {
        pixels[i] = uint8(i * 20)
    }
    stbi_write_bmp_to_memory(2, 2, 3, safe_addr(pixels[0])) <| $(data : array<uint8>#) {
        out_data := data
    }
}

// ---- stbi_load_from_memory ----

[test]
def test_load_from_memory_png(t : T?) {
    t |> run("load PNG from memory returns valid pixels") <| @(tt : T?) {
        var png_data : array<uint8>
        make_test_png(png_data)
        tt |> success(length(png_data) > 0, "PNG data generated")
        var x, y, comp : int
        var pixels = stbi_load_from_memory(unsafe(addr(png_data[0])), length(png_data), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "pixels loaded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        tt |> equal(comp, 4)
        stbi_image_free(pixels)
    }
}

[test]
def test_load_from_memory_bmp(t : T?) {
    t |> run("load BMP from memory returns correct dimensions") <| @(tt : T?) {
        var bmp_data : array<uint8>
        make_test_bmp(bmp_data)
        var x, y, comp : int
        var pixels = stbi_load_from_memory(unsafe(addr(bmp_data[0])), length(bmp_data), safe_addr(x), safe_addr(y), safe_addr(comp), 0)
        tt |> success(pixels != null, "pixels loaded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        tt |> equal(comp, 3)
        stbi_image_free(pixels)
    }
}

[test]
def test_load_from_memory_invalid(t : T?) {
    t |> run("load from invalid data returns null") <| @(tt : T?) {
        var garbage : uint8[4]
        var x, y, comp : int
        var pixels = stbi_load_from_memory(unsafe(addr(garbage[0])), 4, safe_addr(x), safe_addr(y), safe_addr(comp), 0)
        tt |> success(pixels == null, "null for invalid data")
        var reason = stbi_failure_reason()
        tt |> success(length(reason) > 0, "failure reason provided")
    }
}

// ---- stbi_info_from_memory ----

[test]
def test_info_from_memory(t : T?) {
    t |> run("info_from_memory returns dimensions without loading pixels") <| @(tt : T?) {
        var png_data : array<uint8>
        make_test_png(png_data)
        var x, y, comp : int
        var ok = stbi_info_from_memory(unsafe(addr(png_data[0])), length(png_data), safe_addr(x), safe_addr(y), safe_addr(comp))
        tt |> success(ok != 0, "info returned ok")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        tt |> equal(comp, 4)
    }
}

[test]
def test_info_from_memory_invalid(t : T?) {
    t |> run("info_from_memory returns 0 for invalid data") <| @(tt : T?) {
        var garbage : uint8[4]
        var x, y, comp : int
        var ok = stbi_info_from_memory(unsafe(addr(garbage[0])), 4, safe_addr(x), safe_addr(y), safe_addr(comp))
        tt |> equal(ok, 0)
    }
}

// ---- stbi_is_hdr_from_memory ----

[test]
def test_is_hdr_from_memory(t : T?) {
    t |> run("PNG is not HDR") <| @(tt : T?) {
        var png_data : array<uint8>
        make_test_png(png_data)
        var result = stbi_is_hdr_from_memory(unsafe(addr(png_data[0])), length(png_data))
        tt |> equal(result, 0)
    }
}

// ---- stbi_is_16_bit_from_memory ----

[test]
def test_is_16_bit_from_memory(t : T?) {
    t |> run("8-bit PNG is not 16-bit") <| @(tt : T?) {
        var png_data : array<uint8>
        make_test_png(png_data)
        var result = stbi_is_16_bit_from_memory(unsafe(addr(png_data[0])), length(png_data))
        tt |> equal(result, 0)
    }
}

// ---- stbi_load_16_from_memory ----

[test]
def test_load_16_from_memory(t : T?) {
    t |> run("load_16 from 8-bit PNG returns upscaled 16-bit") <| @(tt : T?) {
        var png_data : array<uint8>
        make_test_png(png_data)
        var x, y, comp : int
        var pixels = stbi_load_16_from_memory(unsafe(addr(png_data[0])), length(png_data), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "16-bit pixels loaded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        stbi_image_free(pixels)
    }
}

// ---- stbi_loadf_from_memory ----

[test]
def test_loadf_from_memory(t : T?) {
    t |> run("loadf from PNG returns float pixels") <| @(tt : T?) {
        var png_data : array<uint8>
        make_test_png(png_data)
        var x, y, comp : int
        var pixels = stbi_loadf_from_memory(unsafe(addr(png_data[0])), length(png_data), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(pixels != null, "float pixels loaded")
        tt |> equal(x, 2)
        tt |> equal(y, 2)
        // first pixel R channel should be 0.0 (original was 0)
        tt |> success(unsafe(pixels[0]) < 0.01f, "first float pixel near zero")
        stbi_image_free(pixels)
    }
}

// ---- stbi_failure_reason ----

[test]
def test_failure_reason(t : T?) {
    t |> run("failure_reason returns meaningful string after failed load") <| @(tt : T?) {
        var garbage : uint8[4]
        var x, y, comp : int
        var pixels = stbi_load_from_memory(unsafe(addr(garbage[0])), 4, safe_addr(x), safe_addr(y), safe_addr(comp), 0)
        tt |> success(pixels == null)
        var reason = stbi_failure_reason()
        tt |> success(length(reason) > 0, "got failure reason")
    }
}
