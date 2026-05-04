require dastest/testing_boost
require daslib/strings_boost
require daslib/json_boost
require math

struct BenchmarkEntry {
    id : int64

    commit_hash : string
    tags : string
    insert_date : int64

    full_name : string
    name : string
    sub_name : string

    mode : string

    n : int64
    time_ns : int64

    allocs : int64
    heap_bytes : int64
    string_allocs : int64
    string_heap_bytes : int64
}

struct BenchmarkSampleSet {
    key : string
    list : array<BenchmarkEntry>

    // Assigned later, when stats are computed.
    stats : BenchmarkStats? = null
}

struct BenchmarkStats {
    time_ns : SampleStats = SampleStats()
    allocs : SampleStats = SampleStats()
    heap_bytes : SampleStats = SampleStats()
    string_allocs : SampleStats = SampleStats()
    string_heap_bytes : SampleStats = SampleStats()
}

def as_list(var stats : BenchmarkStats?) : array<SampleStats> {
    return <- [
        stats.time_ns,
        stats.allocs,
        stats.heap_bytes,
        stats.string_allocs,
        stats.string_heap_bytes,
    ]
}

struct SampleStats {
    metric : BenchmarkMetric = BenchmarkMetric()
    values : array<double>
    mean_avg : double
    stddev : double
    variation_coeff : double
    min_value : double
    max_value  : double
}

struct BenchmarkMetric {
    name : string = ""
    header : string = ""
    is_alloc : bool = false
}

def parse_bench_output(data : string) : array<BenchmarkRunStats> {
    var entries : array<BenchmarkRunStats>
    let lines <- split(data, "\n")
    for (l in lines) {
        if (!starts_with(l, "\{")) {
            continue
        }
        var err : string
        let parsed = read_json(l, err)
        if (parsed == null) {
            continue
        }
        let e = from_JV(parsed, type<BenchmarkRunStats>)
        entries |> push(e)
    }
    return <- entries
}

def make_sample_sets(entries : array<BenchmarkEntry>) : table<string, BenchmarkSampleSet?> {
    var result : table<string, BenchmarkSampleSet?>
    for (e in entries) {
        let key := e.name + "/" + e.sub_name
        var samples : BenchmarkSampleSet?
        if (key_exists(result, key)) {
            samples = unsafe(result[key])
        } else {
            samples = new BenchmarkSampleSet()
            result |> insert(key, samples)
        }
        if (samples.key == "") {
            samples.key = key
        }
        samples.list |> push(e)
    }
    return <- result
}

// Fill the sample stats for the provided benchmark sample set.
// It performs outlier filtering and computes other metrics needed to compare
// the sample sets later.
def fill_sample_stats(var samples : BenchmarkSampleSet?) : string {
    var stats = new BenchmarkStats()

    // First, collect all values among every entry.
    // An example: there can be N entries for "foo/bar" benchmark,
    // every such entry is considered to be a single sample for analysis.
    for (e in samples.list) {
        if (e.n == 0l) {
            continue
        }
        let n = double(e.n)
        stats.time_ns.values |> push(double(e.time_ns) / n)
        stats.heap_bytes.values |> push(double(e.heap_bytes) / n)
        stats.allocs.values |> push(double(e.allocs) / n)
        stats.string_heap_bytes.values |> push(double(e.string_heap_bytes) / n)
        stats.string_allocs.values |> push(double(e.string_allocs) / n)
    }

    let process_stat = @@(var s : SampleStats; metric : BenchmarkMetric) : string {
        let err = calc_sample_stats(s)
        s.metric = metric
        if (err != "") {
            return "{metric.name}: {err}"
        }
        return ""
    }
    var errors = [
        process_stat(stats.time_ns, BenchmarkMetric(name = "ns", header = "time")),
        process_stat(stats.heap_bytes, BenchmarkMetric(name = "B", header = "heap bytes", is_alloc = true)),
        process_stat(stats.allocs, BenchmarkMetric(name = "allocs", header = "heap alloc count", is_alloc = true)),
        process_stat(stats.string_heap_bytes, BenchmarkMetric(name = "SB", header = "string heap bytes", is_alloc = true)),
        process_stat(stats.string_allocs, BenchmarkMetric(name = "strings", header = "string heap alloc count", is_alloc = true)),
    ]
    for (err in errors) {
        if (err != "") {
            return err
        }
    }

    samples.stats <- stats
    return ""
}

def private calc_sample_stats(var dst : SampleStats) : string {
    let min_sample_count = 5

    if (length(dst.values) < min_sample_count) {
        return "need more samples, have {length(dst.values)}, want at least {min_sample_count}"
    }

    dst.values = filter_outliers(<- dst.values)
    if (length(dst.values) == 0) {
        return "noisy samples"
    }

    dst.mean_avg = compute_mean_avg(dst.values)
    dst.stddev = compute_stddev(dst.values, dst.mean_avg)
    dst.variation_coeff = dst.mean_avg > 0.0lf ? dst.stddev / dst.mean_avg : 0.0lf

    // Since values are sorted, it's easy to get min/max.
    // We still put them to a separate field for convenience.
    dst.min_value = dst.values[0]
    dst.max_value = dst.values[length(dst.values) - 1]

    return ""
}

