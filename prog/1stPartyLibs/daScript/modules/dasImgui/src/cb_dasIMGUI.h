#pragma once

#include "imgui_stub.h"

namespace das {

template <> struct typeFactory<ImColor> {
	static TypeDeclPtr make(const ModuleLibrary &) {
		auto t = make_smart<TypeDecl>(Type::tFloat4);
		t->alias = "ImColor";
		t->aotAlias = true;
		return t;
	}
};
template <> struct typeName<ImColor> { constexpr static const char * name() { return "ImColor"; } };
template <>
struct cast_arg<const ImColor &> {
    static __forceinline ImColor to ( Context & ctx, SimNode * node ) {
        vec4f res = node->eval(ctx);
        ImColor col; memcpy(&col,&res,sizeof(ImColor));
        return col;
    }
};

template <> struct typeFactory<ImVec2> {
	static TypeDeclPtr make(const ModuleLibrary &) {
		auto t = make_smart<TypeDecl>(Type::tFloat2);
		t->alias = "ImVec2";
		t->aotAlias = true;
		return t;
	}
};
template <> struct typeName<ImVec2> { constexpr static const char * name() { return "ImVec2"; } };
template <> struct cast_arg<const ImVec2 &> {
    static __forceinline ImVec2 to ( Context & ctx, SimNode * node ) {
        vec4f res = node->eval(ctx);
        ImVec2 v2; memcpy(&v2,&res,sizeof(ImVec2));
        return v2;
    }
};

template <> struct typeFactory<ImVec4> {
	static TypeDeclPtr make(const ModuleLibrary &) {
		auto t = make_smart<TypeDecl>(Type::tFloat4);
		t->alias = "ImVec4";
		t->aotAlias = true;
		return t;
	}
};
template <> struct typeName<ImVec4> { constexpr static const char * name() { return "ImVec4"; } };
template <> struct cast_arg<const ImVec4 &> {
    static __forceinline ImVec4 to ( Context & ctx, SimNode * node ) {
        vec4f res = node->eval(ctx);
        ImVec4 v4; memcpy(&v4,&res,sizeof(ImVec4));
        return v4;
    }
};

template<> struct cast <ImVec2>  : cast_fVec_half<ImVec2> {};
template<> struct cast <ImVec4>  : cast_fVec<ImVec4> {};
template<> struct cast <ImColor> : cast_fVec<ImColor> {};

template <>
struct typeName<char> {
    static string name() {
        return string("char");
    }
};

template <typename TT>
struct typeName<TT *> {
    static string name() {
        return string("ptr`") + typeName<TT>::name();
    }
};

template <typename TT>
struct typeName<ImVector<TT>> {
    static string name() {
        return string("ImVector`") + typeName<TT>::name();
    }
};

template <typename TT>
struct typeFactory<ImVector<TT>> {
    using VT = ImVector<TT>;
    static TypeDeclPtr make(const ModuleLibrary & library ) {
        string declN = typeName<VT>::name();
        if ( library.findAnnotation(declN,nullptr).size()==0 ) {
            auto declT = makeType<TT>(library);
            auto ann = make_smart<ManagedVectorAnnotation<VT>>(declN,const_cast<ModuleLibrary &>(library));
            ann->cppName = "ImVector<" + describeCppType(declT) + ">";
            auto mod = library.front();
            mod->addAnnotation(ann);
            addExtern<DAS_BIND_FUN(das_vector_length<VT>)>(*mod, library, "length",
                SideEffects::none, "das_vector_length")->generated = true;
            /*
            registerVectorFunctions<VT,has_cast<TT>::value>::init(mod,library,
                declT->canCopy(),
                declT->canMove()
            );
            */
        }
        return makeHandleType(library,declN.c_str());
    }
};

struct imguiTempFn {
    imguiTempFn() = default;
    ___noinline bool operator()( Function * fn ) {
        if ( tempArgs || implicitArgs ) {
            for ( auto &arg : fn->arguments ) {
                if ( arg->type->isTempType() ) {
                    arg->type->temporary = tempArgs;
                    arg->type->implicit = implicitArgs;
                    arg->type->explicitConst = explicitConstArgs;
                }
            }
        }
        if ( tempResult ) {
            if ( fn->result->isTempType() ) {
                fn->result->temporary = true;
            }
        }

        bool anyString = false;
        for ( auto &arg : fn->arguments ) {
            if ( arg->type->constant && arg->type->ref && arg->type->dim.size() == 0 ) {
                if ( arg->type->baseType == Type::tFloat2 || arg->type->baseType == Type::tFloat4 ) {
                    arg->type->ref = false;
                }
            }
            if ( arg->type->isString() && !arg->type->ref ) {
                anyString = true;
            }
        }

        if (anyString) {
            fn->needStringCast = true;
        }

        return true;
    }
    bool tempArgs = false;
    bool implicitArgs = true;
    bool tempResult = false;
    bool explicitConstArgs = false;
};

}
