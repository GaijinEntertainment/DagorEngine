options gen2

require daslib/fio
require daslib/defer
require daslib/regex_boost
require daslib/strings_boost
require math
require strings

require sqlite/sqlite_boost
require sqlite/sqlite_linq
require bench_table
require bench_args
require benchstat
require utils
require table_fmt

def print_usage() {
    print("benchctl - benchmark database management tool\n\n")
    print("Usage:\n")
    print("  daslang utils/benchctl/main.das -- <command> [options]\n\n")
    print("Commands:\n")
    print("  reset    Initialize or reinitialize the benchmark database (drops all data)\n")
    print("  insert   Insert benchmark JSON output files into the database\n")
    print("  query    Display benchmark records (filter by --commit / --tag)\n")
    print("  compare  Compare two sets of results statistically\n")
    print("  help     Show this help message\n\n")
    print_benchctl_help()
}

[export]
def main : int {
    var argv <- get_command_line_arguments()
    let dash_idx = find_index(argv, "--")
    if (dash_idx != -1) {
        argv <- subarray(argv, (dash_idx + 1)..length(argv))
    }

    if (empty(argv) || find_index(argv, "--help") != -1 || find_index(argv, "-h") != -1) {
        print_usage()
        return 0
    }

    let parsed <- parse_benchctl_args(argv)
    if (!parsed.ok) {
        print("[{red_str("error")}] {parsed.err}\n\n")
        print_usage()
        return 1
    }

    if (parsed.no_color) {
        colored_output = false
    }

    if (parsed.help) {
        print_usage()
        return 0
    }

    if (parsed.command == "" || parsed.command == "help") {
        print_usage()
        return 0
    }

    let err = run_subcommand(parsed)
    if (err != "") {
        print("[{red_str("error")}] {err}\n")
        return 1
    }
    return 0
}

def run_subcommand(parsed : ParsedArgs) : string {
    let cmd = parsed.command
    if (cmd != "reset" && cmd != "insert" && cmd != "query" && cmd != "compare") return "unknown subcommand: {cmd}"
    let need_init = !file_exists(parsed.db) && (cmd == "insert" || cmd == "query" || cmd == "compare")
    var open_r <- try_open_sqlite(parsed.db)
    if (open_r |> is_err) return "open database: {open_r |> unwrap_err}"
    var inscope db <- open_r._value

    if (need_init) {
        let init_err = run_reset_cmd(db)
        if (init_err != "") return init_err
    }

    if (cmd == "reset") {
        return run_reset_cmd(db)
    } elif (cmd == "insert") {
        return run_insert_cmd(db, parsed)
    } elif (cmd == "query") return run_query_cmd(db, parsed)
    return run_compare_cmd(db, parsed)
}

def validate_tag_chars(t : string) : string {
    if (find(t, "[") != -1 || find(t, "]") != -1) return "tag '{t}' contains illegal character '[' or ']'"
    return ""
}

def run_reset_cmd(db : SqlRunner) : string {
    let drop_err = db |> try_drop_table_if_exists(type<Benchmark>)
    if (drop_err |> is_some) return "drop table: {drop_err |> unwrap}"
    let create_err = db |> try_create_table(type<Benchmark>)
    if (create_err |> is_some) return "create table: {create_err |> unwrap}"
    return ""
}

def run_insert_cmd(db : SqlRunner; parsed : ParsedArgs) : string {
    if (empty(parsed.files)) return "missing targets to insert"

    var commit_hash = parsed.commit
    if (commit_hash == "") {
        commit_hash = cmd_exec("git rev-parse HEAD")
    }

    var tag_string = ""
    for (t in parsed.tags) {
        if (t == "") continue
        let tag_err = validate_tag_chars(t)
        if (tag_err != "") return tag_err
        tag_string += "[{t}]"  // nolint:PERF001 — typically 0-2 tags, not a hot path
    }

    let now = int64(get_clock())

    for (filename in parsed.files) {
        let data = read_file(filename)
        if (data == "") {
            print("{filename}: ignored\n")
            continue
        }
        let stats_entries = parse_bench_output(data)
        var rows : array<Benchmark>
        rows |> reserve(length(stats_entries))
        for (e in stats_entries) {
            rows |> push(Benchmark(
                id = 0,
                commit_hash = commit_hash,
                tags = tag_string,
                insert_date = now,
                full_name = "{e.name}/{e.sub_name}",
                name = e.name,
                sub_name = e.sub_name,
                mode = e.func_type,
                n = int64(e.n),
                time_ns = e.time_ns,
                allocs = int64(e.allocs),
                heap_bytes = int64(e.heap_bytes),
                string_allocs = int64(e.string_allocs),
                string_heap_bytes = int64(e.string_heap_bytes),
            ))
        }
        let txn_err = db |> try_insert(rows)
        if (txn_err |> is_err) return "insert {filename}: {txn_err |> unwrap_err}"
        print("{filename}: added {length(rows)} samples\n")
    }
    return ""
}

