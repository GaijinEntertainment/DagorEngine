#ifndef DAS_FORMATTER_H
#define DAS_FORMATTER_H

#include <optional>
#include "daScript/daScript.h"
#include "daScript/das_config.h"

using std::optional;
using std::nullopt;

namespace das::format {

    // formatter config
    enum class FormatOpt {
        Inplace,
        V2Syntax,
        ElseNewLine,
        CommaAtEndOfEnumBitfield,
        SemicolonEOL,
        // in some cases old -> new syntax conversion depends on actual types
        // It can't be done just during parser
        // This flags enables such checks
        // (e.g. [[A()]] can be A() or A(uninitialized))
        VerifyUninitialized,
    };

    class FormatOptions {
    public:
        FormatOptions() = default;
        FormatOptions(das_hash_set<FormatOpt> options_) : options(das::move(options_)) {}

        bool contains(FormatOpt opt) const {
            return options.count(opt);
        }

        void insert(FormatOpt opt) {
            options.emplace(opt);
        }

    private:
        das_hash_set<FormatOpt> options;
    };

    struct Pos {
        uint32_t line;
        uint32_t column;

        static Pos from(LineInfo info) {
            return Pos{info.line, info.column};
        }

        static Pos from_last(LineInfo info) {
            return Pos{info.last_line, info.last_column};
        }


        bool operator != (const Pos &rhs) const {
            return line != rhs.line || column != rhs.column;
        }

        bool operator == (const Pos &rhs) const {
            return line == rhs.line && column == rhs.column;
        }

        bool operator < (const Pos &rhs) const {
            return line < rhs.line || (line == rhs.line && column < rhs.column);
        }


        bool operator <= (const Pos &rhs) const {
            return line < rhs.line || (line == rhs.line && column <= rhs.column);
        }
    };

    // Should be called before and after each formatting operation. Maintains internal compiler state
    void init(TextWriter *ss, string content, FormatOptions options, ProgramPtr program);
    void destroy();

    /**
     * @return output stream
     */
    TextWriter& get_writer();

    /**
     * This function should be called before handling each rule
     * @param pos beginning of the rule
     * @return true if we can handle it now
     *
     * Ex: A(B()). Bison will parse in the following order: B(), A(  ).
     * Due to simplicity of formatter, we can't handle rule, if some part of it was somehow modified.
     * So if B() already handled we can't handle A(   ) in current iteration
     */
    bool prepare_rule(Pos pos);

    /**
     * Input file will be printed until current position.
     * @param pos end of the rule
     */
    void finish_rule(Pos pos);

    /**
     * @return specified substring of input file
     */
    string get_substring(LineInfo info);
    string get_substring(Pos pos1, Pos pos2);
    string substring_between(LineInfo info1, LineInfo info2);

    // hack to get current identation
    string get_line(uint32_t line);

    //
    std::optional<StructurePtr> try_find_struct(const string &name);

    enum class CanInit {
        Can,
        Cannot,
        Unknown,
    };

    bool can_default_init(const string &name);
    CanInit can_init_with(const string &name, uint32_t arg_cnt);

    // Maybe we should replace it with getConfig(), if there will be a lot of options
    bool is_replace_braces();    // use v2 syntax
    bool is_else_newline();    // start else from new line
    bool end_with_semicolon();   // end statements with ';'
    bool enum_bitfield_with_comma();   // end statements with ','
    bool is_formatter_enabled(); // are formatter enabled mode
}

#endif //DAS_FORMATTER_H
