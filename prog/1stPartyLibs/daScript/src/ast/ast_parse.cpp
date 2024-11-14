#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/ast/ast_expressions.h"

#include "../parser/parser_state.h"

typedef void * yyscan_t;
union YYSTYPE;

#define YY_EXTRA_TYPE das::DasParserState *

#define YY_NO_UNISTD_H
#include "../parser/lex.yy.h"
#include "../parser/lex2.yy.h"

void das_yybegin(const char * str, uint32_t len, yyscan_t yyscanner);
int das_yyparse(yyscan_t yyscanner);

void das2_yybegin(const char * str, uint32_t len, yyscan_t yyscanner);
int das2_yyparse(yyscan_t yyscanner);

namespace das {

    bool isUtf8Text ( const char * src, uint32_t length ) {
        if ( length>=3  ) {
            auto usrc = (const uint8_t *)src;
            if ( usrc[0]==0xef && usrc[1]==0xbb && usrc[2]==0xbf) {
                return true;
            }
        }
        return false;
    }

    __forceinline bool isalphaE ( int ch ) {
        return (ch>='a' && ch<='z') || (ch>='A' && ch<='Z');
    }

    __forceinline bool isalnumE ( int ch ) {
        return (ch>='0' && ch<='9') || (ch>='a' && ch<='z') || (ch>='A' && ch<='Z');
    }

    struct ChainGuard {
        ChainGuard ( vector<FileInfo *> & c, FileInfo * fi ) : chain(c) {
            chain.push_back(fi);
        }
        ~ChainGuard() {
            chain.pop_back();
        }
        vector<FileInfo *> & chain;
    };

    void getAllRequireReq ( FileInfo * fi, const FileAccessPtr & access, vector<RequireRecord> & req, vector<FileInfo *> & chain, das_set<FileInfo *> & collected ) {
        ChainGuard guard(chain,fi);
        const char * src = nullptr;
        uint32_t length = 0;
        fi->getSourceAndLength(src, length);
        if ( isUtf8Text(src,length) ) { // skip utf8 byte order mark
            src += 3;
            length -= 3;
        }
        const char * src_end = src + length;
        bool wb = true;
        while ( src < src_end ) {
            if ( src[0]=='"' ) {
                src ++;
                while ( src < src_end && src[0]!='"' ) {
                    src ++;
                }
                src ++;
                wb = true;
                continue;
            } else if ( src[0]=='/' && src[1]=='/' ) {
                while ( src < src_end && !(src[0]=='\n') ) {
                    src ++;
                }
                src ++;
                wb = true;
                continue;
            } else if ( src[0]=='/' && src[1]=='*' ) {
                int depth = 0;
                while ( src < src_end ) {
                    if ( src[0]=='/' && src[1]=='*' ) {
                        depth ++;
                        src += 2;
                    } else if ( src[0]=='*' && src[1]=='/' ) {
                        if ( --depth==0 ) break;
                        src += 2;
                    } else {
                        src ++;
                    }
                }
                src +=2;
                wb = true;
                continue;
            } else if ( wb && ((src+8)<src_end) && (src[0]=='r' || src[0]=='i') ) {   // need space for 'require ' || 'include '
                bool isReq = memcmp(src, "require", 7)==0;
                bool isInc = !isReq && (memcmp(src, "include", 7)==0);
                if ( isReq || isInc ) {
                    src += 7;
                    if ( isspace(src[0]) ) {
                        while ( src < src_end && isspace(src[0]) ) {
                            src ++;
                        }
                        if ( src >= src_end ) {
                            continue;
                        }
                        if ( src[0]=='_' || isalphaE(src[0]) || src[0] == '%' || src[0] == '.' || src[0]=='/' ) {
                            string mod;
                            mod += *src++;
                            while ( src < src_end && (isalnumE(src[0]) || src[0]=='_' || src[0]=='.' || src[0]=='/') ) {
                                mod += *src ++;
                            }
                            if ( isReq ) {
                                req.push_back({mod,chain});
                            } else if ( isInc ) {
                                string incFileName = access->getIncludeFileName(fi->name,mod);
                                auto info = access->getFileInfo(incFileName);
                                if ( info ) {
                                    if ( collected.find(info)==collected.end() ) {
                                        collected.insert(info);
                                        getAllRequireReq(info, access, req, chain, collected);
                                    }
                                }
                            }
                            continue;
                        } else {
                            wb = true;
                            goto nextChar;
                        }
                    } else {
                        wb = false;
                        goto nextChar;
                    }
                } else {
                    goto nextChar;
                }
            }
        nextChar:
            wb = src[0]!='_' && (wb ? !isalnumE(src[0]) : !isalphaE(src[0]));
            src ++;
        }
    }

