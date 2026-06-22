// mismatched curly braces
// verifies parser detects extra closing brace
options gen2
expect 10200, 30151    // syntax_error, mismatching_curly_bracers

def test() {
    if (true) {
    }}
}
