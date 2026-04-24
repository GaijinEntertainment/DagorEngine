// Tutorial STBIMAGE-04: Pixel Access and Format Conversion
//
// This tutorial covers:
//   - Reading pixels with with_pixels (uint8, float, uint16)
//   - Row-level access with with_row
//   - Modifying pixels in-place
//   - Channel conversion with to_channels (RGB↔RGBA, greyscale)
//   - BPC conversion with to_bpc (uint8↔float↔uint16)
//   - Combining channel and BPC conversions
//
// Run: daslang.exe tutorials/dasStbImage/04_pixel_access_and_conversion.das

options gen2
options indenting = 4

require stbimage/stbimage_boost

// ──────────────────────────────────────────────────────────────────────────
// Helper — create a test image
// ──────────────────────────────────────────────────────────────────────────

def make_test_image() : Image {
    var img = make_image(8, 8, 4)
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        for (i in range(8 * 8)) {
            let x = i % 8
            let y = i / 8
            pixels[i * 4 + 0] = uint8(x * 32)
            pixels[i * 4 + 1] = uint8(y * 32)
            pixels[i * 4 + 2] = uint8(128)
            pixels[i * 4 + 3] = uint8(255)
        }
    }
    return <- img
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Reading pixels with with_pixels
// ──────────────────────────────────────────────────────────────────────────
//
// with_pixels() gives you a temporary array view of the raw pixel data.
// Three overloads exist based on the block parameter type:
//   - array<uint8>  — for bpc=1 images (default)
//   - array<float>  — for bpc=4 images (HDR)
//   - array<uint16> — for bpc=2 images (16-bit)
//
// You can also use with_pixels_f() and with_pixels_16() explicitly.

