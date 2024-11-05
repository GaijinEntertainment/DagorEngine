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
	using _method_134 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedTextureID >;
// from imgui/imgui.h:3269:21
	makeExtern<DAS_CALL_METHOD(_method_134), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedTextureID","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedTextureID >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_135 = das::das_call_member< void (ImDrawList::*)(),&ImDrawList::_OnChangedVtxOffset >;
// from imgui/imgui.h:3270:21
	makeExtern<DAS_CALL_METHOD(_method_135), SimNode_ExtFuncCall , imguiTempFn>(lib,"_OnChangedVtxOffset","das_call_member< void (ImDrawList::*)() , &ImDrawList::_OnChangedVtxOffset >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_136 = das::das_call_member< int (ImDrawList::*)(float) const,&ImDrawList::_CalcCircleAutoSegmentCount >;
// from imgui/imgui.h:3271:21
	makeExtern<DAS_CALL_METHOD(_method_136), SimNode_ExtFuncCall , imguiTempFn>(lib,"_CalcCircleAutoSegmentCount","das_call_member< int (ImDrawList::*)(float) const , &ImDrawList::_CalcCircleAutoSegmentCount >::invoke")
		->args({"self","radius"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_137 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int,int),&ImDrawList::_PathArcToFastEx >;
// from imgui/imgui.h:3272:21
	makeExtern<DAS_CALL_METHOD(_method_137), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PathArcToFastEx","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,int,int,int) , &ImDrawList::_PathArcToFastEx >::invoke")
		->args({"self","center","radius","a_min_sample","a_max_sample","a_step"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_138 = das::das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int),&ImDrawList::_PathArcToN >;
// from imgui/imgui.h:3273:21
	makeExtern<DAS_CALL_METHOD(_method_138), SimNode_ExtFuncCall , imguiTempFn>(lib,"_PathArcToN","das_call_member< void (ImDrawList::*)(const ImVec2 &,float,float,float,int) , &ImDrawList::_PathArcToN >::invoke")
		->args({"self","center","radius","a_min","a_max","num_segments"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImDrawData>(*this,lib,"ImDrawData","ImDrawData");
	using _method_139 = das::das_call_member< void (ImDrawData::*)(),&ImDrawData::Clear >;
// from imgui/imgui.h:3293:21
	makeExtern<DAS_CALL_METHOD(_method_139), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImDrawData::*)() , &ImDrawData::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_140 = das::das_call_member< void (ImDrawData::*)(ImDrawList *),&ImDrawData::AddDrawList >;
// from imgui/imgui.h:3294:21
	makeExtern<DAS_CALL_METHOD(_method_140), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddDrawList","das_call_member< void (ImDrawData::*)(ImDrawList *) , &ImDrawData::AddDrawList >::invoke")
		->args({"self","draw_list"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_141 = das::das_call_member< void (ImDrawData::*)(),&ImDrawData::DeIndexAllBuffers >;
// from imgui/imgui.h:3295:21
	makeExtern<DAS_CALL_METHOD(_method_141), SimNode_ExtFuncCall , imguiTempFn>(lib,"DeIndexAllBuffers","das_call_member< void (ImDrawData::*)() , &ImDrawData::DeIndexAllBuffers >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_142 = das::das_call_member< void (ImDrawData::*)(const ImVec2 &),&ImDrawData::ScaleClipRects >;
// from imgui/imgui.h:3296:21
	makeExtern<DAS_CALL_METHOD(_method_142), SimNode_ExtFuncCall , imguiTempFn>(lib,"ScaleClipRects","das_call_member< void (ImDrawData::*)(const ImVec2 &) , &ImDrawData::ScaleClipRects >::invoke")
		->args({"self","fb_scale"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontConfig>(*this,lib,"ImFontConfig","ImFontConfig");
	addCtorAndUsing<ImFontGlyphRangesBuilder>(*this,lib,"ImFontGlyphRangesBuilder","ImFontGlyphRangesBuilder");
	using _method_143 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(),&ImFontGlyphRangesBuilder::Clear >;
// from imgui/imgui.h:3350:21
	makeExtern<DAS_CALL_METHOD(_method_143), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImFontGlyphRangesBuilder::*)() , &ImFontGlyphRangesBuilder::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_144 = das::das_call_member< bool (ImFontGlyphRangesBuilder::*)(size_t) const,&ImFontGlyphRangesBuilder::GetBit >;
// from imgui/imgui.h:3351:21
	makeExtern<DAS_CALL_METHOD(_method_144), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBit","das_call_member< bool (ImFontGlyphRangesBuilder::*)(size_t) const , &ImFontGlyphRangesBuilder::GetBit >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_145 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(size_t),&ImFontGlyphRangesBuilder::SetBit >;
// from imgui/imgui.h:3352:21
	makeExtern<DAS_CALL_METHOD(_method_145), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetBit","das_call_member< void (ImFontGlyphRangesBuilder::*)(size_t) , &ImFontGlyphRangesBuilder::SetBit >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_146 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(unsigned short),&ImFontGlyphRangesBuilder::AddChar >;
// from imgui/imgui.h:3353:21
	makeExtern<DAS_CALL_METHOD(_method_146), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddChar","das_call_member< void (ImFontGlyphRangesBuilder::*)(unsigned short) , &ImFontGlyphRangesBuilder::AddChar >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_147 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(const char *,const char *),&ImFontGlyphRangesBuilder::AddText >;
// from imgui/imgui.h:3354:21
	makeExtern<DAS_CALL_METHOD(_method_147), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddText","das_call_member< void (ImFontGlyphRangesBuilder::*)(const char *,const char *) , &ImFontGlyphRangesBuilder::AddText >::invoke")
		->args({"self","text","text_end"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_148 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(const unsigned short *),&ImFontGlyphRangesBuilder::AddRanges >;
// from imgui/imgui.h:3355:21
	makeExtern<DAS_CALL_METHOD(_method_148), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRanges","das_call_member< void (ImFontGlyphRangesBuilder::*)(const unsigned short *) , &ImFontGlyphRangesBuilder::AddRanges >::invoke")
		->args({"self","ranges"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_149 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(ImVector<unsigned short> *),&ImFontGlyphRangesBuilder::BuildRanges >;
// from imgui/imgui.h:3356:21
	makeExtern<DAS_CALL_METHOD(_method_149), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildRanges","das_call_member< void (ImFontGlyphRangesBuilder::*)(ImVector<unsigned short> *) , &ImFontGlyphRangesBuilder::BuildRanges >::invoke")
		->args({"self","out_ranges"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontAtlasCustomRect>(*this,lib,"ImFontAtlasCustomRect","ImFontAtlasCustomRect");
}
}

