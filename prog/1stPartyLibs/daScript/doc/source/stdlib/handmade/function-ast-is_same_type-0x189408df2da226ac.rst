Compares two type declarations for structural equality. Boolean flags control whether reference, const, and temporary qualifiers affect the comparison, and whether type substitution (e.g. auto inference) is allowed. Returns true if the types are considered the same under the specified criteria.

def is_same_type (argType: smart_ptr<TypeDecl>; passType: smart_ptr<TypeDecl>; refMatters: bool; constMatters: bool; temporaryMatters: bool; allowSubstitute: bool) : bool