    vector<RequireRecord> getAllRequire ( FileInfo * fi, vector<FileInfo *> & chain, const FileAccessPtr & access  ) {
        das_set<FileInfo *> collected;
        vector<RequireRecord> req;
        getAllRequireReq(fi, access, req, chain, collected);
        return req;
    }

    string getModuleName ( const string & nameWithDots ) {
        auto idx = nameWithDots.find_last_of("./");
        if ( idx==string::npos ) return nameWithDots;
        return nameWithDots.substr(idx+1);
    }

    string getModuleFileName ( const string & nameWithDots ) {
        auto fname = nameWithDots;
        // TODO: should we?
        replace ( fname.begin(), fname.end(), '.', '/' );
        return fname;
    }

    bool addRequirements(const string & fileName, ModuleGroup & libGroup, Module * mod, const FileAccessPtr & access,
            vector<RequireRecord> & notAllowed, vector<FileInfo*> & chain, TextWriter * log, int tab ) {
        if ( !access->isModuleAllowed(mod->name, fileName) ) {
            notAllowed.push_back({mod->name,chain});
            if ( log ) {
                *log << string(tab,'\t') << "dependency " << mod->name << " - NOT ALLOWED\n";
            }
            return false;
        } else {
            if ( log ) {
                *log << string(tab,'\t') << "add dependency " << mod->name << "\n";
            }
            if ( libGroup.addModule(mod) ) {
                tab ++;
                for ( auto & dep : mod->requireModule ) {
                    chain.push_back(dep.first->getFileInfo());
                    if ( !addRequirements(fileName, libGroup, dep.first, access, notAllowed, chain, log, tab) ) {
                        chain.pop_back();
                        return false;
                    }
                    chain.pop_back();
                }
                tab --;
            }
            return true;
        }
    }

    string getDasRoot ( void );

