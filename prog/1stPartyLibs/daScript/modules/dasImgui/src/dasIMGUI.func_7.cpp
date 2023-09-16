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
void Module_dasIMGUI::initFunctions_7() {
	makeExtern< unsigned int (*)(const void *) , ImGui::GetID , SimNode_ExtFuncCall , imguiTempFn>(lib,"GetID","ImGui::GetID")
		->args({"ptr_id"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const ImVec2 &) , ImGui::Button , SimNode_ExtFuncCall , imguiTempFn>(lib,"Button","ImGui::Button")
		->args({"label","size"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *) , ImGui::SmallButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"SmallButton","ImGui::SmallButton")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const ImVec2 &,int) , ImGui::InvisibleButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"InvisibleButton","ImGui::InvisibleButton")
		->args({"str_id","size","flags"})
		->arg_type(2,makeType<ImGuiButtonFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiButtonFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int) , ImGui::ArrowButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"ArrowButton","ImGui::ArrowButton")
		->args({"str_id","dir"})
		->arg_type(1,makeType<ImGuiDir_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec4 &,const ImVec4 &) , ImGui::Image , SimNode_ExtFuncCall , imguiTempFn>(lib,"Image","ImGui::Image")
		->args({"user_texture_id","size","uv0","uv1","tint_col","border_col"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,int,const ImVec4 &,const ImVec4 &) , ImGui::ImageButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"ImageButton","ImGui::ImageButton")
		->args({"user_texture_id","size","uv0","uv1","frame_padding","bg_col","tint_col"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool *) , ImGui::Checkbox , SimNode_ExtFuncCall , imguiTempFn>(lib,"Checkbox","ImGui::Checkbox")
		->args({"label","v"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,int) , ImGui::CheckboxFlags , SimNode_ExtFuncCall , imguiTempFn>(lib,"CheckboxFlags","ImGui::CheckboxFlags")
		->args({"label","flags","flags_value"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,unsigned int *,unsigned int) , ImGui::CheckboxFlags , SimNode_ExtFuncCall , imguiTempFn>(lib,"CheckboxFlags","ImGui::CheckboxFlags")
		->args({"label","flags","flags_value"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,bool) , ImGui::RadioButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"RadioButton","ImGui::RadioButton")
		->args({"label","active"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,int) , ImGui::RadioButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"RadioButton","ImGui::RadioButton")
		->args({"label","v","v_button"})
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)(float,const ImVec2 &,const char *) , ImGui::ProgressBar , SimNode_ExtFuncCall , imguiTempFn>(lib,"ProgressBar","ImGui::ProgressBar")
		->args({"fraction","size_arg","overlay"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::Bullet , SimNode_ExtFuncCall , imguiTempFn>(lib,"Bullet","ImGui::Bullet")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,const char *,int) , ImGui::BeginCombo , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginCombo","ImGui::BeginCombo")
		->args({"label","preview_value","flags"})
		->arg_type(2,makeType<ImGuiComboFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiComboFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< void (*)() , ImGui::EndCombo , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndCombo","ImGui::EndCombo")
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,const char *const[],int,int) , ImGui::Combo , SimNode_ExtFuncCall , imguiTempFn>(lib,"Combo","ImGui::Combo")
		->args({"label","current_item","items","items_count","popup_max_height_in_items"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,const char *,int) , ImGui::Combo , SimNode_ExtFuncCall , imguiTempFn>(lib,"Combo","ImGui::Combo")
		->args({"label","current_item","items_separated_by_zeros","popup_max_height_in_items"})
		->arg_init(3,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float *,float,float,float,const char *,int) , ImGui::DragFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat","ImGui::DragFloat")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float[2],float,float,float,const char *,int) , ImGui::DragFloat2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat2","ImGui::DragFloat2")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
}
}

