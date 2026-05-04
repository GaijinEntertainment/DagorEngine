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
void Module_dasIMGUI::initFunctions_8() {
// from imgui/imgui.h:653:29
	makeExtern< bool (*)(const char *) , ImGui::TextLink , SimNode_ExtFuncCall , imguiTempFn>(lib,"TextLink","ImGui::TextLink")
		->args({"label"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:654:29
	makeExtern< bool (*)(const char *,const char *) , ImGui::TextLinkOpenURL , SimNode_ExtFuncCall , imguiTempFn>(lib,"TextLinkOpenURL","ImGui::TextLinkOpenURL")
		->args({"label","url"})
		->arg_init(1,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:662:29
	makeExtern< void (*)(ImTextureRef,const ImVec2 &,const ImVec2 &,const ImVec2 &) , ImGui::Image , SimNode_ExtFuncCall , imguiTempFn>(lib,"Image","ImGui::Image")
		->args({"tex_ref","image_size","uv0","uv1"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:663:29
	makeExtern< void (*)(ImTextureRef,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec4 &,const ImVec4 &) , ImGui::ImageWithBg , SimNode_ExtFuncCall , imguiTempFn>(lib,"ImageWithBg","ImGui::ImageWithBg")
		->args({"tex_ref","image_size","uv0","uv1","bg_col","tint_col"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:664:29
	makeExtern< bool (*)(const char *,ImTextureRef,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec4 &,const ImVec4 &) , ImGui::ImageButton , SimNode_ExtFuncCall , imguiTempFn>(lib,"ImageButton","ImGui::ImageButton")
		->args({"str_id","tex_ref","image_size","uv0","uv1","bg_col","tint_col"})
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:669:29
	makeExtern< bool (*)(const char *,const char *,int) , ImGui::BeginCombo , SimNode_ExtFuncCall , imguiTempFn>(lib,"BeginCombo","ImGui::BeginCombo")
		->args({"label","preview_value","flags"})
		->arg_type(2,makeType<ImGuiComboFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImGuiComboFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:670:29
	makeExtern< void (*)() , ImGui::EndCombo , SimNode_ExtFuncCall , imguiTempFn>(lib,"EndCombo","ImGui::EndCombo")
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:671:29
	makeExtern< bool (*)(const char *,int *,const char *const[],int,int) , ImGui::Combo , SimNode_ExtFuncCall , imguiTempFn>(lib,"Combo","ImGui::Combo")
		->args({"label","current_item","items","items_count","popup_max_height_in_items"})
		->arg_init(4,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:672:29
	makeExtern< bool (*)(const char *,int *,const char *,int) , ImGui::Combo , SimNode_ExtFuncCall , imguiTempFn>(lib,"Combo","ImGui::Combo")
		->args({"label","current_item","items_separated_by_zeros","popup_max_height_in_items"})
		->arg_init(3,make_smart<ExprConstInt>(-1))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:687:29
	makeExtern< bool (*)(const char *,float *,float,float,float,const char *,int) , ImGui::DragFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat","ImGui::DragFloat")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:688:29
	makeExtern< bool (*)(const char *,float[2],float,float,float,const char *,int) , ImGui::DragFloat2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat2","ImGui::DragFloat2")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:689:29
	makeExtern< bool (*)(const char *,float[3],float,float,float,const char *,int) , ImGui::DragFloat3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat3","ImGui::DragFloat3")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:690:29
	makeExtern< bool (*)(const char *,float[4],float,float,float,const char *,int) , ImGui::DragFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat4","ImGui::DragFloat4")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:691:29
	makeExtern< bool (*)(const char *,float *,float *,float,float,float,const char *,const char *,int) , ImGui::DragFloatRange2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloatRange2","ImGui::DragFloatRange2")
		->args({"label","v_current_min","v_current_max","v_speed","v_min","v_max","format","format_max","flags"})
		->arg_init(3,make_smart<ExprConstFloat>(1))
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->arg_init(5,make_smart<ExprConstFloat>(0))
		->arg_init(6,make_smart<ExprConstString>("%.3f"))
		->arg_init(7,make_smart<ExprConstString>(""))
		->arg_type(8,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:692:29
	makeExtern< bool (*)(const char *,int *,float,int,int,const char *,int) , ImGui::DragInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt","ImGui::DragInt")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:693:29
	makeExtern< bool (*)(const char *,int[2],float,int,int,const char *,int) , ImGui::DragInt2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt2","ImGui::DragInt2")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:694:29
	makeExtern< bool (*)(const char *,int[3],float,int,int,const char *,int) , ImGui::DragInt3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt3","ImGui::DragInt3")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:695:29
	makeExtern< bool (*)(const char *,int[4],float,int,int,const char *,int) , ImGui::DragInt4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt4","ImGui::DragInt4")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:696:29
	makeExtern< bool (*)(const char *,int *,int *,float,int,int,const char *,const char *,int) , ImGui::DragIntRange2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragIntRange2","ImGui::DragIntRange2")
		->args({"label","v_current_min","v_current_max","v_speed","v_min","v_max","format","format_max","flags"})
		->arg_init(3,make_smart<ExprConstFloat>(1))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstInt>(0))
		->arg_init(6,make_smart<ExprConstString>("%d"))
		->arg_init(7,make_smart<ExprConstString>(""))
		->arg_type(8,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
// from imgui/imgui.h:697:29
	makeExtern< bool (*)(const char *,int,void *,float,const void *,const void *,const char *,int) , ImGui::DragScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragScalar","ImGui::DragScalar")
		->args({"label","data_type","p_data","v_speed","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(3,make_smart<ExprConstFloat>(1))
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->arg_init(6,make_smart<ExprConstString>(""))
		->arg_type(7,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(7,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
}
}

