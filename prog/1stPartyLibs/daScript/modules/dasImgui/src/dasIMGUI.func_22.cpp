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
void Module_dasIMGUI::initFunctions_22() {
	using _method_71 = das::das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int),&ImDrawList::AddConvexPolyFilled >;
	makeExtern<DAS_CALL_METHOD(_method_71), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddConvexPolyFilled","das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int) , &ImDrawList::AddConvexPolyFilled >::invoke")
		->args({"self","points","num_points","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_72 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddBezierCubic >;
	makeExtern<DAS_CALL_METHOD(_method_72), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddBezierCubic","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddBezierCubic >::invoke")
		->args({"self","p1","p2","p3","p4","col","thickness","num_segments"})
		->arg_init(7,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_73 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddBezierQuadratic >;
	makeExtern<DAS_CALL_METHOD(_method_73), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddBezierQuadratic","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddBezierQuadratic >::invoke")
		->args({"self","p1","p2","p3","col","thickness","num_segments"})
		->arg_init(6,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_74 = das::das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::AddImage >;
	makeExtern<DAS_CALL_METHOD(_method_74), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddImage","das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::AddImage >::invoke")
		->args({"self","user_texture_id","p_min","p_max","uv_min","uv_max","col"})
		->arg_init(6,make_smart<ExprConstUInt>(0xffffffff))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_75 = das::das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::AddImageQuad >;
	makeExtern<DAS_CALL_METHOD(_method_75), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddImageQuad","das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::AddImageQuad >::invoke")
		->args({"self","user_texture_id","p1","p2","p3","p4","uv1","uv2","uv3","uv4","col"})
		->arg_init(10,make_smart<ExprConstUInt>(0xffffffff))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_76 = das::das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddImageRounded >;
	makeExtern<DAS_CALL_METHOD(_method_76), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddImageRounded","das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddImageRounded >::invoke")
		->args({"self","user_texture_id","p_min","p_max","uv_min","uv_max","col","rounding","flags"})
		->arg_type(8,makeType<ImDrawFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_77 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PathClear >;
	makeExtern<DAS_CALL_METHOD(_method_77), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathClear","das_call_member< void (ImDrawList::*)() , &ImDrawList::PathClear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_78 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &),&ImDrawList::PathLineTo >;
	makeExtern<DAS_CALL_METHOD(_method_78), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathLineTo","das_call_member< void (ImDrawList::*)(const ImVec2 &) , &ImDrawList::PathLineTo >::invoke")
		->args({"self","pos"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_79 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &),&ImDrawList::PathLineToMergeDuplicate >;
	makeExtern<DAS_CALL_METHOD(_method_79), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathLineToMergeDuplicate","das_call_member< void (ImDrawList::*)(const ImVec2 &) , &ImDrawList::PathLineToMergeDuplicate >::invoke")
		->args({"self","pos"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_80 = das::das_call_member< void (ImDrawList::*)(unsigned int),&ImDrawList::PathFillConvex >;
	makeExtern<DAS_CALL_METHOD(_method_80), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathFillConvex","das_call_member< void (ImDrawList::*)(unsigned int) , &ImDrawList::PathFillConvex >::invoke")
		->args({"self","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_81 = das::das_call_member< void (ImDrawList::*)(unsigned int,int,float),&ImDrawList::PathStroke >;
	makeExtern<DAS_CALL_METHOD(_method_81), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathStroke","das_call_member< void (ImDrawList::*)(unsigned int,int,float) , &ImDrawList::PathStroke >::invoke")
		->args({"self","col","flags","thickness"})
		->arg_type(2,makeType<ImDrawFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->arg_init(3,make_smart<ExprConstFloat>(1.00000000000000000))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_82 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int),&ImDrawList::PathArcTo >;
	makeExtern<DAS_CALL_METHOD(_method_82), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathArcTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int) , &ImDrawList::PathArcTo >::invoke")
		->args({"self","center","radius","a_min","a_max","num_segments"})
		->arg_init(5,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_83 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int),&ImDrawList::PathArcToFast >;
	makeExtern<DAS_CALL_METHOD(_method_83), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathArcToFast","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int) , &ImDrawList::PathArcToFast >::invoke")
		->args({"self","center","radius","a_min_of_12","a_max_of_12"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_84 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,int),&ImDrawList::PathBezierCubicCurveTo >;
	makeExtern<DAS_CALL_METHOD(_method_84), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathBezierCubicCurveTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,int) , &ImDrawList::PathBezierCubicCurveTo >::invoke")
		->args({"self","p2","p3","p4","num_segments"})
		->arg_init(4,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_85 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,int),&ImDrawList::PathBezierQuadraticCurveTo >;
	makeExtern<DAS_CALL_METHOD(_method_85), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathBezierQuadraticCurveTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,int) , &ImDrawList::PathBezierQuadraticCurveTo >::invoke")
		->args({"self","p2","p3","num_segments"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_86 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,int),&ImDrawList::PathRect >;
	makeExtern<DAS_CALL_METHOD(_method_86), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,int) , &ImDrawList::PathRect >::invoke")
		->args({"self","rect_min","rect_max","rounding","flags"})
		->arg_init(3,make_smart<ExprConstFloat>(0.00000000000000000))
		->arg_type(4,makeType<ImDrawFlags_>(lib))
		->arg_init(4,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_87 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::AddDrawCmd >;
	makeExtern<DAS_CALL_METHOD(_method_87), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddDrawCmd","das_call_member< void (ImDrawList::*)() , &ImDrawList::AddDrawCmd >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_88 = das::das_call_member< ImDrawList * (ImDrawList::*)() const,&ImDrawList::CloneOutput >;
	makeExtern<DAS_CALL_METHOD(_method_88), SimNode_ExtFuncCall , imguiTempFn>(lib,"CloneOutput","das_call_member< ImDrawList * (ImDrawList::*)() const , &ImDrawList::CloneOutput >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_89 = das::das_call_member< void (ImDrawList::*)(int),&ImDrawList::ChannelsSplit >;
	makeExtern<DAS_CALL_METHOD(_method_89), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsSplit","das_call_member< void (ImDrawList::*)(int) , &ImDrawList::ChannelsSplit >::invoke")
		->args({"self","count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_90 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::ChannelsMerge >;
	makeExtern<DAS_CALL_METHOD(_method_90), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsMerge","das_call_member< void (ImDrawList::*)() , &ImDrawList::ChannelsMerge >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

