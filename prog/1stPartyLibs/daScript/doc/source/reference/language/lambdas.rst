.. _lambdas:

======
Lambda
======

Lambdas are nameless functions which capture the local context by clone, copy, or reference.
Lambdas are slower than blocks, but allow for more flexibility in lifetime and capture modes  (see :ref:`Blocks <blocks>`).

The lambda type can be declared with a function-like syntax.
The type is written as ``lambda`` followed by an optional type signature in angle brackets:

.. code-block:: das

    lambda < (arg1:int; arg2:float&) : bool >

The ``->`` operator can be used instead of ``:`` for the return type:

.. code-block:: das

    lambda < (arg1:int; arg2:float&) -> bool >   // equivalent

If no type signature is specified, ``lambda`` alone represents a lambda that takes no
arguments and returns nothing.

Lambdas can be local or global variables, and can be passed as an argument by reference.
Lambdas can be moved, but can't be copied or cloned:

.. code-block:: das

    def foo ( x : lambda < (arg1:int;arg2:float&):bool > ) {
        ...
        var y <- x
        ...
    }

Lambdas can be invoked via ``invoke`` or call-like syntax:

.. code-block:: das

    def inv13 ( x : lambda < (arg1:int):int > ) {
        return invoke(x,13)
    }

    def inv14 ( x : lambda < (arg1:int):int > ) {
        return x(14)
    }

Lambdas are typically declared via move syntax:

.. code-block:: das

    var CNT = 0
    let counter <- @ (extra:int) : int {
        return CNT++ + extra
    }
    let t = invoke(counter,13)

There are many similarities between lambda and block declarations.
The main difference is that blocks are specified with ``$`` symbol, while lambdas are specified with ``@`` symbol.
Lambdas can also be declared via inline syntax.
There is a similar simplified syntax for the lambdas containing return expression only.
If a lambda is sufficiently specified in the generic or function,
its types will be automatically inferred (see :ref:`Blocks <blocks_declarations>`).

-------
Capture
-------

Unlike blocks, lambdas can specify their capture types explicitly. There are several available types of capture:

    * by copy (``=``)
    * by move (``<-``)
    * by clone (``:=``)
    * by reference (``&``)

Capturing by reference requires unsafe.

By default, capture by copy is used. If copy is not available, the ``unsafe`` keyword is required for the default capture by move:

.. code-block:: das

	var a1 <- [1,2]
	var a2 <- [1,2]
	var a3 <- [1,2]
	unsafe { // required do to capture of a1 by reference
		var lam <- @  capture(ref(a1),move(a2),clone(a3)) {
			push(a1,1)
			push(a2,1)
			push(a3,1)
        }
		invoke(lam)
    }

.. _lambdas_finalizer:

Lambdas can be deleted, which causes finalizers to be called on all captured data  (see :ref:`Finalizers <finalizers>`):

.. code-block:: das

    delete lam

Lambdas can specify a custom finalizer which is invoked before the default finalizer:

.. code-block:: das

    var CNT = 0
    var counter <- @ (extra:int) : int {
        return CNT++ + extra
    } finally {
        print("CNT = {CNT}\n")
    }
    var x = invoke(counter,13)
    delete counter                  // this is when the finalizer is called

.. _lambdas_iterator:

---------
Iterators
---------

Lambdas are the main building blocks for implementing custom iterators (see :ref:`Iterators <iterators>`).

Lambdas can be converted to iterators via the ``each`` or ``each_ref`` functions:

.. code-block:: das

    var count = 0
    let lam <- @ (var a:int &) : bool {
        if ( count < 10 ) {
            a = count++
            return true
        } else {
            return false
        }
    }
    for ( x,tx in each(lam),range(0,10) ) {
        assert(x==tx)
    }

To serve as an iterator, a lambda must

    * have single argument, which is the result of the iteration for each step
    * have boolean return type, where ``true`` means continue iteration, and ``false`` means stop

A more straightforward way to make iterator is with generators (see :ref:`Generators <generators>`).

----------------------
Implementation details
----------------------

Lambdas are implemented by creating a nameless structure for the capture, as well as a function for the body of the lambda.

Let's review an example with a singled captured variable:

.. code-block:: das

    var CNT = 0
    let counter <- @ (extra:int) : int {
        return CNT++ + extra
    }

Daslang will generated the following code:

Capture structure:

.. code-block:: das

    struct _lambda_thismodule_7_8_1 {
        __lambda : function<(__this:_lambda_thismodule_7_8_1;extra:int const):int> = @@_lambda_thismodule_7_8_1`function
        __finalize : function<(__this:_lambda_thismodule_7_8_1? -const):void> = @@_lambda_thismodule_7_8_1`finalizer
        CNT : int
    }

Body function:

.. code-block:: das

    def _lambda_thismodule_7_8_1`function ( var __this:_lambda_thismodule_7_8_1; extra:int const ) : int {
        with ( __this ) {
            return CNT++ + extra
        }
    }

Finalizer function:

.. code-block:: das

    def _lambda_thismodule_7_8_1`finalizer ( var __this:_lambda_thismodule_7_8_1? explicit ) {
        delete *this
        delete __this
    }

Lambda creation is replaced with the ascend of the capture structure:

.. code-block:: das

    let counter:lambda<(extra:int const):int> const <- new<lambda<(extra:int const):int>> (CNT = CNT)

The C++ Lambda class contains single void pointer for the capture data:

.. code-block:: das

    struct Lambda {
        ...
        char *      capture;
        ...
    };

The rationale behind passing lambdas by reference is that when ``delete`` is called:

    1. the finalizer is invoked for the capture data
    2. the capture is replaced with null

The lack of copy or move semantics ensures that multiple pointers to a single instance of captured data cannot exist.

.. seealso::

    :ref:`Blocks <blocks>` for stack-bound callable objects without captures,
    :ref:`Generators <generators>` for iterator-producing lambdas,
    :ref:`Functions <functions>` for regular named functions,
    :ref:`Unsafe <unsafe>` for implicit capture by reference or move,
    :ref:`Finalizers <finalizers>` for lambda finalizers.
