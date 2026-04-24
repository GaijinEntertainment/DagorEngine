.. _classes:

=====
Class
=====

In Daslang, classes are an extension of structures designed to provide OOP capabilities.
Classes provide single-parent inheritance, abstract and virtual methods, initializers, and finalizers.

The basic class declaration is similar to that of a structure, but with the ``class`` keyword:

.. code-block:: das

    class Foo {
        x, y : int = 0
        def Foo {                           // custom initializer
            Foo`set(self,1,1)
        }
        def set(X,Y:int) {                  // inline method
            x = X
            y = Y
        }
    }

----------------------------
Member Visibility
----------------------------

Class members can be ``private`` or ``public``. Private members are only accessible from
inside the class:

.. code-block:: das

    class Account {
        private balance : int = 0
        def public deposit(amount : int) {
            balance += amount
        }
        def public get_balance : int {
            return balance
        }
    }

Methods can also be declared ``private``:

.. code-block:: das

    class Processor {
        def private validate(x : int) : bool {
            return x > 0
        }
        def process(x : int) {
            if (validate(x)) {
                print("{x}\n")
            }
        }
    }

----------------------------
Initializers
----------------------------

The initializer is a function with a name matching that of the class.
Classes can have multiple initializers with different arguments:

.. code-block:: das

    class Foo {
        ...
        def Foo(T:int) {                    // custom initializer
            set(T,T)
        }
        def Foo(X,Y:int) {                  // custom initializer
            Foo`set(self,X,Y)
        }
    }

.. _classes_finalizer:

Finalizers can be defined explicitly as void functions named ``finalize``:

.. code-block:: das

    class Foo {
        ...
        def finalize {                      // custom finalizer
            delFoo ++
        }
    }

An alternative syntax for finalizers is:

.. code-block:: das

    class Foo {
        ...
        def operator delete {              // custom finalizer
            delFoo ++
        }
    }

There are no guarantees that a finalizer is called implicitly (see :ref:`Finalizers <finalizers>`).

Derived classes need to override methods explicitly, using the ``override`` keyword:

.. code-block:: das

    class Foo3D : Foo {
        z : int = 13
        def Foo3D {                         // overriding default initializer
            Foo`Foo(self)                   // call parents initializer explicitly
            z = 3
        }
        def override set(X,Y:int) {         // overriding parent method
            Foo`set(self,X,Y)               // calling generated method function directly
            z = 0
        }
    }

----------------------------
Calling Parent Methods
----------------------------

Inside a derived class, ``super()`` calls the parent class constructor:

.. code-block:: das

    class Derived : Base {
        def Derived {
            super()             // calls Base's default initializer
            z = 3
        }
    }

``super.method(args)`` calls a specific parent method, bypassing the virtual dispatch:

.. code-block:: das

    class Derived : Base {
        def override process(x : int) {
            super.process(x)    // calls Base`process, not the overridden version
            // additional logic
        }
    }

Both forms are rewritten by the compiler into explicit calls to the parent class function:
``super()`` becomes ``Base`Base(self)`` and ``super.process(x)`` becomes ``Base`process(self, x)``.

The option ``always_call_super`` can be enabled to require ``super()`` in every constructor
(see :ref:`Options <options>`).

Alternatively, the parent's method can be called directly using the backtick syntax:

.. code-block:: das

    Foo`set(self, X, Y)         // equivalent to super.set(X, Y) from within Foo3D

Classes can define abstract methods using the ``abstract`` keyword:

.. code-block:: das

    class FooAbstract {
        def abstract set(X,Y:int) : void             // inline method
    }

Abstract method declarations must be fully qualified, including their return type.
Class member functions are inferred in the same manner as regular functions.

Sealed functions cannot be overridden. The ``sealed`` keyword is used to prevent overriding:

.. code-block:: das

    class Foo3D : Foo {
        def sealed set(X,Y:int ) { // subclasses of Foo3D can no longer override this method
            xyz = X + Y
        }
    }

Sealed classes can not be inherited from. The ``sealed`` keyword is used to prevent inheritance:

.. code-block:: das

    class sealed Foo3D : Foo        // Foo3D can no longer be inherited from
        ...

A pointer named ``self`` is available inside any class method.
Because the method body is wrapped in ``with(self)``, all fields and methods
can be accessed directly — ``self.`` is not required:

.. code-block:: das

    class Foo {
        x : int
        def set(val : int) {
            x = val             // same as self.x = val
        }
    }

Classes can be created via the ``new`` operator:

.. code-block:: das

    var f = new Foo()

Local class variables are unsafe:

.. code-block:: das

    unsafe {
        var f = Foo()       // unsafe
    }

Class methods can be invoked using ``.`` syntax:

.. code-block:: das

    f.set(1,2)

The ``->`` operator can also be used:

.. code-block:: das

    f->set(1,2)

A specific version of the method can also be called explicitly:

.. code-block:: das

    Foo`set(*f,1,2)

