options gen2
require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require math

// The --jit-debug rail's DWARF, read off the emitted IR rather than off a debugger.
// Shape and reasons: ARCHITECTURE_DEBUG_INFO.md sec.12.

let private ROOT = "modules/dasLLVM/tests/_debug_info_root.das"

def private das_exe() : string {
    let argv <- get_command_line_arguments()
    return find(to_generic_path(argv[0]), "/") < 0 ? argv[0] : get_full_file_name(normalize(argv[0]))
}

def private out_tail(s : string) : string {
    return slice(s, max(0, length(s) - 600))
}

def private count_of(s : string; needle : string) : int {
    var n = 0
    var at = find(s, needle, 0)
    while (at >= 0) {
        n++
        at = find(s, needle, at + length(needle))
    }
    return n
}

//! the dumped IR of one function, by its dumped name - signature line to closing brace
def private function_ir(dump : string; name : string) : string {
    let start = find(dump, "`{name}\"(")
    return "" if (start < 0)
    let body = slice(dump, start)
    let close = find(body, "\n}")
    return close < 0 ? body : slice(body, 0, close)
}

//! the entry block of a dumped function: everything before its first branch
def private entry_block_ir(fn_ir : string) : string {
    let br = find(fn_ir, "\n  br ")
    return br < 0 ? fn_ir : slice(fn_ir, 0, br)
}

//! the fixture's own DIFile line, whichever metadata number it landed on
def private fixture_difile(dump : string) : string {
    var lines <- split(dump, "\n")
    for (l in lines) {
        if (find(l, "!DIFile(filename: \"") >= 0 && find(l, base_name(ROOT)) >= 0) {
            return "{l}"
        }
    }
    return ""
}

let private HOT_IMPL = "hot Ci Cf CS`Lt`Point`Gt` CE`Lt`::Mode`Gt` Cs implementation"

let private FIXTURE_FOR_STATEMENT = "for (i in range(n))"

//! the fixture's block nesting: function body > for body > if body
let private FIXTURE_NESTED_BLOCKS = 3

//! the 1-based line a fixture statement sits on, so a fixture edit moves an assert with it
//! instead of silently pointing it at the wrong line
def private fixture_line_of(needle : string) : int {
    var lines <- split(fread(ROOT), "\n")
    for (l, ix in lines, count(1)) {
        if (find(l, needle) >= 0) {
            return ix
        }
    }
    return -1
}

let private FIXTURE_ARGS = ["n", "scale", "p", "m", "tag"]
let private FIXTURE_LOCALS = ["acc", "f4", "i", "inner", "acc_ref", "spin", "dim3", "xs", "pair", "head", "node", "blocked"]

//! one labelled basic block of a dumped function, label line through the blank line after it
def private named_block_ir(fn_ir : string; block_name : string) : string {
    let start = find(fn_ir, "\n{block_name}:")
    return "" if (start < 0)
    let body = slice(fn_ir, start + 1)
    let close = find(body, "\n\n")
    return close < 0 ? body : slice(body, 0, close)
}

//! the source line of the first !dbg attachment in a piece of IR, -1 when it carries none
def private dbg_line_of(dump : string; ir : string) : int {
    let at = find(ir, ", !dbg !")
    return -1 if (at < 0)
    let rest = slice(ir, at + length(", !dbg "))
    let eol = find(rest, "\n")
    let mdref = eol < 0 ? rest : slice(rest, 0, eol)
    let decl = find(dump, "\n{mdref} = !DILocation(line: ")
    return -1 if (decl < 0)
    let after = slice(dump, decl + length("\n{mdref} = !DILocation(line: "))
    let comma = find(after, ",")
    return comma < 0 ? -1 : to_int(slice(after, 0, comma))
}

def private jit_dump(t : T?; debug_info : bool) : string {
    var args <- [das_exe(), "-no-module-cache", "-jit", "-jit-no-cache", ROOT, "--",
                 "--jit-opt-level=0", "--jit-dump"]
    if (debug_info) {
        args |> push("--jit-debug")
    }
    var out = ""
    let rc = run_and_capture(args, out, 300.0)
    t |> equal(0, rc, "the fixture runs under -jit: {out_tail(out)}")
    t |> success(find(out, "DEBUG_INFO_ROOT ") >= 0, "the fixture printed its witness: {out_tail(out)}")
    return out
}

