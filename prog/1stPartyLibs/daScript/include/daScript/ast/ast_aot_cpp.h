#ifndef DAS_AST_AOT_CPP_H
#define DAS_AST_AOT_CPP_H

#include "daScript/daScript.h"

namespace das {
    inline bool saveToFile ( TextPrinter &tout, const string_view & fname, const string_view & str, bool quiet = false ) {
        if ( !quiet )  {
            tout << "saving to " << fname.data() << "\n";
        }
        FILE * f = fopen ( fname.data(), "w" );
        if ( !f ) {
            tout << "can't open " << fname.data() << "\n";
            return false;
        }
        fwrite ( str.data(), str.length(), 1, f );
        fclose ( f );
        return true;
    }

    /**
     * Shortcut to wrap code into namespace
     */
    class NamespaceGuard {
    public:
        NamespaceGuard(TextWriter &tw, string namesp) : tw_(tw), namesp_(das::move(namesp)) {
            tw_ << "namespace " << namesp_ << " {\n";
        }
        ~NamespaceGuard() {
            tw_ << "} // namespace " << namesp_ << "\n";
        }
    private:
        TextWriter &tw_;
        string namesp_;
    };

    /**
     * Shortcut to wrap code into class
     */
    class ClassGuard {
    public:
        ClassGuard(TextWriter &tw, string class_name) : tw_(tw) {
            tw_ << "class " << class_name << " {\n";
        }
        ~ClassGuard() {
            tw_ << "};\n";
        }
    private:
        TextWriter &tw_;
    };

    struct StandaloneContextCfg {
        string context_name;
        string class_name;
        bool cross_platform = false;
    };

    /**
     * Enumerate all aot functions
     */
    void dumpRegisterAot(TextWriter& tw, ProgramPtr program, Context & context, bool allModules = false);

    /**
     * Create standalone context
     * @param program
     * @param cppOutputDir output directory where context will be created
     * @param standaloneContextName class name
     */
    void runStandaloneVisitor(ProgramPtr program, const string& cppOutputDir, const StandaloneContextCfg &cfg);
}

#endif //DAS_AST_AOT_CPP_H
