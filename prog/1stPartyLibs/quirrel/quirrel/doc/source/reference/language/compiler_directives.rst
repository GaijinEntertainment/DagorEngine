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

   function foo() {
     #relaxed-bool
     if (123)
       print("bar")
   }


Prefix directive with 'default:' to apply it to entire VM, otherwise directive applies to clojure in where it is declared.


=============================
List of Complier directives
=============================


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


----------------------------------------------
`switch` statement
----------------------------------------------

  ::

    #allow-switch-statement

Allow 'switch' statement in syntax (deprecated)

  ::

    #forbid-switch-statement

Exclude ``switch`` statement and ``switch`` / ``case`` / ``default`` keywords from syntax

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


