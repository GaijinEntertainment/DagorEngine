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
void Module_dasIMGUI::initFunctions_14() {
// from imgui/imgui.h:951:29
	makeExtern< int (*)() , ImGui::GetColumnIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnIndex","ImGui::GetColumnIndex")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:952:29
	makeExtern< float (*)(int) , ImGui::GetColumnWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnWidth","ImGui::GetColumnWidth")
		->args({"column_index"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:953:29
	makeExtern< void (*)(int,float) , ImGui::SetColumnWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetColumnWidth","ImGui::SetColumnWidth")
		->args({"column_index","width"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:954:29
	makeExtern< float (*)(int) , ImGui::GetColumnOffset , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnOffset","ImGui::GetColumnOffset")
		->args({"column_index"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:955:29
	makeExtern< void (*)(int,float) , ImGui::SetColumnOffset , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetColumnOffset","ImGui::SetColumnOffset")
		->args({"column_index","offset_x"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:956:29
	makeExtern< int (*)() , ImGui::GetColumnsCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnsCount","ImGui::GetColumnsCount")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:960:29
	makeExtern< bool (*)(const char *,int) , ImGui::BeginTabBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTabBar","ImGui::BeginTabBar")
		->args({"str_id","flags"})
		->arg_type(1,makeType<ImGuiTabBarFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabBarFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:961:29
	makeExtern< void (*)() , ImGui::EndTabBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTabBar","ImGui::EndTabBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:962:29
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::BeginTabItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTabItem","ImGui::BeginTabItem")
		->args({"label","p_open","flags"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_type(2,makeType<ImGuiTabItemFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabItemFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:963:29
	makeExtern< void (*)() , ImGui::EndTabItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTabItem","ImGui::EndTabItem")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:964:29
	makeExtern< bool (*)(const char *,int) , ImGui::TabItemButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"TabItemButton","ImGui::TabItemButton")
		->args({"label","flags"})
		->arg_type(1,makeType<ImGuiTabItemFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabItemFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:965:29
	makeExtern< void (*)(const char *) , ImGui::SetTabItemClosed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetTabItemClosed","ImGui::SetTabItemClosed")
		->args({"tab_or_docked_window_label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:988:29
	makeExtern< unsigned int (*)(unsigned int,const ImVec2 &,int,const ImGuiWindowClass *) , ImGui::DockSpace , SimNode_ExtFuncCall , imguiTempFn>(lib,"DockSpace","ImGui::DockSpace")
		->args({"dockspace_id","size","flags","window_class"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->arg_init(3,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:989:29
	makeExtern< unsigned int (*)(unsigned int,const ImGuiViewport *,int,const ImGuiWindowClass *) , ImGui::DockSpaceOverViewport , SimNode_ExtFuncCall , imguiTempFn>(lib,"DockSpaceOverViewport","ImGui::DockSpaceOverViewport")
		->args({"dockspace_id","viewport","flags","window_class"})
		->arg_init(0,make_smart<ExprConstUInt>(0x0))
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_init(2,make_smart<ExprConstInt>(0))
		->arg_init(3,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:990:29
	makeExtern< void (*)(unsigned int,int) , ImGui::SetNextWindowDockID , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowDockID","ImGui::SetNextWindowDockID")
		->args({"dock_id","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:991:29
	makeExtern< void (*)(const ImGuiWindowClass *) , ImGui::SetNextWindowClass , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowClass","ImGui::SetNextWindowClass")
		->args({"window_class"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:992:29
	makeExtern< unsigned int (*)() , ImGui::GetWindowDockID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowDockID","ImGui::GetWindowDockID")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:993:29
	makeExtern< bool (*)() , ImGui::IsWindowDocked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowDocked","ImGui::IsWindowDocked")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:997:29
	makeExtern< void (*)(int) , ImGui::LogToTTY , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToTTY","ImGui::LogToTTY")
		->args({"auto_open_depth"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:998:29
	makeExtern< void (*)(int,const char *) , ImGui::LogToFile , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToFile","ImGui::LogToFile")
		->args({"auto_open_depth","filename"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->arg_init(1,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
}
}

