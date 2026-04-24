.. _blocks:

=====
Block
=====

Blocks are nameless functions that capture the local context by reference.
Blocks offer significant performance advantages over lambdas (see :ref:`Lambda <lambdas>`).

The block type can be declared with a function-like syntax.
The type is written as ``block`` followed by an optional type signature in angle brackets:

.. code-block:: das

    block < (arg1:int; arg2:float&) : bool >

The ``->`` operator can be used instead of ``:`` for the return type:

.. code-block:: das

    block < (arg1:int; arg2:float&) -> bool >   // equivalent

If no type signature is specified, ``block`` alone represents a block that takes no
arguments and returns nothing.

Blocks capture the current stack, so blocks can be passed, but never returned.
Block variables can only be passed as arguments.
Global or local block variables are prohibited; returning the block type is also prohibited:

.. code-block:: das

    def goo ( b : block )
        ...

    def foo ( b : block < (arg1:int, arg2:float&) : bool >
        ...

Blocks can be called via ``invoke``:

.. code-block:: das

    def radd(var ext:int&;b:block<(var arg:int&):int>):int {
        return invoke(b,ext)
    }

There is also a shorthand, where block can be called as if it was a function:

.. code-block:: das

    def radd(var ext:int&;b:block<(var arg:int&):int>):int {
        return b(ext)   // same as invoke(b,ext)
    }

Typeless blocks are typically declared via construction-like syntax:

.. code-block:: das

    goo() {                                // block without arguments
        print("inside goo")
    }

.. _blocks_declarations:

Similarly typed blocks are typically declared via assumed pipe syntax — a block or
lambda placed immediately after a function call is automatically passed as the last
argument.  This works with named function calls, dot-method calls, and arrow-method
calls (see :ref:`Pipe Operators <expressions>` for full details):

.. code-block:: das

    var v1 = 1                              // block with arguments
    res = radd(v1) $(var a:int&):int {
        return a++
    }

    // dot-method call
    var r = new Receiver()
    r.call_method() $ {                     // block piped to obj.method()
        print("called")
    }

    // arrow-method call
    c->fn() $ {                             // block piped to obj->fn()
        print("called")
    }

Blocks can also be declared via inline syntax:

.. code-block:: das

    res = radd(v1, $(var a:int&) : int { return a++; }) // equivalent to example above

There is a simplified syntax for blocks that only contain a return expression:

.. code-block:: das

    res = radd(v1, $(var a:int&) : int => a++ )         // equivalent to example above

If a block is sufficiently specified in the generic or function,
block types will be automatically inferred:

.. code-block:: das

    res = radd(v1, $(a) => a++ )                        // equivalent to example above

Nested blocks are allowed:

.. code-block:: das

    def passthroughFoo(a:Foo; blk:block<(b:Foo):void>) {
        invoke(blk,a)
    }

    passthrough(1) $(a) {
        assert(a==1)
        passthrough(2) $(b) {
            assert(a==1 && b==2)
            passthrough(3) $(c) {
                assert(a==1 && b==2 && c==3)
            }
        }
    }

Loop control expressions are not allowed to cross block boundaries:

.. code-block:: das

    while ( true ) {
        take_any() {
            break               // 30801, captured block can't break outside the block
        }
    }

Blocks can have annotations:

.. code-block:: das

    def queryOne(dt:float=1.0f) {
        testProfile::queryEs() $ [es] (var pos:float3&;vel:float3 const) { // [es] is annotation
            pos += vel * dt
        }
    }

Block annotations can be implemented via appropriate macros (see :ref:`Macro <macros>`).

Local block variables are allowed:

.. code-block:: das

    var blk = $ ( a, b : int ) {
        return a + b
    }
    verify ( 3 == invoke(blk,1,2) )
    verify ( 7 == invoke(blk,3,4) )

Local block variables cannot be copied or moved.

.. seealso::

    :ref:`Functions <functions>` for regular function declarations,
    :ref:`Lambdas <lambdas>` for heap-allocated callable objects with captures,
    :ref:`Generators <generators>` for iterator-producing lambdas,
    :ref:`Annotations <annotations>` for block annotations.

