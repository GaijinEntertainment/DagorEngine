options gen2

require daslib/sql
require sqlite/sqlite_boost
require daslib/sql_linq
require dastest/testing_boost public

[sql_table(name = "benchmarks")]
struct Benchmark {
    @sql_primary_key id : int
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

//! The stored row carries commit and tag metadata the statistics never read - benchstat works on
//! the shape dastest emits, so a query hands its rows over as those.
def to_run_stats(rows : array<Benchmark>) : array<BenchmarkRunStats> {
    return <- [for (r in rows); BenchmarkRunStats(name = r.name, sub_name = r.sub_name,
        n = int(r.n), time_ns = r.time_ns, allocs = uint64(r.allocs), heap_bytes = uint64(r.heap_bytes),
        string_allocs = uint64(r.string_allocs), string_heap_bytes = uint64(r.string_heap_bytes),
        func_type = r.mode)]
}
