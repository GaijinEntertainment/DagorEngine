options gen2
options rtti
options no_aot
options no_coverage

require dastest/testing_boost public
require daslib/rtti
require daslib/ast
require daslib/rst
require daslib/regex
require daslib/regex_boost
require daslib/fio
require strings
require daslib/strings_boost
require _rst_probe
require _rst_digest_mod

def private write_twin(root, key, text : string) {
    let path = "{root}/handmade/handmade/{key}.rst"
    mkdir_rec(dir_name(path))
    fwrite(path, text)
}

def private bom() : string {
    let bytes <- [uint8(0xEF), uint8(0xBB), uint8(0xBF)]
    return string(bytes)
}

def private has_line(text, line : string) : bool {
    return find(text, "\n{line}\n") >= 0
}

[test]
def test_rst_digest_rows(t : T?) {
    var mod = find_module("_rst_digest_mod")
    let mods : array<Module?> = [mod]
    var two_arg_key = ""
    var lonely_key = ""
    for_each_function(mod, "") $(fn) {
        if (fn.name == "digest_pair" && length(fn.arguments) == 2) {
            two_arg_key = "function-_rst_digest_mod-{function_label_file(fn)}"
        } elif (fn.name == "digest_lonely") {
            lonely_key = "function-_rst_digest_mod-{function_label_file(fn)}"
        }
    }
    t |> success(!empty(two_arg_key) && !empty(lonely_key), "the fixture functions' topic keys")
    var err : string
    let root = create_temp_directory("das_rst_digest", err)
    t |> success(empty(err), "temp directory: {err}")
    write_twin(root, "module-_rst_digest_mod", "Digest fixture module. It has a second sentence.\n\nA second paragraph stays out.\n")
    write_twin(root, two_arg_key, "Adds ``two`` numbers, see :ref:`digest_lonely <lbl-lonely>`. A second sentence.\n")
    write_twin(root, lonely_key, "// stub\ndef digest_lonely (a: int) : int\n")
    write_twin(root, "structure-_rst_digest_mod-DigestPoint", "A point in two axes\nx coordinate\ny coordinate\n")
    write_twin(root, "enumeration-_rst_digest_mod-DigestKind", "Kinds of digest\nthe first kind\nthe second kind\n")
    write_twin(root, "typedef-_rst_digest_mod-DigestFlags", "Two flags\nthe one bit\nthe two bit\n")
    write_twin(root, "Variable-_rst_digest_mod-DIGEST_LIMIT", "{bom()}The digest limit.\n")
    let saved_topic = topic_root
    let saved_handmade = handmade_root
    topic_root = "{root}/generated"
    handmade_root = "{root}/handmade"
    digest_path = "{root}/digest.md"
    ast_gc_guard() {
        var groups <- array<DocGroup>(
            group_by_regex("Pairs", mod, %regex~digest_pair$%%)
        )
        documents("Probe", mods, "probe.rst", groups, DocsHook())
        write_digest()
    }
    digest_path = ""
    topic_root = saved_topic
    handmade_root = saved_handmade
    let digest = fread("{root}/digest.md")
    t |> run("the module has a section with its intro paragraph and an index line with its first sentence") @(t : T?) {
        t |> success(has_line(digest, "## _rst_digest_mod"), "section heading: {digest}")
        t |> success(has_line(digest, "Digest fixture module. It has a second sentence."), "intro paragraph: {digest}")
        t |> success(find(digest, "second paragraph") < 0, "the paragraph ends at the blank line: {digest}")
        t |> success(has_line(digest, "- [_rst_digest_mod](#_rst_digest_mod) - Digest fixture module."), "index line carries the first sentence only: {digest}")
    }
    t |> run("groups become headings, an ungrouped function lands under Uncategorized") @(t : T?) {
        let pairs = find(digest, "\n### Pairs\n")
        let uncategorized = find(digest, "\n### Uncategorized\n")
        let pair_row = find(digest, "- `digest_pair`")
        let lonely_row = find(digest, "- `digest_lonely`")
        t |> success(pairs >= 0 && uncategorized >= 0, "both headings present: {digest}")
        t |> success(pairs < pair_row && pair_row < uncategorized && uncategorized < lonely_row, "each row sits under its heading: {digest}")
    }
    t |> run("overloads collapse to one row and a documented later overload fills it") @(t : T?) {
        t |> success(count(digest, "- `digest_pair`") == 1, "one row per name: {digest}")
        t |> success(has_line(digest, "- `digest_pair` - Adds `two` numbers, see `digest_lonely`."), "first sentence with RST markup stripped: {digest}")
    }
    t |> run("a symbol whose only topic is a stub keeps a bare row") @(t : T?) {
        t |> success(has_line(digest, "- `digest_lonely`"), "the stub's signature line is not a description: {digest}")
    }
    t |> run("tabular topics drop their per-entry tail and keep the description untruncated") @(t : T?) {
        t |> success(has_line(digest, "- `DigestPoint` - A point in two axes"), "struct row: {digest}")
        t |> success(has_line(digest, "- `DigestKind` - Kinds of digest"), "enum row: {digest}")
        t |> success(has_line(digest, "- `DigestFlags` - Two flags"), "bitfield row: {digest}")
        t |> success(find(digest, "coordinate") < 0 && find(digest, "kind\n") < 0 && find(digest, " bit\n") < 0, "entry lines stay out: {digest}")
    }
    t |> run("constants, classes and their methods have rows") @(t : T?) {
        t |> success(has_line(digest, "- `DIGEST_LIMIT` - The digest limit."), "constant row, the source's byte-order mark dropped: {digest}")
        t |> success(has_line(digest, "- `DigestBox`"), "class row: {digest}")
        t |> success(has_line(digest, "- `DigestBox.show`"), "method row: {digest}")
    }
    rmdir_rec(root, err)
}
