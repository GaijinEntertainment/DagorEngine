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
void Module_dasIMGUI::initFunctions_15() {
// from imgui/imgui.h:999:29
	makeExtern< void (*)(int) , ImGui::LogToClipboard , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogToClipboard","ImGui::LogToClipboard")
		->args({"auto_open_depth"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1000:29
	makeExtern< void (*)() , ImGui::LogFinish , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogFinish","ImGui::LogFinish")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1001:29
	makeExtern< void (*)() , ImGui::LogButtons , SimNode_ExtFuncCall , imguiTempFn>(lib,"LogButtons","ImGui::LogButtons")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1010:29
	makeExtern< bool (*)(int) , ImGui::BeginDragDropSource , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDragDropSource","ImGui::BeginDragDropSource")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiDragDropFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiDragDropFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1011:29
	makeExtern< bool (*)(const char *,const void *,uint64_t,int) , ImGui::SetDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetDragDropPayload","ImGui::SetDragDropPayload")
		->args({"type","data","sz","cond"})
		->arg_type(3,makeType<ImGuiCond_>(lib))
		->arg_init(3,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1012:29
	makeExtern< void (*)() , ImGui::EndDragDropSource , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDragDropSource","ImGui::EndDragDropSource")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1013:37
	makeExtern< bool (*)() , ImGui::BeginDragDropTarget , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDragDropTarget","ImGui::BeginDragDropTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1014:37
	makeExtern< const ImGuiPayload * (*)(const char *,int) , ImGui::AcceptDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"AcceptDragDropPayload","ImGui::AcceptDragDropPayload")
		->args({"type","flags"})
		->arg_type(1,makeType<ImGuiDragDropFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiDragDropFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1015:37
	makeExtern< void (*)() , ImGui::EndDragDropTarget , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDragDropTarget","ImGui::EndDragDropTarget")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1016:37
	makeExtern< const ImGuiPayload * (*)() , ImGui::GetDragDropPayload , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDragDropPayload","ImGui::GetDragDropPayload")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1023:29
	makeExtern< void (*)(bool) , ImGui::BeginDisabled , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginDisabled","ImGui::BeginDisabled")
		->args({"disabled"})
		->arg_init(0,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1024:29
	makeExtern< void (*)() , ImGui::EndDisabled , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndDisabled","ImGui::EndDisabled")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1028:29
	makeExtern< void (*)(const ImVec2 &,const ImVec2 &,bool) , ImGui::PushClipRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushClipRect","ImGui::PushClipRect")
		->args({"clip_rect_min","clip_rect_max","intersect_with_current_clip_rect"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1029:29
	makeExtern< void (*)() , ImGui::PopClipRect , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopClipRect","ImGui::PopClipRect")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1032:29
	makeExtern< void (*)() , ImGui::SetItemDefaultFocus , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemDefaultFocus","ImGui::SetItemDefaultFocus")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1033:29
	makeExtern< void (*)(int) , ImGui::SetKeyboardFocusHere , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetKeyboardFocusHere","ImGui::SetKeyboardFocusHere")
		->args({"offset"})
		->arg_init(0,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1036:29
	makeExtern< void (*)(bool) , ImGui::SetNavCursorVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNavCursorVisible","ImGui::SetNavCursorVisible")
		->args({"visible"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1039:29
	makeExtern< void (*)() , ImGui::SetNextItemAllowOverlap , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemAllowOverlap","ImGui::SetNextItemAllowOverlap")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1044:29
	makeExtern< bool (*)(int) , ImGui::IsItemHovered , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemHovered","ImGui::IsItemHovered")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiHoveredFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiHoveredFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:1045:29
	makeExtern< bool (*)() , ImGui::IsItemActive , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsItemActive","ImGui::IsItemActive")
		->addToModule(*this, SideEffects::worstDefault);
}
}

