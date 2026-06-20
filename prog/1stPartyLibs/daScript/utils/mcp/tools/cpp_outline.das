options gen2

require cpp_common public
require daslib/strings_boost public
require strings public

// Containment-tree node — `idx` indexes into the per-file `entries` array,
// `children` indexes into the per-file `nodes` array.
struct CppOutlineNode {
    idx : int
    children : array<int>
}

// `do_cpp_outline` — list C++ declarations grouped by file.
//
// Pass either `file` (single absolute or repo-relative path) for single-file
// scope, or `glob_filter` (e.g. "src/builtin/*.cpp") for multi-file scope
// against the default C++ search dirs. If both are set, `file` wins —
// `glob_filter` is forwarded to ast-grep but a single-file scope makes the
// glob a no-op.
//
// `mode` — display mode:
//   ""     auto: tree if any nesting exists, else flat
//   "tree" force indented hierarchy
//   "flat" force flat list with `Outer::Inner::name` qualified prefixes
//
// Output: `  LINE: [kind] DISPLAY` per entry. For functions, DISPLAY is the
// full signature (return type + params + qualifiers). For classes/structs/etc.,
// DISPLAY is the qualified name plus any template specialization args.
def do_cpp_outline(file, glob_filter, mode : string) : string {
    if (empty(file) && empty(glob_filter)) return make_tool_result("'file' or 'glob_filter' argument is required", true)
    if (!empty(mode) && mode != "tree" && mode != "flat") return make_tool_result("invalid mode '{mode}' — expected '', 'tree', or 'flat'", true)
    var search_dirs : array<string>
    if (!empty(file)) {
        search_dirs |> push(resolve_path(file))
    } else {
        search_dirs <- cpp_default_search_dirs()
    }
    var err : string
    let entries <- cpp_run_scan(search_dirs, glob_filter, err)
    if (!empty(err)) return make_tool_result(err, true)
    if (empty(entries)) return make_tool_result("No C++ declarations found")
    // group by file, preserve discovery order
    var by_file : table<string; array<int>>
    var file_order : array<string>
    file_order |> reserve(length(entries))
    for (i in range(length(entries))) {
        let fp = entries[i].file
        if (!key_exists(by_file, fp)) {
            file_order |> push(fp)
        }
        by_file[fp] |> push(i) // nolint:PERF006
    }
    let multi_file = length(file_order) > 1
    let output = build_string() $(var w) {
        for (fp_idx in range(length(file_order))) {
            let fp = file_order[fp_idx]
            if (multi_file) {
                w |> write("{fp}:\n")
            }
            var nodes : array<CppOutlineNode>
            var roots : array<int>
            cpp_build_forest(entries, by_file[fp], nodes, roots)
            let tree_mode = empty(mode) ? cpp_has_nesting(nodes) : mode == "tree"
            var ancestors : array<int>
            for (root in roots) {
                cpp_render_node(entries, nodes, root, tree_mode, ancestors, w)
            }
            if (multi_file && fp_idx < length(file_order) - 1) {
                w |> write("\n")
            }
        }
    }
    return make_tool_result(output)
}

// Build a containment forest over `idx_list` (indices into `entries`).
// Walks idx_list sorted by (start_line ASC, end_line DESC) so outer scopes
// open first; uses a stack to track currently-open scopes and resolves each
// new entry's parent as the innermost still-open scope.
def cpp_build_forest(entries : array<CppMatch>; idx_list : array<int>; var out_nodes : array<CppOutlineNode>; var out_roots : array<int>) {
    var sorted : array<int>
    sorted := idx_list
    sort(sorted) $(a, b) {
        let ea & = unsafe(entries[a])
        let eb & = unsafe(entries[b])
        if (ea.line != eb.line) return ea.line < eb.line
        return ea.end_line > eb.end_line
    }
    out_nodes |> reserve(length(sorted))
    out_roots |> reserve(length(sorted))
    var stack : array<int>
    stack |> reserve(length(sorted))
    for (entry_idx in sorted) {
        let e_line = entries[entry_idx].line
        // Pop scopes whose span ended before this entry's start.
        while (!empty(stack)) {
            let top_node_idx = stack[length(stack) - 1]
            let top_entry_idx = out_nodes[top_node_idx].idx
            if (entries[top_entry_idx].end_line < e_line) {
                stack |> pop()
            } else {
                break
            }
        }
        out_nodes |> emplace(CppOutlineNode(idx = entry_idx))
        let new_node_idx = length(out_nodes) - 1
        if (empty(stack)) {
            out_roots |> push(new_node_idx)
        } else {
            let parent_idx = stack[length(stack) - 1]
            out_nodes[parent_idx].children |> push(new_node_idx)
        }
        stack |> push(new_node_idx)
    }
}