[test]
def test_debug_info_is_off_by_default(t : T?) {
    t |> run("no --jit-debug means no debug metadata at all") @(t : T?) {
        let out = jit_dump(t, false)
        t |> equal(0, count_of(out, "!DICompileUnit"), "a default run emits no compile unit")
        t |> equal(0, count_of(out, "!DILocalVariable"), "a default run emits no variable metadata")
        t |> equal(0, count_of(out, "#dbg_declare("), "a default run emits no dbg records")
    }
}

[test]
def test_one_compile_unit_per_module(t : T?) {
    t |> run("the whole module shares one compile unit, tagged C++") @(t : T?) {
        let out = jit_dump(t, true)
        t |> equal(1, count_of(out, "distinct !DICompileUnit"),
            "one compile unit per module - a unit per function hides an inlined instance from a debugger's name lookup")
        t |> success(find(out, "language: DW_LANG_C_plus_plus") >= 0,
            "DW_LANG_C would make a debugger treat the unmangled name as the whole truth about the symbol")
    }
}

[test]
def test_subprograms_carry_the_das_name(t : T?) {
    t |> run("a das function is named as its source names it, with no linkage name") @(t : T?) {
        let out = jit_dump(t, true)
        t |> equal(2, count_of(out, "!DISubprogram(name: \"hot\""),
            "both halves of the function pair carry the das name:\n{out_tail(out)}")
        t |> equal(0, count_of(out, "linkageName:"),
            "a linkage name becomes the lookup key a debugger matches, and the generated symbol is unreachable by hand")
        t |> success(find(out, "!DISubprogram(name: \"main\"") >= 0, "main is named too")
    }
}

[test]
def test_locals_and_arguments_are_described(t : T?) {
    t |> run("every argument and local has a DILocalVariable in a lexical scope") @(t : T?) {
        let out = jit_dump(t, true)
        for (arg, ix in FIXTURE_ARGS, count(1)) {
            t |> success(find(out, "!DILocalVariable(name: \"{arg}\", arg: {ix}") >= 0,
                "argument {arg} is described as parameter {ix}:\n{out_tail(out)}")
        }
        for (loc in FIXTURE_LOCALS) {
            t |> success(find(out, "!DILocalVariable(name: \"{loc}\"") >= 0,
                "local {loc} is described:\n{out_tail(out)}")
        }
        t |> success(count_of(out, "distinct !DILexicalBlock") >= FIXTURE_NESTED_BLOCKS,
            "one lexical block per open das block:\n{out_tail(out)}")
        t |> success(count_of(out, "#dbg_declare(") >= length(FIXTURE_ARGS) + length(FIXTURE_LOCALS),
            "one dbg record per described argument and local")
    }
}

[test]
def test_a_global_is_described(t : T?) {
    t |> run("a das global reads in every function that touches it") @(t : T?) {
        let out = jit_dump(t, true)
        //! a global has no static address - it is described at the pointer the function computed
        //! from its own ctx, so it lands as a variable of that function (arch doc sec.12.4)
        t |> success(find(out, "!DILocalVariable(name: \"g_calls\"") >= 0,
            "the fixture's global is described:\n{out_tail(out)}")
        t |> equal(-1, find(out, "!DIGlobalVariableExpression"),
            "no static-address global expression - one address cannot stand for every Context")
    }
}

