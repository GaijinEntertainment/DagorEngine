options gen2

require dastest/testing_boost public
require strings
require daslib/fio
require daslib/strings_boost
require dastest/review_gate

[test]
def test_findings_accumulate_and_reset(t : T?) {
    gate_reset()
    gate_finding("a.das", "message one")
    gate_finding("b.das", 7, "line-anchored")
    gate_finding("c.das", 0, "zero line means no anchor")
    var inscope fs <- gate_findings()
    t |> equal(3, length(fs))
    t |> equal("a.das: message one", fs[0])
    t |> equal("b.das:7: line-anchored", fs[1])
    t |> equal("c.das: zero line means no anchor", fs[2])
    t |> equal(1, gate_verdict("fixture"))
    gate_reset()
    var inscope none <- gate_findings()
    t |> equal(0, length(none))
    t |> equal(0, gate_verdict("fixture"))
}

[test]
def test_kebab_case(t : T?) {
    t |> success(is_kebab_case("like-this"))
    t |> success(is_kebab_case("aot"))
    t |> success(is_kebab_case("gen1-to-gen2"))
    t |> success(!is_kebab_case("like_that"))
    t |> success(!is_kebab_case("likeThis"))
    t |> success(!is_kebab_case("-leading"))
    t |> success(!is_kebab_case("trailing-"))
    t |> success(!is_kebab_case("double--dash"))
    t |> success(!is_kebab_case(""))
}

[test]
def test_find_line(t : T?) {
    let text = "one\ntwo\nthree\n"
    t |> equal(1, find_line(text, "one"))
    t |> equal(2, find_line(text, "two"))
    t |> equal(3, find_line(text, "three"))
    t |> equal(2, find_line(text, "two\nthree"))
    t |> equal(0, find_line(text, "absent"))
    t |> equal(0, find_line("", "x"))
}

[test]
def test_das_requires(t : T?) {
    let fixture_r = create_temp_file_result("review_gate_req", ".das")
    t |> success(fixture_r is value, "temp fixture file created")
    if (!(fixture_r is value)) {
        return
    }
    let fixture = unsafe(fixture_r.value)
    let src = "options gen2\n\nrequire strings\nrequire daslib/fio public\nrequire utils_common // trailing comment\n// require commented_out\n    require indented/mod\nrequire tight//comment with no space\nrequire //nothing before the comment\nrequire ?das_metal dasllama/dasllama_metal_common   // the module-exists guard is not the path\nrequire ?lonely\n"
    t |> success(fwrite(fixture, src))
    var inscope reqs <- das_requires(fixture)
    t |> equal(6, length(reqs))
    t |> equal("strings", reqs[0])
    t |> equal("daslib/fio", reqs[1])
    t |> equal("utils_common", reqs[2])
    t |> equal("indented/mod", reqs[3])
    t |> equal("tight", reqs[4], "comment cut runs before the space split")
    t |> equal("dasllama/dasllama_metal_common", reqs[5], "a ?guard token is skipped, the path after it is the require; a guard with no path is nothing")
    t |> success(remove(fixture))
}

[test]
def test_cmake_command_blocks(t : T?) {
    let text = "# install( inside a comment stays closed\nset(FOO bar)\ninstall(FILES a.das\n    DESTINATION utils/aot) # trailing\nmyinstall(x)\ninstall(PROGRAMS \"weird)name.exe\" DESTINATION bin)\n"
    var inscope blocks <- cmake_command_blocks(text, "install")
    t |> equal(2, length(blocks))
    t |> success(find(blocks[0], "DESTINATION utils/aot") >= 0, "first block spans lines")
    t |> success(find(blocks[1], "weird)name.exe") >= 0, "paren inside quotes does not close the block")
    var inscope sets <- cmake_command_blocks(text, "set")
    t |> equal(1, length(sets))
    t |> equal("set(FOO bar)", sets[0])
}

[test]
def test_cmake_command_blocks_edges(t : T?) {
    // escaped quote inside a string, a paren inside an in-block comment,
    // whitespace before the paren, and an unterminated block at EOF
    let text = "install(FILES \"a\\\")b.das\" DESTINATION x)\ninstall(FILES a.das # comment with ) paren\n    DESTINATION y)\ninstall (PROGRAMS z)\ninstall(FILES never_closed"
    var inscope blocks <- cmake_command_blocks(text, "install")
    t |> equal(3, length(blocks), "the unterminated block yields nothing")
    t |> success(find(blocks[0], "DESTINATION x") >= 0, "escaped quote does not end the string")
    t |> success(find(blocks[1], "DESTINATION y") >= 0, "paren in an in-block comment does not close it")
    t |> success(find(blocks[2], "PROGRAMS z") >= 0, "whitespace before the paren is accepted")
}

