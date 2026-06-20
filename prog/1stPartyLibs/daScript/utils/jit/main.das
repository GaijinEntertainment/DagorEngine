options gen2

require daslib/ast_boost
require daslib/clargs
require daslib/fio
require daslib/jobque_boost
require daslib/strings_boost
require math

require llvm/daslib/llvm_jit_cli
require llvm/daslib/llvm_jit_run

[CommandLineArgs]
struct JitToolArgs {
    @clarg_positional
    @clarg_doc = "Input .das file(s) to JIT-compile"
    inputs : array<string>

    @clarg_name = "exe"
    @clarg_doc = "Emit a standalone executable instead of a shared library"
    exe : bool

    @clarg_short = "o"
    @clarg_name = "output"
    @clarg_doc = "Output path. Empty keeps the default content-hashed .jitted_scripts/<ns>/ naming (dll mode) or appends .exe (exe mode)."
    output : string

    @clarg_name = "exclude"
    @clarg_doc = "Directory name(s) to skip during a recursive walk (matched on the folder's own name, at any depth). Repeatable."
    exclude : array<string>

    @clarg_short = "j"
    @clarg_name = "parallel"
    @clarg_doc = "Spawn worker subprocesses to compile in parallel. `--parallel 0` (or pass with no value) auto-picks get_total_hw_threads(); positive N uses N workers. Omit the flag entirely for in-process sequential compile. daslang's compile pipeline isn't thread-safe inside one process, so workers are full subprocesses like dastest's isolated mode."
    parallel : Option<int>

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show help"
    help : bool
}

// Test files that are EXPECTED to fail compilation — by daslang convention,
// basenames prefixed cant_ / failed_ / invalid_. Skipped during a directory
// walk so a prewarm over a test tree doesn't report them as errors.
def is_expected_failure(name : string) : bool {
    return (name |> starts_with("cant_") ||
            name |> starts_with("failed_") ||
            name |> starts_with("invalid_"))
}

// Expand a positional input into concrete .das files. An explicitly-named file
// is taken as-is; a directory is walked recursively, skipping "_"-prefixed
// entries (fixtures / harness-only files), expected-failure tests, and any
// directory whose name is in `exclude`.
def collect_das_files(path : string; exclude : array<string>; var out : array<string>) {
    let st = stat(path)
    if (!st.is_valid) {
        return
    }
    if (st.is_reg) {
        if (path |> ends_with(".das")) {
            out |> push(path)
        }
        return
    }
    // Skip non-dirs and any directory whose own name is excluded (covers both
    // the top-level argument and nested folders reached via recursion below).
    if (!st.is_dir || find_index(exclude, base_name(path)) >= 0) {
        return
    }
    fio::dir(path) $(name) {
        if (name == "." || name == ".." || name |> starts_with("_")) {
            return
        }
        let full = path_join(path, name)
        let cst = stat(full)
        if (cst.is_dir) {
            collect_das_files(full, exclude, out)
        } elif (cst.is_reg && name |> ends_with(".das") && !is_expected_failure(name)) {
            out |> push(full)
        }
    }
}

def print_usage() {
    print_help(get_command_info(type<JitToolArgs>), "jit")
    print("\nJIT tuning flags (also accepted):\n")
    print_help(get_command_info(type<JitCliOptions>), "jit")
}

// Files-per-child-invocation upper bound. Each chunk of up to this many files
// is dispatched to one worker subprocess that processes them sequentially. The
// cap exists because OSes limit total argv bytes (Linux MAX_ARG_STRLEN ~128k,
// total ARG_MAX ~2MB; Windows CreateProcess command-line ~32k). 1000 paths fits
// comfortably under all of them and still amortizes fork/exec across many compiles.
let WORKER_CHUNK_SIZE = 1000

// Reconstruct the child invocation for --parallel workers. The dispatcher pushes
// one chunk per task; the child re-execs this exe with the chunk's files as
// positional inputs plus the JIT tuning flags the user originally passed.
// Intentionally omits --parallel (child must stay sequential), --exclude (no
// walk on explicit files), and tool.output (would collide across files).
def build_worker_argv(exe_path : string; chunk : array<string>; tool : JitToolArgs; jit : JitCliOptions) : array<string> {
    var w : array<string>
    w |> reserve(2 + length(chunk) + 8)
    w |> push(exe_path)
    w |> push_from(chunk)
    if (tool.exe) {
        w |> push("--exe")
    }
    if (jit.opt_level |> is_some) {
        w |> push("--jit-opt-level")
        w |> push("{jit.opt_level |> unwrap}")
    }
    if (jit.size_level |> is_some) {
        w |> push("--jit-size-level")
        w |> push("{jit.size_level |> unwrap}")
    }
    if (jit.debug_info |> is_some) {
        w |> push("--jit-debug")
        w |> push(jit.debug_info |> unwrap ? "true" : "false")
    }
    if (jit.path_to_linker |> is_some) {
        w |> push("--jit-path-to-linker")
        w |> push(jit.path_to_linker |> unwrap)
    }
    return <- w
}

