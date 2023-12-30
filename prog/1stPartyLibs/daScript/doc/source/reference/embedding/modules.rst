.. _embedding_modules:

========================
Modules and C++ bindings
========================

---------------
Builtin modules
---------------

Builtin modules are the way to expose C++ functionality to Daslang.

Let's look at the ``FIO`` module as an example.
To create a builtin module, an application needs to do the following:

Make a C++ file where the module resides. Additionally, make a header for AOT to include.

Derive from the Module class and provide a custom module name to the constructor::

    class Module_FIO : public Module {
    public:
        Module_FIO() : Module("fio") {              // this module name is ``fio``
            DAS_PROFILE_SECTION("Module_FIO");      // the profile section is there to profile module initialization time
            ModuleLibrary lib;                      // module needs library to register types and functions
            lib.addModule(this);                    // add current module to the library
            lib.addBuiltInModule();                 // make builtin functions visible to the library

Specify the AOT type and provide a prefix with C++ includes (see :ref:`AOT <aot>`)::

    virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
        tw << "#include \"Daslang/simulate/aot_builtin_fio.h\"\n";
        return ModuleAotType::cpp;
    }

Register the module at the bottom of the C++ file using the ``REGISTER_MODULE`` or ``REGISTER_MODULE_IN_NAMESPACE`` macro::

    REGISTER_MODULE_IN_NAMESPACE(Module_FIO,das);

Use the ``NEED_MODULE`` macro during application initialization before the Daslang compiler is invoked::

    NEED_MODULE(Module_FIO);

Its possible to have additional Daslang files that accompany the builtin module,
and have them compiled at initialization time via the ``compileBuiltinModule`` function::

    Module_FIO() : Module("fio") {
        ...
        // add builtin module
        compileBuiltinModule("fio.das",fio_das, sizeof(fio_das));

What happens here is that fio.das is embedded into the executable (via the XXD utility) as a string constant.

Once everything is registered in the module class constructor,
it's a good idea to verify that the module is ready for AOT via the ``verifyAotReady`` function.
It's also a good idea to verify that the builtin names are following the correct naming conventions
and do not collide with keywords via the ``verifyBuiltinNames`` function::

    Module_FIO() : Module("fio") {
        ...
        // lets verify all names
        uint32_t verifyFlags = uint32_t(VerifyBuiltinFlags::verifyAll);
        verifyFlags &= ~VerifyBuiltinFlags::verifyHandleTypes;  // we skip annotatins due to FILE and FStat
        verifyBuiltinNames(verifyFlags);
        // and now its aot ready
        verifyAotReady();
    }

-------------
ModuleAotType
-------------

Modules can specify 3 different AOT options.

``ModuleAotType::no_aot`` means that no ahead of time compilation will occur for the module, as well as any other modules which require it.

``ModuleAotType::hybrid`` means that no ahead of time compilation will occur for the module itself.
Other modules which require this one will will have AOT, but not without performance penalties.

``ModuleAotType::cpp`` means that full blown AOT will occur.
It also means that the module is required to fill in ``cppName`` for every function, field, or property.
The best way to verify it is to call ``verifyAotReady`` at the end of the module constructor.

Additionally, modules need to write out a full list of required C++ includes::

    virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
        tw << "#include \"Daslang/simulate/aot_builtin_fio.h\"\n"; // like this
        return ModuleAotType::cpp;
    }

------------------------
Builtin module constants
------------------------

Constants can be exposed via the ``addConstant`` function::

    addConstant(*this,"PI",(float)M_PI);

The constant's type is automatically inferred, assuming type ``cast`` infrastructure is in place (see :ref:`cast <cast>`).

---------------------------
Builtin module enumerations
---------------------------

Enumerations can be exposed via `the `addEnumeration`` function::

    addEnumeration(make_smart<EnumerationGooEnum>());
    addEnumeration(make_smart<EnumerationGooEnum98>());

For this to work, the enumeration adapter has to be defined via the ``DAS_BASE_BIND_ENUM`` or ``DAS_BASE_BIND_ENUM_98`` C++ preprocessor macros::

    namespace Goo {
        enum class GooEnum {
            regular
        ,   hazardous
        };

        enum GooEnum98 {
            soft
        ,   hard
        };
    }

    DAS_BASE_BIND_ENUM(Goo::GooEnum, GooEnum, regular, hazardous)
    DAS_BASE_BIND_ENUM_98(Goo::GooEnum98, GooEnum98, soft, hard)

-------------------------
Builtin module data types
-------------------------

Custom data types and type annotations can be exposed via the ``addAnnotation`` or ``addStructure`` functions::

    addAnnotation(make_smart<FileAnnotation>(lib));

See :ref:`handles <handles>` for more details.

-------------------------
Builtin module macros
-------------------------

Custom macros of different types can be added via ``addAnnotation``, ``addTypeInfoMacro``, ``addReaderMacro``, ``addCallMacro``, and such.
It is strongly preferred, however, to implement macros in Daslang.

See :ref:`macros <macros>` for more details.

------------------------
Builtin module functions
------------------------

Functions can be exposed to the builtin module via the ``addExtern`` and ``addInterop`` routines.

~~~~~~~~~
addExtern
~~~~~~~~~

``addExtern`` exposes standard C++ functions which are not specifically designed for Daslang interop::

    addExtern<DAS_BIND_FUN(builtin_fprint)>(*this, lib, "fprint", SideEffects::modifyExternal, "builtin_fprint");

Here, the builtin_fprint function is exposed to Daslang and given the name `fprint`.
The AOT name for the function is explicitly specified to indicate that the function is AOT ready.

The side-effects of the function need to be explicitly specified (see :ref:`Side-effects <modules_function_sideeffects>`).
It's always safe, but inefficient, to specify ``SideEffects::worstDefault``.

Let's look at the exposed function in detail::

    void builtin_fprint ( const FILE * f, const char * text, Context * context, LineInfoArg * at ) {
        if ( !f ) context->throw_error_at(at, "can't fprint NULL");
        if ( text ) fputs(text,(FILE *)f);
    }

C++ code can explicitly request to be provided with a Daslang context, by adding the `Context` type argument.
Making it last argument of the function makes context substitution transparent for Daslang code,
i.e. it can simply call::

    fprint(f, "boo")    // current context with be provided transparently

Daslang strings are very similar to C++ ``char *``, however null also indicates empty string.
That's the reason the `fputs` only occurs if text is not null in the example above.

Let's look at another integration example from the builtin `math` module::

    addExtern<DAS_BIND_FUN(float4x4_translation), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "translation",
            SideEffects::none, "float4x4_translation")->arg("xyz");

