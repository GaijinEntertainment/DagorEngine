options gen2

require sqlite/sqlite_boost

require daslib/fio
require math
require daslib/defer
require daslib/regex_boost
require daslib/strings_boost

require sql
require utils
require flags
require benchstat
require table_fmt

[export]
def main() {
    let err := run_main()
    var exit_code = 0
    if (err != "") {
        print("[{red_str("error")}] {err}\n")
        exit_code = 1
    }
    unsafe {
        exit(exit_code)
    }
}

def run_main() : string {
    var args <- get_command_line_arguments()
    let arg_index = find_index(args, "--")
    if (arg_index == -1) {
        return "-- separator not found"
    }
    var script_args <- subarray(args, (arg_index + 1)..length(args))
    if (length(script_args) == 0) {
        return "expected a subcommand name"
    }

    let subcmd_name = script_args[0]
    let subcmd_args <- subarray(script_args, 1..length(script_args))
    let flags = parse_flags(subcmd_args)

    if (get_string_arg(flags, "colors", "true") == "false") {
        colored_output = false
    }

    if (subcmd_name == "help") {
        return run_help_cmd()
    }

    let db_filename = get_string_arg(flags, "db", "benchdata.db")
    var need_db_init = !file_exists(db_filename) && find_index([
        "insert",
        "query",
        "compare",
    ], subcmd_name) != -1

    var db : sqlite3?
    defer() {
        sqlite3_close(db)
    }
    var rc = sqlite3_open(db_filename, unsafe(addr(db)))
    if (rc != SQLITE_OK) {
        return "open database: {sqlite3_errmsg(db)}"
    }

    if (need_db_init) {
        let err = run_reset_cmd(db, flags)
        if (err != "") {
            return "implicit db creation: {err}"
        }
    }

    if (subcmd_name == "reset") {
        return run_reset_cmd(db, flags)
    } elif (subcmd_name == "insert") {
        return run_insert_cmd(db, flags)
    } elif (subcmd_name == "query") {
        return run_query_cmd(db, flags)
    } elif (subcmd_name == "compare") {
        return run_compare_cmd(db, flags)
    } else {
        return "unknown subcommand name: {subcmd_name}"
    }

    return ""
}

def run_help_cmd() : string {
    // TODO: it would be better if we could get this help command for free
    // using a command line library (e.g. like "flag" package in Go)

    print("benchctl - benchmark database management tool\n\n")
    print("Usage:\n")
    print("  daslang benchctl/main.das -- <command> [options]\n\n")
    print("Commands:\n")
    print("  reset    Initialize or reinitialize the benchmark database (drops all data)\n")
    print("  insert   Insert benchmark results from JSON output files\n")
    print("  query    Query and display stored benchmark records\n")
    print("  compare  Compare two sets of benchmark results statistically\n")
    print("  help     Show this help message\n\n")
    print("Common options:\n")
    print("  --db <path>      SQLite3 database file path (default: benchdata.db)\n")
    print("  --colors false   Disable colored terminal output\n\n")
    print("insert options:\n")
    print("  --commit <hash>  Git commit hash to tag results with (default: git rev-parse HEAD)\n")
    print("  --tag <name>     Tag label to attach to results (can be repeated)\n")
    print("  <file>...        Benchmark JSON output files to insert\n\n")
    print("query options:\n")
    print("  --select <cond>  SQL WHERE clause to filter records\n\n")
    print("compare options:\n")
    print("  --select_old <cond>  SQL WHERE clause for baseline (old) results\n")
    print("  --select_new <cond>  SQL WHERE clause for new results\n")
    print("  --s <from=>to>       Regex rename: map old benchmark names to new names\n")
    print("  --old <cond>         --select_old alias\n")
    print("  --new <cond>         --select_new alias\n")
    return ""
}

