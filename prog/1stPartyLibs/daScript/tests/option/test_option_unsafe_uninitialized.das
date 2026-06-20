options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/option

// Regression for typemacro+tuple Option carrying a payload that's
// "unsafe-when-uninitialized" — i.e. a struct with hasInitFields = true.
// Without TypeDeclFlags.safeWhenUninitialized on the value field,
// default<Option<Brittle>> would trip ast_infer_type.cpp:5032's
// "Uninitialized variable" check.

struct Brittle {
    must_init : int = 42
    name : string = "default"
}

[test]
def option_brittle_constructors(t : T?) {
    t |> run("default<Option<Brittle>> compiles and reads as none") @(t : T?) {
        let o = none(type<Brittle>)
        t |> success(o |> is_none)
        t |> success(!(o |> is_some))
    }
    t |> run("bare var Option<Brittle> compiles via @safe_when_uninitialized on _value") @(t : T?) {
        // Without @safe_when_uninitialized on the _value field of the tuple
        // produced by [template_tuple(T)] in daslib/option.das, this would
        // trigger "Uninitialized variable is unsafe" at ast_infer_type.cpp:5040.
        // The bare var (no init) is the only form that exercises this check —
        // ``let`` requires an initializer.
        var bare : Option<Brittle> // nolint:LINT003
        t |> success(bare |> is_none)
    }
    t |> run("some(Brittle(...)) round-trips") @(t : T?) {
        let o = some(Brittle(must_init = 7, name = "x"))
        t |> success(o |> is_some)
        let v = o |> unwrap
        t |> equal(v.must_init, 7)
        t |> equal(v.name, "x")
    }
}

[test]
def option_brittle_unwrap_or_default(t : T?) {
    t |> run("unwrap_or_default returns default<Brittle> on none") @(t : T?) {
        let o = none(type<Brittle>)
        let v = o |> unwrap_or_default
        t |> equal(v.must_init, 42)
        t |> equal(v.name, "default")
    }
}
