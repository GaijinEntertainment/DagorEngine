options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module imgui_boost shared

require imgui public
require daslib/safe_addr
require daslib/static_let
require rtti public
require strings
require math

let IMGUI_PAYLOAD_TYPE_COLOR_3F = "_COL3F"
let IMGUI_PAYLOAD_TYPE_COLOR_4F = "_COL4F"

def to_string(filter : ImGuiTextFilter)
    unsafe
        let pbuf = addr(filter.InputBuf[0])
        return clone_string(reinterpret<string> pbuf)

def set(var filter : ImGuiTextFilter; filterText : string)
    if empty(filterText) || length(filterText) > 255
        return
    for i, ch in iter_range(filterText), filterText
        filter.InputBuf[i] = int8(ch)
    filter.InputBuf[length(filterText)] = int8(0)
    filter |> Build()


// note - this has to correspond to C++ das::DasImguiInputText structure
struct ImGuiInputTextBuffer
    context : Context?
    callback : lambda<(var it : ImGuiInputTextBuffer; var itc : ImGuiInputTextCallbackData) : int>
    buffer : array<uint8>
    at : LineInfo?

def make_input_buffer(var buf : ImGuiInputTextBuffer; txt : string implicit; len : int)
    buf.buffer |> resize(len)
    let bytes = clamp(length(txt), 0, len - 1)
    unsafe
        memcpy(addr(buf.buffer[0]), reinterpret<void?> txt, bytes)

def ImGuiInputTextBuffer(txt : string implicit; len : int)
    var buf : ImGuiInputTextBuffer
    make_input_buffer(buf, txt, len)
    return <- buf

def ImGuiInputTextBuffer(txt : string implicit; len : int; var cb : lambda<(var it : ImGuiInputTextBuffer; var itc : ImGuiInputTextCallbackData) : int>)
    var buf : ImGuiInputTextBuffer
    make_input_buffer(buf, txt, len)
    buf.callback <- cb
    return <- buf