def query_benchmarks(db : sqlite3?; cond : string; exclude : table<int64> = default<table<int64>>) : tuple<array<BenchmarkEntry>, string> {
    var result : array<BenchmarkEntry>

    var sql_text = "SELECT * FROM benchmarks"
    if (cond != "") {
        sql_text += " WHERE {cond}"
    }

    var err_msg : string
    defer() {
        sqlite3_free(err_msg)
    }
    let rc = sqlite3_exec(db, sql_text, unsafe(addr(err_msg))) $(values, columns) {
        var entry : BenchmarkEntry
        for (v, c in values, columns) {
            if (c == "id") {
                entry.id = to_int64(v)
            } elif (c == "commit_hash") {
                entry.commit_hash := v
            } elif (c == "tags") {
                entry.tags := v
            } elif (c == "insert_date") {
                entry.insert_date = to_int64(v)
            } elif (c == "full_name") {
                entry.full_name := v
            } elif (c == "name") {
                entry.name := v
            } elif (c == "sub_name") {
                entry.sub_name := v
            } elif (c == "mode") {
                entry.mode := v
            } elif (c == "n") {
                entry.n = to_int64(v)
            } elif (c == "time_ns") {
                entry.time_ns = to_int64(v)
            } elif (c == "allocs") {
                entry.allocs = to_int64(v)
            } elif (c == "heap_bytes") {
                entry.heap_bytes = to_int64(v)
            } elif (c == "string_allocs") {
                entry.string_allocs = to_int64(v)
            } elif (c == "string_heap_bytes") {
                entry.string_heap_bytes = to_int64(v)
            }
        }
        if (!key_exists(exclude, entry.id)) {
            result |> push(entry)
        }
        return SQLITE_OK
    }

    var err = ""
    if (rc != SQLITE_OK) {
        err = clone_string(sqlite3_errmsg(db))
    }

    return (<- result, err)
}

