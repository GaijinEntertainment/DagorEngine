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
void Module_dasIMGUI::initFunctions_16() {
// from imgui/imgui.h:945:29
	makeExtern< bool (*)() , ImGui::IsAnyItemHovered , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyItemHovered","ImGui::IsAnyItemHovered")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:946:29
	makeExtern< bool (*)() , ImGui::IsAnyItemActive , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyItemActive","ImGui::IsAnyItemActive")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:947:29
	makeExtern< bool (*)() , ImGui::IsAnyItemFocused , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsAnyItemFocused","ImGui::IsAnyItemFocused")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:948:29
	makeExtern< unsigned int (*)() , ImGui::GetItemID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetItemID","ImGui::GetItemID")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:949:29
	makeExtern< ImVec2 (*)() , ImGui::GetItemRectMin , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetItemRectMin","ImGui::GetItemRectMin")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:950:29
	makeExtern< ImVec2 (*)() , ImGui::GetItemRectMax , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetItemRectMax","ImGui::GetItemRectMax")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:951:29
	makeExtern< ImVec2 (*)() , ImGui::GetItemRectSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetItemRectSize","ImGui::GetItemRectSize")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:957:30
	makeExtern< ImGuiViewport * (*)() , ImGui::GetMainViewport , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetMainViewport","ImGui::GetMainViewport")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:960:29
	makeExtern< ImDrawList * (*)(ImGuiViewport *) , ImGui::GetBackgroundDrawList , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBackgroundDrawList","ImGui::GetBackgroundDrawList")
		->args({"viewport"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:961:29
	makeExtern< ImDrawList * (*)(ImGuiViewport *) , ImGui::GetForegroundDrawList , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetForegroundDrawList","ImGui::GetForegroundDrawList")
		->args({"viewport"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:964:29
	makeExtern< bool (*)(const ImVec2 &) , ImGui::IsRectVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsRectVisible","ImGui::IsRectVisible")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:965:29
	makeExtern< bool (*)(const ImVec2 &,const ImVec2 &) , ImGui::IsRectVisible , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsRectVisible","ImGui::IsRectVisible")
		->args({"rect_min","rect_max"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:966:29
	makeExtern< double (*)() , ImGui::GetTime , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTime","ImGui::GetTime")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:967:29
	makeExtern< int (*)() , ImGui::GetFrameCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFrameCount","ImGui::GetFrameCount")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:968:37
	makeExtern< ImDrawListSharedData * (*)() , ImGui::GetDrawListSharedData , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetDrawListSharedData","ImGui::GetDrawListSharedData")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:969:29
	makeExtern< const char * (*)(int) , ImGui::GetStyleColorName , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStyleColorName","ImGui::GetStyleColorName")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:970:29
	makeExtern< void (*)(ImGuiStorage *) , ImGui::SetStateStorage , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetStateStorage","ImGui::SetStateStorage")
		->args({"storage"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:971:29
	makeExtern< ImGuiStorage * (*)() , ImGui::GetStateStorage , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetStateStorage","ImGui::GetStateStorage")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:977:29
	makeExtern< ImVec4 (*)(unsigned int) , ImGui::ColorConvertU32ToFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertU32ToFloat4","ImGui::ColorConvertU32ToFloat4")
		->args({"in"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:978:29
	makeExtern< unsigned int (*)(const ImVec4 &) , ImGui::ColorConvertFloat4ToU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"ColorConvertFloat4ToU32","ImGui::ColorConvertFloat4ToU32")
		->args({"in"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