Here, the float4x4_translation function returns a ref type by value, i.e. `float4x4`.
This needs to be indicated explicitly by specifying a templated SimNode argument for the ``addExtern`` function,
which is ``SimNode_ExtFuncCallAndCopyOrMove``.

Some functions need to return a ref type by reference::

    addExtern<DAS_BIND_FUN(fooPtr2Ref),SimNode_ExtFuncCallRef>(*this, lib, "fooPtr2Ref",
        SideEffects::none, "fooPtr2Ref");

This is indicated with the ``SimNode_ExtFuncCallRef`` argument.

~~~~~~~~~~
addInterop
~~~~~~~~~~

For some functions it may be necessary to access type information as well as non-marshalled data.
Interop functions are designed specifically for that purpose.

Interop functions are of the following pattern::

    vec4f your_function_name_here ( Context & context, SimNode_CallBase * call, vec4f * args )

They receive a context, calling node, and arguments.
They are expected to marshal and return results, or v_zero().

``addInterop`` exposes C++ functions, which are specifically designed around Daslang::

    addInterop<
        builtin_read,               // function to register
        int,                        // function return type
        const FILE*,vec4f,int32_t   // function arguments in order
    >(*this, lib, "_builtin_read",SideEffects::modifyExternal, "builtin_read");

The interop function registration template expects a function name as its first template argument,
function return value as its second, with the rest of the arguments following.

When a function's argument type needs to remain unspecified, an argument type of ``vec4f`` is used.

