// mismatched parentheses
// verifies parser detects extra closing parenthesis
options gen2
expect 20000, 10001    // syntax_error, mismatching_parentheses

def main() {
    if (true))
        debug(a)
}
