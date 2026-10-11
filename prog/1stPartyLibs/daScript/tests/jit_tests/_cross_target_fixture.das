options gen2

// child for cross_target_folds.das: prints the compile-time target folds and which arm a static_if
// over the target triple selects (the same test the tune framework's inert gate makes), so the
// parent can compare a native compile against --jit-target=<triple>

// the module-level `let` gate shape the engine's tier files use (dasllama_math_default's wasm dot
// pick), folded through a different path than the static_if below
let TARGET_IS_WASM = get_target_architecture_name() == "wasm64" || get_target_architecture_name() == "wasm32"

[export]
def main {
    print("TRIPLE=[{get_target_triple()}]\n")
    print("TARGET_ARCH=[{get_target_architecture_name()}]\n")
    print("LET_WASM=[{TARGET_IS_WASM}]\n")
    print("HOST_ARCH=[{get_architecture_name()}]\n")
    print("CROSS_PLATFORM=[{get_cross_platform_name()}]\n")
    static_if (!empty(get_target_triple())) {
        print("CROSS=[yes]\n")
    } else {
        print("CROSS=[no]\n")
    }
}
