#pragma once

namespace das {

    struct DasParserState {
        int das_current_line_indent = 0;
        int das_indent_level = 0;
        int das_def_tab_size = 4;
        int das_tab_size = 4;
        int das_nested_parentheses = 0;
        int das_nested_curly_braces = 0;
        int das_nested_square_braces = 0;
        int das_nested_sb = 0;
        int das_yycolumn = 0;
        int das_c_style_depth = 0;
        int das_arrow_depth = 0;
        bool das_need_oxford_comma = true;
        bool das_force_oxford_comma = false;
        bool das_suppress_errors = false;
        bool das_in_normal = false;
        bool das_has_type_declarations = false;
        vector<int> das_line_no;
        das_hash_set<string> das_already_include;
        das_hash_map<string,string>   das_module_alias;
        das_hash_map<string,bool> das_keywords;
        Structure * g_thisStructure = nullptr;
        vector<CommentReader *> g_CommentReaders;
        ReaderMacro * g_ReaderMacro = nullptr;
        ExprReader * g_ReaderExpr = nullptr;
        vector<FileInfo *> g_FileAccessStack;
        FileAccessPtr g_Access;
        ProgramPtr g_Program;
    };
}
