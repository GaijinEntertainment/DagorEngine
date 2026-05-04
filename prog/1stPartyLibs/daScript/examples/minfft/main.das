options gen2
require minfft
require math

[export]
def main {
    var signal <- [for (i in range(64)); sin(float(i) * 0.2)]
    var complex_frequencies : array<float2>
    fft_real_forward(signal, complex_frequencies)
    var magnitudes : array<float>
    fft_calculate_normalized_magnitudes(complex_frequencies, magnitudes)
    print("FFT test:\n")
    for (i in range(length(magnitudes))) {
        if (magnitudes[i] > 0.01) {
            print("  bin {i}: {magnitudes[i]}\n")
        }
    }
}
