.. _generators:

=========
Generator
=========

Generators allow you to declare a lambda that behaves like an iterator.
For all intents and purposes, a generator is a lambda passed to an ``each`` or ``each_ref`` function.

Generator syntax is similar to lambda syntax::

    generator ::= `generator` < type > $ ( ) block

Generator lambdas must have no arguments. It always returns boolean::

    let gen <- generator<int>() <| $()  // gen is iterator<int>
        for t in range(0,10)
            yield t
        return false                    // returning false stops iteration

The result type of a ``generator`` expression is an iterator (see :ref:`Iterators <iterators>`).

Generators output iterator values via ``yield`` expressions.
Similar to the return statement, move semantic ``yield <-`` is allowed::

    return <- generator<TT> () <| $ ()
        for w in src
            yield <- invoke(blk,w)  // move invoke result
        return false

Generators can output ref types. They can have a capture section::

    unsafe                                                  // unsafe due to capture of src by reference
        var src = [[int 1;2;3;4]]
        var gen <- generator<int&> [[&src]] () <| $ ()      // capturing src by ref
            for w in src
                yield w                                     // yield of int&
            return false
        for t in gen
            t ++
        print("src = {src}\n")  // will output [[2;3;4;5]]

Generators can have loops and other control structures::

    let gen <- generator<int>() <| $()
        var t = 0
        while t < 100
            if t == 10
                break
            yield t ++
        return false

    let gen <- generator<int>() <| $()
        for t in range(0,100)
            if t >= 10
                continue
            yield t
        return false

Generators can have a ``finally`` expression on its blocks, with the exception of the if-then-else blocks::

    let gen <- generator<int>() <| $()
        for t in range(0,9)
            yield t
        finally
            yield 9
        return false

----------------------
implementation details
----------------------

In the following example::

    var gen <- generator<int> () <| $ ()
        for x in range(0,10)
            if (x & 1)==0
                yield x
        return false

A lambda is generated with all captured variables::

    struct _lambda_thismodule_8_8_1
        __lambda : function<(__this:_lambda_thismodule_8_8_1;_yield_8:int&):bool const> = @@_::_lambda_thismodule_8_8_1`function
        __finalize : function<(__this:_lambda_thismodule_8_8_1? -const):void> = @@_::_lambda_thismodule_8_8_1`finalizer
        __yield : int
        _loop_at_8 : bool
        x : int // captured constant
        _pvar_0_at_8 : void?
        _source_0_at_8 : iterator<int>

A lambda function is generated::

    [GENERATOR]
    def _lambda_thismodule_8_8_1`function ( var __this:_lambda_thismodule_8_8_1; var _yield_8:int& ) : bool const
        goto __this.__yield
        label 0:
        __this._loop_at_8 = true
        __this._source_0_at_8 <- __::builtin`each(range(0,10))
        memzero(__this.x)
        __this._pvar_0_at_8 = reinterpret<void?> addr(__this.x)
        __this._loop_at_8 &&= _builtin_iterator_first(__this._source_0_at_8,__this._pvar_0_at_8,__context__)
        label 3: /*begin for at line 8*/
        if !__this._loop_at_8
                goto label 5
        if !((__this.x & 1) == 0)
                goto label 2
        _yield_8 = __this.x
        __this.__yield = 1
        return /*yield*/ true
        label 1: /*yield at line 10*/
        label 2: /*end if at line 9*/
        label 4: /*continue for at line 8*/
        __this._loop_at_8 &&= _builtin_iterator_next(__this._source_0_at_8,__this._pvar_0_at_8,__context__)
        goto label 3
        label 5: /*end for at line 8*/
        _builtin_iterator_close(__this._source_0_at_8,__this._pvar_0_at_8,__context__)
        return false

Control flow statements are replaced with the ``label`` + ``goto`` equivalents.
Generators always start with ``goto __this.yield``.
This effectively produces a finite state machine, with the ``yield`` variable holding current state index.

The ``yield`` expression is converted into a copy result and return value pair.
A label is created to specify where to go to next time, after the ``yield``::

    _yield_8 = __this.x                 // produce next iterator value
    __this.__yield = 1                  // label to go to next (1)
    return /*yield*/ true               // return true to indicate, that iterator produced a value
    label 1: /*yield at line 10*/       // next label marker (1)

Iterator initialization is replaced with the creation of the lambda::

    var gen:iterator<int> <- each(new<lambda<(_yield_8:int&):bool const>> [[_lambda_thismodule_8_8_1]])