[test]
def test_cmake_words(t : T?) {
    let text = "add_test(NAME a # comment ) here\n    COMMAND \"c:/p a t h/x.exe\")\n"
    var inscope words <- cmake_words(text)
    t |> equal(7, length(words))
    t |> equal("add_test", words[0])
    t |> equal("(", words[1], "parens are words of their own")
    t |> equal("NAME", words[2])
    t |> equal("a", words[3])
    t |> equal("COMMAND", words[4], "the comment runs to end of line only")
    t |> equal("c:/p a t h/x.exe", words[5], "a quoted run is one word, quotes dropped")
    t |> equal(")", words[6])

    var inscope args <- cmake_args(text)
    t |> equal(4, length(args), "cmake_args drops the command name and the parens")
    t |> equal("NAME", args[0])
    t |> equal("c:/p a t h/x.exe", args[3])

    var inscope trailing <- cmake_words("set(A \"unterminated\\")
    t |> equal(4, length(trailing), "an escape at EOF terminates the scan")
    t |> equal("unterminated\\", trailing[3], "the unterminated run ends at EOF, not past it")

    var inscope escaped <- cmake_words("\"esc\\\"aped\" x")
    t |> equal(2, length(escaped))
    t |> equal("esc\\\"aped", escaped[0], "an escaped quote does not end the run")

    var inscope eof_comment <- cmake_words("# trailing comment, no newline")
    t |> equal(0, length(eof_comment))

    var inscope tight <- cmake_words("a\"b\" c")
    t |> equal(3, length(tight), "a quote breaks a word even with no space before it")
    t |> equal("a", tight[0])
    var inscope hashed <- cmake_words("a#b\nd")
    t |> equal(2, length(hashed), "a # breaks a word and opens a comment")
    t |> equal("a", hashed[0])
    t |> equal("d", hashed[1])
}

[test]
def test_cmake_test_labels(t : T?) {
    let text = "add_test(NAME one COMMAND x)\nset_tests_properties(one PROPERTIES\n    LABELS \"big;style_lint\"\n    PASS_REGULAR_EXPRESSION \"STYLE014\")\ndoctest_discover_tests(exe PROPERTIES LABELS \"small\")\nset_tests_properties(two PROPERTIES LABELS big)\n"
    var inscope labels <- cmake_test_labels(text)
    t |> equal(3, length(labels))
    t |> equal("big", labels[0])
    t |> equal("style_lint", labels[1], "a semicolon list yields every label")
    t |> equal("small", labels[2], "any command's LABELS property counts")

    var inscope none <- cmake_test_labels("add_test(NAME x COMMAND y)\n# LABELS \"commented\"\n")
    t |> equal(0, length(none))

    var inscope dangling <- cmake_test_labels("set_tests_properties(x PROPERTIES LABELS")
    t |> equal(0, length(dangling), "a LABELS with no value reads past nothing")
    var inscope trailing_semi <- cmake_test_labels("set_tests_properties(x PROPERTIES LABELS \"big;\")")
    t |> equal(1, length(trailing_semi), "a trailing semicolon yields no empty label")
    t |> equal("big", trailing_semi[0])

    // an unquoted list variable expands to one argument per label - the shape the vendored
    // doctestAddTests.cmake writes as `LABELS ${labels}`
    var inscope expanded <- cmake_test_labels(
        "set_tests_properties(x PROPERTIES LABELS big style_lint PASS_REGULAR_EXPRESSION \"STYLE014\")\nadd_dependencies(test-big y)\n")
    t |> equal(2, length(expanded), "every label of an expanded list is read")
    t |> equal("big", expanded[0])
    t |> equal("style_lint", expanded[1])

    var inscope bounded <- cmake_test_labels("set_tests_properties(x PROPERTIES LABELS big)\nadd_dependencies(test-big y)\n")
    t |> equal(1, length(bounded), "the run stops at the end of its own command")
    t |> equal("big", bounded[0])
}

