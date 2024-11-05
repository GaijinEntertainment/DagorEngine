// this file is generated via Daslang automatic C++ binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasIMGUI.h"
#include "need_dasIMGUI.h"
namespace das {
#include "dasIMGUI.func.aot.decl.inc"
void Module_dasIMGUI::initFunctions_3() {
// from imgui/imgui.h:411:29
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetNextWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowSize","ImGui::SetNextWindowSize")
		->args({"size","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:413:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetNextWindowContentSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowContentSize","ImGui::SetNextWindowContentSize")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:414:29
	makeExtern< void (*)(bool,int) , ImGui::SetNextWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowCollapsed","ImGui::SetNextWindowCollapsed")
		->args({"collapsed","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:415:29
	makeExtern< void (*)() , ImGui::SetNextWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowFocus","ImGui::SetNextWindowFocus")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:416:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetNextWindowScroll , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowScroll","ImGui::SetNextWindowScroll")
		->args({"scroll"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:417:29
	makeExtern< void (*)(float) , ImGui::SetNextWindowBgAlpha , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowBgAlpha","ImGui::SetNextWindowBgAlpha")
		->args({"alpha"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:418:29
	makeExtern< void (*)(unsigned int) , ImGui::SetNextWindowViewport , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowViewport","ImGui::SetNextWindowViewport")
		->args({"viewport_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:419:29
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowPos","ImGui::SetWindowPos")
		->args({"pos","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:420:29
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowSize","ImGui::SetWindowSize")
		->args({"size","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:421:29
	makeExtern< void (*)(bool,int) , ImGui::SetWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowCollapsed","ImGui::SetWindowCollapsed")
		->args({"collapsed","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:422:29
	makeExtern< void (*)() , ImGui::SetWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFocus","ImGui::SetWindowFocus")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:423:29
	makeExtern< void (*)(float) , ImGui::SetWindowFontScale , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFontScale","ImGui::SetWindowFontScale")
		->args({"scale"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:424:29
	makeExtern< void (*)(const char *,const ImVec2 &,int) , ImGui::SetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowPos","ImGui::SetWindowPos")
		->args({"name","pos","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:425:29
	makeExtern< void (*)(const char *,const ImVec2 &,int) , ImGui::SetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowSize","ImGui::SetWindowSize")
		->args({"name","size","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:426:29
	makeExtern< void (*)(const char *,bool,int) , ImGui::SetWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowCollapsed","ImGui::SetWindowCollapsed")
		->args({"name","collapsed","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:427:29
	makeExtern< void (*)(const char *) , ImGui::SetWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFocus","ImGui::SetWindowFocus")
		->args({"name"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:432:29
	makeExtern< float (*)() , ImGui::GetScrollX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollX","ImGui::GetScrollX")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:433:29
	makeExtern< float (*)() , ImGui::GetScrollY , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollY","ImGui::GetScrollY")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:434:29
	makeExtern< void (*)(float) , ImGui::SetScrollX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollX","ImGui::SetScrollX")
		->args({"scroll_x"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:435:29
	makeExtern< void (*)(float) , ImGui::SetScrollY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollY","ImGui::SetScrollY")
		->args({"scroll_y"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

