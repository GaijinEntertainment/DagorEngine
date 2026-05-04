// Tutorial STBIMAGE-03: Image Transforms
//
// This tutorial covers:
//   - Resizing images (default and specific filters)
//   - Flipping vertically and horizontally
//   - Cropping (sub-region extraction)
//   - Blitting (compositing one image onto another)
//   - Creating blank canvases with make_image
//
// Run: daslang.exe tutorials/dasStbImage/03_transforms.das

options gen2
options indenting = 4

require math
require daslib/fio
require stbimage/stbimage_boost

// ──────────────────────────────────────────────────────────────────────────
// Helpers — create test images so this tutorial is self-contained
// ──────────────────────────────────────────────────────────────────────────

def make_gradient(w, h : int) : Image {
    var img = make_image(w, h, 4)
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        for (i in range(w * h)) {
            let x = i % w
            let y = i / w
            pixels[i * 4 + 0] = uint8(x * 255 / max(w - 1, 1))
            pixels[i * 4 + 1] = uint8(y * 255 / max(h - 1, 1))
            pixels[i * 4 + 2] = uint8(128)
            pixels[i * 4 + 3] = uint8(255)
        }
    }
    return <- img
}

def make_solid(w, h : int; r, g, b, a : uint8) : Image {
    var img = make_image(w, h, 4)
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        for (i in range(w * h)) {
            pixels[i * 4 + 0] = r
            pixels[i * 4 + 1] = g
            pixels[i * 4 + 2] = b
            pixels[i * 4 + 3] = a
        }
    }
    return <- img
}

def print_img(lbl : string; img : Image) {
    print("  {lbl}: {img.width}x{img.height}, {img.channels} ch, bpc={img.bpc}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Resizing
// ──────────────────────────────────────────────────────────────────────────
//
// Image.resize(new_w, new_h) returns a new resized Image using the
// default filter (Catmull-Rom). You can also specify a filter explicitly.

def example_resize() {
    var inscope img <- make_gradient(64, 64)
    // Default filter
    var inscope small <- img.resize(16, 16)
    print_img("Original", img)
    print_img("Resized 16x16", small)
    // Resize with explicit filter
    var inscope large <- img.resize(128, 128, stbir_filter.STBIR_FILTER_POINT_SAMPLE)
    print_img("Point-sample 128x128", large)
    var inscope smooth <- img.resize(128, 128, stbir_filter.STBIR_FILTER_MITCHELL)
    print_img("Mitchell 128x128", smooth)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Flipping
// ──────────────────────────────────────────────────────────────────────────
//
// flip_vertical() and flip_horizontal() return new flipped Images.
// The original is not modified.

def example_flip() {
    var inscope img <- make_gradient(32, 32)
    var inscope flipped_v <- img.flip_vertical()
    var inscope flipped_h <- img.flip_horizontal()
    print_img("Original", img)
    print_img("Flipped vertical", flipped_v)
    print_img("Flipped horizontal", flipped_h)
    // Verify: top-left pixel of original should be bottom-left of flipped_v
    var match_v = false
    img |> with_pixels() <| $(var orig : array<uint8>#) {
        flipped_v |> with_pixels() <| $(var flip : array<uint8>#) {
            let last_row = (32 - 1) * 32 * 4
            match_v = orig[0] == flip[last_row] && orig[1] == flip[last_row + 1]
        }
    }
    print("  Vertical flip correct: {match_v}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Cropping
// ──────────────────────────────────────────────────────────────────────────
//
// Image.crop(x, y, w, h) extracts a rectangular sub-region.
// Coordinates are clamped to image bounds.

def example_crop() {
    var inscope img <- make_gradient(64, 64)
    // Crop a 16x16 region from the center
    var inscope cropped <- img.crop(24, 24, 16, 16)
    print_img("Original", img)
    print_img("Cropped 16x16 from (24,24)", cropped)
    // Crop with edge clamping — requesting beyond bounds is safe
    var inscope clamped <- img.crop(50, 50, 32, 32)
    print_img("Clamped crop from (50,50) 32x32", clamped)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Blitting
// ──────────────────────────────────────────────────────────────────────────
//
// Image.blit(source, x, y) copies source pixels onto the image at (x, y).
// Both images must have the same number of channels.
// Pixels outside the destination bounds are clipped.

def example_blit() {
    // Create a dark canvas
    var inscope canvas <- make_solid(64, 64, uint8(32), uint8(32), uint8(32), uint8(255))
    // Create a bright overlay
    var inscope overlay <- make_solid(16, 16, uint8(255), uint8(200), uint8(0), uint8(255))
    // Blit the overlay at different positions
    canvas.blit(overlay, 8, 8)
    canvas.blit(overlay, 40, 40)
    print_img("Canvas after blit", canvas)
    // Save to verify visually (optional)
    let path = "_tutorial_blit.png"
    canvas.save(path)
    print("  Saved blitted result to {path}\n")
    remove(path)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Creating blank canvases
// ──────────────────────────────────────────────────────────────────────────
//
// make_image(width, height, channels, bpc) creates a zero-filled Image.
// bpc defaults to 1 (uint8). Use bpc=4 for HDR float data.

def example_make_image() {
    // Standard 8-bit RGBA
    var rgba = make_image(100, 100, 4)
    print_img("RGBA uint8", rgba)
    // HDR float image
    var hdr = make_image(100, 100, 3, 4)
    print_img("RGB float", hdr)
    // Single-channel grayscale
    var grey = make_image(100, 100, 1)
    print_img("Greyscale", grey)
    // 16-bit image
    var img16 = make_image(100, 100, 4, 2)
    print_img("RGBA uint16", img16)
}

[export]
def main() {
    print("=== Section 1: Resizing ===\n")
    example_resize()

    print("\n=== Section 2: Flipping ===\n")
    example_flip()

    print("\n=== Section 3: Cropping ===\n")
    example_crop()

    print("\n=== Section 4: Blitting ===\n")
    example_blit()

    print("\n=== Section 5: Creating blank canvases ===\n")
    example_make_image()
}
