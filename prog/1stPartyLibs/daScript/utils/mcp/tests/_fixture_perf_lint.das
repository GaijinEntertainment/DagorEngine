options gen2

// Test fixture with known performance warnings for MCP perf_lint tool tests.
// PERF001: string += in loop

def perf001_example(n : int) : string {
    var s = ""
    for (i in range(n)) {
        s += "x"
    }
    return s
}

[export]
def main() {
    print("{perf001_example(3)}\n")
}
