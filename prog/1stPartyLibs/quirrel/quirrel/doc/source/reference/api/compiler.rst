.. _api_ref_compiler:

========
Compiler
========


.. _sq_compile:

.. c:function:: SQRESULT sq_compile(HSQUIRRELVM v, const char* s, SQInteger size, const char * sourcename, SQBool raiseerror, const HSQOBJECT *bindings = nullptr)

    :param HSQUIRRELVM v: the target VM
    :param const char* s: a pointer to the buffer that has to be compiled.
    :param SQInteger size: size in characters of the buffer passed in the parameter 's'.
    :param const char * sourcename: the symbolic name of the program (used only for more meaningful runtime errors)
    :param SQBool raiseerror: if this value true the compiler error handler will be called in case of an error
    :param const HSQOBJECT \*bindings: optional compile-time bindings object (default: nullptr)
    :returns: a SQRESULT. If the sq_compile fails nothing is pushed in the stack.
    :remarks: in case of an error the function will call the function set by sq_setcompilererrorhandler().

compiles a quirrel program from a memory buffer; if it succeeds, push the compiled script as function in the stack.



.. _sq_notifyallexceptions:

.. c:function:: void sq_notifyallexceptions(HSQUIRRELVM v, SQBool enable)

    :param HSQUIRRELVM v: the target VM
    :param SQBool enable: if true enables the error callback notification of handled exceptions.
    :remarks: By default the VM will invoke the error callback only if an exception is not handled (no try/catch traps are present in the call stack). If notifyallexceptions is enabled, the VM will call the error callback for any exception even if between try/catch blocks. This feature is useful for implementing debuggers.

enable/disable the error callback notification of handled exceptions.





.. _sq_setcompilererrorhandler:

.. c:function:: void sq_setcompilererrorhandler(HSQUIRRELVM v, SQCOMPILERERROR f)

    :param HSQUIRRELVM v: the target VM
    :param SQCOMPILERERROR f: A pointer to the error handler function
    :remarks: if the parameter f is NULL no function will be called when a compiler error occurs. The compiler error handler is shared between friend VMs.

sets the compiler error handler function


.. _sq_getcompilererrorhandler:

.. c:function:: SQCOMPILERERROR sq_getcompilererrorhandler(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: a pointer to the current compiler error handler function, or NULL if no function is set

gets the current compiler error handler function


.. _sq_setcompilerdiaghandler:

.. c:function:: void sq_setcompilerdiaghandler(HSQUIRRELVM v, SQ_COMPILER_DIAG_CB f)

    :param HSQUIRRELVM v: the target VM
    :param SQ_COMPILER_DIAG_CB f: A pointer to the diagnostic callback function
    :remarks: if the parameter f is NULL no function will be called when a compiler diagnostic occurs.

sets the compiler diagnostic callback function