[test]
def test_cmake_command_targets(t : T?) {
    let text = "add_dependencies(test-big test_a test_b)\n# add_dependencies(test-small ghost)\nadd_dependencies(\n    test-small test_c)\nadd_executable(other x.cpp)\n"
    var inscope deps <- cmake_command_targets(text, "add_dependencies")
    t |> equal(2, length(deps))
    t |> equal("test-big", deps[0])
    t |> equal("test-small", deps[1], "the target may sit on the next line")
    var inscope exes <- cmake_command_targets(text, "add_executable")
    t |> equal(1, length(exes))
    t |> equal("other", exes[0])

    var inscope argless <- cmake_command_targets("add_dependencies()\nadd_dependencies(x y)", "add_dependencies")
    t |> equal(1, length(argless), "an empty block names no target and does not index past it")
    t |> equal("x", argless[0])
    var inscope absent <- cmake_command_targets(text, "add_library")
    t |> equal(0, length(absent))
}

[test]
def test_cmake_commands_are_case_insensitive(t : T?) {
    // CMake itself accepts any casing; a gate that only reads one spelling reports
    // a wiring defect that is not there, or misses one that is
    let text = "ADD_DEPENDENCIES(test-big test_a)\nAdd_Test(NAME m COMMAND test_a)\nset_tests_properties(m PROPERTIES LABELS \"big\")\n"
    var inscope deps <- cmake_command_targets(text, "add_dependencies")
    t |> equal(1, length(deps))
    t |> equal("test-big", deps[0], "an all-caps command opens a block")
    var inscope cmds <- cmake_test_commands(text)
    t |> equal(1, length(cmds))
    t |> equal("test_a", cmds[0], "mixed case opens a block too")
    var inscope labels <- cmake_test_labels(text)
    t |> equal(1, length(labels))
    t |> equal("big", labels[0])
}

[test]
def test_cmake_test_commands(t : T?) {
    let text = "add_test(NAME a COMMAND test_a WORKING_DIRECTORY x)\nadd_test(\n    NAME b\n    COMMAND $<TARGET_FILE:daslang> script.das)\nadd_test(NAME c)\n"
    var inscope cmds <- cmake_test_commands(text)
    t |> equal(2, length(cmds), "a block with no COMMAND contributes nothing")
    t |> equal("test_a", cmds[0])
    t |> equal("$<TARGET_FILE:daslang>", cmds[1], "a generator expression comes back verbatim")
}

[test]
def test_cmake_list_entries(t : T?) {
    let text = "set(DAS_UTILS\n    aot\n    dascov  # a comment\n    internal/jit\n)\nif(NOT X)\n    list(APPEND DAS_UTILS benchctl)   # same gate\nendif()\nset(OTHER_LIST nope)\n"
    var inscope entries <- cmake_list_entries(text, "DAS_UTILS")
    t |> equal(4, length(entries))
    t |> equal("aot", entries[0])
    t |> equal("dascov", entries[1])
    t |> equal("internal/jit", entries[2])
    t |> equal("benchctl", entries[3])
    var inscope other <- cmake_list_entries(text, "OTHER_LIST")
    t |> equal(1, length(other))
    t |> equal("nope", other[0])
    var inscope missing <- cmake_list_entries(text, "NOT_THERE")
    t |> equal(0, length(missing))

    var inscope bare <- cmake_list_entries("set()\nset(EMPTY_LIST)\n", "EMPTY_LIST")
    t |> equal(0, length(bare), "a set with no entries neither crashes nor yields")

    var inscope shouty <- cmake_list_entries("SET(CAPS a)\nLIST(APPEND CAPS b)\n", "CAPS")
    t |> equal(2, length(shouty), "an uppercase command is read once, not twice")
}

[test]
def test_url_host(t : T?) {
    t |> equal(url_host("https://gc.zgo.at/count.js"), "gc.zgo.at")
    t |> equal(url_host("http://example.com"), "example.com")
    t |> equal(url_host("//gc.zgo.at/count.js"), "gc.zgo.at")
    t |> equal(url_host("https://example.com:8443/x"), "example.com", "port is dropped")
    t |> equal(url_host("https://user@example.com/x"), "example.com", "userinfo is dropped")
    t |> equal(url_host("https://example.com?q=1"), "example.com")
    t |> equal(url_host("https://example.com#frag"), "example.com")
    t |> equal(url_host("files/github-star.js"), "", "a relative URL has no host")
    t |> equal(url_host("/doc/index.html"), "", "a rooted path is not protocol-relative")
    t |> equal(url_host("// a trailing comment"), "", "a comment is not host-shaped")
    t |> equal(url_host(""), "")
}