    bool getPrerequisits ( const string & fileName,
                          const FileAccessPtr & access,
                          vector<ModuleInfo> & req,
                          vector<RequireRecord> & missing,
                          vector<RequireRecord> & circular,
                          vector<RequireRecord> & notAllowed,
                          vector<FileInfo *> & chain,
                          das_set<string> & dependencies,
                          ModuleGroup & libGroup,
                          TextWriter * log,
                          int tab,
                          bool allowPromoted ) {
        if ( auto fi = access->getFileInfo(fileName) ) {
            ChainGuard guard(chain,fi);
            if ( log ) {
                *log << string(tab,'\t') << "in " << fileName << "\n";
            }
            vector<RequireRecord> ownReq = getAllRequire(fi, chain, access);
            for ( auto & modRec : ownReq ) {
                string & mod = modRec.name;
                if ( log ) {
                    *log << string(tab,'\t') << "require " << mod << "\n";
                }
                if ( !access->canBeRequired(mod, fileName) )
                {
                    notAllowed.push_back({mod,chain});
                    if ( log ) {
                        *log << string(tab,'\t') << "from " << fileName << " require " << mod << " - CAN'T BE REQUIRED\n";
                    }
                    return false;
                }
                auto module = Module::requireEx(mod, allowPromoted); // try native with that name
                if ( !module ) {
                    auto info = access->getModuleInfo(mod, fileName);
                    if ( !info.moduleName.empty() ) {
                        mod = info.moduleName;
                        if ( log ) {
                            *log << string(tab,'\t') << " resolved as " << mod << "\n";
                        }
                    }
                    module = Module::requireEx(mod, allowPromoted); // try native with that name AGAIN (promoted?)
                    if ( !module ) {
                        auto it_r = find_if(req.begin(), req.end(), [&] ( const ModuleInfo & reqM ) {
                            return reqM.moduleName == mod;
                        });
                        if ( it_r==req.end() ) {
                            if ( dependencies.find(mod) != dependencies.end() ) {
                                // circular dependency
                                if ( log ) {
                                    *log << string(tab,'\t') << "from " << fileName << " require " << mod << " - CIRCULAR DEPENDENCY\n";
                                }
                                circular.push_back({mod, chain});
                                return false;
                            }
                            dependencies.insert(mod);
                            // module file name
                            if ( info.moduleName.empty() ) {
                                // request can't be translated to module name
                                if ( log ) {
                                    *log << string(tab,'\t') << "from " << fileName << " require " << mod << " - MODULE INFO NOT FOUND\n";
                                }
                                missing.push_back({mod,chain});
                                return false;
                            }
                            if ( !getPrerequisits(info.fileName, access, req, missing, circular, notAllowed, chain, dependencies, libGroup, log, tab + 1, allowPromoted) ) {
                                return false;
                            }
                            if ( log ) {
                                *log << string(tab,'\t') << "from " << fileName << " require " << mod
                                    << " - ok, new module " << info.moduleName << " at " << info.fileName << "\n";
                            }
                            req.push_back(info);
                        } else {
                            if ( !access->isSameFileName(it_r->fileName, info.fileName) ) {
                                if ( log ) {
                                    *log << string(tab,'\t') << "from " << fileName << " require " << mod << " - module name collision\n"
                                         << string(tab+1,'\t') << "requested from " << it_r->fileName << " and from " << info.fileName << "\n";
                                }
                                missing.push_back({mod, chain});
                                return false;
                            } else {
                                if ( log ) {
                                    *log << string(tab,'\t') << "from " << fileName << " require " << mod << " - already required\n";
                                }
                            }
                        }
                    } else {
                        if ( log ) {
                            *log << string(tab,'\t') << "from " << fileName << " require " << mod << " - shared, ok\n";
                        }
                        if ( !addRequirements(fileName, libGroup, module, access, notAllowed, chain, log, tab) ) {
                            return false;
                        }
                    }
                } else {
                    if ( log ) {
                        *log << string(tab,'\t') << "from " << fileName << " require " << mod << " - ok\n";
                    }
                    if ( !access->isModuleAllowed(module->name, fileName) ) {
                        notAllowed.push_back({mod,chain});
                        if ( log ) {
                            *log << string(tab,'\t') << "in " << fileName << " module " << module->name << " - NOT ALLOWED\n";
                        }
                        return false;
                    } else {
                        libGroup.addModule(module);
                    }
                }
            }
            return true;
        } else {
            if ( log ) {
                *log    << string(tab,'\t') << "in " << fileName << " - FILE NOT FOUND\n"
                        << string(tab+1,'\t') << "getDasRoot()='" << getDasRoot() << "'\n";
            }
            missing.push_back({fileName,chain});
            return false;
        }
    }

    // PARSER

    extern "C" int64_t ref_time_ticks ();
    extern "C" int get_time_usec (int64_t reft);
    extern "C" int64_t ref_time_delta_to_usec (int64_t reft);

    static DAS_THREAD_LOCAL int64_t totParse = 0;
    static DAS_THREAD_LOCAL int64_t totInfer = 0;
    static DAS_THREAD_LOCAL int64_t totOpt = 0;
    static DAS_THREAD_LOCAL int64_t totM = 0;

    bool trySerializeProgramModule (
            ProgramPtr          & program,
            const FileAccessPtr & access,
            const string        & fileName,
            ModuleGroup         & libGroup,
            TextWriter & logs ) {
        auto & serializer_read = daScriptEnvironment::bound->serializer_read;
        auto & serializer_write = daScriptEnvironment::bound->serializer_write;

        if ( serializer_read == nullptr || serializer_read->seenNewModule ) {
            return false;
        }

        int64_t file_mtime = access->getFileMtime(fileName.c_str());
        int64_t saved_mtime = 0; *serializer_read << saved_mtime;
        string saved_filename{}; *serializer_read << saved_filename;

        if ( saved_filename != fileName || file_mtime != saved_mtime ) {
            serializer_read->seenNewModule = true;
            if (saved_filename != fileName) {
                logs << "ser: file name mismatch. Expected '" << saved_filename << "', got '" << fileName << "'\n";
            }
            if (file_mtime != saved_mtime) {
                logs << "ser: file mtime mismatch. Expected " << saved_mtime << ", got " << file_mtime << "\n";
            }
            serializer_read->failed = true;
            return false;
        }

        serializer_read->thisModuleGroup = &libGroup;
        serializer_read->serializeProgram(program, libGroup);

        if ( program->failed()) {
            serializer_read->seenNewModule = true;
            serializer_read->failed = true;
            program = make_smart<Program>();
            logs << "ser: program failed '" << fileName << "'\n";
            return false;
        }

        program->thisModuleGroup = &libGroup;

        if ( serializer_read->failed ) {
            serializer_read->seenNewModule = true;
            program = make_smart<Program>();
            logs << "ser: serialization failed. Internal issue.\n";
            return false;
        }

        if ( serializer_write != nullptr ) {
            serializer_write->parsedModules.push_back({fileName, file_mtime, program, program->thisModule.get()});
        }

        return true;
    }

