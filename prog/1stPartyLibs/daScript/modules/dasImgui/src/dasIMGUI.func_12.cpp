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
void Module_dasIMGUI::initFunctions_12() {
	makeExtern< bool (*)(const char *,int) , ImGui::IsPopupOpen , SimNode_ExtFuncCall , imguiTempFn>(lib,"IsPopupOpen","ImGui::IsPopupOpen")
		->args({"str_id","flags"})
		->arg_type(1,makeType<ImGuiPopupFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiPopupFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int,int,const ImVec2 &,float) , ImGui::BeginTable , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginTable","ImGui::BeginTable")
		->args({"str_id","column","flags","outer_size","inner_width"})
		->arg_type(2,makeType<ImGuiTableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTableFlags_>(lib)))
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndTable , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndTable","ImGui::EndTable")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,float) , ImGui::TableNextRow , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableNextRow","ImGui::TableNextRow")
		->args({"row_flags","min_row_height"})
		->arg_type(0,makeType<ImGuiTableRowFlags_>(lib))
		->arg_init(0,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTableRowFlags_>(lib)))
		->arg_init(1,make_smart<ExprConstFloat>(0.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)() , ImGui::TableNextColumn , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableNextColumn","ImGui::TableNextColumn")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(int) , ImGui::TableSetColumnIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetColumnIndex","ImGui::TableSetColumnIndex")
		->args({"column_n"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,int,float,unsigned int) , ImGui::TableSetupColumn , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetupColumn","ImGui::TableSetupColumn")
		->args({"label","flags","init_width_or_weight","user_id"})
		->arg_type(1,makeType<ImGuiTableColumnFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTableColumnFlags_>(lib)))
		->arg_init(2,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(3,make_smart<ExprConstUInt>(0x0))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,int) , ImGui::TableSetupScrollFreeze , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetupScrollFreeze","ImGui::TableSetupScrollFreeze")
		->args({"cols","rows"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::TableHeadersRow , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableHeadersRow","ImGui::TableHeadersRow")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::TableHeader , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableHeader","ImGui::TableHeader")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< ImGuiTableSortSpecs * (*)() , ImGui::TableGetSortSpecs , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetSortSpecs","ImGui::TableGetSortSpecs")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)() , ImGui::TableGetColumnCount , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnCount","ImGui::TableGetColumnCount")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)() , ImGui::TableGetColumnIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnIndex","ImGui::TableGetColumnIndex")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)() , ImGui::TableGetRowIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetRowIndex","ImGui::TableGetRowIndex")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< const char * (*)(int) , ImGui::TableGetColumnName , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnName","ImGui::TableGetColumnName")
		->args({"column_n"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)(int) , ImGui::TableGetColumnFlags , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableGetColumnFlags","ImGui::TableGetColumnFlags")
		->args({"column_n"})
		->arg_init(0,make_smart<ExprConstInt>(-1))
		->res_type(makeType<ImGuiTableColumnFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,unsigned int,int) , ImGui::TableSetBgColor , SimNode_ExtFuncCall , imguiTempFn>(lib,"TableSetBgColor","ImGui::TableSetBgColor")
		->args({"target","color","column_n"})
		->arg_type(0,makeType<ImGuiTableBgTarget_>(lib))
		->arg_init(2,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(int,const char *,bool) , ImGui::Columns , SimNode_ExtFuncCall , imguiTempFn>(lib,"Columns","ImGui::Columns")
		->args({"count","id","border"})
		->arg_init(0,make_smart<ExprConstInt>(1))
		->arg_init(1,make_smart<ExprConstString>(""))
		->arg_init(2,make_smart<ExprConstBool>(true))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::NextColumn , SimNode_ExtFuncCall , imguiTempFn>(lib,"NextColumn","ImGui::NextColumn")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< int (*)() , ImGui::GetColumnIndex , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetColumnIndex","ImGui::GetColumnIndex")
		->addToModule(*this, SideEffects::worstDefault);
}
}

