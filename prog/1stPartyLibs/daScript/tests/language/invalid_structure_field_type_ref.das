// ref type in struct field
// verifies compiler rejects reference types as struct fields
options gen2
expect 30104    // invalid_structure_field_type

struct Foo {
    a : int&
}
