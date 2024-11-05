#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasIMGUI.h"
#include "need_dasIMGUI.h"
#include "aot_dasIMGUI.h"

namespace das {

    void Text ( const char * txt ) {
        ImGui::Text("%s",txt);
    }
    void LabelText ( const char * lab, const char * txt ) {
        ImGui::LabelText(lab,"%s",txt ? txt : "");
    }
    void TextWrapped ( const char * txt ) {
        ImGui::TextWrapped("%s",txt ? txt : "");
    }
    void TextDisabled ( const char * txt ) {
        ImGui::TextDisabled("%s",txt ? txt : "");
    }
    void TextColored ( const ImVec4 & col, const char * txt ) {
        ImGui::TextColored(col,"%s",txt ? txt : "");
    }
    void LogText ( const char * txt ) {
        ImGui::LogText("%s",txt ? txt : "");
    }
    bool TreeNode ( const char * id, const char * txt ) {
        return ImGui::TreeNode(id,"%s",txt ? txt : "");
    }
    bool TreeNodeEx ( const char * id, ImGuiTreeNodeFlags_ flags, const char * txt ) {
        return ImGui::TreeNodeEx(id,flags,"%s",txt ? txt : "");
    }
    bool TreeNodeEx2 ( const void * id, ImGuiTreeNodeFlags_ flags, const char * txt ) {
        return ImGui::TreeNodeEx(id,flags,"%s",txt ? txt : "");
    }
    void TextUnformatted ( const char * txt ) {
        ImGui::TextUnformatted(txt ? txt : "", nullptr);
    }
    void BulletText ( const char * txt ) {
        ImGui::BulletText("%s",txt ? txt : "");
    }
    void SetTooltip ( const char * txt ) {
        ImGui::SetTooltip("%s",txt ? txt : "");
    }

    struct DasImguiInputText {
        Context *  context;
        TLambda<void,DasImguiInputText *,ImGuiInputTextCallbackData *>    callback;
        TArray<uint8_t> buffer;
        LineInfo *      at;
    };

    int InputTextCallback (ImGuiInputTextCallbackData* data) {
        auto diit = (DasImguiInputText *) data->UserData;
        DAS_VERIFY(diit->context && "context is always specified");
        if ( !diit->callback.capture ) {
            diit->context->throw_error("ImguiTextCallback: missing capture");
        }
        return das_invoke_lambda<int>::invoke<DasImguiInputText *,ImGuiInputTextCallbackData *>(diit->context, diit->at, diit->callback, diit, data);
    }

    bool InputTextMultiline(vec4f vdiit, const char* label, const ImVec2& size, ImGuiInputTextFlags_ flags, LineInfoArg * at, Context * context ) {
        auto diit = cast<DasImguiInputText *>::to(vdiit);
        if ( diit->buffer.size==0 ) {
            builtin_array_resize(diit->buffer, 256, 1, context, at);
        }
        if ( diit->callback.capture ) {
            diit->context = context;
            diit->at = at;
            return ImGui::InputTextMultiline(
                label,
                diit->buffer.data,
                diit->buffer.size,
                size,
                flags,
                &InputTextCallback,
                diit
            );
        } else {
            return ImGui::InputTextMultiline(label, diit->buffer.data, diit->buffer.size, size, flags);
        }
    }

    bool InputText(vec4f vdiit, const char * label, ImGuiInputTextFlags_ flags, LineInfoArg * at, Context * context ) {
        auto diit = cast<DasImguiInputText *>::to(vdiit);
        if ( diit->buffer.size==0 ) {
            builtin_array_resize(diit->buffer, 256, 1, context, at);
        }
        if ( diit->callback.capture ) {
            diit->context = context;
            diit->at = at;
            return ImGui::InputText(
                label,
                diit->buffer.data,
                diit->buffer.size,
                flags,
                &InputTextCallback,
                diit
            );
        } else {
            return ImGui::InputText(label, diit->buffer.data, diit->buffer.size, flags);
        }
    }