def run_compare_cmd(db : sqlite3?; flags : Flags) : string {
    var cond_old = get_string_aliased_arg(flags, "select_old", "old", "")
    var cond_new = get_string_aliased_arg(flags, "select_new", "new", "")

    var key_map = @(x : string) : string {
        return x
    }
    var key_mapper = get_string_arg(flags, "s", "")
    if (key_mapper != "") {
        let parts <- split(key_mapper, "=>")
        if (length(parts) != 2) {
            return "invalid -s argument, expected 'from=>to' format"
        }
        var re <- regex_compile(parts[0])
        key_map = @capture(:= re, := parts) (x : string) : string {
            return regex_replace(re, x, parts[1])
        }
    }

    let (old_entries, err) = query_benchmarks(db, cond_old)
    if (err != "") {
        return "query --old: {err}"
    }
    var old_ids : table<int64>
    for (e in old_entries) {
        old_ids |> insert(e.id)
    }

    let (new_entries, err) = query_benchmarks(db, cond_new, old_ids)
    if (err != "") {
        return "query --new: {err}"
    }

    var old_samples = make_sample_sets(old_entries)
    var new_samples = make_sample_sets(new_entries)

    var bench_order : array<string>
    for (k in keys(old_samples)) {
        bench_order |> push(k)
    }
    sort(bench_order) $(x, y) {
        return x < y
    }

    var old_total_values = {
        "ns" => array<double>(),
        "B" => array<double>(),
        "allocs" => array<double>(),
        "SB" => array<double>(),
        "strings" => array<double>(),
    }
    var new_total_values = {
        "ns" => array<double>(),
        "B" => array<double>(),
        "allocs" => array<double>(),
        "SB" => array<double>(),
        "strings" => array<double>(),
    }

    for (s in values(old_samples)) {
        let err2 = fill_sample_stats(s)
        if (err2 != "") {
            warning("old_samples: {blue_str(s.key)}: calc stats: {err2}")
        }
    }
    for (s in values(new_samples)) {
        let err2 = fill_sample_stats(s)
        if (err2 != "") {
            warning("new_samples: {blue_str(s.key)}: calc stats: {err2}")
        }
    }

    let fmt_variation <- @(variation : double) : string {
        let percent = variation * 100.0lf
        let s = "{percent:.0f}"
        if (percent < 5.0lf) {
            return "±{s}%"
        }
        return "±{yellow_str(s)}%"
    }
    let fmt_delta <- @(old_v, new_v : double) : string {
        if (old_v > 0.0lf && new_v <= 0.01lf) {
            return green_str("-100%")
        }
        if (old_v == 0.0lf && new_v >= 1.0lf) {
            return red_str("+inf%")
        }
        let delta_percent = (new_v - old_v) / old_v * 100.0lf
        let sign = delta_percent >= 0.0lf ? "+" : ""
        let delta_fmt = "{sign}{delta_percent:.2f}%"
        if (delta_percent < 0.0lf) {
            return green_str(delta_fmt)
        } elif (delta_percent > 0.0lf) {
            return red_str(delta_fmt)
        } else {
            return delta_fmt
        }
    }
    for (k in bench_order) {
        var s_old = unsafe(old_samples[k])
        // The stats are null if an error occured when analyzing it.
        if (s_old.stats == null) {
            continue
        }
        let mapped_k = key_map(k)
        if (!key_exists(new_samples, mapped_k)) {
            if (k != mapped_k) {
                warning("no pair found for {k} ({mapped_k})\n")
            } else {
                warning("no pair found for {k}\n")
            }
            continue
        }

        var s_new = unsafe(new_samples[mapped_k])
        if (s_new.stats == null) {
            continue
        }

        if (k != mapped_k) {
            print("{blue_str(k)} vs {blue_str(mapped_k)}\n")
        } else {
            print("{blue_str(k)}\n")
        }
        var table_rows : array<TableRow?>
        for (stat_old, stat_new in as_list(s_old.stats), as_list(s_new.stats)) {
            if (stat_old.metric.is_alloc && stat_old.mean_avg == 0.0lf && stat_new.mean_avg == 0.0lf) {
                continue
            }
            if (stat_old.metric.is_alloc && abs(stat_old.mean_avg - stat_new.mean_avg) < 1.0lf) {
                continue
            }
            if (length(stat_old.values) == 0 || length(stat_new.values) == 0) {
                continue
            }

            old_total_values[stat_old.metric.name] |> push(stat_old.mean_avg)
            new_total_values[stat_new.metric.name] |> push(stat_new.mean_avg)

            let p = welch_p_value(stat_old, stat_new)
            let n_str = "{length(stat_old.values)}+{length(stat_new.values)}"
            var p_str = ""
            if (p < 0.001lf) {
                p_str = "p<0.001"
            } elif (p < 0.005lf) {
                p_str = "p={p:.3f}"
            } elif (p < 0.010lf) {
                p_str = "p=" + yellow_str("{p:.3f}")
            } else {
                p_str = "p=" + red_str("{p:.3f}")
            }
            let cmp_str = "({p_str} n={n_str})"

            let metric_name = stat_old.metric.name
            let old_v = "{stat_old.mean_avg:.1f}"
            let new_v = "{stat_new.mean_avg:.1f}"

            var delta_str = "~"
            if (p < 0.05lf) {
                delta_str = fmt_delta(stat_old.mean_avg, stat_new.mean_avg)
            }

            table_rows |> push(new TableRow(columns <- [
                "{yellow_str(old_v)} {metric_name}/op {fmt_variation(stat_old.variation_coeff)}",
                "{yellow_str(new_v)} {metric_name}/op {fmt_variation(stat_new.variation_coeff)}",
                delta_str,
                cmp_str,
            ]))
        }
        var tabspec = new TableData(
            rows <- table_rows,
            col_gap = 4,
            indent = "    ",
        )
        print(format_table(tabspec))
    }

    var table_rows : array<TableRow?>
    for (k, old_values in keys(old_total_values), values(old_total_values)) {
        let new_values & = unsafe(new_total_values[k])
        if (length(old_values) == 0 && length(new_values) == 0) {
            continue
        }
        let gm_old = geomean(old_values)
        let gm_new = geomean(new_values)
        let gm_old_s = "{gm_old:.1f}"
        let gm_new_s = "{gm_new:.1f}"
        table_rows |> push(new TableRow(columns <- [
            "{yellow_str(gm_old_s)} {k}",
            "{yellow_str(gm_new_s)} {k}",
            fmt_delta(gm_old, gm_new),
        ]))
    }
    if (length(table_rows) > 0) {
        print("\n")
        print("GEOMEAN:\n\n")
        var tabspec = new TableData(
            rows <- table_rows,
            col_gap = 4,
            indent = "    ",
        )
        print(format_table(tabspec))
    }

    return ""
}

