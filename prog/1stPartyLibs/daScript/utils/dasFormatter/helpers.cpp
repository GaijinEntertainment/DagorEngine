#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"

#include "helpers.h"

namespace das::format {

    optional<string> type_to_string(const TypeDecl *type_decl, LineInfo loc) {
        if (type_decl->isTemp(false) || type_decl->alias == "``MACRO``TAG``") {
            return "struct<" + format::get_substring(loc) + ">";
        } else if (type_decl->isAuto() && !type_decl->isArray() && !type_decl->isPointer()) {
            return nullopt;
        } else {
            return get_substring(loc);
        }
    }

    string handle_msd(LineInfo start, ExprMakeStruct *msd, LineInfo end, string type_name, bool is_initialized) {
        // ";" -> ","
        auto structs = msd->structs;
        if (structs.empty()) {
            return "";
        }
        const string prev_sep = ";";
        string maybe_uninit = is_initialized ? "" : "uninitialized";
        const string prefix = type_name + "(";
        const string sep = "), ";
        const string suffix = ")";

        const auto front = structs.front();
        string result;
        auto last = start;
        for (size_t i = 0; i < structs.size(); i++) {
            const auto &el = structs.at(i);
            auto concat = format::substring_between(last, el->front()->at);
            auto prev_end = concat.find(prev_sep);
            assert(i == 0 || prev_end != npos); // incorrect prev_sep
            {
                auto can_init = can_init_with(type_name, uint32_t(el->size()));
                switch (can_init) {
                    case CanInit::Can: maybe_uninit.clear(); break;
                    case CanInit::Cannot: maybe_uninit = "uninitialized"; break;
                    case CanInit::Unknown: break;
                }
            }
            const auto cur_prefix = prefix + maybe_uninit;
            if (i == 0) {
                concat = cur_prefix + concat;
            } else if (prev_end != npos) {
                concat.replace(prev_end, prev_sep.size(), sep + cur_prefix);
            } else {
                std::abort();
            }
            auto new_line = convert_to_string(*el.get());
            result
                .append(concat)
                .append(new_line);
            last = el->back()->at;
        }
        result.append(format::substring_between(last, end))
            .append(suffix);
        return result;
    }

    string handle_mka(LineInfo start, ExprMakeArray *mka, LineInfo end) {
        const auto &values = mka->values;
        if (values.empty()) {
            return "";
        }
        const string prev_sep = ";";
        const string sep = ",";

        // const auto &front = static_cast<ExprMakeTuple *>(values.front().get())->values;
        string result;
        string suffix;
        Pos last = Pos::from_last(start);
        for (size_t i = 0; i < values.size(); i++) {
            const auto &el = values.at(i);
            string middle;
            string prefix;
            Pos from;
            Pos to;
            auto real_sep = suffix + sep;
            if ( el->rtti_isMakeTuple() ) {
                const auto & Values = static_cast<ExprMakeTuple *>(el.get())->values;
                prefix = "(";
                suffix = ")";
                middle = convert_to_string(Values);
                from = Pos::from(Values.front()->at);
                to = Pos::from_last(Values.back()->at);
            } else {
                suffix.clear();
                middle = format::get_substring(el->at);
                from = Pos::from(el->at);
                to = Pos::from_last(el->at);
            }
            auto concat = format::get_substring(last, from);
            auto prev_end = concat.find(prev_sep);
            assert(i == 0 || prev_end != npos); // incorrect prev_sep
            if (i != 0 && prev_end != npos) {
                concat.replace(prev_end, prev_sep.size(), real_sep);
            }
            result.append(concat)
                .append(prefix)
                .append(middle);
            last = to;
        }
        result.append(get_substring(last, Pos::from(end)))
            .append(suffix);

        return result;
    }

    size_t find_comma_place(string_view line) { // dirty hack to find last meaningful character.
        size_t comment_start = line.find("//");
        if (comment_start == 0) {
            return 0;
        }
        if (comment_start != npos) {
            const auto code_part = line.substr(0, comment_start);
            const auto quotes_code = count(code_part.begin(), code_part.end(), '"');
            if (quotes_code % 2 != 0) {
                comment_start = line.size(); // should be recursive call to find_comma_place, let's simply put to the end
            }
        }
        return line.find_last_not_of(" \t\r", comment_start == npos ? npos : comment_start - 1);
    }