    bool InputTextWithHint(vec4f vdiit, const char * label, const char * hint, ImGuiInputTextFlags_ flags, LineInfoArg * at, Context * context ) {
        auto diit = cast<DasImguiInputText *>::to(vdiit);
        if ( diit->buffer.size==0 ) {
            builtin_array_resize(diit->buffer, 256, 1, context, at);
        }
        if ( diit->callback.capture ) {
            diit->context = context;
            diit->at = at;
            return ImGui::InputTextWithHint(
                label,
                hint,
                diit->buffer.data,
                diit->buffer.size,
                flags,
                &InputTextCallback,
                diit
            );
        } else {
            return ImGui::InputTextWithHint(label, hint, diit->buffer.data, diit->buffer.size, flags);
        }
    }

    // ImGui::ImGuiTextFilter::PassFilter

    bool PassFilter ( ImGuiTextFilter & filter, const char* text ) {
        return filter.PassFilter(text, nullptr);
    }

    char * text_range_string( ImGuiTextFilter::ImGuiTextRange & r, das::Context *context, das::LineInfoArg * at ) {
        auto res = context->allocateString(r.b, r.e - r.b, at);
        if (!res)
          context->throw_out_of_memory(true, r.e - r.b, at);
        return res;
    }

    void AddText( ImDrawList & drawList, const ImVec2& pos, ImU32 col, const char* text ) {
        drawList.AddText(pos, col, text);
    }

    void AddText2( ImDrawList & drawList, const ImFont* font, float font_size, const ImVec2& pos, ImU32 col,
        const char* text_begin, float wrap_width, const ImVec4* cpu_fine_clip_rect) {
        drawList.AddText(font,font_size,pos,col,text_begin,nullptr,wrap_width,cpu_fine_clip_rect);
    }

    // ImColor

    ImColor HSV(float h, float s, float v, float a) {
        return ImColor::HSV(h,s,v,a);
    }

    // ImGuiTextBuffer

    void ImGTB_Append ( ImGuiTextBuffer & buf, const char * txt ) {
        buf.append(txt, nullptr);
    }

    int ImGTB_At ( ImGuiTextBuffer & buf, int32_t index ) {
        return buf[index];
    }

    void ImGTB_SetAt ( ImGuiTextBuffer & buf, int32_t index, int32_t value ) {
        buf.Buf[index] = (char) value;
    }

    char * ImGTB_Slice ( ImGuiTextBuffer & buf, int32_t head, int32_t tail, Context * context, LineInfoArg * at ) {
        if ( head>tail ) {
            context->throw_error_at(at, "can't get slice of ImGuiTextBuffer, head > tail");
        }
        int32_t len = tail - head;
        if ( len>buf.size() ) {
            context->throw_error_at(at, "can't get slice of ImGuiTextBuffer, slice too big");
        }
        auto res = context->allocateString(buf.begin() + head,len + 1, at);
        if (!res)
          context->throw_out_of_memory(true, len + 1, at );
        return res;
    }

    // ImGuiInputTextCallbackData

    void InsertChars(ImGuiInputTextCallbackData & data, int pos, const char* text ) {
        data.InsertChars(pos, text);
    }

    // SetNextWindowSizeConstraints

    struct DasImGuiSizeConstraints {
        Context *   context;
        Lambda      lambda;
        LineInfo *  at;
    };

    void SetNextWindowSizeConstraintsCallback ( ImGuiSizeCallbackData* data ) {
        DasImGuiSizeConstraints * temp = (DasImGuiSizeConstraints *) data->UserData;
        if ( !temp->lambda.capture ) {
            temp->context->throw_error_at(temp->at, "expecting lambda");
        }
        das_invoke_lambda<void>::invoke<ImGuiSizeCallbackData*>(temp->context,temp->at,temp->lambda,data);
    }

    void SetNextWindowSizeConstraints ( vec4f snwscc, const ImVec2& size_min, const ImVec2& size_max, Context * context, LineInfoArg * at ) {
        DasImGuiSizeConstraints * temp = cast<DasImGuiSizeConstraints *>::to(snwscc);
        temp->context = context;
        temp->at = at;
        ImGui::SetNextWindowSizeConstraints(size_min, size_max, &SetNextWindowSizeConstraintsCallback, temp);
    }

    void SetNextWindowSizeConstraintsNoCallback ( const ImVec2& size_min, const ImVec2& size_max ) {
        ImGui::SetNextWindowSizeConstraints(size_min, size_max);
    }

