options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers

// Intentionally opt in for user-facing docs: keep comments concise and actionable.
options _comment_hygiene = true

module sql_boost shared public

require strings public
require daslib/fio                  // [sql_table(schema_from=...)] uses is_absolute / path_join at macro time
require daslib/contracts
require daslib/option public
require daslib/sql public
require daslib/sql_provider public
require daslib/ast_boost
require daslib/macro_boost
require daslib/templates
require daslib/templates_boost
require daslib/json public          // [sql_table] @sql_json adapter codegen calls JV / read_json / write_json
require daslib/json_boost public    // [sql_table] @sql_json adapter codegen calls from_JV
require daslib/archive public       // [sql_table] @sql_blob adapter codegen calls mem_archive_save / mem_archive_load

require ?sqlite sqlite/sqlite_provider    // in-tree provider registration shim -- loaded only when sqlite is linked
require ?duckdb duckdb/duckdb_provider    // external provider (github.com/borisbat/dasDuckDB) -- loaded only when duckdb is linked
require ?postgres postgres/postgres_provider    // external provider (github.com/borisbat/dasPostgreSQL) -- loaded only when postgres is linked

//! Provider-neutral SQL macro layer: ``[sql_table]`` / ``[sql_view]`` / ``[sql_index]``,
//! the typed ALTER macros (``add_column`` / ``create_index``), and the ``sql_bind`` /
//! ``sql_extract`` custom-type adapter rail. Everything dialect- or handle-specific
//! routes through the provider registry (``daslib/sql_provider``); the runtime surface
//! (runner, exec/query, runner helpers) lives in each provider's boost module
//! (e.g. ``sqlite/sqlite_boost``). Contract: modules/dasSQLITE/PROVIDER_CONTRACT.md.

def public sql_register_present_providers {
    //! Registers every linked provider, once per macro context; call from a ``[_macro]``
    //! in each registry-consulting macro module (``[init]`` does not reach macro contexts).
    //! Adding a provider = one ``require ?<mod>`` line above + one ``static_if`` branch here.
    if (sql_register_present_providers_done()) return
    static_if (typeinfo builtin_module_exists(sqlite)) {
        register_sqlite_provider()
    }
    static_if (typeinfo builtin_module_exists(duckdb)) {
        register_duckdb_provider()
    }
    static_if (typeinfo builtin_module_exists(postgres)) {
        register_postgres_provider()
    }
}

// This module's macros consult the provider registry, and only a module's own
// [_macro] functions run at its macro build (linq_fold.das:123 pattern).
[_macro]
def private _sql_boost_register_providers {
    if (!is_compiling_macros_in_module("sql_boost")) return
    sql_register_present_providers()
}

// ===== LIKE-pattern escape helpers (tut 40 — string predicates) =====

def like_escape_prefix(s : string) : string {
    //! Returns `s` with `\`, `%`, `_` doubled via `\`, then `%` appended.
    //! Pair with `col LIKE ? ESCAPE '\'`.
    return (s |> replace("\\", "\\\\") |> replace("%", "\\%") |> replace("_", "\\_")) + "%"
}

def like_escape_suffix(s : string) : string {
    //! Returns `s` with `\`, `%`, `_` doubled via `\`, prepended with `%`.
    //! Pair with `col LIKE ? ESCAPE '\'`.
    return "%" + (s |> replace("\\", "\\\\") |> replace("%", "\\%") |> replace("_", "\\_"))
}

def like_escape_substring(s : string) : string {
    //! Returns `s` with `\`, `%`, `_` doubled via `\`, wrapped in `%…%`.
    //! Pair with `col LIKE ? ESCAPE '\'`.
    return "%" + (s |> replace("\\", "\\\\") |> replace("%", "\\%") |> replace("_", "\\_")) + "%"
}

// ===== SQL adapter rail: sql_bind / sql_extract  (tut 26) =====

// --- four primitive identity overloads ---

def sql_bind(v : int64) : int64 {
    return v
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<int64>) : int64 {
    return v
}

def sql_bind(v : double) : double {
    return v
}

[unused_argument(t)]
def sql_extract(v : double; t : type<double>) : double {
    return v
}

def sql_bind(s : string) : string {
    return s
}

[unused_argument(t)]
def sql_extract(s : string; t : type<string>) : string {
    return s
}

def sql_bind(b : array<uint8>) : array<uint8> {
    //! Identity passthrough for BLOB columns. For multi-MB payloads prefer the streaming
    //! ``sqlite3_blob_open`` API to avoid the implicit clone + ``SQLITE_TRANSIENT`` copy.
    return <- clone_to_move(b)
}

[unused_argument(t)]
def sql_extract(b : array<uint8>; t : type<array<uint8>>) : array<uint8> {
    return <- clone_to_move(b)
}

// --- stdlib widenings (all integer kinds widen to int64; float widens to double) ---

def sql_bind(v : int) : int64 {
    return int64(v)
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<int>) : int {
    return int(v)
}

def sql_bind(v : int8) : int64 {
    return int64(v)
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<int8>) : int8 {
    return int8(v)
}

def sql_bind(v : int16) : int64 {
    return int64(v)
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<int16>) : int16 {
    return int16(v)
}

def sql_bind(v : uint) : int64 {
    return int64(v)
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<uint>) : uint {
    return uint(v)
}

def sql_bind(v : uint8) : int64 {
    return int64(v)
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<uint8>) : uint8 {
    return uint8(v)
}

def sql_bind(v : uint16) : int64 {
    return int64(v)
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<uint16>) : uint16 {
    return uint16(v)
}

def sql_bind(v : uint64) : int64 {
    //! Reinterpret cast — values above ``int64.max`` round-trip through
    //! the sign bit (SQLite has no UNSIGNED type). For workloads that
    //! actually exceed that range, store as ``string`` via a custom adapter.
    return int64(v)
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<uint64>) : uint64 {
    return uint64(v)
}

def sql_bind(v : float) : double {
    return double(v)
}

[unused_argument(t)]
def sql_extract(v : double; t : type<float>) : float {
    return float(v)
}

def sql_bind(v : bool) : int64 {
    return v ? 1l : 0l
}

[unused_argument(t)]
def sql_extract(v : int64; t : type<bool>) : bool {
    return v != 0l
}

// --- enum generic ---

def sql_bind(value : auto(TT)) : int64 {
    static_if (typeinfo is_enum(value)) {
        return int64(value)
    } else {
        concept_assert(false, "sql_bind: unsupported field type. Define `def sql_bind(v : YourType) : int64 | double | string | array<uint8>` (and a matching sql_extract) in YourType's module.")
        return 0l
    }
}

// Option<T> fallback — `_::sql_bind` so caller-module adapters (incl. @sql_json/@sql_blob's `sql_bind(T)`) resolve.
// Without it, `sql_bind(default<Option<int>>)` falls into the auto(TT) catch-all and trips its concept_assert.
[unused_argument(value)]
def sql_bind(value : $Option < auto(TT) >) {
    return _::sql_bind(default<TT>)
}

[unused_argument(t)]
def sql_extract(value : int64; t : type<auto(TT)>) : TT {
    static_if (typeinfo is_enum(default<TT>)) {
        // int64 → enum via reinterpret. Mirrors typemacro_boost's
        // ``int64_to_enum``; inlined here to avoid pulling that module
        // into every dasSQLITE consumer.
        static_if (typeinfo sizeof(type<TT>) == 1) {
            return unsafe(reinterpret<TT>(int8(value)))
        } static_elif (typeinfo sizeof(type<TT>) == 2) {
            return unsafe(reinterpret<TT>(int16(value)))
        } else {
            return unsafe(reinterpret<TT>(int(value)))
        }
    } else {
        concept_assert(false, "sql_extract: unsupported field type. Define `def sql_extract(v : int64 | double | string | array<uint8>; t : type<YourType>) : YourType` (and a matching sql_bind) in YourType's module.")
        return default<TT>
    }
}
// --- DDL: column-type string from sql_bind's return type ---

[unused_argument(witness)]
def sql_storage_type_witness(witness : int64) : string {
    return "INTEGER"
}

[unused_argument(witness)]
def sql_storage_type_witness(witness : double) : string {
    return "REAL"
}

[unused_argument(witness)]
def sql_storage_type_witness(witness : string) : string {
    return "TEXT"
}

[unused_argument(witness)]
def sql_storage_type_witness(witness : array<uint8>) : string {
    return "BLOB"
}

[unused_argument(witness)]
def sql_storage_type_witness(witness : auto(SS)) : string {
    concept_assert(false, "sql_bind must return one of int64 / double / string / array<uint8>")
    return ""
}

[unused_argument(t)]
def sql_storage_type_for(t : type<auto(TT)>) : string {
    return sql_storage_type_witness(default<typedecl(_::sql_bind(default<TT>))>)
}

[unused_argument(t)]
def sql_storage_type_for(t : type<$Option<auto(TT)>>) : string {
    //! Option<T> nullability is decided by the [sql_table] DDL emitter; the
    //! column type comes from the inner ``T``'s storage primitive.
    return sql_storage_type_for(type<TT>)
}

// --- column_info: same compile-time walk, abstract-typed view ---

[unused_argument(witness)]
def sql_storage_enum_witness(witness : int64) : SqlType {
    return SqlType.Integer
}

[unused_argument(witness)]
def sql_storage_enum_witness(witness : double) : SqlType {
    return SqlType.Real
}

[unused_argument(witness)]
def sql_storage_enum_witness(witness : string) : SqlType {
    return SqlType.Text
}

[unused_argument(witness)]
def sql_storage_enum_witness(witness : array<uint8>) : SqlType {
    return SqlType.Blob
}

[unused_argument(witness)]
def sql_storage_enum_witness(witness : auto(SS)) : SqlType {
    concept_assert(false, "sql_bind must return one of int64 / double / string / array<uint8>")
    return SqlType.Null
}

[unused_argument(t)]
def sql_storage_enum_for(t : type<auto(TT)>) : SqlType {
    return sql_storage_enum_witness(default<typedecl(_::sql_bind(default<TT>))>)
}

[unused_argument(t)]
def sql_storage_enum_for(t : type<$Option<auto(TT)>>) : SqlType {
    return sql_storage_enum_for(type<TT>)
}
[unused_argument(t), template(t)]
def column_info(t : type<auto(TT)>) : array<ColumnInfo> {
    //! Returns the column metadata for a `[sql_table]`-tagged struct ``T``.
    //! Render dialect-specific column types via `sqlite_sql_type(c.data_type)`.
    return <- _::_sql_column_info(default<TT>)
}
// ===== [sql_table] structure macro =====

def find_annotation(args : AnnotationArgumentList; argn : string; default_value : auto(TT)) : TT {
    let v = find_arg(args, argn)
    static_if (typeinfo is_string(default_value)) {
        if (v is tString) return v as tString
    } else {
        if (v is tBool) return v as tBool
    }
    return default_value
}

def private find_string_array_annotation(args : AnnotationArgumentList; argn : string) : array<string> {
    return <- [for (a in args); string(a.sValue); where a.name == argn && a.basicType == Type.tString]
}

// Whitelisted to surface typos at macro-expansion instead of as a runtime SQLite parse error.
let private SQL_DEFAULT_FN_WHITELIST = ["CURRENT_TIMESTAMP", "CURRENT_DATE", "CURRENT_TIME"]

let private SQL_FK_ACTION_WHITELIST = ["cascade", "set_null", "set_default", "restrict", "no_action"]

def private fk_action_to_sql(action : string) : string {
    return "CASCADE" if (action == "cascade")
    return "SET NULL" if (action == "set_null")
    return "SET DEFAULT" if (action == "set_default")
    return "RESTRICT" if (action == "restrict")
    return "NO ACTION"
}

def private sql_quote_string_literal(s : string) : string {
    let escaped = replace(s, "'", "''")
    return "'{escaped}'"
}

def private literal_init_to_default_clause(init : ExpressionPtr) : string {
    if (init == null) return ""
    if (init is ExprConstBool) return (init as ExprConstBool).getValue ? " DEFAULT 1" : " DEFAULT 0"
    if (init is ExprConstInt) return " DEFAULT {(init as ExprConstInt).getValue}"
    if (init is ExprConstInt64) return " DEFAULT {(init as ExprConstInt64).getValue}"
    if (init is ExprConstFloat) return " DEFAULT {(init as ExprConstFloat).getValue}"
    if (init is ExprConstDouble) return " DEFAULT {(init as ExprConstDouble).getValue}"
    if (init is ExprConstString) return " DEFAULT {sql_quote_string_literal(string((init as ExprConstString).value))}"
    return ""
}

struct FieldInfo {
    name : string
    col_name : string
    is_pk : bool
    is_int_pk : bool
    is_optional : bool
    is_unique : bool
    is_computed : bool
    is_stored : bool
    is_json : bool
    is_blob : bool
    is_fts_rank : bool   // [sql_fts5] only — read-only, emits `rank`, skipped on INSERT
    is_fts_unindexed : bool   // [sql_fts5] only — stored metadata excluded from the inverted index
    computed_sql : string
    default_clause : string
    references_clause : string
}

// ===== schema_from helpers =====
// The DB readers live provider-side; the macro reaches them via the `readTableSchema` hook.

