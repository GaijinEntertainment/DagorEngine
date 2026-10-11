// Fixture for test_arch_annotation_errors.das — a named argument that is not at.
// `_`-prefix keeps dastest from auto-running it; the `expect` directive makes lint
// skip it (a non-compiling program never reaches the lint passes).
expect 20800

options gen2

[arch(doc="ARCHITECTURE.md#x")]
def cited {
}

[export]
def main {
    cited()
}
