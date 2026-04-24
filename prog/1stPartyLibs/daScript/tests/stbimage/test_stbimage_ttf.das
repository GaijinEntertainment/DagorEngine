options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbimage/stbimage_ttf
require stbimage/stbimage_boost
require math
require daslib/fio
require dastest/testing_boost public

def font_path() : string {
    return "{get_das_root()}/modules/dasStbImage/fonts/droidsansmono.ttf"
}

// ---- load_ttf ----

[test]
def test_load_ttf_basic(t : T?) {
    t |> run("load_ttf loads font and creates valid atlas") <| @(tt : T?) {
        var font <- load_ttf(font_path())
        tt |> success(font.bitmap.valid(), "bitmap is valid")
        tt |> equal(font.bitmap.width, 512)
        tt |> equal(font.bitmap.height, 512)
        tt |> equal(font.bitmap.channels, 1)
        tt |> equal(font.bitmap.bpc, 1)
        tt |> success(font.pixel_height > 0.0f, "pixel_height set")
        tt |> equal(font.char_range.x, 32)
        tt |> equal(font.char_range.y, 96)
        tt |> success(length(font.chardata) == 96, "chardata has 96 entries")
        tt |> success(font.tex == uint64(0), "tex is 0 by default")
        // atlas should have non-zero pixels
        var non_zero = 0
        for (px in font.bitmap.bytes) {
            if (px != uint8(0)) {
                non_zero++
            }
        }
        tt |> success(non_zero > 0, "atlas has rendered glyphs")
    }
}

[test]
def test_load_ttf_custom_params(t : T?) {
    t |> run("load_ttf with custom atlas size and pixel height") <| @(tt : T?) {
        var font <- load_ttf(font_path(), [pixel_height = 48.0f, atlas_dim = int2(256, 256)])
        tt |> success(font.bitmap.valid(), "bitmap is valid")
        tt |> equal(font.bitmap.width, 256)
        tt |> equal(font.bitmap.height, 256)
        tt |> success(font.pixel_height == 48.0f, "pixel_height is 48")
    }
}

[test]
def test_load_ttf_nonexistent(t : T?) {
    t |> run("load_ttf with nonexistent file returns invalid font") <| @(tt : T?) {
        var font <- load_ttf("/does/not/exist.ttf")
        tt |> success(!font.bitmap.valid(), "bitmap is invalid for missing file")
        tt |> success(empty(font.font_data), "no font data loaded")
    }
}

// ---- font_metrics ----

[test]
def test_font_metrics(t : T?) {
    t |> run("font_metrics returns valid ascent, descent, line_height") <| @(tt : T?) {
        var font <- load_ttf(font_path())
        let m = font_metrics(font)
        tt |> success(m.ascent > 0.0f, "ascent is positive")
        tt |> success(m.descent < 0.0f, "descent is negative")
        tt |> success(m.line_height > 0.0f, "line height is positive")
        tt |> success(m.line_height >= m.ascent - m.descent - 0.01f, "line height >= ascent - descent")
    }
}

// ---- render_text ----

[test]
def test_render_text_basic(t : T?) {
    t |> run("render_text produces non-zero pixels") <| @(tt : T?) {
        var font <- load_ttf(font_path())
        let m = font_metrics(font)
        var img = make_image(200, 50, 4)
        // fill black
        img.fill_rect(0, 0, 200, 50, uint(0xFF000000u))
        render_text(img, font, "Hi", 10, int(m.ascent) + 5, 255, 255, 255)
        // check that some pixels were written (not all black)
        var non_black = 0
        let npx = 200 * 50
        for (i in range(npx)) {
            let idx = i * 4
            if (img.bytes[idx] != uint8(0) || img.bytes[idx + 1] != uint8(0) || img.bytes[idx + 2] != uint8(0)) {
                non_black++
            }
        }
        tt |> success(non_black > 0, "render_text wrote some pixels")
    }
}

[test]
def test_render_text_empty(t : T?) {
    t |> run("render_text with empty string does nothing") <| @(tt : T?) {
        var font <- load_ttf(font_path())
        var img = make_image(100, 50, 4)
        img.fill_rect(0, 0, 100, 50, uint(0xFF000000u))
        render_text(img, font, "", 10, 30, 255, 255, 255)
        // all pixels should still be black
        var non_black = 0
        let npx = 100 * 50
        for (i in range(npx)) {
            let idx = i * 4
            if (img.bytes[idx] != uint8(0) || img.bytes[idx + 1] != uint8(0) || img.bytes[idx + 2] != uint8(0)) {
                non_black++
            }
        }
        tt |> equal(non_black, 0)
    }
}

[test]
def test_render_text_color(t : T?) {
    t |> run("render_text uses specified color") <| @(tt : T?) {
        var font <- load_ttf(font_path())
        let m = font_metrics(font)
        var img = make_image(200, 50, 4)
        img.fill_rect(0, 0, 200, 50, uint(0xFF000000u))
        // render in red
        render_text(img, font, "A", 10, int(m.ascent) + 5, 255, 0, 0)
        // find a non-black pixel — it should have red but no green/blue
        var found_red = false
        let npx = 200 * 50
        for (i in range(npx)) {
            let idx = i * 4
            if (int(img.bytes[idx]) > 0) {
                found_red = true
                tt |> equal(int(img.bytes[idx + 1]), 0)
                tt |> equal(int(img.bytes[idx + 2]), 0)
                break
            }
        }
        tt |> success(found_red, "found red pixel")
    }
}

// ---- font bitmap save ----

[test]
def test_font_bitmap_save(t : T?) {
    t |> run("font bitmap can be saved as PNG via Image API") <| @(tt : T?) {
        var font <- load_ttf(font_path())
        let path = "{get_das_root()}/tests/stbimage/_test_ttf_atlas.png"
        let (ok, err) = font.bitmap.save(path)
        tt |> success(ok, "save succeeded: {err}")
        // clean up
        remove(path)
    }
}