def private affinity_to_typedecl(sql_type : SqlType; not_null : bool) : TypeDeclPtr {
    // Build the daslang TypeDecl matching a SQLite affinity class; wrap in Option<T> if nullable.
    var inner : TypeDeclPtr
    if (sql_type == SqlType.Integer) {
        inner = qmacro_type(type<int64>)
    } elif (sql_type == SqlType.Real) {
        inner = qmacro_type(type<double>)
    } elif (sql_type == SqlType.Text) {
        inner = qmacro_type(type<string>)
    } elif (sql_type == SqlType.Blob) {
        inner = qmacro_type(type<array<uint8>>)
    } else {
        return null
    }
    if (not_null) return inner
    return qmacro_type(type<$Option<$t(inner)>>)
}

def private resolve_schema_from_path(p : string) : string {
    // Absolute → as-is; relative → joined under get_das_root() via daslib/fio.
    if (is_absolute(p)) return p
    return path_join(get_das_root(), p)
}

def is_option_field_type(td : TypeDeclPtr) : bool {
    if (td == null) return false
    // Pre-instantiation form: typeMacro `$Option<T>` — what [sql_table] sees during structure_macro apply().
    if (td.baseType == Type.typeMacro && !empty(td.typeMacroExpr) && (td.typeMacroExpr[0] is ExprConstString)) return (td.typeMacroExpr[0] as ExprConstString).value == "Option"
    // Post-instantiation form: structural named tuple ``tuple<_has_value:bool; _value:T>``.
    // Option is built by daslib/option's typemacro_function as a structural tuple — no
    // module home, no Structure clone. Match the canonical shape.
    return (td.baseType == Type.tTuple
            && length(td.argTypes) == 2
            && length(td.argNames) == 2
            && td.argNames[0] == "_has_value"
            && td.argNames[1] == "_value"
            && td.argTypes[0].baseType == Type.tBool)
}

def unwrap_option_payload_type(var td : TypeDeclPtr) : TypeDeclPtr {
    // Unwrap Option<T> → T (or td unchanged); typeMacro carries T in typeMacroExpr[1], the
    // structural tuple form carries T in argTypes[1] (the `_value` slot).
    if (td == null) return td
    if (td.baseType == Type.typeMacro) {
        if (length(td.typeMacroExpr) < 2 || td.typeMacroExpr[0] == null || !(td.typeMacroExpr[0] is ExprConstString)
            || (td.typeMacroExpr[0] as ExprConstString).value != "Option"
            || td.typeMacroExpr[1] == null || !(td.typeMacroExpr[1] is ExprTypeDecl)) return td
        return clone_type((td.typeMacroExpr[1] as ExprTypeDecl).typeexpr)
    }
    // Post-instantiation form: structural named tuple ``tuple<_has_value:bool; _value:T>``.
    if (td.baseType == Type.tTuple
            && length(td.argTypes) == 2
            && length(td.argNames) == 2
            && td.argNames[0] == "_has_value"
            && td.argNames[1] == "_value"
            && td.argTypes[0].baseType == Type.tBool) return clone_type(td.argTypes[1])
    return td
}

def private build_create_indexes_sql(st : StructurePtr; table_name : string; field_names : array<string>; var errors : das_string; var ok : bool&) : string {
    ok = true
    var stmt_count = 0
    let sql = build_string() $(var w) {
        for (ann in st.annotations) {
            if (ann.annotation.name != "sql_index") continue
            let idx_name_arg = find_annotation(ann.arguments, "name", "")
            let is_unique = find_annotation(ann.arguments, "unique", false)
            let cols = find_string_array_annotation(ann.arguments, "fields")
            if (empty(cols)) {
                errors := "[sql_index]: missing or empty `fields=` argument. Use `fields=\"ColName\"` for a single-column index or `fields=(\"A\", \"B\")` for a composite."
                ok = false
                return
            }
            for (col in cols) {
                var found = false
                for (fn in field_names) {
                    if (fn == col) {
                        found = true
                    }
                }
                if (!found) {
                    errors := "[sql_index]: field '{col}' is not a column of struct '{st.name}'. Available: {field_names}"
                    ok = false
                    return
                }
            }
            let auto_name = build_string() $(var w2) {
                w2 |> write("idx_")
                w2 |> write(table_name)
                for (col in cols) {
                    w2 |> write("_")
                    w2 |> write(col)
                }
            }
            let idx_name = empty(idx_name_arg) ? auto_name : idx_name_arg
            let kind = is_unique ? "UNIQUE INDEX" : "INDEX"
            if (stmt_count > 0) {
                w |> write("; ")
            }
            stmt_count++
            w |> write("CREATE {kind} \"{idx_name}\" ON \"{table_name}\" (")
            for (j in range(length(cols))) {
                if (j > 0) {
                    w |> write(", ")
                }
                w |> write("\"")
                w |> write(cols[j])
                w |> write("\"")
            }
            w |> write(")")
        }
    }
    return sql
}

def private extract_string_return(var fn : FunctionPtr) : string {
    if (fn == null || fn.body == null || !(fn.body is ExprBlock)) return ""
    let blk = fn.body as ExprBlock
    // Walk past pre-return concept_asserts emitted by schema_from contract checks.
    for (stmt in blk.list) {
        if (!(stmt is ExprReturn)) continue
        let ret = stmt as ExprReturn
        let val = ret.subexpr
        if (val == null || !(val is ExprConstString)) return ""
        return string((val as ExprConstString).value)
    }
    return ""
}

def private find_struct_helper_fn(name : string; st : StructurePtr) : FunctionPtr {
    var found : FunctionPtr
    for_each_function(compiling_module(), name) $(var fn) {
        if (!empty(fn.arguments)
                && fn.arguments[0]._type != null
                && fn.arguments[0]._type.structType == st) {
            found = fn
        }
    }
    return found
}

def private derive_index_name(table_name : string; sql_cols : array<string>) : string {
    // Shared between [sql_index] structure annotation and create_index call macro: idx_<table>_<col1>_<col2>… on the post-@sql_column SQL identifiers.
    return build_string() $(var w) {
        w |> write("idx_")
        w |> write(table_name)
        for (col in sql_cols) {
            w |> write("_")
            w |> write(col)
        }
    }
}

def private build_field_unique_indexes_sql(table_name : string; fields : array<FieldInfo>) : string {
    var stmt_count = 0
    return build_string() $(var w) {
        for (info in fields) {
            if (!info.is_unique || info.is_pk) continue
            if (stmt_count > 0) {
                w |> write("; ")
            }
            stmt_count++
            w |> write("CREATE UNIQUE INDEX \"uq_{table_name}_{info.col_name}\"")
            w |> write(" ON \"{table_name}\" (\"{info.col_name}\")")
        }
    }
}

[structure_macro(name=sql_index)]
class SqlIndexMacro : AstStructureAnnotation {
    //! Stackable struct-level annotation: ``[sql_index(fields="A"|("A","B"), unique=?, name=?)]``.
    //! Must appear after ``[sql_table]`` in the annotation list (it rewrites that helper's body).
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        // Reject on legacy=true structs — the [sql_table] emits no CREATE TABLE, so the index would
        // have no statement to attach to and would collide with the current struct's index on rebuild.
        if (find_struct_helper_fn("_sql_legacy_table_marker", st) != null) {
            errors := "[sql_index] on '{st.name}': not supported on [sql_table(legacy=true)] structs (they don't define schema). Drop the [sql_index] annotation; the current (non-legacy) struct's indexes are reapplied automatically by ``convert_and_rename``."
            return false
        }
        let cols = find_string_array_annotation(args, "fields")
        if (empty(cols)) {
            errors := "[sql_index]: missing or empty `fields=` argument. Use `fields=\"ColName\"` for a single-column index or `fields=(\"A\", \"B\")` for a composite."
            return false
        }
        let is_unique = find_annotation(args, "unique", false)
        let idx_name_arg = find_annotation(args, "name", "")

        // User writes daslang field names; emit @sql_column-renamed SQL identifiers in DDL.
        var sql_cols : array<string>
        sql_cols |> reserve(length(cols))
        for (col in cols) {
            var has_field = false
            var resolved = col
            for (field in st.fields) {
                if (field.name == col) {
                    has_field = true
                    let ca = find_arg(field.annotation, "sql_column")
                    if (ca is tString) {
                        resolved = ca as tString
                    }
                }
            }
            if (!has_field) {
                errors := "[sql_index]: field '{col}' is not a column of struct '{st.name}'."
                return false
            }
            sql_cols |> push(resolved)
        }

        // [sql_table] must precede [sql_index] in the annotation list (source-order rule).
        var idx_fn = find_struct_helper_fn("_sql_create_indexes_sql", st)
        if (idx_fn == null) {
            errors := "[sql_index] on struct '{st.name}': no [sql_table] declared earlier in the annotation list. Use `[sql_table(...), sql_index(...), ...]` order so the index helper is in scope."
            return false
        }
        var name_fn = find_struct_helper_fn("_sql_table_name", st)
        if (name_fn == null) {
            errors := "[sql_index] on struct '{st.name}': cannot resolve table name (no [sql_table] visible). Same source-order rule as above."
            return false
        }
        let table_name = extract_string_return(name_fn)
        if (empty(table_name)) {
            errors := "[sql_index] on struct '{st.name}': internal - _sql_table_name body did not match expected shape."
            return false
        }

        // Auto-name uses SQL (post-rename) column names so distinct schemas don't collide.
        let auto_name = derive_index_name(table_name, sql_cols)
        let idx_name = empty(idx_name_arg) ? auto_name : idx_name_arg
        let kind = is_unique ? "UNIQUE INDEX" : "INDEX"
        let this_stmt = build_string() $(var w) {
            w |> write("CREATE {kind} \"{idx_name}\" ON \"{table_name}\" (")
            for (j in range(length(sql_cols))) {
                if (j > 0) {
                    w |> write(", ")
                }
                w |> write("\"")
                w |> write(sql_cols[j])
                w |> write("\"")
            }
            w |> write(")")
        }

        // Append this statement to whatever earlier [sql_index] siblings already accumulated.
        let prev_sql = extract_string_return(idx_fn)
        let new_sql = empty(prev_sql) ? this_stmt : "{prev_sql}; {this_stmt}"
        idx_fn.body = qmacro_block() {
            return $v(new_sql)
        }
        idx_fn.body |> force_at(st.at)
        return true
    }
}

def private find_parent_table_and_pk(var st : StructurePtr; parent_name : string) : tuple<tab_name : string; pk_col : string; found : bool> {
    var found_st : StructurePtr
    if (st.name == parent_name) {
        found_st = st
    } else {
        for_each_structure(compiling_module()) $(var ps) {
            if (ps.name == parent_name) {
                found_st = ps
            }
        }
    }
    if (found_st == null) return (tab_name = "", pk_col = "", found = false)
    var tab = string(found_st.name)
    for (ann in found_st.annotations) {
        if (ann.annotation.name == "sql_table") {
            tab = find_annotation(ann.arguments, "name", tab)
        }
    }
    for (pf in found_st.fields) {
        if (find_annotation(pf.annotation, "sql_primary_key", false)) {
            // FK references the SQL identifier (post @sql_column rename), not the daslang field name.
            var pk_col = string(pf.name)
            let pk_anno = find_arg(pf.annotation, "sql_column")
            if (pk_anno is tString) {
                pk_col = pk_anno as tString
            }
            return (tab_name = tab, pk_col = pk_col, found = true)
        }
    }
    return (tab_name = tab, pk_col = "", found = false)
}

[structure_macro(name=sql_table)]
class SqlTableMacro : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {   // nolint:STYLE037,STYLE038 - flat registration table, one emit call per generated [sql_table] helper
        let table_name = find_annotation(args, "name", string(st.name))
        // legacy=true: emit read-side helpers only. Writes fail as unresolved-overload.
        let is_legacy = find_annotation(args, "legacy", false)

        var schema_assert_exprs : array<ExpressionPtr>
        let schema_from_arg = find_arg(args, "schema_from")
        if (schema_from_arg is tString) {
            let raw_path = schema_from_arg as tString
            if (empty(raw_path)) {
                errors := "[sql_table]: schema_from=\"\" is invalid - provide a non-empty path to a SQLite database file."
                return false
            }
            if (!apply_schema_from(st, raw_path, table_name, schema_assert_exprs, errors)) return false
        }

