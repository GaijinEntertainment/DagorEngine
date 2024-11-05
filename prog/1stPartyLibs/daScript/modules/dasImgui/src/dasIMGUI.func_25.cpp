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
void Module_dasIMGUI::initFunctions_25() {
	using _method_94 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int,float),&ImDrawList::AddEllipse >;
// from imgui/imgui.h:3191:21
	makeExtern<DAS_CALL_METHOD(_method_94), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddEllipse","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int,float) , &ImDrawList::AddEllipse >::invoke")
		->args({"self","center","radius","col","rot","num_segments","thickness"})
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->arg_init(5,make_smart<ExprConstInt>(0))
		->arg_init(6,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_95 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddEllipseFilled >;
// from imgui/imgui.h:3192:21
	makeExtern<DAS_CALL_METHOD(_method_95), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddEllipseFilled","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddEllipseFilled >::invoke")
		->args({"self","center","radius","col","rot","num_segments"})
		->arg_init(4,make_smart<ExprConstFloat>(0))
		->arg_init(5,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_96 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddBezierCubic >;
// from imgui/imgui.h:3195:21
	makeExtern<DAS_CALL_METHOD(_method_96), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddBezierCubic","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddBezierCubic >::invoke")
		->args({"self","p1","p2","p3","p4","col","thickness","num_segments"})
		->arg_init(7,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_97 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddBezierQuadratic >;
// from imgui/imgui.h:3196:21
	makeExtern<DAS_CALL_METHOD(_method_97), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddBezierQuadratic","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddBezierQuadratic >::invoke")
		->args({"self","p1","p2","p3","col","thickness","num_segments"})
		->arg_init(6,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_98 = das::das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int,int,float),&ImDrawList::AddPolyline >;
// from imgui/imgui.h:3201:21
	makeExtern<DAS_CALL_METHOD(_method_98), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddPolyline","das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int,int,float) , &ImDrawList::AddPolyline >::invoke")
		->args({"self","points","num_points","col","flags","thickness"})
		->arg_type(4,makeType<ImDrawFlags_>(lib))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_99 = das::das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int),&ImDrawList::AddConvexPolyFilled >;
// from imgui/imgui.h:3202:21
	makeExtern<DAS_CALL_METHOD(_method_99), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddConvexPolyFilled","das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int) , &ImDrawList::AddConvexPolyFilled >::invoke")
		->args({"self","points","num_points","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_100 = das::das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int),&ImDrawList::AddConcavePolyFilled >;
// from imgui/imgui.h:3203:21
	makeExtern<DAS_CALL_METHOD(_method_100), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddConcavePolyFilled","das_call_member< void (ImDrawList::*)(const ImVec2 *,int,unsigned int) , &ImDrawList::AddConcavePolyFilled >::invoke")
		->args({"self","points","num_points","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_101 = das::das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::AddImage >;
// from imgui/imgui.h:3209:21
	makeExtern<DAS_CALL_METHOD(_method_101), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddImage","das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::AddImage >::invoke")
		->args({"self","user_texture_id","p_min","p_max","uv_min","uv_max","col"})
		->arg_init(6,make_smart<ExprConstUInt>(0xffffffff))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_102 = das::das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::AddImageQuad >;
// from imgui/imgui.h:3210:21
	makeExtern<DAS_CALL_METHOD(_method_102), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddImageQuad","das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::AddImageQuad >::invoke")
		->args({"self","user_texture_id","p1","p2","p3","p4","uv1","uv2","uv3","uv4","col"})
		->arg_init(10,make_smart<ExprConstUInt>(0xffffffff))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_103 = das::das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddImageRounded >;
// from imgui/imgui.h:3211:21
	makeExtern<DAS_CALL_METHOD(_method_103), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddImageRounded","das_call_member< void (ImDrawList::*)(void *,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddImageRounded >::invoke")
		->args({"self","user_texture_id","p_min","p_max","uv_min","uv_max","col","rounding","flags"})
		->arg_type(8,makeType<ImDrawFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_104 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PathClear >;
// from imgui/imgui.h:3216:21
	makeExtern<DAS_CALL_METHOD(_method_104), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathClear","das_call_member< void (ImDrawList::*)() , &ImDrawList::PathClear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_105 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &),&ImDrawList::PathLineTo >;
// from imgui/imgui.h:3217:21
	makeExtern<DAS_CALL_METHOD(_method_105), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathLineTo","das_call_member< void (ImDrawList::*)(const ImVec2 &) , &ImDrawList::PathLineTo >::invoke")
		->args({"self","pos"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_106 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &),&ImDrawList::PathLineToMergeDuplicate >;
// from imgui/imgui.h:3218:21
	makeExtern<DAS_CALL_METHOD(_method_106), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathLineToMergeDuplicate","das_call_member< void (ImDrawList::*)(const ImVec2 &) , &ImDrawList::PathLineToMergeDuplicate >::invoke")
		->args({"self","pos"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_107 = das::das_call_member< void (ImDrawList::*)(unsigned int),&ImDrawList::PathFillConvex >;
// from imgui/imgui.h:3219:21
	makeExtern<DAS_CALL_METHOD(_method_107), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathFillConvex","das_call_member< void (ImDrawList::*)(unsigned int) , &ImDrawList::PathFillConvex >::invoke")
		->args({"self","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_108 = das::das_call_member< void (ImDrawList::*)(unsigned int),&ImDrawList::PathFillConcave >;
// from imgui/imgui.h:3220:21
	makeExtern<DAS_CALL_METHOD(_method_108), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathFillConcave","das_call_member< void (ImDrawList::*)(unsigned int) , &ImDrawList::PathFillConcave >::invoke")
		->args({"self","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_109 = das::das_call_member< void (ImDrawList::*)(unsigned int,int,float),&ImDrawList::PathStroke >;
// from imgui/imgui.h:3221:21
	makeExtern<DAS_CALL_METHOD(_method_109), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathStroke","das_call_member< void (ImDrawList::*)(unsigned int,int,float) , &ImDrawList::PathStroke >::invoke")
		->args({"self","col","flags","thickness"})
		->arg_type(2,makeType<ImDrawFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->arg_init(3,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_110 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int),&ImDrawList::PathArcTo >;
// from imgui/imgui.h:3222:21
	makeExtern<DAS_CALL_METHOD(_method_110), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathArcTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int) , &ImDrawList::PathArcTo >::invoke")
		->args({"self","center","radius","a_min","a_max","num_segments"})
		->arg_init(5,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_111 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int),&ImDrawList::PathArcToFast >;
// from imgui/imgui.h:3223:21
	makeExtern<DAS_CALL_METHOD(_method_111), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathArcToFast","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int) , &ImDrawList::PathArcToFast >::invoke")
		->args({"self","center","radius","a_min_of_12","a_max_of_12"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_112 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,float,float,int),&ImDrawList::PathEllipticalArcTo >;
// from imgui/imgui.h:3224:21
	makeExtern<DAS_CALL_METHOD(_method_112), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathEllipticalArcTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,float,float,int) , &ImDrawList::PathEllipticalArcTo >::invoke")
		->args({"self","center","radius","rot","a_min","a_max","num_segments"})
		->arg_init(6,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_113 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,int),&ImDrawList::PathBezierCubicCurveTo >;
// from imgui/imgui.h:3225:21
	makeExtern<DAS_CALL_METHOD(_method_113), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathBezierCubicCurveTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,int) , &ImDrawList::PathBezierCubicCurveTo >::invoke")
		->args({"self","p2","p3","p4","num_segments"})
		->arg_init(4,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
}
}

