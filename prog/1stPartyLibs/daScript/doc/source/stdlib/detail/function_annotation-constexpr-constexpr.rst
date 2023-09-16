This macro implements a constexpr function argument checker. Given list of arguments to verify, it will fail for every one where non-constant expression is passed. For example::

    [constexpr (a)]
    def foo ( t:string; a : int )
        print("{t} = {a}\n")
    var BOO = 13
    [export]
    def main
        foo("blah", 1)
        foo("ouch", BOO)    // comilation error: `a is not a constexpr, BOO`
