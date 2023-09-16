.. _unsafe:


======
Unsafe
======

.. index::
    single: Unsafe

The ``unsafe`` keyword denotes unsafe contents, which is required for operations, but could potentially crash the application::

    unsafe
        let px = addr(x)

Expressions (and subexpressions) can also be unsafe::

    let px = unsafe(addr(x))

Unsafe is followed by a block which can include those operations. Nested unsafe sections are allowed. Unsafe is not inherited in lambda, generator, or local functions; it is, however, inherited in local blocks.

Individual expressions can cause a `CompilationError::unsafe` error, unless they are part of the unsafe section. Additionally, macros can explicitly set the `ExprGenFlags::alwaysSafe` flag.

The address of expression is unsafe::

    unsafe
        let a : int
        let pa = addr(a)
        return pa                               // accessing *pa can potentially corrupt stack

Lambdas or generators require unsafe sections for the implicit capture by move or by reference::

    var a : array<int>
    unsafe
        var counter <- @ <| (extra:int) : int
            return a[0] + extra                 // a is implicitly moved

Deleting any pointer requires an unsafe section::

    var p = new Foo()
    var q = p
    unsafe
        delete p                                // accessing q can potentially corrupt memory

Upcast and reinterpret cast require an unsafe section::

    unsafe
        return reinterpret<void?> 13            // reinterpret can create unsafe pointers

Indexing into a pointer is unsafe::

    unsafe
        var p = new Foo()
        return p[13]                            // accessing out of bounds pointer can potentially corrupt memory

A safe index is unsafe when not followed by the null coalescing operator::

    var a = {{ 13 => 12 }}
    unsafe
        var t = a?[13] ?? 1234                  // safe
        return a?[13]                           // unsafe; safe index is a form of 'addr' operation
                                                // it can create pointers to temporary objects

Variant ``?as`` on local variables is unsafe when not followed by the null coalescing operator::

    unsafe
        return a ?as Bar                        // safe as is a form of 'addr' operation

Variant ``.?field`` is unsafe when not followed by the null coalescing operator::

    unsafe
        return a?.Bar                           // safe navigation of a variant is a form of 'addr' operation


Variant ``.field`` is unsafe::

    unsafe
        return a.Bar                            // this is potentially a reinterpret cast

Certain functions and operators are inherently unsafe or marked unsafe via the [unsafe_operation] annotation::

    unsafe
        var a : int?
        a += 13                                 // pointer arithmetic can create invalid pointers
        var boo : int[13]
        var it = each(boo)                      // each() of array is unsafe, for it does not capture

Moving from a smart pointer value requires unsafe, unless that value is the 'new' operator::

    unsafe
        var a <- new TestObjectSmart()          // safe, its explicitly new
        var b <- someSmartFunction()            // unsafe since lifetime is not obvious
        b <- a                                  // safe, values are not lost

Moving or copying classes is unsafe::

    def foo ( var b : TestClass )
        unsafe
            var a : TestClass
            a <- b                              // potentially moving from derived class

Local class variables are unsafe::

    unsafe
        var g = Goo()                           // potential lifetime issues

========
implicit
========

`implicit` keyword is used to specify that type can be either temporary or regular type, and will be treated as defined.
For example::

    def foo ( a : Foo implicit )    // a will be treated as Foo, but will also accept Foo# as argument
    def foo ( a : Foo# implicit )   // a will be treated as Foo#, but will also accept Foo as argument

Unfortunately implicit conversions like this are unsafe, so `implicit` is unsafe by definition.

