options gen2
options no_aot
options disable_auto_inline = true

require daslib/ast
require daslib/fio

require dastest/testing_boost
require UnitTest

require _trap_block_ann_helper

// A block_macro can fill ExprBlock.annotationData -- a host ECS does that for every query block,
// storing the baked query description there. The LLVM backend used to give up on such a block when
// generating a binary (llvm_exe.das preVisitExprMakeBlock -> add_unimplemented) on the assumption
// that annotationData is never nonnull for a regular das program. Building dagor's enlisted against
// the LLVM AOT backend emitted thousands of
//   [W] LLVM JIT: Emit trap for _anon_..`on_human_requested_reload..._makeblock_ann glob at ...
// and then killed the whole binary with "Some of the nodes above are unimplemented!".
//
// The value is a compile-process payload, so it genuinely cannot be an object-file constant. It is
// now resolved at block-make time from the context's sid->data table (jit_ad_by_sid), which is the
// same route the C++ AOT emitter takes (aot_cpp.das emits __context__->adBySid).

let OUTPUT_DIR = "{get_das_root()}/build/tests"
let EXE_SCRIPT = "{OUTPUT_DIR}/trap_block_ann_exe.das"
let EXE_PATH = "{OUTPUT_DIR}/trap_block_ann_exe"
let EXE_BINARY = "{OUTPUT_DIR}/trap_block_ann_exe.exe"

def call_block(blk : block<(x : int) : int>) : int {
    return invoke(blk, 20)
}

// The payload must reach the runtime block, not merely "the block runs" -- that is the point of
// annotationData and the only thing a host ECS reads. The writer here is a das-side
// AstBlockAnnotation; tests/language/block_annotation_data.das covers the C++ writer across all
// three execution tiers.
[test]
def test_annotated_block(t : T?) {
    let r = call_block() $ [data_block] (x : int) : int {
        return x + 2
    }
    t |> equal(r, 22)
    let ad = probe_annotation_data() $ [data_block] (x : int) : int {
        return x + 2
    }
    t |> equal(ad, DATA_BLOCK_PAYLOAD)
}

def probe_annotation_data(blk : block<(x : int) : int>) : uint64 {
    return testBlockAnnotationData(blk)
}

def write_exe_script() : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(EXE_SCRIPT, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            fprint(f, "options disable_auto_inline = true\n")
            fprint(f, "require UnitTest\n")
            fprint(f, "require ../../tests/jit_tests/_trap_block_ann_helper.das\n")
            fprint(f, "def call_block(blk : block<(x : int) : int>) : int \{ return invoke(blk, 20); }\n")
            fprint(f, "def probe(blk : block<(x : int) : int>) : uint64 \{ return testBlockAnnotationData(blk); }\n")
            fprint(f, "[export] def main() \{\n")
            fprint(f, "    let r = call_block() <| $ [data_block] (x : int) : int \{ return x + 2; }\n")
            fprint(f, "    let ad = probe() <| $ [data_block] (x : int) : int \{ return x + 2; }\n")
            fprint(f, "    print(\"r=\{r} ad=\{ad}\\n\")\n")
            fprint(f, "}\n")
            ok = true
        }
    }
    return ok
}

// Returns false when the link step failed -- typically on the sanitizer matrix, same caveat as
// jit_exe.das compile_to_exe. Before the fix it returned false for a different reason: the trap set
// any_unimplemented, so strict codegen bailed and no binary was ever written.
def compile_to_exe(t : T?) : bool {
    if (fexist(EXE_BINARY)) {
        remove_result(EXE_BINARY)
    }
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.jit_enabled = true
            cop.jit_exe_mode = true
            cop.jit_dll_mode = false
            access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
            cop.jit_output_path := EXE_PATH
            compile_file(EXE_SCRIPT, access, unsafe(addr(mg)), cop) $(ok, program, _errors) {
                t |> equal(ok, true)
                simulate(program) $(sok, _context, _serrors) {
                    t |> equal(sok, true)
                }
            }
        }
    }
    return fexist(EXE_BINARY)
}

[test]
def test_annotated_block_exe(t : T?) {
    // Same gate as jit_exe.das test_executable: Windows has no RPATH for the spawned exe, and
    // without JIT or a shared-lib build there is no library for it to call into.
    if (get_platform_name() == "windows" || !jit_enabled() || !das_is_dll_build()) return
    t |> equal(write_exe_script(), true)
    if (!compile_to_exe(t)) return
    var output = ""
    unsafe {
        popen(EXE_BINARY) $(f) {
            while (!feof(f)) {
                output += fgets(f)
            }
        }
    }
    // ad=0x0 is the correct answer for a standalone exe, not a shortfall of the resolve: the payload
    // is made by the compiling process, and an exe context is built by jit_create_standalone_ctx
    // rather than simulate(), so it has no sid->data table for jit_ad_by_sid to read. Before the fix
    // this exe did not exist at all. A host that needs the payload must be a tier that simulates
    // (interpreter / JIT / AOT), which the sibling test covers.
    t |> equal(output, "r=22 ad=0x0\n")
}
