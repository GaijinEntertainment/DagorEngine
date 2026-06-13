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

.. _sqstd_formaterrortracestring:

.. c:function:: SQRESULT sqstd_formaterrortracestring(HSQUIRRELVM v, HSQOBJECT trace)

    :param HSQUIRRELVM v: the target VM
    :param HSQOBJECT trace: the captured async-fault trace array

    Pushes ``trace`` as a string, prefixed by an ``ERROR TRACE`` header. Returns
    ``SQ_ERROR`` and pushes nothing when ``trace`` is not a non-empty trace array.

.. _sqstd_formaterrorcontextstring:

.. c:function:: SQRESULT sqstd_formaterrorcontextstring(HSQUIRRELVM v, HSQOBJECT trace)

    :param HSQUIRRELVM v: the target VM
    :param HSQOBJECT trace: the captured async-fault trace array, or null if none

    Pushes the stack context for an error report as a string: the live call stack
    (:ref:`sqstd_formatcallstackstring <sqstd_formatcallstackstring>`) when it has
    frames (a synchronous fault), otherwise ``trace``
    (:ref:`sqstd_formaterrortracestring <sqstd_formaterrortracestring>`, for a settled
    asynchronous fault). Returns ``SQ_ERROR`` and pushes nothing when neither is
    available (``trace`` is null/not a trace array and there is no live call stack).
    Intended for embedder error handlers.
