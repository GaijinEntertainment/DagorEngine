options gen2

// MCP lint fixture: the nolint below suppresses nothing - LINT019 must report it.

def pass_through(s : string) : string {
    return s   // nolint:PERF006
}

[export]
def main {
    let _a = pass_through("y")
}
