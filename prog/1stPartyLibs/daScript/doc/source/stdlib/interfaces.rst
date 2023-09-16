
.. _stdlib_interfaces:

==========
Interfaces
==========

.. include:: detail/interfaces.rst

The interface module implements [interface] pattern, which allows classes to expose multiple interfaces.

All functions and symbols are in "interfaces" module, use require to get access to it. ::

    require daslib/interfaces

Lets review the following example::

    require daslib/interfaces

    [interface]
    class ITick
        def abstract beforeTick : bool
        def abstract tick ( dt:float ) : void
        def abstract afterTick : void

    [interface]
    class ILogger
        def abstract log ( message : string ) : void

    [implements(ITick),implements(ILogger)]
    class Foo
        def Foo
            pass
        def ITick`tick ( dt:float )
            print("tick {dt}\n")
        def ITick`beforeTick
            print("beforeTick\n")
            return true
        def ITick`afterTick
            print("afterTick\n")
        def ILogger`log ( message : string )
            print("log {message}\n")

In the example above, we define two interfaces, ITick and ILogger. Then we define a class Foo, which implements both interfaces. The class Foo must implement all methods of both interfaces. The class Foo can implement additional methods, which are not part of the interfaces.

The [implements] attribute is used to specify which interfaces the class implements.

The [interface] attribute is used to define an interface. This macro verifies that the interface does not have any data members, only methods.

Interface methods are automatically bound to specific interfaces, by pattern-matching the method name. For example the method "tick" is bound to the interface ITick, because the method name starts with "ITick`". The method "log" is bound to the interface ILogger, because the method name starts with "ILogger`".

Additionally get`ITick and get`ILogger methods are generated for the Foo class. They are used to get the interface object for the given interface. The interface object is used to call the interface methods. ::

    var f = new Foo()
    f->get`ITick()->beforeTick()
    f->get`ITick()->tick(1.0)
    f->get`ITick()->afterTick()
    f->get`ILogger()->log("hello")
++++++++++++++++
Structure macros
++++++++++++++++

.. _handle-interfaces-interface:

.. das:attribute:: interface

implements 'interface' macro, which verifies if class is an interface (no own variables)

.. _handle-interfaces-implements:

.. das:attribute:: implements

implements 'implements' macro, adds get`{Interface} method as well as interface bindings and implementation.


