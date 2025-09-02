#include "daScript/ast/ast.h"
#include "daScript/daScriptModule.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/misc/platform.h"
#include "fmt.h"

using namespace das;


void InitModules() {
    // register modules
    if (!Module::require("$")) {
        NEED_MODULE(Module_BuiltIn);
    }
    if (!Module::require("math")) {
        NEED_MODULE(Module_Math);
    }
    if (!Module::require("raster")) {
        NEED_MODULE(Module_Raster);
    }
    if (!Module::require("strings")) {
        NEED_MODULE(Module_Strings);
    }
    if (!Module::require("rtti")) {
        NEED_MODULE(Module_Rtti);
    }
    if (!Module::require("ast")) {
        NEED_MODULE(Module_Ast);
    }
    if (!Module::require("jit")) {
        NEED_MODULE(Module_Jit);
    }
    if (!Module::require("debugapi")) {
        NEED_MODULE(Module_Debugger);
    }
    NEED_MODULE(Module_Network);
    NEED_MODULE(Module_UriParser);
    NEED_MODULE(Module_JobQue);
    NEED_MODULE(Module_FIO);
    NEED_MODULE(Module_DASBIND);
#include "modules/external_need.inc"
    Module::Initialize();
    // compile and run

}

string help() {
    return "Tool to convert dascript v1 syntax to v2\n"
           "das-fmt {-i} filename1 {filename2} ...:\n"
           "   -i inplace conversion, write to the same file. Multiple filenames only allowed in inplace mode\n"
           "   --tests Run tests, no filenames required\n"
           "   --semicolon Keep semicolon after convertion\n"
           "";
}


struct TestData {
    string original;
    string expected;
    format::FormatOptions options = {};
};

