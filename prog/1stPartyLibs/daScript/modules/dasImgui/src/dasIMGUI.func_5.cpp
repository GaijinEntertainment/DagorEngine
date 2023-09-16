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
void Module_dasIMGUI::initFunctions_5() {
	makeExtern< float (*)() , ImGui::CalcItemWidth , SimNode_ExtFuncCall , imguiTempFn>(lib,"CalcItemWidth","ImGui::CalcItemWidth")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::PushTextWrapPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"PushTextWrapPos","ImGui::PushTextWrapPos")
		->args({"wrap_local_pos_x"})
		->arg_init(0,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::PopTextWrapPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"PopTextWrapPos","ImGui::PopTextWrapPos")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImFont * (*)() , ImGui::GetFont , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFont","ImGui::GetFont")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetFontSize , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontSize","ImGui::GetFontSize")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetFontTexUvWhitePixel , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetFontTexUvWhitePixel","ImGui::GetFontTexUvWhitePixel")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< unsigned int (*)(int,float) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"idx","alpha_mul"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->arg_init(1,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< unsigned int (*)(const ImVec4 &) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"col"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< unsigned int (*)(unsigned int) , ImGui::GetColorU32 , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColorU32","ImGui::GetColorU32")
		->args({"col"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< const ImVec4 & (*)(int) , ImGui::GetStyleColorVec4 , SimNode_ExtFuncCallRef , imguiTempFn>(lib,"GetStyleColorVec4","ImGui::GetStyleColorVec4")
		->args({"idx"})
		->arg_type(0,makeType<ImGuiCol_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::Separator , SimNode_ExtFuncCall , imguiTempFn>(lib,"Separator","ImGui::Separator")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float,float) , ImGui::SameLine , SimNode_ExtFuncCall , imguiTempFn>(lib,"SameLine","ImGui::SameLine")
		->args({"offset_from_start_x","spacing"})
		->arg_init(0,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(1,make_smart<ExprConstFloat>(-1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::NewLine , SimNode_ExtFuncCall , imguiTempFn>(lib,"NewLine","ImGui::NewLine")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::Spacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"Spacing","ImGui::Spacing")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const ImVec2 &) , ImGui::Dummy , SimNode_ExtFuncCall , imguiTempFn>(lib,"Dummy","ImGui::Dummy")
		->args({"size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::Indent , SimNode_ExtFuncCall , imguiTempFn>(lib,"Indent","ImGui::Indent")
		->args({"indent_w"})
		->arg_init(0,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float) , ImGui::Unindent , SimNode_ExtFuncCall , imguiTempFn>(lib,"Unindent","ImGui::Unindent")
		->args({"indent_w"})
		->arg_init(0,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::BeginGroup , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginGroup","ImGui::BeginGroup")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndGroup , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndGroup","ImGui::EndGroup")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImVec2 (*)() , ImGui::GetCursorPos , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetCursorPos","ImGui::GetCursorPos")
		->addToModule(*this, SideEffects::worstDefault);
}
}

