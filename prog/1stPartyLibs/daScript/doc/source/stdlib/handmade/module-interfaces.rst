The INTERFACES module implements interface-based polymorphism for daslang.
It provides the ``[interface]`` annotation for defining abstract interfaces
with virtual method tables, supporting multiple implementations and dynamic
dispatch without class inheritance.

**Features:**

- Interface inheritance (``class IChild : IParent``)
- Default method implementations (non-abstract methods)
- Compile-time completeness checking (error 30111 on missing methods)
- ``is``/``as``/``?as`` operators via the ``InterfaceAsIs`` variant macro
- Const-only interfaces — when all methods are ``def const``, ``as``/``?as``/``is`` work on const pointers

All functions and symbols are in "interfaces" module, use require to get access to it.

.. code-block:: das

    require daslib/interfaces

Example:

.. code-block:: das

    require daslib/interfaces

    [interface]
    class IGreeter {
        def abstract greet(name : string) : string
    }

    [implements(IGreeter)]
    class MyGreeter {
        def IGreeter`greet(name : string) : string {
            return "Hello, {name}!"
        }
    }

    [export]
    def main() {
        var obj = new MyGreeter()
        var greeter = obj as IGreeter
        print("{greeter->greet("world")}\n")
    }
    // output: Hello, world!

See also: :ref:`Interfaces tutorial <tutorial_interfaces>` for a
complete walkthrough.
