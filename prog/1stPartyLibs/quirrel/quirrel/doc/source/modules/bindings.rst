Quirrel bindings
-----------------------------------

Guide in bindings
=================

Not full guide but some brief introduction.
To get deeper into bindings you need to study samples or read the code of SqRat

Sqrat is a C++ binding library designed for the Quirrel scripting language.
Sqrat simplifies the process of exposing C++ functions and classes to Quirrel scripts.
It automates type conversion between C++ and Quirrel, making it easier to register functions, methods,
and classes without needing to manually handle the Quirrel stack in every case.

Example
^^^^^^^^^^^^^^^^^^

Basic Includes
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

  #include <sqrat.h>
  #include <sqModules.h>


Create a module
^^^^^^^^^^^^^^^^^^^^^

SqModules allows to export Quirrel objects (usually tables and classes) as script modules.
Example of module creation:

::

  Sqrat::Table exports(vm); // Create a table to hold the exported functions and classes
  exports
    .Func("function_name", function) // Export a C++ function with automatic handling of arguments and return values
    .SquirrelFunc("function_name", low_level_function) // Export a C++ function that works with the Squirrel stack
    .SetValue("SOME_ID", value) // Exports is a table, so it can hold any Quirrel value
  ;
  module_manager->addNativeModule("module_name", exports); // Expose the table as a module to the Quirrel scripts


Simple bind of cpp function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use Sqrat::Table::Func() or Sqrat::Class::Func() to register a C++ function for use in a Quirrel script.
Sqrat automatically handles argument and return value conversions between Squirrel and C++ types.

C++:

::

  Sqrat::Table exports(vm);
  exports.Func("pow", &std::pow);  // Register pow function in the script
  module_manager->addNativeModule("math_demo", exports);


Usage in Quirrel:
::

  from "math_demo" import pow
  // or
  let { pow } = require("math_demo")

  local result = pow(2, 3)  // Calls the C++ std::pow(2, 3)
  print(result)  // Output: 8



Bind function that work with Squirrel stack
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For more control, you can manually handle the Squirrel stack by using SquirrelFunc.
This allows you to specify custom logic for processing function arguments and returning values.
For instance, you may need this for variadic functions, arguments of varying types, or functions with complex logic that can't be automatically bound.

::

  SQInteger sum_all_args(HSQUIRRELVM v) {
      SQInteger n = sq_gettop(v); // Number of arguments
      SQFloat sum = 0;
      for (SQInteger i = 2; i <= n; ++i) { // Sum all arguments, starting from 2 (#1 is `this`)
          SQFloat val;
          if (SQ_SUCCEEDED(sq_getfloat(v, i, &val))) // For demo purposes, we only handle numbers and ignore other types
              sum += val;
      }
      sq_pushfloat(v, sum); // Push the result
      return 1; // Number of return values (0 for void, 1 for return value, SQ_ERROR for error)
  }

  exports.SquirrelFunc("sum_all_args", sum_all_args, -2);  // -2: variable arguments, at least 2 (`this` and a number

In script:
::

  local result = sum_all_args(1, 2, 3, 4)  // Calls the C++ function
  print(result)  // Output: 10


Full signature of SquirrelFunc:

.. cpp:function:: TableBase& Table::SquirrelFunc(const SQChar* name, SQFUNCTION func, SQInteger nparamscheck, const SQChar *typemask=nullptr, const SQChar *docstring=nullptr, SQInteger nfreevars=0, const Object *freevars=nullptr)

  :param name: should be string, func should be function that works with Quirrel
  :param nparamscheck: number of arguments of function. If negative - function can have at least this number of arguments but can accept more.
  :param typemask: optional typemask (see sq_setparamscheck in :ref:`API <api_ref_object_creation_and_handling>`)
  :param docstring: optional docstring, nfreevars and freevars - free variables of function.
  :param nfreevars: number of free variables
  :param freevars: free variables to capture

Bind classes, constants and values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sqrat allows to register C++ classes with member variables and methods that can be accessed from Quirrel scripts.

Toy example:
::

  class Rect {
    public:
      float width, height;
      Rect(float w, float h) : width(w), height(h) {}
      float area() const {
          return width * height;
      }
      float perimeter() const {
          return 2 * (width + height);
      }
  };

  Sqrat::Class<Rect> rectClass(table.GetVM(), "Rect");
  rectClass
    .Ctor()
    .Var("width", &Rect::width)
    .Var("height", &Rect::height)
    .Func("area", &Rect::area)
    .Prop("perimeter", &Rect::perimeter)
  ;

  exports.Bind("Rect", rectClass);  // Bind the class to the table
  module_manager->addNativeModule("geometry", exports);

In script:
::

  from "geometry" import Rect
  local r = Rect(1, 3)
  r.width = 2
  print(r.area())  // Output: 6
  print(r.perimeter)  // Output: 10

SquirrelCtor() may be used for a constructor with a flexible behavior.
It has to implement an actual native instance creation.

Example:
::

  static SQInteger rect_ctor(HSQUIRRELVM v) {
      SQInteger n = sq_gettop(v);
      if (n == 2) { // copy constructor
        if (!Sqrat::check_signature<Rect *>(vm, 2))
          return sq_throwerror(vm, "Invalid type passed to copy ctor");
      }
      else if (n != 1 && n != 3)
        return sqstd_throwerrorf(vm, "Invalid arguments count %d", n);

      Rect *instance = new Rect(0, 0);
      if (n == 1) // no arguments
        ; // already initialized with default 0, 0
      else if (n == 3) { // instance and 2 numbers
        SQFloat w, h;
        sq_getfloat(v, 2, &w); // Should always succeed, because types are specified in type mask (see SquirrelCtor)
        sq_getfloat(v, 3, &h);
        instance->width = w;
        instance->height = h;
      }
      else if (n == 2) { // copy constructor (self instance and another instance)
        Rect *other = Sqrat::Var<Rect *>(vm, 2).value;
        instance->width = other->width;
        instance->height = other->height;
      }
      Sqrat::ClassType<Rect>::SetManagedInstance(vm, 1, instance); // Link with script instance
      return 1;
  }

  Sqrat::Class<Rect> rectClass(table.GetVM(), "Rect");
  rectClass
    .SquirrelCtor(rect_ctor, 0, "x y|n n") // 0 - no argument count check, "x y|n n" - type mask (instance('this') and instance or 2 numbers)
    .Var("width", &Rect::width)
    .Var("height", &Rect::height)
    .Func("area", &Rect::area)
    .Prop("perimeter", &Rect::perimeter)
  ;

In script:
::

  from "geometry" import Rect
  local r1 = Rect(1, 3)
  local r2 = Rect(r1)
  local r3 = Rect()


Consttable - not documented

Class property (setter) - not documented

Static functions - not documented
