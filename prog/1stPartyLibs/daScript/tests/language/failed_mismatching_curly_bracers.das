// mismatched curly braces
// verifies parser detects extra closing brace
options gen2
expect 20000, 10002    // syntax_error, mismatching_curly_bracers

def test() {
    if (true) {
    }}
}
