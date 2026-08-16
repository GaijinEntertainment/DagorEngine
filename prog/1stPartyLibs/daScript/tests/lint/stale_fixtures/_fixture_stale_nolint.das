options gen2

// LINT019 fixture: line 6 carries a LIVE suppression (PERF017 fires there and is
// consumed); line 10 carries a STALE one (nothing fires). Otherwise lint-clean.

def check_empty(s : string) : bool {
    return length(s) == 0   // nolint:PERF017
}

def pass_through(s : string) : string {
    return s   // nolint:PERF006
}

[export]
def main {
    let _a = check_empty("x")
    let _b = pass_through("y")
}
