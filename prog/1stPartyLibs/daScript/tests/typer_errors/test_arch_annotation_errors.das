// [arch] owns the SHAPE of a citation and nothing else: exactly one `at=` string, carrying a
// document path and an anchor, both non-empty, separated by '#'. Whether the citation resolves
// to a real section is LINT026's question (utils/lint/main.das), which is why a well-shaped
// citation to a document that is not there still compiles.
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
def test_arch_citation_shape_errors(t : T?) {
    t |> run("[arch] with no citation") @(t : T?) {
        t |> expect_issue("_arch_no_argument.das",
            "expecting exactly one argument, at=\"<doc>.md#<anchor>\"")
    }
    t |> run("[arch] citing the empty string") @(t : T?) {
        t |> expect_issue("_arch_empty_citation.das",
            "citation \"\" is not \"<doc>.md#<anchor>\"")
    }
    t |> run("[arch] citing a document with no anchor") @(t : T?) {
        t |> expect_issue("_arch_no_hash.das",
            "citation \"ARCHITECTURE.md\" is not \"<doc>.md#<anchor>\"")
    }
    t |> run("[arch] with a named argument that is not at") @(t : T?) {
        t |> expect_issue("_arch_wrong_name.das",
            "expecting exactly one argument, at=\"<doc>.md#<anchor>\"")
    }
    t |> run("[arch] with a non-string argument") @(t : T?) {
        t |> expect_issue("_arch_wrong_type.das",
            "expecting exactly one argument, at=\"<doc>.md#<anchor>\"")
    }
    t |> run("[arch] with a second argument") @(t : T?) {
        t |> expect_issue("_arch_two_args.das",
            "expecting exactly one argument, at=\"<doc>.md#<anchor>\"")
    }
    t |> run("[arch] citing an anchor with no document half") @(t : T?) {
        t |> expect_issue("_arch_no_doc_half.das",
            "citation \"#x\" is not \"<doc>.md#<anchor>\"")
    }
    t |> run("[arch] citing a document with an empty anchor half") @(t : T?) {
        t |> expect_issue("_arch_no_anchor_half.das",
            "citation \"ARCHITECTURE.md#\" is not \"<doc>.md#<anchor>\"")
    }
}
