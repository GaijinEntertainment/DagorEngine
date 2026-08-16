options gen2
options indenting = 4
options no_unused_function_arguments = false
options strict_smart_pointers

module sql_provider shared public

require daslib/ast public
require daslib/sql public
require strings

//! Compile-time SQL provider registry CORE — types + the registration/lookup surface.
//! The `_sql` chain analyzer and the `[sql_table]` structure macro are provider-neutral;
//! everything dialect- or handle-specific routes through a `SqlProviderInfo` entry
//! registered here. Registries live per macro context (see skills/das_macros.md "Macro
//! modules each compile into their own context"), so each consulting macro module calls
//! `sql_register_present_providers` (in `daslib/sql_boost`, which pulls each present
//! provider's shim in via `require ?<mod>` — this core module stays provider-blind so
//! shims can require it without cycling). Contract: modules/dasSQLITE/PROVIDER_CONTRACT.md.

enum public SqlPkReport {
    //! How `insert(row)` learns the new row's primary key.
    LastInsertRowid
    ReturningPk
}

bitfield public SqlProviderCaps {
    //! Feature gates for the macros' capability checks (wired as providers land);
    fts5
    client_udfs
    returning
    distinct_on
}

struct public SchemaFromCol {
    //! One column of an existing table, as reported by the provider's schema reader
    //! (`readTableSchema` hook). `data_type` is provider-normalized (e.g. SQLite
    //! affinity rules applied to `decl_type`); `decl_type` is kept raw for diagnostics.
    name : string
    decl_type : string
    data_type : SqlType
    not_null : bool
    pk : bool
    generated : bool    //! GENERATED ALWAYS AS column (VIRTUAL or STORED)
}

struct public SqlProviderInfo {
    //! One registered provider: identity, statement type factory, dialect hooks,
    //! capability flags. Hook fields are plain function pointers — macro code
    //! cannot overload-dispatch across providers, so dispatch is data-driven.
    name : string                                                //! diagnostics: "sqlite"
    runnerModule : string                                        //! registry key: module of the runner struct, e.g. "sqlite_boost"
    runnerStruct : string                                        //! registry key: runner struct name, e.g. "SqlRunner"
    makeStmtType : function<() : TypeDeclPtr>                    //! fresh stmt TypeDecl per splice — bind-block (mutable) parameter flavor
    makeStmtTypeConst : function<() : TypeDeclPtr>               //! const flavor for row-reader block/lambda parameters (spell the qualifier in the type: `type<stmt? const>`)
    renderPlaceholder : function<(ordinal : int) : string>       //! bind placeholder; ordinal is 1-based ("?" ignores it, "$n" styles use it)
    noLimitSpelling : string                                     //! LIMIT clause when only OFFSET is present (sqlite: "LIMIT -1")
    renderJsonExtract : function<(target : string; jsonPath : string) : string>  //! @sql_json descent; jsonPath is the quoted '$.a.b' literal
    quoteId : function<(id : string) : string>                   //! identifier quoting
    sqlTypeFor : function<(t : SqlType) : string>                //! DDL column type spelling
    identityDdl : string                                         //! DDL for the auto-assigned integer primary key column
    readTableSchema : function<(path : string; tbl : string) : Result<array<SchemaFromCol>, string>>  //! `[sql_table(schema_from=...)]` compile-time introspection; null when unsupported
    caps : SqlProviderCaps
    pkReport : SqlPkReport
}

var private g_sql_providers : array<SqlProviderInfo>

def public sql_register_present_providers_done : bool {
    //! True once at least one provider is registered in this macro context — the
    //! `sql_register_present_providers` aggregator (daslib/sql_boost) checks it so twin
    //! `[_macro]`s stay idempotent (a pass that found no providers re-runs harmlessly).
    return !empty(g_sql_providers)
}

def public register_sql_provider(var info : SqlProviderInfo) {
    //! Appends a provider entry. Reached via `sql_register_present_providers`
    //! (daslib/sql_boost); a provider shim exposes its registration function for
    //! the aggregator to call. Duplicate (runnerModule, runnerStruct) keys panic.
    for (p in g_sql_providers) {
        if (p.runnerModule == info.runnerModule && p.runnerStruct == info.runnerStruct) {
            panic("register_sql_provider: duplicate runner key {info.runnerModule}::{info.runnerStruct} ({p.name} vs {info.name})")
        }
    }
    g_sql_providers |> emplace(info)
}

def public find_sql_provider_by_runner(runnerT : TypeDeclPtr) : int {
    //! Registry index for the provider whose runner struct matches `runnerT`
    //! (the inferred type of the chain's db expression), or -1.
    if (runnerT == null || runnerT.structType == null) return -1
    let st = runnerT.structType
    if (st._module == null) return -1
    for (i in range(length(g_sql_providers))) {
        if (g_sql_providers[i].runnerStruct == st.name && g_sql_providers[i].runnerModule == st._module.name) {
            return i
        }
    }
    return -1
}

def public sql_provider_at(idx : int) : SqlProviderInfo {
    //! Copy of the registered entry (hook fields are pointers; the copy is cheap).
    assert(idx >= 0 && idx < length(g_sql_providers), "sql_provider_at: index out of range")
    return g_sql_providers[idx]
}

def public sql_provider_count() : int => length(g_sql_providers)

def public sql_provider_names() : string {
    //! Comma-separated registered provider names, for macro_error diagnostics.
    return build_string() $(var w) {
        for (i in range(length(g_sql_providers))) {
            if (i > 0) {
                w |> write(", ")
            }
            w |> write(g_sql_providers[i].name)
        }
    }
}
