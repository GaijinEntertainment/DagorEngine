options gen2

require strings
require daslib/strings_boost
require daslib/fio
require daslib/regex
require daslib/regex_boost
require dastest/review_gate

// The mechanical half of utils/lint/REVIEW.md (contract: REVIEW_COMMON.md at the repo root).
// Run from the repo root: bin/daslang utils/lint/REVIEW.das - exit 0 clean, 1 with findings.
// Every rule id a lint module emits has a fixture and a lint.rst section.

let RULE_MODULES <- [ "daslib/lint.das", "daslib/perf_lint.das", "daslib/style_lint.das", "daslib/dupe_detect.das", "utils/lint/main.das" ]
let RST = "doc/source/reference/language/lint.rst"
let FIXTURE_DIRS <- [ "utils/lint/tests", "tests/lint" ]
// fixtures whose file name does not carry the id (README.md lists them)
var FIXTURE_ALIAS <- { "LINT019" => "tests/lint/test_stale_nolint.das" }

var private RULE_ID <- %regex~"((LINT|PERF|STYLE)[0-9][0-9][0-9]):%%

def private read_all(path : string) : string {
    var text = ""
    fopen(path, "rb") $(f) {
        if (f != null) {
            text = fread(f)
        }
    }
    return text
}

def private emitted_ids(path : string) : array<string> {
    var ids : array<string>
    var seen : table<string>
    let text = read_all(path)
    regex_foreach(RULE_ID, text) $(_at) {
        let id = regex_group(RULE_ID, 1, text)
        if (!key_exists(seen, id)) {
            seen |> insert(id)
            ids |> push(id)
        }
        return true
    }
    return <- ids
}

def private has_fixture(id : string) : bool {
    if (key_exists(FIXTURE_ALIAS, id)) {
        var st : FStat
        return stat(FIXTURE_ALIAS[id], st)
    }
    let lo = to_lower(id)
    var found = false
    for (d in FIXTURE_DIRS) {
        dir(d) $(name) {
            if (!found && name |> ends_with(".das") && find(to_lower(name), lo) >= 0) {
                found = true
            }
        }
    }
    return found
}

[export]
def main() : int {
    let rst = read_all(RST)
    for (m in RULE_MODULES) {
        for (id in emitted_ids(m)) {
            if (!has_fixture(id)) {
                gate_finding(m, "{id} has no fixture - add utils/lint/tests/{to_lower(id)}_<name>.das (bad and good shapes, exact expect count)")
            }
            // a section heading line carries the id at its start ("PERF026/027/028 — ..." covers a family)
            var documented = false
            for (line in split(rst, "\n")) {
                if (line |> starts_with("{id} ") || (line |> starts_with("PERF") && find(line, id |> slice(4)) >= 0 && find(line, "/") >= 0 && find(line, " — ") >= 0)) {
                    documented = true
                    break
                }
            }
            if (!documented) {
                gate_finding(m, "{id} has no section in {RST}")
            }
        }
    }
    return gate_verdict("utils/lint")
}
