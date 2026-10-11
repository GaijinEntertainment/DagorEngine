// operator declarations dispatch through fixed-operand nodes - shapes no call
// site can honor (like defaulted parameters) must fail compilation
options gen2
options no_aot     // invokes the compiler at runtime (compile_file) — not AOT-linkable

require daslib/ast
require strings
require dastest/testing_boost public


def compile_issues(file_path : string) : string {
    var issuesText = ""
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(file_path, access, unsafe(addr(mg)), cop) $(_ok, _program, issues) {
                issuesText := issues
            }
        }
    }
    return issuesText
}

def expect_issue(t : T?; fixture, message : string) {
    let issues = compile_issues("{get_das_root()}/tests/typer_errors/{fixture}")
    t |> success(find(issues, message) >= 0,
        "expected '{message}' compiling {fixture}, got: {issues}")
}


[test]
def test_operator_declaration_errors(t : T?) {
    t |> run("operator with a defaulted parameter") @(t : T?) {
        t |> expect_issue("_operator_default_arg.das",
            "operator '+' can't have default arguments")
    }
}

// a property whose setter is private to another module reads as a plain rvalue
// at the use site; the copy and clone errors name the setter they could not use
[test]
def test_private_property_setter_hint(t : T?) {
    t |> run("copy into a property with a private setter") @(t : T?) {
        t |> expect_issue("_private_setter_copy.das",
            "property setter _private_setter_helper::operator .`flag`clone(self: _private_setter_helper::Gate; value: bool const): void at")
        t |> expect_issue("_private_setter_copy.das", "is private")
    }
    t |> run("clone into a property with a private setter") @(t : T?) {
        t |> expect_issue("_private_setter_clone.das", "can only clone to a reference")
        t |> expect_issue("_private_setter_clone.das", "property setter _private_setter_helper::operator .`flag`clone")
    }
}

// the `?` and `[` operator families lower to plain ExprCall, where a defaulted
// trailing parameter pads like any call - the reject above must not catch them
struct Boxed {
    v : int
}

def operator ??(a : Boxed?; b : Boxed; scale : int = 2) {
    return a != null ? *a : Boxed(v = b.v * scale)
}

def operator [] (b : Boxed; i : int; scale : int = 3) {
    return b.v * i * scale
}

[test]
def test_operator_defaults_that_pad(t : T?) {
    t |> run("defaulted `??` and `[]` overloads compile and pad") @(t : T?) {
        var p : Boxed?
        t |> equal((p ?? Boxed(v = 5)).v, 10)
        t |> equal(Boxed(v = 2)[3], 18)
    }
}
