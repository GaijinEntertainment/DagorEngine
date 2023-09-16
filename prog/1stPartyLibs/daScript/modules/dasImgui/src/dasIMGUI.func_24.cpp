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
void Module_dasIMGUI::initFunctions_24() {
	using _method_110 = das::das_call_member< void (ImDrawData::*)(),&ImDrawData::DeIndexAllBuffers >;
	makeExtern<DAS_CALL_METHOD(_method_110), SimNode_ExtFuncCall , imguiTempFn>(lib,"DeIndexAllBuffers","das_call_member< void (ImDrawData::*)() , &ImDrawData::DeIndexAllBuffers >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_111 = das::das_call_member< void (ImDrawData::*)(const ImVec2 &),&ImDrawData::ScaleClipRects >;
	makeExtern<DAS_CALL_METHOD(_method_111), SimNode_ExtFuncCall , imguiTempFn>(lib,"ScaleClipRects","das_call_member< void (ImDrawData::*)(const ImVec2 &) , &ImDrawData::ScaleClipRects >::invoke")
		->args({"self","fb_scale"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontConfig>(*this,lib,"ImFontConfig","ImFontConfig");
	addCtorAndUsing<ImFontGlyphRangesBuilder>(*this,lib,"ImFontGlyphRangesBuilder","ImFontGlyphRangesBuilder");
	using _method_112 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(),&ImFontGlyphRangesBuilder::Clear >;
	makeExtern<DAS_CALL_METHOD(_method_112), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImFontGlyphRangesBuilder::*)() , &ImFontGlyphRangesBuilder::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_113 = das::das_call_member< bool (ImFontGlyphRangesBuilder::*)(size_t) const,&ImFontGlyphRangesBuilder::GetBit >;
	makeExtern<DAS_CALL_METHOD(_method_113), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetBit","das_call_member< bool (ImFontGlyphRangesBuilder::*)(size_t) const , &ImFontGlyphRangesBuilder::GetBit >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_114 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(size_t),&ImFontGlyphRangesBuilder::SetBit >;
	makeExtern<DAS_CALL_METHOD(_method_114), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetBit","das_call_member< void (ImFontGlyphRangesBuilder::*)(size_t) , &ImFontGlyphRangesBuilder::SetBit >::invoke")
		->args({"self","n"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_115 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(unsigned short),&ImFontGlyphRangesBuilder::AddChar >;
	makeExtern<DAS_CALL_METHOD(_method_115), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddChar","das_call_member< void (ImFontGlyphRangesBuilder::*)(unsigned short) , &ImFontGlyphRangesBuilder::AddChar >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_116 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(const char *,const char *),&ImFontGlyphRangesBuilder::AddText >;
	makeExtern<DAS_CALL_METHOD(_method_116), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddText","das_call_member< void (ImFontGlyphRangesBuilder::*)(const char *,const char *) , &ImFontGlyphRangesBuilder::AddText >::invoke")
		->args({"self","text","text_end"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_117 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(const unsigned short *),&ImFontGlyphRangesBuilder::AddRanges >;
	makeExtern<DAS_CALL_METHOD(_method_117), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRanges","das_call_member< void (ImFontGlyphRangesBuilder::*)(const unsigned short *) , &ImFontGlyphRangesBuilder::AddRanges >::invoke")
		->args({"self","ranges"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_118 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(ImVector<unsigned short> *),&ImFontGlyphRangesBuilder::BuildRanges >;
	makeExtern<DAS_CALL_METHOD(_method_118), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildRanges","das_call_member< void (ImFontGlyphRangesBuilder::*)(ImVector<unsigned short> *) , &ImFontGlyphRangesBuilder::BuildRanges >::invoke")
		->args({"self","out_ranges"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontAtlasCustomRect>(*this,lib,"ImFontAtlasCustomRect","ImFontAtlasCustomRect");
	using _method_119 = das::das_call_member< bool (ImFontAtlasCustomRect::*)() const,&ImFontAtlasCustomRect::IsPacked >;
	makeExtern<DAS_CALL_METHOD(_method_119), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsPacked","das_call_member< bool (ImFontAtlasCustomRect::*)() const , &ImFontAtlasCustomRect::IsPacked >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontAtlas>(*this,lib,"ImFontAtlas","ImFontAtlas");
	using _method_120 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFont >;
	makeExtern<DAS_CALL_METHOD(_method_120), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFont","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFont >::invoke")
		->args({"self","font_cfg"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_121 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFontDefault >;
	makeExtern<DAS_CALL_METHOD(_method_121), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontDefault","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFontDefault >::invoke")
		->args({"self","font_cfg"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_122 = das::das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromFileTTF >;
	makeExtern<DAS_CALL_METHOD(_method_122), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromFileTTF","das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromFileTTF >::invoke")
		->args({"self","filename","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(3,make_smart<ExprConstPtr>())
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_123 = das::das_call_member< ImFont * (ImFontAtlas::*)(void *,int,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryTTF >;
	makeExtern<DAS_CALL_METHOD(_method_123), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryTTF","das_call_member< ImFont * (ImFontAtlas::*)(void *,int,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryTTF >::invoke")
		->args({"self","font_data","font_size","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_124 = das::das_call_member< ImFont * (ImFontAtlas::*)(const void *,int,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryCompressedTTF >;
	makeExtern<DAS_CALL_METHOD(_method_124), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryCompressedTTF","das_call_member< ImFont * (ImFontAtlas::*)(const void *,int,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryCompressedTTF >::invoke")
		->args({"self","compressed_font_data","compressed_font_size","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_125 = das::das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryCompressedBase85TTF >;
	makeExtern<DAS_CALL_METHOD(_method_125), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryCompressedBase85TTF","das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryCompressedBase85TTF >::invoke")
		->args({"self","compressed_font_data_base85","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(3,make_smart<ExprConstPtr>())
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
}
}

