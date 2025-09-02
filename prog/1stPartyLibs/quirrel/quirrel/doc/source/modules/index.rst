.. _index:

************
Modules
************

Project source code is available at https://github.com/GaijinEntertainment/quirrel

Quirrel provides modules functionality via separate library.
Modules are useful to encapsulate code (only explicitly exported data/methods become
available outside), to make the code reusable and to prevent contaminating global namespace
(root table).
The provided reference implementation also allows reloading of script code while keeping
data intact.

==================
Script API
==================

.. index::
    single: Script API

Module code is executed with a special table as context ('this').
It has the following functions bound:

.. sq:function:: require(filename)

If given module was not loaded yet, reads the file, executes it and returns module exports.
Module exports are data returned by module body code.
If module was loaded before, just returns its exports.
If file was not found, throws an error.

.. sq:function:: require_optional(filename)

The same as require(), but for missing file it returns null instead of throwing an error.

.. sq:function:: persist(key, initializer)

Used to keep data between sequential execution of the same file for hot reloading.
For the first run of the module calls provided initializer function and returns the result.
Also it stores a reference to the result in a special table that is later reused when
the file with the same name is executed.
The 'key' parameter is the unique-per-module identifier of the variable.

Example: ::
  
  let counter = persist("counter", @() {value=0})


.. sq:function:: keepref(var)

An utility function that keeps a reference to the given object.
Might be useful to keep an object from deleting itself if it is not exported outside the module.


Also module 'this' table contains field __name__ which is set to file name for module loaded
by script require() function or parameter __name__ passed to C++ requireModule() function (defaults
to "__main__" for entry point script).


.. quirrel.native_modules

Module that allow to list all registered native modules. 

Example: ::

  require("quirrel.native_modules").each(println)


Will print all currently registered native modules.

==================
Native API
==================

.. index::
    single: Native API

Module functionality is implemented in SqModules C++ class with the following public methods:

.. cpp:function:: SqModules::SqModules(HSQUIRRELVM vm)

Constructs a module manager instance for the given Quirrel VM.

.. cpp:function:: HSQUIRRELVM SqModules::getVM()

Returns module manager VM.

.. cpp:function:: bool SqModules::requireModule(const char *fn, bool must_exist, const char *__name__, SqObjPtr &exports, std::string &out_err_msg)

    :param const char \*fn: name of the file to load
    :param bool must_exist: if set to false treat missing file as success (return true)
    :param const char \*__name__: value of module's __name__ field (set nullptr to use actual file name)
    :param SqObjPtr &exports: object to store export to
    :param std\:\:string &out_err_msg: receives error message if call failed
    :returns: true if succeeded
    :remarks: Actually this is a function bound to script as require()

Loads and executes script module or just returns its exports if it was already run.

.. cpp:function:: bool SqModules::reloadModule(const char *fn, bool must_exist, const char *__name__, SqObjPtr &exports, std::string &out_err_msg)

Basically the same as requireModule, but it unloads all previously loaded modules and newly executes
all modules pulled by require() calls.
This can also be used for initial module execution - so that the first call to
reloadModule(entry_point_fn) will load all modules and initialize persistent data and subsequent calls
to reloadModule(entry_point_fn) will reload the module and all it depends on while reusing kept persistent data.

.. cpp:function:: bool SqModules::addNativeModule(const char *module_name, const SqObjPtr &exports)

Registers a Quirrel object 'exports' under the provided 'module_name', so that it can be require()-d in script.

.. cpp:function:: void SqModules::registerMathLib()

Registers standard "math" native library

.. cpp:function:: void SqModules::registerStringLib()

Registers standard "string" native library

.. cpp:function:: void SqModules::registerIoStreamLib()

Registers standard "iostream" native library module (blob and stream classes)

.. cpp:function:: void SqModules::registerDateTimeLib()

Registers standard "datetime" native library

.. cpp:function:: void SqModules::registerDebugLib()

Registers "debug" native library

.. cpp:function:: void SqModules::registerSystemLib()

Registers standard 'system' library as "system" native module

.. cpp:function:: void SqModules::registerIoLib()

Registers standard 'io' library as "io" native module