vector<TestData> test_cases() {
    const string in_prefix = "[safe_when_uninitialized]\n"
                             "struct Foo\n"
                               "    a : int = 5\n"
                               "    b : float = 3.0\n"
                               "struct Bar\n"
                               "    a : Foo\n"
                               "var var1 = ";
    const string out_prefix = in_prefix;

    const string postfix = "\n";
    vector<TestData> base = {
        {"[[/**/Foo/*0*/a/*a*/=/**/1/*a*///abc\n,//dsa\n/*dsa*/\nb=2.0/**/\n//dsa\n]]",
                                                                "/**/Foo(/*0*/a/*a*/=/**/1/*a*///abc\n,//dsa\n/*dsa*/\nb=2.0/**/\n//dsa\n)"}, // 1 // no uninit because it's redundant
        {"[[/**/Foo/**/]]",                                     "Foo(/**//**/)"}, // 2
        {"[[/*a*/Foo(/*b*/)/*c*/]]",                            "Foo(/*a*//*b*//*c*/)"}, // 3
        {"[[/*a*/Foo(/*b*/)/*c*/ a=1/*d*/,/*e*/b=2.0/*f*/]]",   "/*a*//*b*/Foo(/*c*/ a=1/*d*/,/*e*/b=2.0/*f*/)"}, // 4
        {"[[/*a*/auto/*b*/1/*c*/,/*d*/2/*e*/]]",                "/*a*//*b*/(1/*c*/,/*d*/2/*e*/)"}, // 13
//        {"[[for x in [1, 20]; x*x; where x%2 == 0]];", "[iterator for x in [1, 20]; x*x; where x%2 == 0];"}, // 5 // each result is discarded, which is unsaf
        {"[{/*a*/Foo/*b*/a=1/*c*/,/*d*/b=2./*e*/;/*f*/a=2/*g*/,/*h*/b=1./*i*/}]",
                                                                "[/*a*/Foo(/*b*/a=1/*c*/,/*d*/b=2./*e*/), Foo(/*f*/a=2/*g*/,/*h*/b=1./*i*/)]"}, // 6
        {"[{/*a*/Foo()/*b*/a=1/*c*/,/*d*/b=2./*e*/;/*g*/a=2/*h*/,/*i*/b=1./*j*/}]",
                                                                "[/*a*/Foo(/*b*/a=1/*c*/,/*d*/b=2./*e*/), Foo(/*g*/a=2/*h*/,/*i*/b=1./*j*/)]"}, // 7
//        {"[{Foo a=1,b=2.;a=2,b=1. <optional_block>}]", "[Foo(a=1,b=2.),Foo(a=2,b=1.)]"}, // what about optional block in new syntax
        {"[{/*a*/auto/*b*/1/*c*/;/*d*/2/*e*/;/*f*/3/*g*/;/*h*/4/*i*/}]",
                                                                "[/*a*//*b*/1/*c*/,/*d*/2/*e*/,/*f*/3/*g*/,/*h*/4/*i*/]"}, // 8
        {"[{/*a*/auto/*b*/1/*c*/,/*d*/2.2/*e*/}]",              "[/*a*//*b*/(1/*c*/,/*d*/2.2/*e*/)]"}, // 8
        {"[{/*a*/for/*b*/x/*c*/in/*d*/0..10/*e*/;/*f*/x*x/*g*/;/*h*/where/*i*/x%2==0/*j*/}]",
                                                                "[/*a*/for/*b*/x/*c*/in/*d*/0..10/*e*/;/*f*/x*x/*g*/;/*h*/where/*i*/x%2==0/*j*/]"}, // 9
        {"{{/*a*/for/*b*/x/*c*/in/*d*/0..10/*e*/;/*f*/x*x/*g*/;/*h*/where/*i*/x%2==0/*j*/}}",
                                                                "[/*a*/for/*b*/x/*c*/in/*d*/0..10/*e*/;/*f*/x*x/*g*/;/*h*/where/*i*/x%2==0/*j*/]"}, // 12
        {"{{/*a*/1/*b*/;/*c*/2/*d*/;/*e*/3/*f*/;/*g*/4/*h*/}}", "{/*a*/1/*b*/,/*c*/2/*d*/,/*e*/3/*f*/,/*g*/4/*h*/}"}, // 10
        {R"({{/*a*/1=>"a"/*b*/;/*c*/2=>"b"/*d*/;/*e*/3=>"c"/*f*/;/*g*/4=>"d"/*h*/}})",
                                                                R"({/*a*/1=>"a"/*b*/,/*c*/2=>"b"/*d*/,/*e*/3=>"c"/*f*/,/*g*/4=>"d"/*h*/})"}, // 10
        {R"([[/*a*/auto/*b*/1/*c*/,/*d*/2./*e*/,/*f*/"3"/*g*/;/*h*/1/*i*/,/*j*/4./*k*/,/*l*/"2"/*m*/]])",
                                                                R"(fixed_array(/*a*//*b*/(1/*c*/,/*d*/2./*e*/,/*f*/"3"/*g*/),/*h*/(1/*i*/,/*j*/4./*k*/,/*l*/"2"/*m*/)))"}, // 13
//
//        // anything
        {"[[/*a*/auto/*b*/1/*c*/;/*d*/2/*e*/]]",
                                                                "fixed_array(/*a*//*b*/1/*c*/,/*d*/2/*e*/)"},
        {"[[/*a*/Foo?/*b*/]]",                                  "default<Foo?>/*a*//*b*/"},
        {"[[/*a*/Foo#/*b*/]]",                                  "struct<Foo#>(/*a*//*b*/)"},
        {"[[tuple<string; float>\n"
         "    \"a\", 1.0;\n"
         "    \"b\", 2.0;"
         "]]",
         "fixed_array<tuple<string; float>>(\n"
         "    (\"a\", 1.0),\n"
         "    (\"b\", 2.0))"},
        {
            "[[auto[] \"a\" ]]\n",
            "fixed_array( \"a\" )\n",
        },


        // nested

        {"[[/*a*/Bar/*b*/a=[[/*c*/Foo/*f*/b=2.0/*g*/]]/*h*/]]",
         "/*a*/Bar(/*b*/a=/*c*/Foo(/*f*/b=2.0/*g*/)/*h*/)"},
    };
    for (auto &[in, out, options]: base) {
        in = in_prefix + in + postfix;
        out = out_prefix + out + postfix;
    }

    vector<TestData> tuple_expansion = {
        // tuple expansion (doesn't work in global scope!)
        {"def main()\n    var [[/*a*/a/*b*/,/*c*/b/*d*/]] = (123, 321)",
            "def main()\n    var (/*a*/a/*b*/,/*c*/b/*d*/) = (123, 321)"},
        {"def main()\n    var [[/*a*/a/*b*/,/*c*/b/*d*/]]: tuple<int, int> = (123, 321)",
            "def main()\n    var (/*a*/a/*b*/,/*c*/b/*d*/): tuple<int, int> = (123, 321)"},
    };

    vector<TestData> braces_tests = {
        // test braces
        {"typedef \n    A = int\n    B = string",            "typedef A = int;\ntypedef B = string;\n"},
        {"def b()/**/\n    /**/let a = 5",                   "def b()/**/ {\n    /**/let a = 5;\n}"},
        {"def b()/**/\n    /**/let a = 5",                   "def b()/**/ {\n    /**/let a = 5;\n}"},
        {"def b() /**/\n    let a = 5/**/;\n",             "def b() /**/ {\n    let a = 5/**/;\n}\n"},
        {"def b(it)\n    let x = typeinfo is_iterable (it)", "def b(it) {\n    let x = typeinfo is_iterable (it);\n}"},

        {"class C\n"
         "    a : int = 5\n"
         "class A : C\n"
         "    c : string = \"add_new_call_macro\"",
                                                             "class C {\n"
                                                             "    a : int = 5;\n"
                                                             "}\n"
                                                             "class A : C {\n"
                                                             "    c : string = \"add_new_call_macro\";\n"
                                                             "}",
        },
        {"def main()//aa\n    /**/let x = 1    // 123",      "def main() {//aa\n    /**/let x = 1;    // 123\n}"},
        {"bitfield A\n    refCount",                     "bitfield A {\n    refCount\n}"},

        {"def b(x, y)\n    for x in y\n        x = x + 1",
                                                             "def b(x, y) {\n    for (x in y) {\n        x = x + 1;\n    }\n}"},
        {"def b()\n    let a = 5\n    if a<0\n        a = a + 1",
                                                             "def b() {\n    let a = 5;\n    if (a<0) {\n        a = a + 1;\n    }\n}"},
        {"def lower_bound ( a:array<auto(TT)>; val : TT const-& )\n"
         "    // comment \n"
         "    return lower_bound(a,0,length(a),val)\n"
         "let x = 1;",
                                                             "def lower_bound ( a:array<auto(TT)>; val : TT const-& ) {\n"
                                                             "    // comment \n"
                                                             "    return lower_bound(a,0,length(a),val);\n"
                                                             "}\n"
                                                             "let x = 1;"},
        {
         "bitfield/*a*/Test// b\n"
         "    one //c\n"
         "    two //d\n"
         "    three//e",
                                                             "bitfield/*a*/Test {// b\n"
                                                             "    one //c\n"
                                                             "    two //d\n"
                                                             "    three//e\n"
                                                             "}"},
        {
            "enum/*a*/Test/*b*/:/*c*/int// a\n"
            "    b// d\n",
            "enum/*a*/Test/*b*/:/*c*/int {// a\n"
            "    b// d\n"
            "}\n"
        },
        {
            "def foo(x){}\n"
            "def main() {\n"
            "    foo() $() {} \n"
            "}\n",
            "def foo(x){}\n"
            "def main() {\n"
            "    foo($() {}); \n"
            "}\n"
        },
        {
            "def foo(x, y){}\n"
            "def main() {\n"
            "    foo(123) $() {} \n"
            "}\n",
            "def foo(x, y){}\n"
            "def main() {\n"
            "    foo(123, $() {}); \n"
            "}\n"
        },
        {
            "let x = @ <| () {}\n",
            "let x = @ () {};\n"
        },
        {
            "def test()//a\n"
            "    try//b\n"
            "        for i in range(0, 1)\n"
            "            assert(false)// d\n"
            "    recover//c\n"
            "        assert(false)\n",
            "def test() {//a\n"
            "    try {//b\n"
            "        for (i in range(0, 1)) {\n"
            "            assert(false);// d\n"
            "        }\n"
            "    } recover {//c\n"
            "        assert(false);\n"
            "    }\n"
            "}\n",
        },
        {
            "def f()\n"
            "    assume x = 1",
            "def f() {\n"
            "    assume x = 1;\n"
            "}",
        },
        {
            "def main() {\n"
            "    let x = 1\n"
            "+\n"
            "2;\n"
            "}",
            "def main() {\n"
            "    let x = (1\n"
            "+\n"
            "2);\n"
            "}",
        },
        {
            "def main() {\n"
            "    let x = (1\n"
            "+\n"
            "2 +\n3 + 4);\n"
            "}",
            "def main() {\n"
            "    let x = (1\n"
            "+\n"
            "2 +\n3 + 4);\n"
            "}",
        },
//        {
//            "var css <- @ <| {\n"
//            "    saveLiveContext();\n"
//            "}",
//            "var css <- @ <| $() {\n"
//            "    saveLiveContext();\n"
//            "}",
//        },
    };
    for (auto &[in, out, opt]: braces_tests) {
        opt = format::FormatOptions(das_hash_set<format::FormatOpt>{format::FormatOpt::V2Syntax, format::FormatOpt::SemicolonEOL});
        out = "options gen2;\n" + out;
    }
    vector<TestData> res;
    res.insert(res.end(), base.begin(), base.end());
    res.insert(res.end(), tuple_expansion.begin(), tuple_expansion.end());
    res.insert(res.end(), braces_tests.begin(), braces_tests.end()); // did not implement yet
    return res;
}

