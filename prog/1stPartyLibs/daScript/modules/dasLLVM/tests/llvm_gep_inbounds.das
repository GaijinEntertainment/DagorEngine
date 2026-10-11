options gen2

require dastest/testing_boost public
require daslib/fio
require strings
require daslib/strings_boost
require daslib/module_path

// Machine-enforces REVIEW.md's GEP rule: no dasLLVM source outside bindings/ may call
// LLVMSetIsInBounds - a constant-folded GEP is a ConstantExpr, and the setter's cast writes
// the flag through the wrong type into it (ARCHITECTURE.md#gep-constant-fold). Build the GEP
// in-bounds with LLVMBuildInBoundsGEP2 instead. Weakening this test is a defect.

def private scan_dir(dir : string; var offenders : array<string>) {
    fio::dir(dir) $(name : string) {
        if (name == "." || name == "..") {
            return
        }
        let path = "{dir}/{name}"
        let st = stat(path)
        if (st.is_dir) {
            if (name != "bindings") {
                scan_dir(path, offenders)
            }
            return
        }
        if (!(name |> ends_with(".das")) || name == "llvm_gep_inbounds.das") {   // the scanner names its own quarry
            return
        }
        let text = fread(path)
        var ln = 1
        for (line in split(text, "\n")) {
            let cmt = line |> find("//")
            let code = cmt >= 0 ? slice(line, 0, cmt) : line   // a mention in a comment is not a call
            if (code |> find("LLVMSetIsInBounds(") >= 0) {
                offenders |> push("{path}:{ln}")
            }
            ln++
        }
    }
}

[test]
def test_no_llvm_set_is_in_bounds(t : T?) {
    var offenders : array<string>
    scan_dir("{get_this_module_dir()}/..", offenders)
    for (o in offenders) {
        t |> failure("LLVMSetIsInBounds called at {o} - build the GEP in-bounds with LLVMBuildInBoundsGEP2 (REVIEW.md; ARCHITECTURE.md#gep-constant-fold)")
    }
    t |> success(empty(offenders), "no LLVMSetIsInBounds call sites outside bindings/")
}
