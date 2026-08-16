options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE031: table<K;V> (or table<K> set) with empty default-init followed by a
// run of insert / `[]=` statements — use a table (set) literal move-assign.
//
// Problem: Declaring an empty `table<K;V>` and then populating it with a run of
// `t |> insert(k, v)` / `t[k] = v` statements is the table variant of STYLE012.
// A table literal expresses the same construction in one move-assign:
//
//   var t <- { "a" => 1, "b" => 2 }            // map
//   var s <- { 5, 7 }                          // set (table<K>)
//
// Match rules:
//   * ExprLet declares the table local with NO initializer
//   * >= 2 contiguous statements immediately after are `insert` into it (3-arg
//     map form / 2-arg set form) or `t[k] = v` at-assigns (map only)
//   * runs containing duplicate CONSTANT keys stay silent — sequential inserts
//     overwrite (last wins), but a literal rejects duplicates at compile time
//     (error 30706); computed keys are fine (runtime duplicate = last wins in
//     both forms)
//   * table<string; JsonValue?> const-key runs are owned by STYLE021 (the
//     JV((k1=..., k2=...)) form is the stronger suggestion)

expect 31209:7

require daslib/style_lint
require daslib/json_boost

struct Pt {
    x : int = 0
    y : int = 0
}

def make_key(i : int) : string => "k{i}"

// --- Bad patterns (must fire — 6 x STYLE031 + 1 x STYLE021 below) ---

def bad_map_insert_run() : table<string; Pt> {
    var t : table<string; Pt>                         // STYLE031 — map insert run
    t |> insert("a", Pt(x = 1, y = 10))
    t |> insert("b", Pt(x = 2, y = 20))
    t |> insert("c", Pt(x = 3, y = 30))
    t |> insert("d", Pt(x = 4, y = 40))
    return <- t
}

def bad_set_insert_run() : int {
    var s : table<int>                                // STYLE031 — set insert run
    s |> insert(5)
    s |> insert(7)
    return length(s)
}

def bad_at_assign_run() : int {
    var t : table<string; int>                        // STYLE031 — []= run
    t["a"] = 1
    t["b"] = 2
    return length(t)
}

def bad_mixed_insert_and_at_assign() : int {
    var t : table<string; int>                        // STYLE031 — mixed insert + []=
    t |> insert("a", 1)
    t["b"] = 2
    return length(t)
}

def bad_computed_keys(i : int) : int {
    var t : table<string; int>                        // STYLE031 — computed keys collapse fine
    t |> insert(make_key(i), 1)
    t |> insert(make_key(i + 1), 2)
    return length(t)
}

def bad_int_keys() : int {
    var t : table<int; string>                        // STYLE031 — int-keyed run
    t |> insert(1, "a")
    t |> insert(2, "b")
    return length(t)
}

def bad_jv_table_const_keys() : int {
    var args : table<string; JsonValue?>              // STYLE021 fires; STYLE031 stays silent
    args |> insert("k1", JV(1))
    args |> insert("k2", JV(2))
    return length(args)
}

// --- Good patterns (must NOT fire) ---

def good_single_insert() : int {
    // A single insert is often the more readable form.
    var t : table<string; int>
    t |> insert("a", 1)
    return length(t)
}

def good_preinitialized() : int {
    // Init is non-empty — the literal rewrite would lose the existing entries.
    var t <- { "a" => 1 }
    t |> insert("b", 2)
    t |> insert("c", 3)
    return length(t)
}

def good_duplicate_const_key() : int {
    // Sequential inserts overwrite (last wins); a literal with a duplicate
    // constant key is a compile error (30706) — rewrite must not be suggested.
    var t : table<string; int>
    t |> insert("a", 1)
    t |> insert("a", 2)
    return length(t)
}

def good_intervening_statement() : int {
    // Decl and the insert run are not adjacent.
    var t : table<string; int>
    let n = 42
    t |> insert("a", n)
    t |> insert("b", n)
    return length(t)
}