    bool detectGen2Syntax ( const char * text, uint32_t length ) {
        // we search for #gen2#, and return true if its there
        // we skip /* */ and // comments
        bool in_single_line_comment = false;
        bool in_multi_line_comment = false;
        for (uint32_t i = 0; i < length; ++i) {
            if (in_single_line_comment) {
                if (text[i] == '\n') {
                    in_single_line_comment = false;
                }
                continue;
            }
            if (in_multi_line_comment) {
                if (text[i] == '*' && i + 1 < length && text[i + 1] == '/') {
                    in_multi_line_comment = false;
                    ++i;
                }
                continue;
            }
            if (text[i] == '/' && i + 1 < length) {
                if (text[i + 1] == '/') {
                    in_single_line_comment = true;
                    ++i;
                    continue;
                } else if (text[i + 1] == '*') {
                    in_multi_line_comment = true;
                    ++i;
                    continue;
                }
            }
            // check for options\s*gen2
            if (text[i] == 'o' && i + 7 < length && text[i + 1] == 'p' && text[i + 2] == 't' && text[i + 3] == 'i' && text[i + 4] == 'o' && text[i + 5] == 'n' && text[i + 6] == 's' && isspace(text[i + 7]) ) {
                i += 7;
                while (i < length && isspace(text[i])) {
                    ++i;
                }
                if (i + 4 < length && text[i] == 'g' && text[i + 1] == 'e' && text[i + 2] == 'n' && text[i + 3] == '2') {
                    return true;
                }
            }
        }
        return false;
    }

