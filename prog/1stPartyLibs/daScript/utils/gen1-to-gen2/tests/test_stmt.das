options gen2
options indenting = 4
options rtti = true
options strict_smart_pointers = false

require dastest/testing_boost public
require daslib/fio
require strings
require daslib/strings_boost
require ../patch.das
require ../convert.das

//! Statement and declaration conversion: indentation becomes braces, conditions gain
//! parentheses, and the result has to compile as gen2 — which is the only check that really
//! says the rewrite worked.

let STMT = "{get_das_root()}/utils/gen1-to-gen2/tests/_fixture_gen1_stmt.das"
let DECL = "{get_das_root()}/utils/gen1-to-gen2/tests/_fixture_gen1_decl.das"


def private convert(file : string) : string {
    let res <- convert_file(file, ConvertOptions(v2 = true))
    return res.output
}


def private has(text, needle : string) : bool {
    return text |> find(needle) >= 0
}


[test]
def test_statement_braces(t : T?) {
    t |> run("blocks, conditions and the else/recover joins") @(t : T?) {
        let out = convert(STMT)
        t |> success(has(out, "def classify(n : int) \{"), "function body opens on the header line")
        t |> success(has(out, "for (i in range(n)) \{"), "for head is parenthesised whole")
        t |> success(has(out, "if (i % 2 == 0) \{"), "a condition is its subtree, not its operator")
        t |> success(has(out, "total += i // counted\n        \} else \{"),
            "else joins past a trailing comment")
        t |> success(has(out, "while (total > 100) \{"), "while condition")
        t |> success(has(out, "classify(3) // may fail\n    \} recover \{"),
            "recover joins past a trailing comment")
        t |> success(has(out, "def commented(n : int) \{ // what this one is for"),
            "a commented header still gets its brace")
        t |> success(has(out, "return n + 1 // and what it gives back\n\}"),
            "the closing brace clears a trailing comment")
        t |> success(has(out, "take_block <| $() \{"), "a bare piped block gains its block header")
        t |> success(has(out, "for (tt in a[0]) \{"), "an indexed for source is wrapped whole")
        t |> success(has(out, "$(@optional a : int)"), "an argument annotation becomes `@name`")
        t |> success(has(out, "var (lo, hi) = (1, 2)"), "tuple destructuring drops the make brackets")
        t |> success(has(out, "typeinfo sizeof(a)"), "the typeinfo trait moves out of the parens")
        t |> success(has(out, "let c = Color.green"), "an enumeration value takes the dot")
        t |> success(has(out, "let fl = Flags2.one"), "so does a bitfield value")
        t |> success(has(out, "reinterpret<int&>(h.b)"), "a cast becomes call-style")
        t |> success(has(out, "if (v is i && c == Color.green) \{"),
            "a variant test reaches past its field name")
        t |> success(has(out, "return (1, \"2\")"), "a multi-value return is one tuple")
        t |> success(has(out, "var blk = $ (a, b : int) \{"), "a piped literal drops the pipe")
        t |> success(has(out, "let braced_lambda = @ () \{\}"), "a braced literal drops the pipe too")
        t |> success(has(out, "static_if (GATE) \{"), "static_if takes parentheses")
    }

    t |> run("the converted statements compile as gen2") @(t : T?) {
        var issues = ""
        t |> success(compiles_as_gen2(STMT, convert(STMT), issues), "compiles: {issues}")
    }
}


//! Shapes only v1 spells, and only in a file the formatter never rewrites: they are written
//! to a temporary file rather than kept in a checked-in fixture.

[test]
def test_v1_only_shapes(t : T?) {
    t |> run("a comment where whitespace would be keeps the keyword's head") @(t : T?) {
        let temp = create_temp_file_result("commented_head", ".das")
        t |> success(temp is value, "temporary fixture created")
        if (!(temp is value)) return
        let path = unsafe(temp.value)
        t |> success(fwrite(path, "options gen2 = false\n\ndef f(n : int)\n    var acc = 0\n"
            + "    for/*i*/i in range(n)\n        if/*c*/i % 2 == 0\n            acc += i\n    return acc\n"),
            "temporary fixture written")
        let res <- convert_file(path, ConvertOptions(v2 = true))
        remove(path)
        t |> success(has(res.output, "for/*i*/(i in range(n)) \{"), "the for head is parenthesised")
        t |> success(has(res.output, "if/*c*/(i % 2 == 0) \{"), "and so is the if subject")
    }
    t |> run("a type function takes its type as an argument") @(t : T?) {
        let temp = create_temp_file_result("type_function", ".das")
        t |> success(temp is value, "temporary fixture created")
        if (!(temp is value)) return
        let path = unsafe(temp.value)
        t |> success(fwrite(path, "options gen2 = false\n\n[type_function]\n"
            + "def take_t ( a : auto(TT); a1, a2 : int )\n    return typeinfo(sizeof type<TT>) + a1 + a2\n\n"
            + "def f()\n    return take_t<int>(1, 2)\n"), "temporary fixture written")
        let res <- convert_file(path, ConvertOptions(v2 = true))
        remove(path)
        t |> success(has(res.output, "take_t(type<int>, 1, 2)"),
            "the type moves into the argument list ({res.output})")
    }
    t |> run("a continued initializer keeps its lines in one expression") @(t : T?) {
        let temp = create_temp_file_result("continued_init", ".das")
        t |> success(temp is value, "temporary fixture created")
        if (!(temp is value)) return
        let path = unsafe(temp.value)
        t |> success(fwrite(path, "def main() \{\n    let x = 1\n+\n2;\n\}\n"), "temporary fixture written")
        let res <- convert_file(path, ConvertOptions(v2 = true))
        remove(path)
        t |> success(res.output |> find("let x = (1\n+\n2);") >= 0,
            "the whole expression is parenthesised ({res.output})")
    }
}


[test]
def test_declaration_braces(t : T?) {
    t |> run("declaration bodies") @(t : T?) {
        let out = convert(DECL)
        t |> success(has(out, "enum Color \{"), "enum body")
        t |> success(has(out, "bitfield Flags \{"), "bitfield body")
        t |> success(has(out, "struct Point \{"), "struct body")
        t |> success(has(out, "class Shape \{"), "class body")
        t |> success(has(out, "def area() : int \{"), "a method body is braced once, not twice")
        t |> success(has(out, "typedef IntAlias = int"), "first alias")
        t |> success(has(out, "typedef StrAlias = string"), "second alias")
        t |> success(!has(out, "typedef\n"), "the bare header line is gone")
    }

    t |> run("the converted declarations compile as gen2") @(t : T?) {
        var issues = ""
        t |> success(compiles_as_gen2(DECL, convert(DECL), issues), "compiles: {issues}")
    }
}


[test]
def test_statements_keep_comments(t : T?) {
    t |> run("no comment is lost converting statements or declarations") @(t : T?) {
        for (f in [STMT, DECL]) {
            let res <- convert_file(f, ConvertOptions(v2 = true))
            t |> equal(res.failed, "", "{f} converted")
            t |> equal(length(res.refused), 0, "{f}: no refused edits")
            t |> success(check_comments_preserved(fread(f), res.output).ok, "{f}: comments conserved")
        }
    }
}
