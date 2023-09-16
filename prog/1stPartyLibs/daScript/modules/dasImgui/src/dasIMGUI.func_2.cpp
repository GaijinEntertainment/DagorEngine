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
void Module_dasIMGUI::initFunctions_2() {
	makeExtern< void (*)(ImGuiStyle *) , ImGui::StyleColorsClassic , SimNode_ExtFuncCall , imguiTempFn>(lib,"StyleColorsClassic","ImGui::StyleColorsClassic")
		->args({"dst"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::Begin , SimNode_ExtFuncCall , imguiTempFn>(lib,"Begin","ImGui::Begin")
		->args({"name","p_open","flags"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_type(2,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::End , SimNode_ExtFuncCall , imguiTempFn>(lib,"End","ImGui::End")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const ImVec2 &,bool,int) , ImGui::BeginChild , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginChild","ImGui::BeginChild")
		->args({"str_id","size","border","flags"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->arg_type(3,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(unsigned int,const ImVec2 &,bool,int) , ImGui::BeginChild , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginChild","ImGui::BeginChild")
		->args({"id","size","border","flags"})
		->arg_init(2,make_smart<ExprConstBool>(false))
		->arg_type(3,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndChild , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndChild","ImGui::EndChild")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsWindowAppearing , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowAppearing","ImGui::IsWindowAppearing")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::IsWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowCollapsed","ImGui::IsWindowCollapsed")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsWindowFocused , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowFocused","ImGui::IsWindowFocused")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiFocusedFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiFocusedFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::IsWindowHovered , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowHovered","ImGui::IsWindowHovered")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiHoveredFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiHoveredFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImDrawList * (*)() , ImGui::GetWindowDrawList , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowDrawList","ImGui::GetWindowDrawList")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowPos","ImGui::GetWindowPos")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowSize","ImGui::GetWindowSize")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetWindowWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowWidth","ImGui::GetWindowWidth")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetWindowHeight , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowHeight","ImGui::GetWindowHeight")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &,int,const ImVec2 &) , ImGui::SetNextWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowPos","ImGui::SetNextWindowPos")
		->args({"pos","cond","pivot"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &,int) , ImGui::SetNextWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowSize","ImGui::SetNextWindowSize")
		->args({"size","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetNextWindowContentSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowContentSize","ImGui::SetNextWindowContentSize")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(bool,int) , ImGui::SetNextWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowCollapsed","ImGui::SetNextWindowCollapsed")
		->args({"collapsed","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::SetNextWindowFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowFocus","ImGui::SetNextWindowFocus")
		->addToModule(*this, SideEffects::worstDefault);
}
}

