The export_constructor module simplifies creation of Daslang structure and classes from C++ side.

In the following example::

    [export_constructor]
    class Foo {}

Function make`Foo is generated with an export flag; it returns new Foo() object.

All functions and symbols are in "export_constructor" module, use require to get access to it. ::

    require daslib/export_constructor