    ImGuiSortDirection SortDirection ( const ImGuiTableColumnSortSpecs & specs ) {
        return ImGuiSortDirection(specs.SortDirection);
    }

    ImVec2 CalcTextSize(const char* text,bool hide_text_after_double_hash, float wrap_width) {
        return ImGui::CalcTextSize(text,nullptr,hide_text_after_double_hash,wrap_width);
    }

    // Combo with accessor
    struct ImGuiComboGetter {
        Context *   context;
        Lambda      lambda;
        LineInfo *  at;
    };

    const char *ComboGetterCallback(void* data, int idx) {
        ImGuiComboGetter * getter = (ImGuiComboGetter *) data;
        if ( !getter->lambda.capture ) {
            getter->context->throw_error_at(getter->at, "expecting lambda");
        }
        const char *out_text = nullptr;
        bool res = das_invoke_lambda<bool>::invoke<int,char **>(getter->context,getter->at,getter->lambda,idx,(char **)&out_text);
        G_UNUSED(res);
        if (out_text == nullptr)
          out_text = "";
        return out_text;
    }

    bool Combo ( vec4f cg, const char * label, int * current_item, int items_count, int popup_max_height_in_items, Context * ctx, LineInfoArg * at ) {
        ImGuiComboGetter * getter = cast<ImGuiComboGetter *>::to(cg);
        getter->context = ctx;
        getter->at = at;
        return ImGui::Combo(label,current_item,&ComboGetterCallback,getter,items_count,popup_max_height_in_items);
    }

    // Plot lines or historgrams

    struct ImGuiPlotGetter {
        Context *   context;
        Lambda      lambda;
        LineInfo *  at;
    };

    float PlotLinesCallback ( void* data, int idx ) {
        ImGuiPlotGetter * getter = (ImGuiPlotGetter *) data;
        if ( !getter->lambda.capture ) {
            getter->context->throw_error_at(getter->at, "expecting lambda");
        }
        return  das_invoke_lambda<float>::invoke<int>(getter->context,getter->at,getter->lambda,idx);
    }

    void PlotLines ( vec4f igpg, const char* label, int values_count, int values_offset, const char* overlay_text,
        float scale_min, float scale_max, ImVec2 graph_size, Context * ctx, LineInfoArg * at ) {
        ImGuiPlotGetter * getter = cast<ImGuiPlotGetter *>::to(igpg);
        getter->context = ctx;
        getter->at = at;
        return ImGui::PlotLines(label, &PlotLinesCallback, getter, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size );
    }

    void PlotHistogram ( vec4f igpg, const char* label, int values_count, int values_offset, const char* overlay_text,
        float scale_min, float scale_max, ImVec2 graph_size, Context * ctx, LineInfoArg * at ) {
        ImGuiPlotGetter * getter = cast<ImGuiPlotGetter *>::to(igpg);
        getter->context = ctx;
        getter->at = at;
        return ImGui::PlotHistogram(label, &PlotLinesCallback, getter, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size );
    }

    void Module_dasIMGUI::initAotAlias () {
        addAlias(typeFactory<ImVec2>::make(lib));
        addAlias(typeFactory<ImVec4>::make(lib));
        addAlias(typeFactory<ImColor>::make(lib));
    }

