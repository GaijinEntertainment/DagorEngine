options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage
require daslib/safe_addr
require dastest/testing_boost public

// ---- flip vertically on load ----

[test]
def test_flip_vertically_on_load(t : T?) {
    t |> run("flip vertically on load changes pixel order") <| @(tt : T?) {
        // create a 2x1 image: top row = red, bottom row = blue
        // Actually use 1x2 with stride=4 (RGBA): row0=[255,0,0,255] row1=[0,0,255,255]
        var pixels : uint8[8]
        // row 0: red
        pixels[0] = uint8(255); pixels[1] = uint8(0); pixels[2] = uint8(0); pixels[3] = uint8(255)
        // row 1: blue
        pixels[4] = uint8(0); pixels[5] = uint8(0); pixels[6] = uint8(255); pixels[7] = uint8(255)
        // encode to PNG
        var png_data : array<uint8>
        stbi_write_png_to_memory(1, 2, 4, safe_addr(pixels[0]), 4) <| $(data : array<uint8>#) {
            png_data := data
        }
        // load without flip
        stbi_set_flip_vertically_on_load(0)
        var x, y, comp : int
        var normal = stbi_load_from_memory(unsafe(addr(png_data[0])), length(png_data), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(normal != null, "normal load")
        // first pixel should be red (R=255)
        tt |> equal(int(unsafe(normal[0])), 255)
        tt |> equal(int(unsafe(normal[2])), 0)
        stbi_image_free(normal)
        // load with flip
        stbi_set_flip_vertically_on_load(1)
        var flipped = stbi_load_from_memory(unsafe(addr(png_data[0])), length(png_data), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(flipped != null, "flipped load")
        // first pixel should now be blue (R=0, B=255)
        tt |> equal(int(unsafe(flipped[0])), 0)
        tt |> equal(int(unsafe(flipped[2])), 255)
        stbi_image_free(flipped)
        // reset
        stbi_set_flip_vertically_on_load(0)
    }
}

// ---- flip vertically on write ----

[test]
def test_flip_vertically_on_write(t : T?) {
    t |> run("flip on write reverses row order") <| @(tt : T?) {
        var pixels : uint8[8]
        // row 0: red
        pixels[0] = uint8(255); pixels[1] = uint8(0); pixels[2] = uint8(0); pixels[3] = uint8(255)
        // row 1: blue
        pixels[4] = uint8(0); pixels[5] = uint8(0); pixels[6] = uint8(255); pixels[7] = uint8(255)
        // write with flip enabled
        stbi_flip_vertically_on_write(1)
        var flipped_png : array<uint8>
        stbi_write_png_to_memory(1, 2, 4, safe_addr(pixels[0]), 4) <| $(data : array<uint8>#) {
            flipped_png := data
        }
        stbi_flip_vertically_on_write(0)
        // decode: first pixel should be blue (was row1, now written as row0)
        stbi_set_flip_vertically_on_load(0)
        var x, y, comp : int
        var loaded = stbi_load_from_memory(unsafe(addr(flipped_png[0])), length(flipped_png), safe_addr(x), safe_addr(y), safe_addr(comp), 4)
        tt |> success(loaded != null)
        tt |> equal(int(unsafe(loaded[0])), 0)      // R=0 (was blue)
        tt |> equal(int(unsafe(loaded[2])), 255)     // B=255
        stbi_image_free(loaded)
    }
}

// ---- PNG compression level ----

[test]
def test_png_compression_level(t : T?) {
    t |> run("set and get PNG compression level") <| @(tt : T?) {
        let original = stbi_write_get_png_compression_level()
        stbi_write_set_png_compression_level(1)
        tt |> equal(stbi_write_get_png_compression_level(), 1)
        stbi_write_set_png_compression_level(9)
        tt |> equal(stbi_write_get_png_compression_level(), 9)
        // restore
        stbi_write_set_png_compression_level(original)
    }
}

// ---- PNG filter ----

[test]
def test_force_png_filter(t : T?) {
    t |> run("set and get force PNG filter") <| @(tt : T?) {
        let original = stbi_write_get_force_png_filter()
        stbi_write_set_force_png_filter(0)
        tt |> equal(stbi_write_get_force_png_filter(), 0)
        stbi_write_set_force_png_filter(4)
        tt |> equal(stbi_write_get_force_png_filter(), 4)
        stbi_write_set_force_png_filter(original)
    }
}

// ---- TGA RLE ----

[test]
def test_tga_with_rle(t : T?) {
    t |> run("set and get TGA RLE flag") <| @(tt : T?) {
        let original = stbi_write_get_tga_with_rle()
        stbi_write_set_tga_with_rle(0)
        tt |> equal(stbi_write_get_tga_with_rle(), 0)
        stbi_write_set_tga_with_rle(1)
        tt |> equal(stbi_write_get_tga_with_rle(), 1)
        stbi_write_set_tga_with_rle(original)
    }
}

// ---- HDR gamma/scale ----

[test]
def test_hdr_gamma_scale(t : T?) {
    t |> run("HDR gamma and scale setters do not crash") <| @(tt : T?) {
        // these are fire-and-forget setters with no getters, just verify no crash
        stbi_hdr_to_ldr_gamma(2.2f)
        stbi_hdr_to_ldr_scale(1.0f)
        stbi_ldr_to_hdr_gamma(2.2f)
        stbi_ldr_to_hdr_scale(1.0f)
        tt |> success(true, "gamma/scale setters completed")
    }
}

// ---- unpremultiply on load ----

[test]
def test_unpremultiply_on_load(t : T?) {
    t |> run("set_unpremultiply_on_load does not crash") <| @(tt : T?) {
        stbi_set_unpremultiply_on_load(1)
        stbi_set_unpremultiply_on_load(0)
        tt |> success(true)
    }
}

// ---- convert iphone png to rgb ----

[test]
def test_convert_iphone_png_to_rgb(t : T?) {
    t |> run("convert_iphone_png_to_rgb does not crash") <| @(tt : T?) {
        stbi_convert_iphone_png_to_rgb(1)
        stbi_convert_iphone_png_to_rgb(0)
        tt |> success(true)
    }
}
