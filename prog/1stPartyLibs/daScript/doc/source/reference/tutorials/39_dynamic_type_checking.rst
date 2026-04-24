.. _tutorial_dynamic_type_checking:

==========================================
Dynamic Type Checking
==========================================

.. index::
    single: Tutorial; Dynamic Type Checking
    single: Tutorial; RTTI
    single: Tutorial; is_instance_of
    single: Tutorial; dynamic_type_cast
    single: Tutorial; Downcast
    single: Tutorial; is / as Syntax

This tutorial covers runtime type checking and safe downcasting for class
hierarchies using the ``daslib/dynamic_cast_rtti`` module.  This requires
``options rtti`` to enable runtime type information.

.. code-block:: das

    options rtti
    require daslib/dynamic_cast_rtti

Class hierarchy setup
=====================

All examples use a small shape hierarchy:

.. code-block:: das

    class Shape {
        name : string
        def abstract const area() : float
    }

    class Circle : Shape {
        radius : float
        def override const area() : float {
            return 3.14159265 * radius * radius
        }
    }

    class Rectangle : Shape {
        width : float
        height : float
        def override const area() : float {
            return width * height
        }
    }

    class Square : Rectangle {
        def Square(s : float) {
            width = s
            height = s
        }
    }

Note the use of ``def abstract const`` and ``def override const`` — the
``const`` modifier makes ``self`` immutable, allowing the method to be called
through const pointers.

is_instance_of
==============

``is_instance_of`` checks whether a class pointer is an instance of a given
type at runtime, walking the RTTI chain.  Works with both exact and derived
types:

.. code-block:: das

    var c = new Circle(name = "circle", radius = 5.0)
    let s_c : Shape? = c

    print("{is_instance_of(s_c, type<Circle>)}\n")       // true
    print("{is_instance_of(s_c, type<Rectangle>)}\n")     // false

    // Derived types match base checks
    var sq = new Square(name = "square")
    let s_sq : Shape? = sq
    print("{is_instance_of(s_sq, type<Rectangle>)}\n")    // true
    print("{is_instance_of(s_sq, type<Shape>)}\n")        // true

    // Null is never an instance of anything
    var np : Shape? = null
    print("{is_instance_of(np, type<Shape>)}\n")          // false

dynamic_type_cast — safe downcast
=================================

``dynamic_type_cast`` returns a typed pointer if the cast succeeds, or
``null`` if it fails.  This is the "safe" downcast:

.. code-block:: das

    def describe_shape(s : Shape?) {
        let circle = dynamic_type_cast(s, type<Circle>)
        if (circle != null) {
            print("Circle: radius={circle.radius}, area={s->area()}\n")
            return
        }
        let rect = dynamic_type_cast(s, type<Rectangle>)
        if (rect != null) {
            print("Rectangle: {rect.width}x{rect.height}\n")
            return
        }
        print("Unknown shape: area={s->area()}\n")
    }

force_dynamic_type_cast
=======================

``force_dynamic_type_cast`` panics if the cast fails.  Use when you are
certain of the type and want a clear error if your assumption is wrong:

.. code-block:: das

    var c = new Circle(name = "c", radius = 7.0)
    let s : Shape? = c
    let fc = force_dynamic_type_cast(s, type<Circle>)
    print("radius={fc.radius}\n")    // 7

    // This would panic:
    // let fr = force_dynamic_type_cast(s, type<Rectangle>)

is / as / ?as syntax sugar
==========================

After requiring ``daslib/dynamic_cast_rtti``, you get syntactic sugar
that maps to the functions above:

==============================  ============================================
Syntax                          Equivalent
==============================  ============================================
``ptr is ClassName``            ``is_instance_of(ptr, type<ClassName>)``
``ptr as ClassName``            ``force_dynamic_type_cast(ptr, type<ClassName>)``
``ptr ?as ClassName``           ``dynamic_type_cast(ptr, type<ClassName>)``
==============================  ============================================

.. code-block:: das

    let s_c : Shape? = c

    // is — type check
    print("{s_c is Circle}\n")          // true
    print("{s_c is Rectangle}\n")       // false

    // as — force cast (panics on failure)
    let circle = s_c as Circle
    print("radius={circle.radius}\n")

    // ?as — safe cast (null on failure)
    let maybe_rect = s_c ?as Rectangle
    if (maybe_rect == null) {
        print("not a rectangle\n")
    }

Practical: polymorphic processing
=================================

Combine ``is`` and ``as`` to process a heterogeneous collection of shapes:

.. code-block:: das

    var shapes : array<Shape?>
    shapes |> push(new Circle(name = "sun", radius = 10.0))
    shapes |> push(new Rectangle(name = "wall", width = 8.0, height = 3.0))

    var total_area = 0.0
    for (s in shapes) {
        if (s is Circle) {
            let c = s as Circle
            print("{c.name}: circle r={c.radius}\n")
        } elif (s is Square) {
            let sq = s as Square
            print("{sq.name}: square side={sq.width}\n")
        } elif (s is Rectangle) {
            let r = s as Rectangle
            print("{r.name}: rect {r.width}x{r.height}\n")
        }
        total_area += s->area()
    }
    print("total area: {total_area}\n")

.. note::

   Check more-derived types first — ``Square`` must come before
   ``Rectangle`` since ``Square`` extends ``Rectangle``.

Summary
=======

======================================  ================================================
Function / Syntax                       Description
======================================  ================================================
``is_instance_of(ptr, type<T>)``        ``true`` if ptr is instance of T via RTTI
``dynamic_type_cast(ptr, T)``           Returns ``T?`` or ``null`` on failure
``force_dynamic_type_cast(ptr, T)``     Returns ``T?`` or panics on failure
``ptr is ClassName``                    Syntactic sugar for ``is_instance_of``
``ptr as ClassName``                    Syntactic sugar for force cast
``ptr ?as ClassName``                   Syntactic sugar for safe cast
======================================  ================================================

.. seealso::

   :ref:`Classes <classes>` — class inheritance and virtual methods.

   Full source: :download:`tutorials/language/39_dynamic_type_checking.das <../../../../tutorials/language/39_dynamic_type_checking.das>`

   Previous tutorial: :ref:`tutorial_random`

   Next tutorial: :ref:`tutorial_coroutines`
