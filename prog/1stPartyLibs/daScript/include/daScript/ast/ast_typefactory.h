#pragma once

namespace das {
    template <typename TT>
    struct TTemporary {};


    template <typename TT>
    struct TExplicit {};
}

#define MAKE_TYPE_FACTORY(TYPE,CTYPE) \
namespace das { \
  template <> \
  struct typeFactory<CTYPE> { \
      static ___noinline TypeDeclPtr make(const ModuleLibrary & library ) { \
          return makeHandleType(library,#TYPE); \
      } \
  }; \
  template <> \
  struct typeName<CTYPE> { \
      constexpr static const char * name() { return #TYPE; } \
  }; \
};

#define MAKE_EXTERNAL_TYPE_FACTORY(TYPE,CTYPE) \
namespace das { \
  class ModuleLibrary; \
  struct TypeDecl; \
  typedef smart_ptr<TypeDecl> TypeDeclPtr; \
  template <typename TT> struct typeFactory; \
  template <> \
  struct typeFactory<CTYPE> { \
      static ___noinline TypeDeclPtr make(const ModuleLibrary & library ); \
  }; \
  template <typename TT> struct typeName; \
  template <> \
  struct typeName<CTYPE> { \
      constexpr static const char * name() { return #TYPE; } \
  }; \
};

#define IMPLEMENT_EXTERNAL_TYPE_FACTORY(TYPE,CTYPE) \
namespace das { \
    TypeDeclPtr typeFactory<CTYPE>::make(const ModuleLibrary & library ) { \
        return makeHandleType(library,#TYPE); \
    } \
};
