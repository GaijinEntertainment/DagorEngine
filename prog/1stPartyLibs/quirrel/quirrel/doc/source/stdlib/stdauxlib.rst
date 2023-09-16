.. _stdlib_stdauxlib:

===============
The Aux library
===============

The aux library implements default handlers for compiler and runtime errors and a stack dumping.

+++++++++++
C API
+++++++++++

.. _sqstd_seterrorhandlers:

.. c:function:: void sqstd_seterrorhandlers(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

    initialize compiler and runtime error handlers, the handlers
    use the print function set through(:ref:`sq_setprintfunc <sq_setprintfunc>`) to output
    the error.

.. _sqstd_printcallstack:

.. c:function:: void sqstd_printcallstack(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

    prints the call stack and stack contents. the function
    uses the error printing function set through(:ref:`sq_setprintfunc <sq_setprintfunc>`) to output
    the stack dump.

.. _sqstd_formatcallstackstring:

.. c:function:: void sqstd_formatcallstackstring(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM

    Prints the call stack and stack contents to the string and pushes resulting string
    to the stack. Produces the same output as (:ref:`sqstd_printcallstack <sqstd_printcallstack>`)
    but without using print function.
