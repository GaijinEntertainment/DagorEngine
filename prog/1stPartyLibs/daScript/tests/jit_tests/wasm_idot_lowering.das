// The signed idot family on the wasm64 cross target lowers to the ISA's exact int8 dot halves -
// the i16x8 extmul products summed by i32x4.extadd_pairwise_i16x8_s and folded even/odd - and never
// to the generic widen-multiply chain that runs a fifth as fast there; the relaxed 7-bit dot is
// refused by design (it wraps at -128, and the feature string carries no +relaxed-simd - WebKit
// runs none of it), so the dump carries no relaxed intrinsic either. A host compile of the same
// fixture emits no wasm intrinsic.
// Compile-only keeps both compiles on this host and never touches the artifact cache, so --jit-dump
// prints every time.
options gen2
options no_aot

require dastest/testing_boost
require daslib/fio
require daslib/strings_boost
require strings
require math

let FIXTURE = "{get_das_root()}/tests/jit_tests/_wasm_idot_fixture.das"
let PAIRWISE_ADD = "llvm.wasm.extadd.pairwise.signed"
let RELAXED_DOT = "llvm.wasm.relaxed.dot"

def private spawn_child(cmd : string; var lines : array<string>) : int {
    var rc : int
    var full = "{cmd} 2>&1"
    if (get_platform_name() == "windows") {
        full = "\"{full}\""   // cmd.exe strips the first and last quote of a line that opens with one
    }
    rc = unsafe(popen_timeout(full, 300.0) $(f) {
        if (f == null) {
            return
        }
        while (!feof(f)) {
            let ln = strip(fgets(f))
            if (!empty(ln)) {
                lines |> push("{ln}")
            }
        }
    })
    return rc
}

def private count_with(lines : array<string>; needle : string) : int {
    var n = 0
    for (ln in lines) {
        if (find(ln, needle) >= 0) {
            n++
        }
    }
    return n
}

[test]
def test_wasm_idot_lowering(t : T?) {
    if (!jit_enabled()) {
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]
    var terr : string
    let dump = path_join(temp_directory(terr), "wasm_idot_{ref_time_ticks()}")
    t |> run("the wasm64 cross compile lowers idot4 to the exact pairwise int8 dot") @(t : T?) {
        var out : array<string>
        spawn_child("\"{bin}\" -jit \"{FIXTURE}\" -- --jit-target=wasm64-unknown-emscripten --jit-compile-only --jit-dump --jit-dump-to \"{dump}.wasm\"", out)
        let ln <- split(fread("{dump}.wasm"), "\n")
        let pairs = count_with(ln, PAIRWISE_ADD)
        t |> success(pairs >= 3, "the declaration and the two half calls of {PAIRWISE_ADD} are in the dump ({pairs} lines, {length(ln)} lines total)")
        t |> success(count_with(ln, RELAXED_DOT) == 0, "the relaxed 7-bit dot is never emitted")
        t |> success(count_with(ln, "idot_prod") == 0, "no generic widen-multiply chain remains")
        // the unsigned-by-signed form takes the same path, widening its first operand with ZExt:
        // drop that arm and these two halves become the 16-lane chain the line above bans
        let zexts = count_with(ln, "zext <8 x i8>")
        t |> success(zexts >= 2, "both halves of the unsigned operand widen with ZExt ({zexts} lines)")
    }
    t |> run("a host compile emits no wasm intrinsic") @(t : T?) {
        var out : array<string>
        spawn_child("\"{bin}\" -jit \"{FIXTURE}\" -- --jit-compile-only --jit-dump --jit-dump-to \"{dump}.host\"", out)
        let ln <- split(fread("{dump}.host"), "\n")
        t |> success(length(ln) > 20, "the host dump printed ({length(ln)} lines)")
        t |> success(count_with(ln, "llvm.wasm.") == 0, "no wasm intrinsic on the host")
    }
    t |> run("the wasm object's target features carry simd128 and no relaxed-simd") @(t : T?) {
        let dir_r = create_temp_directory_result("wasm_idot_features")
        if (!(dir_r is value)) {
            t |> failure("temp dir failed")
            return
        }
        let dir = unsafe(dir_r.value)
        let obj = "{dir}/idot.o"
        var ln : array<string>
        spawn_child("\"{bin}\" -exe -output \"{obj}\" \"{FIXTURE}\" -- --jit-target=wasm64-unknown-emscripten --jit-emit-object", ln)
        let features = object_target_features(obj)
        t |> success(find(features, "simd128") >= 0, "the object's target_features section names simd128: {features}")
        t |> success(find(features, "relaxed-simd") < 0, "no relaxed opcode can be in a module WebKit must run: {features}")
        rmdir_rec_result(dir)
    }
    remove("{dump}.wasm", terr)
    remove("{dump}.host", terr)
}

// the object's `target_features` custom section as one text line - each feature name is ASCII
// inside a binary file, so the bytes are read into an array and every non-printable byte becomes
// a space before the search
def private object_target_features(path : string) : string {
    var bytes : array<uint8>
    fopen(path, "rb") $(f) {
        if (f == null) {
            return
        }
        bytes |> resize(int64(stat(path).size))
        fread(f, bytes)
    }
    let text = build_string() $(w) {
        for (b in bytes) {
            let c = int(b)
            w |> write(c >= 0x20 && c < 0x7f ? to_char(c) : " ")
        }
    }
    let at = find(text, "target_features")
    return at < 0 ? "" : slice(text, at, min(at + 256, length(text)))
}
