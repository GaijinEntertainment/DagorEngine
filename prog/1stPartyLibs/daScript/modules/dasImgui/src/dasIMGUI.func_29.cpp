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
void Module_dasIMGUI::initFunctions_29() {
	using _method_161 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(unsigned short),&ImFontGlyphRangesBuilder::AddChar >;
// from imgui/imgui.h:3735:21
	makeExtern<DAS_CALL_METHOD(_method_161), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddChar","das_call_member< void (ImFontGlyphRangesBuilder::*)(unsigned short) , &ImFontGlyphRangesBuilder::AddChar >::invoke")
		->args({"self","c"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_162 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(const char *,const char *),&ImFontGlyphRangesBuilder::AddText >;
// from imgui/imgui.h:3736:21
	makeExtern<DAS_CALL_METHOD(_method_162), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddText","das_call_member< void (ImFontGlyphRangesBuilder::*)(const char *,const char *) , &ImFontGlyphRangesBuilder::AddText >::invoke")
		->args({"self","text","text_end"})
		->arg_init(2,make_smart<ExprConstString>(""))
		->addToModule(*this, SideEffects::worstDefault);
	using _method_163 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(const unsigned short *),&ImFontGlyphRangesBuilder::AddRanges >;
// from imgui/imgui.h:3737:21
	makeExtern<DAS_CALL_METHOD(_method_163), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddRanges","das_call_member< void (ImFontGlyphRangesBuilder::*)(const unsigned short *) , &ImFontGlyphRangesBuilder::AddRanges >::invoke")
		->args({"self","ranges"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_164 = das::das_call_member< void (ImFontGlyphRangesBuilder::*)(ImVector<unsigned short> *),&ImFontGlyphRangesBuilder::BuildRanges >;
// from imgui/imgui.h:3738:21
	makeExtern<DAS_CALL_METHOD(_method_164), SimNode_ExtFuncCall , imguiTempFn>(lib,"BuildRanges","das_call_member< void (ImFontGlyphRangesBuilder::*)(ImVector<unsigned short> *) , &ImFontGlyphRangesBuilder::BuildRanges >::invoke")
		->args({"self","out_ranges"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontAtlasRect>(*this,lib,"ImFontAtlasRect","ImFontAtlasRect");
	addCtorAndUsing<ImFontAtlas>(*this,lib,"ImFontAtlas","ImFontAtlas");
	using _method_165 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFont >;
// from imgui/imgui.h:3790:33
	makeExtern<DAS_CALL_METHOD(_method_165), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFont","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFont >::invoke")
		->args({"self","font_cfg"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_166 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFontDefault >;
// from imgui/imgui.h:3791:33
	makeExtern<DAS_CALL_METHOD(_method_166), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontDefault","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFontDefault >::invoke")
		->args({"self","font_cfg"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_167 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFontDefaultVector >;
// from imgui/imgui.h:3792:33
	makeExtern<DAS_CALL_METHOD(_method_167), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontDefaultVector","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFontDefaultVector >::invoke")
		->args({"self","font_cfg"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_168 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFontDefaultBitmap >;
// from imgui/imgui.h:3793:33
	makeExtern<DAS_CALL_METHOD(_method_168), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontDefaultBitmap","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFontDefaultBitmap >::invoke")
		->args({"self","font_cfg"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_169 = das::das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromFileTTF >;
// from imgui/imgui.h:3794:33
	makeExtern<DAS_CALL_METHOD(_method_169), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromFileTTF","das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromFileTTF >::invoke")
		->args({"self","filename","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->arg_init(3,make_smart<ExprConstPtr>())
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_170 = das::das_call_member< ImFont * (ImFontAtlas::*)(void *,int,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryTTF >;
// from imgui/imgui.h:3795:33
	makeExtern<DAS_CALL_METHOD(_method_170), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryTTF","das_call_member< ImFont * (ImFontAtlas::*)(void *,int,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryTTF >::invoke")
		->args({"self","font_data","font_data_size","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_171 = das::das_call_member< ImFont * (ImFontAtlas::*)(const void *,int,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryCompressedTTF >;
// from imgui/imgui.h:3796:33
	makeExtern<DAS_CALL_METHOD(_method_171), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryCompressedTTF","das_call_member< ImFont * (ImFontAtlas::*)(const void *,int,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryCompressedTTF >::invoke")
		->args({"self","compressed_font_data","compressed_font_data_size","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(3,make_smart<ExprConstFloat>(0))
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_172 = das::das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryCompressedBase85TTF >;
// from imgui/imgui.h:3797:33
	makeExtern<DAS_CALL_METHOD(_method_172), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryCompressedBase85TTF","das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryCompressedBase85TTF >::invoke")
		->args({"self","compressed_font_data_base85","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(2,make_smart<ExprConstFloat>(0))
		->arg_init(3,make_smart<ExprConstPtr>())
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_173 = das::das_call_member< void (ImFontAtlas::*)(ImFont *),&ImFontAtlas::RemoveFont >;
// from imgui/imgui.h:3798:33
	makeExtern<DAS_CALL_METHOD(_method_173), SimNode_ExtFuncCall , imguiTempFn>(lib,"RemoveFont","das_call_member< void (ImFontAtlas::*)(ImFont *) , &ImFontAtlas::RemoveFont >::invoke")
		->args({"self","font"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_174 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::Clear >;
// from imgui/imgui.h:3800:33
	makeExtern<DAS_CALL_METHOD(_method_174), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_175 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::CompactCache >;
// from imgui/imgui.h:3801:33
	makeExtern<DAS_CALL_METHOD(_method_175), SimNode_ExtFuncCall , imguiTempFn>(lib,"CompactCache","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::CompactCache >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_176 = das::das_call_member< void (ImFontAtlas::*)(const ImFontLoader *),&ImFontAtlas::SetFontLoader >;
// from imgui/imgui.h:3802:33
	makeExtern<DAS_CALL_METHOD(_method_176), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetFontLoader","das_call_member< void (ImFontAtlas::*)(const ImFontLoader *) , &ImFontAtlas::SetFontLoader >::invoke")
		->args({"self","font_loader"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_177 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearInputData >;
// from imgui/imgui.h:3805:33
	makeExtern<DAS_CALL_METHOD(_method_177), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputData","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearInputData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_178 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearFonts >;
// from imgui/imgui.h:3806:33
	makeExtern<DAS_CALL_METHOD(_method_178), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearFonts","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearFonts >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

