options gen2
options indenting = 4
options rtti = true
options strict_smart_pointers = false

require dastest/testing_boost public
require daslib/fio
require strings
require daslib/strings_boost
require ../convert.das

//! v1 spellings whose rewrite is not a delimiter substitution.
//!
//! Each of these came back from converting a game's script tree in place: the output either did
//! not parse or lost a brace. They are written to temporary files rather than to a checked-in
//! fixture, because the formatter rewrites most of them on sight.

struct private Shape {
    name : string
    source : string
}


def private has(text, needle : string) : bool {
    return text |> find(needle) >= 0
}


def private convert_shape(t : T?; shape : Shape; var out : string&) : bool {
    let temp = create_temp_file_result(shape.name, ".das")
    t |> success(temp is value, "{shape.name}: temporary fixture created")
    if (!(temp is value)) return false
    let path = unsafe(temp.value)
    if (!(t |> success(fwrite(path, shape.source), "{shape.name}: temporary fixture written"))) {
        remove(path)
        return false
    }
    let res <- convert_file(path, ConvertOptions(v2 = true))
    out = res.output
    t |> success(empty(res.refused), "{shape.name}: no edit refused")
    var issues = ""
    let ok = compiles_as_gen2(path, res.output, issues)
    t |> success(ok, "{shape.name}: compiles as gen2: {issues}")
    remove(path)
    return ok
}


[test]
def test_else_with_trailing_comment(t : T?) {
    t |> run("a comment after `else` leaves room for the brace") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "else_comment", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def f(a : int) : int\n    if a > 0\n        return 1\n"
            + "    else // which branch this is\n        return 2\n        return 3\n"), out)
        t |> success(has(out, "\} else \{ // which branch this is"),
            "the brace goes in front of the comment, not after it")
    }
}


[test]
def test_global_let_block(t : T?) {
    t |> run("a bare `let` heading a block of globals takes braces") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "global_let", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "let\n    A = 1\n    B = 2\n\n"
            + "var private\n    C = 3\n\n"
            + "var // the header carries a comment of its own\n    E = 5\n\n"
            + "let D = 4\n\ndef f\n    return A + B + C + D + E\n"), out)
        t |> success(has(out, "let \{"), "a `let` block opens")
        t |> success(has(out, "var private \{"), "a modifier stays on the header")
        t |> success(has(out, "var \{ // the header carries a comment of its own"),
            "a header comment does not stop the brace")
        t |> success(has(out, "let D = 4") && !has(out, "let D = 4 \{"),
            "a `let` that declares its own variable is not a block")
    }
}


[test]
def test_typeinfo_condition(t : T?) {
    t |> run("a `typeinfo` condition reaches past its trait") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "typeinfo_cond", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def gen(a : auto(TT))\n    static_if typeinfo is_string(type<TT>)\n"
            + "        return 1\n    return 0\n"), out)
        t |> success(has(out, "static_if (typeinfo is_string(type<TT>)) \{"),
            "the whole typeinfo expression is the condition")
    }
}


[test]
def test_call_macro_body(t : T?) {
    t |> run("a quoted body is converted like any other block") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "qmacro_body", source =
            "options gen2 = false\noptions indenting = 4\noptions rtti\n\n"
            + "require daslib/ast_boost\nrequire daslib/templates_boost\n\n"
            + "def make_it\n    return <- qmacro_block() <|\n        if true\n"
            + "            print(\"x\")\n"), out)
        t |> success(has(out, "if (true) \{"), "the quoted body gets its braces")
    }
}


[test]
def test_piped_make_literal(t : T?) {
    t |> run("a make-literal at the head of a pipe becomes an argument") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "piped_make", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def has_it(w : string)\n    return \{\"a\", \"b\"\} |> key_exists(w)\n\n"
            + "def count_it\n    return \{\"a\", \"b\"\} |> length()\n\n"
            + "def keep(a : array<int>; w : int)\n    return a |> find_index(w)\n"), out)
        t |> success(has(out, "return key_exists(\{\"a\", \"b\"\}, w)"),
            "the literal moves into the argument list")
        t |> success(has(out, "return length(\{\"a\", \"b\"\})"),
            "a call with no other argument needs no comma")
        t |> success(has(out, "return a |> find_index(w)"),
            "a pipe that is not headed by a literal is left alone")
    }
}


[test]
def test_stacked_block_closers(t : T?) {
    t |> run("blocks that end on one line each keep their brace") @(t : T?) {
        var out = ""
        let ok = convert_shape(t, Shape(name = "stacked_closers", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def f(a : array<int>; n : int)\n    if n > 0\n        for i in a\n"
            + "            if i > 0\n                print(\"\{i\}\")\n    return\n"), out)
        t |> success(ok, "the stacked closers balance")
    }
}


[test]
def test_indexed_condition(t : T?) {
    t |> run("a condition anchored on its index is not wrapped from there") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "indexed_cond", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "struct Rec\n    node : int\n\n"
            + "def f(arr : array<Rec>; m : int)\n"
            + "    if ((arr[0].node == m &&\n         arr[0].node == m) ||\n         m > 3)\n"
            + "        return true\n    return false\n"), out)
        t |> success(!has(out, "[("), "no parenthesis opens inside the brackets")
    }
}


