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
void Module_dasIMGUI::initFunctions_21() {
	using _method_51 = das::das_call_member< void (ImDrawList::*)(ImVec2,ImVec2,bool),&ImDrawList::PushClipRect >;
	makeExtern<DAS_CALL_METHOD(_method_51), SimNode_ExtFuncCall , imguiTempFn>(lib,"PushClipRect","das_call_member< void (ImDrawList::*)(ImVec2,ImVec2,bool) , &ImDrawList::PushClipRect >::invoke")
		->args({"self","clip_rect_min","clip_rect_max","intersect_with_current_clip_rect"})
		->arg_init(3,make_smart<ExprConstBool>(false))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_52 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PushClipRectFullScreen >;
	makeExtern<DAS_CALL_METHOD(_method_52), SimNode_ExtFuncCall , imguiTempFn>(lib,"PushClipRectFullScreen","das_call_member< void (ImDrawList::*)() , &ImDrawList::PushClipRectFullScreen >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_53 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PopClipRect >;
	makeExtern<DAS_CALL_METHOD(_method_53), SimNode_ExtFuncCall , imguiTempFn>(lib,"PopClipRect","das_call_member< void (ImDrawList::*)() , &ImDrawList::PopClipRect >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_54 = das::das_call_member< void (ImDrawList::*)(void *),&ImDrawList::PushTextureID >;
	makeExtern<DAS_CALL_METHOD(_method_54), SimNode_ExtFuncCall , imguiTempFn>(lib,"PushTextureID","das_call_member< void (ImDrawList::*)(void *) , &ImDrawList::PushTextureID >::invoke")
		->args({"self","texture_id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_55 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PopTextureID >;
	makeExtern<DAS_CALL_METHOD(_method_55), SimNode_ExtFuncCall , imguiTempFn>(lib,"PopTextureID","das_call_member< void (ImDrawList::*)() , &ImDrawList::PopTextureID >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_56 = das::das_call_member< ImVec2 (ImDrawList::*)() const,&ImDrawList::GetClipRectMin >;
	makeExtern<DAS_CALL_METHOD(_method_56), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetClipRectMin","das_call_member< ImVec2 (ImDrawList::*)() const , &ImDrawList::GetClipRectMin >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_57 = das::das_call_member< ImVec2 (ImDrawList::*)() const,&ImDrawList::GetClipRectMax >;
	makeExtern<DAS_CALL_METHOD(_method_57), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetClipRectMax","das_call_member< ImVec2 (ImDrawList::*)() const , &ImDrawList::GetClipRectMax >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_58 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float),&ImDrawList::AddLine >;
	makeExtern<DAS_CALL_METHOD(_method_58), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddLine","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float) , &ImDrawList::AddLine >::invoke")
		->args({"self","p1","p2","col","thickness"})
		->arg_init(4,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_59 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int,float),&ImDrawList::AddRect >;
	makeExtern<DAS_CALL_METHOD(_method_59), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int,float) , &ImDrawList::AddRect >::invoke")
		->args({"self","p_min","p_max","col","rounding","flags","thickness"})
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_type(5,makeType<ImDrawFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->arg_init(6,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_60 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddRectFilled >;
	makeExtern<DAS_CALL_METHOD(_method_60), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRectFilled","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddRectFilled >::invoke")
		->args({"self","p_min","p_max","col","rounding","flags"})
		->arg_init(4,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_type(5,makeType<ImDrawFlags_>(lib))
		->arg_init(5,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_61 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,unsigned int,unsigned int,unsigned int),&ImDrawList::AddRectFilledMultiColor >;
	makeExtern<DAS_CALL_METHOD(_method_61), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRectFilledMultiColor","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,unsigned int,unsigned int,unsigned int) , &ImDrawList::AddRectFilledMultiColor >::invoke")
		->args({"self","p_min","p_max","col_upr_left","col_upr_right","col_bot_right","col_bot_left"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_62 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float),&ImDrawList::AddQuad >;
	makeExtern<DAS_CALL_METHOD(_method_62), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddQuad","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float) , &ImDrawList::AddQuad >::invoke")
		->args({"self","p1","p2","p3","p4","col","thickness"})
		->arg_init(6,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_63 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::AddQuadFilled >;
	makeExtern<DAS_CALL_METHOD(_method_63), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddQuadFilled","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::AddQuadFilled >::invoke")
		->args({"self","p1","p2","p3","p4","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_64 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float),&ImDrawList::AddTriangle >;
	makeExtern<DAS_CALL_METHOD(_method_64), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddTriangle","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float) , &ImDrawList::AddTriangle >::invoke")
		->args({"self","p1","p2","p3","col","thickness"})
		->arg_init(5,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_65 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::AddTriangleFilled >;
	makeExtern<DAS_CALL_METHOD(_method_65), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddTriangleFilled","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::AddTriangleFilled >::invoke")
		->args({"self","p1","p2","p3","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_66 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int,float),&ImDrawList::AddCircle >;
	makeExtern<DAS_CALL_METHOD(_method_66), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddCircle","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int,float) , &ImDrawList::AddCircle >::invoke")
		->args({"self","center","radius","col","num_segments","thickness"})
		->arg_init(4,make_smart<ExprConstInt>(0))
		->arg_init(5,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_67 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int),&ImDrawList::AddCircleFilled >;
	makeExtern<DAS_CALL_METHOD(_method_67), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddCircleFilled","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int) , &ImDrawList::AddCircleFilled >::invoke")
		->args({"self","center","radius","col","num_segments"})
		->arg_init(4,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_68 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int,float),&ImDrawList::AddNgon >;
	makeExtern<DAS_CALL_METHOD(_method_68), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddNgon","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int,float) , &ImDrawList::AddNgon >::invoke")
		->args({"self","center","radius","col","num_segments","thickness"})
		->arg_init(5,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_69 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int),&ImDrawList::AddNgonFilled >;
	makeExtern<DAS_CALL_METHOD(_method_69), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddNgonFilled","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,unsigned int,int) , &ImDrawList::AddNgonFilled >::invoke")
		->args({"self","center","radius","col","num_segments"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_70 = das::das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int,int,float),&ImDrawList::AddPolyline >;
	makeExtern<DAS_CALL_METHOD(_method_70), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddPolyline","das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int,int,float) , &ImDrawList::AddPolyline >::invoke")
		->args({"self","points","num_points","col","flags","thickness"})
		->arg_type(4,makeType<ImDrawFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
}
}

