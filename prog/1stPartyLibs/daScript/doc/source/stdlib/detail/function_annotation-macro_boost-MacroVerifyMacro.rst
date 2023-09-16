This macro implements `macro_verify` macro. It's equivalent to a function call::

    def macro_verify ( expr:bool; prog:ProgramPtr; at:LineInfo; message:string )

However, result will be substituted with::

    if !expr
        macro_error( prog, at, message )
        return [[ExpressionPtr]]