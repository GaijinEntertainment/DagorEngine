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
	using _method_150 = das::das_call_member< bool (ImFontAtlasCustomRect::*)() const,&ImFontAtlasCustomRect::IsPacked >;
// from imgui/imgui.h:3369:10
	makeExtern<DAS_CALL_METHOD(_method_150), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsPacked","das_call_member< bool (ImFontAtlasCustomRect::*)() const , &ImFontAtlasCustomRect::IsPacked >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	addCtorAndUsing<ImFontAtlas>(*this,lib,"ImFontAtlas","ImFontAtlas");
	using _method_151 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFont >;
// from imgui/imgui.h:3402:33
	makeExtern<DAS_CALL_METHOD(_method_151), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFont","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFont >::invoke")
		->args({"self","font_cfg"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_152 = das::das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *),&ImFontAtlas::AddFontDefault >;
// from imgui/imgui.h:3403:33
	makeExtern<DAS_CALL_METHOD(_method_152), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontDefault","das_call_member< ImFont * (ImFontAtlas::*)(const ImFontConfig *) , &ImFontAtlas::AddFontDefault >::invoke")
		->args({"self","font_cfg"})
		->arg_init(1,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_153 = das::das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromFileTTF >;
// from imgui/imgui.h:3404:33
	makeExtern<DAS_CALL_METHOD(_method_153), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromFileTTF","das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromFileTTF >::invoke")
		->args({"self","filename","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(3,make_smart<ExprConstPtr>())
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_154 = das::das_call_member< ImFont * (ImFontAtlas::*)(void *,int,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryTTF >;
// from imgui/imgui.h:3405:33
	makeExtern<DAS_CALL_METHOD(_method_154), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryTTF","das_call_member< ImFont * (ImFontAtlas::*)(void *,int,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryTTF >::invoke")
		->args({"self","font_data","font_data_size","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_155 = das::das_call_member< ImFont * (ImFontAtlas::*)(const void *,int,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryCompressedTTF >;
// from imgui/imgui.h:3406:33
	makeExtern<DAS_CALL_METHOD(_method_155), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryCompressedTTF","das_call_member< ImFont * (ImFontAtlas::*)(const void *,int,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryCompressedTTF >::invoke")
		->args({"self","compressed_font_data","compressed_font_data_size","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->arg_init(5,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_156 = das::das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *),&ImFontAtlas::AddFontFromMemoryCompressedBase85TTF >;
// from imgui/imgui.h:3407:33
	makeExtern<DAS_CALL_METHOD(_method_156), SimNode_ExtFuncCall , imguiTempFn>(lib,"AddFontFromMemoryCompressedBase85TTF","das_call_member< ImFont * (ImFontAtlas::*)(const char *,float,const ImFontConfig *,const unsigned short *) , &ImFontAtlas::AddFontFromMemoryCompressedBase85TTF >::invoke")
		->args({"self","compressed_font_data_base85","size_pixels","font_cfg","glyph_ranges"})
		->arg_init(3,make_smart<ExprConstPtr>())
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_157 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearInputData >;
// from imgui/imgui.h:3408:33
	makeExtern<DAS_CALL_METHOD(_method_157), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearInputData","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearInputData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_158 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearTexData >;
// from imgui/imgui.h:3409:33
	makeExtern<DAS_CALL_METHOD(_method_158), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearTexData","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearTexData >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_159 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::ClearFonts >;
// from imgui/imgui.h:3410:33
	makeExtern<DAS_CALL_METHOD(_method_159), SimNode_ExtFuncCall , imguiTempFn>(lib,"ClearFonts","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::ClearFonts >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_160 = das::das_call_member< void (ImFontAtlas::*)(),&ImFontAtlas::Clear >;
// from imgui/imgui.h:3411:33
	makeExtern<DAS_CALL_METHOD(_method_160), SimNode_ExtFuncCall , imguiTempFn>(lib,"Clear","das_call_member< void (ImFontAtlas::*)() , &ImFontAtlas::Clear >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_161 = das::das_call_member< bool (ImFontAtlas::*)(),&ImFontAtlas::Build >;
// from imgui/imgui.h:3418:33
	makeExtern<DAS_CALL_METHOD(_method_161), SimNode_ExtFuncCall , imguiTempFn>(lib,"Build","das_call_member< bool (ImFontAtlas::*)() , &ImFontAtlas::Build >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_162 = das::das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *),&ImFontAtlas::GetTexDataAsAlpha8 >;
// from imgui/imgui.h:3419:33
	makeExtern<DAS_CALL_METHOD(_method_162), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexDataAsAlpha8","das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *) , &ImFontAtlas::GetTexDataAsAlpha8 >::invoke")
		->args({"self","out_pixels","out_width","out_height","out_bytes_per_pixel"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_163 = das::das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *),&ImFontAtlas::GetTexDataAsRGBA32 >;
// from imgui/imgui.h:3420:33
	makeExtern<DAS_CALL_METHOD(_method_163), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetTexDataAsRGBA32","das_call_member< void (ImFontAtlas::*)(unsigned char **,int *,int *,int *) , &ImFontAtlas::GetTexDataAsRGBA32 >::invoke")
		->args({"self","out_pixels","out_width","out_height","out_bytes_per_pixel"})
		->arg_init(4,make_smart<ExprConstPtr>())
		->addToModule(*this, SideEffects::worstDefault);
	using _method_164 = das::das_call_member< bool (ImFontAtlas::*)() const,&ImFontAtlas::IsBuilt >;
// from imgui/imgui.h:3421:33
	makeExtern<DAS_CALL_METHOD(_method_164), SimNode_ExtFuncCall , imguiTempFn>(lib,"IsBuilt","das_call_member< bool (ImFontAtlas::*)() const , &ImFontAtlas::IsBuilt >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_165 = das::das_call_member< void (ImFontAtlas::*)(void *),&ImFontAtlas::SetTexID >;
// from imgui/imgui.h:3422:33
	makeExtern<DAS_CALL_METHOD(_method_165), SimNode_ExtFuncCall , imguiTempFn>(lib,"SetTexID","das_call_member< void (ImFontAtlas::*)(void *) , &ImFontAtlas::SetTexID >::invoke")
		->args({"self","id"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_166 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesDefault >;
// from imgui/imgui.h:3432:33
	makeExtern<DAS_CALL_METHOD(_method_166), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesDefault","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesDefault >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_167 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesGreek >;
// from imgui/imgui.h:3433:33
	makeExtern<DAS_CALL_METHOD(_method_167), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesGreek","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesGreek >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
	using _method_168 = das::das_call_member< const unsigned short * (ImFontAtlas::*)(),&ImFontAtlas::GetGlyphRangesKorean >;
// from imgui/imgui.h:3434:33
	makeExtern<DAS_CALL_METHOD(_method_168), SimNode_ExtFuncCall , imguiTempFn>(lib,"GetGlyphRangesKorean","das_call_member< const unsigned short * (ImFontAtlas::*)() , &ImFontAtlas::GetGlyphRangesKorean >::invoke")
		->args({"self"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