def run_query_cmd(db : sqlite3?; flags : Flags) : string {
    var cond = get_string_arg(flags, "select", "")
    let (entries, err) = query_benchmarks(db, cond)
    if (err != "") {
        return "run query: {err}"
    }
    if (length(entries) == 0) {
        print("no results\n")
        return ""
    }
    var table_rows : array<TableRow?>
    for (e in entries) {
        let commit_short = length(e.commit_hash) > 8 ? slice(e.commit_hash, 0, 8) : e.commit_hash
        let ns_per_op = e.n > 0l ? double(e.time_ns) / double(e.n) : 0.0lf
        table_rows |> push(new TableRow(columns <- [
            e.full_name,
            commit_short,
            e.tags,
            "{e.n}",
            "{ns_per_op:.1f} ns/op",
        ]))
    }
    print("  {yellow_str("benchmark")}  {yellow_str("commit")}  {yellow_str("tags")}  {yellow_str("n")}  {yellow_str("ns/op")}\n\n")
    var tabspec = new TableData(
        rows <- table_rows,
        col_gap = 2,
        indent = "  ",
    )
    print(format_table(tabspec))
    return ""
}

def run_reset_cmd(db : sqlite3?; flags : Flags) : string {
    var err = db_exec(db, sql_db_init)
    if (err != "") {
        return "run init query: {err}"
    }

    return ""
}

def run_insert_cmd(db : sqlite3?; flags : Flags) : string {
    if (length(flags.values) == 0) {
        return "missing targets to insert"
    }

    var commit_hash = get_string_arg(flags, "commit", "")
    if (commit_hash == "") {
        commit_hash = cmd_exec("git rev-parse HEAD")
    }

    var tag_string = ""
    for (t in flags.tags) {
        tag_string += "[{t}]"
    }

    var stmt : sqlite3_stmt?
    let rc = sqlite3_prepare_v2(db, sql_bench_insert, -1, unsafe(addr(stmt)), null)
    if (rc != SQLITE_OK) {
        return "prepare insertion statement: {sqlite3_errmsg(db)}"
    }
    defer() {
        sqlite3_finalize(stmt)
    }

    for (filename in flags.values) {
        let data = read_file(filename)
        if (data == "") {
            print("{filename}: ignored\n")
            continue
        }
        let entries = parse_bench_output(data)
        var added = 0
        for (e in entries) {
            db_bind_text_param(stmt, "@commit_hash", commit_hash)
            db_bind_text_param(stmt, "@tags", tag_string)
            db_bind_text_param(stmt, "@name", e.name)
            db_bind_text_param(stmt, "@sub_name", e.sub_name)
            db_bind_text_param(stmt, "@mode", e.func_type)
            db_bind_int64_param(stmt, "@n", int64(e.n))
            db_bind_int64_param(stmt, "@time_ns", e.time_ns)
            db_bind_int64_param(stmt, "@allocs", int64(e.allocs))
            db_bind_int64_param(stmt, "@heap_bytes", int64(e.heap_bytes))
            db_bind_int64_param(stmt, "@string_allocs", int64(e.string_allocs))
            db_bind_int64_param(stmt, "@string_heap_bytes", int64(e.string_heap_bytes))
            let step_rc = sqlite3_step(stmt)
            if (step_rc != SQLITE_DONE) {
                return "insert failed: {sqlite3_errmsg(db)}"
            }
            added++
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        }
        print("{filename}: added {added} samples\n")
    }

    return ""
}

def warning(msg : string) {
    print("{yellow_str("[warning]")} {msg}\n")
}
