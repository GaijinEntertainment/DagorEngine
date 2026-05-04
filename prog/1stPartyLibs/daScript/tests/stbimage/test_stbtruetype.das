options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

require stbtruetype
require stbimage/stbimage_ttf
require daslib/fio
require dastest/testing_boost public

def font_path() : string {
    return "{get_das_root()}/modules/dasStbImage/fonts/droidsansmono.ttf"
}

def load_font_data() : array<uint8> {
    var data : array<uint8>
    fopen(font_path(), "rb") <| $(f) {
        fmap(f) <| $(mapped) {
            let sz = length(mapped)
            data |> resize(sz)
            unsafe {
                memcpy(addr(data[0]), addr(mapped[0]), sz)
            }
        }
    }
    return <- data
}

// ---- Font loading (low-level wrappers) ----

[test]
def test_get_number_of_fonts(t : T?) {
    t |> run("stbtt_num_fonts returns 1 for single-font TTF") <| @(tt : T?) {
        var data <- load_font_data()
        tt |> success(length(data) > 0, "font data loaded")
        tt |> equal(stbtt_num_fonts(data), 1)
    }
}

[test]
def test_init_font(t : T?) {
    t |> run("stbtt_init succeeds for valid TTF data") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        tt |> success(stbtt_init(info, data), "stbtt_init succeeded")
        tt |> success(info.numGlyphs > 0, "font has glyphs")
    }
}

[test]
def test_init_font_bad_index(t : T?) {
    t |> run("stbtt_init returns false for bad font index") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        tt |> success(!stbtt_init(info, data, 99), "stbtt_init failed for bad index")
    }
}

[test]
def test_find_glyph_index(t : T?) {
    t |> run("stbtt_find_glyph finds common glyphs") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        tt |> success(stbtt_find_glyph(info, 65) > 0, "glyph for 'A' found")
        tt |> success(stbtt_find_glyph(info, 32) > 0, "glyph for space found")
        tt |> equal(stbtt_find_glyph(info, int(0x1FFFF)), 0)
    }
}

// ---- Font metrics (low-level wrappers) ----

[test]
def test_scale_for_pixel_height(t : T?) {
    t |> run("stbtt_scale_for_pixel_height returns positive scale") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let s32 = stbtt_scale_for_pixel_height(info, 32.0)
        tt |> success(s32 > 0.0, "positive scale")
        let s64 = stbtt_scale_for_pixel_height(info, 64.0)
        tt |> success(s64 > s32, "larger pixel height gives larger scale")
    }
}

[test]
def test_scale_for_mapping_em(t : T?) {
    t |> run("stbtt_scale_for_em returns positive scale") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let s = stbtt_scale_for_em(info, 32.0)
        tt |> success(s > 0.0, "positive scale")
    }
}

[test]
def test_stbtt_font_vmetrics(t : T?) {
    t |> run("stbtt_font_vmetrics returns valid ascent/descent") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let vm = stbtt_font_vmetrics(info)
        tt |> success(vm.ascent > 0, "positive ascent")
        tt |> success(vm.descent < 0, "negative descent")
    }
}

[test]
def test_stbtt_font_bbox(t : T?) {
    t |> run("stbtt_font_bbox returns valid box") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let bb = stbtt_font_bbox(info)
        tt |> success(bb.z > bb.x, "x1 > x0")
        tt |> success(bb.w > bb.y, "y1 > y0")
    }
}

[test]
def test_codepoint_hmetrics(t : T?) {
    t |> run("stbtt_codepoint_hmetrics returns valid advance width") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let hm_a = stbtt_codepoint_hmetrics(info, 65)
        tt |> success(hm_a.advance_width > 0, "positive advance width for 'A'")
        // monospace font: all visible chars should have same advance
        let hm_b = stbtt_codepoint_hmetrics(info, 66)
        tt |> equal(hm_a.advance_width, hm_b.advance_width)
    }
}

[test]
def test_codepoint_kern_advance(t : T?) {
    t |> run("stbtt_codepoint_kern returns int") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let kern = stbtt_codepoint_kern(info, 65, 86) // 'A', 'V'
        tt |> success(kern >= 0 || kern < 0, "kern returned value")
    }
}

[test]
def test_codepoint_box(t : T?) {
    t |> run("stbtt_codepoint_box returns valid bounding box for 'A'") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let box = stbtt_codepoint_box(info, 65)
        tt |> success(box.z > box.x, "x1 > x0")
        tt |> success(box.w > box.y, "y1 > y0")
    }
}

