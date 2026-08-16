options gen2

require dastest/testing_boost public

// cpu_supports — runtime CPU-feature truth for kernel/backend gating, by LLVM target-feature
// name. Fail-closed by design: an unknown name, or a name belonging to another architecture,
// reports false. x86 uses cpuid plus the OSXSAVE/XGETBV dance so "avx*" answers stay OS-honest;
// arm64 uses sysctl (macOS), AT_HWCAP (Linux) or IsProcessorFeaturePresent (Windows).
// Assertions here are box-independent (no "this CI runner has AVX2" claims — an x86_64 binary
// under emulation may legitimately lack it); only baselines guaranteed by the ISA are asserted.

[test]
def test_cpu_supports(t : T?) {
    t |> run("unknown names fail closed") @(t : T?) {
        t |> success(!cpu_supports("bogus-feature-name"), "unknown feature reports false")
        t |> success(!cpu_supports(""), "empty feature reports false")
    }
    t |> run("architecture consistency") @(t : T?) {
        let arch = get_architecture_name()
        if (arch == "x86_64" || arch == "x86") {
            // implication ladder on real silicon (shared OS-state requirement makes these safe)
            if (cpu_supports("avx2")) {
                t |> success(cpu_supports("avx"), "avx2 implies avx")
            }
            if (cpu_supports("avx512vnni")) {
                t |> success(cpu_supports("avx512f"), "avx512vnni implies avx512f")
            }
            if (cpu_supports("avx512bw")) {
                t |> success(cpu_supports("avx512f"), "avx512bw implies avx512f")
            }
            if (cpu_supports("avx512vl")) {
                t |> success(cpu_supports("avx512f"), "avx512vl implies avx512f")
            }
        } elif (arch == "arm64") {
            // neon and fp-armv8 are mandatory in every arm64 profile, so these are ISA facts
            t |> success(cpu_supports("neon"), "neon is baseline on arm64")
            t |> success(cpu_supports("fp-armv8"), "fp-armv8 is baseline on arm64")
            // optional extensions: only the implications are box-independent
            if (cpu_supports("i8mm")) {
                t |> success(cpu_supports("neon"), "i8mm implies neon")
            }
            if (cpu_supports("bf16")) {
                t |> success(cpu_supports("neon"), "bf16 implies neon")
            }
            t |> success(!cpu_supports("avx2"), "x86 feature names report false off-x86")
            t |> success(!cpu_supports("sse4.2"), "x86 feature names report false off-x86")
        } else {
            t |> success(!cpu_supports("avx2"), "x86 feature names report false off-x86")
            t |> success(!cpu_supports("sse4.2"), "x86 feature names report false off-x86")
            t |> success(!cpu_supports("dotprod"), "arm feature names report false off-arm")
        }
    }
    t |> run("arm names off-arm and x86 names off-x86 both fail closed") @(t : T?) {
        let arch = get_architecture_name()
        if (arch == "x86_64" || arch == "x86") {
            t |> success(!cpu_supports("dotprod"), "arm feature names report false on x86")
            t |> success(!cpu_supports("i8mm"), "arm feature names report false on x86")
        }
    }
}
