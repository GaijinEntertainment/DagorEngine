expect 30115    // structure_field_already_declared

struct Foo
    a:int
    a:int       // 30115: structure field is already declared a, use override to replace initial value instead

