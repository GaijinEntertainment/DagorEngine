.. _destructuring_assignment:


=========================
Destructuring assignment
=========================

.. index::
    single: Destructuring assignment

The destructuring assignment syntax is a an expression that makes it possible to unpack
values from arrays, or properties from tables/classes/instances, into distinct variables.
This is similar to destructuring assignment in JavaScript (ECMAScript 2015)

Example
::

   let arr = [123, 567]
   let [a, b] = arr
   print(a) // => 123
   print(b) // => 567

   let function foo() {
     return {x = 555, y=777, z=999, w=111}
   }
   let {x, y=1, q=3} = foo()
   print(x) // => 555
   print(y) // => 777
   print(q) // => 3

If default value is provided it will be used if slot does not exist in source object.
If no default value is given and slot with this name/index is missing, a runtime erro will be raised.
Comma separators are optional.