    ProgramPtr parseDaScript ( const string & fileName,
                               const string & moduleName,
                              const FileAccessPtr & access,
                              TextWriter & logs,
                              ModuleGroup & libGroup,
                              bool exportAll,
                              bool isDep,
                              CodeOfPolicies policies ) {
        ProgramPtr program = make_smart<Program>();
        program->thisModule->name = moduleName;
        ReuseCacheGuard rcg;
        auto time0 = ref_time_ticks();

        if ( trySerializeProgramModule(program, access, fileName, libGroup, logs) ) {
            return program;
        }

        int err;
        daScriptEnvironment::bound->g_Program = program;
        daScriptEnvironment::bound->g_compilerLog = &logs;
        program->promoteToBuiltin = false;
        program->isCompiling = true;
        program->isDependency = isDep;
        program->needMacroModule = false;
        program->policies = policies;
        program->thisModuleGroup = &libGroup;
        program->thisModuleName = program->thisModule->name;
        libGroup.foreach([&](Module * pm){
            program->library.addModule(pm);
            return true;
        },"*");
        DasParserState parserState;
        parserState.g_Access = access;
        parserState.g_Program = program;
        parserState.das_def_tab_size = daScriptEnvironment::bound->das_def_tab_size;
        parserState.das_gen2_make_syntax = policies.gen2_make_syntax;
        yyscan_t scanner = nullptr;
        int64_t file_mtime = access->getFileMtime(fileName.c_str());
        if ( auto fi = access->getFileInfo(fileName) ) {
            parserState.g_FileAccessStack.push_back(fi);
            const char * src = nullptr;
            uint32_t len = 0;
            fi->getSourceAndLength(src,len);
            bool gen2 = policies.version_2_syntax || detectGen2Syntax(src, len);
            program->policies.version_2_syntax = gen2;
            if ( gen2 ) {
                das2_yylex_init_extra(&parserState, &scanner);
            } else {
                das_yylex_init_extra(&parserState, &scanner);
            }
            if ( isUtf8Text(src, len) ) {
                if ( gen2 ) {
                    das2_yybegin(src + 3, len-3, scanner);
                } else {
                    das_yybegin(src + 3, len-3, scanner);
                }
            } else {
                if ( gen2 ) {
                    das2_yybegin(src, len, scanner);
                } else {
                    das_yybegin(src, len, scanner);
                }
            }
            libGroup.foreach([&](Module * mod){
                if ( mod->commentReader ) {
                    parserState.g_CommentReaders.push_back(mod->commentReader.get());
                }
                return true;
            },"*");
            if ( gen2 ) {
                err = das2_yyparse(scanner);
                das2_yylex_destroy(scanner);
            } else {
                err = das_yyparse(scanner);
                das_yylex_destroy(scanner);
            }
        } else {
            program->error(fileName + " not found", "","",LineInfo());
            program->isCompiling = false;
            daScriptEnvironment::bound->g_Program.reset();
            daScriptEnvironment::bound->g_compilerLog = nullptr;
            return program;
        }
        parserState = DasParserState();
        totParse += get_time_usec(time0);
        if ( err || program->failed() ) {
            daScriptEnvironment::bound->g_Program.reset();
            daScriptEnvironment::bound->g_compilerLog = nullptr;
            sort(program->errors.begin(),program->errors.end());
            program->isCompiling = false;
            return program;
        } else {
            program->thisModule->doNotAllowUnsafe = !access->canModuleBeUnsafe(program->thisModule->name, fileName);
            if ( policies.solid_context || program->options.getBoolOption("solid_context",false) ) {
                program->thisModule->isSolidContext = true;
            }
            auto timeI = ref_time_ticks();
            restartInfer: program->inferTypes(logs, libGroup);
            if ( policies.macro_context_collect ) libGroup.collectMacroContexts();
            totInfer += get_time_usec(timeI);
            if ( !program->failed() ) {
                program->buildAccessFlags(logs);    // this is used by the lint pass
                if ( program->patchAnnotations() ) {
                    goto restartInfer;
                }
            }
            if ( !program->failed() ) {
                program->normalizeOptionTypes();
                if (!program->failed())
                    program->lint(logs, libGroup);
                if ( policies.macro_context_collect ) libGroup.collectMacroContexts();
                program->foldUnsafe();
                auto timeO = ref_time_ticks();
                if (program->getOptimize()) {
                    program->optimize(logs,libGroup);
                } else {
                    program->buildAccessFlags(logs);
                }
                if ( policies.macro_context_collect ) libGroup.collectMacroContexts();
                totOpt += get_time_usec(timeO);
                if (!program->failed())
                    program->verifyAndFoldContracts();
                if (!program->failed()) {
                    if ( program->thisModule->isModule || exportAll ) {
                        program->markModuleSymbolUse();
                    } else {
                        program->markExecutableSymbolUse();
                        program->removeUnusedSymbols();
                    }
                }
                if (!program->failed())
                    program->fixupAnnotations();
                if (!program->failed())
                    program->deriveAliases(logs,true,true);
                if (!program->failed())
                    program->allocateStack(logs,true,true);
                if (!program->failed())
                    program->finalizeAnnotations();
                if (!program->failed())
                    program->updateSemanticHash();
                if ( policies.macro_context_collect ) libGroup.collectMacroContexts();
            }
            if (!program->failed()) {
                if (program->options.getBoolOption("log")) {
                    logs << *program;
                }
            }
            daScriptEnvironment::bound->g_compilerLog = nullptr;
            sort(program->errors.begin(), program->errors.end());
            program->isCompiling = false;
            if ( !program->failed() ) {
                if ( program->needMacroModule ) {
                    if ( !program->thisModule->isModule ) { // checking if its a module
                        program->error("Module " + fileName + " is not setup correctly for macros",
                            "module Module_Name is required", "", LineInfo(),
                                CompilationError::module_does_not_have_a_name);
                    }
                    auto timeM = ref_time_ticks();
                    if (!program->failed())
                        program->markMacroSymbolUse();
                    if (!program->failed())
                        program->deriveAliases(logs,true,false);
                    if (!program->failed())
                        program->allocateStack(logs,true,false);
                    if (!program->failed())
                        program->makeMacroModule(logs);
                    totM += get_time_usec(timeM);
                }
            }
            daScriptEnvironment::bound->g_Program.reset();
            if ( policies.macro_context_collect ) libGroup.collectMacroContexts();
            if ( program->options.getBoolOption("log_compile_time",policies.log_compile_time) ) {
                auto dt = get_time_usec(time0) / 1000000.;
                logs << "compiler took " << dt << ", " << fileName << "\n";
            }
            auto & serializer_write = daScriptEnvironment::bound->serializer_write;
            if ( serializer_write != nullptr ) {
                serializer_write->parsedModules.push_back({fileName, file_mtime, program, program->thisModule.get()});
            }
            return program;
        }
    }

