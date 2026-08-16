// JPEG-style block-DCT image compression, built on minfft's DCT.
//
// This is the heart of JPEG / MPEG intra coding, minus the entropy (Huffman)
// stage: split the image into 8x8 blocks, run a 2D DCT on each, quantize the
// coefficients with the standard JPEG luminance table (scaled by a quality
// knob), then reverse the process. Lowering quality zeroes more high-frequency
// coefficients -> smaller "file", lower fidelity. We report PSNR and the
// fraction of coefficients quantized to zero as a stand-in for compression.
//
// Run: daslang examples/minfft/dct_jpeg.das

options gen2

require minfft
require math
require daslib/fio
require stbimage/stbimage_boost

let BLOCK = 8

// Standard JPEG luminance quantization table (JPEG spec Annex K, Table K.1),
// in natural row-major 8x8 order.
let JPEG_LUMA = [
    16f, 11f, 10f, 16f,  24f,  40f,  51f,  61f,
    12f, 12f, 14f, 19f,  26f,  58f,  60f,  55f,
    14f, 13f, 16f, 24f,  40f,  57f,  69f,  56f,
    14f, 17f, 22f, 29f,  51f,  87f,  80f,  62f,
    18f, 22f, 37f, 56f,  68f, 109f, 103f,  77f,
    24f, 35f, 55f, 64f,  81f, 104f, 113f,  92f,
    49f, 64f, 78f, 87f, 103f, 121f, 120f, 101f,
    72f, 92f, 95f, 98f, 112f, 100f, 103f,  99f ]

// Per-axis scale that turns minfft's unnormalized DCT-II into the orthonormal
// DCT that the JPEG quant table is designed for. s[0] = 1/(4*sqrt2), else 1/4.
// (minfft round-trips as idct(dct(x)) == 2N*x per axis; here 2N = 16 for N=8.)
def ortho_scale() : float[BLOCK] {
    var s : float[BLOCK]
    s[0] = 1.0f / (4.0f * sqrt(2.0f))
    for (k in range(1, BLOCK)) {
        s[k] = 0.25f
    }
    return s
}

// JPEG quality (1..100) -> multiplier applied to the quant table.
def quality_to_scale(quality : int) : float {
    let q = clamp(quality, 1, 100)
    return q < 50 ? 5000.0f / float(q) : 200.0f - 2.0f * float(q)
}

// Compress one channel (values 0..255) through the block-DCT pipeline.
// Writes the reconstruction into `recon`; returns the fraction of AC+DC
// coefficients that were quantized to zero.
def compress_channel(gray : array<float>; w, h : int; quality : int; var recon : array<float>) : float {
    let s = ortho_scale()
    let qscale = quality_to_scale(quality)

    // Pre-scale the quant table once (clamped to a sane minimum step).
    var qt : float[64]   // BLOCK * BLOCK
    for (i in range(BLOCK * BLOCK)) {
        qt[i] = clamp(floor((JPEG_LUMA[i] * qscale + 50.0f) / 100.0f), 1.0f, 255.0f)   // baseline JPEG: 8-bit steps
    }

    // Start from the source image; the full 8x8 blocks below overwrite it, so any
    // remainder past a multiple of 8 keeps the original pixels instead of going black.
    recon := gray

    var plan = make_dct_plan_2d(BLOCK, BLOCK)
    if (plan == null) {
        print("compress_channel: failed to build DCT plan\n")
        return 0.0f   // recon already holds the original image
    }
    var coeff : array<float>
    var deq : array<float>
    var blk : array<float>
    blk |> resize(BLOCK * BLOCK)

    var zeros = 0
    var total = 0
    let inv256 = 1.0f / float((2 * BLOCK) * (2 * BLOCK))

    // Whole 8x8 blocks only: any w/h remainder past a multiple of 8 is left
    // unprocessed. main()'s image is 256x256 (a multiple of 8); if you adapt this
    // for arbitrary sizes, crop or pad to a multiple of 8 first (real JPEG pads).
    for (by in range(h / BLOCK)) {
        for (bx in range(w / BLOCK)) {
            // gather block, JPEG level shift (-128)
            for (yy in range(BLOCK)) {
                for (xx in range(BLOCK)) {
                    blk[yy * BLOCK + xx] = gray[(by * BLOCK + yy) * w + (bx * BLOCK + xx)] - 128.0f
                }
            }
            dct(blk, coeff, plan)
            // to orthonormal, quantize, dequantize, back to minfft scale
            for (i in range(BLOCK * BLOCK)) {
                let u = i / BLOCK
                let v = i % BLOCK
                let sc = s[u] * s[v]
                let level = round(coeff[i] * sc / qt[i])
                total++
                if (level == 0.0f) {
                    zeros++
                }
                coeff[i] = (level * qt[i]) / sc
            }
            idct(coeff, deq, plan)
            for (yy in range(BLOCK)) {
                for (xx in range(BLOCK)) {
                    let px = clamp(deq[yy * BLOCK + xx] * inv256 + 128.0f, 0.0f, 255.0f)
                    recon[(by * BLOCK + yy) * w + (bx * BLOCK + xx)] = px
                }
            }
        }
    }
    unsafe { delete plan }
    return float(zeros) / float(max(total, 1))
}

