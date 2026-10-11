// Fixture for test_arch_annotation_errors.das — a document with an empty anchor half.
// `_`-prefix keeps dastest from auto-running it; the `expect` directive makes lint
// skip it (a non-compiling program never reaches the lint passes).
expect 20800

options gen2

[arch(at="ARCHITECTURE.md#")]
def cited {
}

[export]
def main {
    cited()
}