    bool addExtraDependency(
        string modName,
        string modFile,
        vector<RequireRecord> & missing,
        vector<RequireRecord> & circular,
        vector<RequireRecord> & notAllowed,
        vector<ModuleInfo> & req,
        das_set<string> & dependencies,
        const FileAccessPtr & access,
        ModuleGroup & libGroup,
        CodeOfPolicies policies,
        TextWriter * log = nullptr) {
        bool hasModule = false;
        for ( auto & mod : req ) {
            if ( mod.moduleName==modName) {
                hasModule = true;
                break;
            }
        }
        if ( !hasModule && !modFile.empty() ) {
            vector<FileInfo *> chain;
            if ( !getPrerequisits(modFile, access, req, missing, circular, notAllowed, chain,
                dependencies, libGroup, nullptr, 1, !policies.ignore_shared_modules) ) {
                if ( log ) {
                    *log << "failed to add extra dependency " << modName << " from " << modFile << "\n";
                }
                return false;
            }
            auto finfo = access->getFileInfo(modFile);
            ModuleInfo info;
            info.fileName = finfo->name;
            info.importName = "";
            info.moduleName = modName;
            req.push_back(info);
        }
        return true;
    }

    bool aotModuleHasName ( ProgramPtr program, const ModuleInfo & mod ) {
        if ( bool no_aot = program->options.getBoolOption("no_aot",false); no_aot )
            return true;
        if ( !program->thisModule->name.empty() )
            return true;
        program->error("Module " + mod.moduleName + " is not setup correctly for AOT",
            "module " + mod.moduleName + " is required", "", LineInfo(),
                CompilationError::module_does_not_have_a_name);
        return false;
    }

    void addNewModules ( ModuleGroup & libGroup, ProgramPtr program ) {
        libGroup.addModule(program->thisModule.release());
        program->library.foreach([&](Module * pm) -> bool {
            if ( !pm->name.empty() && pm->name!="$" ) {
                if ( !libGroup.findModule(pm->name) ) {
                    libGroup.addModule(pm);
                }
            }
            return true;
        }, "*");
    }

    bool canShareModule ( ProgramPtr program ) {
        // Check if all dependencies are shared too
        bool regFromShar = false;
        for ( auto & reqM : program->thisModule->requireModule ) {
            if ( !reqM.first->builtIn ) {
                program->error("Shared module " + program->thisModule->name + " has incorrect dependency type.",
                    "Can't require " + reqM.first->name + " because its not shared", "", LineInfo(),
                        CompilationError::module_required_from_shared);
                regFromShar = true;
            }
        }
        return !regFromShar;
    }

    void writebackModules ( ModuleGroup & libGroup ) {
        auto & serializer_write = daScriptEnvironment::bound->serializer_write;
        for ( auto & parsedModule : serializer_write->parsedModules ) {
            auto & [fileName, fileMtime, program, thisModule] = parsedModule; // parsedModule is tuple<string, int64_t, ProgramPtr, Module *>
            *serializer_write << fileMtime;
            *serializer_write << const_cast<string &>(fileName);
            if ( program->thisModule && program->thisModule->name.empty() )  {
                serializer_write->serializeProgram(program, libGroup);
            } else {
                // set thisModule to program
                program->thisModule.reset(thisModule);
                serializer_write->serializeProgram(program, libGroup);
                program->thisModule.release();
            }
        }
    }

    void addRttiRequireVariable ( ProgramPtr res, string fileName ) {
        TextWriter ss;
        for ( const auto & arq : res->allRequireDecl ) {
            ss << get<1>(arq) << " ";
        }
        ss << fileName;
        auto rtti_require = make_smart<Variable>();
        rtti_require->name = "__rtti_require";
        rtti_require->type = make_smart<TypeDecl>(Type::tString);
        rtti_require->init = make_smart<ExprConstString>(ss.str());
        rtti_require->init->type = make_smart<TypeDecl>(Type::tString);
        rtti_require->used = true;
        rtti_require->private_variable = true;
        res->thisModule->addVariable(rtti_require);
    }