// True if any node has at least one child — i.e. tree-mode auto-trigger.
def cpp_has_nesting(nodes : array<CppOutlineNode>) : bool {
    for (n in nodes) {
        if (!empty(n.children)) return true
    }
    return false
}

def cpp_render_node(entries : array<CppMatch>; nodes : array<CppOutlineNode>; node_idx : int; tree_mode : bool; var ancestors : array<int>; var w : StringBuilderWriter -const) {
    let n & = unsafe(nodes[node_idx])
    let e & = unsafe(entries[n.idx])
    let depth = length(ancestors)
    let indent = tree_mode ? cpp_spaces(depth * 2) : ""
    let display = cpp_format_entry(e, ancestors, entries, tree_mode)
    w |> write("  {e.line}: {indent}[{e.kind}] {display}\n")
    ancestors |> push(n.idx)
    for (child_idx in n.children) {
        cpp_render_node(entries, nodes, child_idx, tree_mode, ancestors, w)
    }
    ancestors |> pop()
}

def cpp_spaces(n : int) : string {
    return build_string() $(var w) {
        for (_ in range(n)) {
            w |> write(" ")
        }
    }
}

// Compose the display string for a single entry.
// In flat mode, prepend the `Outer::Inner::` chain when the entry's qualified
// name doesn't already include it (avoids double prefixing on out-of-class
// definitions like `void TypeDecl::isString()`).
def cpp_format_entry(e : CppMatch; ancestors : array<int>; entries : array<CppMatch>; tree_mode : bool) : string {
    // Whether the entry's NAME is already qualified — checked against
    // `e.qualified` rather than the full signature so `::` in return/parameter
    // types (e.g. `virtual das::FileInfo * getNewFileInfo(...)`) doesn't
    // suppress prefix injection on an in-class declaration.
    let name_qualified = !empty(e.qualified) && find(e.qualified, "::") >= 0
    if (e.kind == "function" && !empty(e.signature)) {
        if (!tree_mode && !empty(ancestors) && !name_qualified) {
            let prefix = cpp_ancestor_prefix(ancestors, entries)
            if (!empty(prefix)) return cpp_inject_qualifier(e.signature, prefix)
        }
        return e.signature
    }
    var name = empty(e.qualified) ? e.text : e.qualified
    name = "{name}{e.template_args}"
    if (!tree_mode && !empty(ancestors) && !name_qualified) {
        let prefix = cpp_ancestor_prefix(ancestors, entries)
        if (!empty(prefix)) {
            name = "{prefix}{name}"
        }
    }
    return name
}

// Build "Outer::Inner::" from the ancestor stack — only class/struct/namespace
// kinds contribute; functions don't (a free function nested inside a namespace
// would still have function ancestors of its own only via lambdas, which the
// outline rules don't match).
def cpp_ancestor_prefix(ancestors : array<int>; entries : array<CppMatch>) : string {
    var parts : array<string>
    for (a_idx in ancestors) {
        let ae & = unsafe(entries[a_idx])
        if ((ae.kind == "class" || ae.kind == "struct" || ae.kind == "namespace") && !empty(ae.qualified)) {
            parts |> push("{ae.qualified}{ae.template_args}")
        }
    }
    if (empty(parts)) return ""
    return "{join(parts, "::")}::"
}

// Insert `Class::` immediately before the function name in a signature string
// like "void  foo(int x) const" → "void  Class::foo(int x) const".
// Locates the name by walking back from the first non-template '(' over
// identifier chars.
def cpp_inject_qualifier(signature : string; prefix : string) : string {
    let op = find_open_paren(signature)
    if (op <= 0) return "{prefix}{signature}"
    var nm = op - 1
    while (nm >= 0 && (character_at(signature, nm) == ' ' || character_at(signature, nm) == '\t')) { // nolint:PERF002,PERF003
        nm--
    }
    while (nm >= 0 && is_cpp_op_char(character_at(signature, nm))) { // nolint:PERF002,PERF003
        nm--
    }
    while (nm >= 0 && is_cpp_ident_char(character_at(signature, nm))) { // nolint:PERF002,PERF003
        nm--
    }
    let name_start = nm + 1
    return "{slice(signature, 0, name_start)}{prefix}{slice(signature, name_start)}"
}
