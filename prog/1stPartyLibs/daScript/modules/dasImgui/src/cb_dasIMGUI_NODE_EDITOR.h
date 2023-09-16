#pragma once

#include "imgui_node_editor_stub.h"

namespace das {

// node

template <> struct typeFactory<ax::NodeEditor::NodeId> {
	static TypeDeclPtr make(const ModuleLibrary &) {
		auto t = make_smart<TypeDecl>(Type::tInt);
		t->alias = "ax::NodeEditor::NodeId";
		t->aotAlias = true;
		return t;
	}
};
template <> struct typeName<ax::NodeEditor::NodeId> { constexpr static const char * name() { return "NodeId"; } };
template <> struct cast_arg<ax::NodeEditor::NodeId> {
    static __forceinline ax::NodeEditor::NodeId to ( Context & ctx, SimNode * node ) {
        vec4f res = node->eval(ctx);
        return ax::NodeEditor::NodeId(cast<int32_t>::to(res));
    }
};
template <> struct cast_res<ax::NodeEditor::NodeId> {
    static __forceinline vec4f from ( ax::NodeEditor::NodeId node, Context * ) {
        return cast<int32_t>::from(int32_t(node.Get()));
    }
};

// pin

template <> struct typeFactory<ax::NodeEditor::PinId> {
	static TypeDeclPtr make(const ModuleLibrary &) {
		auto t = make_smart<TypeDecl>(Type::tInt);
		t->alias = "ax::NodeEditor::PinId";
		t->aotAlias = true;
		return t;
	}
};
template <> struct typeName<ax::NodeEditor::PinId> { constexpr static const char * name() { return "PinId"; } };
template <> struct cast_arg<ax::NodeEditor::PinId> {
    static __forceinline ax::NodeEditor::PinId to ( Context & ctx, SimNode * node ) {
        vec4f res = node->eval(ctx);
        return ax::NodeEditor::PinId(cast<int32_t>::to(res));
    }
};
template <> struct cast_res<ax::NodeEditor::PinId> {
    static __forceinline vec4f from ( ax::NodeEditor::PinId node, Context * ) {
        return cast<int32_t>::from(int32_t(node.Get()));
    }
};

// link

template <> struct typeFactory<ax::NodeEditor::LinkId> {
	static TypeDeclPtr make(const ModuleLibrary &) {
		auto t = make_smart<TypeDecl>(Type::tInt);
		t->alias = "ax::NodeEditor::LinkId";
		t->aotAlias = true;
		return t;
	}
};
template <> struct typeName<ax::NodeEditor::LinkId> { constexpr static const char * name() { return "LinkId"; } };
template <> struct cast_arg<ax::NodeEditor::LinkId> {
    static __forceinline ax::NodeEditor::LinkId to ( Context & ctx, SimNode * node ) {
        vec4f res = node->eval(ctx);
        return ax::NodeEditor::LinkId(cast<int32_t>::to(res));
    }
};
template <> struct cast_res<ax::NodeEditor::LinkId> {
    static __forceinline vec4f from ( ax::NodeEditor::LinkId node, Context * ) {
        return cast<int32_t>::from(int32_t(node.Get()));
    }
};

struct imgui_node_editorTempFn {
    imgui_node_editorTempFn() = default;
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
