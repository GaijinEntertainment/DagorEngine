#pragma once

#include "for_each.h"

#define DAS_BIND_ENUM_PRINT_HELPER(x, ARG) #x,
#define DAS_BIND_ENUM_QUALIFIED_HELPER(x, ARG) ARG::x,
#define DAS_BIND_ENUM_UNQUALIFIED_HELPER(x, ARG) x,

// sample of enumeration
#define DAS_BASE_BIND_ENUM_CAST(enum_name, das_enum_name)\
  namespace das \
  { \
    template <> struct cast < enum_name > : cast_enum < enum_name > {};\
  };

// sample of enumeration
#define DAS_BASE_BIND_ENUM_FACTORY(enum_name, das_enum_name)\
  namespace das \
  { \
    template <>\
    struct typeFactory< enum_name > {\
        static TypeDeclPtr make(const ModuleLibrary & library ) {\
            return library.makeEnumType(das_enum_name);\
        }\
    }; \
  };

#define DAS_BIND_ENUM_CAST(enum_name) \
    DAS_BASE_BIND_ENUM_CAST(enum_name, #enum_name)

#define DAS_BIND_ENUM_CAST_98(enum_name) \
    enum class enum_name##_DasProxy {}; \
    DAS_BASE_BIND_ENUM_CAST(enum_name##_DasProxy, #enum_name) \
    DAS_BASE_BIND_ENUM_CAST(enum_name, #enum_name)

#define DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(enum_name,stripped_enum_name) \
    enum class stripped_enum_name##_DasProxy {}; \
    DAS_BASE_BIND_ENUM_CAST(stripped_enum_name##_DasProxy, #enum_name) \
    DAS_BASE_BIND_ENUM_CAST(enum_name, #enum_name)

#define DAS_BASE_BIND_ENUM_BOTH(helper, enum_name, das_enum_name, ...) \
class Enumeration##das_enum_name : public das::Enumeration {\
public:\
    Enumeration##das_enum_name() : das::Enumeration(#das_enum_name) {\
        external = true;\
        cppName = #enum_name; \
        baseType = (das::Type) das::ToBasicType< das::underlying_type< enum_name >::type >::type; \
        enum_name enumArray[] = { DAS_FOR_EACH(helper, enum_name, __VA_ARGS__) };\
        static const char *enumArrayName[] = { DAS_FOR_EACH(DAS_BIND_ENUM_PRINT_HELPER, enum_name, __VA_ARGS__) };\
        for (uint32_t i = 0; i < sizeof(enumArray)/sizeof(enumArray[0]); ++i)\
            addI(enumArrayName[i], int64_t(enumArray[i]), das::LineInfo());\
    }\
};

#define DAS_BASE_BIND_ENUM_GEN(enum_name, das_enum_name) \
    DAS_BASE_BIND_ENUM_FACTORY(enum_name, #das_enum_name)

#define DAS_BASE_BIND_ENUM_98_GEN(enum_name, das_enum_name) \
    DAS_BASE_BIND_ENUM_FACTORY(enum_name, #das_enum_name)\
    DAS_BASE_BIND_ENUM_FACTORY(enum_name##_DasProxy, #das_enum_name)

#define DAS_BASE_BIND_ENUM(enum_name, das_enum_name, ...) \
    DAS_BASE_BIND_ENUM_BOTH(DAS_BIND_ENUM_QUALIFIED_HELPER, enum_name, das_enum_name, __VA_ARGS__)\
    DAS_BASE_BIND_ENUM_FACTORY(enum_name, #das_enum_name)

#define DAS_BASE_BIND_ENUM_98(enum_name, das_enum_name, ...) \
    DAS_BASE_BIND_ENUM_BOTH(DAS_BIND_ENUM_QUALIFIED_HELPER, enum_name, das_enum_name, __VA_ARGS__)\
    DAS_BASE_BIND_ENUM_FACTORY(enum_name, #das_enum_name)\
    DAS_BASE_BIND_ENUM_FACTORY(das_enum_name##_DasProxy, #das_enum_name)

#define DAS_BASE_BIND_ENUM_IMPL(enum_name, das_enum_name, ...) \
    DAS_BASE_BIND_ENUM_BOTH(DAS_BIND_ENUM_QUALIFIED_HELPER, enum_name, das_enum_name, __VA_ARGS__)
