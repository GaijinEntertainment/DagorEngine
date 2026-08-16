options gen2
options indenting = 4
options strict_smart_pointers = true

module sql shared public

// Abstract layer for daslang's SQL story. This module is intentionally
// thin in the first chunk of the dasSQLITE rework — its purpose is to
// be the canonical require path that user code points at, regardless of
// the underlying SQL provider (`sqlite/sqlite_boost`, future
// `postgres/postgres_boost`, etc.).
//
// For now it re-exports Option<T> and Result<T, string> so user code
// only needs `require daslib/sql` to use the read-side / write-side
// optional and fallible return types. Provider-specific symbols
// (SqlRunner, with_<provider>, [sql_table], etc.) currently live in
// the provider's own boost module; they migrate up here when a second
// provider lands.

require daslib/option public
require daslib/result public
require strings

typedef SqlError = Option<string>

// SQL quoting — pure string helpers, shared by every provider and by the
// neutral macro layer (daslib/sql_boost) without pulling in any provider.

def sql_quote_id(s : string) : string {
    //! Wraps ``s`` in double quotes (portable identifier quoting); embedded ``"`` doubled.
    let escaped = s |> replace("\"", "\"\"")
    return "\"{escaped}\""
}

def sql_quote_lit(s : string) : string {
    //! Wraps ``s`` in single quotes (SQL string literal); embedded ``'`` doubled.
    //! Used for ``inline_lit`` fragments where the value can't go through ``?`` (CREATE VIEW, PRAGMA, …).
    let escaped = s |> replace("'", "''")
    return "'{escaped}'"
}

// Schema introspection — surfaced by `column_info(type<T>)` over a `[sql_table]`-tagged struct. The data here is the cross-provider subset:

enum SqlType {
    Integer            //!< INTEGER (SQLite) / INT (MySQL, MSSQL) / INT4 (PG)
    Real               //!< REAL    / DOUBLE / FLOAT / FLOAT8
    Text               //!< TEXT    / VARCHAR / NVARCHAR / TEXT
    Blob               //!< BLOB    / BLOB    / VARBINARY / BYTEA
    Null               //!< sentinel for missing / NULL-only contexts
}

struct ColumnInfo {
    name : string                  //!< column name (post-`@sql_column` rename, if any)
    data_type : SqlType            //!< abstract storage class; render via the provider's `<dialect>_sql_type`
    is_pk : bool                   //!< this column is part of `PRIMARY KEY`
    is_nullable : bool             //!< the daslang field is `Option<T>` — column has no `NOT NULL`
    default_expr : string          //!< SQL default expression, or `""` if none
    is_computed : bool             //!< `@sql_computed` — GENERATED ALWAYS AS column
}
