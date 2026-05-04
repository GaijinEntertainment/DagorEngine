options gen2
options indenting = 4

module audio_analysis shared

require math

let FFT_SIZE = 512
let FFT_BINS = FFT_SIZE / 2

struct AudioAnalysis {
    sample_ring     : array<float>
    ring_write      : int = 0
    sample_ring_l   : array<float>
    sample_ring_r   : array<float>
    ring_write_lr   : int = 0
    spectrum_bins   : array<float>
    spectrum_smooth : array<float>
}

def audio_analysis_init(var aa : AudioAnalysis) {
    aa.sample_ring |> resize(FFT_SIZE)
    aa.sample_ring_l |> resize(FFT_SIZE)
    aa.sample_ring_r |> resize(FFT_SIZE)
    aa.spectrum_bins |> resize(FFT_BINS)
    aa.spectrum_smooth |> resize(FFT_BINS)
}

def audio_analysis_feed(var aa : AudioAnalysis; samples : array<float>) {
    let n = length(samples) / 2
    for (i in range(n)) {
        let l = samples[i * 2]
        let r = samples[i * 2 + 1]
        aa.sample_ring[aa.ring_write] = (l + r) * 0.5
        aa.ring_write = (aa.ring_write + 1) % FFT_SIZE
        aa.sample_ring_l[aa.ring_write_lr] = l
        aa.sample_ring_r[aa.ring_write_lr] = r
        aa.ring_write_lr = (aa.ring_write_lr + 1) % FFT_SIZE
    }
}

def audio_analysis_update_spectrum(var aa : AudioAnalysis) {
    var re : array<float>
    var im : array<float>
    re |> resize(FFT_SIZE)
    im |> resize(FFT_SIZE)
    for (i in range(FFT_SIZE)) {
        let idx = (aa.ring_write + i) % FFT_SIZE
        let window = 0.5 - 0.5 * cos(2.0 * PI * float(i) / float(FFT_SIZE))
        re[i] = aa.sample_ring[idx] * window
    }
    fft_in_place(re, im)
    let scale = 2.0 / float(FFT_SIZE)
    for (i in range(FFT_BINS)) {
        let mag = sqrt(re[i] * re[i] + im[i] * im[i]) * scale
        let db = max(20.0 * log2(max(mag, 1.0e-6)) / log2(10.0), -80.0)
        aa.spectrum_bins[i] = (db + 80.0) / 80.0
    }
    let smooth = 0.3
    for (i in range(FFT_BINS)) {
        aa.spectrum_smooth[i] = aa.spectrum_smooth[i] * (1.0 - smooth) + aa.spectrum_bins[i] * smooth
    }
}

// --- FFT (radix-2 Cooley-Tukey, in-place) ---

def fft_in_place(var re, im : array<float>) {
    let n = length(re)
    var j = 0
    for (i in range(1, n)) {
        var bit = n >> 1
        while ((j & bit) != 0) {
            j ^= bit
            bit >>= 1
        }
        j ^= bit
        if (i < j) {
            let tmp_r = re[i]; re[i] = re[j]; re[j] = tmp_r
            let tmp_i = im[i]; im[i] = im[j]; im[j] = tmp_i
        }
    }
    var len = 2
    while (len <= n) {
        let half = len / 2
        let angle = -PI * 2.0 / float(len)
        let w_r = cos(angle)
        let w_i = sin(angle)
        var i = 0
        while (i < n) {
            var cur_r = 1.0
            var cur_i = 0.0
            for (k in range(half)) {
                let t_r = cur_r * re[i + half + k] - cur_i * im[i + half + k]
                let t_i = cur_r * im[i + half + k] + cur_i * re[i + half + k]
                re[i + half + k] = re[i + k] - t_r
                im[i + half + k] = im[i + k] - t_i
                re[i + k] += t_r
                im[i + k] += t_i
                let next_r = cur_r * w_r - cur_i * w_i
                cur_i = cur_r * w_i + cur_i * w_r
                cur_r = next_r
            }
            i += len
        }
        len <<= 1
    }
}
