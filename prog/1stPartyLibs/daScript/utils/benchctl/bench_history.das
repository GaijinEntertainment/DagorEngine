options gen2
options indenting = 4

module bench_history public

require bench_runner
require daslib/fio
require daslib/json_boost

enum RunStatus {
    ok
    build_failed
    bench_failed
}

enum StandExit {
    ok = 0
    failed = 1
}

// JSON: run record, written per night, read by `report` - see README.md section 2
struct CommitInfo {
    sha, date, subject, author : string
}

// JSON: run record
struct MachineInfo {
    host : string
    cores : int
}

// JSON: run record
struct BuildInfo {
    status : string = "ok"
    seconds : double
    log_tail : string
}

// JSON: run record
struct RunRecord {
    schema : int = 1
    run_id, started, finished : string
    seconds : double
    commit : CommitInfo
    machine : MachineInfo
    build : BuildInfo = BuildInfo()
    lanes : table<string; string>
    status : RunStatus
    files : array<FileResult>
}

// JSON: run record
struct Failure {
    path, lane, status, message : string
}

// JSON: data.json, read by site/app.js
struct RunSummary {
    id, started : string
    status : RunStatus
    seconds : double
    commit : CommitInfo
    build : BuildInfo = BuildInfo()
    machine : MachineInfo
    lanes : table<string; string>
    failures, skipped : array<Failure>
    files_ok : int
}

// JSON: data.json, read by site/app.js
struct Series {
    id, group, file, lane : string
    runs : array<int>
    ns, spread : array<double>
}

// JSON: data.json, read by site/app.js
struct Dataset {
    schema : int = 1
    generated, repo_url : string
    latest : int
    runs : array<RunSummary>
    groups : array<string>
    series : array<Series>
}

def derive_status(rec : RunRecord) : RunStatus {
    return RunStatus.build_failed if (rec.build.status != "ok")
    var measured = 0
    for (f in rec.files) {
        return RunStatus.bench_failed if (is_failure(f.status))
        measured++ if (f.status == FileStatus.ok)
    }
    return measured == 0 ? RunStatus.bench_failed : RunStatus.ok
}

def mark_failed_run(var rec : RunRecord&; finished, reason : string) {
    rec.finished = finished
    rec.files |> clear()
    if (rec.build.status == "ok") {
        rec.files |> emplace(FileResult(path = "stand.cmake", id = "bench-stand", group = "stand", lane = "driver",
            status = FileStatus.exit_nonzero, exit_code = 1, message = empty(reason) ? "the driver could not run the suite" : reason))
    }
    rec.status = derive_status(rec)
}

def load_runs(runs_dir : string; var errors : array<string>) : array<RunRecord> {
    var runs : array<RunRecord>
    dir(runs_dir) $(name) {
        return if (extension(name) != ".json")
        var rec = RunRecord()
        if (!sscan_json(fread(path_join(runs_dir, name)), rec) || empty(rec.run_id)) {
            errors |> push("run record does not parse or has no run_id: {path_join(runs_dir, name)}")
            return
        }
        rec.status = derive_status(rec)
        runs |> emplace(rec)
    }
    sort(runs) $(a, b) {
        return a.commit.date != b.commit.date ? a.commit.date < b.commit.date : a.started < b.started
    }
    return <- runs
}

def latest_run_index(runs : array<RunRecord>) : int {
    var best = -1
    for (i in range(length(runs))) {
        if (best < 0 || runs[i].started >= runs[best].started) {
            best = i
        }
    }
    return best
}

def private summarize_run(rec : RunRecord) : RunSummary {
    var s <- RunSummary(id = rec.run_id, started = rec.started, status = rec.status, seconds = rec.seconds,
        commit = rec.commit, build = rec.build, machine = rec.machine)
    s.lanes := rec.lanes
    for (f in rec.files) {
        if (f.status == FileStatus.ok) {
            s.files_ok++
        } else {
            let row = Failure(path = f.path, lane = f.lane, status = "{f.status}", message = f.message)
            (f.status == FileStatus.skipped ? s.skipped : s.failures) |> push(row)
        }
    }
    return <- s
}

def private build_series(runs : array<RunRecord>; var groups : table<string>) : array<Series> {
    var index_of : table<string; int>
    var out : array<Series>
    for (ri, rec in range(length(runs)), runs) {
        for (f in rec.files) {
            groups |> insert(f.group) if (!empty(f.samples))
            for (smp in f.samples) {
                let key = "{f.lane}\t{f.id}#{smp.id}"
                var si = index_of?[key] ?? -1
                if (si < 0) {
                    si = length(out)
                    index_of[key] = si
                    out |> emplace(Series(id = "{f.id}#{smp.id}", group = f.group, file = f.id, lane = f.lane))
                }
                continue if (!empty(out[si].runs) && out[si].runs[length(out[si].runs) - 1] == ri)
                out[si].runs |> push(ri)
                out[si].ns |> push(smp.ns)
                out[si].spread |> push(smp.spread)
            }
        }
    }
    sort(out) $(a, b) {
        return a.lane != b.lane ? a.lane < b.lane : a.id < b.id
    }
    return <- out
}

def build_dataset(runs : array<RunRecord>; generated, repo_url : string) : Dataset {
    var ds <- Dataset(generated = generated, repo_url = repo_url, latest = latest_run_index(runs))
    var groups : table<string>
    ds.series <- build_series(runs, groups)
    ds.groups <- [for (g in keys(groups)); g]
    sort(ds.groups)
    ds.runs <- [for (rec in runs); summarize_run(rec)]
    return <- ds
}

def report_verdict(ds : Dataset) : StandExit {
    return ds.latest < 0 || ds.runs[ds.latest].status != RunStatus.ok ? StandExit.failed : StandExit.ok
}
