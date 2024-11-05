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
	using _method_114 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,int),&ImDrawList::PathBezierQuadraticCurveTo >;
// from imgui/imgui.h:3226:21
	makeExtern<DAS_CALL_METHOD(_method_114), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathBezierQuadraticCurveTo","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,int) , &ImDrawList::PathBezierQuadraticCurveTo >::invoke")
		->args({"self","p2","p3","num_segments"})
		->arg_init(3,make_smart<ExprConstInt>(0))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_115 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,int),&ImDrawList::PathRect >;
// from imgui/imgui.h:3227:21
	makeExtern<DAS_CALL_METHOD(_method_115), SimNode_ExtFuncCall , imguiTempFn>(lib,"PathRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,float,int) , &ImDrawList::PathRect >::invoke")
		->args({"self","rect_min","rect_max","rounding","flags"})
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_type(4,makeType<ImDrawFlags_>(lib))
		->arg_init(4,make_smart<ExprConstEnumeration>(0,makeType<ImDrawFlags_>(lib)))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_116 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::AddDrawCmd >;
// from imgui/imgui.h:3231:21
	makeExtern<DAS_CALL_METHOD(_method_116), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddDrawCmd","das_call_member< void (ImDrawList::*)() , &ImDrawList::AddDrawCmd >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_117 = das::das_call_member< ImDrawList * (ImDrawList::*)() const,&ImDrawList::CloneOutput >;
// from imgui/imgui.h:3232:27
	makeExtern<DAS_CALL_METHOD(_method_117), SimNode_ExtFuncCall , imguiTempFn>(lib,"CloneOutput","das_call_member< ImDrawList * (ImDrawList::*)() const , &ImDrawList::CloneOutput >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_118 = das::das_call_member< void (ImDrawList::*)(int),&ImDrawList::ChannelsSplit >;
// from imgui/imgui.h:3240:21
	makeExtern<DAS_CALL_METHOD(_method_118), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsSplit","das_call_member< void (ImDrawList::*)(int) , &ImDrawList::ChannelsSplit >::invoke")
		->args({"self","count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_119 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::ChannelsMerge >;
// from imgui/imgui.h:3241:21
	makeExtern<DAS_CALL_METHOD(_method_119), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsMerge","das_call_member< void (ImDrawList::*)() , &ImDrawList::ChannelsMerge >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_120 = das::das_call_member< void (ImDrawList::*)(int),&ImDrawList::ChannelsSetCurrent >;
// from imgui/imgui.h:3242:21
	makeExtern<DAS_CALL_METHOD(_method_120), SimNode_ExtFuncCall , imguiTempFn>(lib,"ChannelsSetCurrent","das_call_member< void (ImDrawList::*)(int) , &ImDrawList::ChannelsSetCurrent >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_121 = das::das_call_member< void (ImDrawList::*)(int,int),&ImDrawList::PrimReserve >;
// from imgui/imgui.h:3247:21
	makeExtern<DAS_CALL_METHOD(_method_121), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimReserve","das_call_member< void (ImDrawList::*)(int,int) , &ImDrawList::PrimReserve >::invoke")
		->args({"self","idx_count","vtx_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_122 = das::das_call_member< void (ImDrawList::*)(int,int),&ImDrawList::PrimUnreserve >;
// from imgui/imgui.h:3248:21
	makeExtern<DAS_CALL_METHOD(_method_122), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimUnreserve","das_call_member< void (ImDrawList::*)(int,int) , &ImDrawList::PrimUnreserve >::invoke")
		->args({"self","idx_count","vtx_count"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_123 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimRect >;
// from imgui/imgui.h:3249:21
	makeExtern<DAS_CALL_METHOD(_method_123), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimRect >::invoke")
		->args({"self","a","b","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_124 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimRectUV >;
// from imgui/imgui.h:3250:21
	makeExtern<DAS_CALL_METHOD(_method_124), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimRectUV","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimRectUV >::invoke")
		->args({"self","a","b","uv_a","uv_b","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_125 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimQuadUV >;
// from imgui/imgui.h:3251:21
	makeExtern<DAS_CALL_METHOD(_method_125), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimQuadUV","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimQuadUV >::invoke")
		->args({"self","a","b","c","d","uv_a","uv_b","uv_c","uv_d","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_126 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimWriteVtx >;
// from imgui/imgui.h:3252:21
	makeExtern<DAS_CALL_METHOD(_method_126), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimWriteVtx","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimWriteVtx >::invoke")
		->args({"self","pos","uv","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_127 = das::das_call_member< void (ImDrawList::*)(unsigned short),&ImDrawList::PrimWriteIdx >;
// from imgui/imgui.h:3253:21
	makeExtern<DAS_CALL_METHOD(_method_127), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimWriteIdx","das_call_member< void (ImDrawList::*)(unsigned short) , &ImDrawList::PrimWriteIdx >::invoke")
		->args({"self","idx"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_128 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimVtx >;
// from imgui/imgui.h:3254:21
	makeExtern<DAS_CALL_METHOD(_method_128), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimVtx","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimVtx >::invoke")
		->args({"self","pos","uv","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_129 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_ResetForNewFrame >;
// from imgui/imgui.h:3264:21
	makeExtern<DAS_CALL_METHOD(_method_129), SimNode_ExtFuncCall , imguiTempFn>(lib,"_ResetForNewFrame","das_call_member< void (ImDrawList::*)() , &ImDrawList::_ResetForNewFrame >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_130 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_ClearFreeMemory >;
// from imgui/imgui.h:3265:21
	makeExtern<DAS_CALL_METHOD(_method_130), SimNode_ExtFuncCall , imguiTempFn>(lib,"_ClearFreeMemory","das_call_member< void (ImDrawList::*)() , &ImDrawList::_ClearFreeMemory >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_131 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_PopUnusedDrawCmd >;
// from imgui/imgui.h:3266:21
	makeExtern<DAS_CALL_METHOD(_method_131), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PopUnusedDrawCmd","das_call_member< void (ImDrawList::*)() , &ImDrawList::_PopUnusedDrawCmd >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_132 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_TryMergeDrawCmds >;
// from imgui/imgui.h:3267:21
	makeExtern<DAS_CALL_METHOD(_method_132), SimNode_ExtFuncCall , imguiTempFn>(lib,"_TryMergeDrawCmds","das_call_member< void (ImDrawList::*)() , &ImDrawList::_TryMergeDrawCmds >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_133 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedClipRect >;
// from imgui/imgui.h:3268:21
	makeExtern<DAS_CALL_METHOD(_method_133), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedClipRect","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedClipRect >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