[test]
def test_for_head_from_keyword(t : T?) {
    t |> run("the for head starts after the keyword, not at the first iterator") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "for_head", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def f(a : array<int>; b : array<int>)\n    for x, y in a, b\n"
            + "        print(\"\{x\}\{y\}\")\n"), out)
        t |> success(has(out, "for (x, y in a, b) \{"), "both iterators are inside the head")
    }
}


[test]
def test_literal_header_forms(t : T?) {
    t |> run("a capture list or an annotation still belongs to the literal") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "literal_header", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def f\n    var templ = \"t\"\n"
            + "    let c <- @ <| capture(<- templ) (reid : int)\n        print(\"\{reid\}\")\n"
            + "    let d <- $ <| [unused_argument(w)] (w : int)\n        return 1\n"
            + "    invoke(c, 1)\n    return invoke(d, 1)\n"), out)
        t |> success(!has(out, "<|"), "the pipe goes away in both forms")
    }
}


[test]
def test_multiline_initializer(t : T?) {
    t |> run("a block in an initializer is not a continued expression") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "multiline_init", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def f\n"
            + "    let a <- @ <| (eid : int)\n        // what this one does\n"
            + "        print(\"\{eid\}\")\n"
            + "    invoke(a, 1)\n"), out)
        t |> success(!has(out, "(eid : int)("), "the lambda is not wrapped in parentheses")
    }
}

[test]
def test_bodyless_declaration(t : T?) {
    t |> run("a declaration with no body still gets one") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "bodyless_decl", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "struct Empty\n\nstruct WithBody\n    a : int\n\n"
            + "struct AlreadyBraced \{\}\n\nstruct BracedNextLine\n\{\}\n\n"
            + "typedef\n    Alias = int\n"), out)
        t |> success(has(out, "struct Empty \{\}"), "an empty struct gets an empty body")
        t |> success(!has(out, "struct BracedNextLine \{\}"),
            "braces already on the next line are a body")
        t |> success(has(out, "struct WithBody \{"), "a struct with a body is unaffected")
        t |> success(has(out, "typedef Alias = int") && !has(out, "typedef Alias = int \{\}"),
            "a typedef has no body to give")
    }
}

[test]
def test_match_subject(t : T?) {
    t |> run("a call macro spelled as a keyword parenthesises its subject") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "match_subject", source =
            "options gen2 = false\noptions indenting = 4\n\nrequire daslib/match\n\n"
            + "def pick(v : int) : float\n    match v\n        if 1\n            return 0.75\n"
            + "        if _\n            return 1.3\n    return 0.0\n"), out)
        t |> success(has(out, "match(v) \{"), "the subject takes parentheses")
    }
}


[test]
def test_comment_does_not_end_a_body(t : T?) {
    t |> run("a comment between methods does not close the class") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "comment_banner", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "class Canvas\n    origin : float2\n\n    def reset\n"
            + "        origin = float2(0.0, 0.0)\n\n\n"
            + "// *** a banner at column 0 ***\n\n"
            + "    def setOrigin(o : float2)\n        origin = o\n"), out)
        t |> success(has(out, "    \}\n\}"), "the class closes after the last method")
    }
}


[test]
def test_make_literal_closer(t : T?) {
    t |> run("a make-literal reaches past its closing bracket") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "make_closer", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def f(n : int)\n    for k in [n - 1, n + 1]\n        print(\"\{k\}\")\n"), out)
        t |> success(has(out, "for (k in [n - 1, n + 1]) \{"), "the head closes after the bracket")
    }
}


[test]
def test_static_elif(t : T?) {
    t |> run("static_elif takes parentheses like static_if") @(t : T?) {
        var out = ""
        convert_shape(t, Shape(name = "static_elif", source =
            "options gen2 = false\noptions indenting = 4\n\n"
            + "def gen(value : auto(TT))\n"
            + "    static_if typeinfo is_string(type<TT>)\n        return 1\n"
            + "    static_elif typeinfo is_array(type<TT>)\n        return 2\n    return 0\n"), out)
        t |> success(has(out, "static_elif (typeinfo is_array(type<TT>)) \{"),
            "the elif form is a condition too")
    }
}


[test]
def test_closer_and_generators(t : T?) {
    t |> run("an index next to the closer is not the closer") @(t : T?) {
        var out = ""
        if (convert_shape(t, Shape(name = "index_before_closer", source =
                "options gen2 = false\n\nstruct Foo\n    a : int\n\n"
                + "def f(arr : array<int>)\n    var v = [[Foo a=arr[0]]]\n    return v.a\n"), out)) {
            t |> success(has(out, "Foo( a=arr[0])"), "the whole index survives ({out})")
        }
    }

    t |> run("both generator spellings stay generators") @(t : T?) {
        var out = ""
        if (convert_shape(t, Shape(name = "generator_spellings", source =
                "options gen2 = false\n\ndef g()\n"
                + "    var c <- \{\{for x in range(3); x*2\}\}\n"
                + "    var d <- \{\{for x in range(3); x => x*2\}\}\n"
                + "    var e <- [\{for x in range(3); x*2\}]\n    return length(e)\n"), out)) {
            t |> success(has(out, "var c <- [iterator for (x in range(3)); x*2]"), "no-arrow stays an iterator")
            t |> success(has(out, "var d <- [iterator for (x in range(3)); x => x*2]"), "arrow stays an iterator")
            t |> success(has(out, "var e <- [for (x in range(3)); x*2]"), "the array spelling stays an array")
        }
    }
}
