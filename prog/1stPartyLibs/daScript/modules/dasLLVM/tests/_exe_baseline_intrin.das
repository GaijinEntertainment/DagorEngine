options gen2

// child for exe_host_cpu.das: a kernel-free program reaching every feature-gated intrinsic
// family through a plain call, built with -exe onto the generic baseline machine — the build
// must not emit an instruction that machine cannot select, and the artifact prints the
// fallback bodies' exact answers
require daslib/aarch64_neon
require daslib/x64_avx

def lane_sum(v : int4) : int => v.x + v.y + v.z + v.w

[export]
def main {
    var w : array<int8>
    w |> resize(16)
    for (i in range(16)) {
        w[i] = int8(i + 1)
    }
    let p = unsafe(addr(w[0]))
    let sdot = lane_sum(aarch64_neon::sdot4(int4(0), p, p))
    let mmla = lane_sum(aarch64_neon::smmla(int4(0), p, p))
    let dot = lane_sum(x64_avx::dot4(int4(0), p, p))
    print("INTRIN=[{sdot},{mmla},{dot}]\n")
}
