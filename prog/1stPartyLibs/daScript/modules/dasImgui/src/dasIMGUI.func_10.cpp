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
void Module_dasIMGUI::initFunctions_10() {
	makeExtern< void (*)(int) , ImGui::SetColorEditOptions , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetColorEditOptions","ImGui::SetColorEditOptions")
		->args({"flags"})
		->arg_type(0,makeType<ImGuiColorEditFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *) , ImGui::TreeNode , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreeNode","ImGui::TreeNode")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::TreeNodeEx , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreeNodeEx","ImGui::TreeNodeEx")
		->args({"label","flags"})
		->arg_type(1,makeType<ImGuiTreeNodeFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTreeNodeFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *) , ImGui::TreePush , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreePush","ImGui::TreePush")
		->args({"str_id"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const void *) , ImGui::TreePush , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreePush","ImGui::TreePush")
		->args({"ptr_id"})
		->arg_init(0,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::TreePop , SimNode_ExtFuncCall , imguiTempFn>(lib,"TreePop","ImGui::TreePop")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< float (*)() , ImGui::GetTreeNodeToLabelSpacing , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTreeNodeToLabelSpacing","ImGui::GetTreeNodeToLabelSpacing")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::CollapsingHeader , SimNode_ExtFuncCall , imguiTempFn>(lib,"CollapsingHeader","ImGui::CollapsingHeader")
		->args({"label","flags"})
		->arg_type(1,makeType<ImGuiTreeNodeFlags_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTreeNodeFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool *,int) , ImGui::CollapsingHeader , SimNode_ExtFuncCall , imguiTempFn>(lib,"CollapsingHeader","ImGui::CollapsingHeader")
		->args({"label","p_visible","flags"})
		->arg_type(2,makeType<ImGuiTreeNodeFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiTreeNodeFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(bool,int) , ImGui::SetNextItemOpen , SimNode_ExtFuncCall , imguiTempFn>(lib,"SetNextItemOpen","ImGui::SetNextItemOpen")
		->args({"is_open","cond"})
		->arg_type(1,makeType<ImGuiCond_>(lib))
		->arg_init(1,make_smart<ExprConstEnumeration>(0,makeType<ImGuiCond_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool,int,const ImVec2 &) , ImGui::Selectable , SimNode_ExtFuncCall , imguiTempFn>(lib,"Selectable","ImGui::Selectable")
		->args({"label","selected","flags","size"})
		->arg_init(1,make_smart<ExprConstBool>(false))
		->arg_type(2,makeType<ImGuiSelectableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSelectableFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool *,int,const ImVec2 &) , ImGui::Selectable , SimNode_ExtFuncCall , imguiTempFn>(lib,"Selectable","ImGui::Selectable")
		->args({"label","p_selected","flags","size"})
		->arg_type(2,makeType<ImGuiSelectableFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSelectableFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const ImVec2 &) , ImGui::BeginListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginListBox","ImGui::BeginListBox")
		->args({"label","size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndListBox","ImGui::EndListBox")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,const char *const[],int,int) , ImGui::ListBox , SimNode_ExtFuncCall , imguiTempFn>(lib,"ListBox","ImGui::ListBox")
		->args({"label","current_item","items","items_count","height_in_items"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,const float *,int,int,const char *,float,float,ImVec2,int) , ImGui::PlotLines , SimNode_ExtFuncCall , imguiTempFn>(lib,"PlotLines","ImGui::PlotLines")
		->args({"label","values","values_count","values_offset","overlay_text","scale_min","scale_max","graph_size","stride"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstString>(""))
		->arg_init(5,make_smart<ExprConstFloat>(340282346638528859811704183484516925440.00000000000000000))
		->arg_init(6,make_smart<ExprConstFloat>(340282346638528859811704183484516925440.00000000000000000))
		->arg_init(8,make_smart<ExprConstInt>(4))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,const float *,int,int,const char *,float,float,ImVec2,int) , ImGui::PlotHistogram , SimNode_ExtFuncCall , imguiTempFn>(lib,"PlotHistogram","ImGui::PlotHistogram")
		->args({"label","values","values_count","values_offset","overlay_text","scale_min","scale_max","graph_size","stride"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstString>(""))
		->arg_init(5,make_smart<ExprConstFloat>(340282346638528859811704183484516925440.00000000000000000))
		->arg_init(6,make_smart<ExprConstFloat>(340282346638528859811704183484516925440.00000000000000000))
		->arg_init(8,make_smart<ExprConstInt>(4))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,bool) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","b"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,int) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(const char *,unsigned int) , ImGui::Value , SimNode_ExtFuncCall , imguiTempFn>(lib,"Value","ImGui::Value")
		->args({"prefix","v"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

