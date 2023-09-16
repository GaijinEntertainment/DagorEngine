.. _lambdas:

======
Lambda
======

Lambdas are nameless functions which capture the local context by clone, copy, or reference.
Lambdas are slower than blocks, but allow for more flexibility in lifetime and capture modes  (see :ref:`Blocks <blocks>`).

The lambda type can be declared with a function-like syntax::

    lambda_type ::= lambda { optional_lambda_type }
    optional_lambda_type ::= < { optional_lambda_arguments } { : return_type } >
    optional_lambda_arguments := ( lambda_argument_list )
    lambda_argument_list := argument_name : type | lambda_argument_list ; argument_name : type

    lambda < (arg1:int;arg2:float&):bool >

Lambdas can be local or global variables, and can be passed as an argument by reference.
Lambdas can be moved, but can't be copied or cloned::

    def foo ( x : lambda < (arg1:int;arg2:float&):bool > )
        ...
        var y <- x
        ...

Lambdas can be invoked via ``invoke``::

    def inv13 ( x : lambda < (arg1:int):int > )
        return invoke(x,13)

Lambdas are typically declared via pipe syntax::

    var CNT = 0
    let counter <- @ <| (extra:int) : int
        return CNT++ + extra
    let t = invoke(counter,13)

There are a lot of similarities between lambda and block declarations.
The main difference is that blocks are specified with ``$`` symbol, while lambdas are specified with ``@`` symbol.
Lambdas can also be declared via inline syntax.
There is a similar simplified syntax for the lambdas containing return expression only.
If a lambda is sufficiently specified in the generic or function,
its types will be automatically inferred (see :ref:`Blocks <blocks_declarations>`).

-------
Capture
-------

Unlike blocks, lambdas can specify their capture types explicitly. There are several available types of capture:

    * by copy
    * by move
    * by clone
    * by reference

Capturing by reference requires unsafe.

By default, capture by copy will be generated. If copy is not available, unsafe is required for the default capture by move::

	var a1 <- [{int 1;2}]
	var a2 <- [{int 1;2}]
	var a3 <- [{int 1;2}]
	unsafe  // required do to capture of a1 by reference
		var lam <- @ <| [[&a1,<-a2,:=a3]]
			push(a1,1)
			push(a2,1)
			push(a3,1)
		invoke(lam)

.. _lambdas_finalizer:

Lambdas can be deleted, which cause finalizers to be called on all captured data  (see :ref:`Finalizers <finalizers>`)::

    delete lam

Lambdas can specify a custom finalizer which is invoked before the default finalizer::

    var CNT = 0
    var counter <- @ <| (extra:int) : int
        return CNT++ + extra
    finally
        print("CNT = {CNT}\n")
    var x = invoke(counter,13)
    delete counter                  // this is when the finalizer is called

.. _lambdas_iterator:

---------
Iterators
---------

Lambdas are the main building blocks for implementing custom iterators (see :ref:`Iterators <iterators>`).

Lambdas can be converted to iterators via the ``each`` or ``each_ref`` functions::

    var count = 0
    let lam <- @ <| (var a:int &) : bool
        if count < 10
            a = count++
            return true
        else
            return false
    for x,tx in each(lam),range(0,10)
        assert(x==tx)

To serve as an iterator, a lambda must

    * have single argument, which is the result of the iteration for each step
    * have boolean return type, where ``true`` means continue iteration, and ``false`` means stop

A more straightforward way to make iterator is with generators (see :ref:`Generators <generators>`).

----------------------
Implementation details
----------------------

Lambdas are implemented by creating a nameless structure for the capture, as well as a function for the body of the lambda.

Let's review an example with a singled captured variable::

    var CNT = 0
    let counter <- @ <| (extra:int) : int
        return CNT++ + extra

daScript will generated the following code:

Capture structure::

    struct _lambda_thismodule_7_8_1
        __lambda : function<(__this:_lambda_thismodule_7_8_1;extra:int const):int> = @@_lambda_thismodule_7_8_1`function
        __finalize : function<(__this:_lambda_thismodule_7_8_1? -const):void> = @@_lambda_thismodule_7_8_1`finalizer
        CNT : int

Body function::

    def _lambda_thismodule_7_8_1`function ( var __this:_lambda_thismodule_7_8_1; extra:int const ) : int
        with __this
            return CNT++ + extra

Finalizer function::

    def _lambda_thismodule_7_8_1`finalizer ( var __this:_lambda_thismodule_7_8_1? explicit )
        delete *this
        delete __this

Lambda creation is replaced with the ascend of the capture structure::

    let counter:lambda<(extra:int const):int> const <- new<lambda<(extra:int const):int>> [[CNT = CNT]]

The C++ Lambda class contains single void pointer for the capture data::

    struct Lambda {
        ...
        char *      capture;
        ...
    };

The rational behind passing lambda by reference is that when delete is called

    1. the finalizer is invoked for the capture data
    2. the capture is replaced via null

The lack of a copy or move ensures there are not multiple pointers to a single instance of the captured data floating around.
