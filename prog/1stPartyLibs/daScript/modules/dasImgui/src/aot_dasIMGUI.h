#pragma once

namespace das {

    template <> struct das_alias<ImVec2>  : das_alias_vec<ImVec2,float2> {};
    template <> struct das_alias<ImVec4>  : das_alias_vec<ImVec4,float4> {};
    template <> struct das_alias<ImColor> : das_alias_vec<ImColor,float4> {};

    template <typename TT>
    struct das_index<ImVector<TT>> : das_default_vector_index<ImVector<TT>, TT> {};
    template <typename TT>
    struct das_index<ImVector<TT> const> : das_default_vector_index<ImVector<TT>, TT> {};

    template <typename TT>
    struct das_default_vector_size<ImVector<TT>> {
        static __forceinline uint32_t size( const ImVector<TT> & value ) {
            return uint32_t(value.size());
        }
    };

    void Text ( const char * txt );
    void LabelText ( const char * lab, const char * txt );
    void TextWrapped ( const char * txt );
    void TextDisabled ( const char * txt );
    void TextColored ( const ImVec4 & col, const char * txt );
    void LogText ( const char * txt );
    bool TreeNode ( const char * id, const char * txt );
    bool TreeNodeEx ( const char * id, ImGuiTreeNodeFlags_ flags, const char * txt );
    bool TreeNodeEx2 ( const void * id, ImGuiTreeNodeFlags_ flags, const char * txt );
    void TextUnformatted ( const char * txt );
    void BulletText ( const char * txt );
    void SetTooltip ( const char * txt );
    bool InputTextMultiline(vec4f vdiit, const char* label, const ImVec2& size, ImGuiInputTextFlags_ flags, LineInfoArg * at, Context * context );
    bool InputText(vec4f vdiit, const char * label, ImGuiInputTextFlags_ flags, LineInfoArg * at, Context * context );
    bool InputTextWithHint(vec4f vdiit, const char * label, const char * hint, ImGuiInputTextFlags_ flags, LineInfoArg * at, Context * context );
    bool PassFilter ( ImGuiTextFilter & filter, const char* text );
    char* text_range_string(ImGuiTextFilter::ImGuiTextRange &r, das::Context *context, das::LineInfoArg *at);
    void AddText( ImDrawList & drawList, const ImVec2& pos, ImU32 col, const char* text );
    void AddText2( ImDrawList & drawList, const ImFont* font, float font_size, const ImVec2& pos, ImU32 col,
        const char* text_begin, float wrap_width = 0.0f, const ImVec4* cpu_fine_clip_rect = nullptr);
    ImColor HSV(float h, float s, float v, float a = 1.0f);
    void ImGTB_Append ( ImGuiTextBuffer & buf, const char * txt );
    int ImGTB_At ( ImGuiTextBuffer & buf, int32_t index );
    void ImGTB_SetAt ( ImGuiTextBuffer & buf, int32_t index, int32_t value );
    char * ImGTB_Slice ( ImGuiTextBuffer & buf, int32_t head, int32_t tail, Context * context, LineInfoArg * at );
    void InsertChars(ImGuiInputTextCallbackData & data, int pos, const char* text );
    void SetNextWindowSizeConstraints(vec4f snwscc, const ImVec2& size_min, const ImVec2& size_max, Context * context, LineInfoArg * at );
    void SetNextWindowSizeConstraintsNoCallback(const ImVec2& size_min, const ImVec2& size_max);
    ImGuiSortDirection_ SortDirection ( const ImGuiTableColumnSortSpecs & specs );
    ImVec2 CalcTextSize(const char* text,bool hide_text_after_double_hash, float wrap_width);
    bool Combo ( vec4f cg, const char * label, int * current_item, int items_count, int popup_max_height_in_items, Context * ctx, LineInfoArg * at );
    void PlotLines ( vec4f igpg, const char* label, int values_count, int values_offset, const char* overlay_text,
        float scale_min, float scale_max, ImVec2 graph_size, Context * ctx, LineInfoArg * at );
    void PlotHistogram ( vec4f igpg, const char* label, int values_count, int values_offset, const char* overlay_text,
        float scale_min, float scale_max, ImVec2 graph_size, Context * ctx, LineInfoArg * at );
}
