#pragma once

namespace das {
    class AnnotationFactory;

    template <typename T>
    struct DeserializationFactory {
    public:
        static bool registerCreator ( const char* type, smart_ptr<T> (*creator)() ) {
            map[string(type)] = creator;
            return true;
        }
        static smart_ptr<T> create ( string type ) {
            return map[type]();
        }
    private:
        inline static das_hash_map<string, smart_ptr<T> (*)() > map;
    };
}

// Use in the definition of the annotation class
// Do not use in abstract classes
#define ANNOTATION_DECLARE_SERIALIZABLE( annotation_type )                            \
    virtual const char * getFactoryTag () override { return #annotation_type; }       \
    static AnnotationPtr createInstance () { return make_smart<annotation_type>(); }  \
    static bool registered;


// Use in the .cpp file where the other annotation methods are defined
#define FACTORY_REGISTER_ANNOTATION( annotation_type )                          \
    bool annotation_type :: registered = AnnotationFactory::registerCreator (   \
        #annotation_type, annotation_type::createInstance                       \
    );


// Example:
//
// -- ast.h --
// class Annotation : public BasicAnnotation {
//     ...
//     ANNOTATION_DECLARE_SERIALIZABLE ( Annotation )
// };
//
// -- module_builtin_ast_serializable.cpp --
// ..
// FACTORY_REGISTER_ANNOTATION ( Annotation )
// ..

// A more general version, for all difference macro versions.

#define DECLARE_SERIALIZABLE( BaseType, DerivedType )                                   \
    virtual const char * getFactoryTag () override { return #DerivedType; }             \
    static smart_ptr<BaseType> createInstance () { return make_smart<DerivedType>(); }  \
    static bool registered;

#define FACTORY_REGISTER( BaseType, DerivedType )                                      \
    bool DerivedType::registered = DeserializationFactory<BaseType>::registerCreator ( \
        #DerivedType, DerivedType::createInstance                                      \
    );
