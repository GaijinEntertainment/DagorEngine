options gen2
options indenting = 4

module bench_suite public

require daslib/fio
require daslib/json_boost

// JSON: suite.json
struct FileOverride {
    @optional skip : string
    @optional skip_lanes : table<string; string>
    @optional timeout_seconds : int
}

// JSON: suite.json
struct SuiteConfig {
    root : string = "benchmarks"
    exclude, lanes : array<string>
    repeat : int = 3
    timeout_seconds : int = 900
    files : table<string; FileOverride>
    lane_excludes : table<string; table<string; string>>
}

struct BenchFile {
    path, id, group : string
    timeout_seconds : int
    skip : string
    skip_lanes : table<string; string>
}

def skip_reason(f : BenchFile; lane : string) : string {
    return !empty(f.skip) ? f.skip : f.skip_lanes?[lane] ?? ""
}

def load_suite_config(path : string; var error : string&) : SuiteConfig {
    error = ""
    var cfg = SuiteConfig()
    if (!stat(path).is_reg) {
        error = "suite config not found: {path}"
    } elif (!sscan_json(fread(path), cfg)) {
        error = "suite config does not parse as JSON: {path}"
    } elif (empty(cfg.lanes)) {
        error = "suite config lists no lanes: {path}"
    } elif (cfg.repeat <= 0 || cfg.timeout_seconds <= 0) {
        error = "suite config needs positive repeat and timeout_seconds: {path}"
    }
    for (lane in keys(cfg.lane_excludes)) {
        if (find_index(cfg.lanes, lane) < 0) {
            error = "suite config excludes files from lane \"{lane}\", which it does not run: {path}"
        }
    }
    for (lane in cfg.lanes) {
        if (lane != "interp" && lane != "jit" && lane != "aot") {
            error = "suite config names an unknown lane \"{lane}\" (interp, jit or aot): {path}"
        }
    }
    return <- cfg
}

def discover_files(repo_root : string; cfg : SuiteConfig; var error : string&) : array<BenchFile> {
    error = ""
    let root = path_join(repo_root, cfg.root)
    var rels : array<string>
    dir_rec(root) $(name, is_dir) {
        let rel = to_generic_path(name)
        return if (is_dir || extension(rel) != ".das")
        for (pattern in cfg.exclude) {
            return if (match_glob(pattern, rel))
        }
        rels |> push(rel)
    }
    sort(rels)
    var out : array<BenchFile>
    out |> reserve(length(rels))
    for (rel in rels) {
        var f = BenchFile(path = rel, id = stem_path(rel), group = group_of(rel), timeout_seconds = cfg.timeout_seconds)
        for (lane in keys(cfg.lane_excludes)) {
            cfg.lane_excludes |> get(lane) $(globs) {
                for (pattern in keys(globs)) {
                    if (match_glob(pattern, rel)) {
                        f.skip_lanes[lane] = globs?[pattern] ?? ""
                    }
                }
            }
        }
        cfg.files |> get(rel) $(ov) {
            f.skip = ov.skip
            for (lane in keys(ov.skip_lanes)) {
                f.skip_lanes[lane] = ov.skip_lanes?[lane] ?? ""
            }
            if (ov.timeout_seconds > 0) {
                f.timeout_seconds = ov.timeout_seconds
            }
        }
        out |> emplace(f)
    }
    for (k in keys(cfg.files)) {
        if (!stat(path_join(root, k)).is_reg) {
            error = "suite config overrides \"{k}\", which is not a file under {root}"
        }
    }
    return <- out
}

def group_of(rel : string) : string {
    let d = to_generic_path(dir_name(rel))
    return (empty(d) || d == ".") ? "root" : d
}

def stem_path(rel : string) : string {
    let d = group_of(rel)
    return d == "root" ? stem(rel) : "{d}/{stem(rel)}"
}