        var fields : array<FieldInfo>
        fields |> reserve(length(st.fields))
        for (field in st.fields) {
            let fname = string(field.name)
            let is_pk = find_annotation(field.annotation, "sql_primary_key", false)
            let is_unique = find_annotation(field.annotation, "sql_unique", false)
            let is_optional = is_option_field_type(field._type)

            // @sql_computed expression
            let computed_sql = find_annotation(field.annotation, "sql_computed", "")
            let is_computed = !empty(computed_sql)
            let is_stored = find_annotation(field.annotation, "sql_stored", false)

            // @sql_json / @sql_blob — non-scalar column storage with an emitted sql_bind/sql_extract pair.
            let is_json = find_annotation(field.annotation, "sql_json", false)
            let is_blob = find_annotation(field.annotation, "sql_blob", false)

            // Defaults — native daslang field init OR @sql_default_fn (mutually exclusive)
            let sql_default_fn = find_annotation(field.annotation, "sql_default_fn", "")

            // Foreign key — @sql_references + optional @sql_on_delete / @sql_on_update
            let references = find_annotation(field.annotation, "sql_references", "")
            let on_delete = find_annotation(field.annotation, "sql_on_delete", "no_action")
            let on_update = find_annotation(field.annotation, "sql_on_update", "no_action")
            let has_on_delete_attr = !empty(find_annotation(field.annotation, "sql_on_delete", ""))
            let has_on_update_attr = !empty(find_annotation(field.annotation, "sql_on_update", ""))

            // @sql_column rename. find_arg distinguishes absent from `= ""` (find_annotation can't).
            var col_name = fname
            let col_anno = find_arg(field.annotation, "sql_column")
            if (col_anno is tString) {
                let cv = col_anno as tString
                if (empty(cv)) {
                    errors := "[sql_table]: field '{fname}' @sql_column = \"\" is invalid - provide a non-empty SQL column name."
                    return false
                }
                if (find(cv, "\"") >= 0 || find(cv, "\\") >= 0) {
                    errors := "[sql_table]: field '{fname}' @sql_column=\"{cv}\" - embedded `\"` / `\\` are rejected (SQL identifier hygiene). Use a plain identifier."
                    return false
                }
                col_name = cv
            }

            // ---- validation ----
            if (is_pk && is_optional) {
                errors := "[sql_table]: @sql_primary_key on field '{fname}' must not be Option<T> - primary keys cannot be NULL in SQL. Use a non-optional type."
                return false
            }
            if (is_computed && is_pk) {
                errors := "[sql_table]: field '{fname}' cannot be both @sql_primary_key and @sql_computed - SQLite rejects PRIMARY KEY on a generated column."
                return false
            }
            if (is_computed && is_unique) {
                errors := "[sql_table]: field '{fname}' cannot be both @sql_unique and @sql_computed - declare an explicit [sql_index(fields=\"{fname}\", unique=true)] if the provider supports indexing that generated expression."
                return false
            }
            if (is_computed && field.init != null) {
                errors := "[sql_table]: field '{fname}' is @sql_computed; remove the daslang field initializer (the SQL expression provides the value)."
                return false
            }
            if (is_computed && !empty(sql_default_fn)) {
                errors := "[sql_table]: field '{fname}' is @sql_computed; @sql_default_fn does not apply (computed columns derive their value from the expression)."
                return false
            }
            if (is_json && is_blob) {
                errors := "[sql_table]: field '{fname}' has both @sql_json and @sql_blob - pick one. Use @sql_json for queryable, schema-tolerant TEXT storage; use @sql_blob for opaque, faster binary storage."
                return false
            }
            let json_or_blob_kind = is_json ? "json" : "blob"
            if ((is_json || is_blob) && is_pk) {
                errors := "[sql_table]: field '{fname}' is @sql_{json_or_blob_kind} - non-scalar columns cannot be a PRIMARY KEY."
                return false
            }
            if ((is_json || is_blob) && is_computed) {
                errors := "[sql_table]: field '{fname}' is @sql_computed; @sql_{json_or_blob_kind} does not apply (computed columns derive their value from the SQL expression, not from a daslang adapter)."
                return false
            }
            if (field.init != null && !empty(sql_default_fn)) {
                errors := "[sql_table]: field '{fname}' has both a daslang field initializer and @sql_default_fn - pick one. Use the field init for literal defaults, @sql_default_fn for SQL built-ins like CURRENT_TIMESTAMP."
                return false
            }
            if (!empty(sql_default_fn)) {
                var fn_ok = false
                for (allowed in SQL_DEFAULT_FN_WHITELIST) {
                    if (allowed == sql_default_fn) {
                        fn_ok = true
                    }
                }
                if (!fn_ok) {
                    errors := "[sql_table]: field '{fname}' @sql_default_fn=\"{sql_default_fn}\" is not in the supported set (CURRENT_TIMESTAMP / CURRENT_DATE / CURRENT_TIME). Use raw SQL via `db |> exec(...)` for other expressions."
                    return false
                }
            }
            if ((has_on_delete_attr || has_on_update_attr) && empty(references)) {
                errors := "[sql_table]: field '{fname}' has @sql_on_delete / @sql_on_update without @sql_references - these annotations only apply to a foreign-key relationship."
                return false
            }
            if (!empty(references)) {
                var od_ok = false
                var ou_ok = false
                for (allowed in SQL_FK_ACTION_WHITELIST) {
                    if (allowed == on_delete) {
                        od_ok = true
                    }
                    if (allowed == on_update) {
                        ou_ok = true
                    }
                }
                if (!od_ok) {
                    errors := "[sql_table]: field '{fname}' @sql_on_delete=\"{on_delete}\" must be one of cascade / set_null / set_default / restrict / no_action."
                    return false
                }
                if (!ou_ok) {
                    errors := "[sql_table]: field '{fname}' @sql_on_update=\"{on_update}\" must be one of cascade / set_null / set_default / restrict / no_action."
                    return false
                }
                if (on_delete == "set_null" && !is_optional) {
                    errors := "[sql_table]: field '{fname}' @sql_on_delete=\"set_null\" requires column type Option<T> so the column can hold NULL. Either change the column type or pick a different action (cascade / restrict / no_action)."
                    return false
                }
                if (on_delete == "set_default" && field.init == null && empty(sql_default_fn)) {
                    errors := "[sql_table]: field '{fname}' @sql_on_delete=\"set_default\" requires a daslang field initializer (`{fname} : T = <literal>`) or @sql_default_fn. Otherwise SET DEFAULT has nothing to set."
                    return false
                }
            }

            // ---- DDL fragment construction ----
            var default_clause = ""
            if (!empty(sql_default_fn)) {
                default_clause = " DEFAULT {sql_default_fn}"
            } elif (field.init != null) {
                default_clause = literal_init_to_default_clause(field.init)
            }

            var references_clause = ""
            if (!empty(references)) {
                let pk_lookup = find_parent_table_and_pk(st, references)
                if (!pk_lookup.found) {
                    if (empty(pk_lookup.tab_name)) {
                        errors := "[sql_table]: field '{fname}' @sql_references=\"{references}\" - no struct named '{references}' is visible in this module."
                    } else {
                        errors := "[sql_table]: field '{fname}' @sql_references=\"{references}\" - parent struct '{references}' has no @sql_primary_key field; FK target column cannot be resolved."
                    }
                    return false
                }
                references_clause = " REFERENCES \"{pk_lookup.tab_name}\"(\"{pk_lookup.pk_col}\") ON DELETE {fk_action_to_sql(on_delete)} ON UPDATE {fk_action_to_sql(on_update)}"
            }

            fields |> push(FieldInfo(
                name = fname,
                col_name = col_name,
                is_pk = is_pk,
                is_int_pk = is_pk && (field._type.baseType == Type.tInt || field._type.baseType == Type.tInt64),
                is_optional = is_optional,
                is_unique = is_unique,
                is_computed = is_computed,
                is_stored = is_stored,
                is_json = is_json,
                is_blob = is_blob,
                computed_sql = computed_sql,
                default_clause = default_clause,
                references_clause = references_clause
            ))
        }

        // ---- @sql_column collision detection ----
        {
            var seen_cols : table<string; string>  // sql_col -> first daslang field it came from
            for (info in fields) {
                if (key_exists(seen_cols, info.col_name)) {
                    let first_owner = seen_cols[info.col_name]
                    errors := "[sql_table]: SQL column name '{info.col_name}' is claimed by both field '{first_owner}' and field '{info.name}'. @sql_column renames must produce a unique column name across the struct."
                    return false
                }
                seen_cols |> insert(info.col_name, info.name)
            }
        }

        // ---- @sql_json / @sql_blob adapter codegen (shared with [sql_view]) ----
        if (!emit_json_blob_adapters_for_struct(st, fields, "[sql_table]", errors)) return false

        // Read-side helpers are always emitted; write-side helpers are skipped for legacy=true structs
        // so the natural overload-resolution error fires when a caller tries to write to one.
        // schema_assert_exprs ride along inside _sql_table_name's body as typecheck-time concept_asserts.
        if (is_legacy) {
            // Marker so [sql_index] can reject legacy structs with a precise error.
            var fn_legacy_marker = qmacro_function("_sql_legacy_table_marker") $(typ : $t(st)) : bool {
                return true
            }
            fn_legacy_marker.flags.generated = true
            fn_legacy_marker.body |> force_at(st.at)
            compiling_module() |> add_function(fn_legacy_marker)
        }
        var fn_table_name = make_table_name_fn(st, table_name, schema_assert_exprs)
        compiling_module() |> add_function(fn_table_name)
        // DROP is read-side enough — devs may legitimately drop a legacy table during migration cleanup.
        // 2-arg form must be added BEFORE the 1-arg form so find_struct_helper_fn returns the 1-arg form.
        var fn_drop_named = make_drop_sql_named_fn(st)
        compiling_module() |> add_function(fn_drop_named)
        var fn_drop = make_drop_sql_fn(st, table_name)
        compiling_module() |> add_function(fn_drop)
        // SELECT and read-row stay so select_from(type<S>) works on legacy structs.
        var fn_select_all_named = make_select_all_sql_named_fn(st, fields)
        compiling_module() |> add_function(fn_select_all_named)
        var fn_select_all = make_select_all_sql_fn(st, table_name, fields)
        compiling_module() |> add_function(fn_select_all)
        var fn_read = make_read_row_fn(st, fields)
        compiling_module() |> add_generic(fn_read)
        var fn_col_info = make_column_info_fn(st, fields)
        compiling_module() |> add_function(fn_col_info)

        // ---- Write-side helpers (skipped for [sql_table(legacy=true)]) ----
        if (!is_legacy) {
            var fn_create_named = make_create_table_sql_named_fn(st, fields)
            compiling_module() |> add_function(fn_create_named)
            var fn_create = make_create_table_sql_fn(st, table_name, fields)
            compiling_module() |> add_function(fn_create)
            // 2-arg form must be added BEFORE the 1-arg form so find_struct_helper_fn (used by [sql_index]
            // finish-pass) returns the 1-arg form whose body owns the burned-in SQL.
            var fn_indexes_named = make_create_indexes_sql_named_fn(st, table_name)
            compiling_module() |> add_function(fn_indexes_named)
            // Field-level @sql_unique constraints start the provider-neutral
            // CREATE INDEX list; [sql_index] siblings append to the same helper.
            let unique_indexes_sql = build_field_unique_indexes_sql(table_name, fields)
            var fn_indexes = make_create_indexes_sql_fn(st, unique_indexes_sql)
            compiling_module() |> add_function(fn_indexes)
            var fn_ins_pk_named = make_insert_sql_named_fn(st, fields, true)
            compiling_module() |> add_function(fn_ins_pk_named)
            var fn_ins_pk = make_insert_sql_fn(st, table_name, fields, true)
            compiling_module() |> add_function(fn_ins_pk)
            var fn_ins_nopk_named = make_insert_sql_named_fn(st, fields, false)
            compiling_module() |> add_function(fn_ins_nopk_named)
            var fn_ins_nopk = make_insert_sql_fn(st, table_name, fields, false)
            compiling_module() |> add_function(fn_ins_nopk)
            var fn_pk_unset = make_pk_is_unset_fn(st, fields)
            compiling_module() |> add_function(fn_pk_unset)
            var fn_bind = make_bind_row_fn(st, fields)
            compiling_module() |> add_generic(fn_bind)
            var fn_upd_pk_named = make_update_by_pk_sql_named_fn(st, fields)
            compiling_module() |> add_function(fn_upd_pk_named)
            var fn_upd_pk = make_update_by_pk_sql_fn(st, table_name, fields)
            compiling_module() |> add_function(fn_upd_pk)
            var fn_del_pk_named = make_delete_by_pk_sql_named_fn(st, fields)
            compiling_module() |> add_function(fn_del_pk_named)
            var fn_del_pk = make_delete_by_pk_sql_fn(st, table_name, fields)
            compiling_module() |> add_function(fn_del_pk)
            var fn_bind_upd = make_bind_row_for_update_fn(st, fields)
            compiling_module() |> add_generic(fn_bind_upd)
            var fn_bind_pk = make_bind_row_pk_only_fn(st, fields)
            compiling_module() |> add_generic(fn_bind_pk)
        }