[test]
def test_types_are_described(t : T?) {
    t |> run("a struct, an enum, a string and a vector each reach DWARF as themselves") @(t : T?) {
        let out = jit_dump(t, true)
        t |> success(find(out, "tag: DW_TAG_structure_type, name: \"Point") >= 0,
            "the struct is a real structure type, not an opaque blob:\n{out_tail(out)}")
        t |> success(find(out, "tag: DW_TAG_member, name: \"x\"") >= 0, "member x is named")
        t |> success(find(out, "tag: DW_TAG_member, name: \"y\"") >= 0 && find(out, "name: \"y\", scope") >= 0,
            "member y is named")
        t |> success(find(out, "!DIEnumerator(name: \"slow\", value: 1)") >= 0,
            "an enum carries its entries, so a debugger prints the name and not the number")
        t |> success(find(out, "!DIBasicType(name: \"char\", size: 8, encoding: DW_ATE_signed_char)") >= 0,
            "a das string is char* - the shape every debugger already prints as text")
        //! a scalar's encoding and a member's byte offset are the two things a debugger prints
        //! wrong if the factory is wrong, so both are pinned rather than only the shapes
        t |> success(find(out, "!DIBasicType(name: \"int\", size: 32, encoding: DW_ATE_signed)") >= 0,
            "int is a signed 32-bit base type:\n{out_tail(out)}")
        t |> success(find(out, "!DIBasicType(name: \"float\", size: 32, encoding: DW_ATE_float)") >= 0,
            "float is a 32-bit float base type")
        t |> success(find(out, "name: \"y\", scope: !") >= 0 && find(out, "offset: 32") >= 0,
            "a member sits at its own byte offset, not at zero:\n{out_tail(out)}")
        t |> success(find(out, "tag: DW_TAG_array_type") >= 0 && find(out, "!DISubrange(count: 3") >= 0,
            "a dim is an array type with one subrange per dimension")
        t |> success(find(out, "flags: DIFlagFwdDecl") >= 0,
            "a pointer back into the type under construction takes a forward declaration")
        t |> success(find(out, "identifier: \"array<int>") >= 0,
            "a container reaches DWARF as a named opaque declaration")
        t |> success(find(out, "DW_OP_deref") >= 0,
            "a ref local's slot holds a pointer, so its location derefs once")
        t |> success(find(out, "flags: DIFlagVector") >= 0,
            "float4 is a vector of lanes, not one opaque value:\n{out_tail(out)}")
        t |> equal(0, count_of(out, "<temporary!>"), "no unresolved placeholder survives into the module")
    }
}

[test]
def test_source_paths_are_absolute(t : T?) {
    t |> run("a DIFile names an absolute path, so a debugger lists the source from any cwd") @(t : T?) {
        let out = jit_dump(t, true)
        let line = fixture_difile(out)
        t |> success(!empty(line), "the dump carries a DIFile for the fixture:\n{out_tail(out)}")
        let open = find(line, "\"")
        let close = find(line, "\"", open + 1)
        let path = open < 0 || close < 0 ? "" : slice(line, open + 1, close)
        t |> success(is_absolute(path), "the DIFile filename is absolute, got '{path}'")
        t |> success(find(line, "directory: \"\"") >= 0,
            "the directory field stays empty - CodeView consumers join the two and would print dir/dir/file")
    }
}

[test]
def test_hoisted_instructions_carry_no_stale_line(t : T?) {
    t |> run("the prologue carries no line, and every argument is declared in it") @(t : T?) {
        let out = jit_dump(t, true)
        let ir = function_ir(out, HOT_IMPL)
        t |> success(!empty(ir), "the dump carries hot's implementation:\n{out_tail(out)}")
        let entry = entry_block_ir(ir)
        t |> equal(-1, find(entry, ", !dbg !"),
            "no instruction in the entry block carries a line:\n{entry}")
        t |> success(count_of(entry, "#dbg_declare(") >= 5,
            "all five arguments are declared in the entry block:\n{entry}")
    }
    t |> run("a loop latch carries the loop's own line, not its body's last statement") @(t : T?) {
        let out = jit_dump(t, true)
        let ir = function_ir(out, HOT_IMPL)
        t |> success(!empty(ir), "the dump carries hot's implementation:\n{out_tail(out)}")
        let latch = named_block_ir(ir, "for_continue")
        t |> success(!empty(latch), "the dump carries the for latch:\n{ir}")
        let forLine = fixture_line_of(FIXTURE_FOR_STATEMENT)
        t |> success(forLine > 0, "the fixture still carries `{FIXTURE_FOR_STATEMENT}`")
        t |> equal(forLine, dbg_line_of(out, latch),
            "the latch reports the `for` line {forLine}:\n{latch}")
    }
}
