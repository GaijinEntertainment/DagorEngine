// non-local type as local variable
// verifies compiler rejects local variables, arrays, and tables of non-local types
// uses ast::TypeDecl as a representative non-local handled type
options gen2
expect 30108:4, 30101:2, 31300:3
options force_inscope_pod = false

require daslib/ast

struct StructureNotLocal {
    a : TypeDecl
}

[export]
def test : bool {
    var a : TypeDecl                    // 30108
    var b : StructureNotLocal           // 30108
    var c : array<StructureNotLocal>    // 30108
                                        // 30101: array type has to be 'local'
    var d : table<int; StructureNotLocal>// 30108
                                        // 30101: table value has to be 'local'
    return true
}
