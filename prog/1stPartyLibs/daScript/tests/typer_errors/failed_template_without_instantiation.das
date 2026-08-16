// Regression test for issue #3065 (fuzzer): a `struct template` used directly
// as a type (here a function parameter) without going through its typemacro
// instantiation slipped past infer and tripped the DAS_VERIFYF in
// makeStructureDebugInfo ("cannot make debug info for template structure").
// verifyType now rejects it cleanly with invalid_structure_template (30241).
options gen2
expect 30241

struct template Foo {
    x : int
}

[export]
def use_template(r : Foo) {}
