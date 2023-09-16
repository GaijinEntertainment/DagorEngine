Quirrel bindings
-----------------------------------

Guide in bindings
=================

Not full guide but some brief introduction.
To get deeper into bindings you need to study samples or read the code of SqRat

Example
^^^^^^^^^^^^^^^^^^

Basic Includes
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

  #include <sqrat.h>
  #include <sqModules.h>


Create a module
^^^^^^^^^^^^^^^^^^^^^


Simple bind of cpp function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. cpp:function:: .Func(const char *fn, bool must_exist, const char *__name__, SqObjPtr &exports, std::string &out_err_msg)
  .Func(name:string, cpp_function:function of basic quirrel types (int, float, bool, char*, void))


    :param const char \*fn: name of the file to load
    :param bool must_exist: if set to false treat missing file as success (return true)
    :param const char \*__name__: value of module's __name__ field (set nullptr to use actual file name)
    :param SqObjPtr &exports: object to store export to
    :param name

function should return type that can be converted to quirrel basic types


Bind function that work with Squirrel stack
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

  TableBase& SquirrelFunc(const SQChar* name, SQFUNCTION func, SQInteger nparamscheck, const SQChar *typemask=nullptr,
                            const SQChar *docstring=nullptr, SQInteger nfreevars=0, const Object *freevars=nullptr)

name should be string, func should be function that works with Quirrel

nparamscheck - number of arguments of function. If negative - function can have at least this number
of arguments but can accept more.

typemask - optional typemask (see sq_setparamscheck in :ref:`API <api_ref_object_creation_and_handling>`)

docstring - optional docstring, nfreevars and freevars - free variables of function.

Bind classes, constants and values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

build const value - not documented

Consttable - not documented

build class - not documented
  ctors
  function
  property
  static function
