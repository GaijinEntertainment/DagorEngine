options gen2
options indenting = 4
options _comment_hygiene = true

module linq_fold_sql shared public

//! linq_fold SQL source pass-through: ``extract_sql_source``. Lets ``_fold`` over a
//! ``db |> select_from(type<Row>)`` chain (where ``Row`` is a ``[sql_table]``/``[sql_view]``/
//! ``[sql_fts5]`` struct) re-dispatch the whole chain to the ``_sql`` macro instead of folding the
//! materialized full-table ``array<Row>`` in memory. There is no emitting adapter — ``_sql`` is a
//! complete parallel lowering; this module only recognizes the source. Companion to
//! ``daslib/linq_fold``; provider-neutral (matches the ``[sql_table]`` annotation, not a runner
//! type), so it loads unconditionally. See daslib/ARCHITECTURE_LINQ.md sec. 37.

require daslib/linq_fold_common public
require daslib/ast_boost
require daslib/ast

// True iff `sql_linq` (which defines the `_sql` macro) is in the current program. A consumer may
// require a provider boost (for `select_from`) without `daslib/sql_linq`; then we must NOT rewrite
// to `_sql` (it wouldn't resolve) — `_fold` falls back to the in-memory array path instead.
[macro_function]
def sql_linq_loaded(prog : ProgramPtr) : bool {
    var found = false
    program_for_each_module(prog) $(mod) {
        if (mod.name == "sql_linq") {
            found = true
        }
    }
    return found
}

def private is_sql_table_select(c : ExprCall?) : bool {
    if (c._type == null || !c._type.isGoodArrayType || c._type.firstType == null) return false
    let st = c._type.firstType.structType
    if (st == null) return false
    for (annDecl in st.annotations) {
        if (annDecl.annotation.name == "sql_table" || annDecl.annotation.name == "sql_view"
                || annDecl.annotation.name == "sql_fts5") return true
    }
    return false
}

[macro_function]
def extract_sql_source(var expr : Expression?) : ExprCall? {
    var e = expr
    while (e != null && e is ExprCall) {
        var c = e as ExprCall
        if (get_call_short_name(c) == "select_from") {
            return is_sql_table_select(c) ? c : null
        }
        if (empty(c.arguments)) break
        e = c.arguments[0]
    }
    return null
}
