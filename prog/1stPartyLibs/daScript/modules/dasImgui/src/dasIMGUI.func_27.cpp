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
void Module_dasIMGUI::initFunctions_27() {
	using _method_126 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimRect >;
// from imgui/imgui.h:3523:21
	makeExtern<DAS_CALL_METHOD(_method_126), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimRect","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimRect >::invoke")
		->args({"self","a","b","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_127 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimRectUV >;
// from imgui/imgui.h:3524:21
	makeExtern<DAS_CALL_METHOD(_method_127), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimRectUV","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimRectUV >::invoke")
		->args({"self","a","b","uv_a","uv_b","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_128 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimQuadUV >;
// from imgui/imgui.h:3525:21
	makeExtern<DAS_CALL_METHOD(_method_128), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimQuadUV","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimQuadUV >::invoke")
		->args({"self","a","b","c","d","uv_a","uv_b","uv_c","uv_d","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_129 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimWriteVtx >;
// from imgui/imgui.h:3526:21
	makeExtern<DAS_CALL_METHOD(_method_129), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimWriteVtx","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimWriteVtx >::invoke")
		->args({"self","pos","uv","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_130 = das::das_call_member< void (ImDrawList::*)(unsigned short),&ImDrawList::PrimWriteIdx >;
// from imgui/imgui.h:3527:21
	makeExtern<DAS_CALL_METHOD(_method_130), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimWriteIdx","das_call_member< void (ImDrawList::*)(unsigned short) , &ImDrawList::PrimWriteIdx >::invoke")
		->args({"self","idx"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_131 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int),&ImDrawList::PrimVtx >;
// from imgui/imgui.h:3528:21
	makeExtern<DAS_CALL_METHOD(_method_131), SimNode_ExtFuncCall , imguiTempFn>(lib,"PrimVtx","das_call_member< void (ImDrawList::*)(const ImVec2 &,const ImVec2 &,unsigned int) , &ImDrawList::PrimVtx >::invoke")
		->args({"self","pos","uv","col"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_132 = das::das_call_member< void (ImDrawList::*)(ImDrawListSharedData *),&ImDrawList::_SetDrawListSharedData >;
// from imgui/imgui.h:3542:21
	makeExtern<DAS_CALL_METHOD(_method_132), SimNode_ExtFuncCall , imguiTempFn>(lib,"_SetDrawListSharedData","das_call_member< void (ImDrawList::*)(ImDrawListSharedData *) , &ImDrawList::_SetDrawListSharedData >::invoke")
		->args({"self","data"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_133 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_ResetForNewFrame >;
// from imgui/imgui.h:3543:21
	makeExtern<DAS_CALL_METHOD(_method_133), SimNode_ExtFuncCall , imguiTempFn>(lib,"_ResetForNewFrame","das_call_member< void (ImDrawList::*)() , &ImDrawList::_ResetForNewFrame >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_134 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_ClearFreeMemory >;
// from imgui/imgui.h:3544:21
	makeExtern<DAS_CALL_METHOD(_method_134), SimNode_ExtFuncCall , imguiTempFn>(lib,"_ClearFreeMemory","das_call_member< void (ImDrawList::*)() , &ImDrawList::_ClearFreeMemory >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_135 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_PopUnusedDrawCmd >;
// from imgui/imgui.h:3545:21
	makeExtern<DAS_CALL_METHOD(_method_135), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PopUnusedDrawCmd","das_call_member< void (ImDrawList::*)() , &ImDrawList::_PopUnusedDrawCmd >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_136 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_TryMergeDrawCmds >;
// from imgui/imgui.h:3546:21
	makeExtern<DAS_CALL_METHOD(_method_136), SimNode_ExtFuncCall , imguiTempFn>(lib,"_TryMergeDrawCmds","das_call_member< void (ImDrawList::*)() , &ImDrawList::_TryMergeDrawCmds >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_137 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedClipRect >;
// from imgui/imgui.h:3547:21
	makeExtern<DAS_CALL_METHOD(_method_137), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedClipRect","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedClipRect >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_138 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedTexture >;
// from imgui/imgui.h:3548:21
	makeExtern<DAS_CALL_METHOD(_method_138), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedTexture","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedTexture >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_139 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedVtxOffset >;
// from imgui/imgui.h:3549:21
	makeExtern<DAS_CALL_METHOD(_method_139), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedVtxOffset","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedVtxOffset >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_140 = das::das_call_member< void (ImDrawList::*)(ImTextureRef),&ImDrawList::_SetTexture >;
// from imgui/imgui.h:3550:21
	makeExtern<DAS_CALL_METHOD(_method_140), SimNode_ExtFuncCall , imguiTempFn>(lib,"_SetTexture","das_call_member< void (ImDrawList::*)(ImTextureRef) , &ImDrawList::_SetTexture >::invoke")
		->args({"self","tex_ref"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_141 = das::das_call_member< int (ImDrawList::*)(float) const,&ImDrawList::_CalcCircleAutoSegmentCount >;
// from imgui/imgui.h:3551:21
	makeExtern<DAS_CALL_METHOD(_method_141), SimNode_ExtFuncCall , imguiTempFn>(lib,"_CalcCircleAutoSegmentCount","das_call_member< int (ImDrawList::*)(float) const , &ImDrawList::_CalcCircleAutoSegmentCount >::invoke")
		->args({"self","radius"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_142 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int,int),&ImDrawList::_PathArcToFastEx >;
// from imgui/imgui.h:3552:21
	makeExtern<DAS_CALL_METHOD(_method_142), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PathArcToFastEx","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int,int) , &ImDrawList::_PathArcToFastEx >::invoke")
		->args({"self","center","radius","a_min_sample","a_max_sample","a_step"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_143 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int),&ImDrawList::_PathArcToN >;
// from imgui/imgui.h:3553:21
	makeExtern<DAS_CALL_METHOD(_method_143), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PathArcToN","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int) , &ImDrawList::_PathArcToN >::invoke")
		->args({"self","center","radius","a_min","a_max","num_segments"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawData>(*this,lib,"ImDrawData","ImDrawData");
	using _method_144 = das::das_call_member< void (ImDrawData::*)(),&ImDrawData::Clear >;
// from imgui/imgui.h:3574:21
	makeExtern<DAS_CALL_METHOD(_method_144), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImDrawData::*)() , &ImDrawData::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

