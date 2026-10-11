options gen2

// LINT019 fixture: check_empty carries a LIVE suppression (PERF017 fires there and
// is consumed); pass_through carries a STALE one (nothing fires). spell_the_syntax
// carries PERF008 spellings that are NOT directives (string literal, URL,
// mid-comment prose) and must not be reported stale. Otherwise lint-clean.

def check_empty(s : string) : bool {
    return length(s) == 0   // nolint:PERF017
}

def pass_through(s : string) : string {
    return s   // nolint:PERF006
}

def spell_the_syntax() : string {
    // docs quoting the syntax: see http://daslang.io/nolint:PERF008 for details
    // the marker nolint:PERF008 in mid-comment prose is not a directive either
    return "// nolint:PERF008 quoted"
}

//! nolint:PERF010
def doc_comment_directive(s : string) : string {
    return s
}

[export]
def main {
    let _a = check_empty("x")
    let _b = pass_through("y")
    let _c = spell_the_syntax()
    let _d = doc_comment_directive("z")
}
