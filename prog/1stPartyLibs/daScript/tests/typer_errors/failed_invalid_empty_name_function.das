// Regression test for invalid_empty_name (30296) on Function declaration.
// The structure_macro patch() adds an empty-named Function to the
// compiling module; the typer fires preVisit(Function) and rejects.
options gen2
expect 30296

require _helper_empty_names

[bad_emit_function]
struct private Target {
    x : int
}

[export]
def main {}
