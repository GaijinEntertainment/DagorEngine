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
void Module_dasIMGUI::initFunctions_15() {
	makeExtern< ImVec2 (*)() , ImGui::GetItemRectMin , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetItemRectMin","ImGui::GetItemRectMin")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetItemRectMax , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetItemRectMax","ImGui::GetItemRectMax")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetItemRectSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetItemRectSize","ImGui::GetItemRectSize")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::SetItemAllowOverlap , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetItemAllowOverlap","ImGui::SetItemAllowOverlap")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImGuiViewport * (*)() , ImGui::GetMainViewport , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMainViewport","ImGui::GetMainViewport")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const ImVec2 &) , ImGui::IsRectVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsRectVisible","ImGui::IsRectVisible")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const ImVec2 &,const ImVec2 &) , ImGui::IsRectVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsRectVisible","ImGui::IsRectVisible")
		->args({"rect_min","rect_max"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< double (*)() , ImGui::GetTime , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTime","ImGui::GetTime")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)() , ImGui::GetFrameCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFrameCount","ImGui::GetFrameCount")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImDrawList * (*)() , ImGui::GetBackgroundDrawList , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBackgroundDrawList","ImGui::GetBackgroundDrawList")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImDrawList * (*)() , ImGui::GetForegroundDrawList , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetForegroundDrawList","ImGui::GetForegroundDrawList")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImDrawListSharedData * (*)() , ImGui::GetDrawListSharedData , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDrawListSharedData","ImGui::GetDrawListSharedData")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< const char * (*)(int) , ImGui::GetStyleColorName , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStyleColorName","ImGui::GetStyleColorName")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(ImGuiStorage *) , ImGui::SetStateStorage , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetStateStorage","ImGui::SetStateStorage")
		->args({"storage"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImGuiStorage * (*)() , ImGui::GetStateStorage , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStateStorage","ImGui::GetStateStorage")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,float,int *,int *) , ImGui::CalcListClipping , SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcListClipping","ImGui::CalcListClipping")
		->args({"items_count","items_height","out_items_display_start","out_items_display_end"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(unsigned int,const ImVec2 &,int) , ImGui::BeginChildFrame , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginChildFrame","ImGui::BeginChildFrame")
		->args({"id","size","flags"})
		->arg_type(2,makeType<ImGuiWindowFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiWindowFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndChildFrame , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndChildFrame","ImGui::EndChildFrame")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec4 (*)(unsigned int) , ImGui::ColorConvertU32ToFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertU32ToFloat4","ImGui::ColorConvertU32ToFloat4")
		->args({"in"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< unsigned int (*)(const ImVec4 &) , ImGui::ColorConvertFloat4ToU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertFloat4ToU32","ImGui::ColorConvertFloat4ToU32")
		->args({"in"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