[test]
def test_script_urls(t : T?) {
    let src = "var A = 'https://api.github.com/repos/x';\nvar B = \"//gc.zgo.at/count.js\";\n// var C = 'https://commented.example/x';\nfetch(\"http://plain.example/y\");\nvar D = 'files/local.js';\n"
    var inscope urls <- script_urls(src)
    t |> equal(3, length(urls), "the commented address and the relative path do not count")
    t |> equal("https://api.github.com/repos/x", urls[0])
    t |> equal("//gc.zgo.at/count.js", urls[1])
    t |> equal("http://plain.example/y", urls[2])
}

[test]
def test_html_resource_urls(t : T?) {
    let page = "<link rel=\"stylesheet\" href=\"https://cdn.example/a.css\" />\n<a href=\"https://clicked.example/page\">link</a>\n<meta property=\"og:image\" content=\"https://meta.example/i.png\" />\n<script data-goatcounter=\"https://beacon.example/count\"\n        async src=\"//gc.zgo.at/count.js\"></script>\n<img src=\"files/local.png\" />\n<script>fetch(\"https://inline.example/x\");</script>\n"
    var inscope urls <- html_resource_urls(page)
    var inscope hosts : array<string>
    hosts |> reserve(length(urls))
    for (u in urls) {
        hosts |> push(url_host(u))
    }
    t |> equal(4, length(hosts), "an <a> link and a <meta> address are not loads")
    t |> equal("cdn.example", hosts[0])
    t |> equal("beacon.example", hosts[1], "a beacon endpoint travels in a data-* attribute")
    t |> equal("gc.zgo.at", hosts[2])
    t |> equal("inline.example", hosts[3], "an inline script body is scanned too")
}

[test]
def test_gate_web_third_party(t : T?) {
    let dir_r = create_temp_directory_result("review_gate_web")
    t |> success(dir_r is value, "temp fixture directory created")
    if (!(dir_r is value)) {
        return
    }
    let root = unsafe(dir_r.value)
    t |> success(mkdir("{root}/skipped"))
    t |> success(fwrite("{root}/ok.html", "<script src=\"https://allowed.example/a.js\"></script>\n"))
    t |> success(fwrite("{root}/bad.html", "<x>\n<script src=\"https://tracker.example/t.js\"></script>\n"))
    t |> success(fwrite("{root}/bad.js", "\ndocument.cookie = 'a=1';\n"))
    t |> success(fwrite("{root}/skipped/ignored.js", "var u = 'https://tracker.example/t.js';\n"))
    t |> success(fwrite("{root}/notes.txt", "https://tracker.example/t.js\n"))
    gate_reset()
    gate_web_third_party([root], {"skipped"}, {"allowed.example"})
    var inscope fs <- gate_findings()
    gate_reset()
    t |> equal(2, length(fs), "only the two scanned files with a defect report")
    let joined = join(fs, "\n")
    t |> success(find(joined, "bad.html:2: reaches tracker.example") >= 0, joined)
    t |> success(find(joined, "bad.js:2: uses document.cookie") >= 0, joined)
    t |> success(rmdir_rec("{root}"))
}

[test]
def test_css_urls(t : T?) {
    let sheet = "@import url(\"https://fonts.googleapis.com/css2?family=Inter\");\n@font-face \{ src: url(//cdn.example/a.woff2) format('woff2'); \}\nbody \{ background: url('files/local.png'); \}\n/* see http://ie.microsoft.com/testdrive/ for the prefix history */\n"
    var inscope urls <- css_urls(sheet)
    t |> equal(2, length(urls), "a local path and an address inside a /* */ comment do not count")
    t |> equal("https://fonts.googleapis.com/css2?family=Inter", urls[0])
    t |> equal("//cdn.example/a.woff2", urls[1], "a bare protocol-relative url() counts")
}