    void try_emit_at_eol(Pos loc, char sep) {
        if (format::is_replace_braces()) {
            const auto &line = format::get_line(loc.line);
            auto comma_place = format::find_comma_place(line);
            if (comma_place == npos) {
                // it means EOF.
                return;
            }
            loc.column = uint32_t(comma_place + 1);
            if (line.at(comma_place) != sep && // ad-hoc, fix location
                format::prepare_rule(loc)) {
                format::get_writer() << sep;
                format::finish_rule(loc);
            }
        }
    }

    void try_semicolon_at_eol(Pos loc) {
        if (format::end_with_semicolon()) {
            try_emit_at_eol(loc, ';');
        }
    }

    size_t get_indent(Pos loc, size_t tab_size) {
        auto start = loc;
        start.column = 0;
        auto substr = get_substring(start, loc);
        size_t result = 0;
        for (auto c: substr) {
            if (c == ' ') {
                result += 1;
            } else if (c == '\t') {
                result += tab_size;
            } else {
                break;
            }
        }
        return result;
    }

    void handle_brace(Pos prev_loc, uint32_t value, const string &internal, size_t tab_size, Pos end_loc) {
        const auto &line = format::get_line(prev_loc.line);
        auto brace_column = format::find_comma_place(line);
        prev_loc.column = uint32_t(brace_column + 1);
        if (format::is_replace_braces() && value != 0xdeadbeef &&
            format::prepare_rule(prev_loc)) {

            if (format::prepare_rule(prev_loc)) {
                format::get_writer() << " {" << internal << "\n" << string(value * tab_size, ' ') + "}";
                format::finish_rule(end_loc);
            }
        }
    }

    void replace_with(bool v2_only, Pos start, const string &internal, Pos end, const string &open, const string &close) {
        if ((!v2_only || format::is_replace_braces()) && format::prepare_rule(start)) {
            format::get_writer() << open << internal << close;
            format::finish_rule(end);
        }
    }

    void replace_with(bool v2_only, Pos start, LineInfo internal, Pos end, const string &open, const string &close) {
        replace_with(v2_only, start, format::get_substring(internal), end, open, close);
    }

    bool skip_token(bool v2_only, bool need_skip, LineInfo token) {
        if (!need_skip) {
            return false;
        }
        auto start = format::Pos::from(token);
        auto end = format::Pos::from_last(token);
        if (start == end) {
            return false;
        }
        if ((!v2_only || format::is_replace_braces()) && format::prepare_rule(start)) {
            format::finish_rule(end);
            return true;
        } else {
            return false;
        }
    }

    void wrap_par_expr(LineInfo real_expr, LineInfo info_expr) {
        if (format::is_replace_braces() && real_expr == info_expr && format::prepare_rule(Pos::from(real_expr))) {
            format::get_writer() << "(" << format::get_substring(real_expr) << ")";
            format::finish_rule(Pos::from_last(real_expr));
        }
    }

    void wrap_par_expr_newline(LineInfo real_expr, LineInfo info_expr) {
        if (real_expr.line != real_expr.last_line) {
            wrap_par_expr(real_expr, info_expr);
        }
    }


    LineInfo concat(LineInfo first, LineInfo last) {
        return LineInfo(
            first.fileInfo,
            first.column,
            first.line,
            last.last_column,
            last.last_line);
    }

    static bool is_empty(const string &str) {
        return all_of(str.begin(), str.end(), [](auto c)
        { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; });
    }

    void skip_spaces_or_print(LineInfo /*prev*/, LineInfo block, LineInfo next, size_t tab_size, const string& change) {
        auto internal = format::substring_between(block, next);
        auto same_depth = format::get_indent(format::Pos::from_last(block), tab_size) ==
                          format::get_indent(format::Pos::from(next), tab_size);
        if (is_empty(internal) && same_depth && internal != change && format::prepare_rule(Pos::from_last(block))) {
            format::get_writer() << change;
            format::finish_rule(Pos::from(next));
        }
    }
}
