options gen2
options indenting = 4

require stbimage
require daslib/safe_addr

// Generate 64x64 test images for file-based load tests
// Run once: daslang.exe tests/stbimage/gen_test_images.das

[export]
def main() {
    let W = 64
    let H = 64

    // 64x64 RGBA gradient: R increases left-to-right, G increases top-to-bottom
    var rgba : array<uint8>
    rgba |> resize(W * H * 4)
    for (y in range(H)) {
        for (x in range(W)) {
            let idx = (y * W + x) * 4
            rgba[idx + 0] = uint8(x * 4)       // R
            rgba[idx + 1] = uint8(y * 4)       // G
            rgba[idx + 2] = uint8(128)          // B
            rgba[idx + 3] = uint8(255)          // A
        }
    }

    let root = "{get_das_root()}/tests/stbimage"

    // PNG (RGBA, lossless)
    var ok = stbi_write_png("{root}/test.png", W, H, 4, unsafe(addr(rgba[0])), W * 4)
    print(ok != 0 ? "PNG: ok\n" : "PNG: FAILED\n")

    // BMP (RGBA)
    ok = stbi_write_bmp("{root}/test.bmp", W, H, 4, unsafe(addr(rgba[0])))
    print(ok != 0 ? "BMP: ok\n" : "BMP: FAILED\n")

    // TGA (RGBA)
    ok = stbi_write_tga("{root}/test.tga", W, H, 4, unsafe(addr(rgba[0])))
    print(ok != 0 ? "TGA: ok\n" : "TGA: FAILED\n")

    // JPG (RGB, lossy - strip alpha)
    var rgb : array<uint8>
    rgb |> resize(W * H * 3)
    for (y in range(H)) {
        for (x in range(W)) {
            let src = (y * W + x) * 4
            let dst = (y * W + x) * 3
            rgb[dst + 0] = rgba[src + 0]
            rgb[dst + 1] = rgba[src + 1]
            rgb[dst + 2] = rgba[src + 2]
        }
    }
    ok = stbi_write_jpg("{root}/test.jpg", W, H, 3, unsafe(addr(rgb[0])), 90)
    print(ok != 0 ? "JPG: ok\n" : "JPG: FAILED\n")

    // HDR (RGBA float)
    var hdr : array<float>
    hdr |> resize(W * H * 4)
    for (y in range(H)) {
        for (x in range(W)) {
            let idx = (y * W + x) * 4
            hdr[idx + 0] = float(x) / float(W)     // R
            hdr[idx + 1] = float(y) / float(H)     // G
            hdr[idx + 2] = 0.5f                     // B
            hdr[idx + 3] = 1.0f                     // A
        }
    }
    ok = stbi_write_hdr("{root}/test.hdr", W, H, 4, unsafe(addr(hdr[0])))
    print(ok != 0 ? "HDR: ok\n" : "HDR: FAILED\n")
}
