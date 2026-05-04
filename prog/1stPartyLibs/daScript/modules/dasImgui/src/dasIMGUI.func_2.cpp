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
void Module_dasIMGUI::initFunctions_2() {
// from imgui/imgui.h:420:29
	makeExtern< bool (*)(const char *) , ImGui::ShowStyleSelector , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowStyleSelector","ImGui::ShowStyleSelector")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:421:29
	makeExtern< void (*)(const char *) , ImGui::ShowFontSelector , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowFontSelector","ImGui::ShowFontSelector")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:422:29
	makeExtern< void (*)() , ImGui::ShowUserGuide , SimNode_ExtFuncCall , imguiTempFn>(lib,"ShowUserGuide","ImGui::ShowUserGuide")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:423:29
	makeExtern< const char * (*)() , ImGui::GetVersion , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetVersion","ImGui::GetVersion")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:426:29
	makeExtern< void (*)(ImGuiStyle *) , ImGui::StyleColorsDark , SimNode_ExtFuncCall , imguiTempFn>(lib,"StyleColorsDark","ImGui::StyleColorsDark")
		->args({"dst"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:427:29
	makeExtern< void (*)(ImGuiStyle *) , ImGui::StyleColorsLight , SimNode_ExtFuncCall , imguiTempFn>(lib,"StyleColorsLight","ImGui::StyleColorsLight")
		->args({"dst"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:428:29
	makeExtern< void (*)(ImGuiStyle *) , ImGui::StyleColorsClassic , SimNode_ExtFuncCall , imguiTempFn>(lib,"StyleColorsClassic","ImGui::StyleColorsClassic")
		->args({"dst"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:442:29
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::Begin , SimNode_ExtFuncCall , imguiTempFn>(lib,"Begin","ImGui::Begin")
		->args({"name","p_open","flags"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_type(2,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:443:29
	makeExtern< void (*)() , ImGui::End , SimNode_ExtFuncCall , imguiTempFn>(lib,"End","ImGui::End")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:463:29
	makeExtern< bool (*)(const char *,const ImVec2 &,int,int) , ImGui::BeginChild , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginChild","ImGui::BeginChild")
		->args({"str_id","size","child_flags","window_flags"})
		->arg_type(2,makeType<ImGuiChildFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiChildFlags_>(lib)))
		->arg_type(3,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:464:29
	makeExtern< bool (*)(unsigned int,const ImVec2 &,int,int) , ImGui::BeginChild , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginChild","ImGui::BeginChild")
		->args({"id","size","child_flags","window_flags"})
		->arg_type(2,makeType<ImGuiChildFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiChildFlags_>(lib)))
		->arg_type(3,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:465:29
	makeExtern< void (*)() , ImGui::EndChild , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndChild","ImGui::EndChild")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:469:29
	makeExtern< bool (*)() , ImGui::IsWindowAppearing , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowAppearing","ImGui::IsWindowAppearing")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:470:29
	makeExtern< bool (*)() , ImGui::IsWindowCollapsed , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowCollapsed","ImGui::IsWindowCollapsed")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:471:29
	makeExtern< bool (*)(int) , ImGui::IsWindowFocused , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowFocused","ImGui::IsWindowFocused")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiFocusedFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiFocusedFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:472:29
	makeExtern< bool (*)(int) , ImGui::IsWindowHovered , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowHovered","ImGui::IsWindowHovered")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiHoveredFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiHoveredFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:473:29
	makeExtern< ImDrawList * (*)() , ImGui::GetWindowDrawList , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowDrawList","ImGui::GetWindowDrawList")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:474:29
	makeExtern< float (*)() , ImGui::GetWindowDpiScale , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowDpiScale","ImGui::GetWindowDpiScale")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:475:29
	makeExtern< ImVec2 (*)() , ImGui::GetWindowPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowPos","ImGui::GetWindowPos")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:476:29
	makeExtern< ImVec2 (*)() , ImGui::GetWindowSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowSize","ImGui::GetWindowSize")
		->addToModule(*this, SideEffects::worstDefault);
}
}

