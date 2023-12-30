.. _cast:

=======================================
C++ ABI and type factory infrastructure
=======================================

----
Cast
----

When C++ interfaces with Daslang, the `cast` ABI is followed.

 * Value types are converted to and from `vec4f`, in specific memory layout
 * Reference types have their address converted to and from `vec4f`

It is expected that vec4f * can be pruned to a by value type by simple pointer cast
becase the Daslang interpreter will in certain cases access pre-cast data via the v_ldu intrinsic::

    template <typename TT>
    TT get_data ( vec4f * dasData ) {           // default version
        return cast<TT>::to(v_ldu((float *)dasData));
    }

    int32_t get_data ( vec4f * dasData ) {      // legally optimized version
        return * (int32_t *) dasData;
    }

ABI infrastructure is implemented via the C++ cast template, which serves two primary functions:

 * Casting ``from`` C++ to Daslang
 * Casting ``to`` C++ from Daslang

The ``from`` function expects a Daslang type as an input, and outputs a vec4f.

The ``to`` function expects a vec4f, and outputs a Daslang type.

Let's review the following example::

    template <>
    struct cast <int32_t> {
        static __forceinline int32_t to ( vec4f x )            { return v_extract_xi(v_cast_vec4i(x)); }
        static __forceinline vec4f from ( int32_t x )          { return v_cast_vec4f(v_splatsi(x)); }
    };

It implements the ABI for the int32_t, which packs an int32_t value at the beginning of the vec4f using multiplatform intrinsics.

Let's review another example, which implements default packing of a reference type::

    template <typename TT>
    struct cast <TT &> {
        static __forceinline TT & to ( vec4f a )               { return *(TT *) v_extract_ptr(v_cast_vec4i((a))); }
        static __forceinline vec4f from ( const TT & p )       { return v_cast_vec4f(v_splats_ptr((const void *)&p)); }
    };

Here, a pointer to the data is packed in a vec4f using multiplatform intrinsics.

------------
Type factory
------------

When C++ types are exposed to Daslang, type factory infrastructure is employed.

To expose any custom C++ type, use the ``MAKE_TYPE_FACTORY`` macro,
or the ``MAKE_EXTERNAL_TYPE_FACTORY`` and ``IMPLEMENT_EXTERNAL_TYPE_FACTORY`` macro pair::

    MAKE_TYPE_FACTORY(clock, das::Time)

The example above tells Daslang that the C++ type `das::Time` will be exposed to Daslang with the name `clock`.

Let's look at the implementation of the ``MAKE_TYPE_FACTORY`` macro::

    #define MAKE_TYPE_FACTORY(TYPE,CTYPE) \
        namespace das { \
        template <> \
        struct typeFactory<CTYPE> { \
            static TypeDeclPtr make(const ModuleLibrary & library ) { \
                return makeHandleType(library,#TYPE); \
            } \
        }; \
        template <> \
        struct typeName<CTYPE> { \
            constexpr static const char * name() { return #TYPE; } \
        }; \
        };

What happens in the example above is that two templated policies are exposed to C++.

The ``typeName`` policy has a single static function ``name``, which returns the string name of the type.

The ``typeFactory`` policy creates a smart pointer to Daslang the `das::TypeDecl` type, which corresponds to C++ type.
It expects to find the type somewhere in the provided ModuleLibrary (see :ref:`Modules <modules>`).

------------
Type aliases
------------

A custom type factory is the preferable way to create aliases::

    struct Point3 { float x, y, z; };

    template <>
    struct typeFactory<Point3> {
        static TypeDeclPtr make(const ModuleLibrary &) {
            auto t = make_smart<TypeDecl>(Type::tFloat3);
            t->alias = "Point3";
            t->aotAlias = true;
            return t;
        }
    };

    template <> struct typeName<Point3>   { constexpr static const char * name() { return "Point3"; } };

In the example above, the C++ application already has a `Point3` type, which is very similar to Daslang's float3.
Exposing C++ functions which operate on Point3 is preferable, so the implementation creates an alias named `Point3`
which corresponds to the das Type::tFloat3.

Sometimes, a custom implementation of ``typeFactory`` is required to expose C++ to a Daslang
type in a more native fashion. Let's review the following example::

    struct SampleVariant {
        int32_t _variant;
        union {
            int32_t     i_value;
            float       f_value;
            char *      s_value;
        };
    };

  template <>
  struct typeFactory<SampleVariant> {
      static TypeDeclPtr make(const ModuleLibrary & library ) {
          auto vtype = make_smart<TypeDecl>(Type::tVariant);
          vtype->alias = "SampleVariant";
          vtype->aotAlias = true;
          vtype->addVariant("i_value", typeFactory<decltype(SampleVariant::i_value)>::make(library));
          vtype->addVariant("f_value", typeFactory<decltype(SampleVariant::f_value)>::make(library));
          vtype->addVariant("s_value", typeFactory<decltype(SampleVariant::s_value)>::make(library));
          // optional validation
          DAS_ASSERT(sizeof(SampleVariant) == vtype->getSizeOf());
          DAS_ASSERT(alignof(SampleVariant) == vtype->getAlignOf());
          DAS_ASSERT(offsetof(SampleVariant, i_value) == vtype->getVariantFieldOffset(0));
          DAS_ASSERT(offsetof(SampleVariant, f_value) == vtype->getVariantFieldOffset(1));
          DAS_ASSERT(offsetof(SampleVariant, s_value) == vtype->getVariantFieldOffset(2));
          return vtype;
      }
  };

Here, C++ type `SomeVariant` matches the Daslang variant type with its memory layout.
The code above exposes a C++ type alias and creates a corresponding TypeDecl.
