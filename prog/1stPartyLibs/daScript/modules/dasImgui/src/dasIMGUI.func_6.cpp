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
void Module_dasIMGUI::initFunctions_6() {
// from imgui/imgui.h:496:29
	makeExtern< void (*)(float,float) , ImGui::SameLine , SimNode_ExtFuncCall , imguiTempFn>(lib,"SameLine","ImGui::SameLine")
		->args({"offset_from_start_x","spacing"})
		->arg_init(0,make_smart<ExprConstFloat>(0))
		->arg_init(1,make_smart<ExprConstFloat>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:497:29
	makeExtern< void (*)() , ImGui::NewLine , SimNode_ExtFuncCall , imguiTempFn>(lib,"NewLine","ImGui::NewLine")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:498:29
	makeExtern< void (*)() , ImGui::Spacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"Spacing","ImGui::Spacing")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:499:29
	makeExtern< void (*)(const ImVec2 &) , ImGui::Dummy , SimNode_ExtFuncCall , imguiTempFn>(lib,"Dummy","ImGui::Dummy")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:500:29
	makeExtern< void (*)(float) , ImGui::Indent , SimNode_ExtFuncCall , imguiTempFn>(lib,"Indent","ImGui::Indent")
		->args({"indent_w"})
		->arg_init(0,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:501:29
	makeExtern< void (*)(float) , ImGui::Unindent , SimNode_ExtFuncCall , imguiTempFn>(lib,"Unindent","ImGui::Unindent")
		->args({"indent_w"})
		->arg_init(0,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:502:29
	makeExtern< void (*)() , ImGui::BeginGroup , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginGroup","ImGui::BeginGroup")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:503:29
	makeExtern< void (*)() , ImGui::EndGroup , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndGroup","ImGui::EndGroup")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:504:29
	makeExtern< void (*)() , ImGui::AlignTextToFramePadding , SimNode_ExtFuncCall , imguiTempFn>(lib,"AlignTextToFramePadding","ImGui::AlignTextToFramePadding")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:505:29
	makeExtern< float (*)() , ImGui::GetTextLineHeight , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTextLineHeight","ImGui::GetTextLineHeight")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:506:29
	makeExtern< float (*)() , ImGui::GetTextLineHeightWithSpacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTextLineHeightWithSpacing","ImGui::GetTextLineHeightWithSpacing")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:507:29
	makeExtern< float (*)() , ImGui::GetFrameHeight , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFrameHeight","ImGui::GetFrameHeight")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:508:29
	makeExtern< float (*)() , ImGui::GetFrameHeightWithSpacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFrameHeightWithSpacing","ImGui::GetFrameHeightWithSpacing")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:521:29
	makeExtern< void (*)(const char *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"str_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:522:29
	makeExtern< void (*)(const char *,const char *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"str_id_begin","str_id_end"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:523:29
	makeExtern< void (*)(const void *) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"ptr_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:524:29
	makeExtern< void (*)(int) , ImGui::PushID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushID","ImGui::PushID")
		->args({"int_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:525:29
	makeExtern< void (*)() , ImGui::PopID , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopID","ImGui::PopID")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:526:29
	makeExtern< unsigned int (*)(const char *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"str_id"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:527:29
	makeExtern< unsigned int (*)(const char *,const char *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"str_id_begin","str_id_end"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

