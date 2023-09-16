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
void Module_dasIMGUI::initFunctions_6() {
	makeExtern< float (*)() , ImGui::GetCursorPosX , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPosX","ImGui::GetCursorPosX")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetCursorPosY , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPosY","ImGui::GetCursorPosY")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetCursorPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorPos","ImGui::SetCursorPos")
		->args({"local_pos"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::SetCursorPosX , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorPosX","ImGui::SetCursorPosX")
		->args({"local_x"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::SetCursorPosY , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorPosY","ImGui::SetCursorPosY")
		->args({"local_y"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetCursorStartPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorStartPos","ImGui::GetCursorStartPos")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetCursorScreenPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorScreenPos","ImGui::GetCursorScreenPos")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &) , ImGui::SetCursorScreenPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetCursorScreenPos","ImGui::SetCursorScreenPos")
		->args({"pos"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::AlignTextToFramePadding , SimNode_ExtFuncCall , imguiTempFn>(lib,"AlignTextToFramePadding","ImGui::AlignTextToFramePadding")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetTextLineHeight , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTextLineHeight","ImGui::GetTextLineHeight")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetTextLineHeightWithSpacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTextLineHeightWithSpacing","ImGui::GetTextLineHeightWithSpacing")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetFrameHeight , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFrameHeight","ImGui::GetFrameHeight")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetFrameHeightWithSpacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFrameHeightWithSpacing","ImGui::GetFrameHeightWithSpacing")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"str_id"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,const char *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"str_id_begin","str_id_end"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const void *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"ptr_id"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"int_id"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::PopID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopID","ImGui::PopID")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< unsigned int (*)(const char *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"str_id"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< unsigned int (*)(const char *,const char *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"str_id_begin","str_id_end"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

