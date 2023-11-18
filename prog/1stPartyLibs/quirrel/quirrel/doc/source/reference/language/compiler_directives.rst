.. _compiler_directives:


=========================
Complier directives
=========================

.. index::
    single: Complier directives

Quirrel has a way to for flexibly customize language features.

This allows to smoothly change compiler, while keeping backward compatibility if needed.

Example
::

   #default:strict

   let function foo() {
     #relaxed-bool
     if (123)
       print("bar")
   }


Prefix directive with 'default:' to apply it to entire VM, otherwise directive applies to clojure in where it is declared.


=============================
List of Complier directives
=============================

----------------
Strict booleans
----------------

::  

    #strict-bool

Force booleans in conditional expressions, raise an error on non-boolean types.

Non-boolean in conditional expressions can lead to errors.
Because all programming languages with such feature behave differently, for example Python treats empty arrays and strings as false,
but JS doesn't; PHP convert logical expression to Boolean, etc.
Another reason is that due to dynamic nature of Quirrel those who read code can't know what exactly author meant with it.
::

   if (some_var) //somevar can be null, 0, 0.0 or object

or

::

   local foo = a() || b() && c //what author means? What will be foo?



----------------------------
Implicit bool expressions
----------------------------

::

    #relaxed-bool

Original Squirrel 3.1 behavior (treat boolean false, null, 0 and 0.0 as false, all other values as true)


-----------------------------------------------
Disable access to root table via ``::``
-----------------------------------------------

::

    #forbid-root-table

Forbids use of ``::`` operator for the current unit
Using root table is dangerous (as all global vairables)

-----------------------------------------------
Enable access to root table via ``::``
-----------------------------------------------

::

    #allow-root-table

Allows use of ``::`` operator for the current unit

----------------------------------------------
Disable implicit string concatenation
----------------------------------------------

::

  #no-plus-concat

Throws error on string concatenation with +.
It is slower for multple strings, use concat or join instead.
It is also not safe and can cause errors, as + is supposed to be at least Associative operation, and usually Commutative too.
But + for string concatenation is not associative, e.g.

Example:
::

   let a = 1
   let b = 2
   let c = "3"
   (a + b) + c != a + (b + c) // "33" != "123"

This actually happens especially on reduce of arrays and alike.

----------------------------------------------
Enable plus string concatenation
----------------------------------------------

::

   #allow-plus-concat

Allow using plus operator '+' to concatenate strings.

----------------------------------------------
Clone operator
----------------------------------------------

  ::
    
    #allow-clone-operator

Allow using 'clone' operator (let a = clone {})

  ::
    
    forbid-clone-operator

Forbid using 'clone' operator use .$clone instead (let a = {}.$clone())
'clone' is not a keyword in this case you call variables with it for example.

----------------------------------------------
Delete operator
----------------------------------------------

  ::
    
    #allow-delete-operator

Allow using 'delete' operator (let a = delete {foo = 2}["foo"] //2)

  ::
    
    forbid-delete-operator

Forbid using 'delete' operator use .$rawdelete instead (let a = {foo=2}.$rawdelete("foo") //2)
'delete' is not a keyword in this case and you call variables with it for example.

------------------
#strict
------------------

::

   #strict

Enable all extra checks/restrictions


------------------
#relaxed
------------------

::

   #relaxed

Disable all extra checks/restrictions