Class methods can be constant:

.. code-block:: das

    class Foo {
        dir : float3
        def const length {
            return length(dir)  // dir is const float3 here
        }
    }

Class methods can be operators:

.. code-block:: das

    class Foo {
        dir : float3
        def Foo ( x,y,z:float ) {
            dir = float3(x,y,z)
        }
        def Foo ( d:float3 ) {
            dir = d
        }
        def const operator . length {
            return length(dir)
        }
        def operator . length := (value : float) {
            dir = normalize(dir) * value
        }
        def const operator + (other : Foo) : Foo {
            return unsafe(Foo(dir + other.dir))
        }
    }

Class fields can be declared static, i.e. shared between all instances of the class:

.. code-block:: das

    class Foo {
        static count : int = 0
        def Foo {
            count ++
        }
        def finalize {
            count --
        }
    }

Class methods can be declared static. Static methods don't have access to 'self' but can access static fields:

    .. code-block:: das

        class Foo {
            static count : int = 0
            def static getCount : int {
                return count
            }
        }

	    let count = Foo`getCount()  // they can be accessed outside of class

----------------------------
Runtime Type Checking (is)
----------------------------

The ``is`` operator checks whether a class instance is of a given type or any type derived
from it. This requires the ``daslib/dynamic_cast_rtti`` module:

.. code-block:: das

    require daslib/dynamic_cast_rtti

    class Animal {
        def abstract speak : string
    }

    class Dog : Animal {
        def override speak : string {
            return "woof"
        }
    }

    class Cat : Animal {
        def override speak : string {
            return "meow"
        }
    }

    var a : Animal? = new Dog()
    assert(a is Dog)            // true — a points to a Dog
    assert(a is Animal)         // true — Dog is an Animal
    assert(!(a is Cat))         // true — a is not a Cat

Without ``daslib/dynamic_cast_rtti``, the ``is`` operator performs a static (compile-time) type
check only. With the module, it performs runtime RTTI checking by walking the class hierarchy
via the ``__rtti`` field.

----------------------------
Type Casting (as, ?as)
----------------------------

The ``as`` and ``?as`` operators cast a class pointer to a more specific type.
These also require ``daslib/dynamic_cast_rtti``.

``as`` performs a forced cast — it panics if the cast fails:

.. code-block:: das

    var a : Animal? = new Dog()
    var d = a as Dog            // succeeds: a is a Dog
    d.speak()                   // "woof"

``?as`` performs a safe cast — it returns ``null`` if the cast fails:

.. code-block:: das

    var a : Animal? = new Dog()
    var c = a ?as Cat           // returns null: a is not a Cat
    if (c != null) {
        c.speak()
    }

Use ``?as`` when you are not certain of the runtime type. Use ``as`` when failure indicates a
programming error.

Custom ``operator is``, ``operator as``, and ``operator ?as`` can also be defined for non-class
types (see :ref:`Pattern Matching <pattern-matching>`).

----------------------------
Class Templates
----------------------------

A ``class template`` declaration creates a parameterized class pattern. Template classes are
not instantiated directly — they serve as blueprints for code generation via macros:

.. code-block:: das

    [template_structure(KeyType, ValueType)]
    class template TCache {
        keys : array<KeyType>
        values : array<ValueType>
        def lookup(key : KeyType) : ValueType? {
            for (k, v in keys, values) {
                if (k == key) {
                    return unsafe(addr(v))
                }
            }
            return null
        }
    }

Template parameters (``KeyType``, ``ValueType``) are replaced with concrete types during
instantiation. The ``template_structure`` annotation (from ``daslib/typemacro_boost``) handles
the substitution.

Template methods automatically inherit the template flag.

.. _class_method_annotation:

-------------------------------------------
Static methods with ``[class_method]``
-------------------------------------------

The ``[class_method]`` annotation (from ``daslib/class_boost``) turns a ``def static`` function
inside a struct or class into a method with an implicit ``self`` argument. The macro
automatically adds ``self`` as the first parameter and wraps the function body in
``with(self) { ... }``, allowing direct access to fields without ``self.`` prefix.

This is the primary way to define methods on structs, since structs don't have built-in
method dispatch like classes. It also works on classes when virtual dispatch is not needed.

Methods defined with ``[class_method]`` are called using dot syntax: ``obj.method()``.

``def static`` — mutable method
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``self`` parameter is mutable, so the method can modify fields:

.. code-block:: das

    require daslib/class_boost

    struct Counter {
        value : int = 0

        [class_method]
        def static increment(amount : int = 1) {
            value += amount
        }

        [class_method]
        def static reset {
            value = 0
        }
    }

``def static const`` — const method
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``self`` parameter is const, so the method cannot modify fields:

.. code-block:: das

    struct Counter {
        ...
        [class_method]
        def static const get_value : int {
            return value
        }
    }

``[explicit_const_class_method]``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When both a const and a non-const overload of the same method are needed, use
``[explicit_const_class_method]`` instead of ``[class_method]``. This is common for
methods like ``foreach`` or ``operator []`` that should work on both mutable and
immutable instances:

.. code-block:: das

    require daslib/class_boost

    struct Container {
        data : array<int>

        [explicit_const_class_method]
        def static foreach(blk : block<(x : int) : void>) {
            for (x in data) {
                blk |> invoke(x)
            }
        }

        [explicit_const_class_method]
        def static const foreach(blk : block<(x : int) : void>) {
            for (x in data) {
                blk |> invoke(x)
            }
        }
    }

Complete ``[class_method]`` example
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    require daslib/class_boost

    struct Counter {
        value : int = 0

        [class_method]
        def static increment(amount : int = 1) {
            value += amount
        }

        [class_method]
        def static const get_value : int {
            return value
        }

        [class_method]
        def static reset {
            value = 0
        }
    }

    [export]
    def main {
        var c = Counter()
        c.increment()
        c.increment(10)
        print("value = {c.get_value()}\n")
        c.reset()
        print("after reset = {c.get_value()}\n")
    }

Expected output:

.. code-block:: text

    value = 11
    after reset = 0

-------------------------------
Stack-Allocated Classes (using)
-------------------------------

The ``using`` construct creates a stack-allocated handle instance that is automatically
finalized at the end of the scope:

.. code-block:: das

    using() $(var s : das_string) {
        s := "hello"
        print("{s}\n")
        // s is finalized here
    }

This avoids a heap allocation and manual cleanup. The handle is constructed on the
stack, and its finalizer runs when the block exits. The ``using`` function is provided by
C++ handle types (such as ``das_string``) through their factory bindings.

----------------
Complete Example
----------------

Here is a complete example demonstrating inheritance, abstract methods, custom initializers,
and virtual dispatch:

.. code-block:: das

    class Shape {
        name : string
        def abstract area : float
        def describe {
            print("{name}: area = {area()}\n")
        }
    }

    class Circle : Shape {
        radius : float
        def Circle(r : float) {
            name = "Circle"
            radius = r
        }
        def override area : float {
            return 3.14159 * radius * radius
        }
    }

    class Rectangle : Shape {
        width, height : float
        def Rectangle(w, h : float) {
            name = "Rectangle"
            width = w
            height = h
        }
        def override area : float {
            return width * height
        }
    }

    [export]
    def main {
        var shapes : array<Shape?>
        unsafe {
            shapes |> push(new Circle(5.0))
            shapes |> push(new Rectangle(3.0, 4.0))
        }
        for (s in shapes) {
            s.describe()
        }
        unsafe {
            for (s in shapes) {
                delete s
            }
        }
    }

Expected output:

.. code-block:: text

    Circle: area = 78.53975
    Rectangle: area = 12

----------------------
Implementation details
----------------------

Class initializers are generated by adding a local ``self`` variable with ``construct`` syntax.
The body of the method is prefixed via a ``with self`` expression.
The final expression is a ``return <- self``:

.. code-block:: das

    def Foo ( X:int const; Y:int const ) : Foo {
        var self:Foo <- Foo(uninitialized)
        with ( self ) {
            Foo`Foo(self,X,Y)
        }
        return <- self
    }