[test]
def test_gate_web_third_party_file_kinds(t : T?) {
    let dir_r = create_temp_directory_result("review_gate_kinds")
    t |> success(dir_r is value, "temp fixture directory created")
    if (!(dir_r is value)) {
        return
    }
    let root = unsafe(dir_r.value)
    t |> success(fwrite("{root}/page.rst", ".. raw:: html\n\n   <script src=\"https://tracker.example/t.js\"></script>\n"))
    t |> success(fwrite("{root}/post.md", "text\n<img src=\"https://tracker.example/pixel.png\" />\n"))
    t |> success(fwrite("{root}/theme.css", "@import url(\"https://tracker.example/f.css\");\n"))
    t |> success(fwrite("{root}/REVIEW.md", "**A page that writes document.cookie is a defect.**\n"))
    t |> success(fwrite("{root}/README.md", "<script src=\"https://tracker.example/t.js\"></script>\n"))
    gate_reset()
    let no_skipped_dirs : table<string>
    gate_web_third_party([root], no_skipped_dirs, {"allowed.example"})
    var inscope fs <- gate_findings()
    gate_reset()
    t |> equal(3, length(fs), "rst, md and css are scanned; a rule document is not a page")
    let joined = join(fs, "\n")
    t |> success(find(joined, "page.rst:3: reaches tracker.example") >= 0, joined)
    t |> success(find(joined, "post.md:2: reaches tracker.example") >= 0, joined)
    t |> success(find(joined, "theme.css:1: reaches tracker.example") >= 0, joined)
    t |> success(rmdir_rec("{root}"))
}

[test]
def test_html_resource_urls_edges(t : T?) {
    let page = "<script src=https://bare.example/a.js></script>\n<source srcset=\"https://first.example/a 1x, https://second.example/b 2x\" />\n<img src='https://quoted.example/c.png' />\n"
    var inscope urls <- html_resource_urls(page)
    var inscope hosts : array<string>
    hosts |> reserve(length(urls))
    for (u in urls) {
        hosts |> push(url_host(u))
    }
    t |> equal(4, length(hosts), "an unquoted value and every entry of a srcset count")
    t |> equal("bare.example", hosts[0], "HTML permits an unquoted attribute value")
    t |> equal("first.example", hosts[1])
    t |> equal("second.example", hosts[2], "a srcset carries more than one address")
    t |> equal("quoted.example", hosts[3])
}

[test]
def test_script_urls_template_literal(t : T?) {
    let src = "fetch(`//tpl.example/a`);\nfetch(`https://tpl2.example/b`);\n"
    var inscope urls <- script_urls(src)
    t |> equal(2, length(urls), "a template literal opens a protocol-relative address too")
    t |> equal("//tpl.example/a", urls[0])
    t |> equal("https://tpl2.example/b", urls[1])
}

[test]
def test_url_host_is_case_insensitive(t : T?) {
    t |> equal(url_host("HTTPS://Tracker.Example/x.js"), "tracker.example", "a scheme is case-insensitive")
    t |> equal(url_host("HtTp://EXAMPLE.COM"), "example.com", "a host is case-insensitive")
}

[test]
def test_html_resource_urls_uppercase_script(t : T?) {
    let page = "<SCRIPT>fetch(\"https://inline.example/x\");</SCRIPT>\n"
    var inscope urls <- html_resource_urls(page)
    t |> equal(1, length(urls), "an uppercase closing tag still bounds the inline body")
    t |> equal("inline.example", url_host(urls[0]))
}

[test]
def test_url_host_ipv6_literal(t : T?) {
    t |> equal(url_host("http://[::1]/t.js"), "[::1]", "a bracketed IPv6 literal is a host")
    t |> equal(url_host("https://[2001:DB8::1]:8443/x"), "[2001:db8::1]", "the port after the bracket is dropped")
    t |> equal(url_host("http://[::1"), "", "an unclosed bracket is not host-shaped")
}

[test]
def test_script_urls_multiline_template_literal(t : T?) {
    let src = "const s = `//tpl.example/a\nhttps://tpl2.example/b\n`;\nconst u = \"x\"; // https://commented.example/c\n"
    var inscope urls <- script_urls(src)
    t |> equal(2, length(urls), "the second line of a template literal is still inside it; a plain comment still does not count")
    t |> equal("//tpl.example/a", urls[0])
    t |> equal("https://tpl2.example/b", urls[1])
}

[test]
def test_html_resource_urls_script_end_tag_exact(t : T?) {
    let page = "<script>var a = \"</scriptx>\"; fetch(\"https://after.example/x\");</script >\n"
    var inscope urls <- html_resource_urls(page)
    t |> equal(1, length(urls), "only a real end tag - `</script` then whitespace, `/` or `>` - closes the body")
    t |> equal("after.example", url_host(urls[0]))
}