    void reportChain ( TextWriter & tw, const vector<FileInfo *> & chain ) {
        bool first = true;
        for ( auto fi = chain.rbegin(); fi != chain.rend(); ++fi ) {
            if ( *fi ) {
                tw << "\t" << (first ? "from " : "required from ") << (*fi)->name << "\n";
            }
            first = false;
        }
    }

    ProgramPtr reportPrerequisitesErrors (
            string fileName,
            vector<RequireRecord> & missing,
            vector<RequireRecord> & circular,
            vector<RequireRecord> & notAllowed,
            vector<ModuleInfo> & req,
            das_set<string> & dependencies,
            const FileAccessPtr & access,
            ModuleGroup & libGroup,
            CodeOfPolicies policies ) {
        TextWriter tw;
        req.clear();
        missing.clear();
        circular.clear();
        dependencies.clear();
        notAllowed.clear();
        vector<FileInfo *> chain;
        getPrerequisits(fileName, access, req, missing, circular, notAllowed, chain, dependencies, libGroup, &tw, 1, false);
        auto program = make_smart<Program>();
        program->policies = policies;
        program->thisModuleGroup = &libGroup;
        TextWriter err;
        for ( auto & mis : missing ) {
            err << "missing prerequisit " << mis.name << "\n";
            reportChain(err, mis.chain);
        }
        for ( auto & mis : circular ) {
            err << "circular dependency " << mis.name << "\n";
            reportChain(err, mis.chain);
        }
        for ( auto & mis : notAllowed ) {
            err << "module not allowed " << mis.name << "\n";
            reportChain(err, mis.chain);
        }
        program->error(err.str(), "", "", LineInfo(),
                        CompilationError::module_not_found);
        return program;
    }

    void disableSerializationOnDebugger ( vector<ModuleInfo> & req ) {
        if ( daScriptEnvironment::bound->serializer_read == nullptr )
            return;
        for ( auto & mod : req ) {
            if ( mod.fileName.find("daslib/debug") != string::npos ) {
                auto & serializer_read = daScriptEnvironment::bound->serializer_read;
                auto & serializer_write = daScriptEnvironment::bound->serializer_read;
                serializer_read = serializer_write = nullptr;
                break;
            }
        }
    }

    bool verifyModuleNamesUnique ( const vector<ModuleInfo> & req, TextWriter & logs ) {
        das_hash_map<string, string> fullName;
        for ( auto & r : req ) {
            if ( fullName.find(r.moduleName) != fullName.end() ) {
                logs << "several modules with the name " << r.moduleName << "\n" <<
                        "namely " << r.fileName << "\n\tand " << fullName[r.moduleName] << "\n";
                return false;
            }
            fullName[r.moduleName] = r.fileName;
        }
        return true;
    }

