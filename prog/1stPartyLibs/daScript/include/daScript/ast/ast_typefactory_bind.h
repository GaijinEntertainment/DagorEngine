#pragma once

#include "ast_typefactory.h"

namespace das {
    // note:
    //  this binds any unspecified 'callback' thing to 'void *'
    //  generally this is included with generated headers
    template <typename ResT, typename ...Args>
    struct typeFactory<ResT (*)(Args...)> {
        static TypeDeclPtr make(const ModuleLibrary & library ) {
            return typeFactory<void *>::make(library);
        };
    };

    // note:
    //  this is here to pass strings safely
    template <>
    struct cast_arg<char *> {
        static __forceinline char * to ( Context & ctx, SimNode * node ) {
            char * res = node->evalPtr(ctx);
            return res ? res : ((char *)"");
        }
    };

    template <>
    struct cast_arg<const char *> {
        static __forceinline char * to ( Context & ctx, SimNode * node ) {
            char * res = node->evalPtr(ctx);
            return res ? res : ((char *)"");
        }
    };

    template <>
    struct cast_arg<das::string> {
        static __forceinline string to ( Context & ctx, SimNode * node ) {
            auto res = (das::string *) node->evalPtr(ctx);
            return *res;
        }
    };
}