def psnr(a, b : array<float>) : float {
    var se = 0.0lf
    for (i in range(length(a))) {
        let d = double(a[i] - b[i])
        se += d * d
    }
    let mse = se / double(max(length(a), 1))
    if (mse <= 0.0lf) {
        return 999.0f
    }
    return float(20.0lf * log(255.0lf / sqrt(mse)) / log(10.0lf))
}

// Synthetic 256x256 grayscale test image: concentric rings (mid frequency),
// a smooth gradient, and a high-frequency checkerboard corner.
def make_test_image(w, h : int) : array<float> {
    var g : array<float>
    g |> resize(w * h)
    let cx = float(w) * 0.5f
    let cy = float(h) * 0.5f
    for (y in range(h)) {
        for (x in range(w)) {
            let fx = float(x)
            let fy = float(y)
            let d = sqrt((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy))
            var val = 128.0f + 90.0f * sin(d * 0.18f) + 0.15f * fx
            if (x > w * 5 / 8 && y > h * 5 / 8) {
                val = ((x / 4 + y / 4) % 2 == 0) ? 235.0f : 25.0f   // high-freq checker
            }
            g[y * w + x] = clamp(val, 0.0f, 255.0f)
        }
    }
    return <- g
}

def save_gray(gray : array<float>; w, h : int; path : string) {
    var img = make_image(w, h, 1)
    img |> with_pixels() $(var pixels : array<uint8>#) {
        for (i in range(w * h)) {
            pixels[i] = uint8(clamp(gray[i], 0.0f, 255.0f))
        }
    }
    let (ok, err) = img.save(path)
    if (ok) {
        print("    saved {path}\n")
    } else {
        print("    save failed: {err}\n")
    }
}

[export]
def main {
    let w = 256
    let h = 256
    let original <- make_test_image(w, h)

    var terr : string
    let tdir = temp_directory(terr)
    let outdir = empty(tdir) ? "." : tdir

    print("JPEG-style block-DCT compression ({w}x{h}, 8x8 blocks)\n")
    save_gray(original, w, h, path_join(outdir, "minfft_dct_original.png"))

    print("\n  quality   PSNR(dB)   coeffs zeroed\n")
    print("  -------   --------   -------------\n")
    for (quality in [90, 50, 20, 5]) {
        var recon : array<float>
        let zfrac = compress_channel(original, w, h, quality, recon)
        let p = psnr(original, recon)
        print("  {quality}\t  {p}\t   {int(zfrac * 100.0f)}%\n")
        save_gray(recon, w, h, path_join(outdir, "minfft_dct_q{quality}.png"))
    }
    print("\nLower quality -> more coefficients zeroed -> lower PSNR. That tradeoff,\n")
    print("plus an entropy coder over the zeroed coefficients, is baseline JPEG.\n")
}
