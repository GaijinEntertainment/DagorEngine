#ifndef DAS_HELPERS_H
#define DAS_HELPERS_H

#include <daScript/ast/ast_typedecl.h>

#include "formatter.h"

namespace das::format {

    static inline constexpr size_t npos = size_t(-1);

    template <typename T>
    string convert_to_string(const vector<T> &vec, string sep = ",", string prev_sep = ",") {
        if (vec.empty()) {
            return "";
        }
        string result = format::get_substring(vec.front()->at);
        Pos last = Pos::from_last(vec.front()->at);
        for_each(vec.begin()+1, vec.end(), [&last, &prev_sep, &result, &sep](const auto& el) {
            auto concat = format::get_substring(last, Pos::from(el->at));
            auto prev_end = concat.find(prev_sep);
            if (prev_end != npos) {
                concat.replace(prev_end, prev_sep.size(), sep);
            } else {
                assert(concat.find("=>") != npos);
                TextPrinter() << "Be careful, => is not safe yet\n";
                concat.replace(concat.find("=>"), 2, ","); // It's not safe!
            }
            auto new_line = format::get_substring(el->at);
            result += concat;
            result += new_line;
            last = Pos::from_last(el->at);
        });
        return result;
    }

    optional<string> type_to_string(const TypeDecl *type_decl, LineInfo loc /* pass loc, since it's sometimes incorrect on type itself*/);

    string handle_msd(LineInfo start, ExprMakeStruct* msd, LineInfo end, string type_name, bool is_initialized = true);

    string handle_mka(LineInfo start, ExprMakeArray* mka, LineInfo end);

    size_t find_comma_place(string_view line);

    void try_emit_at_eol(Pos loc, char sep);

    void try_semicolon_at_eol(Pos loc);

    size_t get_indent(Pos loc, size_t tab_size);

    void handle_brace(Pos prev_loc, uint32_t value, const string &internal, size_t tab_size, Pos end_loc);

    void replace_with(bool v2_only, Pos start, const string &internal, Pos end, const string &open, const string &close);

    void replace_with(bool v2_only, Pos start, LineInfo internal, Pos end, const string &open, const string &close);

    bool skip_token(bool v2_only, bool need_skip, LineInfo token);

    void wrap_par_expr(LineInfo real_expr, LineInfo info_expr);
    // wrap with parenthesis only if it is mult line expression
    void wrap_par_expr_newline(LineInfo real_expr, LineInfo info_expr);

    LineInfo concat(LineInfo first, LineInfo last);

    void skip_spaces_or_print(LineInfo prev, LineInfo block, LineInfo next, size_t tab_size, const string& change = " ");
}

#endif //DAS_HELPERS_H
