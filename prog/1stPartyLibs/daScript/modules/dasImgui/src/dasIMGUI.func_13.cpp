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
void Module_dasIMGUI::initFunctions_13() {
	makeExtern< float (*)(int) , ImGui::GetColumnWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnWidth","ImGui::GetColumnWidth")
		->args({"column_index"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,float) , ImGui::SetColumnWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetColumnWidth","ImGui::SetColumnWidth")
		->args({"column_index","width"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)(int) , ImGui::GetColumnOffset , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnOffset","ImGui::GetColumnOffset")
		->args({"column_index"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,float) , ImGui::SetColumnOffset , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetColumnOffset","ImGui::SetColumnOffset")
		->args({"column_index","offset_x"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)() , ImGui::GetColumnsCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnsCount","ImGui::GetColumnsCount")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::BeginTabBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTabBar","ImGui::BeginTabBar")
		->args({"str_id","flags"})
		->arg_type(1,makeType<ImGuiTabBarFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabBarFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndTabBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTabBar","ImGui::EndTabBar")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::BeginTabItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTabItem","ImGui::BeginTabItem")
		->args({"label","p_open","flags"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_type(2,makeType<ImGuiTabItemFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabItemFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndTabItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTabItem","ImGui::EndTabItem")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::TabItemButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"TabItemButton","ImGui::TabItemButton")
		->args({"label","flags"})
		->arg_type(1,makeType<ImGuiTabItemFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabItemFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::SetTabItemClosed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetTabItemClosed","ImGui::SetTabItemClosed")
		->args({"tab_or_docked_window_label"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ImGui::LogToTTY , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToTTY","ImGui::LogToTTY")
		->args({"auto_open_depth"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,const char *) , ImGui::LogToFile , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToFile","ImGui::LogToFile")
		->args({"auto_open_depth","filename"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->arg_init(1,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ImGui::LogToClipboard , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToClipboard","ImGui::LogToClipboard")
		->args({"auto_open_depth"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::LogFinish , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogFinish","ImGui::LogFinish")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::LogButtons , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogButtons","ImGui::LogButtons")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::BeginDragDropSource , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDragDropSource","ImGui::BeginDragDropSource")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiDragDropFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiDragDropFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const void *,size_t,int) , ImGui::SetDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetDragDropPayload","ImGui::SetDragDropPayload")
		->args({"type","data","sz","cond"})
		->arg_type(3,makeType<ImGuiCond_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndDragDropSource , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDragDropSource","ImGui::EndDragDropSource")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::BeginDragDropTarget , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDragDropTarget","ImGui::BeginDragDropTarget")
		->addToModule(*this, SideEffects::worstDefault);
}
}

