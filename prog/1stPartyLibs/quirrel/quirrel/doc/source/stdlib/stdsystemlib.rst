.. _stdlib_stdsystemlib:

==================
The System library
==================

The system library exposes operating system facilities like environment variables,
process execution and file operations

--------------
Squirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++


.. sq:function:: getenv(varaname)

    Returns a string containing the value of the environment variable `varname`

.. sq:function:: remove(path)

    deletes the file specified by `path`

.. sq:function:: rename(oldname, newname)

    renames the file or directory specified by `oldname` to the name given by `newname`

.. sq:function:: system(cmd)

    executes the string `cmd` through the os command interpreter.

--------------
C API
--------------

.. _sqstd_register_systemlib:

.. c:function:: SQRESULT sqstd_register_systemlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the system library in the given VM.
