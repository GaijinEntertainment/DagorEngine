.. _destructuring_assignment:


=========================
Destructuring assignment
=========================

.. index::
    single: Destructuring assignment

The destructuring assignment syntax is an expression that makes it possible to unpack
values from arrays, or properties from tables/classes/instances, into distinct variables.
This is similar to destructuring assignment in JavaScript (ECMAScript 2015)

Example
::

   let arr = [123, 567]
   let [a, b] = arr
   print(a) // => 123
   print(b) // => 567

   function foo() {
     return {x = 555, y=777, z=999, w=111}
   }
   let {x, y=1, q=3} = foo()
   print(x) // => 555
   print(y) // => 777
   print(q) // => 3

If a default value is provided it will be used if the slot does not exist in the source object.
If no default value is given and a slot with this name/index is missing, a runtime error will be raised.
Comma separators are optional.


Destructuring in function parameters
====================================

Destructuring patterns can also be used directly in function parameter lists.
This allows function arguments to be unpacked at the call site, with full support
for default values and type annotations.

Example
::

   function test1({x, y}, [a, b]) {
     println(x, y, a, b)
   }

   function test2({x: int = 5}, [a: string, b: float = 2.2]) {
     println(x, a, b)
   }

Default values in destructured parameters are applied when the corresponding
slot is missing in the passed argument. Type annotations may be used on individual
destructured elements in the same way as for regular parameters.