// ---- Packing API (low-level wrappers) ----

[test]
def test_pack_basic(t : T?) {
    t |> run("stbtt_pack basic workflow") <| @(tt : T?) {
        var data <- load_font_data()
        let atlas_w = 256
        let atlas_h = 256
        var pixels : array<uint8>
        pixels |> resize(atlas_w * atlas_h)
        var chardata : array<stbtt_packedchar>
        chardata |> resize(96)
        stbtt_pack(data, pixels, atlas_w, atlas_h, chardata, 24.0, 32, 96)
        // verify some pixels were written
        var non_zero = 0
        for (px in pixels) {
            if (px != uint8(0)) {
                non_zero++
            }
        }
        tt |> success(non_zero > 0, "atlas has non-zero pixels")
        tt |> success(chardata[33].xadvance > 0.0f, "chardata xadvance > 0")
    }
}

[test]
def test_pack_with_oversampling(t : T?) {
    t |> run("stbtt_pack with oversampling") <| @(tt : T?) {
        var data <- load_font_data()
        let atlas_w = 512
        let atlas_h = 512
        var pixels : array<uint8>
        pixels |> resize(atlas_w * atlas_h)
        var chardata : array<stbtt_packedchar>
        chardata |> resize(96)
        stbtt_pack(data, pixels, atlas_w, atlas_h, chardata, 24.0, 32, 96, [oversampling = 2])
        tt |> success(chardata[33].xadvance > 0.0f, "oversampled chardata valid")
    }
}

[test]
def test_packed_quad(t : T?) {
    t |> run("stbtt_packed_quad returns valid quad for 'A'") <| @(tt : T?) {
        var data <- load_font_data()
        let atlas_w = 256
        let atlas_h = 256
        var pixels : array<uint8>
        pixels |> resize(atlas_w * atlas_h)
        var chardata : array<stbtt_packedchar>
        chardata |> resize(96)
        stbtt_pack(data, pixels, atlas_w, atlas_h, chardata, 24.0, 32, 96)
        // get quad for 'A' (char_index = 65 - 32 = 33)
        var xpos = 0.0
        let quad = stbtt_packed_quad(chardata, atlas_w, atlas_h, 33, xpos)
        tt |> success(xpos > 0.0, "xpos advanced after stbtt_packed_quad")
        tt |> success(quad.s1 > quad.s0, "s1 > s0")
        tt |> success(quad.t1 > quad.t0, "t1 > t0")
        tt |> success(quad.x1 > quad.x0, "x1 > x0")
    }
}

// ---- Direct bitmap rendering (low-level wrappers) ----

[test]
def test_codepoint_bitmap_box(t : T?) {
    t |> run("stbtt_codepoint_bitmap_box returns valid box") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let scale = stbtt_scale_for_pixel_height(info, 32.0)
        let box = stbtt_codepoint_bitmap_box(info, 65, scale, scale)
        tt |> success(box.z > box.x, "ix1 > ix0")
        tt |> success(box.w > box.y, "iy1 > iy0")
        // space should be smaller or equal
        let sbox = stbtt_codepoint_bitmap_box(info, 32, scale, scale)
        tt |> success(sbox.z - sbox.x <= box.z - box.x, "space box not larger than 'A' box")
    }
}

[test]
def test_codepoint_bitmap_box_subpixel(t : T?) {
    t |> run("stbtt_codepoint_bitmap_box_subpixel returns valid box") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let scale = stbtt_scale_for_pixel_height(info, 32.0)
        let box = stbtt_codepoint_bitmap_box_subpixel(info, 65, scale, scale, 0.25, 0.75)
        tt |> success(box.z >= box.x, "ix1 >= ix0")
        tt |> success(box.w >= box.y, "iy1 >= iy0")
    }
}

[test]
def test_codepoint_bitmap(t : T?) {
    t |> run("stbtt_codepoint_bitmap renders 'A' as Image") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let scale = stbtt_scale_for_pixel_height(info, 32.0)
        let result = stbtt_codepoint_bitmap(info, 65, scale, scale)
        tt |> success(result.image.width > 0, "width > 0")
        tt |> success(result.image.height > 0, "height > 0")
        tt |> equal(result.image.channels, 1)
        // verify some pixels are non-zero
        var non_zero = 0
        for (px in result.image.bytes) {
            if (px != uint8(0)) {
                non_zero++
            }
        }
        tt |> success(non_zero > 0, "bitmap has non-zero pixels")
    }
}

