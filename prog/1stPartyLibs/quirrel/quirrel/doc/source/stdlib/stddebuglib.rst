.. _stdlib_stddebuglib:

========================
The Debug library
========================

the library implements some basic debug routines.

--------------
Squirrel API
--------------


.. sq:function:: getbuildinfo(x)

returns table containing information on VM build parameters.

  * **version** - string values describing the version of VM and compiler.
  * **charsize** - size in bytes of the internal VM representation for characters(1 for ASCII builds 2 for UNICODE builds).
  * **intsize** - size in bytes of the internal VM representation for integers(4 for 32bits builds 8 for 64bits builds).
  * **floatsize** - size in bytes of the internal VM representation for floats(4 for single precision builds 8 for double precision builds).


.. sq:function:: seterrorhandler(func)

sets the runtime error handler


.. sq:function:: setdebughook(hook_func)

sets the debug hook

hook_func should have follow signature: ::

   hook_func(hook_type:integer, source_file:string, line_num:integer, func_name:string)

hook_type can be 'l' - line, 'r' - return, 'c' - call or 'x' for VM shutdown

call of debughook for each line performed only when debuginfo is enabled


.. sq:function:: getstackinfos(level)

returns the stack informations of a given call stack level. returns a table formatted as follow: ::

    {
        func="DoStuff", //function name
        src="test.nut", //source file
        line=10,        //line number
        locals = {      //a table containing the local variables
            a=10,
            testy="I'm a string"
        }
    }

level = 0 is getstackinfos() itself! level = 1 is the current function, level = 2 is the caller of the current function, and so on.
If the stack level doesn't exist the function returns null.


.. sq:function:: getlocals([level=1], [include_internal=false])

Returns a table containing the local variables of a given call stack level.
(level = 1 is the current function, level = 2 is the caller of the current function, and so on).
If the stack level doesn't exist the function returns null.
If include_internal is true, the table will also contain the internal variables (``foreach`` iterators, ``this`` and ``vargv``).
Very similar to ``getstackinfos()``, added for convenience.


.. sq:function:: collectgarbage()

    Runs the garbage collector and returns the number of reference cycles found (and deleted). This function only works on garbage collector builds.


.. sq:function:: resurrectunreachable()

Runs the garbage collector and returns an array containing all unreachable object found. If no unreachable object is found, null is returned instead. This function is meant to help debugging reference cycles. This function only works on garbage collector builds.

