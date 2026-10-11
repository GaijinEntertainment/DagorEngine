// Fixture for test_arch_annotation_errors.das — [arch] citing the empty string.
// `_`-prefix keeps dastest from auto-running it; the `expect` directive makes lint
// skip it (a non-compiling program never reaches the lint passes).
expect 20800

options gen2

[arch(at="")]
def cited {
}

[export]
def main {
    cited()
}
