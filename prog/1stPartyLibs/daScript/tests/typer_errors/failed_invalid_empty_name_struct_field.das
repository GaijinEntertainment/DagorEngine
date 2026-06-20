// Regression test for invalid_empty_name (30296) on Structure::FieldDeclaration.
// The structure_macro patch() emits a Structure with one empty-named field;
// the typer fires preVisitStructureField and rejects.
options gen2
expect 30296

require _helper_empty_names

[bad_emit_struct_field]
struct private Target {
    x : int
}

[export]
def main {}