[test]
def test_gate_web_third_party_unreadable_file(t : T?) {
    let dir_r = create_temp_directory_result("review_gate_unreadable")
    t |> success(dir_r is value, "temp fixture directory created")
    if (!(dir_r is value)) {
        return
    }
    let root = unsafe(dir_r.value)
    t |> success(fwrite("{root}/empty.js", ""))
    t |> success(fwrite("{root}/locked.js", "var u = 'https://tracker.example/t.js';\n"))
    unsafe(system("chmod 000 {root}/locked.js"))
    if (!empty(fread("{root}/locked.js"))) {
        unsafe(system("chmod 644 {root}/locked.js"))
        t |> success(rmdir_rec("{root}"))
        t |> skip("this platform or user reads a file whose permission bits say no")
        return
    }
    gate_reset()
    let no_skipped_dirs : table<string>
    gate_web_third_party([root], no_skipped_dirs, {"allowed.example"})
    var inscope fs <- gate_findings()
    gate_reset()
    unsafe(system("chmod 644 {root}/locked.js"))
    t |> success(rmdir_rec("{root}"))
    t |> equal(1, length(fs), "an empty file is scanned and clean; an unreadable one is a finding, never a silent pass")
    t |> success(find(join(fs, "\n"), "locked.js: unreadable") >= 0, join(fs, "\n"))
}

[test]
def test_html_resource_urls_unclosed_script(t : T?) {
    let page = "<p>x</p>\n<script>fetch(\"https://open.example/x\");\n"
    var inscope urls <- html_resource_urls(page)
    t |> equal(1, length(urls), "a script with no end tag runs to end of file in a browser, so it is scanned to end of file")
    t |> equal("open.example", url_host(urls[0]))
}

[test]
def test_shell_cp_sources(t : T?) {
    let sh = "set -eux\ncp -r web/examples/ui/src/* _site/playground/\ncp \"web/output64/examples/$g/$g.html\" \"_site/examples/$g/\"\ncp -r site/playground/samples _site/playground/samples 2>/dev/null || true\ncp web/a.js web/b.js _site/x/\ncp web/elsewhere.js build/out/\n"
    var inscope srcs <- shell_cp_sources(sh, "_site/")
    t |> equal(5, length(srcs), "flags dropped, quotes removed, a shell operator ends the argument list, a non-site destination is skipped")
    t |> equal("web/examples/ui/src/*", srcs[0])
    t |> equal("web/output64/examples/$g/$g.html", srcs[1])
    t |> equal("site/playground/samples", srcs[2])
    t |> equal("web/a.js", srcs[3])
    t |> equal("web/b.js", srcs[4], "every source of a multi-source cp")
}

[test]
def test_cmake_copy_sources(t : T?) {
    let cm = "add_custom_target(stage ALL\n    COMMAND $\{CMAKE_COMMAND} -E copy_directory\n        $\{CMAKE_CURRENT_SOURCE_DIR\}/examples/ui/src\n        $\{SITE_PLAYGROUND_DIR\}\n    COMMAND $\{CMAKE_COMMAND} -E copy\n        $\{DAS_WEB_OUTPUT_DIR\}/a.js\n        $\{DAS_WEB_OUTPUT_DIR\}/a.wasm\n        $\{SITE_FILES_WASM_DIR\}/\n    COMMAND $\{CMAKE_COMMAND} -E copy_directory\n        $\{CMAKE_CURRENT_SOURCE_DIR\}/examples/ui\n        $\{DAS_WEB_OUTPUT_DIR\}\n    COMMENT \"stage\"\n)\n"
    var inscope srcs <- cmake_copy_sources(cm, "SITE_")
    t |> equal(3, length(srcs), "a copy whose destination lacks the marker is skipped")
    t |> equal("$\{CMAKE_CURRENT_SOURCE_DIR\}/examples/ui/src", srcs[0])
    t |> equal("$\{DAS_WEB_OUTPUT_DIR\}/a.js", srcs[1])
    t |> equal("$\{DAS_WEB_OUTPUT_DIR\}/a.wasm", srcs[2], "every source of a multi-source copy")
}

[test]
def test_gate_web_third_party_assets_json(t : T?) {
    let dir_r = create_temp_directory_result("review_gate_assets")
    t |> success(dir_r is value, "temp fixture directory created")
    if (!(dir_r is value)) {
        return
    }
    let root = unsafe(dir_r.value)
    t |> success(fwrite("{root}/demo.das.assets.json", "[\"tutorials/_assets/cat/cat.obj\",\n \"https://cdn.example/tex.jpg\"]\n"))
    t |> success(fwrite("{root}/records.json", "[\{\"link\": \"https://tracker.example/not-a-load\"\}]\n"))
    gate_reset()
    let no_skipped_dirs : table<string>
    gate_web_third_party([root], no_skipped_dirs, {"allowed.example"})
    var inscope fs <- gate_findings()
    gate_reset()
    t |> success(rmdir_rec("{root}"))
    t |> equal(1, length(fs), "an .assets.json sidecar is scanned - every entry is a fetch; any other .json is not")
    t |> success(find(join(fs, "\n"), "demo.das.assets.json:2: reaches cdn.example") >= 0, join(fs, "\n"))
}