def lock(buf : ImGuiInputTextBuffer; blk : block < (st : string#) : void >)
    unsafe
        let pbuf = addr(buf.buffer[0])
        blk |> invoke(reinterpret<string#> pbuf)

def to_string(buf : ImGuiInputTextBuffer)
    unsafe
        let pbuf = addr(buf.buffer[0])
        return clone_string(reinterpret<string> pbuf)

def clear(var buf : ImGuiInputTextBuffer)
    for t in buf.buffer
        t = uint8(0)

def InputText(lab : string; buf : ImGuiInputTextBuffer; flags : ImGuiInputTextFlags = ImGuiInputTextFlags None)
    return _builtin_InputText(buf, lab, flags)

def InputTextWithHint(lab, hint : string; buf : ImGuiInputTextBuffer; flags : ImGuiInputTextFlags = ImGuiInputTextFlags None)
    return _builtin_InputTextWithHint(buf, lab, hint, flags)

def InputTextMultiline(lab : string; buf : ImGuiInputTextBuffer; siz : float2 = float2(0.0f, 0.0f); flags : ImGuiInputTextFlags = ImGuiInputTextFlags None)
    return _builtin_InputTextMultiline(buf, lab, siz, flags)

// note - this needs to correspond to C++ structure
struct ImGuiSizeConstraints
    context : Context?
    callback : lambda<(var data : ImGuiSizeCallbackData) : void>
    at : LineInfo?

def ImGuiSizeConstraints(var lmb : lambda<(var data : ImGuiSizeCallbackData) : void>)
    var self : ImGuiSizeConstraints
    self.callback <- lmb
    return <- self

def SetNextWindowSizeConstraints(size_min, size_max : float2; var cn : ImGuiSizeConstraints)
    _builtin_SetNextWindowSizeConstraints(cn, size_min, size_max)

// note - this needs to correspond to C++ structure
struct ImGuiComboGetter
    context : Context?
    callback : lambda<(idx : int; var result : string&) : bool>
    at : LineInfo?

def ImGuiComboGetter(var lmb : lambda<(idx : int; var result : string&) : bool>)
    var self : ImGuiComboGetter
    self.callback <- lmb
    return <- self

def Combo(lab : string; current_item : int? implicit; getter : ImGuiComboGetter; items_count : int; popup_max_height_in_items : int = -1)
    return _builtin_Combo(getter, lab, current_item, items_count, popup_max_height_in_items)

// note - this needs to correspond to C++ structure
struct ImGuiPlotGetter
    context : Context?
    callback : lambda<(idx : int) : float>
    at : LineInfo?

def ImGuiPlotGetter(var lmb : lambda<(idx : int) : float>)
    var self : ImGuiPlotGetter
    self.callback <- lmb
    return <- self

def PlotLines(
              lab : string; getter : ImGuiPlotGetter; values_count : int; values_offset : int = 0;
              overlay_text : string = "";
              scale_min : float = FLT_MAX; scale_max : float = FLT_MAX;
              graph_size : float2 = float2())
    let gsize = graph_size
    unsafe
        _builtin_PlotLines(getter, lab, values_count, values_offset, overlay_text, scale_min, scale_max, gsize)

def PlotHistogram(
                  lab : string; getter : ImGuiPlotGetter; values_count : int; values_offset : int = 0;
                  overlay_text : string = "";
                  scale_min : float = FLT_MAX; scale_max : float = FLT_MAX;
                  graph_size : float2 = float2())
    let gsize = graph_size
    unsafe
        _builtin_PlotHistogram(getter, lab, values_count, values_offset, overlay_text, scale_min, scale_max, gsize)

let IM_COL32_WHITE = IM_COL32(255, 255, 255, 255)
let IM_COL32_BLACK = IM_COL32(0, 0, 0, 255)
let IM_COL32_BLACK_TRANS = IM_COL32(0, 0, 0, 0)

def IM_COL32(R, G, B, A : uint)
    return (A << uint(IM_COL32_A_SHIFT)) | (B << uint(IM_COL32_B_SHIFT)) | (G << uint(IM_COL32_G_SHIFT)) | (R << uint(IM_COL32_R_SHIFT))

def IM_COL32(R, G, B, A : int)
    return (uint(A) << uint(IM_COL32_A_SHIFT)) | (uint(B) << uint(IM_COL32_B_SHIFT)) | (uint(G) << uint(IM_COL32_G_SHIFT)) | (uint(R) << uint(IM_COL32_R_SHIFT))

def Combo(lab : string; current_item : int? implicit; items : array<string>; popul_max_heigh_in_items : int = -1)
    let nItems = length(items)
    unsafe
        if nItems != 0
            return imgui::Combo(lab, current_item, addr(items[0]), nItems, popul_max_heigh_in_items)
        else
            return imgui::Combo(lab, current_item, null, 0, popul_max_heigh_in_items)

def Combo(lab : string; current_item : int? implicit; items : string[]; popul_max_heigh_in_items : int = -1)
    unsafe
        return imgui::Combo(lab, current_item, addr(items[0]), length(items), popul_max_heigh_in_items)

def ListBox(lab : string; current_item : int? implicit; items : array<string> implicit; height_in_items : int = -1)
    let nItems = length(items)
    unsafe
        if nItems != 0
            return imgui::ListBox(lab, current_item, addr(items[0]), nItems, height_in_items)
        else
            return imgui::ListBox(lab, current_item, null, 0, height_in_items)

def ListBox(lab : string; current_item : int? implicit; items : string[]; height_in_items : int = -1)
    unsafe
        return imgui::ListBox(lab, current_item, addr(items[0]), length(items), height_in_items)

def PlotLines(
              lab : string; values : float[]; values_offset : int = 0;
              overlay_text : string = "";
              scale_min : float = FLT_MAX; scale_max : float = FLT_MAX;
              graph_size : float2 = float2();
              stride : int = typeinfo(sizeof type<float>))
    var gsize = graph_size
    unsafe
        imgui::PlotLines(lab, addr(values[0]), length(values), values_offset, overlay_text, scale_min, scale_max, gsize, stride)

def PlotLines(
              lab : string; values : array<float>; values_offset : int = 0;
              overlay_text : string = "";
              scale_min : float = FLT_MAX; scale_max : float = FLT_MAX;
              graph_size : float2 = float2();
              stride : int = typeinfo(sizeof type<float>))
    let gsize = graph_size
    unsafe
        imgui::PlotLines(lab, addr(values[0]), length(values), values_offset, overlay_text, scale_min, scale_max, gsize, stride)

def PlotHistogram(
                  lab : string; values : float[]; values_offset : int = 0;
                  overlay_text : string = "";
                  scale_min : float = FLT_MAX; scale_max : float = FLT_MAX;
                  graph_size : float2 = float2();
                  stride : int = typeinfo(sizeof type<float>))
    var gsize = graph_size
    unsafe
        imgui::PlotHistogram(lab, addr(values[0]), length(values), values_offset, overlay_text, scale_min, scale_max, gsize, stride)

def PlotHistogram(
                  lab : string; values : array<float>; values_offset : int = 0;
                  overlay_text : string = "";
                  scale_min : float = FLT_MAX; scale_max : float = FLT_MAX;
                  graph_size : float2 = float2();
                  stride : int = typeinfo(sizeof type<float>))
    let gsize = graph_size
    unsafe
        imgui::PlotHistogram(lab, addr(values[0]), length(values), values_offset, overlay_text, scale_min, scale_max, gsize, stride)

def CheckboxFlags(lab : string; var p_flags : auto(FLAGT)? implicit; flags : FLAGT)
    unsafe
        return imgui::CheckboxFlags(lab,
            reinterpret<int?> p_flags,
            int(flags))

def RadioButton(lab : string; var p_flags : auto(FLAGT)? implicit; flags : FLAGT)
    unsafe
        return imgui::RadioButton(lab,
            reinterpret<int?> p_flags,
            int(flags))

def Begin(name : string; blk : block)
    Begin(name)
    invoke(blk)
    End()

def Begin(name : string; p_open : bool?; blk : block)
    Begin(name, p_open)
    invoke(blk)
    End()

def Begin(name : string; p_open : bool?; flags : ImGuiWindowFlags; blk : block)
    Begin(name, p_open, flags)
    invoke(blk)
    End()

def BeginMainMenuBar(blk : block)
    if BeginMainMenuBar()
        invoke(blk)
        EndMainMenuBar()

def BeginMenuBar(blk : block)
    if BeginMenuBar()
        invoke(blk)
        EndMenuBar()

def BeginMenu(lab : string; blk : block)
    if BeginMenu(lab)
        invoke(blk)
        EndMenu()

def BeginMenu(lab : string; enabled : bool; blk : block)
    if BeginMenu(lab, enabled)
        invoke(blk)
        EndMenu()

def MenuItem(lab : string; blk : block)
    if MenuItem(lab)
        invoke(blk)

def MenuItem(lab, shortcut : string; blk : block)
    if MenuItem(lab, shortcut)
        invoke(blk)

def MenuItem(lab, shortcut : string; selected : bool; blk : block)
    if MenuItem(lab, shortcut, selected)
        invoke(blk)

def MenuItem(lab, shortcut : string; selected, enabled : bool; blk : block)
    if MenuItem(lab, shortcut, selected, enabled)
        invoke(blk)

def MenuItem(lab, shortcut : string; selected : bool?; blk : block)
    if MenuItem(lab, shortcut, selected)
        invoke(blk)

def MenuItem(lab, shortcut : string; selected : bool?; enabled : bool; blk : block)
    if MenuItem(lab, shortcut, selected, enabled)
        invoke(blk)

def ImGuiEx_BeginColumn()
    BeginGroup()

def ImGuiEx_NextColumn()
    EndGroup()
    let size = GetItemRectSize()
    SameLine()
    BeginGroup()
    return size

def ImGuiEx_EndColumn()
    EndGroup()

def Combo(name : string; var ent : auto(EnumT)&; max_height_in_items : int = -1)
    concept_assert(typeinfo(is_enum type<EnumT>), "expecting enum")
    if typeinfo(is_enum type<EnumT>)
        var ti = typeinfo(rtti_typeinfo type<EnumT>)
        var names : array<string>
        for ef in *ti.enumType
            names |> push(ef.name)
        unsafe
            return Combo(name, reinterpret<int?> addr(ent), names, max_height_in_items)
    return false

def ListBox(name : string; var ent : auto(EnumT)&; max_height_in_items : int = -1)
    concept_assert(typeinfo(is_enum type<EnumT>), "expecting enum")
    if typeinfo(is_enum type<EnumT>)
        var ti = typeinfo(rtti_typeinfo type<EnumT>)
        var names : array<string>
        for ef in *ti.enumType
            names |> push(ef.name)
        unsafe
            return ListBox(name, reinterpret<int?> addr(ent), names, max_height_in_items)
    return false

[unused_argument(name)]
def EmbeddedListBox(name : string; var ent : auto(EnumT)&; max_height_in_items : int = -1)
    concept_assert(typeinfo(is_enum type<EnumT>), "expecting enum")
    if typeinfo(is_enum type<EnumT>)
        var ti = typeinfo(rtti_typeinfo type<EnumT>)
        var max_tw = 0.
        for ef in *ti.enumType
            max_tw = max(max_tw, CalcTextSize(ef.name).x)
        for ef in *ti.enumType
            let isSelected = ef.value == int64(int(ent))
            if Selectable("{ef.name}##{name}", isSelected, ImGuiSelectableFlags None, float2(max_tw, 0.))
                unsafe
                    ent = reinterpret<EnumT> (ef.value)
                return true
    return false

[unused_argument(name)]
def EmbeddedListBox(name : string; var ent : string&; var items : array<string>; max_height_in_items : int = -1)
    var max_tw = 0.
    for ef in items
        max_tw = max(max_tw, CalcTextSize(ef).x)
    for ef in items
        let isSelected = ent == ef
        if Selectable("{ent}##{name}", isSelected, ImGuiSelectableFlags None, float2(max_tw, 0.))
            ent = ef
            return true
    return false

def HelpMarker(desc : string; qsign : bool = false)
    if qsign
        SameLine()
        TextDisabled("[?]")
    if IsItemHovered()
        BeginTooltip()
        PushTextWrapPos(GetFontSize() * 35.0f)
        TextUnformatted(desc)
        PopTextWrapPos()
        EndTooltip()

def ImGui_GetItemRect()
    let minR = GetItemRectMin()
    let maxR = GetItemRectMax()
    return float4(minR.x, minR.y, maxR.x, maxR.y)


def ImRect_Expanded(rect : float4&; x, y : float)
    var result = rect
    result.x -= x
    result.y -= y
    result.z += x
    result.w += y
    return result

def GetTL(rect : float4)
    return rect.xy

def GetTR(rect : float4)
    return rect.zy

def GetBL(rect : float4)
    return rect.xw

def GetBR(rect : float4)
    return rect.zw

def VerticalSeparator(name : string; var PANNEL_WIDTH : float&; inverted : float = 1.; min_width : float = 200.; max_width : float = 100500.0)
    let IO & = unsafe(imgui::GetIO())
    PushStyleVar(ImGuiStyleVar ItemSpacing, float2(0.))
    static_let <|
        var resizing_vertical = false
        var resizing_name = ""
    let sz = GetItemRectSize()
    InvisibleButton("##vertical_separator_{name}", float2(10., sz.y))
    if IsItemHovered()
        SetMouseCursor(ImGuiMouseCursor ResizeEW)
        if IsMouseDown(ImGuiMouseButton Left)
            resizing_vertical = true
            resizing_name = name
    if resizing_vertical && resizing_name == name
        SetMouseCursor(ImGuiMouseCursor ResizeEW)
        if IsMouseDragging(ImGuiMouseButton Left)
            PANNEL_WIDTH = clamp(PANNEL_WIDTH + IO.MouseDelta.x * inverted, min_width, max_width)
        elif !IsMouseDown(ImGuiMouseButton Left)
            resizing_vertical = false
    SameLine()
    imgui::PopStyleVar(1)
