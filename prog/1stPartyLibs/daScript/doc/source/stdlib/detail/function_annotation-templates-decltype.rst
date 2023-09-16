This macro returns ast::TypeDecl for the corresponding expression. For example::

    let x = 1
    let y <- decltype(x) // [[TypeDecl() baseType==Type tInt, flags=TypeDeclFlags constant | TypeDeclFlags ref]]

