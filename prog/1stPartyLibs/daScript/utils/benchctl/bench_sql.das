require sqlite/sqlite_boost

require daslib/stringify
require daslib/defer

def db_exec(db : sqlite3?; sql : string) : string {
    var err_msg : string
    defer() {
        sqlite3_free(err_msg)
    }
    let rc = sqlite3_exec(db, sql, unsafe(addr(err_msg)))
    return (rc != SQLITE_OK) ? clone_string(err_msg) : ""
}

def db_bind_text_param(stmt : sqlite3_stmt?; key : string; v : string) {
    let idx = sqlite3_bind_parameter_index(stmt, key)
    sqlite3_bind_text(stmt, idx, v)
}

def db_bind_int64_param(stmt : sqlite3_stmt?; key : string; v : int64) {
    let idx = sqlite3_bind_parameter_index(stmt, key)
    sqlite3_bind_int64(stmt, idx, v)
}

let sql_bench_insert = %stringify~
    INSERT INTO benchmarks (
        commit_hash,
        tags,
        insert_date,
        full_name,
        name,
        sub_name,
        mode,
        n,
        time_ns,
        allocs,
        heap_bytes,
        string_allocs,
        string_heap_bytes
    )
    VALUES (
        @commit_hash,
        @tags,
        strftime('%s', 'now'),
        @name || '/' || @sub_name,
        @name,
        @sub_name,
        @mode,
        @n,
        @time_ns,
        @allocs,
        @heap_bytes,
        @string_allocs,
        @string_heap_bytes
    );
%%

let sql_db_init = %stringify~
    DROP TABLE IF EXISTS benchmarks;

    CREATE TABLE benchmarks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,

        commit_hash TEXT,
        tags TEXT,
        insert_date INTEGER,

        full_name TEXT,
        name TEXT,
        sub_name TEXT,

        mode TEXT,

        n INTEGER,
        time_ns INTEGER,

        allocs INTEGER,
        heap_bytes INTEGER,
        string_allocs INTEGER,
        string_heap_bytes INTEGER
    );
%%