def query_benchmarks(db : SqlRunner; commit_filter : string; tag_filter : string) : array<Benchmark> {
    let want_tag = (tag_filter == "" ? "" : "[{tag_filter}]")
    return <- _sql(db |> select_from(type<Benchmark>)
        |> _where(commit_filter == "" || _.commit_hash == commit_filter)
        |> _where(want_tag == "" || _.tags |> contains(want_tag)))
}

def run_query_cmd(db : SqlRunner; parsed : ParsedArgs) : string {
    if (length(parsed.tags) > 1) return "--tag may only be specified once for query"
    let tag_filter = !empty(parsed.tags) ? parsed.tags[0] : ""
    let tag_err = validate_tag_chars(tag_filter)
    if (tag_err != "") return tag_err
    let entries <- query_benchmarks(db, parsed.commit, tag_filter)
    if (empty(entries)) {
        print("no results\n")
        return ""
    }
    var table_rows : array<TableRow?>
    table_rows |> reserve(length(entries))
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

def run_compare_cmd(db : SqlRunner; parsed : ParsedArgs) : string {
    let old_tag_err = validate_tag_chars(parsed.old_tag)
    if (old_tag_err != "") return old_tag_err
    let new_tag_err = validate_tag_chars(parsed.new_tag)
    if (new_tag_err != "") return new_tag_err

    let old_entries <- query_benchmarks(db, parsed.old_commit, parsed.old_tag)

    // Exclude rows already in --old from --new so identical / overlapping
    // filters (e.g. both empty) cannot compare a result set against itself.
    // Push to SQL via `_not_in(old_ids)` — JSON-bridge handles arbitrary size.
    var old_ids : array<int>
    old_ids |> reserve(length(old_entries))
    for (e in old_entries) {
        old_ids |> push(e.id)
    }
    let new_want_tag = (parsed.new_tag == "" ? "" : "[{parsed.new_tag}]")
    let new_entries <- _sql(db |> select_from(type<Benchmark>)
        |> _where(parsed.new_commit == "" || _.commit_hash == parsed.new_commit)
        |> _where(new_want_tag == "" || _.tags |> contains(new_want_tag))
        |> _where(_.id._not_in(old_ids)))

    if (empty(old_entries)) return "no rows match --old-commit / --old-tag"
    if (empty(new_entries)) return "no rows match --new-commit / --new-tag (after excluding rows already in --old)"

    var key_map = @(x : string) : string {
        return x
    }
    if (parsed.s != "") {
        let parts <- split(parsed.s, "=>")
        if (length(parts) != 2) return "invalid -s argument, expected 'from=>to' format"
        var re <- regex_compile(parts[0])
        key_map = @capture(:= re, := parts) (x : string) : string {
            return regex_replace(re, x, parts[1])
        }
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
        if (percent < 5.0lf) return "±{s}%"
        return "±{yellow_str(s)}%"
    }
    let fmt_delta <- @(old_v, new_v : double) : string {
        if (old_v > 0.0lf && new_v <= 0.01lf) return green_str("-100%")
        if (old_v == 0.0lf && new_v >= 1.0lf) return red_str("+inf%")
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
        if (s_old.stats == null) continue
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
        if (s_new.stats == null) continue

        if (k != mapped_k) {
            print("{blue_str(k)} vs {blue_str(mapped_k)}\n")
        } else {
            print("{blue_str(k)}\n")
        }
        var table_rows : array<TableRow?>
        for (stat_old, stat_new in as_list(s_old.stats), as_list(s_new.stats)) {
            if ((stat_old.metric.is_alloc && stat_old.mean_avg == 0.0lf && stat_new.mean_avg == 0.0lf)
                || (stat_old.metric.is_alloc && abs(stat_old.mean_avg - stat_new.mean_avg) < 1.0lf)
                || empty(stat_old.values) || empty(stat_new.values)) continue

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
        if (empty(old_values) && empty(new_values)) continue
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
    if (!empty(table_rows)) {
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

def warning(msg : string) {
    print("{yellow_str("[warning]")} {msg}\n")
}
