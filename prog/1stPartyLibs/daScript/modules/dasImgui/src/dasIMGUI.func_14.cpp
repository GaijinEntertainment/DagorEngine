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
// from imgui/imgui.h:862:29
	makeExtern< void (*)() , ImGui::EndTabBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTabBar","ImGui::EndTabBar")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:863:29
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::BeginTabItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTabItem","ImGui::BeginTabItem")
		->args({"label","p_open","flags"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_type(2,makeType<ImGuiTabItemFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabItemFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:864:29
	makeExtern< void (*)() , ImGui::EndTabItem , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTabItem","ImGui::EndTabItem")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:865:29
	makeExtern< bool (*)(const char *,int) , ImGui::TabItemButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"TabItemButton","ImGui::TabItemButton")
		->args({"label","flags"})
		->arg_type(1,makeType<ImGuiTabItemFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTabItemFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:866:29
	makeExtern< void (*)(const char *) , ImGui::SetTabItemClosed , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetTabItemClosed","ImGui::SetTabItemClosed")
		->args({"tab_or_docked_window_label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:881:29
	makeExtern< unsigned int (*)(unsigned int,const ImVec2 &,int,const ImGuiWindowClass *) , ImGui::DockSpace , SimNode_ExtFuncCall , imguiTempFn>(lib,"DockSpace","ImGui::DockSpace")
		->args({"dockspace_id","size","flags","window_class"})
		->arg_init(2,make_smart<ExprConstInt>(0))
		->arg_init(3,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:882:29
	makeExtern< unsigned int (*)(unsigned int,const ImGuiViewport *,int,const ImGuiWindowClass *) , ImGui::DockSpaceOverViewport , SimNode_ExtFuncCall , imguiTempFn>(lib,"DockSpaceOverViewport","ImGui::DockSpaceOverViewport")
		->args({"dockspace_id","viewport","flags","window_class"})
		->arg_init(0,make_smart<ExprConstUInt>(0x0))
		->arg_init(1,make_smart<ExprConstPtr>())
		->arg_init(2,make_smart<ExprConstInt>(0))
		->arg_init(3,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:883:29
	makeExtern< void (*)(unsigned int,int) , ImGui::SetNextWindowDockID , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowDockID","ImGui::SetNextWindowDockID")
		->args({"dock_id","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:884:29
	makeExtern< void (*)(const ImGuiWindowClass *) , ImGui::SetNextWindowClass , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextWindowClass","ImGui::SetNextWindowClass")
		->args({"window_class"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:885:29
	makeExtern< unsigned int (*)() , ImGui::GetWindowDockID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetWindowDockID","ImGui::GetWindowDockID")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:886:29
	makeExtern< bool (*)() , ImGui::IsWindowDocked , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsWindowDocked","ImGui::IsWindowDocked")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:890:29
	makeExtern< void (*)(int) , ImGui::LogToTTY , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToTTY","ImGui::LogToTTY")
		->args({"auto_open_depth"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:891:29
	makeExtern< void (*)(int,const char *) , ImGui::LogToFile , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToFile","ImGui::LogToFile")
		->args({"auto_open_depth","filename"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->arg_init(1,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:892:29
	makeExtern< void (*)(int) , ImGui::LogToClipboard , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToClipboard","ImGui::LogToClipboard")
		->args({"auto_open_depth"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:893:29
	makeExtern< void (*)() , ImGui::LogFinish , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogFinish","ImGui::LogFinish")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:894:29
	makeExtern< void (*)() , ImGui::LogButtons , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogButtons","ImGui::LogButtons")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:903:29
	makeExtern< bool (*)(int) , ImGui::BeginDragDropSource , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDragDropSource","ImGui::BeginDragDropSource")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiDragDropFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiDragDropFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:904:29
	makeExtern< bool (*)(const char *,const void *,uint64_t,int) , ImGui::SetDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetDragDropPayload","ImGui::SetDragDropPayload")
		->args({"type","data","sz","cond"})
		->arg_type(3,makeType<ImGuiCond_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:905:29
	makeExtern< void (*)() , ImGui::EndDragDropSource , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDragDropSource","ImGui::EndDragDropSource")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:906:37
	makeExtern< bool (*)() , ImGui::BeginDragDropTarget , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDragDropTarget","ImGui::BeginDragDropTarget")
		->addToModule(*this, SideEffects::worstDefault);
}
}

