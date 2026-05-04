// Tutorial STBIMAGE-01: Loading and Inspecting Images
//
// This tutorial covers:
//   - Loading images from files (PNG, JPG, BMP, TGA, HDR)
//   - Error handling with tuple destructuring
//   - Querying image properties (dimensions, channels, bpc)
//   - Getting image info without loading pixels
//   - Format detection (HDR, 16-bit)
//   - Convenience load_image() function
//
// Run: daslang.exe tutorials/dasStbImage/01_loading_images.das

options gen2
options indenting = 4

require daslib/fio
require stbimage/stbimage_boost

// ──────────────────────────────────────────────────────────────────────────
// Helper — create a test image in memory so this tutorial is self-contained
// ──────────────────────────────────────────────────────────────────────────

def make_test_gradient() : Image {
    var img = make_image(64, 64, 4)
    img |> with_pixels() <| $(var pixels : array<uint8>#) {
        for (i in range(64 * 64)) {
            let x = i % 64
            let y = i / 64
            pixels[i * 4 + 0] = uint8(x * 4)
            pixels[i * 4 + 1] = uint8(y * 4)
            pixels[i * 4 + 2] = uint8(128)
            pixels[i * 4 + 3] = uint8(255)
        }
    }
    return <- img
}

// ──────────────────────────────────────────────────────────────────────────
// Section 1 — Loading from file with error handling
// ──────────────────────────────────────────────────────────────────────────
//
// Image.load() returns a tuple<bool; string>. On success the first element
// is true and the error string is empty. On failure it's false with a
// description of what went wrong.

def example_load_with_error_handling(path : string) {
    var inscope img : Image
    let (ok, error) = img.load(path)
    if (!ok) {
        print("  Failed to load: {error}\n")
        return
    }
    print("  Loaded {img.width}x{img.height}, {img.channels} channels, bpc={img.bpc}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 2 — Image properties
// ──────────────────────────────────────────────────────────────────────────
//
// Once loaded, an Image exposes several query methods:
//   valid()      — true if width>0 and height>0
//   stride()     — bytes per row (width * channels * bpc)
//   has_alpha()  — true if channels is 2 or 4
//   pixel_size() — bytes per pixel (channels * bpc)
//   is_hdr()     — true if bpc == 4 (float data)
//   data_size()  — total bytes of pixel data

def example_properties() {
    var inscope img <- make_test_gradient()
    print("  valid:      {img.valid()}\n")
    print("  width:      {img.width}\n")
    print("  height:     {img.height}\n")
    print("  channels:   {img.channels}\n")
    print("  bpc:        {img.bpc}\n")
    print("  stride:     {img.stride()}\n")
    print("  has_alpha:  {img.has_alpha()}\n")
    print("  pixel_size: {img.pixel_size()}\n")
    print("  is_hdr:     {img.is_hdr()}\n")
    print("  data_size:  {img.data_size()}\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 3 — Loading HDR and 16-bit images
// ──────────────────────────────────────────────────────────────────────────
//
// load_hdr() loads float data (bpc=4). load_16() loads 16-bit data (bpc=2).
// Both follow the same tuple<bool; string> error pattern.

def example_load_variants(png_path : string) {
    // Load as 16-bit (PNG supports this)
    var inscope img16 : Image
    let (ok16, err16) = img16.load_16(png_path)
    if (ok16) {
        print("  16-bit: {img16.width}x{img16.height}, bpc={img16.bpc}\n")
    } else {
        print("  16-bit load failed: {err16}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 4 — Image info without loading pixels
// ──────────────────────────────────────────────────────────────────────────
//
// image_info() reads just the header — much faster than loading the full
// image. Returns a tuple with width, height, and channel count.

def example_image_info(path : string) {
    let info = image_info(path)
    print("  Info: {info.w}x{info.h}, {info.comp} channels\n")
}

// ──────────────────────────────────────────────────────────────────────────
// Section 5 — Convenience loading
// ──────────────────────────────────────────────────────────────────────────
//
// load_image() is a one-liner that panics on failure — good for scripts
// where you don't want to handle errors. There's also a variant with
// an error out-parameter.

def example_convenience_load(path : string) {
    // Panics on error
    var inscope img <- load_image(path)
    print("  load_image: {img.width}x{img.height}\n")

    // With error string
    var error : string
    var inscope img2 <- load_image(path, error)
    if (img2.valid()) {
        print("  load_image (error variant): {img2.width}x{img2.height}\n")
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Section 6 — Format detection
// ──────────────────────────────────────────────────────────────────────────
//
// is_hdr() and is_16_bit() check file format without loading.
// Memory variants (is_hdr_from_memory, is_16_bit_from_memory) also exist.

def example_format_detection(path : string) {
    print("  is_hdr:    {is_hdr(path)}\n")
    print("  is_16_bit: {is_16_bit(path)}\n")
}

[export]
def main() {
    // Save a test image so we have a file to load
    var inscope gradient <- make_test_gradient()
    let test_path = "_tutorial_test.png"
    gradient.save(test_path)

    print("=== Section 1: Loading with error handling ===\n")
    example_load_with_error_handling(test_path)
    example_load_with_error_handling("nonexistent.png")

    print("\n=== Section 2: Image properties ===\n")
    example_properties()

    print("\n=== Section 3: Loading HDR and 16-bit ===\n")
    example_load_variants(test_path)

    print("\n=== Section 4: Image info ===\n")
    example_image_info(test_path)

    print("\n=== Section 5: Convenience loading ===\n")
    example_convenience_load(test_path)

    print("\n=== Section 6: Format detection ===\n")
    example_format_detection(test_path)

    // Cleanup
    remove(test_path)
}
