.. _aot:

====================================================
Ahead of time compilation and C++ operation bindings
====================================================

For optimal performance and seamless integration, Daslang is capable of ahead of time compilation,
i.e. producing C++ files, which are semantically equivalent to simulated Daslang nodes.

The output C++ is designed to be to some extent human readable.

For the most part, Daslang produces AOT automatically,
but some integration effort may be required for custom types.
Plus, certain performance optimizations can be achieved with additional integration effort.

Daslang AOT integration is done on the AST expression tree level, and not on the simulation node level.

---------
das_index
---------

The ``das_index`` template is used to provide the implementation of the ``ExprAt`` and ``ExprSafeAt`` AST nodes.

Given the input type `VecT`, output result `TT`, and index type of int32_t,
``das_index`` needs to implement the following functions::

    // regular index
        static __forceinline TT & at ( VecT & value, int32_t index, Context * __context__ );
        static __forceinline const TT & at ( const VecT & value, int32_t index, Context * __context__ );
    // safe index
        static __forceinline TT * safe_at ( VecT * value, int32_t index, Context * );
        static __forceinline const TT * safe_at ( const VecT * value, int32_t index, Context * );

Note that sometimes more than one index type is possible.
In that case, implementation for each index type is required.

Note how both const and not const versions are available.
Additionally, const and non const versions of the ``das_index`` template itself may be required.

------------
das_iterator
------------

The ``das_iterator`` template is used to provide the for loop backend for the ``ExprFor`` sources.

Let's review the following example, which implements iteration over the range type::

    template <>
    struct das_iterator <const range> {
        __forceinline das_iterator(const range & r) : that(r) {}
        __forceinline bool first ( Context *, int32_t & i ) { i = that.from; return i!=that.to; }
        __forceinline bool next  ( Context *, int32_t & i ) { i++; return i!=that.to; }
        __forceinline void close ( Context *, int32_t &   ) {}
        range that;
    };

The ``das_iterator`` template needs to implement the constructor for the specified type,
and also the ``first``, ``next``, and ``close`` functions, similar to that of the Iterator.

Both the const and regular versions of the ``das_iterator`` template are to be provided::

    template <>
    struct das_iterator <range> : das_iterator<const range> {
        __forceinline das_iterator(const range & r) : das_iterator<const range>(r) {}
    };

Ref iterator return types are C++ pointers::

    template <typename TT>
    struct das_iterator<TArray<TT>> {
        __forceinline bool first(Context * __context__, TT * & i) {

Out of the box, ``das_iterator`` is implemented for all integrated types.

---------------------
AOT template function
---------------------

By default, AOT generated functions expect blocks to be passed as the C++ TBlock class (see :ref:`Blocks <blocks>`).
This creates significant performance overhead, which can be reduced by AOT template machinery.

Let's review the following example::

    void peek_das_string(const string & str, const TBlock<void,TTemporary<const char *>> & block, Context * context) {
        vec4f args[1];
        args[0] = cast<const char *>::from(str.c_str());
        context->invoke(block, args, nullptr);
    }

The overhead consists of type marshalling, as well as context block invocation.
However, the following template can be called like this, instead::

    template <typename TT>
    void peek_das_string_T(const string & str, TT && block, Context *) {
        block((char *)str.c_str());
    }

Here, the block is templated, and can be called without any marshalling whatsoever.
To achieve this, the function registration in the module needs to be modified::

    addExtern<DAS_BIND_FUN(peek_das_string)>(*this, lib, "peek",
        SideEffects::modifyExternal,"peek_das_string_T")->setAotTemplate();

-------------------------------------
AOT settings for individual functions
-------------------------------------

There are several function annotations which control how function AOT is generated.

The ``[hybrid]`` annotation indicates that a function is always called via the full Daslang interop ABI (slower),
as oppose to a direct function call via C++ language construct (faster).
Doing this removes the dependency between the two functions in the semantic hash,
which in turn allows for replacing only one of the functions with the simulated version.

The ``[no_aot]`` annotation indicates that the AOT version of the function will not be generated.
This is useful for working around AOT code-generation issues, as well as during builtin module development.

---------------------
AOT prefix and suffix
---------------------

Function or type trait expressions can have custom annotations to specify prefix and suffix text around the generated call.
This may be necessary to completely replace the call itself, provide additional type conversions, or perform other customizations.

Let's review the following example::

    struct ClassInfoMacro : TypeInfoMacro {
        ....
        virtual void aotPrefix ( TextWriter & ss, const ExpressionPtr & ) override {
            ss << "(void *)(&";
        }
        virtual void aotSuffix ( TextWriter & ss, const ExpressionPtr & ) override {
            ss << ")";
        }

Here, the class info macro converts the requested type information to `void *`.
This part of the class machinery allows the ``__rtti`` pointer of the class to remain void,
without including RTTI everywhere the class is included.

---------------------------
AOT field prefix and suffix
---------------------------

``ExprField`` is covered by the following functions in the handled type annotation (see :ref:`Handles <handles>`)::

    virtual void aotPreVisitGetField ( TextWriter &, const string & fieldName )
    virtual void aotPreVisitGetFieldPtr ( TextWriter &, const string & fieldName )
    virtual void aotVisitGetField ( TextWriter & ss, const string & fieldName )
    virtual void aotVisitGetFieldPtr ( TextWriter & ss, const string & fieldName )

By default, prefix functions do nothing, and postfix functions append `.fieldName` and `->fieldName` accordingly.

Note that ``ExprSafeField`` is not covered yet, and will be implemented for AOT at some point.