[test]
def test_gate_home_paths(t : T?) {
    let fixture_r = create_temp_file_result("review_gate_home", ".json")
    t |> success(fixture_r is value, "temp fixture file created")
    if (!(fixture_r is value)) {
        return
    }
    let fixture = unsafe(fixture_r.value)
    t |> success(fwrite(fixture, "[\n \"cmd\": \"-m ~/models/x.gguf\",\n \"env\": \"D=/Users/ci/models\",\n \"bin\": \"C:\\\\Users\\\\ci\\\\x.exe\"\n]\n"))
    gate_reset()
    gate_home_paths(fixture)
    var inscope fs <- gate_findings()
    gate_reset()
    t |> success(remove(fixture))
    t |> equal(2, length(fs), "a line already spelled ~ is clean; a unix home and a drive home each report")
    t |> success(find(fs[0], ":3: names a home directory") >= 0, fs[0])
    t |> success(find(fs[1], ":4: names a home directory") >= 0, fs[1])
}

[test]
def test_prefixed_tokens(t : T?) {
    let text = "<tr class=\"dl-row dl-row--noted\">.dl-table--bench tbody tr.dl-row--flat \{ \} var x = 'dl-win'; model-dl-x; the dl-* names"
    var inscope toks <- prefixed_tokens(text, "dl-")
    t |> equal(5, length(toks), "a prefix glued to a longer token (model-dl-x) and the bare prefix in prose (dl-*) are not tokens")
    t |> success(key_exists(toks, "dl-row"))
    t |> success(key_exists(toks, "dl-row--noted"), "a double-dash modifier is one token")
    t |> success(key_exists(toks, "dl-table--bench"))
    t |> success(key_exists(toks, "dl-row--flat"))
    t |> success(key_exists(toks, "dl-win"))
}

[test]
def test_markdown_table_rows(t : T?) {
    let md = "# title\n\n## INTERP\n\nprose\n\n| Benchmark | A (m1) | B (m2) |\n|---|---:|---:|\n| `fam_one` | 1.5 | - |\n| `fam_two` | 2.0 | 3.5 |\n\ntrailing prose\n\n## JIT\n\n| Benchmark | A (m1) |\n|---|---:|\n| `fam_one` | 0.5 |\n"
    var inscope rows <- markdown_table_rows(md, "INTERP")
    t |> equal(3, length(rows), "header plus two body rows; the alignment row is dropped")
    let header = join(rows[0], "|")
    let first = join(rows[1], "|")
    let second = join(rows[2], "|")
    t |> equal("Benchmark|A (m1)|B (m2)", header)
    t |> equal("fam_one|1.5|-", first, "backticks removed")
    t |> equal("fam_two|2.0|3.5", second)
    var inscope jit <- markdown_table_rows(md, "JIT")
    t |> equal(2, length(jit), "a second section has its own table")
    t |> equal("fam_one|0.5", join(jit[1], "|"))
    var inscope none <- markdown_table_rows(md, "ABSENT")
    t |> equal(0, length(none))
}

[test]
def test_html_ids(t : T?) {
    var inscope ids <- html_ids("<span id=\"dl-hero-x\">6</span> <div data-id=\"not-one\" uid=\"nor-this\"><b id=\"plain\">")
    t |> equal(2, length(ids), "only a bare id attribute counts")
    t |> success(key_exists(ids, "dl-hero-x"))
    t |> success(key_exists(ids, "plain"))
}

[test]
def test_html_code_blocks(t : T?) {
    let page = "<pre><code class=\"language-daslang\">if (a &lt; b &amp;&amp; c) \{ \}</code></pre>\n<a href=\"/playground/\">try</a>\n<code class=\"language-js\">x</code>\n<code class=\"language-daslang\" data-playground=\"none\">second</code>"
    var inscope blocks <- html_code_blocks(page, "language-daslang")
    t |> equal(2, length(blocks), "only blocks of the named class")
    t |> equal("if (a < b && c) \{ \}", blocks[0].source, "entities decoded")
    t |> success(find(slice(page, blocks[0].after), "playground") >= 0, "the offset lands after the block's close tag")
    t |> equal("second", blocks[1].source, "a block with an extra attribute is still a block")
    t |> success(find(blocks[1].tag, "data-playground=\"none\"") >= 0, "the opening tag is carried whole")
}

