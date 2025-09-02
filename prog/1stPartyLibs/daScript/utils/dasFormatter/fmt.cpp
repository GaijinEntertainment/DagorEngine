#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"
#include "../src/parser/parser_state.h"

#include <fstream>
#include <filesystem>

#include "daScript/ast/ast.h"

typedef void * yyscan_t;
union YYSTYPE;

#define YY_EXTRA_TYPE das::DasParserState *

#define YY_NO_UNISTD_H
#include "../src/parser/lex.yy.h"
#include "../src/parser/lex2.yy.h"
#include "../src/parser/parser_state.h"

#include "formatter.h"
#include "fmt.h"
#include "helpers.h"

void das_yybegin(const char * str, uint32_t len, yyscan_t yyscanner);
int fmt_yyparse(yyscan_t scanner);

das::FileAccessPtr get_file_access(char *pak);//link time resolved dependencies

namespace das::format {

enum class NoCommentReason {
    OpenComment,
    CloseComment,
    String,
};

/**
 * Removes all useless semicolons
 * @param str
 * @return formatted string
 */
string remove_semicolons(string_view str, bool is_gen2) {
    string result;
    size_t line_end = size_t(-1);
    int par_balance = 0; // ()
    int sq_braces_balance = 0; // []
    int braces_balance = 0; // {}
    vector<NoCommentReason> disables;
    do {
        size_t offset = line_end + 1;
        line_end = str.find('\n', offset);
        // bool is_eof = str.size() <= offset;
        // bool indent_non_zero = (!is_eof && (str.at(offset) == ' ' || str.at(offset) == '\t'));
        auto last_char_idx = offset + format::find_comma_place(str.substr(offset, line_end - offset));
        auto cur_line = str.substr(offset, last_char_idx - offset + 1);
        for (size_t i = 0; i < cur_line.size(); i++) {
            const auto c = cur_line.at(i);
            if (c == '\\') {
                i++;
                continue;
            }
            optional<NoCommentReason> maybe_reason;
            if (disables.empty() && c == '/' && cur_line.size() > i + 1 && cur_line.at(i + 1) == '/') {
                break;
            }
            if (c == '"') {
                maybe_reason = NoCommentReason::String;
            } else if (c == '/' && cur_line.size() > i + 1 && cur_line.at(i + 1) == '*') {
                maybe_reason = NoCommentReason::OpenComment;
            } else if (c == '*' && cur_line.size() > i + 1 && cur_line.at(i + 1) == '/') {
                maybe_reason = NoCommentReason::CloseComment;
            }
            if (maybe_reason) {
                if (!disables.empty() && disables.back() == maybe_reason) {
                    disables.pop_back();
                } else {
                    if (maybe_reason == NoCommentReason::OpenComment) {
                        maybe_reason = NoCommentReason::CloseComment;
                    }
                    if (disables.empty() || disables.back() != NoCommentReason::String) {
                        disables.emplace_back(maybe_reason.value());
                    }
                }
            }
            if (!disables.empty()) {
                continue;
            }
            switch (c) {
                case '(': par_balance += 1; break;
                case ')': par_balance -= 1; break;
                case '[': sq_braces_balance += 1; break;
                case ']': sq_braces_balance -= 1; break;
                case '{': braces_balance += 1; break;
                case '}': braces_balance -= 1; break;
                default: break;
            }
        }
        if (par_balance == 0 && (braces_balance == 0 || is_gen2) &&
            sq_braces_balance == 0 &&
            last_char_idx != offset - 1 &&
            str.at(last_char_idx) == ';') {
            result += string(str.substr(offset, last_char_idx - offset));
            result += string(str.substr(last_char_idx + 1, line_end - last_char_idx));
        } else {
            result += string(str.substr(offset, line_end + 1 - offset));
        }
    } while (line_end != size_t(-1));
    return result;
}

void addNewModules(ModuleGroup &libGroup, ProgramPtr program) {
    libGroup.addModule(program->thisModule.release());
    program->library.foreach([&](Module *pm) -> bool {
        if (!pm->name.empty() && pm->name != "$") {
            if (!libGroup.findModule(pm->name)) {
                libGroup.addModule(pm);
            }
        }
        return true;
    }, "*");
}

Result transform_syntax(const string &filename, const string content, format::FormatOptions options) {
    auto src = content;
    if (src.find("options gen2") == 0) {
        Result res;
        res.ok = src;
        return res;
    }
    string prev;
    vector<ModuleInfo> req;
    vector<MissingRecord> missing;
    vector<RequireRecord> circular, notAllowed;
    vector<FileInfo *> chain;
    das_set<string> dependencies;
    ModuleGroup libGroup;
    CodeOfPolicies policies;
    policies.aot = false;
    policies.aot_module = false;
    policies.fail_on_lack_of_aot_export = false;
    policies.version_2_syntax = false;

    TextPrinter tp;

    auto access = get_file_access(nullptr);
    TextPrinter tout;
    string moduleName;
    das_hash_map<string, NamelessModuleReq> namelessReq;
    vector<NamelessMismatch> namelessMismatches;
    if (getPrerequisits(filename, access, moduleName, req, missing, circular, notAllowed, chain,
                        dependencies, namelessReq, namelessMismatches, libGroup, nullptr, 1, !policies.ignore_shared_modules)) {
        for (auto &mod: req) {
            if (libGroup.findModule(mod.moduleName)) {
                continue;
            }
            auto program = parseDaScript(mod.fileName, mod.moduleName, access, tout, libGroup, true, true,
                                         policies);
            policies.threadlock_context |= program->options.getBoolOption("threadlock_context", false);
            if (program->failed()) {
                for (const auto& err: program->errors) {
                    tp << err.what << " " << err.at.describe() << '\n';
                }
                // return {}; // still try to compile
            }
            if (program->thisModule->name.empty()) {
                program->thisModule->name = mod.moduleName;
                program->thisModule->wasParsedNameless = true;
            }
            program->thisModule->fileName = mod.fileName;
            addNewModules(libGroup, program);
        }
    }

    int iter = 0;
    policies.version_2_syntax = false;
    const auto tmp_name1 = std::filesystem::temp_directory_path() / "tmp1.das";
    {
        std::ofstream ostream(tmp_name1);
        ostream << src.c_str();
    }
    auto src_program = parseDaScript(das::string(tmp_name1.string().c_str()), "", access, tout, libGroup, true, true, policies);
    while (prev != src) {
        prev = src;

        TextWriter ss;
        format::init(&ss, src, options, src_program);

        // All initialization and parsing took from daslang source
        yyscan_t scanner = nullptr;
        ProgramPtr program = make_smart<Program>();
        (*daScriptEnvironment::bound)->g_Program = program;
        (*daScriptEnvironment::bound)->g_compilerLog = &tout;
        program->promoteToBuiltin = false;
        program->isCompiling = true;
        program->isDependency = false;
        program->needMacroModule = false;
        program->policies = policies;
        program->thisModuleGroup = &libGroup;
        program->thisModuleName = program->thisModule->name;
        libGroup.foreach([&](Module *pm) {
            program->library.addModule(pm);
            return true;
        }, "*");


        DasParserState parserState;
        parserState.g_Access = access;
        parserState.g_FileAccessStack.push_back(access->getFileInfo(filename));
        parserState.g_Program = program;
        parserState.das_def_tab_size = (*daScriptEnvironment::bound)->das_def_tab_size;
        parserState.das_gen2_make_syntax = false;
        libGroup.foreach([&](Module *mod) {
            if (mod->commentReader) {
                parserState.g_CommentReaders.push_back(mod->commentReader.get());
            }
            return true;
        }, "*");

        das_yylex_init_extra(&parserState, &scanner);
        das_yybegin(src.c_str(), uint32_t(src.size()), scanner);
        auto err = fmt_yyparse(scanner);

        // end of parsing

        format::destroy();
        das_yylex_destroy(scanner);
        if (err != 0) {
            for (const auto erR: program->errors) {
                tp << erR.at.describe() << '\n' << erR.what << '\n';
            }
            if (iter == 0) {
                return {};
            }
            Result res;
            res.error = Result::ErrorInfo{prev, program->errors.front().what};
            return res;
//            return {error=Result::ErrorInfo{prev, program->errors.front().what}}; // designated initializers is c++ 20 feature
        }
        src = ss.str();
        iter++;
    }
    if (!options.contains(FormatOpt::SemicolonEOL)) {
        src = remove_semicolons(src, options.contains(FormatOpt::V2Syntax));
    }
    const auto tmp_name = std::filesystem::temp_directory_path() / "tmp.das";
    {
        std::ofstream ostream(tmp_name);
        ostream << src.c_str();
        ostream.flush();
    }
    policies.version_2_syntax = options.contains(format::FormatOpt::V2Syntax);
    auto program = parseDaScript(das::string(tmp_name.string().c_str()), "", access, tout, libGroup, true, true, policies);
    Result res;
    if (!program->failed()) {
        res.ok = src; // designated initializers not supported in CI
    } else {
        tp << program->errors.front().at.describe() << '\n';
        res.error = Result::ErrorInfo{prev, program->errors.front().what};
    }
    return res;
}

int run(FormatOptions opts, const vector<string> &files) {
    int ret_code = 0;
    TextPrinter tp;
    if (files.size() > 1 && !opts.contains(FormatOpt::Inplace)) {
        tp << "Expected inplace mode in case of more than 1 file\n";
    }
    for (const auto &file: files) {
        tp << "input file=" << file << '\n';
        std::ifstream t(file.c_str());

        string str(std::string((std::istreambuf_iterator<char>(t)),
                   std::istreambuf_iterator<char>()).c_str());

        auto res = transform_syntax(file, str, opts);
        if (res.ok) {
            if (opts.contains(FormatOpt::Inplace)) {
                std::ofstream out(file.c_str());
                out << res.ok.value().c_str();
            } else {
                tp << res.ok.value().c_str();
            }
        } else if (res.error) {
            if (!opts.contains(FormatOpt::Inplace)) {
                tp << res.error->content << '\n';
            }
            tp << file << '\n' << res.error->what << '\n';
            ret_code = -1;
        } else {
            tp << "cant_compile=" << file << '\n';
        }
    }
    return ret_code;
}
}
