options gen2
options no_unused_function_arguments = false

require dastest/testing_boost public

// ---- functions under test ----------------------------------------------------

// mixed positional + a trailing default; used for the flagship bare form
def three(a : int; hint : string; extra : int = 7) : string {
    return "{a}/{hint}/{extra}"
}

// four positional floats + a named hint -- the tool_slider(...) shape
def slider4(name : string; val, lo, hi : float; hint : string) : string {
    return "{name}:{hint}:{int(val)}:{int(lo)}:{int(hi)}"
}

// overload set disambiguated by the named argument's name + type
def over(a : int; kind : string) : string { return "{a}:S:{kind}" }
def over(a : int; count : int) : string { return "{a}:I:{count}" }

// generic first argument + a named argument
def gen_named(a; hint : string) : string { return "{a}:{hint}" }

// a named call INSIDE a generic body: the body is cloned (clone_function) on instantiation,
// before the named call demotes - the clone must carry the positional in nonNamedArguments,
// else 'g' is dropped and matching fails "can't skip hint" (regression for ExprNamedCall::clone)
def in_generic(g) : string { return three(g, hint = "gen") }

// mutable-ref param: lets clone (:=) be observed vs by-ref (= / <-).
// _tag is a leading positional so the array can be bound as a named arg.
def drain(_tag : int; var a : array<int>) { a |> clear() }

// piped block as the trailing positional, with a named option after it
def section(_tag : int; body : block<() : int>; open : bool = false) : int {
    return invoke(body) + (open ? 1000 : 0)
}

// method-style (dot) call target
struct Widget { id : int }
def render(self : Widget; title : string; depth : int = 0) : string {
    return "W{self.id}:{title}:{depth}"
}

// a name that is BOTH a struct and a function (struct construction must win)
struct SameName { p : int }
def SameName(p : int; q : int) : string { return "fn:{p}:{q}" }

// genuine class method (def inside class {}) - a middle positional after self plus a
// named/defaulted arg. render() above is a FREE function via dot, so it resolves through the
// regular lookup; a class method is a struct field, unreachable by bare name, so it goes through
// the member-call path where middle positionals after self used to be dropped.
class Gadget {
    id : int
    def paint(title : string; depth : int = 0) : string {
        return "G{id}:{title}:{depth}"
    }
    // calls its own method with implicit self
    def repaint(title : string) : string {
        return paint(title, depth = 5)
    }
}

// ---- tests -------------------------------------------------------------------

[test]
def test_bare_mixed(t : T?) {
    // bracket-less mixed named call
    t |> equal(three(1, hint = "bare"), "1/bare/7")
    // multiple named args (must be given in parameter order)
    t |> equal(three(2, hint = "multi", extra = 9), "2/multi/9")
    // the flagship 4-positional + named-hint shape
    t |> equal(slider4("radius", 5.0, 0.1, 50.0, hint = "scatter"), "radius:scatter:5:0:50")
}

[test]
def test_bracketed_still_works(t : T?) {
    // the existing bracketed forms remain valid (regression)
    t |> equal(three(3, [hint = "brk"]), "3/brk/7")
    t |> equal(three(4, [hint = "brk", extra = 5]), "4/brk/5")
}

[test]
def test_named_move_clone(t : T?) {
    // '=' binds by ref to the var param -> source is drained
    var s1 <- [1, 2, 3]
    drain(0, a = s1)
    t |> equal(length(s1), 0)
    // ':=' clones -> the callee drains a copy, source is preserved
    var s2 <- [1, 2, 3]
    drain(0, a := s2)
    t |> equal(length(s2), 3)
    // '<-' moves (consume_argument), same as the positional foo(<- x)
    var s3 <- [1, 2, 3]
    drain(0, a <- s3)
    t |> equal(length(s3), 0)
}

[test]
def test_overload_by_named(t : T?) {
    t |> equal(over(1, kind = "hi"), "1:S:hi")
    t |> equal(over(1, count = 9), "1:I:9")
}

[test]
def test_generic_named(t : T?) {
    t |> equal(gen_named(42, hint = "g"), "42:g")
    t |> equal(gen_named("hey", hint = "s"), "hey:s")
}

[test]
def test_piped_block_named(t : T?) {
    // piped block on a named call: block fills the body slot, open is named after it
    let r = section(5, open = true) {
        return 42
    }
    t |> equal(r, 1042)
    // without the named option (default)
    let r2 = section(6) {
        return 8
    }
    t |> equal(r2, 8)
}

[test]
def test_method_bare(t : T?) {
    let w = Widget(id = 7)
    t |> equal(w.render("hdr", depth = 2), "W7:hdr:2")
}

[test]
def test_method_arrow(t : T?) {
    // the '->' arrow method-call form parses to the same ExprNamedCall as '.'
    let w = Widget(id = 7)
    t |> equal(w->render("hdr", depth = 2), "W7:hdr:2")
    // all-named bracketed '->' form (rule mirrors the '.' bracketed rule 'm([name=v])')
    t |> equal(w->render([title = "brk", depth = 3]), "W7:brk:3")
}

[test]
def test_method_all_named(t : T?) {
    // pure all-named method call, no positional and no brackets -- both '.' and '->'
    let w = Widget(id = 7)
    t |> equal(w.render(title = "hi"), "W7:hi:0")
    t |> equal(w->render(title = "hi"), "W7:hi:0")
    // multiple named args, still no positional (self is the only non-named arg)
    t |> equal(w.render(title = "yo", depth = 3), "W7:yo:3")
    t |> equal(w->render(title = "yo", depth = 3), "W7:yo:3")
}

[test]
def test_all_named(t : T?) {
    // pure all-named (no positional at all) resolves as a function call
    t |> equal(three(a = 1, hint = "an"), "1/an/7")
    t |> equal(three(a = 2, hint = "an", extra = 9), "2/an/9")
    // all-named still picks the overload by named-arg name + type
    t |> equal(over(a = 5, kind = "z"), "5:S:z")
}

[test]
def test_struct_beats_function(t : T?) {
    // SameName is both a struct and a function; all-named construction builds the STRUCT
    let s = SameName(p = 7)
    t |> equal(s.p, 7)
}

[test]
def test_class_method_bare(t : T?) {
    var g = new Gadget(id = 3)
    // middle positional "hdr" after self + named depth -> the positional is kept (was dropped)
    t |> equal(g.paint("hdr", depth = 2), "G3:hdr:2")
    // implicit-self member call with a middle positional + named arg
    t |> equal(g.repaint("hi"), "G3:hi:5")
    unsafe { delete g }
}

[test]
def test_class_method_arrow(t : T?) {
    var g = new Gadget(id = 3)
    // '->' on a genuine class method: middle positional after self + named depth
    t |> equal(g->paint("hdr", depth = 2), "G3:hdr:2")
    unsafe { delete g }
}

[test]
def test_class_method_all_named(t : T?) {
    var g = new Gadget(id = 3)
    // pure all-named on a genuine class method, no positional -- both '.' and '->'
    t |> equal(g.paint(title = "hi"), "G3:hi:0")
    t |> equal(g->paint(title = "hi"), "G3:hi:0")
    unsafe { delete g }
}

[test]
def test_named_call_in_generic(t : T?) {
    // the named call lives in a generic body; its positional must survive the clone
    t |> equal(in_generic(1), "1/gen/7")
}
