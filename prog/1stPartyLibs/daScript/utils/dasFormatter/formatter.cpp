#include "formatter.h"

namespace das::format {

    struct State {
        FormatOptions options;
        vector<string> content_;
        Pos last;
        ProgramPtr program;
        bool enabled = false;
        TextWriter *ss;
    };

    static State state;

    optional<Pos> try_print_until(Pos info) {
        if (!state.enabled) {
            return nullopt;
        }
        if (info.line < state.last.line) {
            return state.last;
        }
        get_writer() << get_substring(state.last, info);
        state.last = info;
        return nullopt;
    }


    void init(TextWriter *ss, string content, FormatOptions options, ProgramPtr program) {
        string line;
        content.push_back('\n'); // easiest way to flush the last line
        for (const auto c: content) {
            if (c != '\n') {
                line.push_back(c);
            } else {
                state.content_.emplace_back(das::move(line));
                line.clear();
            }
        }
        state.ss = ss;
        state.enabled = true;
        state.last = Pos{1, 0};
        state.program = program;
        state.options = das::move(options);
        if ((state.content_.empty() || state.content_.front().substr(0, strlen("options gen2")) != "options gen2") &&
             state.options.contains(FormatOpt::V2Syntax)) {
            *state.ss << "options gen2";
            if (state.options.contains(FormatOpt::SemicolonEOL)) {
                *state.ss << ";";
            }
            *state.ss << "\n";
        }
    }

    void destroy() {
        if (!state.content_.empty()) {
            try_print_until(Pos{static_cast<uint32_t>(state.content_.size()),
                                static_cast<uint32_t>(state.content_.back().size())});
        }
        state.ss = nullptr;
        state.enabled = false;
        state.last = {};
        state.options = {};
        state.content_ = {};
    }

    void set_to(Pos info) {
        assert(state.last <= info);
        state.last = info;
    }

    string get_substring(Pos pos1, Pos pos2) {
        if (!state.enabled || pos1 < state.last) {
            return "";
        }

        TextPrinter tp;
        string result;
        for (; pos1.line < pos2.line; pos1.line++, pos1.column = 0) {
            if (pos1.line > state.content_.size()) {
                tp << "Warning, location line out of range\n";
                return "";
            } else if (pos1.column > state.content_[pos1.line - 1].length()) {
//                tp << "incorrect location info, extra symbols "
//                          << pos1.column << " "
//                          << state.content_[pos1.line - 1].length() << '\n';
            } else {
                result += state.content_[pos1.line - 1].substr(pos1.column);
            }
            result += '\n';
        }
        if (pos2.column < pos1.column || pos2.line < pos1.line) {
            assert(result.empty());
            assert(pos1.line == pos2.line);
            return "";
        }

        if (pos1.line > state.content_.size()) {
            tp << "Warning, location line out of range\n";
            return "";
        }
        if (pos1.column > state.content_[pos1.line - 1].size()) {
            tp << "Warning, location column out of range\n";
            return "";
        }
        result += state.content_[pos1.line - 1].substr(pos1.column, pos2.column - pos1.column);
        return result;
    }


    string get_substring(LineInfo info) {
        return get_substring(Pos{info.line, info.column},
                             Pos{info.last_line, info.last_column});
    }

    string substring_between(LineInfo info1, LineInfo info2) {
        return get_substring(Pos{info1.last_line, info1.last_column},
                             Pos{info2.line, info2.column});
    }

    string get_line(uint32_t line) {
        if (line - 1 >= state.content_.size()) {
            TextPrinter() << "Warning, requested line is too big\n";
            return "";
        }
        return state.content_.at(line - 1);
    }

    TextWriter& get_writer() {
        return *state.ss;
    }

    bool prepare_rule(Pos pos) {
        if (state.enabled && state.last <= pos) {
            try_print_until(pos);
            return true;
        }
        return false;
    }

    void finish_rule(Pos pos) {
        assert(state.enabled && state.last <= pos);
        set_to(pos);
    }

    optional<StructurePtr> try_find_struct(const string &name) {
        if (state.program != nullptr) {
            const auto library = state.program->library;
            auto structs = state.program->findStructure(name);
            auto aliases = state.program->findAlias(name);
            if (!structs.empty()) {
                assert(structs.size() == 1);
                assert(aliases.empty());
                return structs.front();
            } else if (!aliases.empty()) {
                assert(aliases.size() == 1);
            }
        }
        return std::nullopt;
    }

    bool can_default_init(const string &name) {
        auto maybe_struct = try_find_struct(name);
        if (!maybe_struct) {
            return false;
        } else {
            const auto &fields = maybe_struct->get()->fields;
            return all_of(fields.begin(), fields.end(), [](auto field) {
                return field.init;
            });
        }
    }

    CanInit can_init_with(const string &name, uint32_t arg_cnt) {
        return CanInit::Can;
        // Possible uninitialized
        if (arg_cnt == 0) {
            if (can_default_init(name)) {
                return CanInit::Can;
            } else {
                return CanInit::Cannot;
            }
        }
        auto maybe_struct = try_find_struct(name);
        if (!maybe_struct) {
            return CanInit::Unknown;
        }
        return (*maybe_struct)->fields.size() == arg_cnt ? CanInit::Can : CanInit::Unknown;
    }

    bool is_else_newline() {
        return state.options.contains(FormatOpt::ElseNewLine);
    }

    bool is_replace_braces() {
        return state.options.contains(FormatOpt::V2Syntax);
    }

    bool end_with_semicolon() {
        return true; // state.options.contains(FormatOpt::SemicolonEOL);
    }

    bool enum_bitfield_with_comma() {
        return state.options.contains(FormatOpt::CommaAtEndOfEnumBitfield);
    }

    bool is_formatter_enabled() {
        return state.enabled;
    }

}

