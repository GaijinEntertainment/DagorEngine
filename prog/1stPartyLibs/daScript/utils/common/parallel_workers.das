// Generic subprocess fan-out for utils/* tools that need to parallelize
// CPU-bound work (compile_file, lint, etc.) across cores. Each tool
// builds per-chunk argv vectors (its own flags + a paths-from tempfile),
// then calls `run_chunk_workers(argvs)`; this module owns the worker
// pool, popen plumbing, and result collection.
//
// Mechanism: N threads, each pulling chunks from a typed channel and
// driving one `popen_argv` at a time. captures stdout + exit code per
// chunk. Returns results in input order.
//
// Lifted out of utils/detect-dupe so utils/lint (and future tools) can
// share the pattern.

options gen2

require daslib/fio public
require daslib/jobque_boost public
require math
require strings


// Below this many work items, the daslang startup cost dominates over
// parallel speedup. Tuned for detect-dupe's per-file compile cost;
// re-validate if a tool with cheaper-per-item work adopts this module.
let private PARALLEL_THRESHOLD = 16


struct public ChunkResult {
    index : int
    exit_code : int
    stdout : string
}


// Decide whether parallel fan-out is worthwhile. Callers should fall
// back to a serial in-process loop when this returns false (or when
// workers_setting == 1 explicitly).
def public should_parallelize(n_items : int; workers_setting : int) : bool {
    return false if (workers_setting == 1)
    return n_items >= PARALLEL_THRESHOLD
}


// Map `workers_setting` to an actual worker count. 0 => hardware
// threads. Clamps to [1, n_items].
def public compute_worker_count(n_items : int; workers_setting : int) : int {
    var w = workers_setting
    if (w == 0) {
        w = get_total_hw_threads()
        if (w < 1) {
            w = 1
        }
    }
    if (w > n_items) {
        w = n_items
    }
    if (w < 1) {
        w = 1
    }
    return w
}


// Split a flat list into N contiguous chunks (last chunk gets the
// remainder). Contiguous (not round-robin) so chunk index correlates
// with input position and shard merges stay deterministic. `n_chunks`
// is clamped to `[1, length(items)]`. Empty trailing chunks are NOT
// emitted: when `length(items) < n_chunks` the result has exactly
// `length(items)` chunks.
def public chunk_files(items : array<string>; n_chunks : int) : array<array<string>> {
    var out : array<array<string>>
    let total = length(items)
    return <- out if (total == 0)
    var n = n_chunks
    if (n < 1) {
        n = 1
    }
    if (n > total) {
        n = total
    }
    let per_chunk = (total + n - 1) / n
    for (k in range(n)) {
        let start_idx = k * per_chunk
        break if (start_idx >= total)
        let end_excl = min((k + 1) * per_chunk, total)
        var chunk : array<string>
        chunk |> reserve(end_excl - start_idx)
        for (j in range(start_idx, end_excl)) {
            chunk |> push(items[j])
        }
        out |> emplace(chunk)
    }
    return <- out
}


// Argv prefix for self-spawning workers: everything before the `--`
// separator, so binary flags like `-dasroot <path>` survive into workers.
// Without a separator, everything up to and including the first `.das`
// script argument; [exe, script] only as a last resort.
def public spawn_argv_prefix() : array<string> {
    let raw_args <- get_command_line_arguments()
    var prefix : array<string>
    prefix |> reserve(length(raw_args))
    var saw_sep = false
    for (a in raw_args) {
        if (a == "--") {
            saw_sep = true
            break
        }
        prefix |> push(a)
    }
    if (!saw_sep) {
        prefix |> resize(0)
        var found_script = false
        for (a in raw_args) {
            prefix |> push(a)
            if (a |> ends_with(".das")) {
                found_script = true
                break
            }
        }
        if (!found_script && length(prefix) > 2) {
            prefix |> resize(2)
        }
    }
    return <- prefix
}


struct private InternalInput {
    index : int
    argv : array<string>
}


// Fan out one popen_argv per element of `argvs`. Spawns N worker
// threads (one per chunk), captures stdout + exit code per chunk,
// blocks until all done. Returns results in input order
// (results[k].index == k).
//
// Each worker thread runs `popen_argv(argv, 0.0, ...)` — no per-worker
// timeout. Callers needing timeouts should pass them through their own
// argv flags.
def public run_chunk_workers(argvs : array<array<string>>) : array<ChunkResult> {
    let n = length(argvs)
    var results : array<ChunkResult>
    results |> resize(n)
    for (i in range(n)) {
        results[i].index = i
        results[i].exit_code = -1
    }
    return <- results if (n == 0)

    var inputs : array<InternalInput>
    inputs |> reserve(n)
    for (k in range(n)) {
        var av : array<string>
        av := argvs[k]
        inputs |> emplace(InternalInput(index = k, argv <- av))
    }

    with_channel(n) $(inCh) {
        with_channel(n) $(outCh) {
            with_job_status(n) $(done) {
                for (_t in range(n)) {
                    new_thread <| @ {
                        for_each_clone(inCh) $(input : InternalInput#) {
                            var output : string
                            let exit_code = unsafe(popen_argv(input.argv, 0.0, $(f) {
                                if (f != null) {
                                    output := unsafe(fread_to_eof(f))
                                }
                            }))
                            outCh |> push_clone(ChunkResult(
                                index = input.index,
                                exit_code = exit_code,
                                stdout = output))
                            outCh |> notify()
                        }
                        inCh |> release()
                        outCh |> release()
                        done |> notify_and_release()
                    }
                }
                for (inp in inputs) {
                    inCh |> push_clone(inp)
                    inCh |> notify()
                }
                for (r in each_clone(outCh, type<ChunkResult>)) {
                    if (r.index >= 0 && r.index < length(results)) {
                        results[r.index].index = r.index
                        results[r.index].exit_code = r.exit_code
                        results[r.index].stdout := r.stdout
                    }
                }
                done |> join()
            }
        }
    }

    return <- results
}
