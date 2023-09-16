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
void Module_dasIMGUI::initFunctions_8() {
	makeExtern< bool (*)(const char *,float[3],float,float,float,const char *,int) , ImGui::DragFloat3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat3","ImGui::DragFloat3")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float[4],float,float,float,const char *,int) , ImGui::DragFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloat4","ImGui::DragFloat4")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(5,make_smart<ExprConstString>("%.3f"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float *,float *,float,float,float,const char *,const char *,int) , ImGui::DragFloatRange2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragFloatRange2","ImGui::DragFloatRange2")
		->args({"label","v_current_min","v_current_max","v_speed","v_min","v_max","format","format_max","flags"})
		->arg_init(3,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(5,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_init(6,make_smart<ExprConstString>("%.3f"))
		->arg_init(7,make_smart<ExprConstString>(""))
		->arg_type(8,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,float,int,int,const char *,int) , ImGui::DragInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt","ImGui::DragInt")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int[2],float,int,int,const char *,int) , ImGui::DragInt2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt2","ImGui::DragInt2")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int[3],float,int,int,const char *,int) , ImGui::DragInt3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt3","ImGui::DragInt3")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int[4],float,int,int,const char *,int) , ImGui::DragInt4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragInt4","ImGui::DragInt4")
		->args({"label","v","v_speed","v_min","v_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(3,make_smart<ExprConstInt>(0))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstString>("%d"))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,int *,float,int,int,const char *,const char *,int) , ImGui::DragIntRange2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragIntRange2","ImGui::DragIntRange2")
		->args({"label","v_current_min","v_current_max","v_speed","v_min","v_max","format","format_max","flags"})
		->arg_init(3,make_smart<ExprConstFloat>(1.00000000000000000))
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstInt>(0))
		->arg_init(6,make_smart<ExprConstString>("%d"))
		->arg_init(7,make_smart<ExprConstString>(""))
		->arg_type(8,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int,void *,float,const void *,const void *,const char *,int) , ImGui::DragScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragScalar","ImGui::DragScalar")
		->args({"label","data_type","p_data","v_speed","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->arg_init(6,make_smart<ExprConstString>(""))
		->arg_type(7,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(7,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int,void *,int,float,const void *,const void *,const char *,int) , ImGui::DragScalarN , SimNode_ExtFuncCall , imguiTempFn>(lib,"DragScalarN","ImGui::DragScalarN")
		->args({"label","data_type","p_data","components","v_speed","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(5,make_smart<ExprConstPtr>())
		->arg_init(6,make_smart<ExprConstPtr>())
		->arg_init(7,make_smart<ExprConstString>(""))
		->arg_type(8,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float *,float,float,const char *,int) , ImGui::SliderFloat , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat","ImGui::SliderFloat")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float[2],float,float,const char *,int) , ImGui::SliderFloat2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat2","ImGui::SliderFloat2")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float[3],float,float,const char *,int) , ImGui::SliderFloat3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat3","ImGui::SliderFloat3")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float[4],float,float,const char *,int) , ImGui::SliderFloat4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderFloat4","ImGui::SliderFloat4")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%.3f"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,float *,float,float,const char *,int) , ImGui::SliderAngle , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderAngle","ImGui::SliderAngle")
		->args({"label","v_rad","v_degrees_min","v_degrees_max","format","flags"})
		->arg_init(2,make_smart<ExprConstFloat>(-360.00000000000000000))
		->arg_init(3,make_smart<ExprConstFloat>(360.00000000000000000))
		->arg_init(4,make_smart<ExprConstString>("%.0f deg"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int *,int,int,const char *,int) , ImGui::SliderInt , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt","ImGui::SliderInt")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int[2],int,int,const char *,int) , ImGui::SliderInt2 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt2","ImGui::SliderInt2")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int[3],int,int,const char *,int) , ImGui::SliderInt3 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt3","ImGui::SliderInt3")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int[4],int,int,const char *,int) , ImGui::SliderInt4 , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderInt4","ImGui::SliderInt4")
		->args({"label","v","v_min","v_max","format","flags"})
		->arg_init(4,make_smart<ExprConstString>("%d"))
		->arg_type(5,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	makeExtern< bool (*)(const char *,int,void *,const void *,const void *,const char *,int) , ImGui::SliderScalar , SimNode_ExtFuncCall , imguiTempFn>(lib,"SliderScalar","ImGui::SliderScalar")
		->args({"label","data_type","p_data","p_min","p_max","format","flags"})
		->arg_type(1,makeType<ImGuiDataType_>(lib))
		->arg_init(5,make_smart<ExprConstString>(""))
		->arg_type(6,makeType<ImGuiSliderFlags_>(lib))
		->arg_init(6,make_smart<ExprConstEnumeration>(0,makeType<ImGuiSliderFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
}
}

