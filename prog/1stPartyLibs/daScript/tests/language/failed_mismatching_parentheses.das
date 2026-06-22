// mismatched parentheses
// verifies parser detects extra closing parenthesis
options gen2
expect 10202, 30151    // syntax_error, mismatching_parentheses

def main() {
    if (true))
        debug(a)
}
