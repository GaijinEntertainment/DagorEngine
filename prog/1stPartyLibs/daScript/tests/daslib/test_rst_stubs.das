options gen2
options rtti
options no_aot
options no_coverage

require dastest/testing_boost public
require daslib/rtti
require daslib/ast
require daslib/rst
require strings
require _rst_probe
require _rst_stub_mod_a
require _rst_stub_mod_b

[test]
def test_rst_substitution_stubs_span_every_module(t : T?) {
    let mods : array<Module?> = [find_module("_rst_stub_mod_a"), find_module("_rst_stub_mod_b")]
    let docs <- document_into_temp(t, mods, DocsHook())
    t |> run("the stub file keeps the first module's symbols too") @(t : T?) {
        t |> success(find(docs.stubs, "rst_stub_beta") >= 0, "last module missing: {docs.stubs}")
        t |> success(find(docs.stubs, "rst_stub_alpha") >= 0, "first module missing: {docs.stubs}")
    }
    t |> run("stub topic names are the ones the page reads back") @(t : T?) {
        for (name in fixed_array<string>("rst_stub_alpha", "rst_stub_alpha_two", "rst_stub_beta", "rst_stub_beta_two")) {
            let needle = "{name}-0x"
            t |> success(find(docs.stubs, needle) >= 0, "{name} stub carries no argument hash: {docs.stubs}")
        }
    }
    drop_run(docs)
}