[test]
def test_markdown_fences(t : T?) {
    let md = "prose\n```das\nlet x = 1\n```\nmore\n```dasx\nnot this\n```\n```das\nlet y = 2\n```\n"
    var inscope fences <- markdown_fences(md, "das")
    t |> equal(2, length(fences), "a longer language tag is another language")
    t |> equal("let x = 1\n", fences[0].source)
    t |> equal("let y = 2\n", fences[1].source)
    t |> success(find(slice(md, fences[0].after), "more") >= 0, "the offset lands after the closing fence")
}

[test]
def test_css_selector_tokens(t : T?) {
    let sheet = "/* dl-ghost is only mentioned here */\n.dl-row, .dl-table--bench tbody tr.dl-row--flat \{ content: \"dl-value\"; \}\n@media (max-width: 600px) \{ .dl-narrow \{ color: red; \} @supports (gap: 1px) \{ .dl-gapped \{ gap: 1px; \} \} \}\n@font-face \{ font-family: dl-face; src: url(x.woff); \}\n@keyframes dl-spin \{ from \{ opacity: 0; \} to \{ opacity: 1; \} \}\n.dl-after \{ color: blue; \}\n"
    var inscope defined <- css_selector_tokens(sheet, "dl-")
    t |> equal(6, length(defined), "a comment mention, a property value and a descriptor inside @font-face define nothing; a selector under @media or @supports does")
    t |> success(key_exists(defined, "dl-row"))
    t |> success(key_exists(defined, "dl-table--bench"))
    t |> success(key_exists(defined, "dl-row--flat"))
    t |> success(key_exists(defined, "dl-narrow"), "a rule nested in @media styles the narrow viewport")
    t |> success(key_exists(defined, "dl-gapped"), "nesting goes as deep as the group at-rules do")
    t |> success(key_exists(defined, "dl-after"), "the census resumes after a group at-rule closes")
    t |> success(!key_exists(defined, "dl-face"), "@font-face holds descriptors, not rules")
}

[test]
def test_shell_cp_sources_comments_and_quotes(t : T?) {
    let sh = "cp web/a.js _site/   # a trailing comment names no destination\ncp \"web/my dir/b.js\" _site/x/\n# cp web/c.js _site/\n"
    var inscope srcs <- shell_cp_sources(sh, "_site/")
    t |> equal(2, length(srcs), "a # comment ends the argument list and a commented-out cp is no cp")
    t |> equal("web/a.js", srcs[0])
    t |> equal("web/my dir/b.js", srcs[1], "a quoted source with a space is one token")
}

[test]
def test_gate_home_paths_unreadable_file(t : T?) {
    let fixture_r = create_temp_file_result("review_gate_home_locked", ".json")
    t |> success(fixture_r is value, "temp fixture file created")
    if (!(fixture_r is value)) {
        return
    }
    let fixture = unsafe(fixture_r.value)
    t |> success(fwrite(fixture, "\{\"cmd\": \"-m /Users/ci/x\"\}\n"))
    unsafe(system("chmod 000 {fixture}"))
    if (!empty(fread(fixture))) {
        unsafe(system("chmod 644 {fixture}"))
        t |> success(remove(fixture))
        t |> skip("this platform or user reads a file whose permission bits say no")
        return
    }
    gate_reset()
    gate_home_paths(fixture)
    var inscope fs <- gate_findings()
    gate_reset()
    unsafe(system("chmod 644 {fixture}"))
    t |> success(remove(fixture))
    t |> equal(1, length(fs), "a file the gate could not read is a finding, never a pass")
    t |> success(find(fs[0], "unreadable") >= 0, fs[0])
}

[test]
def test_gate_home_paths_missing_file(t : T?) {
    gate_reset()
    gate_home_paths("/nonexistent/review_gate_probe/records.json")
    var inscope fs <- gate_findings()
    gate_reset()
    t |> equal(1, length(fs), "a path that does not exist is a finding, never a pass")
    t |> success(find(fs[0], "missing or unreadable") >= 0, fs[0])
}
