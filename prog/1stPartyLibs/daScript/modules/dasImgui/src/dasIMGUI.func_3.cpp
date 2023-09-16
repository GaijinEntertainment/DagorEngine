// this file is generated via daScript automatic C++ binder
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
	makeExtern< void (*)(float) , ImGui::SetNextWindowBgAlpha , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowBgAlpha","ImGui::SetNextWindowBgAlpha")
		->args({"alpha"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowPos","ImGui::SetWindowPos")
		->args({"pos","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowSize","ImGui::SetWindowSize")
		->args({"size","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(bool,int) , ImGui::SetWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowCollapsed","ImGui::SetWindowCollapsed")
		->args({"collapsed","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::SetWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFocus","ImGui::SetWindowFocus")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::SetWindowFontScale , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFontScale","ImGui::SetWindowFontScale")
		->args({"scale"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,const ImVec2 &,int) , ImGui::SetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowPos","ImGui::SetWindowPos")
		->args({"name","pos","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,const ImVec2 &,int) , ImGui::SetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowSize","ImGui::SetWindowSize")
		->args({"name","size","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,bool,int) , ImGui::SetWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowCollapsed","ImGui::SetWindowCollapsed")
		->args({"name","collapsed","cond"})
		->arg_type(2,makeType<ImGuiCond_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::SetWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetWindowFocus","ImGui::SetWindowFocus")
		->args({"name"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetContentRegionAvail , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetContentRegionAvail","ImGui::GetContentRegionAvail")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetContentRegionMax , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetContentRegionMax","ImGui::GetContentRegionMax")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetWindowContentRegionMin , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowContentRegionMin","ImGui::GetWindowContentRegionMin")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetWindowContentRegionMax , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowContentRegionMax","ImGui::GetWindowContentRegionMax")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetWindowContentRegionWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowContentRegionWidth","ImGui::GetWindowContentRegionWidth")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetScrollX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollX","ImGui::GetScrollX")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetScrollY , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollY","ImGui::GetScrollY")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::SetScrollX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollX","ImGui::SetScrollX")
		->args({"scroll_x"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::SetScrollY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetScrollY","ImGui::SetScrollY")
		->args({"scroll_y"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetScrollMaxX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetScrollMaxX","ImGui::GetScrollMaxX")
		->addToModule(*this, SideEffects::worstDefault);
}
}

