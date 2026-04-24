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
void Module_dasIMGUI::initFunctions_26() {
	using _method_106 = das::das_call_member< void (ImDrawList::*)(ImTextureRef,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int),&ImDrawList::AddImageRounded >;
// from imgui/imgui.h:3475:21
	makeExtern<DAS_CALL_METHOD(_method_106), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddImageRounded","das_call_member< void (ImDrawList::*)(ImTextureRef,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int,float,int) , &ImDrawList::AddImageRounded >::invoke")
		->args({"self","tex_ref","p_min","p_max","uv_min","uv_max","col","rounding","flags"})
		->arg_type(8,makeType<ImDrawFlags_>(lib))
		->arg_init(8,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_107 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::PathClear >;
// from imgui/imgui.h:3480:21
	makeExtern<DAS_CALL_METHOD(_method_107), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathClear","das_call_member< void (ImDrawList::*)() , &ImDrawList::PathClear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_108 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &),&ImDrawList::PathLineTo >;
// from imgui/imgui.h:3481:21
	makeExtern<DAS_CALL_METHOD(_method_108), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathLineTo","das_call_member< void (ImDrawList::*)(const ImVec2 &) , &ImDrawList::PathLineTo >::invoke")
		->args({"self","pos"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_109 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &),&ImDrawList::PathLineToMergeDuplicate >;
// from imgui/imgui.h:3482:21
	makeExtern<DAS_CALL_METHOD(_method_109), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathLineToMergeDuplicate","das_call_member< void (ImDrawList::*)(const ImVec2 &) , &ImDrawList::PathLineToMergeDuplicate >::invoke")
		->args({"self","pos"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_110 = das::das_call_member< void (ImDrawList::*)(unsigned int),&ImDrawList::PathFillConvex >;
// from imgui/imgui.h:3483:21
	makeExtern<DAS_CALL_METHOD(_method_110), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathFillConvex","das_call_member< void (ImDrawList::*)(unsigned int) , &ImDrawList::PathFillConvex >::invoke")
		->args({"self","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_111 = das::das_call_member< void (ImDrawList::*)(unsigned int),&ImDrawList::PathFillConcave >;
// from imgui/imgui.h:3484:21
	makeExtern<DAS_CALL_METHOD(_method_111), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathFillConcave","das_call_member< void (ImDrawList::*)(unsigned int) , &ImDrawList::PathFillConcave >::invoke")
		->args({"self","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_112 = das::das_call_member< void (ImDrawList::*)(unsigned int,int,float),&ImDrawList::PathStroke >;
// from imgui/imgui.h:3485:21
	makeExtern<DAS_CALL_METHOD(_method_112), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathStroke","das_call_member< void (ImDrawList::*)(unsigned int,int,float) , &ImDrawList::PathStroke >::invoke")
		->args({"self","col","flags","thickness"})
		->arg_type(2,makeType<ImDrawFlags_>(lib))
		->arg_init(2,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->arg_init(3,make_smart<ExprConstFloat>(1))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_113 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int),&ImDrawList::PathArcTo >;
// from imgui/imgui.h:3486:21
	makeExtern<DAS_CALL_METHOD(_method_113), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathArcTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int) , &ImDrawList::PathArcTo >::invoke")
		->args({"self","center","radius","a_min","a_max","num_segments"})
		->arg_init(5,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_114 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int),&ImDrawList::PathArcToFast >;
// from imgui/imgui.h:3487:21
	makeExtern<DAS_CALL_METHOD(_method_114), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathArcToFast","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int) , &ImDrawList::PathArcToFast >::invoke")
		->args({"self","center","radius","a_min_of_12","a_max_of_12"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_115 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,float,float,int),&ImDrawList::PathEllipticalArcTo >;
// from imgui/imgui.h:3488:21
	makeExtern<DAS_CALL_METHOD(_method_115), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathEllipticalArcTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,float,float,int) , &ImDrawList::PathEllipticalArcTo >::invoke")
		->args({"self","center","radius","rot","a_min","a_max","num_segments"})
		->arg_init(6,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_116 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,int),&ImDrawList::PathBezierCubicCurveTo >;
// from imgui/imgui.h:3489:21
	makeExtern<DAS_CALL_METHOD(_method_116), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathBezierCubicCurveTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,int) , &ImDrawList::PathBezierCubicCurveTo >::invoke")
		->args({"self","p2","p3","p4","num_segments"})
		->arg_init(4,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_117 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,int),&ImDrawList::PathBezierQuadraticCurveTo >;
// from imgui/imgui.h:3490:21
	makeExtern<DAS_CALL_METHOD(_method_117), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathBezierQuadraticCurveTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,int) , &ImDrawList::PathBezierQuadraticCurveTo >::invoke")
		->args({"self","p2","p3","num_segments"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_118 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,int),&ImDrawList::PathRect >;
// from imgui/imgui.h:3491:21
	makeExtern<DAS_CALL_METHOD(_method_118), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,int) , &ImDrawList::PathRect >::invoke")
		->args({"self","rect_min","rect_max","rounding","flags"})
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_type(4,makeType<ImDrawFlags_>(lib))
		->arg_init(4,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_119 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::AddDrawCmd >;
// from imgui/imgui.h:3505:21
	makeExtern<DAS_CALL_METHOD(_method_119), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddDrawCmd","das_call_member< void (ImDrawList::*)() , &ImDrawList::AddDrawCmd >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_120 = das::das_call_member< ImDrawList * (ImDrawList::*)() const,&ImDrawList::CloneOutput >;
// from imgui/imgui.h:3506:27
	makeExtern<DAS_CALL_METHOD(_method_120), SimNode_ExtFuncCall , imguiTempFn>(lib,"CloneOutput","das_call_member< ImDrawList * (ImDrawList::*)() const , &ImDrawList::CloneOutput >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_121 = das::das_call_member< void (ImDrawList::*)(int),&ImDrawList::ChannelsSplit >;
// from imgui/imgui.h:3514:21
	makeExtern<DAS_CALL_METHOD(_method_121), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsSplit","das_call_member< void (ImDrawList::*)(int) , &ImDrawList::ChannelsSplit >::invoke")
		->args({"self","count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_122 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::ChannelsMerge >;
// from imgui/imgui.h:3515:21
	makeExtern<DAS_CALL_METHOD(_method_122), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsMerge","das_call_member< void (ImDrawList::*)() , &ImDrawList::ChannelsMerge >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_123 = das::das_call_member< void (ImDrawList::*)(int),&ImDrawList::ChannelsSetCurrent >;
// from imgui/imgui.h:3516:21
	makeExtern<DAS_CALL_METHOD(_method_123), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsSetCurrent","das_call_member< void (ImDrawList::*)(int) , &ImDrawList::ChannelsSetCurrent >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_124 = das::das_call_member< void (ImDrawList::*)(int,int),&ImDrawList::PrimReserve >;
// from imgui/imgui.h:3521:21
	makeExtern<DAS_CALL_METHOD(_method_124), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimReserve","das_call_member< void (ImDrawList::*)(int,int) , &ImDrawList::PrimReserve >::invoke")
		->args({"self","idx_count","vtx_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_125 = das::das_call_member< void (ImDrawList::*)(int,int),&ImDrawList::PrimUnreserve >;
// from imgui/imgui.h:3522:21
	makeExtern<DAS_CALL_METHOD(_method_125), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimUnreserve","das_call_member< void (ImDrawList::*)(int,int) , &ImDrawList::PrimUnreserve >::invoke")
		->args({"self","idx_count","vtx_count"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

