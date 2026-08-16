options gen2

module worker_policy shared

require math

//! Conservative default for CPU/memory-heavy isolated JIT workers.
//! Small hosted runners retain at least four-way latency hiding; large boxes
//! use half their logical threads but never exceed 32 without an explicit
//! per-box or command-line override.
def public default_jit_isolated_threads(hw_threads : int) : int {
    let hw = max(1, hw_threads)
    return clamp(hw / 2, 4, 32)
}
