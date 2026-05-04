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
void Module_dasIMGUI::initFunctions_13() {
// from imgui/imgui.h:912:29
	makeExtern< void (*)() , ImGui::EndTable , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTable","ImGui::EndTable")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:913:29
	makeExtern< void (*)(int,float) , ImGui::TableNextRow , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableNextRow","ImGui::TableNextRow")
		->args({"row_flags","min_row_height"})
		->arg_type(0,makeType<ImGuiTableRowFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTableRowFlags_>(lib)))
		->arg_init(1,make_smart<ExprConstFloat>(0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:914:29
	makeExtern< bool (*)() , ImGui::TableNextColumn , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableNextColumn","ImGui::TableNextColumn")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:915:29
	makeExtern< bool (*)(int) , ImGui::TableSetColumnIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetColumnIndex","ImGui::TableSetColumnIndex")
		->args({"column_n"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:925:29
	makeExtern< void (*)(const char *,int,float,unsigned int) , ImGui::TableSetupColumn , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetupColumn","ImGui::TableSetupColumn")
		->args({"label","flags","init_width_or_weight","user_id"})
		->arg_type(1,makeType<ImGuiTableColumnFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTableColumnFlags_>(lib)))
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->arg_init(3,make_smart<ExprConstUInt>(0x0))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:926:29
	makeExtern< void (*)(int,int) , ImGui::TableSetupScrollFreeze , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetupScrollFreeze","ImGui::TableSetupScrollFreeze")
		->args({"cols","rows"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:927:29
	makeExtern< void (*)(const char *) , ImGui::TableHeader , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableHeader","ImGui::TableHeader")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:928:29
	makeExtern< void (*)() , ImGui::TableHeadersRow , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableHeadersRow","ImGui::TableHeadersRow")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:929:29
	makeExtern< void (*)() , ImGui::TableAngledHeadersRow , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableAngledHeadersRow","ImGui::TableAngledHeadersRow")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:937:37
	makeExtern< ImGuiTableSortSpecs * (*)() , ImGui::TableGetSortSpecs , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetSortSpecs","ImGui::TableGetSortSpecs")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:938:37
	makeExtern< int (*)() , ImGui::TableGetColumnCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnCount","ImGui::TableGetColumnCount")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:939:37
	makeExtern< int (*)() , ImGui::TableGetColumnIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnIndex","ImGui::TableGetColumnIndex")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:940:37
	makeExtern< int (*)() , ImGui::TableGetRowIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetRowIndex","ImGui::TableGetRowIndex")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:941:37
	makeExtern< const char * (*)(int) , ImGui::TableGetColumnName , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnName","ImGui::TableGetColumnName")
		->args({"column_n"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:942:37
	makeExtern< int (*)(int) , ImGui::TableGetColumnFlags , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnFlags","ImGui::TableGetColumnFlags")
		->args({"column_n"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->res_type(makeType<ImGuiTableColumnFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:943:37
	makeExtern< void (*)(int,bool) , ImGui::TableSetColumnEnabled , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetColumnEnabled","ImGui::TableSetColumnEnabled")
		->args({"column_n","v"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:944:37
	makeExtern< int (*)() , ImGui::TableGetHoveredColumn , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetHoveredColumn","ImGui::TableGetHoveredColumn")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:945:37
	makeExtern< void (*)(int,unsigned int,int) , ImGui::TableSetBgColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetBgColor","ImGui::TableSetBgColor")
		->args({"target","color","column_n"})
		->arg_type(0,makeType<ImGuiTableBgTarget_>(lib))
		->arg_init(2,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:949:29
	makeExtern< void (*)(int,const char *,bool) , ImGui::Columns , SimNode_ExtFuncCall , imguiTempFn>(lib,"Columns","ImGui::Columns")
		->args({"count","id","borders"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->arg_init(1,make_smart<ExprConstString>(""))
		->arg_init(2,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:950:29
	makeExtern< void (*)() , ImGui::NextColumn , SimNode_ExtFuncCall , imguiTempFn>(lib,"NextColumn","ImGui::NextColumn")
		->addToModule(*this, SideEffects::worstDefault);
}
}

