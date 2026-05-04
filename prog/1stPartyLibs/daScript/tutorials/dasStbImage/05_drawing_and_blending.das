// Tutorial STBIMAGE-05: Drawing and Alpha Blending
//
// This tutorial covers:
//   - Filling rectangles with solid colors (RGBA and greyscale)
//   - Alpha blending a 1-channel source onto an RGBA destination
//   - Combining fill and blit to create composited images
//
// Run: daslang.exe tutorials/dasStbImage/05_drawing_and_blending.das

options gen2
options indenting = 4

require stbimage/stbimage_boost
require daslib/fio

// ──────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────

def pack_rgba(r, g, b, a : int) : uint {
    return uint(r) | (uint(g) << 8u) | (uint(b) << 16u) | (uint(a) << 24u)
}

def print_pixel(img : Image; x, y : int; lbl : string) {
    let idx = (y * img.width + x) * 4
    print("  {lbl} ({x},{y}): R={int(img.bytes[idx])} G={int(img.bytes[idx+1])} B={int(img.bytes[idx+2])} A={int(img.bytes[idx+3])}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — fill_rect on RGBA images
// ──────────────────────────────────────────────────────────────────────────
//
// Image.fill_rect(x, y, w, h, color) fills a rectangle with a packed RGBA
// color value. The image must be 4-channel uint8. Coordinates are clipped
// to image bounds automatically.

def example_fill_rect_rgba() {
    var img = make_image(32, 32, 4)
    // Fill background with dark blue
    let dark_blue = pack_rgba(20, 20, 60, 255)
    img.fill_rect(0, 0, 32, 32, dark_blue)
    // Draw a red square in the center
    let red = pack_rgba(255, 60, 60, 255)
    img.fill_rect(8, 8, 16, 16, red)
    // Draw a green stripe across the top
    let green = pack_rgba(60, 200, 60, 255)
    img.fill_rect(0, 0, 32, 4, green)
    print_pixel(img, 16, 16, "Center (red)")
    print_pixel(img, 0, 0, "Top-left (green)")
    print_pixel(img, 0, 30, "Bottom-left (blue)")
    // Clipping: fill extending past right edge
    let yellow = pack_rgba(255, 255, 0, 255)
    img.fill_rect(28, 14, 10, 4, yellow)
    print_pixel(img, 31, 15, "Right edge (yellow, clipped)")
    let path = "_tutorial_fill_rect.png"
    img.save(path)
    print("  Saved to {path}\n")
    remove(path)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — fill_rect on greyscale images
// ──────────────────────────────────────────────────────────────────────────
//
// For 1-channel uint8 images, fill_rect takes a uint8 value instead.

def example_fill_rect_grey() {
    var img = make_image(32, 32, 1)
    // Fill background with dark grey
    img.fill_rect(0, 0, 32, 32, uint8(40))
    // Draw a bright square
    img.fill_rect(4, 4, 24, 24, uint8(200))
    print("  Grey pixel (16,16): {int(img.bytes[16 * 32 + 16])}\n")
    print("  Grey pixel (0,0):   {int(img.bytes[0])}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Alpha blending with blit_alpha
// ──────────────────────────────────────────────────────────────────────────
//
// Image.blit_alpha(src, sx, sy, dx, dy, w, h, r, g, b) blends a
// 1-channel alpha source onto a 4-channel RGBA destination. Each source
// pixel's value is used as the alpha for blending the given (r,g,b) color.
//
// This is the building block for software text rendering — the font atlas
// is a 1-channel image where each pixel stores glyph coverage.

def example_blit_alpha() {
    // Create a light grey destination
    var dst = make_image(32, 32, 4)
    dst.fill_rect(0, 0, 32, 32, pack_rgba(180, 180, 180, 255))
    // Create a 1-channel alpha source: a gradient from 0 to 255
    var src = make_image(16, 16, 1)
    for (y in range(16)) {
        for (x in range(16)) {
            src.bytes[y * 16 + x] = uint8(x * 255 / 15)
        }
    }
    // Blit the alpha gradient in red — left side transparent, right side opaque
    dst.blit_alpha(src, 0, 0, 8, 8, 16, 16, 255, 0, 0)
    print_pixel(dst, 8, 12, "Left edge (alpha~0, still grey)")
    print_pixel(dst, 23, 12, "Right edge (alpha~255, red)")
    print_pixel(dst, 16, 12, "Middle (blended)")
    let path = "_tutorial_blit_alpha.png"
    dst.save(path)
    print("  Saved to {path}\n")
    remove(path)
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Compositing: layered drawing
// ──────────────────────────────────────────────────────────────────────────
//
// Combining fill_rect and blit_alpha to build up a composited image.
// This pattern is common in UI rendering and software text rendering.

def example_compositing() {
    var canvas = make_image(64, 48, 4)
    // Background gradient: dark top, lighter bottom
    for (y in range(48)) {
        let v = y * 60 / 47
        canvas.fill_rect(0, y, 64, 1, pack_rgba(v + 20, v + 20, v + 40, 255))
    }
    // Draw a filled rectangle as a "button" background
    canvas.fill_rect(8, 12, 48, 24, pack_rgba(60, 120, 200, 255))
    // Draw a 1px border around the button using thin fill_rects
    let border = pack_rgba(200, 220, 255, 255)
    canvas.fill_rect(8, 12, 48, 1, border)   // top
    canvas.fill_rect(8, 35, 48, 1, border)   // bottom
    canvas.fill_rect(8, 12, 1, 24, border)   // left
    canvas.fill_rect(55, 12, 1, 24, border)  // right
    // Create a small alpha "icon" (a circle-ish shape)
    var icon = make_image(12, 12, 1)
    for (y in range(12)) {
        for (x in range(12)) {
            let dx = float(x) - 5.5
            let dy = float(y) - 5.5
            let dist = dx * dx + dy * dy
            if (dist < 25.0) {
                icon.bytes[y * 12 + x] = uint8(255)
            }
        }
    }
    // Blit the icon onto the button in white
    canvas.blit_alpha(icon, 0, 0, 14, 18, 12, 12, 255, 255, 255)
    let path = "_tutorial_compositing.png"
    canvas.save(path)
    print("  Canvas: {canvas.width}x{canvas.height}\n")
    print("  Saved to {path}\n")
    remove(path)
}

[export]
def main() {
    print("=== Section 1: fill_rect (RGBA) ===\n")
    example_fill_rect_rgba()

    print("\n=== Section 2: fill_rect (greyscale) ===\n")
    example_fill_rect_grey()

    print("\n=== Section 3: Alpha blending ===\n")
    example_blit_alpha()

    print("\n=== Section 4: Compositing ===\n")
    example_compositing()
}