Class methods and finalizers are generated by providing the extra argument ``self``.
The body of the method is prefixed with a ``with self`` expression:

.. code-block:: das

    def Foo3D`set ( var self:Foo3D; X:int const; Y:int const ) {
        with ( self ) {
            Foo`set(self,X,Y)
            z = 0
        }
    }

Calling virtual methods is implemented via invoke:

.. code-block:: das

    invoke(f3d.set,cast<Foo> f3d,1,2)

Every base class gets an ``__rtti`` pointer, and a ``__finalize`` function pointer.
Additionally, a function pointer is added for each member function:

.. code-block:: das

    class Foo {
        __rtti : void? = typeinfo rtti_classinfo(type<Foo>)
        __finalize : function<(self:Foo):void> = @@_::Foo'__finalize
        x : int = 0
        y : int = 0
        set : function<(self:Foo;X:int const;Y:int const):void> = @@_::Foo`set
    }

``__rtti`` contains rtti::TypeInfo for the specific class instance.
There is helper function in the rtti module to access class_info safely:

.. code-block:: das

    def class_info ( cl ) : StructInfo const?

The ``finalize`` pointer is invoked when the finalizer is called for the class pointer.
That way, when delete is called on the base class pointer, the correct version of the derived finalizer is called.

.. seealso::

    :ref:`Structs <structs>` for the pure-data struct type without member functions,
    :ref:`Move, copy, and clone <clone_to_move>` for class copy and move rules (unsafe),
    :ref:`Annotations <annotations>` for class annotations,
    :ref:`Pattern matching <pattern-matching>` for ``is``/``as``/``?as`` with class hierarchies.