// Channel payload types — workhorse scalars (string, int) can't ride
// push_clone / for_each_clone directly because their finalizer (delete temp)
// doesn't apply, so wrap in structs like dastest's IsoInput/IsolatedResult.
// JitWorkItem ships a fully-built argv so the worker doesn't need to capture
// tool / jit (non-copyable structs across thread contexts).
struct JitWorkItem {
    chunk_id : int
    file_count : int
    argv : array<string>
}

struct JitWorkResult {
    chunk_id : int
    file_count : int
    exit_code : int
    captured : string
}

def run_parallel(files : array<string>; tool : JitToolArgs; jit : JitCliOptions) : int {
    let argv0 <- get_command_line_arguments()
    if (empty(argv0)) {
        to_log(LOG_ERROR, "jit: --parallel: cannot determine own exe path (argv empty)\n")
        return 1
    }
    let exe_path = argv0[0]
    // --parallel 0 (or bare flag) → auto-pick hw threads, matching dastest's
    // isolated-mode policy.
    var totalThreads = tool.parallel |> unwrap_or(0)
    if (totalThreads <= 0) {
        totalThreads = get_total_hw_threads()
        if (totalThreads <= 0) {
            totalThreads = 1
        }
    }
    if (totalThreads > length(files)) {
        totalThreads = length(files)
    }
    // Pick chunk size from worker count first: ceil(files / threads) gives
    // each worker roughly equal work. Cap at WORKER_CHUNK_SIZE so a huge file
    // set doesn't blow OS argv limits — extra chunks just queue up.
    var chunk_size = (length(files) + totalThreads - 1) / totalThreads
    if (chunk_size > WORKER_CHUNK_SIZE) {
        chunk_size = WORKER_CHUNK_SIZE
    }
    if (chunk_size < 1) {
        chunk_size = 1
    }
    // Build one JitWorkItem per chunk. Pre-compose argv here (in main thread)
    // so workers do no per-task setup.
    var chunks : array<JitWorkItem>
    var idx = 0
    while (idx < length(files)) {
        var item : JitWorkItem
        item.chunk_id = length(chunks)
        var chunk_files : array<string>
        var stop = idx + chunk_size
        if (stop > length(files)) {
            stop = length(files)
        }
        chunk_files |> reserve(stop - idx)
        for (i in range(idx, stop)) {
            chunk_files |> push(clone_string(files[i]))
        }
        item.file_count = length(chunk_files)
        item.argv <- build_worker_argv(exe_path, chunk_files, tool, jit)
        chunks |> emplace(item)
        idx = stop
    }
    let totalChunks = length(chunks)
    if (totalThreads > totalChunks) {
        totalThreads = totalChunks
    }
    var rc = 0
    to_log(LOG_INFO, "jit: dispatching {length(files)} files in {totalChunks} chunks across {totalThreads} workers\n")
    with_channel(totalChunks) $(inputChannel) {
        with_channel(totalChunks) $(outputChannel) {
            with_job_status(totalThreads) $(completion) {
                for (_t in range(totalThreads)) {
                    new_thread <| @capture(= _t) { // nolint:LINT002 — `_t` placates the new_thread macro (capture frame must be non-empty); body never references it
                        for_each_clone(inputChannel) $(input : JitWorkItem#) {
                            var local_argv : array<string>
                            local_argv |> reserve(length(input.argv))
                            for (a in input.argv) {
                                local_argv |> push(clone_string(a))
                            }
                            var res : JitWorkResult
                            res.chunk_id = input.chunk_id
                            res.file_count = input.file_count
                            res.exit_code = run_and_capture(local_argv, res.captured)
                            outputChannel |> push_clone(res)
                            outputChannel |> notify()
                        }
                        inputChannel |> release()
                        outputChannel |> release()
                        completion |> notify_and_release()
                    }
                }
                for (chunk in chunks) {
                    inputChannel |> push_clone(chunk)
                    inputChannel |> notify()
                }
                for (r in each_clone(outputChannel, type<JitWorkResult>)) {
                    print(r.captured)
                    if (r.exit_code != 0) {
                        to_log(LOG_ERROR, "jit: chunk {r.chunk_id} ({r.file_count} files) exited {r.exit_code}\n")
                        rc = 1
                    }
                }
            }
        }
    }
    return rc
}