	void Module_dasIMGUI::initMain () {
        addConstant(*this,"IMGUI_VERSION", IMGUI_VERSION);
        // imgui text filter
        addExtern<DAS_BIND_FUN(das::PassFilter), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "PassFilter",
            SideEffects::worstDefault, "das::PassFilter");
        addExtern<DAS_BIND_FUN(das::text_range_string), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "string",
            SideEffects::worstDefault, "das::text_range_string");
        // imcolor
        addExtern<DAS_BIND_FUN(das::HSV), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "HSV",
            SideEffects::none, "das::HSV")
                ->args({"h","s","v","a"})
                    ->arg_init(3,make_smart<ExprConstFloat>(1.0f));
        // imgui draw list
        addExtern<DAS_BIND_FUN(das::AddText), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "AddText",
            SideEffects::worstDefault, "das::AddText");
        addExtern<DAS_BIND_FUN(das::AddText2), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "AddText",
            SideEffects::worstDefault, "das::AddText2")
                ->args({"drawList","font","font_size","pos","col","text","wrap_width","cpu_fine_clip_rect"})
                    ->arg_init(6,make_smart<ExprConstFloat>(0.0f))
                    ->arg_init(7,make_smart<ExprConstPtr>());
        // variadic functions
        addExtern<DAS_BIND_FUN(das::Text), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"Text",
            SideEffects::worstDefault,"das::Text");
        addExtern<DAS_BIND_FUN(das::TextWrapped), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"TextWrapped",
            SideEffects::worstDefault,"das::TextWrapped");
        addExtern<DAS_BIND_FUN(das::TextDisabled), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"TextDisabled",
            SideEffects::worstDefault,"das::TextDisabled");
        addExtern<DAS_BIND_FUN(das::TextColored), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"TextColored",
            SideEffects::worstDefault,"das::TextColored");
        addExtern<DAS_BIND_FUN(das::LabelText), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"LabelText",
            SideEffects::worstDefault,"das::LabelText");
        addExtern<DAS_BIND_FUN(das::LogText), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"LogText",
            SideEffects::worstDefault,"das::LogText");
        addExtern<DAS_BIND_FUN(das::TreeNode), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"TreeNode",
            SideEffects::worstDefault,"das::TreeNode");
        addExtern<DAS_BIND_FUN(das::TreeNodeEx), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"TreeNodeEx",
            SideEffects::worstDefault,"das::TreeNodeEx");
        addExtern<DAS_BIND_FUN(das::TreeNodeEx2), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"TreeNodeEx",
            SideEffects::worstDefault,"das::TreeNodeEx2");
        addExtern<DAS_BIND_FUN(das::BulletText), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"BulletText",
            SideEffects::worstDefault,"das::BulletText");
        addExtern<DAS_BIND_FUN(das::SetTooltip), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"SetTooltip",
            SideEffects::worstDefault,"das::SetTooltip");
        // text unfromatted
        addExtern<DAS_BIND_FUN(das::TextUnformatted), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "TextUnformatted",
            SideEffects::worstDefault, "das::TextUnformatted")
            ->arg("text");
        // input text
        addExtern<DAS_BIND_FUN(das::InputText), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "_builtin_InputText",
            SideEffects::worstDefault, "das::InputText");
        addExtern<DAS_BIND_FUN(das::InputTextWithHint), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "_builtin_InputTextWithHint",
            SideEffects::worstDefault, "das::InputTextWithHint");
        addExtern<DAS_BIND_FUN(das::InputTextMultiline), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "_builtin_InputTextMultiline",
            SideEffects::worstDefault, "das::InputTextMultiline");
        // imgui text buffer
        addExtern<DAS_BIND_FUN(das::ImGTB_Append), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"append",
            SideEffects::worstDefault,"das::ImGTB_Append");
        addExtern<DAS_BIND_FUN(das::ImGTB_At), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"at",          // TODO: do we need to learn to map operator []?
            SideEffects::worstDefault,"das::ImGTB_At");
        addExtern<DAS_BIND_FUN(das::ImGTB_SetAt), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"set_at",   // TODO: do we need to learn to map operator []?
            SideEffects::worstDefault,"das::ImGTB_SetAt");
        addExtern<DAS_BIND_FUN(das::ImGTB_Slice), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"slice",
            SideEffects::worstDefault,"das::ImGTB_Slice");
        // ImGuiInputTextCallbackData
        addExtern<DAS_BIND_FUN(das::InsertChars), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"InsertChars",
            SideEffects::worstDefault,"das::InsertChars");
        // SetNextWindowSizeConstraints
        addExtern<DAS_BIND_FUN(das::SetNextWindowSizeConstraints), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"_builtin_SetNextWindowSizeConstraints",
            SideEffects::worstDefault,"das::SetNextWindowSizeConstraints");
        addExtern<DAS_BIND_FUN(das::SetNextWindowSizeConstraintsNoCallback), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"SetNextWindowSizeConstraints",
            SideEffects::worstDefault,"das::SetNextWindowSizeConstraintsNoCallback")
                ->args({"size_min","size_max"});
        // ImGuiTableColumnSortSpecs
        addExtern<DAS_BIND_FUN(das::SortDirection), SimNode_ExtFuncCall, imguiTempFn>(*this,lib,"SortDirection",
            SideEffects::none,"das::SortDirection");
        // CalcTextSize
        addExtern<DAS_BIND_FUN(das::CalcTextSize), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "CalcTextSize",SideEffects::worstDefault, "das::CalcTextSize")
        ->args({"text","hide_text_after_double_hash","wrap_width"})
            ->arg_init(1,make_smart<ExprConstBool>(false))
            ->arg_init(2,make_smart<ExprConstFloat>(-1.0f));
        // combo
        addExtern<DAS_BIND_FUN(das::Combo), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "_builtin_Combo",
            SideEffects::worstDefault, "das::Combo");
        // plot lines and historgram
        addExtern<DAS_BIND_FUN(das::PlotLines), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "_builtin_PlotLines",
            SideEffects::worstDefault, "das::PlotLines");
        addExtern<DAS_BIND_FUN(das::PlotHistogram), SimNode_ExtFuncCall, imguiTempFn>(*this, lib, "_builtin_PlotHistogram",
            SideEffects::worstDefault, "das::PlotHistogram");
        // additional default values
        findUniqueFunction("AddRect")
            ->arg_init(5, make_smart<ExprConstEnumeration>("RoundCornersAll",makeType<ImDrawFlags_>(lib)));
        findUniqueFunction("AddRectFilled")
            ->arg_init(5, make_smart<ExprConstEnumeration>("RoundCornersAll",makeType<ImDrawFlags_>(lib)));
        findUniqueFunction("BeginTable")
            ->arg_init(3, make_smart<ExprCall>(LineInfo(), "ImVec2"));
        for ( auto & fn : functionsByName[hash64z("Selectable")] ) {
            fn->arg_init(3, make_smart<ExprCall>(LineInfo(), "ImVec2"));
        }
        findUniqueFunction("SetNextWindowPos")
            ->arg_init(2, make_smart<ExprCall>(LineInfo(), "ImVec2"));
        findUniqueFunction("Button")
            ->arg_init(1, make_smart<ExprCall>(LineInfo(), "ImVec2"));
        for ( auto & fn : functionsByName[hash64z("PlotHistogram")] ) {
            if ( fn->arguments.size()==9 ) {
                fn->arg_init(7, make_smart<ExprCall>(LineInfo(), "ImVec2"));
                fn->arg_init(8, make_smart<ExprConstInt>(int32_t(sizeof(float))));
            }
        }
        findUniqueFunction("TableSetupColumn")
            ->arg_init(3, make_smart<ExprConstUInt>(uint32_t(0)));
        findUniqueFunction("BeginListBox")
            ->arg_init(1, make_smart<ExprCall>(LineInfo(), "ImVec2"));
        findUniqueFunction("ColorButton")
            ->arg_init(3, make_smart<ExprCall>(LineInfo(), "ImVec2"));
        // time to fix-up const & ImVec2 and const & ImVec4
        for ( auto & pfn : this->functions.each() ) {
            bool anyString = false;
            for ( auto & arg : pfn->arguments ) {
                if ( arg->type->constant && arg->type->ref && arg->type->dim.size()==0 ) {
                    if ( arg->type->baseType==Type::tFloat2 || arg->type->baseType==Type::tFloat4 ) {
                        arg->type->ref = false;
                    }
                }
                if ( arg->type->isString() && !arg->type->ref ) {
                    anyString = true;
                }
            }
            if ( anyString ) {
                pfn->needStringCast = true;
            }
        }
    }

	ModuleAotType Module_dasIMGUI::aotRequire ( TextWriter & tw ) const {
        // add your stuff here
        tw << "#include \"../modules/dasImgui/src/imgui_stub.h\"\n";
        tw << "#include \"../modules/dasImgui/src/aot_dasIMGUI.h\"\n";
        tw << "#include \"daScript/simulate/bind_enum.h\"\n";
        tw << "#include \"../modules/dasImgui/src/dasIMGUI.enum.decl.cast.inc\"\n";
        // specifying AOT type, in this case direct cpp mode (and not hybrid mode)
        return ModuleAotType::cpp;
    }

}