        return true
    }

    def apply_schema_from(var st : StructurePtr;   // nolint:STYLE037,STYLE038 - one arm per SQL affinity; the arms share the covered-column bookkeeping
                          raw_path : string;
                          tbl_name : string;
                          var assert_exprs : array<ExpressionPtr>&;
                          var errors : das_string) : bool {
        // Resolve + read the .db at compile time through the provider's `readTableSchema`
        // hook (read-only — a bad path won't create a stray empty file). Annotation time
        // has no runner to dispatch on, so exactly one registered provider must supply the hook.
        let resolved = resolve_schema_from_path(raw_path)
        var pidx = -1
        for (i in range(sql_provider_count())) {
            if (sql_provider_at(i).readTableSchema != null) {
                if (pidx >= 0) {
                    errors := "[sql_table]: schema_from=\"{raw_path}\" - ambiguous: multiple registered providers support schema introspection ({sql_provider_names()})."
                    return false
                }
                pidx = i
            }
        }
        if (pidx < 0) {
            errors := "[sql_table]: schema_from=\"{raw_path}\" - no registered SQL provider supports schema introspection (registered: {sql_provider_names()})."
            return false
        }
        let provider = sql_provider_at(pidx)
        var cols_result <- invoke(provider.readTableSchema, resolved, tbl_name)
        if (cols_result |> is_err) {
            errors := "[sql_table]: schema_from=\"{raw_path}\" - {cols_result |> unwrap_err}"
            return false
        }
        let cols <- cols_result._value
        if (empty(cols)) {
            errors := "[sql_table]: schema_from=\"{raw_path}\" - table \"{tbl_name}\" has no columns or doesn't exist in this database."
            return false
        }
        // Track which schema cols are covered by hand-declared fields.
        var covered : array<bool>
        covered |> resize(length(cols))
        assert_exprs |> reserve(length(st.fields))
        // Validate hand-declared fields: name, PK, nullability, and emit SqlType concept_asserts.
        for (ufi in range(length(st.fields))) {
            let ufname = string(st.fields[ufi].name)
            var matched_col_idx = -1
            for (ci in range(length(cols))) {
                if (cols[ci].name == ufname) {
                    matched_col_idx = ci
                    break
                }
            }
            if (matched_col_idx < 0) {
                errors := "[sql_table]: schema_from=\"{raw_path}\" - hand-declared field '{ufname}' not found in table \"{tbl_name}\". Either remove the field, rename it, or update the .db schema."
                return false
            }
            covered[matched_col_idx] = true
            let col_name_s = cols[matched_col_idx].name
            let col_not_null = cols[matched_col_idx].not_null
            let col_pk = cols[matched_col_idx].pk
            // Computed contract - GENERATED columns reject NOT NULL/PK in DDL, so a computed
            // pair skips those checks below (SqlType still applies: reads bind the value).
            let col_generated = cols[matched_col_idx].generated
            let user_computed = find_arg(st.fields[ufi].annotation, "sql_computed")
            let user_is_computed = user_computed is tString && !empty(user_computed as tString)
            if (user_is_computed != col_generated) {
                if (user_is_computed) {
                    errors := "[sql_table]: schema_from=\"{raw_path}\" - field '{ufname}' is @sql_computed but column \"{col_name_s}\" in \"{tbl_name}\" is not GENERATED."
                } else {
                    errors := "[sql_table]: schema_from=\"{raw_path}\" - column \"{col_name_s}\" in \"{tbl_name}\" is GENERATED; declare field '{ufname}' with @sql_computed (the DB does not store the expression - copy it from your DDL)."
                }
                return false
            }
            // PK contract.
            let user_is_pk = find_annotation(st.fields[ufi].annotation, "sql_primary_key", false)
            if (user_is_pk != col_pk) {
                if (user_is_pk) {
                    errors := "[sql_table]: schema_from=\"{raw_path}\" - field '{ufname}' is @sql_primary_key but column \"{col_name_s}\" in \"{tbl_name}\" is not PRIMARY KEY."
                } else {
                    errors := "[sql_table]: schema_from=\"{raw_path}\" - column \"{col_name_s}\" in \"{tbl_name}\" is PRIMARY KEY but field '{ufname}' is missing @sql_primary_key."
                }
                return false
            }
            // Nullability contract — Option<T> ↔ NULL allowed.
            // SQLite quirk: INTEGER PRIMARY KEY reports notnull=0 in pragma — treat PK as NOT NULL anyway.
            let user_is_nullable = is_option_field_type(st.fields[ufi]._type)
            let col_is_nullable = !col_not_null && !col_pk
            if (user_is_nullable != col_is_nullable) {
                if (user_is_nullable) {
                    errors := "[sql_table]: schema_from=\"{raw_path}\" - field '{ufname}' is Option<T> but column \"{col_name_s}\" in \"{tbl_name}\" is NOT NULL. Drop the Option<> wrapper."
                } else {
                    errors := "[sql_table]: schema_from=\"{raw_path}\" - column \"{col_name_s}\" in \"{tbl_name}\" allows NULL but field '{ufname}' is non-optional. Use Option<T>."
                }
                return false
            }
            // SqlType contract: static_if peeks at `_::sql_bind`'s return via `default<typedecl(...)>`.
            // Generic `sql_bind(Option<T>)` fallback recurses to inner T's adapter via `_::`.
            let expected_sql_type = cols[matched_col_idx].data_type
            let storage_label = invoke(provider.sqlTypeFor, expected_sql_type)
            var expected_native = ""
            if (expected_sql_type == SqlType.Integer) {
                expected_native = "int64"
            } elif (expected_sql_type == SqlType.Real) {
                expected_native = "double"
            } elif (expected_sql_type == SqlType.Text) {
                expected_native = "string"
            } else {
                expected_native = "array<uint8>"
            }
            let msg = "[sql_table] schema_from: field '{ufname}' SqlType does not match column \"{col_name_s}\" in \"{tbl_name}\" (expected {storage_label})."
            assert_exprs |> push(qmacro_expr(${
                static_if (typeinfo stripped_typename(default<typedecl(_::sql_bind(default<$t(st.fields[ufi]._type)>))>) != $v(expected_native)) {
                    concept_assert(false, $v(msg));
                }
            }))
        }
        // Synthesize fields for uncovered schema columns.
        for (is_covered, col in covered, cols) {
            // GENERATED columns get no synthesized field: the DB doesn't store the generation
            // expression, so a faithful @sql_computed can't be emitted, and a plain field would
            // break INSERTs. Hand-declare the field with @sql_computed to read it through linq.
            if (is_covered || col.generated) continue
            let sql_type = col.data_type
            // SQLite quirk: INTEGER PRIMARY KEY columns report notnull=0 in pragma_table_info because
            // `INTEGER PRIMARY KEY` aliases rowid (which has its own NULL handling). Treat PK as NOT NULL.
            let effective_not_null = col.not_null || col.pk
            var td = affinity_to_typedecl(sql_type, effective_not_null)
            if (td == null) {
                errors := "[sql_table]: schema_from=\"{raw_path}\" - column \"{col.name}\" declared type \"{col.decl_type}\" couldn't be mapped to a daslang type."
                return false
            }
            let fi = st |> add_structure_field(col.name, td, default<ExpressionPtr>)
            if (col.pk) {
                st.fields[fi].annotation |> add_annotation_argument("sql_primary_key", true)
            }
        }
        return true
    }

    def make_table_name_fn(var st : StructurePtr; table_name : string; var assert_exprs : array<ExpressionPtr>) : FunctionPtr {
        var fn = qmacro_function("_sql_table_name") $(typ : $t(st)) : string {
            $b(assert_exprs)
            return $v(table_name)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_drop_sql_fn(var st : StructurePtr; table_name : string) : FunctionPtr {
        // 1-arg wrapper forwards via `default<T>` so `typ` stays unread (avoids "type expression result is used").
        var fn = qmacro_function("_sql_drop_table_if_exists_sql") $(typ : $t(st)) : string {
            return _sql_drop_table_if_exists_sql(default<$t(st)>, $v(table_name))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_drop_sql_named_fn(var st : StructurePtr) : FunctionPtr {
        // 2-arg form: runtime ``name`` lets a caller drop a staging/aliased table identifier
        // (used by struct_convert rebuild). [unused_argument(typ)] because daslang dispatches on T,
        // but the body needs only ``name``.
        var fn = qmacro_function("_sql_drop_table_if_exists_sql") $(typ : $t(st); name : string) : string {
            return "DROP TABLE IF EXISTS " + sql_quote_id(name)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_create_indexes_sql_fn(var st : StructurePtr; indexes_sql : string) : FunctionPtr {
        //! Per-struct helper returning ``;``-joined CREATE INDEX statements for every
        //! ``[sql_index(...)]`` (or ``""`` — ``try_create_table`` short-circuits on empty).
        var fn = qmacro_function("_sql_create_indexes_sql") $(typ : $t(st)) : string {
            return $v(indexes_sql)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_create_indexes_sql_named_fn(var st : StructurePtr; table_name : string) : FunctionPtr {
        //! 2-arg form: rewrites the 1-arg SQL onto ``name``. Auto-named ``idx_<table>_<col>`` follows; user-explicit
        //! ``[sql_index(name=...)]`` is preserved.
        let orig_quoted = "\"{table_name}\""
        let orig_idx_prefix = "\"idx_{table_name}_"
        let orig_unique_prefix = "\"uq_{table_name}_"
        var fn = qmacro_function("_sql_create_indexes_sql") $(typ : $t(st); name : string) : string {
            let escaped_name = name |> replace("\"", "\"\"")
            return (_sql_create_indexes_sql(default<$t(st)>)
                |> replace($v(orig_quoted), "\"" + escaped_name + "\"")
                |> replace($v(orig_idx_prefix), "\"idx_" + escaped_name + "_")
                |> replace($v(orig_unique_prefix), "\"uq_" + escaped_name + "_"))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_create_table_sql_fn(var st : StructurePtr; table_name : string; fields : array<FieldInfo>) : FunctionPtr {
        // 1-arg form: delegates to the 2-arg form with the struct's own [sql_table(name=...)] identifier.
        // See make_drop_sql_fn for why we construct default<$t(st)> rather than forwarding typ.
        var fn = qmacro_function("_sql_create_table_sql") $(typ : $t(st)) : string {
            return _sql_create_table_sql(default<$t(st)>, $v(table_name))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_create_table_sql_named_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        // 2-arg form: composes CREATE TABLE DDL against runtime ``name``. Per-field SQL is baked at macro time
        // (column name, type, constraints), but the table identifier comes from the runtime parameter so a
        // struct_convert rebuild can land the new schema in a staging table.
        var write_exprs : array<ExpressionPtr>
        write_exprs |> reserve(3 * length(fields) + 4)

        // CREATE TABLE "<runtime name>" ( — name routed through sql_quote_id for `"` escaping.
        write_exprs |> push(qmacro_expr(${ writer |> write("CREATE TABLE "); }))
        write_exprs |> push(qmacro_expr(${ writer |> write(sql_quote_id(name)); }))
        write_exprs |> push(qmacro_expr(${ writer |> write("("); }))

        for (i in range(length(fields))) {
            let info = fields[i]
            let prefix = i == 0 ? "\"{info.col_name}\" " : ", \"{info.col_name}\" "
            // computed columns reject NOT NULL/DEFAULT/UNIQUE/FK; non-computed get them in DDL order.
            var suffix = ""
            if (info.is_computed) {
                let storage = info.is_stored ? "STORED" : "VIRTUAL"
                suffix = " GENERATED ALWAYS AS ({info.computed_sql}) {storage}"
            } else {
                if (info.is_pk) {
                    suffix = " PRIMARY KEY"
                } elif (!info.is_optional) {
                    suffix = " NOT NULL"
                }
                suffix += info.default_clause
                suffix += info.references_clause
            }
            write_exprs |> push(qmacro_expr(${ writer |> write($v(prefix)); }))
            // @sql_json/@sql_blob pin storage to TEXT/BLOB; sql_storage_type_for would fail on struct/tuple T.
            if (info.is_json) {
                write_exprs |> push(qmacro_expr(${ writer |> write("TEXT"); }))
            } elif (info.is_blob) {
                write_exprs |> push(qmacro_expr(${ writer |> write("BLOB"); }))
            } else {
                write_exprs |> push(qmacro_expr(${ writer |> write(sql_storage_type_for(type<$t(st.fields[i]._type)>)); }))
            }
            write_exprs |> push(qmacro_expr(${ writer |> write($v(suffix)); }))
        }
        write_exprs |> push(qmacro_expr(${ writer |> write(")"); }))

        var fn = qmacro_function("_sql_create_table_sql") $(typ : $t(st); name : string) : string {
            return build_string() $(var writer) {
                $b(write_exprs)
            }
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_column_info_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        //! Per-struct helper backing the user-facing ``column_info(type<T>)`` generic.
        //! ``is_nullable`` follows the daslang-side meaning (``Option<T>``-derived, not NOT-NULL).
        var push_exprs : array<ExpressionPtr>
        push_exprs |> reserve(length(fields))
        for (info, i in fields, count()) {
            let name = info.col_name
            let is_pk = info.is_pk
            let is_nullable = info.is_optional
            // Strip leading " DEFAULT " so default_expr matches PRAGMA table_info's dflt_value column.
            var default_expr = ""
            if (length(info.default_clause) > length(" DEFAULT ")) {
                default_expr = info.default_clause |> slice(length(" DEFAULT "))
            }
            let is_computed = info.is_computed
            if (info.is_json) {
                push_exprs |> push(qmacro_expr(${ result |> push(ColumnInfo(
                    name = $v(name),
                    data_type = SqlType.Text,
                    is_pk = $v(is_pk),
                    is_nullable = $v(is_nullable),
                    default_expr = $v(default_expr),
                    is_computed = $v(is_computed))); }))
            } elif (info.is_blob) {
                push_exprs |> push(qmacro_expr(${ result |> push(ColumnInfo(
                    name = $v(name),
                    data_type = SqlType.Blob,
                    is_pk = $v(is_pk),
                    is_nullable = $v(is_nullable),
                    default_expr = $v(default_expr),
                    is_computed = $v(is_computed))); }))
            } else {
                push_exprs |> push(qmacro_expr(${ result |> push(ColumnInfo(
                    name = $v(name),
                    data_type = sql_storage_enum_for(type<$t(st.fields[i]._type)>),
                    is_pk = $v(is_pk),
                    is_nullable = $v(is_nullable),
                    default_expr = $v(default_expr),
                    is_computed = $v(is_computed))); }))
            }
        }
        var fn = qmacro_function("_sql_column_info") $(typ : $t(st)) : array<ColumnInfo> {
            var result : array<ColumnInfo>
            $b(push_exprs)
            return <- result
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_insert_sql_fn(var st : StructurePtr; table_name : string; fields : array<FieldInfo>; include_pk : bool) : FunctionPtr {
        // 1-arg form: delegates to the same-named 2-arg form. The 1-arg call site historically takes
        // either an instance OR `type<T>`; the wrapper must not READ typ or the type-witness path errors.
        let helper_name = include_pk ? "_sql_insert_with_pk_sql" : "_sql_insert_no_pk_sql"
        var fn = qmacro_function(helper_name) $(typ : $t(st)) : string {
            return $c(helper_name)(default<$t(st)>, $v(table_name))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_insert_sql_named_fn(var st : StructurePtr; fields : array<FieldInfo>; include_pk : bool) : FunctionPtr {
        // 2-arg form: composes the INSERT with runtime ``name``. The cols-list / placeholders chunk is
        // burned at macro time; the table identifier is the runtime parameter so a struct_convert rebuild
        // can target a staging table.
        let helper_name = include_pk ? "_sql_insert_with_pk_sql" : "_sql_insert_no_pk_sql"
        let cols = build_string() $(var w) {
            var first = true
            for (info in fields) {
                if (info.is_computed || (!include_pk && info.is_pk)) continue
                if (!first) {
                    w |> write(", ")
                }
                w |> write("\"")
                w |> write(info.col_name)
                w |> write("\"")
                first = false
            }
        }
        let placeholders = build_string() $(var w) {
            var first = true
            for (info in fields) {
                if (info.is_computed || (!include_pk && info.is_pk)) continue
                if (!first) {
                    w |> write(", ")
                }
                w |> write("?")
                first = false
            }
        }
        // Two-tail SQL: with explicit column list, or DEFAULT VALUES if every column is computed/excluded.
        // name routed through sql_quote_id for `"` escaping; suffix carries the post-identifier tail only.
        let suffix = (empty(cols)
            ? " DEFAULT VALUES"
            : " ({cols}) VALUES ({placeholders})")
        var fn = qmacro_function(helper_name) $(typ : $t(st); name : string) : string {
            return "INSERT INTO " + sql_quote_id(name) + $v(suffix)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_pk_is_unset_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        var pk_field_index = -1
        for (i in range(length(fields))) {
            if (fields[i].is_int_pk) {
                pk_field_index = i
                break
            }
        }
        if (pk_field_index >= 0) {
            let pk_name = fields[pk_field_index].name
            var fn = qmacro_function("_sql_pk_is_unset") $(row : $t(st)) : bool {
                return row.$f(pk_name) == default<$t(st.fields[pk_field_index]._type)>
            }
            fn.flags.generated = true
            fn.body |> force_at(st.at)
            return <- fn
        } else {
            var fn = qmacro_function("_sql_pk_is_unset") $(row : $t(st)) : bool {
                return false
            }
            fn.flags.generated = true
            fn.body |> force_at(st.at)
            return <- fn
        }
    }

    def make_bind_row_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        // Bind index counts bindable (non-computed) columns, NOT field position.
        var with_pk_exprs : array<ExpressionPtr>
        var no_pk_exprs : array<ExpressionPtr>
        with_pk_exprs |> reserve(length(fields))
        no_pk_exprs |> reserve(length(fields))

        var with_pk_idx = 0
        for (info in fields) {
            if (info.is_computed) continue
            with_pk_idx++
            let wi = with_pk_idx
            let field_name = info.name
            with_pk_exprs |> push(qmacro_expr(${
                sql_bind_to_stmt(stmt, $v(wi), row.$f(field_name));
            }))
        }

        var no_pk_idx = 0
        for (info in fields) {
            if (info.is_pk || info.is_computed) continue
            no_pk_idx++
            let field_name = info.name
            no_pk_exprs |> push(qmacro_expr(${
                sql_bind_to_stmt(stmt, $v(no_pk_idx), row.$f(field_name));
            }))
        }

        // stmt is untyped (generic): monomorphizes per provider stmt type; the interior
        // sql_bind_to_stmt overloads dispatch on whichever concrete stmt instantiated it.
        var fn = qmacro_function("_sql_bind_row") $(var stmt; row : $t(st); include_pk : bool) : void {
            if (include_pk) {
                $b(with_pk_exprs)
            } else {
                $b(no_pk_exprs)
            }
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_select_all_sql_fn(var st : StructurePtr; table_name : string; fields : array<FieldInfo>) : FunctionPtr {
        // 1-arg form: delegates to the 2-arg form. See make_drop_sql_fn for the default<$t(st)> rationale.
        var fn = qmacro_function("_sql_select_all_sql") $(typ : $t(st)) : string {
            return _sql_select_all_sql(default<$t(st)>, $v(table_name))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_select_all_sql_named_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        // 2-arg form: column list is baked at macro time; table identifier is the runtime parameter.
        let col_list = build_string() $(var w) {
            for (i in range(length(fields))) {
                if (i > 0) {
                    w |> write(", ")
                }
                w |> write("\"")
                w |> write(fields[i].col_name)
                w |> write("\"")
            }
        }
        let prefix = "SELECT {col_list} FROM "
        var fn = qmacro_function("_sql_select_all_sql") $(typ : $t(st); name : string) : string {
            return $v(prefix) + sql_quote_id(name)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_read_row_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        var assign_exprs : array<ExpressionPtr>
        assign_exprs |> reserve(length(fields))

        for (i in range(length(fields))) {
            let info = fields[i]
            let col_idx = i
            let field_name = info.name
            assign_exprs |> push(qmacro_expr(${
                static_if (typeinfo can_copy(default<$t(st.fields[i]._type)>)) {
                    row.$f(field_name) = read_via_adapter(stmt, $v(col_idx), type<$t(st.fields[i]._type)>);
                } else {
                    var field_tmp <- clone_to_move(read_via_adapter(stmt, $v(col_idx), type<$t(st.fields[i]._type)>));
                    row.$f(field_name) <- field_tmp;
                }
            }))
        }

        // Explicit default<>: Option<T> fields don't satisfy strict_smart_pointers' uninit check otherwise.
        // stmt untyped (generic over provider stmt type) — same rationale as make_bind_row_fn.
        var fn = qmacro_function("_sql_read_row") $(stmt; typ : $t(st)) : $t(st) {
            var row = default<$t(st)>
            $b(assign_exprs)
            return <- row
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def find_pk_field(fields : array<FieldInfo>) : int {
        for (i in range(length(fields))) {
            if (fields[i].is_pk) return i
        }
        return -1
    }

    def make_update_by_pk_sql_fn(var st : StructurePtr; table_name : string; fields : array<FieldInfo>) : FunctionPtr {
        // 1-arg form: delegates to the 2-arg form. See make_drop_sql_fn for the default<$t(st)> rationale.
        // The PK-less / no-SET-columns runtime-panic stubs ride inside the 2-arg form.
        var fn = qmacro_function("_sql_update_by_pk_sql") $(typ : $t(st)) : string {
            return _sql_update_by_pk_sql(default<$t(st)>, $v(table_name))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_update_by_pk_sql_named_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        // 2-arg form: composes the UPDATE with runtime ``name``. SET-clause is baked at macro time.
        // PK-less structs get a runtime-panic stub (concept_assert would fire on every [sql_table] decl).
        let pk_idx = find_pk_field(fields)
        if (pk_idx < 0) {
            let st_name = string(st.name)
            let msg = "[sql_table] {st_name}: update(row) / delete_(row) / delete_by_id are unavailable - no @sql_primary_key field. Use the macro forms `_sql_update(type<{st_name}>, where, set)` / `_sql_delete(type<{st_name}>, where)` instead."
            var fn = qmacro_function("_sql_update_by_pk_sql") $(typ : $t(st); name : string) : string {
                panic($v(msg))
                return ""
            }
            fn.flags.generated = true
            fn.body |> force_at(st.at)
            return <- fn
        }
        let pk_col = fields[pk_idx].col_name
        let set_clause = build_string() $(var w) {
            var first = true
            for (info in fields) {
                if (info.is_pk || info.is_computed) continue
                if (!first) {
                    w |> write(", ")
                }
                w |> write("\"")
                w |> write(info.col_name)
                w |> write("\" = ?")
                first = false
            }
        }
        // No SET columns -> `UPDATE T SET WHERE pk=?` is invalid SQL; emit a runtime-panic stub.
        if (empty(set_clause)) {
            let st_name = string(st.name)
            let msg = "[sql_table] {st_name}: update(row) has nothing to SET - every non-PK column is missing or @sql_computed. Add at least one writable non-PK column, or use raw `exec`."
            var fn = qmacro_function("_sql_update_by_pk_sql") $(typ : $t(st); name : string) : string {
                panic($v(msg))
                return ""
            }
            fn.flags.generated = true
            fn.body |> force_at(st.at)
            return <- fn
        }
        let prefix = "UPDATE "
        let suffix = " SET {set_clause} WHERE \"{pk_col}\" = ?"
        var fn = qmacro_function("_sql_update_by_pk_sql") $(typ : $t(st); name : string) : string {
            return $v(prefix) + sql_quote_id(name) + $v(suffix)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_delete_by_pk_sql_fn(var st : StructurePtr; table_name : string; fields : array<FieldInfo>) : FunctionPtr {
        // 1-arg form: delegates to the 2-arg form. See make_drop_sql_fn for the default<$t(st)> rationale.
        var fn = qmacro_function("_sql_delete_by_pk_sql") $(typ : $t(st)) : string {
            return _sql_delete_by_pk_sql(default<$t(st)>, $v(table_name))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_delete_by_pk_sql_named_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        // 2-arg form: composes the DELETE with runtime ``name``. WHERE-clause + PK column baked at macro time.
        let pk_idx = find_pk_field(fields)
        if (pk_idx < 0) {
            let st_name = string(st.name)
            let msg = "[sql_table] {st_name}: delete_(row) / delete_by_id require a @sql_primary_key field. Use `_sql_delete(type<{st_name}>, where)` instead."
            var fn = qmacro_function("_sql_delete_by_pk_sql") $(typ : $t(st); name : string) : string {
                panic($v(msg))
                return ""
            }
            fn.flags.generated = true
            fn.body |> force_at(st.at)
            return <- fn
        }
        let pk_col = fields[pk_idx].col_name
        let prefix = "DELETE FROM "
        let suffix = " WHERE \"{pk_col}\" = ?"
        var fn = qmacro_function("_sql_delete_by_pk_sql") $(typ : $t(st); name : string) : string {
            return $v(prefix) + sql_quote_id(name) + $v(suffix)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_bind_row_for_update_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        // SET-then-WHERE: non-PK cols at indices 1..N, PK at N+1 (mirrors _sql_update_by_pk_sql).
        let pk_idx = find_pk_field(fields)
        if (pk_idx < 0) {
            var fn = qmacro_function("_sql_bind_row_for_update") $(var stmt; row : $t(st)) : void {
                pass
            }
            fn.flags.generated = true
            fn.body |> force_at(st.at)
            return <- fn
        }

        var bind_exprs : array<ExpressionPtr>
        var bind_idx = 0
        // Non-PK, non-computed columns first.
        for (field in fields) {
            if (field.is_pk || field.is_computed) continue
            bind_idx++
            let bi = bind_idx
            let field_name = field.name
            bind_exprs |> push(qmacro_expr(${
                sql_bind_to_stmt(stmt, $v(bi), row.$f(field_name));
            }))
        }
        // PK column last (WHERE clause).
        bind_idx++
        let pk_bind_idx = bind_idx
        let pk_field_name = fields[pk_idx].name
        bind_exprs |> push(qmacro_expr(${
            sql_bind_to_stmt(stmt, $v(pk_bind_idx), row.$f(pk_field_name));
        }))

        var fn = qmacro_function("_sql_bind_row_for_update") $(var stmt; row : $t(st)) : void {
            $b(bind_exprs)
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_bind_row_pk_only_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
        // Binds the PK at index 1 for `DELETE FROM T WHERE pk=?`; no-op stub for PK-less.
        let pk_idx = find_pk_field(fields)
        if (pk_idx < 0) {
            var fn = qmacro_function("_sql_bind_row_pk_only") $(var stmt; row : $t(st)) : void {
                pass
            }
            fn.flags.generated = true
            fn.body |> force_at(st.at)
            return <- fn
        }
        let pk_field_name = fields[pk_idx].name
        var fn = qmacro_function("_sql_bind_row_pk_only") $(var stmt; row : $t(st)) : void {
            sql_bind_to_stmt(stmt, 1, row.$f(pk_field_name))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }

}

// ===== @sql_json adapter codegen =====

def private make_json_sql_bind_fn(field_type : TypeDeclPtr; at : LineInfo) : FunctionPtr {
    // Parser marks struct/tuple params const; clone + set constant so generic resolution picks this overload.
    var t1 <- clone_type(field_type)
    t1.flags.constant = true
    var fn = qmacro_function("sql_bind") $(v : $t(t1)) : string {
        return _::write_json(_::JV(v))
    }
    fn.body |> force_at(at)
    return <- fn
}

def private make_json_sql_extract_fn(field_type : TypeDeclPtr; at : LineInfo) : FunctionPtr {
    var t1 <- clone_type(field_type)
    t1.flags.constant = true
    var t2 <- clone_type(field_type)
    var t3 <- clone_type(field_type)
    // Panic on malformed JSON (mirrors @sql_blob); else schema drift surfaces as silent zero rows.
    var fn = qmacro_function("sql_extract") $(v : string; t : type<$t(t1)>) : $t(t2) {
        var err = ""
        let jv = _::read_json(v, err)
        if (err != "") {
            panic("sql_extract: malformed JSON in column data: {err}")
        }
        return _::from_JV(jv, default<$t(t3)>)
    }
    fn.body |> force_at(at)
    return <- fn
}

// ===== @sql_blob adapter codegen =====

def private make_blob_sql_bind_fn(field_type : TypeDeclPtr; at : LineInfo) : FunctionPtr {
    var t1 <- clone_type(field_type)
    t1.flags.constant = true
    var fn = qmacro_function("sql_bind") $(v : $t(t1)) : array<uint8> {
        var v_local <- clone_to_move(v)
        return <- _::mem_archive_save(v_local)
    }
    fn.body |> force_at(at)
    return <- fn
}

def private make_blob_sql_extract_fn(field_type : TypeDeclPtr; at : LineInfo) : FunctionPtr {
    var t1 <- clone_type(field_type)
    t1.flags.constant = true
    var t2 <- clone_type(field_type)
    var t3 <- clone_type(field_type)
    var fn = qmacro_function("sql_extract") $(v : array<uint8>; t : type<$t(t1)>) : $t(t2) {
        var r = default<$t(t3)>
        var v_local <- clone_to_move(v)
        _::mem_archive_load(v_local, r)
        return <- r
    }
    fn.body |> force_at(at)
    return <- fn
}

// Classifies an existing `sql_bind(T)` overload's return-type shape for @sql_json / @sql_blob dedup.
enum private SqlAdapterKind {
    None
    Json
    Blob
    Other
}

def private classify_sql_bind_return(td : TypeDeclPtr) : SqlAdapterKind {
    if (td == null) return SqlAdapterKind.None
    if (td.baseType == Type.tString) return SqlAdapterKind.Json
    if (td.baseType == Type.tArray && td.firstType != null && td.firstType.baseType == Type.tUInt8) return SqlAdapterKind.Blob
    return SqlAdapterKind.Other
}

def private find_sql_bind_kind_for_type(mod : Module?; td : TypeDeclPtr) : SqlAdapterKind {
    if (td == null) return SqlAdapterKind.None
    var kind = SqlAdapterKind.None
    for_each_function(mod, "sql_bind") $(var fn) {
        if (!empty(fn.arguments)
                && fn.arguments[0]._type != null
                && is_same_type(td, fn.arguments[0]._type, RefMatters.no, ConstMatters.no, TemporaryMatters.no)) {
            kind = classify_sql_bind_return(fn.result)
        }
    }
    return kind
}

def private emit_json_blob_adapters_for_struct(var st : StructurePtr; fields : array<FieldInfo>; tag : string; var errors : das_string) : bool {
    var seen_kinds : table<string; SqlAdapterKind>
    for (i in range(length(fields))) {
        let info = fields[i]
        if (!info.is_json && !info.is_blob) continue
        // Unwrap: runtime sql_bind_to_stmt(Option<T>) strips Option before calling sql_bind, so adapter must bind bare T.
        let ft = unwrap_option_payload_type(st.fields[i]._type)
        if (ft == null) continue
        let key = describe(ft)
        let expected_kind = info.is_json ? SqlAdapterKind.Json : SqlAdapterKind.Blob
        let kind_label = info.is_json ? "@sql_json" : "@sql_blob"
        let prev_kind = seen_kinds?[key] ?? SqlAdapterKind.None
        if (prev_kind != SqlAdapterKind.None) {
            if (prev_kind != expected_kind) {
                let prev_label = prev_kind == SqlAdapterKind.Json ? "@sql_json" : "@sql_blob"
                errors := "{tag}: field '{info.name}' is {kind_label}, but an earlier field of struct '{st.name}' already used type '{key}' with {prev_label}. Adapter dedup is per-(T, kind); the same T must be used with one storage kind across this struct. Pick one kind, or wrap '{key}' in a typedef so the two storage views are distinct types."
                return false
            }
            continue
        }
        seen_kinds |> insert(key, expected_kind)
        let existing_kind = find_sql_bind_kind_for_type(compiling_module(), ft)
        if (existing_kind != SqlAdapterKind.None) {
            if (existing_kind != expected_kind) {
                let existing_label = existing_kind == SqlAdapterKind.Json ? "@sql_json (sql_bind returns string)" : (existing_kind == SqlAdapterKind.Blob ? "@sql_blob (sql_bind returns array<uint8>)" : "an unrelated `sql_bind` overload")
                errors := "{tag}: field '{info.name}' is {kind_label}, but type '{key}' is already bound by {existing_label} (from a sibling annotation or a hand-written adapter). Adapter dedup is per-(T, kind); the same T must be used with one storage kind. Pick one kind, or wrap '{key}' in a typedef so the two storage views are distinct types."
                return false
            }
            continue
        }
        if (info.is_json) {
            var fn_b = make_json_sql_bind_fn(ft, st.at)
            compiling_module() |> add_function(fn_b)
            var fn_e = make_json_sql_extract_fn(ft, st.at)
            compiling_module() |> add_function(fn_e)
        } else {
            var fn_b = make_blob_sql_bind_fn(ft, st.at)
            compiling_module() |> add_function(fn_b)
            var fn_e = make_blob_sql_extract_fn(ft, st.at)
            compiling_module() |> add_function(fn_e)
        }
    }
    return true
}

// ===== [sql_view] — read-only struct view backed by a SQLite VIEW =====

[structure_macro(name=sql_view)]
class private SqlViewMacro : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {   // nolint:STYLE038 - flat registration table, one emit call per generated [sql_view] helper
        let view_name = find_annotation(args, "name", string(st.name))
        // Reject sibling [sql_index] up front so the error points at the macro stack, not a per-field follow-on.
        for (annDecl in st.annotations) {
            if (annDecl.annotation.name == "sql_index") {
                errors := "[sql_view] on struct '{st.name}': [sql_index] cannot apply to a view - views are read-only and inherit indexing from the underlying table. Move the index to the underlying table's struct."
                return false
            }
        }
        // ---- per-field validation: views accept only @sql_column / @sql_json / @sql_blob ----
        var fields : array<FieldInfo>
        fields |> reserve(length(st.fields))
        for (field in st.fields) {
            let fname = string(field.name)
            if (find_annotation(field.annotation, "sql_primary_key", false)) {
                errors := "[sql_view]: field '{fname}' has @sql_primary_key - views are read-only and cannot declare keys; primary keys belong on the underlying table."
                return false
            }
            if (find_annotation(field.annotation, "sql_unique", false)) {
                errors := "[sql_view]: field '{fname}' has @sql_unique - uniqueness constraints belong on the underlying table, not the view."
                return false
            }
            let computed_sql = find_annotation(field.annotation, "sql_computed", "")
            if (!empty(computed_sql)) {
                errors := "[sql_view]: field '{fname}' has @sql_computed - view bodies are arbitrary SELECT expressions (write the expression directly inside `_create_view`'s chain), not GENERATED ALWAYS columns."
                return false
            }
            if (find_annotation(field.annotation, "sql_stored", false)) {
                errors := "[sql_view]: field '{fname}' has @sql_stored - only @sql_computed columns can be STORED, and views don't have @sql_computed. Remove the annotation."
                return false
            }
            if (!empty(find_annotation(field.annotation, "sql_default_fn", ""))) {
                errors := "[sql_view]: field '{fname}' has @sql_default_fn - defaults belong on the underlying table, not the view."
                return false
            }
            if (field.init != null) {
                errors := "[sql_view]: field '{fname}' has a daslang field initializer - view fields are read-only projections of the SELECT body. Remove the initializer."
                return false
            }
            if (!empty(find_annotation(field.annotation, "sql_references", ""))) {
                errors := "[sql_view]: field '{fname}' has @sql_references - foreign keys belong on the underlying table, not the view."
                return false
            }
            if (!empty(find_annotation(field.annotation, "sql_on_delete", ""))) {
                errors := "[sql_view]: field '{fname}' has @sql_on_delete - FK actions belong on the underlying table, not the view."
                return false
            }
            if (!empty(find_annotation(field.annotation, "sql_on_update", ""))) {
                errors := "[sql_view]: field '{fname}' has @sql_on_update - FK actions belong on the underlying table, not the view."
                return false
            }
            let is_optional = is_option_field_type(field._type)
            let is_json = find_annotation(field.annotation, "sql_json", false)
            let is_blob = find_annotation(field.annotation, "sql_blob", false)
            if (is_json && is_blob) {
                errors := "[sql_view]: field '{fname}' has both @sql_json and @sql_blob - pick one. Use @sql_json for queryable, schema-tolerant TEXT storage; use @sql_blob for opaque binary storage."
                return false
            }
            // @sql_column rename, same rules as [sql_table].
            var col_name = fname
            let col_anno = find_arg(field.annotation, "sql_column")
            if (col_anno is tString) {
                let cv = col_anno as tString
                if (empty(cv)) {
                    errors := "[sql_view]: field '{fname}' @sql_column = \"\" is invalid - provide a non-empty SQL column name."
                    return false
                }
                if (find(cv, "\"") >= 0 || find(cv, "\\") >= 0) {
                    errors := "[sql_view]: field '{fname}' @sql_column=\"{cv}\" - embedded `\"` / `\\` are rejected (SQL identifier hygiene). Use a plain identifier."
                    return false
                }
                col_name = cv
            }
            fields |> push(FieldInfo(
                name = fname,
                col_name = col_name,
                is_pk = false,
                is_int_pk = false,
                is_optional = is_optional,
                is_unique = false,
                is_computed = false,
                is_stored = false,
                is_json = is_json,
                is_blob = is_blob,
                computed_sql = "",
                default_clause = "",
                references_clause = ""))
        }
        // ---- @sql_column collision detection ----
        {
            var seen_cols : table<string; string>
            for (info in fields) {
                if (key_exists(seen_cols, info.col_name)) {
                    let first_owner = seen_cols[info.col_name]
                    errors := "[sql_view]: SQL column name '{info.col_name}' is claimed by both field '{first_owner}' and field '{info.name}'. @sql_column renames must produce a unique column name across the struct."
                    return false
                }
                seen_cols |> insert(info.col_name, info.name)
            }
        }
        // ---- @sql_json / @sql_blob adapter codegen ----
        if (!emit_json_blob_adapters_for_struct(st, fields, "[sql_view]", errors)) return false
        // ---- read-path helpers (same names as [sql_table]) ----
        var fn_table_name = make_view_table_name_fn(st, view_name)
        compiling_module() |> add_function(fn_table_name)
        var fn_drop = make_view_drop_sql_fn(st, view_name)
        compiling_module() |> add_function(fn_drop)
        var fn_select_all = make_view_select_all_sql_fn(st, view_name, fields)
        compiling_module() |> add_function(fn_select_all)
        var fn_read = make_read_row_helper_fn(st, fields)
        compiling_module() |> add_generic(fn_read)
        var fn_col_info = make_view_column_info_fn(st, fields)
        compiling_module() |> add_function(fn_col_info)
        // ---- mutation stubs (panic at runtime with a clear message) ----
        let st_name = string(st.name)
        var fn_ins_pk = make_view_string_panic_stub_fn(st, "_sql_insert_with_pk_sql", st_name, "insert")
        compiling_module() |> add_function(fn_ins_pk)
        var fn_ins_nopk = make_view_string_panic_stub_fn(st, "_sql_insert_no_pk_sql", st_name, "insert")
        compiling_module() |> add_function(fn_ins_nopk)
        var fn_upd_pk = make_view_string_panic_stub_fn(st, "_sql_update_by_pk_sql", st_name, "update")
        compiling_module() |> add_function(fn_upd_pk)
        var fn_del_pk = make_view_string_panic_stub_fn(st, "_sql_delete_by_pk_sql", st_name, "delete")
        compiling_module() |> add_function(fn_del_pk)
        var fn_pk_unset = make_pk_is_unset_returns_false_fn(st)
        compiling_module() |> add_function(fn_pk_unset)
        var fn_bind = make_view_bind_row_panic_stub_fn(st, "_sql_bind_row", st_name, "insert", true)
        compiling_module() |> add_generic(fn_bind)
        var fn_bind_upd = make_view_bind_row_panic_stub_fn(st, "_sql_bind_row_for_update", st_name, "update", false)
        compiling_module() |> add_generic(fn_bind_upd)
        var fn_bind_pk = make_view_bind_row_panic_stub_fn(st, "_sql_bind_row_pk_only", st_name, "delete", false)
        compiling_module() |> add_generic(fn_bind_pk)
        return true
    }
}

// ===== Shared helper-function builders (used by [sql_view] and [sql_fts5]) =====
// Factored out because [sql_view]/[sql_fts5] generate identical qmacro_function shells.

def make_const_string_helper_fn(var st : StructurePtr; helper_name, val : string) : FunctionPtr {
    var fn = qmacro_function(helper_name) $(typ : $t(st)) : string {
        return $v(val)
    }
    fn.flags.generated = true
    fn.body |> force_at(st.at)
    return <- fn
}

def make_pk_is_unset_returns_false_fn(var st : StructurePtr) : FunctionPtr {
    var fn = qmacro_function("_sql_pk_is_unset") $(row : $t(st)) : bool {
        return false
    }
    fn.flags.generated = true
    fn.body |> force_at(st.at)
    return <- fn
}

def make_string_panic_stub_helper_fn(var st : StructurePtr; helper_name, msg : string) : FunctionPtr {
    var fn = qmacro_function(helper_name) $(typ : $t(st)) : string {
        panic($v(msg))
        return ""
    }
    fn.flags.generated = true
    fn.body |> force_at(st.at)
    return <- fn
}

def make_void_panic_stub_helper_fn(var st : StructurePtr; helper_name, msg : string;
                                   has_include_pk : bool) : FunctionPtr {
    if (has_include_pk) {
        var fn = qmacro_function(helper_name) $(var stmt; row : $t(st); include_pk : bool) : void {
            panic($v(msg))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    } else {
        var fn = qmacro_function(helper_name) $(var stmt; row : $t(st)) : void {
            panic($v(msg))
        }
        fn.flags.generated = true
        fn.body |> force_at(st.at)
        return <- fn
    }
}

def private make_view_table_name_fn(var st : StructurePtr; view_name : string) : FunctionPtr {
    return <- make_const_string_helper_fn(st, "_sql_table_name", view_name)
}

def private make_view_drop_sql_fn(var st : StructurePtr; view_name : string) : FunctionPtr {
    return <- make_const_string_helper_fn(st, "_sql_drop_view_if_exists_sql",
                                          "DROP VIEW IF EXISTS \"{view_name}\"")
}

def private make_view_select_all_sql_fn(var st : StructurePtr; view_name : string; fields : array<FieldInfo>) : FunctionPtr {
    let col_list = build_string() $(var w) {
        for (i in range(length(fields))) {
            if (i > 0) {
                w |> write(", ")
            }
            w |> write("\"")
            w |> write(fields[i].col_name)
            w |> write("\"")
        }
    }
    let sql = "SELECT {col_list} FROM \"{view_name}\""
    var fn = qmacro_function("_sql_select_all_sql") $(typ : $t(st)) : string {
        return $v(sql)
    }
    fn.flags.generated = true
    fn.body |> force_at(st.at)
    return <- fn
}

def make_read_row_helper_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
    // Per-struct _sql_read_row: column index = field index in declaration order.
    var assign_exprs : array<ExpressionPtr>
    assign_exprs |> reserve(length(fields))
    for (i in range(length(fields))) {
        let info = fields[i]
        let col_idx = i
        let field_name = info.name
        assign_exprs |> push(qmacro_expr(${
            static_if (typeinfo can_copy(default<$t(st.fields[i]._type)>)) {
                row.$f(field_name) = read_via_adapter(stmt, $v(col_idx), type<$t(st.fields[i]._type)>);
            } else {
                var field_tmp <- clone_to_move(read_via_adapter(stmt, $v(col_idx), type<$t(st.fields[i]._type)>));
                row.$f(field_name) <- field_tmp;
            }
        }))
    }
    // stmt untyped (generic over provider stmt type) — same rationale as make_read_row_fn.
    var fn = qmacro_function("_sql_read_row") $(stmt; typ : $t(st)) : $t(st) {
        var row = default<$t(st)>
        $b(assign_exprs)
        return <- row
    }
    fn.flags.generated = true
    fn.body |> force_at(st.at)
    return <- fn
}

def private make_view_column_info_fn(var st : StructurePtr; fields : array<FieldInfo>) : FunctionPtr {
    var push_exprs : array<ExpressionPtr>
    push_exprs |> reserve(length(fields))
    for (info, i in fields, count()) {
        let name = info.col_name
        let is_nullable = info.is_optional
        if (info.is_json) {
            push_exprs |> push(qmacro_expr(${ result |> push(ColumnInfo(
                name = $v(name),
                data_type = SqlType.Text,
                is_pk = false,
                is_nullable = $v(is_nullable),
                default_expr = "")); }))
        } elif (info.is_blob) {
            push_exprs |> push(qmacro_expr(${ result |> push(ColumnInfo(
                name = $v(name),
                data_type = SqlType.Blob,
                is_pk = false,
                is_nullable = $v(is_nullable),
                default_expr = "")); }))
        } else {
            push_exprs |> push(qmacro_expr(${ result |> push(ColumnInfo(
                name = $v(name),
                data_type = sql_storage_enum_for(type<$t(st.fields[i]._type)>),
                is_pk = false,
                is_nullable = $v(is_nullable),
                default_expr = "")); }))
        }
    }
    var fn = qmacro_function("_sql_column_info") $(typ : $t(st)) : array<ColumnInfo> {
        var result : array<ColumnInfo>
        $b(push_exprs)
        return <- result
    }
    fn.flags.generated = true
    fn.body |> force_at(st.at)
    return <- fn
}

def private make_view_string_panic_stub_fn(var st : StructurePtr; helper_name : string;
                                           st_name : string; verb : string) : FunctionPtr {
    let msg = "[sql_view] {st_name}: cannot {verb} into a view - views are read-only. Mutate the underlying table(s) and re-query the view."
    return <- make_string_panic_stub_helper_fn(st, helper_name, msg)
}

def private make_view_bind_row_panic_stub_fn(var st : StructurePtr; helper_name : string;
                                             st_name : string; verb : string;
                                             has_include_pk : bool) : FunctionPtr {
    let msg = "[sql_view] {st_name}: cannot {verb} into a view - views are read-only. Mutate the underlying table(s) and re-query the view."
    return <- make_void_panic_stub_helper_fn(st, helper_name, msg, has_include_pk)
}
// ===== Typed ALTER macros — migration-context ergonomics =====

[macro_function]
def private resolve_sql_table(prog : ProgramPtr; at : LineInfo; var typeArg : ExpressionPtr;
                              macroName : string; var st : StructurePtr&; var table_name : string&) : bool {
    // T must carry [sql_table]'s _sql_table_name helper.
    if (typeArg._type == null) {
        macro_error(prog, at, "{macroName}: `type<T>` argument has no inferred type yet")
        return false
    }
    if (typeArg._type.baseType != Type.tStructure || typeArg._type.structType == null) {
        macro_error(prog, at, "{macroName}: second argument must be `type<SomeStruct>` - got '{describe(typeArg._type)}'")
        return false
    }
    st = typeArg._type.structType
    var tname_fn = find_struct_helper_fn("_sql_table_name", st)
    if (tname_fn == null) {
        macro_error(prog, at, "{macroName}: type '{st.name}' has no [sql_table] declaration; only [sql_table]-decorated structs can drive typed ALTER")
        return false
    }
    table_name = extract_string_return(tname_fn)
    if (empty(table_name)) {
        macro_error(prog, at, "{macroName}: internal - _sql_table_name body did not match expected shape on '{st.name}'")
        return false
    }
    return true
}

[macro_function]
def private resolve_field_index(var st : StructurePtr; field_name : string) : int {
    // -1 if missing. Caller emits the macro_error with full context (macro name + struct name).
    for (i in range(length(st.fields))) {
        if (st.fields[i].name == field_name) return i
    }
    return -1
}

[macro_function]
def private field_sql_name_at(var st : StructurePtr; idx : int) : string {
    // @sql_column("X") rename → X; otherwise daslang field name. idx must be in range.
    let ca = find_arg(st.fields[idx].annotation, "sql_column")
    if (ca is tString) return ca as tString
    return string(st.fields[idx].name)
}

[macro_function]
def private decode_string_const(e : ExpressionPtr) : tuple<ok : bool; value : string> {
    // Strict literal-only: peel ExprRef2Value, accept ExprConstString.
    var inner = e
    if (inner != null && inner is ExprRef2Value) {
        inner = (inner as ExprRef2Value).subexpr
    }
    if (inner == null || !(inner is ExprConstString)) return (ok = false, value = "")
    return (ok = true, value = string((inner as ExprConstString).value))
}

[call_macro(name="add_column")]
class private AddColumnMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {   // nolint:STYLE038 - one arm per column-default literal kind
        // call.arguments = [db, type<T>, "FieldName"] (3) or [..., defaultLit] (4) after the `db |>` desugar.
        let nArgs = call.arguments |> length
        macro_verify(nArgs == 3 || nArgs == 4, prog, call.at,
            "add_column expects (db, type<T>, \"Field\" [, defaultLit]) — got {nArgs} args")
        var st : StructurePtr
        var table_name : string
        if (!resolve_sql_table(prog, call.at, call.arguments[1], "add_column", st, table_name)) return null
        // Field-name string literal.
        let nameRes = decode_string_const(call.arguments[2])
        if (!nameRes.ok) {
            macro_error(prog, call.at, "add_column: third argument must be a string literal naming the field - got '{describe(call.arguments[2])}'")
            return null
        }
        let field_name = nameRes.value
        let idx = resolve_field_index(st, field_name)
        if (idx < 0) {
            macro_error(prog, call.at, "add_column: field '{field_name}' is not a column of struct '{st.name}'")
            return null
        }
        // Reject shapes that need a table rebuild.
        let pk_arg = find_arg(st.fields[idx].annotation, "sql_primary_key")
        if (pk_arg is tBool && (pk_arg as tBool)) {
            macro_error(prog, call.at,
                "add_column cannot add a PRIMARY KEY column to an existing table; recreate the table - see chunk 14c struct_convert")
            return null
        }
        let uniq_arg = find_arg(st.fields[idx].annotation, "sql_unique")
        if (uniq_arg is tBool && (uniq_arg as tBool)) {
            macro_error(prog, call.at,
                "add_column cannot add a UNIQUE column to an existing table; create_unique_index(type<T>, \"{field_name}\") instead")
            return null
        }
        let computed_arg = find_arg(st.fields[idx].annotation, "sql_computed")
        if (computed_arg is tString || (computed_arg is tBool && (computed_arg as tBool))) {
            macro_error(prog, call.at,
                "add_column cannot add a generated/computed column to an existing table; recreate the table - see chunk 14c struct_convert")
            return null
        }
        let default_fn_arg = find_arg(st.fields[idx].annotation, "sql_default_fn")
        if (default_fn_arg is tString) {
            macro_error(prog, call.at,
                "add_column does not support @sql_default_fn fields yet; use a literal default or `db |> exec(...)`")
            return null
        }
        // Read storage hints + nullability.
        let json_arg = find_arg(st.fields[idx].annotation, "sql_json")
        let is_json = json_arg is tBool && (json_arg as tBool)
        let blob_arg = find_arg(st.fields[idx].annotation, "sql_blob")
        let is_blob = blob_arg is tBool && (blob_arg as tBool)
        let is_optional = is_option_field_type(st.fields[idx]._type)
        let col_name = field_sql_name_at(st, idx)
        // DEFAULT clause from optional 4th arg (compile-time literal only).
        var default_clause = ""
        if (nArgs == 4) {
            if (is_json || is_blob) {
                macro_error(prog, call.at,
                    "add_column: explicit default not supported for @sql_json / @sql_blob columns; the column accepts NULL by default - wrap the field in Option<T> if nullability is desired")
                return null
            }
            var defArg = call.arguments[3]
            if (defArg != null && defArg is ExprRef2Value) {
                defArg = (defArg as ExprRef2Value).subexpr
            }
            default_clause = literal_init_to_default_clause(defArg)
            if (empty(default_clause)) {
                macro_error(prog, call.at,
                    "add_column: DEFAULT must be a compile-time literal (int / int64 / float / double / bool / string); got '{describe(call.arguments[3])}'. Inline the value, or use `db |> exec(...)` for runtime expressions.")
                return null
            }
        } elif (!is_optional && !is_json && !is_blob) {
            // SQLite ADD COLUMN against existing rows: NOT NULL without DEFAULT panics for any pre-existing row.
            macro_error(prog, call.at,
                "add_column: field '{field_name}' on '{st.name}' is non-nullable; provide a literal default (e.g. `add_column(type<T>, \"{field_name}\", 0)`) or wrap the field in Option<T>")
            return null
        }
        // Build prefix + suffix at macro time; SQL type comes from the runtime adapter rail (sql_storage_type_for) unless the field is explicitly @sql_json / @sql_blob.
        let null_clause = is_optional ? "" : " NOT NULL"
        let prefix = "ALTER TABLE \"{table_name}\" ADD COLUMN \"{col_name}\" "
        let suffix = "{null_clause}{default_clause}"
        var dbExpr = call.arguments[0]
        var res : ExpressionPtr
        if (is_json) {
            let full = "{prefix}TEXT{suffix}"
            res = qmacro(_::exec($e(dbExpr), $v(full)))
        } elif (is_blob) {
            let full = "{prefix}BLOB{suffix}"
            res = qmacro(_::exec($e(dbExpr), $v(full)))
        } else {
            var fieldType <- clone_type(st.fields[idx]._type)
            res = qmacro(_::exec($e(dbExpr), build_string() $(var w) {
                w |> write($v(prefix))
                w |> write(_::sql_storage_type_for(type<$t(fieldType)>))
                w |> write($v(suffix))
            }))
        }
        res.force_at(call.at)
        return res
    }
}

[macro_function]
def private resolve_field_columns(prog : ProgramPtr; at : LineInfo; var st : StructurePtr;
                                  fieldsArg : ExpressionPtr; macroName : string;
                                  var sql_cols : array<string>&) : bool {
    // Single string literal or a tuple of string literals. Array literals are AST-hairy and rejected.
    if (fieldsArg == null) {
        macro_error(prog, at, "{macroName}: missing fields argument")
        return false
    }
    var inner = fieldsArg
    if (inner is ExprRef2Value) {
        inner = (inner as ExprRef2Value).subexpr
    }
    var col_names : array<string>
    if (inner is ExprConstString) {
        col_names |> push(string((inner as ExprConstString).value))
    } elif (inner is ExprMakeTuple) {
        let mkt = inner as ExprMakeTuple
        for (v in mkt.values) {
            var item = v
            if (item is ExprRef2Value) {
                item = (item as ExprRef2Value).subexpr
            }
            if (item == null || !(item is ExprConstString)) {
                macro_error(prog, at, "{macroName}: tuple fields entries must be string literals - got '{describe(v)}'")
                return false
            }
            col_names |> push(string((item as ExprConstString).value))
        }
    } else {
        macro_error(prog, at, "{macroName}: fields argument must be a string literal (\"Field\") or a tuple of string literals (\"A\", \"B\") - got '{describe(fieldsArg)}'")
        return false
    }
    if (empty(col_names)) {
        macro_error(prog, at, "{macroName}: empty fields list. Provide \"Field\" for a single-column index or (\"A\", \"B\") for a composite.")
        return false
    }
    sql_cols |> reserve(length(col_names))
    for (cn in col_names) {
        let idx = resolve_field_index(st, cn)
        if (idx < 0) {
            macro_error(prog, at, "{macroName}: field '{cn}' is not a column of struct '{st.name}'")
            return false
        }
        sql_cols |> push(field_sql_name_at(st, idx))
    }
    return true
}

[macro_function]
def private emit_create_index(prog : ProgramPtr; var call : ExprCallMacro?; is_unique : bool) : ExpressionPtr {
    let macroName = is_unique ? "create_unique_index" : "create_index"
    let nArgs = call.arguments |> length
    if (nArgs < 3 || nArgs > 4) {
        macro_error(prog, call.at,
            "{macroName} expects (db, type<T>, fields [, \"name\"]) - got {nArgs} args")
        return null
    }
    var st : StructurePtr
    var table_name : string
    if (!resolve_sql_table(prog, call.at, call.arguments[1], macroName, st, table_name)) return null
    var sql_cols : array<string>
    if (!resolve_field_columns(prog, call.at, st, call.arguments[2], macroName, sql_cols)) return null
    var idx_name = derive_index_name(table_name, sql_cols)
    if (nArgs == 4) {
        let nameRes = decode_string_const(call.arguments[3])
        if (!nameRes.ok) {
            macro_error(prog, call.at,
                "{macroName}: optional `name` argument must be a string literal - got '{describe(call.arguments[3])}'")
            return null
        }
        if (empty(nameRes.value)) {
            macro_error(prog, call.at, "{macroName}: index name string is empty")
            return null
        }
        idx_name = nameRes.value
    }
    let kind = is_unique ? "UNIQUE INDEX" : "INDEX"
    let ddl = build_string() $(var w) {
        w |> write("CREATE {kind} \"{idx_name}\" ON \"{table_name}\" (")
        for (j in range(length(sql_cols))) {
            if (j > 0) {
                w |> write(", ")
            }
            w |> write("\"")
            w |> write(sql_cols[j])
            w |> write("\"")
        }
        w |> write(")")
    }
    var res = qmacro(_::exec($e(call.arguments[0]), $v(ddl)))
    res.force_at(call.at)
    return res
}

[call_macro(name="create_index")]
class private CreateIndexMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        return emit_create_index(prog, call, false)
    }
}

[call_macro(name="create_unique_index")]
class private CreateUniqueIndexMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        return emit_create_index(prog, call, true)
    }
}

// ===== [sql_function] — auto-registered scalar UDF, visible in `_sql` chains =====
// Provider-neutral: tagged functions land in ONE erased queue; every UDF-capable
// provider's open path replays it (`_install_sql_function_registry` in each boost).

enum public SqlFnTag : uint8 {
    //! Type-tag wire contract shared with each provider's userfn trampoline C++
    //! (dasSQLITE.userfn.cpp, dasDUCKDB.userfn.cpp). Order is wire-stable; do NOT
    //! renumber without also updating every provider's C++.
    Int = uint8(0)
    Int64 = uint8(1)
    Float = uint8(2)
    Double = uint8(3)
    Bool = uint8(4)
    String = uint8(5)
}

typedef public SqlFnCarrier = function<() : void>
//! Erasure carrier for the queued function pointer. The provider registration externs
//! take a raw ``das::Func`` (any function), so only the 64-bit value matters — the
//! spelled signature is a placeholder; never invoke through it.

struct public SqlFunctionEntry {
    //! One queued ``[sql_function]`` registration, type-erased. Providers marshal via
    //! the tag slots (unused slots are 0/ignored per ``nArgs``), not the das-side type.
    name : string
    nArgs : int
    tag0 : uint8
    tag1 : uint8
    tag2 : uint8
    tag3 : uint8
    retTag : uint8
    deterministic : bool
    directonly : bool
    fn : SqlFnCarrier
}

// Populated by [init] thunks emitted into consumer modules; replayed by every provider
// open path (try_open_sqlite / try_open_duckdb / ...).
var private g_sql_functions : array<SqlFunctionEntry>

def public _add_sql_function_entry(name : string; nArgs : int;
                                   tag0, tag1, tag2, tag3 : uint8;
                                   retTag : uint8; deterministic : bool; directonly : bool;
                                   fn : auto(FN)) : void {
    //! Public entry point used by ``[sql_function]``-generated ``[init]`` blocks to append
    //! a registration to the neutral queue. End users never call this directly.
    var e <- SqlFunctionEntry(name = name, nArgs = nArgs,
        tag0 = tag0, tag1 = tag1, tag2 = tag2, tag3 = tag3,
        retTag = retTag, deterministic = deterministic, directonly = directonly,
        fn = unsafe(reinterpret<SqlFnCarrier>(fn)))
    g_sql_functions |> emplace(e)
}

def public for_each_sql_function(blk : block<(e : SqlFunctionEntry) : void>) : void {
    //! Iterates the queued registrations — the provider boosts' install loops drive this
    //! once per opened connection.
    for (e in g_sql_functions) {
        invoke(blk, e)
    }
}

def public sql_function_duplicate_error : SqlError {
    //! ``some(msg)`` when two queued registrations collide on (name, nArgs) — engines would
    //! silently replace the earlier binding. Provider install loops check this first.
    var seen : table<string; bool>
    for (e in g_sql_functions) {
        let key = "{e.name}/{e.nArgs}"
        if (key_exists(seen, key)) {
            return some("[sql_function] duplicate registration: '{e.name}' with {e.nArgs} args is tagged in more than one place; the engine would silently replace the earlier binding")
        }
        seen |> insert(key, true)
    }
    return none(type<string>)
}

[macro_function]
def public sql_fn_tag_for_type(td : TypeDeclPtr) : tuple<tag : uint8; ok : bool> {
    //! Maps a scalar das type to its SqlFnTag wire value; ``ok = false`` for anything a
    //! scalar UDF can't marshal. Shared by the annotation and the providers' direct
    //! ``register_function`` call macros.
    if (td == null || td.baseType == Type.tFixedArray) return (tag = uint8(0), ok = false)
    if (td.baseType == Type.tInt) return (tag = uint8(SqlFnTag.Int), ok = true)
    if (td.baseType == Type.tInt64) return (tag = uint8(SqlFnTag.Int64), ok = true)
    if (td.baseType == Type.tFloat) return (tag = uint8(SqlFnTag.Float), ok = true)
    if (td.baseType == Type.tDouble) return (tag = uint8(SqlFnTag.Double), ok = true)
    if (td.baseType == Type.tBool) return (tag = uint8(SqlFnTag.Bool), ok = true)
    if (td.baseType == Type.tString) return (tag = uint8(SqlFnTag.String), ok = true)
    return (tag = uint8(0), ok = false)
}

def private _is_valid_sql_function_name(s : string) : bool {
    // Unquoted identifier shape: [A-Za-z_][A-Za-z0-9_]* — anything else would need quoting in emitted SQL.
    if (empty(s)) return false
    var ok = true
    peek_data(s) $(arr) {
        let c0 = int(arr[0])
        if (!(is_alpha(c0) || c0 == '_')) {
            ok = false
            return
        }
        for (i in range(1, length(arr))) {
            let c = int(arr[i])
            if (!(is_alnum(c) || c == '_')) {
                ok = false
                return
            }
        }
    }
    return ok
}

[function_macro(name="sql_function")]
class SqlFunctionMacro : AstFunctionAnnotation {
    //! Marks a daslang function as a scalar SQL UDF: registered against every connection
    //! of every UDF-capable provider, and emitted as ``<name>(args)`` inside ``_sql(...)``
    //! chains. Optional args: ``name``, ``deterministic``, ``directonly``.
    def override apply(var func : FunctionPtr; var group : ModuleGroup;   // nolint:STYLE038 - flat arity ladder, one binding form per UDF signature
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        if (func.fromGeneric != null) {
            errors := "[sql_function] is not supported on generic functions; instantiate with concrete types and tag the concrete instance"
            return false
        }
        let nArgs = func.arguments |> length
        if (nArgs > 4) {
            errors := "[sql_function]: scalar SQL functions support up to 4 arguments; got {nArgs}"
            return false
        }
        var tags : array<uint8>
        tags |> reserve(4)
        for (i in range(nArgs)) {
            let r = sql_fn_tag_for_type(func.arguments[i]._type)
            if (!r.ok) {
                errors := "[sql_function]: argument #{i + 1} of '{func.name}' has unsupported type '{describe(func.arguments[i]._type)}' - supported: int, int64, float, double, bool, string"
                return false
            }
            tags |> push(r.tag)
        }
        while (tags |> length < 4) {
            tags |> push(uint8(0))
        }
        let retR = sql_fn_tag_for_type(func.result)
        if (!retR.ok) {
            errors := "[sql_function]: return type '{describe(func.result)}' of '{func.name}' is unsupported - supported: int, int64, float, double, bool, string"
            return false
        }
        let funcDasName = "{func.name}"
        var sqlName = funcDasName
        let nameArg = args |> find_arg("name")
        if (nameArg is tString) {
            sqlName = nameArg as tString
            if (empty(sqlName)) {
                errors := "[sql_function]: name= cannot be empty"
                return false
            }
            if (!_is_valid_sql_function_name(sqlName)) {
                errors := "[sql_function]: name='{sqlName}' is not a valid SQL identifier (must match [A-Za-z_][A-Za-z0-9_]*)"
                return false
            }
        } elif (!(nameArg is nothing)) {
            errors := "[sql_function]: name= must be a string literal"
            return false
        }
        var deterministic = false
        let detArg = args |> find_arg("deterministic")
        if (detArg is tBool) {
            deterministic = detArg as tBool
        } elif (!(detArg is nothing)) {
            errors := "[sql_function]: deterministic= must be a bool literal (true/false)"
            return false
        }
        var directonly = false
        let donArg = args |> find_arg("directonly")
        if (donArg is tBool) {
            directonly = donArg as tBool
        } elif (!(donArg is nothing)) {
            errors := "[sql_function]: directonly= must be a bool literal (true/false)"
            return false
        }
        var reg <- setup_call_list("register`sql`functions", func.at, true/*isInit*/, true/*isPrivate*/)
        let nArgsLit = nArgs
        let tag0 = tags[0]
        let tag1 = tags[1]
        let tag2 = tags[2]
        let tag3 = tags[3]
        let retTag = retR.tag
        // ExprAddr with explicit funcType + `__::` target — funcType alone ties on same-signature imports.
        var fnType = new TypeDecl(at = func.at, baseType = Type.tFunction)
        for (a in func.arguments) {
            fnType.argTypes |> emplace_new <| clone_type(a._type)
        }
        fnType.firstType = clone_type(func.result)
        // `__::<name>` scopes ExprAddr resolution to *this* module (where `func` lives), so
        // a same-signature import (e.g. fio_core::normalize) doesn't tie with user's `normalize`.
        var fnAddr = new ExprAddr(at = func.at, target := "__::{funcDasName}", funcType <- fnType)
        reg.list |> emplace_new <| qmacro_block() {
            sql_boost::_add_sql_function_entry($v(sqlName), $v(nArgsLit),
                $v(tag0), $v(tag1), $v(tag2), $v(tag3),
                $v(retTag), $v(deterministic), $v(directonly), $e(fnAddr))
        }
        return true
    }
}