def filter_outliers(var values : array<double>) : array<double> {
    // The algorithm below requires values being sorted.
    // https://en.wikipedia.org/wiki/Outlier ctrl+f Tukey's fences
    sort(values) $(x, y) {
        return x < y
    }

    let q1 = percentile(values, 0.25lf)
    let q3 = percentile(values, 0.75lf)
    let iqr = q3 - q1 // interquartile range
    if (iqr == 0.0lf) {
        return <- values
    }

    let lo_fence = q1 - 1.5lf * iqr
    let hi_fence = q3 + 1.5lf * iqr
    var filtered : array<double>
    filtered |> reserve(length(values))
    for (v in values) {
        if (v >= lo_fence && v <= hi_fence) {
            filtered |> push(v)
        }
    }

    return <- filtered
}

def private percentile(values : array<double>; p : double) : double {
    let n = length(values)
    if (n == 0) {
        return 0.0lf
    }
    if (n == 1) {
        return values[0]
    }

    let idx = p * double(n - 1)
    let lo = int(idx)
    let hi = min(lo + 1, n - 1)
    let frac = idx - double(lo)
    return values[lo] * (1.0lf - frac) + values[hi] * frac
}

def private compute_mean_avg(values : array<double>) : double {
    var sum = 0.0lf
    for (v in values) {
        sum += v
    }
    return sum / double(length(values))
}

def private compute_stddev(values : array<double>; mean_avg : double) : double {
    let n = length(values)
    if (n < 2) {
        return 0.0lf
    }
    var variance = 0.0lf
    for (v in values) {
        let d = v - mean_avg
        variance += d * d
    }
    return sqrt(variance / double(n - 1))
}

def geomean(values : array<double>) : double {
    if (length(values) == 0) {
        return 0.0lf
    }
    var log_sum = 0.0lf
    for (v in values) {
        if (v <= 0.0lf) {
            return 0.0lf
        }
        log_sum += log(v)
    }
    return exp(log_sum / double(length(values)))
}

def welch_p_value(stats_old : SampleStats; stats_new : SampleStats) : double {
    let n1 = double(length(stats_old.values))
    let n2 = double(length(stats_new.values))
    if (n1 < 2.0lf || n2 < 2.0lf) {
        return 1.0lf
    }
    let se1_sq = stats_old.stddev * stats_old.stddev / n1
    let se2_sq = stats_new.stddev * stats_new.stddev / n2
    let se_sq = se1_sq + se2_sq
    if (se_sq <= 0.0lf) {
        return stats_old.mean_avg == stats_new.mean_avg ? 1.0lf : 0.0lf
    }
    let t = (stats_old.mean_avg - stats_new.mean_avg) / sqrt(se_sq)
    let df_denom = se1_sq * se1_sq / (n1 - 1.0lf) + se2_sq * se2_sq / (n2 - 1.0lf)
    if (df_denom <= 0.0lf) {
        return 1.0lf
    }
    let df = se_sq * se_sq / df_denom
    let x = df / (df + t * t)
    return beta_i(df * 0.5lf, 0.5lf, x)
}

def private beta_i(a : double; b : double; x : double) : double {
    if (x <= 0.0lf) {
        return 0.0lf
    }
    if (x >= 1.0lf) {
        return 1.0lf
    }
    let lbeta = lgamma_stirling(a + b) - lgamma_stirling(a) - lgamma_stirling(b)
    let bt = exp(lbeta + a * log(x) + b * log(1.0lf - x))
    if (x < (a + 1.0lf) / (a + b + 2.0lf)) {
        return bt * betacf(a, b, x) / a
    }
    return 1.0lf - bt * betacf(b, a, 1.0lf - x) / b
}

def private lgamma_stirling(x : double) : double {
    var xx = x
    var adj = 0.0lf
    while (xx < 7.0lf) {
        adj -= log(xx)
        xx += 1.0lf
    }
    let lx = log(xx)
    var r = (xx - 0.5lf) * lx - xx + 0.9189385332046727lf
    r += 1.0lf / (12.0lf * xx) - 1.0lf / (360.0lf * xx * xx * xx)
    return r + adj
}

def private betacf(a : double; b : double; x : double) : double {
    let FPMIN = 1.0e-30lf
    let EPS = 3.0e-7lf
    let qab = a + b
    let qap = a + 1.0lf
    let qam = a - 1.0lf
    var c = 1.0lf
    var d = 1.0lf - qab * x / qap
    if (abs(d) < FPMIN) {
        d = FPMIN
    }
    d = 1.0lf / d
    var h = d
    var mi = 0
    while (mi < 200) {
        let m = double(mi + 1)
        let m2 = 2.0lf * m
        var aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0lf + aa * d
        if (abs(d) < FPMIN) {
            d = FPMIN
        }
        c = 1.0lf + aa / c
        if (abs(c) < FPMIN) {
            c = FPMIN
        }
        d = 1.0lf / d
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0lf + aa * d
        if (abs(d) < FPMIN) {
            d = FPMIN
        }
        c = 1.0lf + aa / c
        if (abs(c) < FPMIN) {
            c = FPMIN
        }
        d = 1.0lf / d
        let delta = d * c
        h *= delta
        if (abs(delta - 1.0lf) < EPS) {
            break
        }
        mi++
    }
    return h
}