def example_read_pixels() {
    var inscope img <- make_test_image()
    // Read uint8 pixels
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        let idx = (3 * 8 + 5) * 4  // pixel at (5, 3)
        print("  Pixel (5,3): R={int(pixels[idx])} G={int(pixels[idx+1])} B={int(pixels[idx+2])} A={int(pixels[idx+3])}\n")
    }
    // Read as float — requires converting to bpc=4 first
    var inscope hdr <- img.to_bpc(4)
    hdr |> with_pixels_f() <| $(var pixels : array<float>#) {
        let idx = (3 * 8 + 5) * 4
        print("  Pixel (5,3) as float: R={pixels[idx]} G={pixels[idx+1]} B={pixels[idx+2]} A={pixels[idx+3]}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Row-level access with with_row
// ──────────────────────────────────────────────────────────────────────────
//
// with_row(y, block) gives access to a single row of pixels.
// More efficient when you only need one row at a time.

def example_row_access() {
    var inscope img <- make_test_image()
    // Access row 4
    img |> with_row(4) <| $(var row : array<uint8>#) {
        let bytes_per_row = length(row)
        print("  Row 4: {bytes_per_row} bytes ({bytes_per_row / 4} pixels)\n")
        // First pixel of row 4
        print("  Row 4, pixel 0: R={int(row[0])} G={int(row[1])} B={int(row[2])} A={int(row[3])}\n")
    }
    // Access row as floats — requires bpc=4
    var inscope hdr <- img.to_bpc(4)
    hdr |> with_row_f(4) <| $(var row : array<float>#) {
        print("  Row 4, pixel 0 as float: R={row[0]} G={row[1]} B={row[2]} A={row[3]}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Modifying pixels in-place
// ──────────────────────────────────────────────────────────────────────────
//
// The pixel arrays from with_pixels are mutable (var parameter).
// Changes are written directly back to the image data.

def example_modify_pixels() {
    var inscope img <- make_test_image()
    // Invert all pixels
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        for (i in range(8 * 8)) {
            pixels[i * 4 + 0] = uint8(255 - int(pixels[i * 4 + 0]))  // R
            pixels[i * 4 + 1] = uint8(255 - int(pixels[i * 4 + 1]))  // G
            pixels[i * 4 + 2] = uint8(255 - int(pixels[i * 4 + 2]))  // B
            // Alpha stays the same
        }
    }
    // Verify: pixel (0,0) was R=0 G=0 B=128, now R=255 G=255 B=127
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        print("  After invert, pixel (0,0): R={int(pixels[0])} G={int(pixels[1])} B={int(pixels[2])}\n")
    }
    // Modify a single row — draw a red line on row 4
    img |> with_row(4) <| $(var row : array<uint8>#) {
        for (x in range(8)) {
            row[x * 4 + 0] = uint8(255)
            row[x * 4 + 1] = uint8(0)
            row[x * 4 + 2] = uint8(0)
            row[x * 4 + 3] = uint8(255)
        }
    }
    img |> with_row(4) <| $(var row : array<uint8>#) {
        print("  Row 4 after red line: R={int(row[0])} G={int(row[1])} B={int(row[2])}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Channel conversion with to_channels
// ──────────────────────────────────────────────────────────────────────────
//
// Image.to_channels(n) returns a new image with the specified number of
// channels. Supported conversions:
//   4 → 3 (RGBA → RGB, drop alpha)
//   3 → 4 (RGB → RGBA, add alpha=255)
//   4 → 1 or 3 → 1 (to greyscale, using ITU-R BT.601 luminance)
//   1 → 3 or 1 → 4 (grey → RGB/RGBA)
//   4 → 2 or 1 → 2 (add alpha channel)
//   2 → 1 or 2 → 4 (grey+alpha conversions)

def example_to_channels() {
    var inscope rgba <- make_test_image()
    print("  Source: {rgba.channels} channels\n")
    // RGBA → RGB (drop alpha)
    var inscope rgb <- rgba.to_channels(3)
    print("  to_channels(3): {rgb.channels} ch, size={rgb.data_size()}\n")
    // RGB → greyscale
    var inscope grey <- rgb.to_channels(1)
    print("  to_channels(1): {grey.channels} ch, size={grey.data_size()}\n")
    // Greyscale → RGBA
    var inscope back <- grey.to_channels(4)
    print("  to_channels(4): {back.channels} ch, size={back.data_size()}\n")
    // Grey+alpha
    var inscope grey_a <- grey.to_channels(2)
    print("  to_channels(2): {grey_a.channels} ch, size={grey_a.data_size()}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — BPC conversion with to_bpc
// ──────────────────────────────────────────────────────────────────────────
//
// Image.to_bpc(n) returns a new image with the specified bytes per
// component:
//   1 — uint8  (0-255)
//   2 — uint16 (0-65535)
//   4 — float  (0.0-1.0)
//
// Conversions preserve the visual value:
//   uint8(255) → uint16(65535) → float(1.0)

def example_to_bpc() {
    var inscope img <- make_test_image()
    print("  Source: bpc={img.bpc}, data_size={img.data_size()}\n")
    // uint8 → float
    var inscope hdr <- img.to_bpc(4)
    print("  to_bpc(4): bpc={hdr.bpc}, data_size={hdr.data_size()}\n")
    // Verify: pixel (0,0) B=128 → float ~0.502
    hdr |> with_pixels() <| $(var pixels : array<float>#) {
        print("  Float pixel (0,0): R={pixels[0]} G={pixels[1]} B={pixels[2]} A={pixels[3]}\n")
    }
    // float → uint16
    var inscope img16 <- hdr.to_bpc(2)
    print("  to_bpc(2): bpc={img16.bpc}, data_size={img16.data_size()}\n")
    // uint16 → uint8 (round-trip)
    var inscope back <- img16.to_bpc(1)
    print("  to_bpc(1): bpc={back.bpc}, data_size={back.data_size()}\n")
    // Verify round-trip preserves values
    var match = true
    img |> with_pixels() <| $(var orig : array<uint8>#) {
        back |> with_pixels() <| $(var rt : array<uint8>#) {
            for (i in range(length(orig))) {
                if (orig[i] != rt[i]) {
                    match = false
                    break
                }
            }
        }
    }
    print("  Round-trip uint8->float->uint16->uint8 exact: {match}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Combined conversions
// ──────────────────────────────────────────────────────────────────────────
//
// Channel and BPC conversions can be chained. Order matters when you
// want to minimize precision loss — convert BPC first (to higher
// precision), then convert channels.

def example_combined() {
    var inscope img <- make_test_image()
    print("  Source: {img.channels} ch, bpc={img.bpc}\n")
    // Convert RGBA uint8 → RGB float (HDR pipeline input)
    var inscope hdr_rgb <- img.to_bpc(4).to_channels(3)
    print("  HDR RGB: {hdr_rgb.channels} ch, bpc={hdr_rgb.bpc}, data_size={hdr_rgb.data_size()}\n")
    // Convert back: RGB float → RGBA uint8
    var inscope rgba_u8 <- hdr_rgb.to_channels(4).to_bpc(1)
    print("  Back to RGBA uint8: {rgba_u8.channels} ch, bpc={rgba_u8.bpc}\n")
    // Convert to greyscale uint16 (scientific imaging)
    var inscope grey16 <- img.to_channels(1).to_bpc(2)
    print("  Grey uint16: {grey16.channels} ch, bpc={grey16.bpc}, data_size={grey16.data_size()}\n")
}

[export]
def main() {
    print("=== Section 1: Reading pixels ===\n")
    example_read_pixels()

    print("\n=== Section 2: Row-level access ===\n")
    example_row_access()

    print("\n=== Section 3: Modifying pixels ===\n")
    example_modify_pixels()

    print("\n=== Section 4: Channel conversion ===\n")
    example_to_channels()

    print("\n=== Section 5: BPC conversion ===\n")
    example_to_bpc()

    print("\n=== Section 6: Combined conversions ===\n")
    example_combined()
}
