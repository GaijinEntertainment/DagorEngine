options gen2
module llvm_targets shared
require llvm/bindings/llvm_func

// All target related functions use #define-s in LLVM-C.
// There's no way to extract them to here. All we can do
// is manually call requested functions.
//
// The set below mirrors the universal_targets list in bind_llvm.das (X86,
// AArch64) — the only targets whose per-target Initialize<X>* symbols
// are guaranteed to be exported by every prebuilt LLVM.dll we ship across
// linux x86_64 / linux arm64 / mac arm64 / windows x86_64. Adding more
// targets here will break loading the bindings on platforms whose curated
// LLVM build omits them; if cross-target codegen is ever needed, add the
// missing target to universal_targets AND ensure every prebuilt exports it.

def LLVMInitializeAllTargetInfos() {
    LLVMInitializeAArch64TargetInfo()
    LLVMInitializeX86TargetInfo()
}
def LLVMInitializeAllTargets() {
    LLVMInitializeAArch64Target()
    LLVMInitializeX86Target()
}
def LLVMInitializeAllTargetMCs() {
    LLVMInitializeAArch64TargetMC()
    LLVMInitializeX86TargetMC()
}
def LLVMInitializeAllAsmPrinters() {
    LLVMInitializeAArch64AsmPrinter()
    LLVMInitializeX86AsmPrinter()
}

// WebAssembly target — kept separate from LLVMInitializeAll* so the JIT path
// doesn't pay its static-init cost on every startup. Called lazily by the
// wasm32 cross-compile entry point.
def LLVMInitializeWasmTarget() {
    LLVMInitializeWebAssemblyTargetInfo()
    LLVMInitializeWebAssemblyTarget()
    LLVMInitializeWebAssemblyTargetMC()
    LLVMInitializeWebAssemblyAsmPrinter()
    LLVMInitializeWebAssemblyAsmParser()
}
