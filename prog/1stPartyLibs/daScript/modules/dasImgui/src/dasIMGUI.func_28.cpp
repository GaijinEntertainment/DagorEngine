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
void Module_dasIMGUI::initFunctions_28() {
	using _method_145 = das::das_call_member< void (ImDrawData::*)(ImDrawList *),&ImDrawData::AddDrawList >;
// from imgui/imgui.h:3575:21
	makeExtern<DAS_CALL_METHOD(_method_145), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddDrawList","das_call_member< void (ImDrawData::*)(ImDrawList *) , &ImDrawData::AddDrawList >::invoke")
		->args({"self","draw_list"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_146 = das::das_call_member< void (ImDrawData::*)(),&ImDrawData::DeIndexAllBuffers >;
// from imgui/imgui.h:3576:21
	makeExtern<DAS_CALL_METHOD(_method_146), SimNode_ExtFuncCall , imguiTempFn>(lib,"DeIndexAllBuffers","das_call_member< void (ImDrawData::*)() , &ImDrawData::DeIndexAllBuffers >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_147 = das::das_call_member< void (ImDrawData::*)(const ImVec2 &),&ImDrawData::ScaleClipRects >;
// from imgui/imgui.h:3577:21
	makeExtern<DAS_CALL_METHOD(_method_147), SimNode_ExtFuncCall , imguiTempFn>(lib,"ScaleClipRects","das_call_member< void (ImDrawData::*)(const ImVec2 &) , &ImDrawData::ScaleClipRects >::invoke")
		->args({"self","fb_scale"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImTextureData>(*this,lib,"ImTextureData","ImTextureData");
	using _method_148 = das::das_call_member< void (ImTextureData::*)(ImTextureFormat,int,int),&ImTextureData::Create >;
// from imgui/imgui.h:3647:25
	makeExtern<DAS_CALL_METHOD(_method_148), SimNode_ExtFuncCall , imguiTempFn>(lib,"Create","das_call_member< void (ImTextureData::*)(ImTextureFormat,int,int) , &ImTextureData::Create >::invoke")
		->args({"self","format","w","h"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_149 = das::das_call_member< void (ImTextureData::*)(),&ImTextureData::DestroyPixels >;
// from imgui/imgui.h:3648:25
	makeExtern<DAS_CALL_METHOD(_method_149), SimNode_ExtFuncCall , imguiTempFn>(lib,"DestroyPixels","das_call_member< void (ImTextureData::*)() , &ImTextureData::DestroyPixels >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_150 = das::das_call_member< void * (ImTextureData::*)(),&ImTextureData::GetPixels >;
// from imgui/imgui.h:3649:25
	makeExtern<DAS_CALL_METHOD(_method_150), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetPixels","das_call_member< void * (ImTextureData::*)() , &ImTextureData::GetPixels >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_151 = das::das_call_member< void * (ImTextureData::*)(int,int),&ImTextureData::GetPixelsAt >;
// from imgui/imgui.h:3650:25
	makeExtern<DAS_CALL_METHOD(_method_151), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetPixelsAt","das_call_member< void * (ImTextureData::*)(int,int) , &ImTextureData::GetPixelsAt >::invoke")
		->args({"self","x","y"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_152 = das::das_call_member< int (ImTextureData::*)() const,&ImTextureData::GetSizeInBytes >;
// from imgui/imgui.h:3651:25
	makeExtern<DAS_CALL_METHOD(_method_152), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetSizeInBytes","das_call_member< int (ImTextureData::*)() const , &ImTextureData::GetSizeInBytes >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_153 = das::das_call_member< int (ImTextureData::*)() const,&ImTextureData::GetPitch >;
// from imgui/imgui.h:3652:25
	makeExtern<DAS_CALL_METHOD(_method_153), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetPitch","das_call_member< int (ImTextureData::*)() const , &ImTextureData::GetPitch >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_154 = das::das_call_member< ImTextureRef (ImTextureData::*)(),&ImTextureData::GetTexRef >;
// from imgui/imgui.h:3653:25
	makeExtern<DAS_CALL_METHOD(_method_154), SimNode_ExtFuncCallAndCopyOrMove , imguiTempFn>(lib,"GetTexRef","das_call_member< ImTextureRef (ImTextureData::*)() , &ImTextureData::GetTexRef >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_155 = das::das_call_member< void * (ImTextureData::*)() const,&ImTextureData::GetTexID >;
// from imgui/imgui.h:3654:25
	makeExtern<DAS_CALL_METHOD(_method_155), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexID","das_call_member< void * (ImTextureData::*)() const , &ImTextureData::GetTexID >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_156 = das::das_call_member< void (ImTextureData::*)(void *),&ImTextureData::SetTexID >;
// from imgui/imgui.h:3659:13
	makeExtern<DAS_CALL_METHOD(_method_156), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetTexID","das_call_member< void (ImTextureData::*)(void *) , &ImTextureData::SetTexID >::invoke")
		->args({"self","tex_id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_157 = das::das_call_member< void (ImTextureData::*)(ImTextureStatus),&ImTextureData::SetStatus >;
// from imgui/imgui.h:3660:13
	makeExtern<DAS_CALL_METHOD(_method_157), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetStatus","das_call_member< void (ImTextureData::*)(ImTextureStatus) , &ImTextureData::SetStatus >::invoke")
		->args({"self","status"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontConfig>(*this,lib,"ImFontConfig","ImFontConfig");
	addCtorAndUsing<ImFontGlyph>(*this,lib,"ImFontGlyph","ImFontGlyph");
	addCtorAndUsing<ImFontGlyphRangesBuilder>(*this,lib,"ImFontGlyphRangesBuilder","ImFontGlyphRangesBuilder");
	using _method_158 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(),&ImFontGlyphRangesBuilder::Clear >;
// from imgui/imgui.h:3732:21
	makeExtern<DAS_CALL_METHOD(_method_158), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImFontGlyphRangesBuilder::*)() , &ImFontGlyphRangesBuilder::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_159 = das::das_call_member< bool (ImFontGlyphRangesBuilder::*)(size_t) const,&ImFontGlyphRangesBuilder::GetBit >;
// from imgui/imgui.h:3733:21
	makeExtern<DAS_CALL_METHOD(_method_159), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBit","das_call_member< bool (ImFontGlyphRangesBuilder::*)(size_t) const , &ImFontGlyphRangesBuilder::GetBit >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_160 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(size_t),&ImFontGlyphRangesBuilder::SetBit >;
// from imgui/imgui.h:3734:21
	makeExtern<DAS_CALL_METHOD(_method_160), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetBit","das_call_member< void (ImFontGlyphRangesBuilder::*)(size_t) , &ImFontGlyphRangesBuilder::SetBit >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