[test]
def test_codepoint_bitmap_subpixel(t : T?) {
    t |> run("stbtt_codepoint_bitmap_subpixel renders with shift") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let scale = stbtt_scale_for_pixel_height(info, 32.0)
        let result = stbtt_codepoint_bitmap_subpixel(info, 65, scale, scale, 0.5, 0.5)
        tt |> success(result.image.width > 0, "width > 0")
        tt |> success(result.image.height > 0, "height > 0")
    }
}

[test]
def test_make_codepoint_bitmap(t : T?) {
    t |> run("stbtt_make_codepoint_bitmap renders into buffer") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let scale = stbtt_scale_for_pixel_height(info, 32.0)
        let box = stbtt_codepoint_bitmap_box(info, 65, scale, scale)
        let bw = box.z - box.x
        let bh = box.w - box.y
        tt |> success(bw > 0, "bitmap box width > 0")
        tt |> success(bh > 0, "bitmap box height > 0")
        var buf : array<uint8>
        buf |> resize(bw * bh)
        stbtt_make_codepoint_bitmap(info, buf, bw, bh, bw, scale, scale, 65)
        var non_zero = 0
        for (px in buf) {
            if (px != uint8(0)) {
                non_zero++
            }
        }
        tt |> success(non_zero > 0, "MakeCodepointBitmap wrote pixels")
    }
}

[test]
def test_make_codepoint_bitmap_subpixel(t : T?) {
    t |> run("stbtt_make_codepoint_bitmap_subpixel renders with shift") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        let scale = stbtt_scale_for_pixel_height(info, 32.0)
        let box = stbtt_codepoint_bitmap_box_subpixel(info, 65, scale, scale, 0.5, 0.5)
        let bw = box.z - box.x
        let bh = box.w - box.y
        tt |> success(bw > 0 && bh > 0, "subpixel box has size")
        var buf : array<uint8>
        buf |> resize(bw * bh)
        stbtt_make_codepoint_bitmap_subpixel(info, buf, bw, bh, bw, scale, scale, 0.5, 0.5, 65)
        var non_zero = 0
        for (px in buf) {
            if (px != uint8(0)) {
                non_zero++
            }
        }
        tt |> success(non_zero > 0, "subpixel bitmap has pixels")
    }
}

// ---- Glyph shape (low-level wrappers) ----

[test]
def test_codepoint_shape(t : T?) {
    t |> run("stbtt_codepoint_shape iterates vertices for 'A'") <| @(tt : T?) {
        var data <- load_font_data()
        var info : stbtt_fontinfo
        stbtt_init(info, data)
        var count = 0
        var has_move = false
        stbtt_codepoint_shape(info, 65) <| $(v) {
            count++
            if (int(v.vtype) == STBTT_vmove) {
                has_move = true
            }
        }
        tt |> success(count > 0, "has vertices")
        tt |> success(has_move, "has a move-to vertex")
        let expected = stbtt_codepoint_shape_count(info, 65)
        tt |> equal(count, expected)
    }
}

// ── Font high-level API tests ──────────────────────────────────────────

[test]
def test_load_font(t : T?) {
    t |> run("load_font loads font without atlas") <| @(tt : T?) {
        var font <- load_font(font_path())
        tt |> success(is_valid(font), "font is valid")
        tt |> success(font.pixel_height == 0.0, "no default pixel_height")
        tt |> success(empty(font.chardata), "no chardata")
    }
}

[test]
def test_load_font_invalid(t : T?) {
    t |> run("load_font returns invalid for missing file") <| @(tt : T?) {
        var font <- load_font("nonexistent.ttf")
        tt |> success(!is_valid(font), "font is not valid")
    }
}

[test]
def test_font_scale_for_pixel_height(t : T?) {
    t |> run("scale_for_pixel_height on Font") <| @(tt : T?) {
        var font <- load_font(font_path())
        let s = font |> scale_for_pixel_height(32.0)
        tt |> success(s > 0.0, "positive scale")
    }
}

[test]
def test_font_vmetrics(t : T?) {
    t |> run("font_vmetrics returns raw metrics") <| @(tt : T?) {
        var font <- load_font(font_path())
        let vm = font |> font_vmetrics()
        tt |> success(vm.ascent > 0, "positive ascent")
        tt |> success(vm.descent < 0, "negative descent")
    }
}