    ProgramPtr compileDaScript ( const string & fileName,
                                const FileAccessPtr & access,
                                TextWriter & logs,
                                ModuleGroup & libGroup,
                                CodeOfPolicies policies ) {
        ReuseCacheGuard rcg;
        bool exportAll = policies.export_all;
        auto time0 = ref_time_ticks();
        totParse = 0;
        totInfer = 0;
        totOpt = 0;
        totM = 0;
        daScriptEnvironment::bound->macroTimeTicks = 0;
        vector<ModuleInfo> req;
        vector<RequireRecord> missing, circular, notAllowed;
        vector<FileInfo *> chain;
        das_set<string> dependencies;
        uint64_t preqT = 0;
        if ( getPrerequisits(fileName, access, req, missing, circular, notAllowed, chain,
                dependencies, libGroup, nullptr, 1, !policies.ignore_shared_modules) ) {
            preqT = get_time_usec(time0);
            disableSerializationOnDebugger(req);
            bool allGood = true;
            if ( policies.debugger ) {
                allGood = addExtraDependency("debug", policies.debug_module, missing, circular, notAllowed, req, dependencies, access, libGroup, policies, &logs) && allGood;
            } else if ( policies.profiler ) {
                allGood = addExtraDependency("profiler", policies.profile_module, missing, circular, notAllowed, req, dependencies, access, libGroup, policies, &logs) && allGood;
            } /* else */ if ( policies.jit ) {
                allGood = addExtraDependency("just_in_time", policies.jit_module, missing, circular, notAllowed, req, dependencies, access, libGroup, policies, &logs) && allGood;
            }
            if ( !allGood ) {
                return make_smart<Program>();
            }
            if ( !verifyModuleNamesUnique(req, logs) ) {
                return make_smart<Program>();
            }
            for ( auto & mod : req ) {
                if ( libGroup.findModule(mod.moduleName) ) {
                    continue;
                }
                auto program = parseDaScript(mod.fileName, mod.moduleName, access, logs, libGroup, true, true, policies);
                policies.threadlock_context |= program->options.getBoolOption("threadlock_context",false);
                if ( program->failed() ) {
                    return program;
                }
                if ( policies.fail_on_lack_of_aot_export && !aotModuleHasName(program, mod) ) {
                    return program;
                }
                if ( program->thisModule->name.empty() ) {
                    program->thisModule->name = mod.moduleName;
                    program->thisModule->wasParsedNameless = true;
                }
                program->thisModule->fileName = mod.fileName;
                if ( program->promoteToBuiltin ) {
                    if ( canShareModule(program) ) {
                        program->thisModule->promoteToBuiltin(access);
                    } else {
                        return program;
                    }
                }
                addNewModules(libGroup, program);
            }
            auto & serializer_read = daScriptEnvironment::bound->serializer_read;
            if ( serializer_read && !policies.serialize_main_module ) serializer_read->seenNewModule = true;
            auto res = parseDaScript(fileName, "", access, logs, libGroup, exportAll, false, policies);
            // wirteback all parsed modules from serializer_write
            if ( daScriptEnvironment::bound->serializer_write != nullptr
                && (!daScriptEnvironment::bound->serializer_read || daScriptEnvironment::bound->serializer_read->failed) ) {
                writebackModules(libGroup);
            }
            policies.threadlock_context |= res->options.getBoolOption("threadlock_context",false);
            if ( !res->failed() ) {
                if ( res->options.getBoolOption("log_symbol_use") ) {
                    res->markSymbolUse(false, false, false, nullptr, &logs);
                }
            }
            if ( policies.aot_module && (res->promoteToBuiltin || res->thisModule->isModule || exportAll) ) {
                if (!res->failed()) {
                    if(exportAll)
                        res->markSymbolUse(false,true,true,nullptr);
                    else
                        res->markModuleSymbolUse();
                }
                if (!res->failed() && !exportAll)
                    res->removeUnusedSymbols();
                if (!res->failed())
                    res->deriveAliases(logs,true,false);
                if (!res->failed())
                    res->allocateStack(logs,true,false);
            } else {
                if (!res->failed())
                    res->markExecutableSymbolUse();
                if (res->getDebugger())
                    addRttiRequireVariable(res, fileName);
                if (!res->failed())
                    res->removeUnusedSymbols();
                if (!res->failed())
                    res->deriveAliases(logs,true,false);
                if (!res->failed())
                    res->allocateStack(logs,true,false);
            }
            if ( res->options.getBoolOption("log_require",false) ) {
                TextWriter tw;
                req.clear();
                missing.clear();
                circular.clear();
                notAllowed.clear();
                dependencies.clear();
                getPrerequisits(fileName, access, req, missing, circular, notAllowed, chain, dependencies, libGroup, &tw, 1, false);
                logs << "module dependency graph:\n" << tw.str();
            }
            if ( !res->failed() ) {
                auto hf = hash_blockz64((uint8_t *)fileName.c_str());
                res->thisNamespace = "_anon_" + to_string(hf);
            }
            if ( res->options.getBoolOption("log_total_compile_time",policies.log_total_compile_time) ) {
                auto totT = get_time_usec(time0);
                logs << "compiler took " << (totT  / 1000000.) << ", " << fileName << "\n"
                     << "\trequire  " << (preqT    / 1000000.) << "\n"
                     << "\tparse    " << (totParse / 1000000.) << "\n"
                     << "\tinfer    " << (totInfer / 1000000.) << "\n"
                     << "\toptimize " << (totOpt   / 1000000.) << "\n"
                     << "\tmacro    " << (ref_time_delta_to_usec(daScriptEnvironment::bound->macroTimeTicks)  / 1000000.) << "\n"
                     << "\tmacro mods " << (totM     / 1000000.) << "\n"
                ;
            }
            return res;
        } else {
            return reportPrerequisitesErrors(fileName, missing, circular, notAllowed,
                                        req, dependencies, access, libGroup, policies);
        }
    }
}

