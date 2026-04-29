.. _tutorial_interfaces:

==========================================
Interfaces
==========================================

.. index::
    single: Tutorial; Interfaces
    single: Tutorial; Interface Polymorphism
    single: Tutorial; is / as / ?as (interfaces)
    single: Tutorial; Interface Inheritance
    single: Tutorial; Default Methods
    single: Tutorial; Const-only Interfaces

This tutorial covers **interface-based polymorphism** using the
``daslib/interfaces`` module.  Interfaces let you define abstract
method contracts that structs can implement, enabling polymorphic
dispatch without class inheritance.

Prerequisites: :ref:`tutorial_dynamic_type_checking` (Tutorial 39).

.. code-block:: das

    require daslib/interfaces


Defining an interface
=====================

Use ``[interface]`` on a class to declare it as an interface.  An
interface may contain only function-typed fields — abstract methods
or default implementations:

.. code-block:: das

    [interface]
    class IGreeter {
        def abstract greet(name : string) : string
    }


Implementing an interface
=========================

Use ``[implements(IFoo)]`` on a struct to generate the proxy class
and getter.  Implement each method with the ``InterfaceName`method``
naming convention:

.. code-block:: das

    [implements(IGreeter)]
    class FriendlyGreeter {
        def FriendlyGreeter() {
            pass
        }
        def IGreeter`greet(name : string) : string {
            return "Hey, {name}!"
        }
    }


``is`` / ``as`` / ``?as`` operators
====================================

The ``InterfaceAsIs`` variant macro enables three operators that
work with interface types:

.. code-block:: das

    var g = new FriendlyGreeter()

    // is — compile-time check (zero runtime cost)
    print("{g is IGreeter}\n")         // true

    // as — returns the interface proxy
    var iface = g as IGreeter
    iface->greet("Alice")

    // ?as — null-safe: returns null when the pointer is null
    var nothing : FriendlyGreeter?
    var safe = nothing ?as IGreeter    // null

``is`` is resolved entirely at compile time — the result is baked
into the program as a boolean constant.


Multiple interfaces
===================

A struct can implement any number of interfaces by listing multiple
``[implements(...)]`` annotations:

.. code-block:: das

    [interface]
    class IDrawable {
        def abstract draw(x, y : int) : void
    }

    [interface]
    class ISerializable {
        def abstract serialize : string
    }

    [implements(IDrawable), implements(ISerializable)]
    class Sprite {
        name : string
        def Sprite(n : string) { name = n }
        def IDrawable`draw(x, y : int) {
            print("Sprite \"{name}\" at ({x},{y})\n")
        }
        def ISerializable`serialize() : string {
            return "sprite:{name}"
        }
    }


Polymorphic dispatch
====================

Interfaces enable true polymorphic dispatch — pass interface proxies
to functions that accept the interface type:

.. code-block:: das

    def draw_all(var objects : array<IDrawable?>) {
        for (obj in objects) {
            obj->draw(0, 0)
        }
    }

    var drawables : array<IDrawable?>
    drawables |> push(circle as IDrawable)
    drawables |> push(sprite as IDrawable)
    draw_all(drawables)


Interface inheritance
=====================

Interfaces can extend other interfaces using normal class inheritance
syntax.  A struct that implements a derived interface automatically
supports ``is``/``as``/``?as`` for all ancestor interfaces:

.. code-block:: das

    [interface]
    class IAnimal {
        def abstract name : string
        def abstract sound : string
    }

    [interface]
    class IPet : IAnimal {
        def abstract owner : string
    }

    [implements(IPet)]
    class Dog {
        dog_name : string
        owner_name : string
        def Dog(n, o : string) { dog_name = n; owner_name = o }
        def IPet`name() : string { return dog_name }
        def IPet`sound() : string { return "Woof" }
        def IPet`owner() : string { return owner_name }
    }

Now ``Dog`` supports both ``IPet`` and ``IAnimal``:

.. code-block:: das

    var d = new Dog("Rex", "Alice")
    print("{d is IPet}\n")       // true
    print("{d is IAnimal}\n")    // true

    var animal = d as IAnimal
    animal->name()               // "Rex"
    animal->sound()              // "Woof"


Default method implementations
==============================

Non-abstract methods in an interface class provide default
implementations.  The implementing struct only needs to override
the abstract methods — defaults are inherited by the proxy:

.. code-block:: das

    [interface]
    class ILogger {
        def abstract log_name : string
        // Default — optional to override
        def format(message : string) : string {
            return "[{self->log_name()}] {message}"
        }
    }

    [implements(ILogger)]
    class SimpleLogger {
        def SimpleLogger() { pass }
        def ILogger`log_name() : string { return "simple" }
        // format() uses ILogger's default
    }

A struct that overrides the default provides its own implementation:

.. code-block:: das

    [implements(ILogger)]
    class FancyLogger {
        def FancyLogger() { pass }
        def ILogger`log_name() : string { return "fancy" }
        def ILogger`format(message : string) : string {
            return "*** [fancy] {message} ***"
        }
    }


Completeness checking
=====================

If an implementing struct is missing an abstract method, the compiler
reports an error at compile time:

.. code-block:: text

    error[30111]: Foo does not implement IBar.method

Methods with default implementations are optional — the proxy
inherits the default from the interface class.  Only abstract
methods (``def abstract``) are required.


Const-only interfaces
=====================

When all methods in an interface are ``def abstract const``, the
generated getter also works on ``const`` pointers.  This means
``is``, ``as``, and ``?as`` work without requiring a mutable pointer:

.. code-block:: das

    [interface]
    class IReadable {
        def abstract const read() : string
    }

    [implements(IReadable)]
    class Document {
        text : string
        def Document(t : string) { text = t }
        def IReadable`read() : string { return text }
    }

Because ``IReadable`` is const-only, you can use ``as`` on a
``const`` pointer:

.. code-block:: das

    def print_content(doc : Document? const) {
        var reader = doc as IReadable
        print("{reader->read()}\n")
    }

This is particularly useful for read-only interfaces where callers
should not need a mutable reference to query an object's state.


Quick reference
===============

======================================  ================================================
Syntax                                  Description
======================================  ================================================
``[interface]``                         Mark a class as an interface
``[implements(IFoo)]``                  Generate proxy and getter for ``IFoo``
``def abstract method(...) : T``        Declare an abstract method (required)
``def abstract const method(...) : T``  Declare an abstract const method
``def method(...) : T { ... }``         Declare a default method (optional to override)
``def IFoo`method(...)``                Implement (or override) a method on a struct
``ptr is IFoo``                         Compile-time check (true/false)
``ptr as IFoo``                         Get the interface proxy
``ptr ?as IFoo``                        Null-safe proxy access
``class IChild : IParent``              Interface inheritance
======================================  ================================================

When all methods are ``const``, ``is``/``as``/``?as`` also work on
``const`` pointers.


.. seealso::

   :ref:`Dynamic Type Checking <tutorial_dynamic_type_checking>` — ``is``/``as`` on
   class hierarchies (Tutorial 39).

   :ref:`Variant Macros <tutorial_macro_variant_macro>` — how the
   ``InterfaceAsIs`` macro works internally (Macro Tutorial 8).

   :ref:`Interfaces module <stdlib_interfaces>` — standard library reference.

   Full source: :download:`tutorials/language/43_interfaces.das <../../../../tutorials/language/43_interfaces.das>`

   Previous tutorial: :ref:`tutorial_testing_tools`

   Next tutorial: :ref:`tutorial_compile_and_run`
