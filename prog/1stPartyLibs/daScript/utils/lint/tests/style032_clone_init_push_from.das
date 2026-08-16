options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// STYLE032: array<T> default-initialized then filled by a single `push_from` /
// `push_clone_from` from another array<T> — that is a clone of the source, so
// `var w := SRC` (clone-assign) expresses it in one line.
//
// Problem: an empty `var w : array<T>` immediately followed by a single bulk
// `w |> push_from(src)` (or `push_clone_from`) is just a verbose clone of src.
// `:=` clones the whole array (each element, for push_clone_from) in one step:
//
//   var w := src
//
// Match rules:
//   * ExprLet declares an array<T> local with NO initializer
//   * the IMMEDIATELY following statement is `w |> push_from(src)` or
//     `w |> push_clone_from(src)` (exactly 2 args, arg[0] == w)
//   * src is itself an array<T> — a C-array (fixed-array) source has no clean
//     `:=` rewrite, so it stays silent

expect 31209:3

require daslib/style_lint

struct Item {
    data : array<int>
}

// --- Bad patterns (must fire) ---

def bad_push_from(src : array<uint>) : array<uint> {
    var w : array<uint>                       // STYLE032 — clone via push_from
    w |> push_from(src)
    return <- w
}

def bad_push_clone_from(src : array<Item>) : array<Item> {
    var w : array<Item>                       // STYLE032 — clone-each-element via push_clone_from
    w |> push_clone_from(src)
    return <- w
}

def bad_push_from_no_return(src : array<uint>) : int {
    var w : array<uint>                       // STYLE032 — fires even without a return
    w |> push_from(src)
    w |> push(99u)
    return length(w)
}

// --- Good patterns (must NOT fire) ---

def good_clone_init(src : array<uint>) : array<uint> {
    // Already the suggested form.
    var w := src
    return <- w
}

def good_c_array_source() : array<uint> {
    // Source is a fixed C-array — `var w := cdata` would not yield an array<uint>.
    var cdata = fixed_array(1u, 2u, 3u)
    var w : array<uint>
    w |> push_from(cdata)
    return <- w
}

def good_nonempty_init(src : array<uint>) : array<uint> {
    // Init is non-empty — push_from genuinely appends here.
    var w <- [7u]
    w |> push_from(src)
    return <- w
}

def good_intervening_statement(src : array<uint>) : array<uint> {
    // Decl and the push_from are not adjacent; w is not fresh-empty at push_from.
    var w : array<uint>
    w |> push(0u)
    w |> push_from(src)
    return <- w
}

def good_single_push(src : uint) : array<uint> {
    // A single element push is not a bulk clone.
    var w : array<uint>
    w |> push(src)
    return <- w
}