Let's look at the exposed function in detail::

    vec4f builtin_read ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        DAS_ASSERT ( call->types[1]->isRef() || call->types[1]->isRefType() || call->types[1]->type==Type::tString);
        auto fp = cast<FILE *>::to(args[0]);
        if ( !fp ) context.throw_error_at(call->debugInfo, "can't read NULL");
        auto buf = cast<void *>::to(args[1]);
        auto len = cast<int32_t>::to(args[2]);
        int32_t res = (int32_t) fread(buf,1,len,fp);
        return cast<int32_t>::from(res);
    }

Argument types can be accessed via the call->types array.
Argument values and return value are marshalled via ``cast`` infrastructure (see :ref:`cast <cast>`).

.. _modules_function_sideeffects:

---------------------
Function side-effects
---------------------

The Daslang compiler is very much an optimizin compiler and pays a lot of attention to functions' side-effects.

On the C++ side, ``enum class SideEffects`` contains possible side effect combinations.

``none`` indicates that a function is pure, i.e it has no side-effects whatsoever.
A good example would be purely computational functions like ``cos`` or ``strlen``.
Daslang may choose to fold those functions at compilation time
as well as completely remove them in cases where the result is not used.

Trying to register void functions with no arguments and no side-effects causes the module initialization to fail.

``unsafe`` indicates that a function has unsafe side-effects, which can cause a panic or crash.

``userScenario`` indicates that some other uncategorized side-effects are in works.
Daslang does not optimize or fold those functions.

``modifyExternal`` indicates that the function modifies state, external to Daslang;
typically it's some sort of C++ state.

``accessExternal`` indicates that the function reads state, external to Daslang.

``modifyArgument`` means that the function modifies one of its input parameters.
Daslang will look into non-constant ref arguments and will assume that they may be modified during the function call.

Trying to register functions without mutable ref arguments and ``modifyArgument`` side effects causes module initialization to fail.

``accessGlobal`` indicates that that function accesses global state, i.e. global Daslang variables or constants.

``invoke`` indicates that the function may invoke another functions, lambdas, or blocks.

.. _modules_file_access:

-----------
File access
-----------

Daslang provides machinery to specify custom file access and module name resolution.

Default file access is implemented with the ``FsFileAccess`` class.

File access needs to implement the following file and name resolution routines::

    virtual das::FileInfo * getNewFileInfo(const das::string & fileName) override;
    virtual ModuleInfo getModuleInfo ( const string & req, const string & from ) const override;

``getNewFileInfo`` provides a file name to file data machinery. It returns null if the file is not found.

``getModuleInfo`` provides a module name to file name resolution machinery.
Given require string `req` and the module it was called `from`, it needs to fully resolve the module::

    struct ModuleInfo {
        string  moduleName;     // name of the module (by default tail of req)
        string  fileName;       // file name, where the module is to be found
        string  importName;     // import name, i.e. module namespace (by default same as module name)
    };

It is better to implement module resolution in Daslang itself, via a project.

.. _modules_project:

-------
Project
-------

Projects need to export a ``module_get`` function, which essentially implements the default C++ ``getModuleInfo`` routine::

    require strings
    require daslib/strings_boost

    typedef
        module_info = tuple<string;string;string> const // mirror of C++ ModuleInfo

    [export]
    def module_get(req,from:string) : module_info
        let rs <- split_by_chars(req,"./")                  // split request
        var fr <- split_by_chars(from,"/")
        let mod_name = rs[length(rs)-1]
        if length(fr)==0                                    // relative to local
            return [[auto mod_name, req + ".das", ""]]
        elif length(fr)==1 && fr[0]=="daslib"               // process `daslib` prefix
            return [[auto mod_name, "{get_das_root()}/daslib/{req}.das", ""]]
        else
            pop(fr)
            for se in rs
                push(fr,se)
            let path_name = join(fr,"/") + ".das"           // treat as local path
            return [[auto mod_name, path_name, ""]]

The implementation above splits the require string and looks for recognized prefixes.
If a module is requested from another module, parent module prefixes are used.
If the root `daslib` prefix is recognized, modules are looked for from the ``get_das_root`` path.
Otherwise, the request is treated as local path.