int test() {
    TextPrinter tp;
    auto cases = test_cases();
    int ret_code = 0;
    for (const auto &[in, out, cfg]: cases) {
        auto res = transform_syntax("test.das", in, cfg); // any filename
        if (!res.ok) {
            tp << "input:\n" << in << " \noutput:\n" << res.error->content << " \nerror:\n" << res.error->what
                 << "\n";
            ret_code = -1;
        } else if (res.ok != out) {
            tp << " output:\n" << res.ok.value() << "\nexpected:\n" << out << "\n";
            ret_code = -1;
        }
    }
    return ret_code;
}

int main(int argc, char** argv) {
    format::FormatOptions opts;
    bool is_tests = false;
    TextPrinter tp;
    vector<string> files;
    if (argc < 2) {
        tp << help();
        return -1;
    }
    for (int i = 1; i < argc; i++) {
        const string arg = argv[i];
        if (arg.front() != '-') {
            files.emplace_back(arg);
        } else {
            if (arg == "--tests") {
                is_tests = true;
            } else if (arg == "-i" || arg == "--inplace") {
                opts.insert(format::FormatOpt::Inplace);
            } else if (arg == "-v2" || arg == "--v2") {
                opts.insert(format::FormatOpt::V2Syntax);
            } else if (arg == "--semicolon") {
                opts.insert(format::FormatOpt::SemicolonEOL);
            } else {
                tp << help();
            }
        }
    }
    if (!is_tests && files.empty()) {
        tp << "no files" << "\n";
    }
    InitModules();
    if (is_tests) {
        return test();
    } else {
        return das::format::run(opts, files);
    }
}