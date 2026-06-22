options gen2
require cbind/cbind_boost
require daslib/fio
require daslib/defer
require daslib/strings_boost
require daslib/clargs

[CommandLineArgs]
struct Config {
    @clarg_short = "i"
    @clarg_doc = "Path to imgui headers dir (default: <das_root>/modules/dasImgui/imgui/)"
    input : string = "{get_das_root()}/modules/dasImgui/imgui/"

    @clarg_short = "o"
    @clarg_doc = "Output folder (default: <das_root>/modules/dasImgui/src)"
    output : string = "{get_das_root()}/modules/dasImgui/src"

    @clarg_short = "p"
    @clarg_doc = "Prefix in comment path (default is empty, affects: // from <prefix>/imgui.h:3574:8). "
    prefix : string

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}

class ImguiGen : CppGenBind {
    override include_parse_file = false
    override func_to_stdout = false
    override generate_constant_decl = true
    imgui_skip_func : table<string; bool>
    def ImguiGen(mod_name : string, das_mod_name : string, pfn : string, cfg : Config) {
        bind_root = cfg.output
        bind_module = mod_name
        bind_das_module = das_mod_name
        prefix = cfg.prefix
        let i_imgui = "{get_das_root()}/modules/dasImgui/imgui/"
        let args <- array<string>(
            "-xc++-header",
            "-std=c++1z",
            "-I{get_full_file_name(i_imgui)}",
            "-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS"
        )
        func_per_chunk = 20
        init_args(pfn, cfg.input, args)
        init_rename()
        setDefaultFiles()
        openAllFiles()
    }
    def override get_module_temp_fn : string {
        return ", imguiTempFn"
    }
    def init_rename {
        aot_alias <- {
            "ImVec2"    =>  true,
            "ImVec4"    =>  true,
            "ImColor"   =>  true
        }
        alias2type <- {
            "ImGuiCol"                  => "ImGuiCol_",
            "ImGuiCond"                 => "ImGuiCond_",
            "ImGuiDataType"             => "ImGuiDataType_",
            "ImGuiDir"                  => "ImGuiDir_",
            "ImGuiNavInput"             => "ImGuiNavInput_",
            "ImGuiMouseButton"          => "ImGuiMouseButton_",
            "ImGuiMouseCursor"          => "ImGuiMouseCursor_",
            "ImGuiSortDirection"        => "ImGuiSortDirection_",
            "ImGuiStyleVar"             => "ImGuiStyleVar_",
            "ImGuiTableBgTarget"        => "ImGuiTableBgTarget_",
            "ImDrawFlags"               => "ImDrawFlags_",
            "ImDrawListFlags"           => "ImDrawListFlags_",
            "ImFontAtlasFlags"          => "ImFontAtlasFlags_",
            "ImGuiBackendFlags"         => "ImGuiBackendFlags_",
            "ImGuiButtonFlags"          => "ImGuiButtonFlags_",
            "ImGuiColorEditFlags"       => "ImGuiColorEditFlags_",
            "ImGuiConfigFlags"          => "ImGuiConfigFlags_",
            "ImGuiComboFlags"           => "ImGuiComboFlags_",
            "ImGuiDragDropFlags"        => "ImGuiDragDropFlags_",
            "ImGuiFocusedFlags"         => "ImGuiFocusedFlags_",
            "ImGuiHoveredFlags"         => "ImGuiHoveredFlags_",
            "ImGuiInputTextFlags"       => "ImGuiInputTextFlags_",
            "ImGuiKeyModFlags"          => "ImGuiKeyModFlags_",
            "ImGuiPopupFlags"           => "ImGuiPopupFlags_",
            "ImGuiSelectableFlags"      => "ImGuiSelectableFlags_",
            "ImGuiSliderFlags"          => "ImGuiSliderFlags_",
            "ImGuiTabBarFlags"          => "ImGuiTabBarFlags_",
            "ImGuiTabItemFlags"         => "ImGuiTabItemFlags_",
            "ImGuiTableFlags"           => "ImGuiTableFlags_",
            "ImGuiTableColumnFlags"     => "ImGuiTableColumnFlags_",
            "ImGuiTableRowFlags"        => "ImGuiTableRowFlags_",
            "ImGuiTreeNodeFlags"        => "ImGuiTreeNodeFlags_",
            "ImGuiViewportFlags"        => "ImGuiViewportFlags_",
            "ImGuiWindowFlags"          => "ImGuiWindowFlags_",
            "ImGuiChildFlags"           => "ImGuiChildFlags_"
        }
        local_type_names <- {
            "ImGuiTableColumnSortSpecs" => true,
            "Config" => true}
        imgui_skip_func <- {
            "ImGui::TextUnformatted" => true,
            "ImGuiTextFilter::PassFilter" => true,
            "ImGuiTextBuffer::append" => true,
            "ImGuiInputTextCallbackData::InsertChars" => true,
            "ImGui::CalcTextSize" => true,
            "ImGui::SetNextWindowSizeConstraints" => true,
            "ImDrawList::AddText" => true
        }
        for (v in values(alias2type)) {
            enums_with_flags.insert(v, true)
        }
    }
    def override skip_anyFunction(var c : CXCursor; isMethod : bool) {
        if (AnyGenBind`skip_anyFunction(self, c, isMethod)) {
            return true
        }
        let function_name = namespace_name(string(clang_getCursorSpelling(c)))
        if (imgui_skip_func |> key_exists(function_name)) {
            to_log(LOG_INFO, "skipping IMGUI blocked function {function_name} at {clang_getCursorLocationDescription(c)}")
            return true
        }
        return false
    }
    def override isArgByValue(name : string) {
        return true
    }
    def override generateModuleHPrefix {
        module_h_file |> fwrite("#include \"cb_dasIMGUI.h\"\n")
    }
}

[export]
def main {
    var r <- parse_args(type<Config>)
    if (r |> is_err) {
        print("error: {r |> unwrap_err}\n\n")
        print_help(get_command_info(type<Config>), "bind_imgui")
        return
    }
    var cfg <- r |> move_unwrap
    if (cfg.help) {
        print_help(get_command_info(type<Config>), "bind_imgui")
        return
    }
    var cgb = new ImguiGen("dasIMGUI", "imgui", "imgui.h", cfg)
    defer <| $() {
        unsafe {
            delete cgb
        }
    }
    cgb->generate()
    cgb->genCMakeDecl("DAS_IMGUI_BIND")
}
