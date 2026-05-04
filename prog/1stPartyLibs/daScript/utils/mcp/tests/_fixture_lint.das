options gen2

// Test fixture with known lint issues for MCP lint tool tests.
// - unused variable
// - unreachable code

[export]
def main() {
    var unused_var = 42
    print("hello\n")
    return
    print("unreachable\n")
}
