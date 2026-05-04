Any type declaration.
Basic declaration type
Structure type if baseType is Type::tStructure
Enumeration type if baseType is Type::tEnumeration
Handled type if baseType is Type::tHandle
First type for compound types (like array<firstType> or table<firstType, secondType>)
Second type for compound types (like table<firstType, secondType>)
Argument types for function types, tuples, variants, etc
Argument names for function types
Dimensions for fixed array types
Dimension expressions for fixed array types, when dimension is specified by expression
Type declaration flags
Alias name for typedefs, i.e. 'int aka MyInt' or 'MyInt'
Location of the type declaration in the source code
Module this type belongs to
