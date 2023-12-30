.. _classes:


=================
Classes
=================

.. index::
    single: Classes

Quirrel implements a class mechanism similar to languages like Java/C++/etc...
however because of its dynamic nature it differs in several aspects.
Classes are first class objects like integer or strings and can be stored in
table slots local variables, arrays and passed as function parameters.

-----------------
Class Declaration
-----------------

.. index::
    pair: declaration; Class
    single: Class Declaration

A class object is created through the keyword ``class`` . The class object follows
the same declaration syntax of a table(see :ref:`Tables <tables>`) with the only difference
of using ``;`` as optional separator rather than ``,``.

For instance: ::

    class Foo {
        //constructor
        constructor(a) {
            testy = ["stuff",1,2,3,a]
        }
        //member function
        function PrintTesty() {
            foreach(i,val in testy) {
                print("idx = "+i+" = "+val)
            }
        }
        //property
        testy = null
    }


After its declaration, methods or properties can be added or modified by following
the same rules that apply to a table (operator ``<-``).::

    //adds a new property
    Foo.stuff <- 10

    //modifies the default value of an existing property
    Foo.testy <- "I'm a string"

After a class is instantiated is no longer possible to add new properties however is possible to add or replace methods.

^^^^^^^^^^^^^^^^
Static variables
^^^^^^^^^^^^^^^^

.. index::
    pair: static variables; Class
    single: Static variables

Quirrel's classes support static member variables. A static variable shares its value
between all instances of the class. Statics are declared by prefixing the variable declaration
with the keyword ``static``; the declaration must be in the class body.

.. note:: Statics are read-only.

::

    class Foo {
        constructor() {
            //..stuff
        }
        name = "normal variable"
        //static variable
        static classname = "The class name is foo"
    };

-----------------
Class Instances
-----------------

.. index::
    pair: instances; Class
    single: Class Instances

The class objects inherits several of the table's feature with the difference that multiple instances of the
same class can be created.
A class instance is an object that share the same structure of the table that created it but
holds is own values.
Class *instantiation* uses function notation.
A class instance is created by calling a class object. Can be useful to imagine a class like a function
that returns a class instance.::

    //creates a new instance of Foo
    let inst = Foo()

When a class instance is created its member are initialized *with the same value* specified in the
class declaration. The values are copied verbatim, *no cloning is performed* even if the value is a container or a class instances.

.. note:: FOR C# and Java programmers:

    Quirrel doesn't clone member's default values nor executes the member declaration for each instance(as C# or java).

    So consider this example: ::

        class Foo {
            myarray = [1,2,3]
            mytable = {}
        }

        let a = Foo()
        let b = Foo()

    In the snippet above both instances will refer to the same array and same table.
    To achieve what a C# or Java programmer would expect, the following approach should be taken. ::

        class Foo {
            myarray = null
            mytable = null
            constructor() {
                myarray = [1,2,3]
                mytable = {}
            }
        }

        let a = Foo()
        let b = Foo()

When a class defines a method called 'constructor', the class instantiation operation will
automatically invoke it for the newly created instance.
The constructor method can have parameters, this will impact on the number of parameters
that the *instantiation operation* will require.
Constructors, as normal functions, can have variable number of parameters (using the parameter ``...``).

::

    class Rect {
        constructor(w,h) {
            width = w
            height = h
        }
        x = 0
        y = 0
        width = null
        height = null
    }

    //Rect's constructor has 2 parameters so the class has to be 'called'
    //with 2 parameters
    let rc = Rect(100,100)

After an instance is created, its properties can be set or fetched following the
same rules that apply to tables. Methods cannot be set.

Instance members cannot be removed.

The class object that created a certain instance can be retrieved through the built-in function
``instance.getclass()`` (see :ref:`built-in functions <builtin_functions>`)

The operator ``instanceof`` tests if a class instance is an instance of a certain class.

::

    let rc = Rect(100, 100)
    if (rc instanceof Rect) {
        println("It's a rect")
    }
    else {
        println("It isn't a rect")
    }

.. note:: Since Squirrel 3.x instanceof doesn't throw an exception if the left expression is not a class, it simply fails

--------------
Inheritance
--------------

.. index::
    pair: inheritance; Class
    single: Inheritance

Quirrel's classes support single inheritance.
The syntax for a derived class is the following: ::

    class DerivedClas(BaseClass) {
        function DoSomething() {
            println("I'm doing something")
        }
    }

When a derived class is declared, Quirrel first copies all base's members in the
new class then proceeds with evaluating the rest of the declaration.

A derived class inherit all members and properties of it's base, if the derived class
overrides a base function the base implementation is shadowed.
It's possible to access a overridden method of the base class by fetching the method from it
through the 'base' keyword.

Here an example:

::

    class Foo {
        function DoSomething() {
            println("I'm the base")
        }
    };

    class SuperFoo(Foo) {
        //overridden method
        function DoSomething() {
            //calls the base method
            base.DoSomething()
            println("I'm doing something")
        }
    }


Same rule apply to the constructor. The constructor is a regular function (apart from being automatically invoked on construction).

::

    class BaseClass {
        constructor() {
            println("Base constructor")
        }
    }

    class ChildClass(BaseClass) {
        constructor() {
            base.constructor()
            println("Child constructor")
        }
    }

    let test = ChildClass()


The base class of a derived class can be retrieved through the built-in method ``getbase()``.

::

    let thebaseclass = SuperFoo.getbase()


Note that because methods do not have special protection policies when calling methods of the same
objects, a method of a base class that calls a method of the same class can end up calling a overridden method of the derived class.

A method of a base class can be explicitly invoked by a method of a derived class though the keyword ``base`` (as in base.MyMethod() ).

::

    class Foo {
        function DoSomething() {
            println("I'm the base")
        }
        function DoIt() {
            DoSomething()
        }
    };

    class SuperFoo(Foo) {
        //overridden method
        function DoSomething() {
            println("I'm the derived")
        }
        function DoIt() {
            base.DoIt()
        }
    }

    //creates a new instance of SuperFoo
    let inst = SuperFoo()

    //prints "I'm the derived"
    inst.DoIt()


An alternative way to inheret class it to use Python-style syntax. It works the same way as described above.

::

    class SuperFoo(Foo) {
        function DoSomething() {
            println("I'm doing something")
        }
    }


----------------------
Metamethods
----------------------

.. index::
    pair: metamethods; Class
    single: Class metamethods

Class instances allow the customization of certain aspects of the
their semantics through metamethods(see see :ref:`Metamethods <metamethods>`).
For C++ programmers: "metamethods behave roughly like overloaded operators".
The metamethods supported by classes are ``_add, _sub, _mul, _div, _unm, _modulo,
_set, _get, _typeof, _nexti, _cmp, _call, _delslot, _tostring``

the following example show how to create a class that implements the metamethod ``_add``.

::

    class Vector3 {
        constructor(...) {
            if(vargv.len() >= 3) {
                x = vargv[0]
                y = vargv[1]
                z = vargv[2]
            }
        }
        function _add(other) {
            return ::Vector3(x+other.x,y+other.y,z+other.z)
        }

        x = 0
        y = 0
        z = 0
    }

    let v0 = Vector3(1,2,3)
    let v1 = Vector3(11,12,13)
    let v2 = v0 + v1
    println($"{v2.x}, "{v2.y}, {v2.z}")
