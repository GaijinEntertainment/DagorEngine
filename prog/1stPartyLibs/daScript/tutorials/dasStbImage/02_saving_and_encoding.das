// Tutorial STBIMAGE-02: Saving and Encoding Images
//
// This tutorial covers:
//   - Saving images to file (PNG, BMP, TGA, JPG, HDR)
//   - JPEG quality parameter
//   - Encoding images to memory (in-memory byte arrays)
//   - Loading images from memory
//   - Round-trip: encode → decode
//
// Run: daslang.exe tutorials/dasStbImage/02_saving_and_encoding.das

options gen2
options indenting = 4

require daslib/fio
require stbimage/stbimage_boost

// ──────────────────────────────────────────────────────────────────────────
// Helper — create a test image so this tutorial is self-contained
// ──────────────────────────────────────────────────────────────────────────

def make_test_image() : Image {
    var img = make_image(32, 32, 3)
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        for (i in range(32 * 32)) {
            let x = i % 32
            let y = i / 32
            pixels[i * 3 + 0] = uint8(x * 8)
            pixels[i * 3 + 1] = uint8(y * 8)
            pixels[i * 3 + 2] = uint8(128)
        }
    }
    return <- img
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Saving to file
// ──────────────────────────────────────────────────────────────────────────
//
// Image.save(path, quality) saves to file. The format is determined by
// the file extension: .png, .bmp, .tga, .jpg, .hdr.
// The quality parameter only matters for JPEG (1-100, default 90).
// Returns tuple<bool; string> for error handling.

def example_save_formats() {
    var inscope img <- make_test_image()
    let formats = fixed_array("_tutorial_out.png", "_tutorial_out.bmp",
                              "_tutorial_out.tga", "_tutorial_out.jpg")
    for (path in formats) {
        let (ok, error) = img.save(path)
        if (ok) {
            print("  Saved {path}\n")
        } else {
            print("  Failed to save {path}: {error}\n")
        }
    }
    // Cleanup
    for (path in formats) {
        remove(path)
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — JPEG quality
// ──────────────────────────────────────────────────────────────────────────
//
// The second parameter to save() is quality (1-100). Lower quality means
// smaller files but more compression artifacts. Default is 90.

def example_jpeg_quality() {
    var inscope img <- make_test_image()
    let qualities = fixed_array(10, 50, 90)
    for (q in qualities) {
        let path = "_tutorial_q{q}.jpg"
        let (ok, _) = img.save(path, q)
        if (ok) {
            // Encode to memory to show size difference
            var buf : array<uint8>
            let (ok2, _) = img.encode("jpg", buf, q)
            if (ok2) {
                print("  JPEG quality {q}: {length(buf)} bytes\n")
            }
        }
        remove(path)
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Encoding to memory
// ──────────────────────────────────────────────────────────────────────────
//
// Image.encode(format_ext, buf, quality) writes compressed image data
// into a byte array instead of a file. The format_ext uses the same
// extension strings as save(): "png", "bmp", "tga", "jpg".

def example_encode_to_memory() {
    var inscope img <- make_test_image()
    let formats = fixed_array("png", "bmp", "tga", "jpg")
    for (fmt in formats) {
        var buf : array<uint8>
        let (ok, error) = img.encode(fmt, buf)
        if (ok) {
            print("  Encoded {fmt}: {length(buf)} bytes\n")
        } else {
            print("  Encode {fmt} failed: {error}\n")
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Loading from memory
// ──────────────────────────────────────────────────────────────────────────
//
// Image.load_from_memory(buf) decodes an image from a byte array.
// Also available: load_hdr_from_memory(), load_16_from_memory().

def example_load_from_memory() {
    // Encode a test image to PNG bytes
    var inscope img <- make_test_image()
    var png_buf : array<uint8>
    let (ok_enc, _) = img.encode("png", png_buf)
    assert(ok_enc)

    // Load it back from memory
    var inscope img2 : Image
    let (ok, error) = img2.load_from_memory(png_buf)
    if (ok) {
        print("  Loaded from memory: {img2.width}x{img2.height}, {img2.channels} ch\n")
    } else {
        print("  Load from memory failed: {error}\n")
    }

    // Format detection from memory
    print("  is_hdr_from_memory:    {is_hdr_from_memory(png_buf)}\n")
    print("  is_16_bit_from_memory: {is_16_bit_from_memory(png_buf)}\n")

}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Round-trip: encode → decode
// ──────────────────────────────────────────────────────────────────────────
//
// A common pattern: create an image, encode it, then decode it back.
// Lossless formats (PNG, BMP, TGA) preserve pixel values exactly.

def example_round_trip() {
    var inscope original <- make_test_image()
    // Encode to PNG (lossless)
    var buf : array<uint8>
    let (ok_enc, _) = original.encode("png", buf)
    assert(ok_enc)
    // Decode
    var inscope decoded : Image
    let (ok_dec, _) = decoded.load_from_memory(buf)
    assert(ok_dec)
    // Verify dimensions match
    print("  Original:  {original.width}x{original.height}, {original.channels} ch\n")
    print("  Decoded:   {decoded.width}x{decoded.height}, {decoded.channels} ch\n")
    // Verify pixels match (lossless round-trip)
    var pixels_match = true
    original |> with_pixels() <| $(var orig_px : array<uint8>#) {
        decoded |> with_pixels() <| $(var dec_px : array<uint8>#) {
            for (i in range(length(orig_px))) {
                if (orig_px[i] != dec_px[i]) {
                    pixels_match = false
                    break
                }
            }
        }
    }
    print("  Pixels match: {pixels_match}\n")

}

[export]
def main() {
    print("=== Section 1: Saving to file ===\n")
    example_save_formats()

    print("\n=== Section 2: JPEG quality ===\n")
    example_jpeg_quality()

    print("\n=== Section 3: Encoding to memory ===\n")
    example_encode_to_memory()

    print("\n=== Section 4: Loading from memory ===\n")
    example_load_from_memory()

    print("\n=== Section 5: Round-trip ===\n")
    example_round_trip()
}
