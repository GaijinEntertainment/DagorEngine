.. _handles:

==========================
Exposing C++ handled types
==========================

Handled types represent the machinery designed to expose C++ types to daScript.

A handled type is created by deriving a custom type annotation from TypeAnnotation
and adding an instance of that annotation to the desired module. For example::

    template <typename VecT, int RowC>
    class MatrixAnnotation : public TypeAnnotation {
    ...

    typedef MatrixAnnotation<float4,4> float4x4_ann;

    Module_Math() : Module("math") {
        ...
        addAnnotation(make_smart<float4x4_ann>());

--------------
TypeAnnotation
--------------

``TypeAnnotation`` contains a collection of virtual methods to describe type properties,
as well as methods to implement simulation nodes for the specific functionality.

``canAot`` returns true if the type can appear in :ref:`AOT <aot>`::

    virtual bool canAot(das_set<Structure *> &) const


``canCopy``, ``canMove`` and ``canClone`` allow a type to be copied, moved, or cloned::

        virtual bool canMove() const
        virtual bool canCopy() const
        virtual bool canClone() const

``isPod`` and ``isRawPod`` specify if a type is plain old data, and plain old data without pointers, respectively::

        virtual bool isPod() const
        virtual bool isRawPod() const

``isRefType`` specifies the type ABI, i.e. if it's passed by reference or by value::

        virtual bool isRefType() const

``isLocal`` allows creation of a local variable of that type::

        virtual bool isLocal() const

``canNew``, ``canDelete`` and ``canDeletePtr`` specify if new and delete operations are allowed for the type,
as well as whether a pointer to the type can be deleted::

        virtual bool canNew() const
        virtual bool canDelete() const
        virtual bool canDeletePtr() const

``needDelete`` specifies if automatically generated finalizers are to delete this type::

        virtual bool needDelete() const

``isIndexable`` specifies if the index operation ``[]`` is allowed for the type::

        virtual bool isIndexable ( const TypeDeclPtr & ) const

``isIterable`` specifies if the type can be the source of a for loop::

        virtual bool isIterable ( ) const

``isShareable`` specifies if global variables of the type can be marked as `shared`::

        virtual bool isShareable ( ) const

``isSmart`` specifies, if a pointer to the type appears as a smart_ptr::

        virtual bool isSmart() const

``canSubstitute`` queries if LSP is allowed for the type, i.e. the type can be downcast::

        virtual bool canSubstitute ( TypeAnnotation * /* passType */ ) const

``getSmartAnnotationCloneFunction`` returns the clone function name for the ``:=`` operator substitution::

        virtual string getSmartAnnotationCloneFunction () const { return ""; }

``getSizeOf`` and ``getAlignOf`` return the size and alignment of the type, respectively::

        virtual size_t getSizeOf() const
        virtual size_t getAlignOf() const

``makeFieldType`` and ``makeSafeFieldType`` return the type of the specified field (or null if the field is not found)::

        virtual TypeDeclPtr makeFieldType ( const string & ) const
        virtual TypeDeclPtr makeSafeFieldType ( const string & ) const

``makeIndexType`` returns the type of the ``[]`` operator, given an index expression (or null if unsupported)::

        virtual TypeDeclPtr makeIndexType ( const ExpressionPtr & /*src*/, const ExpressionPtr & /*idx*/ ) const

``makeIteratorType`` returns the type of the iterable variable when serving as a for loop source (or null if unsupported)::

        virtual TypeDeclPtr makeIteratorType ( const ExpressionPtr & /*src*/ ) const

``aotPreVisitGetField``, ``aotPreVisitGetFieldPtr``, ``aotVisitGetField``, and ``aotVisitGetFieldPtr`` generate
specific AOT prefixes and suffixes for the field and pointer field dereference::

        virtual void aotPreVisitGetField ( TextWriter &, const string & )
        virtual void aotPreVisitGetFieldPtr ( TextWriter &, const string & )
        virtual void aotVisitGetField ( TextWriter & ss, const string & fieldName )
        virtual void aotVisitGetFieldPtr ( TextWriter & ss, const string & fieldName )

There are numerous ``simulate...`` routines that provide specific simulation nodes for different scenarios::

        virtual SimNode * simulateDelete ( Context &, const LineInfo &, SimNode *, uint32_t ) const
        virtual SimNode * simulateDeletePtr ( Context &, const LineInfo &, SimNode *, uint32_t ) const
        virtual SimNode * simulateCopy ( Context &, const LineInfo &, SimNode *, SimNode * ) const
        virtual SimNode * simulateClone ( Context &, const LineInfo &, SimNode *, SimNode * ) const
        virtual SimNode * simulateRef2Value ( Context &, const LineInfo &, SimNode * ) const
        virtual SimNode * simulateGetNew ( Context &, const LineInfo & ) const
        virtual SimNode * simulateGetAt ( Context &, const LineInfo &, const TypeDeclPtr &,
                                         const ExpressionPtr &, const ExpressionPtr &, uint32_t ) const
        virtual SimNode * simulateGetAtR2V ( Context &, const LineInfo &, const TypeDeclPtr &,
                                            const ExpressionPtr &, const ExpressionPtr &, uint32_t ) const
        virtual SimNode * simulateGetIterator ( Context &, const LineInfo &, const ExpressionPtr & ) const

``walk`` provides custom data walking functionality, to allow for inspection and binary serialization of the type::

        virtual void walk ( DataWalker &, void * )

--------------------------
ManagedStructureAnnotation
--------------------------

``ManagedStructureAnnotation`` is a helper type annotation template,
designed to streamline the binding of a majority of C++ classes.

Lets review the following example::

    struct Object {
        das::float3   pos;
        das::float3   vel;
        __forceinline float speed() { return sqrt(vel.x*vel.x + vel.y*vel.y + vel.z*vel.z); }
    };

To bind it, we inherit from ``ManagedStructureAnnotation``, provide a name, and register fields and properties::

    struct ObjectStructureTypeAnnotation : ManagedStructureAnnotation <Object> {
        ObjectStructureTypeAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Object",ml) {
            ...

``addField`` and ``addProperty`` are used to add fields and properties accordingly.
Fields are registered as ref values.
Properties are registered with an offset of -1 and are returned by value::

    ObjectStructureTypeAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Object",ml) {
        addField<DAS_BIND_MANAGED_FIELD(pos)>("position","pos");
        addField<DAS_BIND_MANAGED_FIELD(vel)>("velocity","vel");
        addProperty<DAS_BIND_MANAGED_PROP(speed)>("speed","speed");

Afterwards, we register a type factory and add type annotations to the module::

    MAKE_TYPE_FACTORY(Object, Object)

    addAnnotation(make_smart<ObjectStructureTypeAnnotation>(lib));

``addFieldEx`` allows registering custom offsets or types::

    addFieldEx ( "flags", "flags", offsetof(MakeFieldDecl, flags), makeMakeFieldDeclFlags() );

That way, the field of one type can be registered as another type.

Managed structure annotation automatically implements ``walk`` for the exposed fields.

-------------------
DummyTypeAnnotation
-------------------

``DummyTypeAnnotation`` is there when a type needs to be exposed to daScript,
but no contents or operations are allowed.

That way, the type can be part of other structures, and be passed to C++ functions which require it.

The dummy type annotation constructor takes a daScript type name, C++ type name, its size, and alignment::

    DummyTypeAnnotation(const string & name, const string & cppName, size_t sz, size_t al)

Since ``TypeAnnotation`` is a strong daScript type,
``DummyTypeAnnotation`` allows 'gluing' code in daScript without exposing the details of the C++ types.
Consider the following example:

        send_unit_to(get_unit("Ally"), get_unit_pos(get_unit("Enemy")))

The result of ``get_unit`` is passed directly to ``send_unit_to``,
without daScript knowing anything about the unit type (other than that it exists).

-----------------------
ManagedVectorAnnotation
-----------------------

``ManagedVectorAnnotation`` is there to expose standard library vectors to daScript.

For the most part, no integration is required, and vector annotations are automatically
added to the modules, which register anything vector related in any form.

Vectors get registered together with the following 4 functions, similar to those of daScript arrays::

    push(vec, value)
    pop(vec)
    clear(vec)
    resize(vec, newSize)

Vectors also expose the field ``length`` which returns current size of vector.

Managed vector annotation automatically implements ``walk``, similar to daScript arrays.

----------------------
ManagedValueAnnotation
----------------------

``ManagedValueAnnotation`` is designed to expose C++ POD types, which are passed by value.

It expects type :ref:`cast <cast>` machinery to be implemented for that type.



