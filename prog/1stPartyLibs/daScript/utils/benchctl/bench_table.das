options gen2

require daslib/sql
require sqlite/sqlite_boost
require sqlite/sqlite_linq

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