def jit_compile_one(input : string; tool : JitToolArgs; jit : JitCliOptions) : bool {
    var success = true
    ast_gc_guard() {
        var inscope access <- make_file_access("")
        access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.jit_enabled = true
                cop.version_2_syntax = true
                // Match dastest's JIT setup: aot_module prepares finalizers /
                // lambdas / generators for AOT-JIT codegen (without it, the JIT
                // can't emit IR for them -> "Failed to get IR for functions ...").
                cop.aot_module = true
                cop.threadlock_context = true
                if (tool.exe) {
                    cop.jit_exe_mode = true
                    cop.jit_dll_mode = false
                } else {
                    cop.jit_dll_mode = true
                }
                if (!empty(tool.output)) {
                    cop.jit_output_path := tool.output
                }
                cop.jit_opt_level = jit.opt_level |> unwrap_or(cop.jit_opt_level)
                cop.jit_size_level = jit.size_level |> unwrap_or(cop.jit_size_level)
                cop.jit_debug_info = jit.debug_info |> unwrap_or(cop.jit_debug_info)
                let ptl = jit.path_to_linker |> unwrap_or("")
                if (!empty(ptl)) {
                    cop.jit_path_to_linker := ptl
                }
                var ns : string
                compile_file(input, access, unsafe(addr(mg)), cop) $(ok, program, errors) {
                    if (!ok || program == null) {
                        to_log(LOG_ERROR, "jit: failed to compile {input}\n{errors}\n")
                        success = false
                        return
                    }
                    ns = string(program.thisNamespace)
                    // The jit_llvm simulate-macro — pulled into the target's module
                    // graph via the just_in_time extra module above — runs during
                    // simulate() and emits the .dll / .exe as a side effect. The
                    // target is compiled jit-aware (unsupported constructs lowered /
                    // marked no-jit correctly), which a post-simulate run_jit call
                    // would miss. `require llvm_jit_run` above bakes the native llvm
                    // module into a standalone -exe build of this tool.
                    simulate(program) $(sok, context, serrors) {
                        if (!sok) {
                            to_log(LOG_ERROR, "jit: failed to simulate {input}\n{serrors}\n")
                            success = false
                        }
                    }
                }
                if (success) {
                    let target = tool.exe ? "executable" : "shared library"
                    if (!empty(tool.output)) {
                        let suffix = tool.exe ? ".exe" : ".dll"
                        to_log(LOG_INFO, "jit: wrote {target} for {input} -> {tool.output}{suffix}\n")
                    } else {
                        to_log(LOG_INFO, "jit: wrote {target} for {input} under .jitted_scripts/{ns}/\n")
                    }
                }
            }
        }
    }
    return success
}

[export]
def main() : int {
    var tool_r <- parse_args(type<JitToolArgs>)
    if (tool_r |> is_err) {
        to_log(LOG_ERROR, "jit: {tool_r |> unwrap_err}\n\n")
        print_usage()
        return 1
    }
    let tool <- tool_r |> move_unwrap
    if (tool.help || empty(tool.inputs)) {
        print_usage()
        return tool.help ? 0 : 1
    }
    var jit_r <- parse_args(type<JitCliOptions>)
    if (jit_r |> is_err) {
        to_log(LOG_ERROR, "jit: {jit_r |> unwrap_err}\n\n")
        print_usage()
        return 1
    }
    let jit <- jit_r |> move_unwrap

    var files : array<string>
    for (input in tool.inputs) {
        collect_das_files(input, tool.exclude, files)
    }
    if (empty(files)) {
        to_log(LOG_ERROR, "jit: no .das files found in inputs\n")
        return 1
    }

    if (tool.parallel |> is_some) {
        if (!empty(tool.output)) {
            to_log(LOG_ERROR, "jit: --parallel is incompatible with --output (multiple files would collide on one path)\n")
            return 1
        }
        return run_parallel(files, tool, jit)
    }

    var rc = 0
    for (f in files) {
        if (!jit_compile_one(f, tool, jit)) {
            rc = 1
        }
    }
    return rc
}
