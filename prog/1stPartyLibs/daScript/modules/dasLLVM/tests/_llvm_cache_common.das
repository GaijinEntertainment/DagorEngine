options gen2
module _llvm_cache_common

// Shared harness for the jit cache round-trip tests (llvm_obj_cache, llvm_ast_roundtrip):
// fixture staging into a per-run temp dir, DLL eviction, and a child runner that greps
// stdout for marker lines.

require dastest/testing_boost
require daslib/strings_boost
require daslib/linq
require daslib/fio
require daslib/ast_boost
require daslib/debugger public
require daslib/module_path
require daslib/option public
require daslib/rtti public
require llvm/daslib/llvm_jit_cli public
require llvm/daslib/llvm_jit_run public
require strings

// re-derive a fixture from its pristine source, optionally with one edit applied
def public copy_fixture(src_dir, dst_dir, fname : string; from_s : string = ""; to_s : string = "") : bool {
    var content = ""
    fopen("{src_dir}/{fname}", "rb") $(f) {
        if (f != null) {
            content = fread(f)
        }
    }
    if (empty(content)) {
        return false
    }
    if (!empty(from_s)) {
        // a stale edit string must fail HERE, not two asserts later as a missing marker
        return false if (find(content, from_s) < 0)
        content = content |> replace(from_s, to_s)
    }
    var wrote = false
    fopen("{dst_dir}/{fname}", "wb") $(f) {
        if (f != null) {
            f |> fwrite(content)
            wrote = true
        }
    }
    return wrote
}

// the DLL name is content-addressed (unknown up front) - walk the child's cache tree for it
def public delete_dlls_under(root : string) : int {
    var deleted = 0
    dir(root) $(sub) {
        if (sub == "." || sub == "..") return
        dir("{root}/{sub}") $(fname) {
            if (fname |> ends_with(".dll")) {
                var err : string
                if (remove("{root}/{sub}/{fname}", err)) {
                    deleted++
                }
            }
        }
    }
    return deleted
}

// run cmd, report exit code plus which marker substrings appeared on stdout
def public run_child(cmd : string; markers : array<string>) : tuple<rc : int; saw : array<bool>> {
    var saw : array<bool>
    saw |> resize(length(markers))
    let rc = unsafe(popen_timeout(cmd, 300.0) $(f) {
        if (f == null) {
            return
        }
        while (!feof(f)) {
            let ln = fgets(f)
            for (m, i in markers, count()) {
                saw[i] ||= find(ln, m) >= 0
            }
        }
    })
    return <- (rc = rc, saw <- saw)
}

// one child run: rc must be 0 and every marker must appear on stdout
def public expect_round(t : T?; round, cmd : string; markers : array<string>) {
    let no_alternatives : array<string>
    expect_round(t, round, cmd, markers, no_alternatives)
}

// alternatives form: every `markers` entry must appear AND at least one of `any_of` must -
// for asserts like "the verdict is partial OR clean" that must survive a strict improvement
def public expect_round(t : T?; round, cmd : string; markers, any_of : array<string>) {
    let all <- concat(markers, any_of)
    let r <- run_child(cmd, all)
    t |> equal(r.rc, 0, "{round}: child rc")
    for (i in range(length(markers))) {
        t |> success(r.saw[i], "{round}: {markers[i]}")
    }
    if (!empty(any_of)) {
        var any = false
        for (i in range(length(any_of))) {
            any ||= r.saw[length(markers) + i]
        }
        t |> success(any, "{round}: one of {join(any_of, " | ")}")
    }
}

struct public SplitRound {
    ok : bool
    partitions : int
    cached : int
    output_path : string
    acc : int
    lo : int
    hi : int
    hit : int
    miss : int
}

def public compile_for_jit(script : string; exe_mode : bool; blk : block<(var program : smart_ptr<Program>) : void>) {
    ast_gc_guard() {
        var inscope access <- make_file_access("")
        access |> add_extra_module("llvm_jit_finalizer",
            "{get_das_root()}/modules/dasLLVM/daslib/llvm_jit_finalizer.das")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.jit_enabled = true
                cop.version_2_syntax = true
                cop.aot_module = true
                cop.threadlock_context = true
                cop.jit_exe_mode = exe_mode
                cop.jit_dll_mode = !exe_mode
                compile_file(script, access, unsafe(addr(mg)), cop) $(ok, program, _errors) {
                    if (ok && program != null) {
                        invoke(blk, program)
                    }
                }
            }
        }
    }
}

def public split_round(var program : smart_ptr<Program>) : SplitRound {
    var res : SplitRound
    simulate(program) $(sok, context, _serrors) {
        if (!sok) {
            return
        }
        var cfg : JitCliOptions
        cfg.split_modules = some(4)
        let r = run_jit(get_ptr(program), get_ptr(context), cfg)
        res = SplitRound(ok = r.ok, partitions = r.partitions, cached = r.cached, output_path = r.output_path)
        unsafe {
            invoke_in_context(context, "split_values", addr(res.acc), addr(res.lo), addr(res.hi),
                addr(res.hit), addr(res.miss))
        }
    }
    return res
}

def public split_round(client : string) : SplitRound {
    var res : SplitRound
    compile_for_jit(client, false) $(var program) {
        res = split_round(program)
    }
    return res
}

def public stage_split_client(t : T?; name : string) : string {
    var terr : string
    let tmp = create_temp_directory(name, terr)
    if (empty(tmp)) {
        t |> failure("create_temp_directory: {terr}")
        return ""
    }
    for (f in ["_llvm_obj_cache_client.das", "_llvm_split_modules_mod_a.das", "_llvm_split_modules_mod_b.das"]) {
        t |> success(copy_fixture(get_this_module_dir(), tmp, f), "stage {f}")
    }
    return "{tmp}/_llvm_obj_cache_client.das"
}
