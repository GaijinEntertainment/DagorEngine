// Regression test for invalid_empty_name (30296) on Structure declaration.
// The structure_macro patch() adds an empty-named Structure to the
// compiling module; the typer fires preVisit(Structure) and rejects.
options gen2
expect 30296

require _helper_empty_names

[bad_emit_structure]
struct private Target {
    x : int
}

[export]
def main {}