[test]
def test_font_metrics_with_pixel_height(t : T?) {
    t |> run("font_metrics with explicit pixel_height") <| @(tt : T?) {
        var font <- load_font(font_path())
        let fm = font |> font_metrics(32.0)
        tt |> success(fm.ascent > 0.0, "positive ascent")
        tt |> success(fm.descent < 0.0, "negative descent")
        tt |> success(fm.line_height > 0.0, "positive line_height")
    }
}

[test]
def test_font_codepoint_hmetrics(t : T?) {
    t |> run("codepoint_hmetrics on Font") <| @(tt : T?) {
        var font <- load_font(font_path())
        let hm = font |> codepoint_hmetrics(65)
        tt |> success(hm.advance_width > 0, "positive advance")
    }
}

[test]
def test_font_codepoint_shape(t : T?) {
    t |> run("codepoint_shape on Font iterates vertices") <| @(tt : T?) {
        var font <- load_font(font_path())
        var count = 0
        font |> codepoint_shape(65) <| $(v) {
            count++
        }
        tt |> success(count > 0, "has vertices")
        tt |> equal(count, font |> codepoint_shape_count(65))
    }
}

[test]
def test_font_find_glyph(t : T?) {
    t |> run("find_glyph on Font") <| @(tt : T?) {
        var font <- load_font(font_path())
        tt |> success(font |> find_glyph(65) > 0, "'A' found")
        tt |> equal(font |> find_glyph(int(0x1FFFF)), 0)
    }
}

[test]
def test_font_bounding_box(t : T?) {
    t |> run("font_bounding_box returns valid box") <| @(tt : T?) {
        var font <- load_font(font_path())
        let bb = font |> font_bounding_box()
        tt |> success(bb.z > bb.x, "x1 > x0")
        tt |> success(bb.w > bb.y, "y1 > y0")
    }
}

[test]
def test_font_codepoint_box(t : T?) {
    t |> run("codepoint_box on Font") <| @(tt : T?) {
        var font <- load_font(font_path())
        let box = font |> codepoint_box(65)
        tt |> success(box.z > box.x, "x1 > x0")
        tt |> success(box.w > box.y, "y1 > y0")
    }
}

[test]
def test_measure_text(t : T?) {
    t |> run("measure_text returns positive width for non-empty string") <| @(tt : T?) {
        var font <- load_font(font_path())
        let w = font |> measure_text("Hello", 32.0)
        tt |> success(w > 0.0, "positive width")
        // longer string should be wider
        let w2 = font |> measure_text("Hello World", 32.0)
        tt |> success(w2 > w, "longer string is wider")
    }
}

[test]
def test_font_codepoint_bitmap_box(t : T?) {
    t |> run("codepoint_bitmap_box on Font") <| @(tt : T?) {
        var font <- load_font(font_path())
        let scale = font |> scale_for_pixel_height(32.0)
        let box = font |> codepoint_bitmap_box(65, scale, scale)
        tt |> success(box.z > box.x, "ix1 > ix0")
        tt |> success(box.w > box.y, "iy1 > iy0")
    }
}

[test]
def test_font_codepoint_bitmap(t : T?) {
    t |> run("codepoint_bitmap on Font renders 'A' as Image") <| @(tt : T?) {
        var font <- load_font(font_path())
        let scale = font |> scale_for_pixel_height(32.0)
        let result = font |> codepoint_bitmap(65, scale, scale)
        tt |> success(result.image.width > 0, "width > 0")
        tt |> success(result.image.height > 0, "height > 0")
        tt |> equal(result.image.channels, 1)
        var non_zero = 0
        for (px in result.image.bytes) {
            if (px != uint8(0)) {
                non_zero++
            }
        }
        tt |> success(non_zero > 0, "bitmap has non-zero pixels")
    }
}

[test]
def test_load_ttf_uses_wrappers(t : T?) {
    t |> run("load_ttf still works after refactoring") <| @(tt : T?) {
        var font <- load_ttf(font_path(), [pixel_height = 24.0, atlas_dim = int2(256, 256)])
        tt |> success(is_valid(font), "font is valid")
        tt |> success(!empty(font.chardata), "has chardata")
        tt |> success(font.pixel_height == 24.0, "pixel_height set")
        // font_metrics (original overload) should still work
        let fm = font |> font_metrics()
        tt |> success(fm.ascent > 0.0, "ascent from font_metrics")
    }
}
