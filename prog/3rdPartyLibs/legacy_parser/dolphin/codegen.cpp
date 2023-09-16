
#include "dolphin.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "codegen.h"
#include "tables.h"
#include "stl.h"
#include "utilities.h"
using namespace std;
using namespace Whale;

void generate_terminals(FILE *a);
void generate_dolphin_class(FILE *a);
void generate_scalar_constants(FILE *a, const char *indent, bool it_is_cpp);
void generate_get_token_function(FILE *a);
void generate_table_of_states(FILE *a);
void generate_table_of_lines(FILE *a);
void generate_table_of_symbol_classes(FILE *a);
void generate_table_of_lookahead_states(FILE *a);
void generate_table_of_actions(FILE *a);
void generate_whale_emulator(FILE *a, int mode);
void print_parametrized_string(FILE *a, const char *s, const char *parameters, const char *values[]);
void generate_action(FILE *a, const char *indent, ActionData &action, bool break_after);
void generate_actions_for_special_expression(FILE *a, const char *indent, RecognizedExpressionData &re, bool break_after);
bool type_is_a_pointer(const char *s);
bool type_should_be_printed_without_space(const char *s);
void insert_cpp_code_in_file(FILE *a, const char *indent, Terminal *code);

// the generated code will have all state and action numbers incremented by
// one - to make 0 rather than -1 an error marker.

static std::string term_namespace_qual,cpp_glob_decl,h_glob_decl;

void generate_code()
{
        string h_file_name=DolphinData::data.file_name+string(".h");
        string h_file_name_short = h_file_name;
        std::size_t s_found = h_file_name_short.rfind('/');
        if (s_found != std::string::npos)
          h_file_name_short.erase(0, s_found+1);
        s_found = h_file_name_short.rfind('\\');
        if (s_found != std::string::npos)
          h_file_name_short.erase(0, s_found+1);

        string cpp_file_name=DolphinData::data.file_name+string(".cpp");
        FILE *h_file=fopen(h_file_name.c_str(), "w");
        FILE *cpp_file=fopen(cpp_file_name.c_str(), "w");

        string inclusion_indicator="__LEXICAL_ANALYZER_GENERATED_BY_DOLPHIN__"+convert_file_name_to_identifier(h_file_name_short, 1);

        fprintf(h_file, "%s", FIRST_LINE_FOR_GENERATED_FILES);
        fprintf(cpp_file, "%s", FIRST_LINE_FOR_GENERATED_FILES);


        /* .h */

        if(DolphinData::data.variables.code_in_h_before_all)
                fprintf(h_file, "\n%s\n", DolphinData::data.variables.code_in_h_before_all);

        fprintf(h_file, "\n#ifndef %s\n", inclusion_indicator.c_str());
        fprintf(h_file, "\n#define %s\n", inclusion_indicator.c_str());

        if(!DolphinData::data.variables.input_stream_is_FILE_asterisk)
                fprintf(h_file, "\n#include <iostream>\n");
        else
                fprintf(h_file, "\n#include <stdio.h>\n");
        //fprintf(h_file, "#include <iterator>\n");
        //fprintf(h_file, "#include <vector>\n");
        //fprintf(h_file, "#include <typeinfo>\n");
        //fprintf(h_file, "#include <tab.h>\n");
        //fprintf(h_file, "#include <dyntab.h>\n");
        if(DolphinData::data.variables.generate_sanity_checks)
                fprintf(h_file, "#include <stdexcept>\n");
        //if(DolphinData::data.variables.store_lexeme_in_string)fprintf(h_file, "#include <string>\n");

        fprintf(h_file, "\nclass %s;\n", DolphinData::data.variables.dolphin_class_name);
        //fprintf(h_file, "\n#include \"base_lex.h\"\n");

        if(DolphinData::data.variables.code_in_h)
                fprintf(h_file, "\n%s\n", DolphinData::data.variables.code_in_h);

        if(DolphinData::data.variables.using_whale)
        {
                //if(DolphinData::data.variables.whale_emulation_mode)
                //      generate_whale_emulator(h_file, 1);
                //else
                //if(DolphinData::data.variables.allow_inclusion_cycle_between_whale_and_dolphin)
                //      fprintf(h_file, "\n#include \x22%s\x22\n", DolphinData::data.variables.whale_file);
                //else
                        //fprintf(h_file, "\nnamespace %s { class Terminal; }\n", DolphinData::data.variables.whale_namespace);
        }
        if(DolphinData::data.variables.generate_terminals){
                if(DolphinData::data.variables.generate_terminals_file){
                        string ht_file_name=DolphinData::data.variables.generate_terminals_file+string(".h");
                        FILE *ht_file=fopen(ht_file_name.c_str(),"wt");
                        if(!ht_file){
                                cout<<"Could not write terminal file" << endl;
                        }else{
                                string inclusion_indicator="__TERMINALS_FOR_LEXICAL_ANALYZER_GENERATED_BY_DOLPHIN__"+convert_file_name_to_identifier(ht_file_name, 1);
                                fprintf(ht_file, "\n#ifndef %s\n", inclusion_indicator.c_str());
                                fprintf(ht_file, "\n#define %s\n", inclusion_indicator.c_str());
                                generate_terminals(ht_file);
                                fprintf(ht_file, "\n#endif\n");
                                fclose(ht_file);
                        }
                }else{
                    generate_terminals(h_file);
                }
        }
        if(DolphinData::data.variables.terminals_namespace){
                term_namespace_qual=DolphinData::data.variables.terminals_namespace;
                term_namespace_qual+="::";
        }

        if(DolphinData::data.variables.tokens_prefix){
                term_namespace_qual+=DolphinData::data.variables.tokens_prefix;
        }

        if(DolphinData::data.variables.glob_decl_in_h) {
          h_glob_decl=DolphinData::data.variables.glob_decl_in_h;
          h_glob_decl+=" ";
        }else h_glob_decl="";

        if(DolphinData::data.variables.glob_decl_in_cpp) {
          cpp_glob_decl=DolphinData::data.variables.glob_decl_in_cpp;
          cpp_glob_decl+=" ";
        }else cpp_glob_decl="";


        generate_dolphin_class(h_file);

        //if(DolphinData::data.variables.whale_emulation_mode)
        //      generate_whale_emulator(h_file, 2);

        if(DolphinData::data.variables.code_in_h_after_all)
                fprintf(h_file, "\n%s\n", DolphinData::data.variables.code_in_h_after_all);

        fprintf(h_file, "\n#endif\n");
        fclose(h_file);


        /* .cpp */

        if(DolphinData::data.variables.code_in_cpp_before_all)
                fprintf(cpp_file, "\n%s\n", DolphinData::data.variables.code_in_cpp_before_all);

        fprintf(cpp_file, "\n");
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                fprintf(cpp_file, "#include <utility>\n"); // std::pair is used.
        fprintf(cpp_file, "#include \x22%s\x22\n", h_file_name_short.c_str());
        //if(!DolphinData::data.variables.allow_inclusion_cycle_between_whale_and_dolphin)
              //fprintf(cpp_file, "#include \x22%s.h\x22\n", DolphinData::data.variables.whale_file);
        if(DolphinData::data.variables.generate_terminals_file)
              fprintf(cpp_file, "#include \x22%s.h\x22\n", DolphinData::data.variables.generate_terminals_file);
        if(DolphinData::data.variables.terminals_namespace)
                fprintf(cpp_file, "using namespace %s;\n",DolphinData::data.variables.terminals_namespace);

        fprintf(cpp_file, "using namespace std;\n");

        //fprintf(cpp_file, "\nconst char *%s::dolphin_copyright_notice=\n%s;\n",
        //      DolphinData::data.variables.dolphin_class_name, COPYRIGHT_NOTICE_FOR_GENERATED_FILES);

        if(DolphinData::data.variables.code_in_cpp)
                fprintf(cpp_file, "\n%s\n", DolphinData::data.variables.code_in_cpp);

        fprintf(cpp_file, "\n");
        generate_scalar_constants(cpp_file, "", true);

        fprintf(cpp_file, "\n");
        generate_get_token_function(cpp_file);

        fprintf(cpp_file, "\n");
        fprintf(cpp_file, "%svoid %s::clear_lexeme()\n", cpp_glob_decl.c_str(),DolphinData::data.variables.dolphin_class_name);
        fprintf(cpp_file, "{\n");
        if(DolphinData::data.variables.store_lexeme_in_string)
                fprintf(cpp_file, "\tfor(int i=0; i<lexeme.size(); i++)\n");
        else
                fprintf(cpp_file, "\tfor(int i=0; i<number_of_characters_in_lexeme; i++)\n");
        fprintf(cpp_file, "\t\tinternal_position_counter(buffer[i]);\n");
        fprintf(cpp_file, "\t\n");
        if(DolphinData::data.variables.store_lexeme_in_string)
                fprintf(cpp_file, "\tbuffer.erase(buffer.begin(), buffer.begin()+lexeme.size());\n");
        else
                fprintf(cpp_file, "\tbuffer.erase(buffer.begin(), buffer.begin()+number_of_characters_in_lexeme);\n");
        fprintf(cpp_file, "\t\n");
        if(DolphinData::data.variables.store_lexeme_in_string)
        {
                fprintf(cpp_file, "\tlexeme.clear();\n");
        }
        else
        {
                fprintf(cpp_file, "\tif(lexeme)\n");
                fprintf(cpp_file, "\t{\n");
                fprintf(cpp_file, "\t\tdelete lexeme;\n");
                fprintf(cpp_file, "\t\tlexeme=NULL;\n");
                fprintf(cpp_file, "\t}\n");
        }
        fprintf(cpp_file, "}\n");

        if(DolphinData::data.variables.compress_tables)
                generate_table_of_lines(cpp_file);
        generate_table_of_states(cpp_file);
        generate_table_of_symbol_classes(cpp_file);
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                generate_table_of_lookahead_states(cpp_file);
        if(DolphinData::data.variables.generate_table_of_actions)
                generate_table_of_actions(cpp_file);

        if(DolphinData::data.variables.code_in_cpp_after_all)
                fprintf(cpp_file, "\n%s\n", DolphinData::data.variables.code_in_cpp_after_all);

        fclose(cpp_file);
}

void generate_dolphin_class(FILE *a)
{
        fprintf(a, "\nclass %s:public BaseLexicalAnalyzer\n", DolphinData::data.variables.dolphin_class_name);
        fprintf(a, "{\n");
        //fprintf(a, "\tstatic const char *dolphin_copyright_notice;\n");

        fprintf(a, "\t\n" "public:\n");
        //fprintf(a, "\t\tTab<struct Symbol *> &symbols;\n");
        //fprintf(a, "\t\tDynTab<TerminalContainer> &linear_symbols;\n");

        if(DolphinData::data.variables.code_in_class_before_all)
                fprintf(a, "\t%s\n\t\n", DolphinData::data.variables.code_in_class_before_all);

        generate_scalar_constants(a, "\t", false);

        if(DolphinData::data.variables.start_conditions_enabled)
        {
                fprintf(a, "\t\n"
                        "\tenum StartCondition { ");
                for(int i=0; i<DolphinData::data.start_conditions.size(); i++)
                {
                        if(i) fprintf(a, ", ");
                        fprintf(a, "%s", DolphinData::data.start_conditions[i].name);
                        if(!i) fprintf(a, "=0");
                }
                fprintf(a, " };\n");
        }

        fprintf(a, "\t\n" "protected:\n");

        fprintf(a, "\tstruct StateData\n");
        fprintf(a, "\t{\n");
        if(DolphinData::data.variables.compress_tables)
        {
                if(DolphinData::data.variables.using_layer2)
                {
                        assert(DolphinData::data.variables.table_compression_exception_width==1);
                        fprintf(a, "\t\tint exception_location; // -1 for none\n");
                        fprintf(a, "\t\t%s exception_data;\n", DolphinData::data.variables.analyzer_state_type);
                }

                fprintf(a, "\t\tconst %s *transitions;\n", DolphinData::data.variables.analyzer_state_type);
        }
        else
                fprintf(a, "\t\t%s transitions[number_of_symbol_classes];\n", DolphinData::data.variables.analyzer_state_type);
        if(DolphinData::data.variables.start_conditions_enabled)
        {
                if(DolphinData::data.variables.compress_action_vectors)
                        fprintf(a, "\t\tint *action_upon_accept;\n");
                else
                        fprintf(a, "\t\tint action_upon_accept[number_of_start_conditions];\n");
        }
        else
                fprintf(a, "\t\tint action_upon_accept;\n");
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                fprintf(a, "\t\tbool is_lookahead_state;\n");
        if(DolphinData::data.variables.access_transitions_through_a_method)
        {
                fprintf(a, "\t\t\n");
                fprintf(a, "\t\t%s access_transition(int input_symbol) const\n", DolphinData::data.variables.analyzer_state_type);
                fprintf(a, "\t\t{\n");
                fprintf(a, "\t\t\tif(input_symbol==exception_location)\n");
                fprintf(a, "\t\t\t\treturn exception_data;\n");
                fprintf(a, "\t\t\telse\n");
                fprintf(a, "\t\t\t\treturn transitions[input_symbol];\n");
                fprintf(a, "\t\t}\n");
        }
        fprintf(a, "\t};\n");

        if(DolphinData::data.variables.generate_table_of_actions)
        {
                fprintf(a, "\tstruct ActionData\n");
                fprintf(a, "\t{\n");
                if(DolphinData::data.variables.generate_fixed_length_lookahead_support)
                        fprintf(a, "\t\tint lookahead_length;\n");
                if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                {
                        fprintf(a, "\t\tconst %s *lookahead_states;\n", DolphinData::data.variables.analyzer_state_type);
                        fprintf(a, "\t\tint number_of_lookahead_states;\n");
                }
                if(DolphinData::data.variables.generate_eof_lookahead_support)
                        fprintf(a, "\t\tint lookahead_eof; // -1 - if not eof, 0 - doesn't matter, 1 - if eof\n");
                fprintf(a, "\t};\n");
        }

        fprintf(a, "\t\n");
        fprintf(a, "\t%sstatic const int symbol_to_symbol_class[alphabet_cardinality];\n",h_glob_decl.c_str());
        if(DolphinData::data.variables.compress_tables)
                fprintf(a, "\t%sstatic const %s table_of_lines[size_of_table_of_lines];\n", h_glob_decl.c_str(),DolphinData::data.variables.analyzer_state_type);
        fprintf(a, "\t%sstatic const StateData states[number_of_dfa_states+1];\n",h_glob_decl.c_str());
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                fprintf(a, "\t%sstatic const %s lookahead_states[size_of_table_of_lookahead_states];\n",h_glob_decl.c_str(), DolphinData::data.variables.analyzer_state_type);
        if(DolphinData::data.variables.generate_table_of_actions)
                fprintf(a, "\t%sstatic const ActionData actions[number_of_actions+1];\n",h_glob_decl.c_str());
        fprintf(a, "\t\n");

        string stream_class_suffix=string(type_should_be_printed_without_space(DolphinData::data.variables.input_stream_class) ? "" : " ")+
                string(type_is_a_pointer(DolphinData::data.variables.input_stream_class) ? "" : "&");

        fprintf(a, "\t%s%sinput_stream;\n", DolphinData::data.variables.input_stream_class, stream_class_suffix.c_str());
        fprintf(a, "\t%s::vector<%s> buffer;\n", DolphinData::data.variables.std_namespace,DolphinData::data.variables.input_character_class);
        if(DolphinData::data.variables.start_conditions_enabled)
                fprintf(a, "\tStartCondition start_condition;\n");
        fprintf(a, "\tbool eof_reached;\n");
        if(DolphinData::data.variables.append_data_member)
                fprintf(a, "\tbool append;\n");

        fprintf(a, "\t\n");
        if(DolphinData::data.variables.store_lexeme_in_string)
        {
                fprintf(a, "\t%s lexeme;\n", DolphinData::data.variables.internal_string_type);
        }
        else
        {
                fprintf(a, "\t%s *lexeme;\n", DolphinData::data.variables.internal_char_type);
                fprintf(a, "\tint number_of_characters_in_lexeme;\n");
        }
        fprintf(a, "\tint current_line, current_column, current_offset,current_file;\n");
        //fprintf(a, "\tint tab_size;\n");

        fprintf(a, "\t\n");
        fprintf(a, "\t%svoid clear_lexeme();\n",h_glob_decl.c_str());

        fprintf(a, "\t\n");
        if(DolphinData::data.variables.using_whale)
        {
                /*fprintf(a, "\t\Terminal *make_token(Terminal *t)\n");
                fprintf(a, "\t{\n");
                if(DolphinData::data.variables.generate_verbose_prints)
                        fprintf(a, "\t\tstd::cout << \x22make_token<\x22 << typeid(T).name() << \x22>()\\n\x22;\n");
                fprintf(a, "\t\tsymbols.append(1,(Symbol **)&t);\n");
                fprintf(a, "\t\tt->file=get_cur_file();\n");
                fprintf(a, "\t\tt->line=get_cur_line();\n");
                fprintf(a, "\t\tt->column=get_cur_column();\n");
                fprintf(a, "\t\tt->text=capture_lexeme();\n");
                fprintf(a, "\t\treturn t;\n");
                fprintf(a, "\t}\n");

                fprintf(a, "\t\Terminal *make_token(int n)\n");
                fprintf(a, "\t{\n");
                if(DolphinData::data.variables.generate_verbose_prints)
                        fprintf(a, "\t\tstd::cout << \x22make_token<\x22 << typeid(T).name() << \x22>()\\n\x22;\n");
                fprintf(a, "\t\tint c;\n");
                fprintf(a, "\t\tif(!linear_symbols.count())c=linear_symbols.append(1,NULL);\n");
                fprintf(a, "\t\tif(!linear_symbols[linear_symbols.count()-1].free_space())c=linear_symbols.append(1,NULL,32);\n");
                fprintf(a, "\t\tc=linear_symbols.count()-1;\n");
                fprintf(a, "\t\tTerminal *t=linear_symbols[c].next();\n");
                fprintf(a, "\t\tt->num=n;\n");
                fprintf(a, "\t\tt->file=get_cur_file();\n");
                fprintf(a, "\t\tt->line=get_cur_line();\n");
                fprintf(a, "\t\tt->column=get_cur_column();\n");
                fprintf(a, "\t\tt->text=capture_lexeme();\n");
                fprintf(a, "\t\treturn t;\n");
                fprintf(a, "\t}\n");*/
        }
        /*fprintf(a, "\tvoid internal_position_counter(%s c)\n", DolphinData::data.variables.internal_char_type);
        fprintf(a, "\t{\n");
        fprintf(a, "\t\tif(c=='\\n')\n");
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\tcurrent_line++;\n");
        fprintf(a, "\t\t\tcurrent_column=1;\n");
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t\telse if(c=='\\t')\n");
        fprintf(a, "\t\t\tcurrent_column+=tab_size-(current_column-1)%%tab_size;\n");
        fprintf(a, "\t\telse\n");
        fprintf(a, "\t\t\tcurrent_column++;\n");
        fprintf(a, "\t\t\n");
        fprintf(a, "\t\tcurrent_offset++;\n");
        fprintf(a, "\t}\n");
        fprintf(a, "\t\n");
        */

        fprintf(a, "public:\n");

        if(!DolphinData::data.variables.internal_char_type_is_char)
        {
                fprintf(a, "\ttemplate<class T> int basic_strlen(const T *s)\n");
                fprintf(a, "\t{\n");
                fprintf(a, "\t\tint i=0;\n");
                fprintf(a, "\t\twhile(s[i++]);\n");
                fprintf(a, "\t\treturn i;\n");
                fprintf(a, "\t}\n");

                fprintf(a, "\t\n\ttemplate<class T> T *basic_strdup(const T *s)\n");
                fprintf(a, "\t{\n");
                fprintf(a, "\t\tint l=basic_strlen<T>(s);\n");
                fprintf(a, "\t\tT *result=new wchar_t[l+1];\n");
                fprintf(a, "\t\tfor(int i=0; s[i]; i++)\n");
                fprintf(a, "\t\t\tresult[i]=s[i];\n");
                fprintf(a, "\t\tresult[l]=0;\n");
                fprintf(a, "\t\treturn result;\n");
                fprintf(a, "\t}\n");
        }

        if(DolphinData::data.variables.code_in_class)
                fprintf(a, "\t\n\t%s\n", DolphinData::data.variables.code_in_class);
//Tab<Symbol *> &symb_suppl,symbols(symb_suppl),
        fprintf(a, "\t%s(%s%sstream_supplied) : BaseLexicalAnalyzer(),input_stream(stream_supplied)\n",
                DolphinData::data.variables.dolphin_class_name, DolphinData::data.variables.input_stream_class,
                stream_class_suffix.c_str());
        fprintf(a, "\t{\n");
        if(DolphinData::data.variables.start_conditions_enabled)
                fprintf(a, "\t\tstart_condition=%s;\n", DolphinData::data.start_conditions[0].name);
        if(!DolphinData::data.variables.store_lexeme_in_string)
        {
                fprintf(a, "\t\tlexeme=NULL;\n");
                fprintf(a, "\t\tnumber_of_characters_in_lexeme=0;\n");
        }
        fprintf(a, "\t\teof_reached=false;\n");
        fprintf(a, "\t\tcurrent_line=1;\n");
        fprintf(a, "\t\tcurrent_column=1;\n");
        fprintf(a, "\t\tcurrent_offset=0;\n");
        //fprintf(a, "\t\tset_tab_size(8);\n");
        if(DolphinData::data.variables.code_in_constructor)
                fprintf(a, "\t\t\n\t\t%s\n", DolphinData::data.variables.code_in_constructor);
        fprintf(a, "\t}\n");
        if(DolphinData::data.variables.store_lexeme_in_string){
                fprintf(a, "\t~%s() {\n", DolphinData::data.variables.dolphin_class_name);
                /*fprintf(a, "\t\tfor(int i=0;i<symbols.count();++i) {\n");
                fprintf(a, "\t\t\t delete symbols[i];\n");
                fprintf(a, "\t\t}\n");
                fprintf(a, "\t\tsymbols.clear();\n");*/
                fprintf(a, "\t}\n");
        }else
                fprintf(a, "\t~%s() { if(lexeme) delete lexeme; }\n", DolphinData::data.variables.dolphin_class_name);
        if(DolphinData::data.variables.start_conditions_enabled)
        {
                fprintf(a, "\tvoid set_start_condition(StartCondition new_start_condition) { start_condition=new_start_condition; }\n");
                fprintf(a, "\tStartCondition get_start_condition() const { return start_condition; }\n");
        }
        fprintf(a, "\t%sint get_token();\n",h_glob_decl.c_str());
        fprintf(a, "\tvoid set_error(const char *){}\n");
        fprintf(a, "\tvoid set_error(int file,int ln,int col,const char *){}\n");
        fprintf(a, "\tvoid set_warning(int file,int ln,int col,const char *){}\n");
        if(DolphinData::data.variables.store_lexeme_in_string)
        {
                fprintf(a, "\tconst %s *get_lexeme() const { return lexeme.c_str(); }\n", DolphinData::data.variables.internal_char_type);
                fprintf(a, "\tconst %s &get_lexeme_str() const { return lexeme; }\n", DolphinData::data.variables.internal_string_type);
                /*if(DolphinData::data.variables.internal_char_type_is_char)
                        fprintf(a, "\t%s *capture_lexeme() { char *s=strdup(lexeme.c_str()); clear_lexeme(); return s; }\n", DolphinData::data.variables.internal_char_type);
                else
                        fprintf(a, "\t%s *capture_lexeme() { %s *s=basic_strdup<%s>(lexeme.c_str()); clear_lexeme(); return s; }\n", DolphinData::data.variables.internal_char_type, DolphinData::data.variables.internal_char_type, DolphinData::data.variables.internal_char_type);
                */
                fprintf(a, "\tint get_lexeme_length() const { return lexeme.size(); }\n");
        }
        else
        {
                fprintf(a, "\tconst %s *get_lexeme() const { return lexeme; }\n", DolphinData::data.variables.internal_char_type);
                fprintf(a, "\t%s *capture_lexeme() { %s *s=lexeme; lexeme=NULL; clear_lexeme(); return s; }\n", DolphinData::data.variables.internal_char_type, DolphinData::data.variables.internal_char_type);
                fprintf(a, "\tint get_lexeme_length() const { return number_of_characters_in_lexeme; }\n");
        }
        fprintf(a, "\tint get_cur_file() const { return current_file; }\n");
        fprintf(a, "\tint get_cur_line() const { return current_line; }\n");
        fprintf(a, "\tint get_cur_column() const { return current_column; }\n");
        fprintf(a, "\tint get_cur_offset() const { return current_offset; }\n");

        fprintf(a, "\tvoid set_cur_file(int a) {  current_file=a; }\n");
        fprintf(a, "\tvoid set_cur_line(int a) {  current_line=a; }\n");
        fprintf(a, "\tvoid set_cur_column(int a) { current_column=a; }\n");
        fprintf(a, "\tvoid set_cur_offset(int a) { current_offset=a; }\n");
        //fprintf(a, "\tvoid set_tab_size(int n) { tab_size=n; }\n");
        //fprintf(a, "\tint get_tab_size() const { return tab_size; }\n");

        if(DolphinData::data.variables.code_in_class_after_all)
                fprintf(a, "\t\n\t%s\n", DolphinData::data.variables.code_in_class_after_all);

        fprintf(a, "};\n");
}

template<class T> void generate_single_scalar_constant(FILE *a, const char *indent, const char *type, const char *id, T value, const char *format, const char *class_name, bool it_is_cpp)
{
        if(!it_is_cpp)
        {
                //string s=string("%sstatic const %s %s=")+string(format)+string(";\n");
                //fprintf(a, s.c_str(), indent, type, id, value);
                fprintf(a, "%senum {%s=%d};\n", indent, id, (int)value);
        }
        else {
//                fprintf(a, "%sconst %s %s::%s::%s;\n", indent, type, DolphinData::data.variables.whale_namespace,DolphinData::data.variables.whale_class,id);
             }
}

/*template<class T> void generate_single_scalar_constant(FILE *a, char *indent, char *type, char *id, T value, char *format, char *class_name, bool it_is_cpp)
{
        if(!it_is_cpp)
        {
                string s=string("%sstatic const %s %s=")+string(format)+string(";\n");
                fprintf(a, s.c_str(), indent, type, id, value);
        }
        else
                fprintf(a, "%sconst %s %s::%s;\n", indent, type, class_name, id);
}*/

void generate_scalar_constants(FILE *a, const char *indent, bool it_is_cpp)
{
//      if(DolphinData::data.first_terminal)
//              generate_single_scalar_constant(a, indent, "int", "first_terminal_symbol", DolphinData::data.first_terminal, "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "alphabet_cardinality", DolphinData::data.variables.alphabet_cardinality, "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "number_of_symbol_classes", DolphinData::data.number_of_symbol_classes, "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);
        if(DolphinData::data.variables.start_conditions_enabled)
                generate_single_scalar_constant(a, indent, "int", "number_of_start_conditions", DolphinData::data.start_conditions.size(), "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "number_of_dfa_states", DolphinData::data.dfa_partition.groups.size(), "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);

        /* all state numbers are incremented by one */
        generate_single_scalar_constant(a, indent, DolphinData::data.variables.analyzer_state_type, "initial_dfa_state", DolphinData::data.dfa_partition.group_containing_initial_state+1, "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);

        if(DolphinData::data.variables.compress_tables)
                generate_single_scalar_constant(a, indent, "int", "size_of_table_of_lines", DolphinData::data.tables.compressed_table_of_lines.size(), "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "number_of_actions", DolphinData::data.actions.size(), "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                generate_single_scalar_constant(a, indent, "int", "size_of_table_of_lookahead_states", DolphinData::data.tables.compressed_table_of_lookahead_states.size(), "%u", DolphinData::data.variables.dolphin_class_name, it_is_cpp);
}

void generate_get_token_function(FILE *a)
{
        fprintf(a, "%sint %s::get_token()\n", cpp_glob_decl.c_str(),DolphinData::data.variables.dolphin_class_name);
                //DolphinData::data.variables.get_token_function_parameters);
        fprintf(a, "{\n");
        if(DolphinData::data.variables.generate_verbose_prints)
        {
                fprintf(a, "\tcout << \x22" "DolphinLexicalAnalyzer::get_token()\\n\x22;\n");
                fprintf(a, "\t\n");
        }

        if(DolphinData::data.variables.store_lexeme_in_string)
                fprintf(a, "\tif(lexeme.size())\n");
        else
                fprintf(a, "\tif(lexeme)\n");
        fprintf(a, "\t\tclear_lexeme();\n");

        fprintf(a, "\t\n");
        fprintf(a, "\t%s state=initial_dfa_state;\n", DolphinData::data.variables.analyzer_state_type);
        if(DolphinData::data.variables.append_data_member)
        {
                fprintf(a, "\tint start_pos=0, accepting_pos=0, action_to_call=0;\n");
                fprintf(a, "\tappend=false;\n");
        }
        else
                fprintf(a, "\tint accepting_pos=0, action_to_call=0;\n");

        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                fprintf(a, "\tvector<pair<int, int> > possible_lookahead_positions_and_states;\n");

        fprintf(a, "\t\n");
        fprintf(a, "\tfor(int pos=%s;; pos++)\n", (DolphinData::data.variables.append_data_member ? "start_pos" : "0"));
        fprintf(a, "\t{\n");

        fprintf(a, "\t\tbool eof_reached_right_now=false;\n");
        fprintf(a, "\t\t\n");

        if(DolphinData::data.variables.start_conditions_enabled)
                fprintf(a, "\t\tint recognized_action=states[state].action_upon_accept[start_condition];\n");
        else
                fprintf(a, "\t\tint recognized_action=states[state].action_upon_accept;\n");

        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
        {
                fprintf(a, "\t\t\n");
                fprintf(a, "\t\tif(states[state].is_lookahead_state)\n");
                fprintf(a, "\t\t\tpossible_lookahead_positions_and_states.push_back(make_pair(pos, state));\n");
        }

        fprintf(a, "\t\t\n");
        fprintf(a, "\t\tif(buffer.size()==pos)\n");
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\tif(eof_reached)\n");
        fprintf(a, "\t\t\t\teof_reached_right_now=true;\n");
        fprintf(a, "\t\t\telse\n");
        fprintf(a, "\t\t\t{\n");
        fprintf(a, "\t\t\t\t%s c=0;\n", DolphinData::data.variables.input_character_class);
        /*{fprintf(a, "\t\t\t\tbool was_eof=", DolphinData::data.variables.input_character_class);
        char *ceof_parameter_values[]={"input_stream"};
        print_parametrized_string(a, DolphinData::data.variables.how_to_check_eof, "s", ceof_parameter_values);
        fprintf(a, ";\n");}*/

        fprintf(a, "\t\t\t\t");
        const char *gcfs_parameter_values[]={"c", "input_stream"};
        print_parametrized_string(a, DolphinData::data.variables.how_to_get_character_from_stream, "cs", gcfs_parameter_values);
        fprintf(a, ";\n");

        fprintf(a, "\t\t\t\t\n");

        fprintf(a, "\t\t\t\tif(");
        const char *ceof_parameter_values[]={"input_stream"};
        print_parametrized_string(a, DolphinData::data.variables.how_to_check_eof, "s", ceof_parameter_values);
        fprintf(a, ")\n");

        fprintf(a, "\t\t\t\t{\n");
        fprintf(a, "\t\t\t\t\teof_reached=true;\n");
        fprintf(a, "\t\t\t\t\teof_reached_right_now=true;\n");
        fprintf(a, "\t\t\t\t}\n");
        fprintf(a, "\t\t\t\telse\n");
        fprintf(a, "\t\t\t\t{\n");
        if(DolphinData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\t\t\tcout << \x22read symbol \x22 << (unsigned int)c << \x22\\n\x22;\n");
        fprintf(a, "\t\t\t\t\tbuffer.push_back(c);\n");
        fprintf(a, "\t\t\t\t}\n");
        fprintf(a, "\t\t\t}\n");
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t\t\n");
        fprintf(a, "\t\tif(eof_reached_right_now)\n");
        fprintf(a, "\t\t{\n");
        if(DolphinData::data.variables.append_data_member)
                fprintf(a, "\t\t\tif(pos==start_pos)\n");
        else
                fprintf(a, "\t\t\tif(pos==0)\n");
        fprintf(a, "\t\t\t{\n");

        if(!DolphinData::data.variables.store_lexeme_in_string)
        {
                const char *indent=(DolphinData::data.variables.append_data_member ? "\t\t\t\t\t" : "\t\t\t\t");
                if(DolphinData::data.variables.append_data_member)
                {
                        fprintf(a, "\t\t\t\tif(lexeme==NULL)\n");
                        fprintf(a, "\t\t\t\t{\n");
                }
                fprintf(a, "%snumber_of_characters_in_lexeme=0;\n", indent);
                fprintf(a, "%slexeme=new %s[1];\n", indent, DolphinData::data.variables.internal_char_type);
                fprintf(a, "%slexeme[0]=0;\n", indent);
                if(DolphinData::data.variables.append_data_member)
                        fprintf(a, "\t\t\t\t}\n");
                fprintf(a, "\t\t\t\t\n");
        }
        if(DolphinData::data.recognized_expression_search.count("eof"))
        {
                RecognizedExpressionData &re=DolphinData::data.recognized_expressions[DolphinData::data.recognized_expression_search["eof"]];
                generate_actions_for_special_expression(a, "\t\t\t\t", re, false);
        }
        else if(DolphinData::data.variables.using_whale)
                fprintf(a, "\t\t\t\treturn (%sTerminalEOF);\n",term_namespace_qual.c_str());
        else
                fprintf(a, "\t\t\t\treturn 0;\n");

        fprintf(a, "\t\t\t}\n");
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t\telse\n");
        fprintf(a, "\t\t{\n");

        fprintf(a, "\t\t\tunsigned int c=(unsigned char)");
        const char *gac_parameter_values[]={"buffer[pos]"};
        print_parametrized_string(a, DolphinData::data.variables.how_to_get_actual_character, "c", gac_parameter_values);
        fprintf(a, ";\n");

        if(DolphinData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << \x22transition from \x22 << int(state) << \x22 to \x22;\n");
        if(DolphinData::data.variables.access_transitions_through_a_method)
                fprintf(a, "\t\t\tstate=states[state].access_transition(symbol_to_symbol_class[c]);\n");
        else
                fprintf(a, "\t\t\tstate=states[state].transitions[symbol_to_symbol_class[c]];\n");
        if(DolphinData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << int(state) << \x22 on \x22 << c << \x22\\n\x22;\n");
        fprintf(a, "\t\t}\n");

        fprintf(a, "\t\t\n");
        if(DolphinData::data.variables.generate_eof_lookahead_support)
        {
                fprintf(a, "\t\tint lookahead_eof=actions[recognized_action].lookahead_eof;\n");
                fprintf(a, "\t\t\n");
                fprintf(a, "\t\tif(recognized_action && (lookahead_eof==0 ||\n");
                fprintf(a, "\t\t\t(lookahead_eof==-1 && !eof_reached_right_now) ||\n");
                fprintf(a, "\t\t\t(lookahead_eof==1 && eof_reached_right_now)))\n");
        }
        else
                fprintf(a, "\t\tif(recognized_action)\n");
        fprintf(a, "\t\t{\n");
        if(DolphinData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << \x22pos \x22 << pos << \x22: recognized action \x22 << recognized_action << \x22\\n\x22;\n");
        fprintf(a, "\t\t\taccepting_pos=pos;\n");
        fprintf(a, "\t\t\taction_to_call=recognized_action;\n");
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t\t\n");

        fprintf(a, "\t\tif(!state || eof_reached_right_now)\n");
        fprintf(a, "\t\t{\n");

        if(DolphinData::data.variables.eat_one_character_upon_lexical_error)
        {
                fprintf(a, "\t\t\tif(action_to_call==0)\t// if it is a lexical error,\n");
                if(DolphinData::data.variables.append_data_member)
                        fprintf(a, "\t\t\t\taccepting_pos=start_pos+1; // then eat one character.\n");
                else
                        fprintf(a, "\t\t\t\taccepting_pos=1; // then eat one character.\n");
                fprintf(a, "\t\t\t\n");
        }
        else
                fprintf(a, "\t\t\t// if there is no action to call, then accepting_pos=0\n");
        if(DolphinData::data.variables.generate_fixed_length_lookahead_support ||
                DolphinData::data.variables.generate_arbitrary_lookahead_support)
        {
                fprintf(a, "\t\t\tif(action_to_call>0)\n");
                fprintf(a, "\t\t\t{\n");
                fprintf(a, "\t\t\t\tconst ActionData &action=actions[action_to_call];\n");
                fprintf(a, "\t\t\t\t\n");
                if(DolphinData::data.variables.generate_fixed_length_lookahead_support)
                {
                        fprintf(a, "\t\t\t\tif(action.lookahead_length>0)\n");
                        fprintf(a, "\t\t\t\t\taccepting_pos-=action.lookahead_length;\n");
                }
                if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                {
                        fprintf(a, "\t\t\t\t%sif(action.lookahead_states)\n",
                                (DolphinData::data.variables.generate_fixed_length_lookahead_support ? "else " : ""));
                        fprintf(a, "\t\t\t\t{\n");
                        if(DolphinData::data.variables.generate_verbose_prints)
                        {
                                fprintf(a, "\t\t\t\t\tcout << \x22Processing lookahead: state=\x22 << int(state) << \x22, action=\x22 << action_to_call << \x22, accepting_pos=\x22 << accepting_pos << \x22, pos=\x22 << pos << \x22.\\n\x22;\n");
                                fprintf(a, "\t\t\t\t\t\n");
                        }
                        fprintf(a, "\t\t\t\t\tint result=-1;\n");
                        fprintf(a, "\t\t\t\t\t\n");
                        fprintf(a, "\t\t\t\t\tfor(int i=possible_lookahead_positions_and_states.size()-1; i>=0; i++)\n");
                        fprintf(a, "\t\t\t\t\t{\n");
                        fprintf(a, "\t\t\t\t\t\tpair<int, int> p=possible_lookahead_positions_and_states[i];\n");
                        if(DolphinData::data.variables.generate_verbose_prints)
                                fprintf(a, "\t\t\t\t\t\tcout << \x22Trying (\x22 << p.first << \x22, \x22 << p.second << \x22), char \x22 << (unsigned int)(buffer[p.first]) << \x22.\\n\x22;\n");
                        fprintf(a, "\t\t\t\t\t\tif(p.first>accepting_pos) continue;\n");
                        fprintf(a, "\t\t\t\t\t\t\n");
                        fprintf(a, "\t\t\t\t\t\tfor(int j=0; j<action.number_of_lookahead_states; j++)\n");
                        fprintf(a, "\t\t\t\t\t\t\tif(action.lookahead_states[j]==p.second)\n");
                        fprintf(a, "\t\t\t\t\t\t\t{\n");
                        fprintf(a, "\t\t\t\t\t\t\t\tresult=p.first;\n");
                        fprintf(a, "\t\t\t\t\t\t\t\tbreak;\n");
                        fprintf(a, "\t\t\t\t\t\t\t}\n");
                        fprintf(a, "\t\t\t\t\t\t\n");
                        fprintf(a, "\t\t\t\t\t\tif(result!=-1)\n");
                        fprintf(a, "\t\t\t\t\t\t\tbreak;\n");
                        fprintf(a, "\t\t\t\t\t}\n");
                        fprintf(a, "\t\t\t\t\t\n");
                        if(DolphinData::data.variables.generate_verbose_prints)
                                fprintf(a, "\t\t\t\t\tcout << \x22Using position \x22 << result << \x22.\\n\x22;\n");
                        if(DolphinData::data.variables.generate_sanity_checks)
                                fprintf(a, "\t\t\t\t\tif(result==-1) throw logic_error(\x22%s::get_token(): Internal error processing lookahead.\x22);\n", DolphinData::data.variables.dolphin_class_name);
                        fprintf(a, "\t\t\t\t\taccepting_pos=result;\n");
                        fprintf(a, "\t\t\t\t}\n");
                }
                fprintf(a, "\t\t\t}\n");
                fprintf(a, "\t\t\t\n");
        }
        if(DolphinData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << \x22" "Creating a \x22 << accepting_pos << \x22-character long lexeme.\\n\x22;\n");
        if(DolphinData::data.variables.store_lexeme_in_string){
//                fprintf(a, "\t\t\tcopy(buffer.begin()%s, buffer.begin()+accepting_pos, back_inserter(lexeme));\n",
//                        (DolphinData::data.variables.append_data_member ? "+start_pos" : ""));
                fprintf(a, "\t\t\tcopy(buffer,%s,accepting_pos,lexeme);\n",
                        (DolphinData::data.variables.append_data_member ? "+start_pos" : "0"));
        }
        else
        {
                if(DolphinData::data.variables.append_data_member)
                {
                        fprintf(a, "\t\t\tif(lexeme)\n");
                        fprintf(a, "\t\t\t\tdelete lexeme;\n");
                }

                fprintf(a, "\t\t\tnumber_of_characters_in_lexeme=accepting_pos;\n");
                fprintf(a, "\t\t\tlexeme=new %s[number_of_characters_in_lexeme+1];\n", DolphinData::data.variables.internal_char_type);
                fprintf(a, "\t\t\tcopy(buffer.begin(), buffer.begin()+number_of_characters_in_lexeme, lexeme);\n");
                fprintf(a, "\t\t\tlexeme[number_of_characters_in_lexeme]=0;\n");
        }
        fprintf(a, "\t\t\t\n");

        fprintf(a, "\t\t\tswitch(action_to_call)\n");
        fprintf(a, "\t\t\t{\n");
        for(int i=0; i<DolphinData::data.actions.size()+1; i++)
        {
                if(i==0)
                {
                        // action upon error
                        fprintf(a, "\t\t\tcase %u:\n", i);
                        if(DolphinData::data.recognized_expression_search.count("error"))
                        {
                                RecognizedExpressionData &re=DolphinData::data.recognized_expressions[DolphinData::data.recognized_expression_search["error"]];
                                generate_actions_for_special_expression(a, "\t\t\t\t", re, true);
                        }
                        else
                        {
                                if(DolphinData::data.variables.using_whale)
                                {
                                        fprintf(a, "\t\t\t\tcout << \x22Lexical error at line \x22 << line()\n");
                                        fprintf(a, "\t\t\t\t\t<< \x22 column \x22 << column() << \x22.\\n\x22;\n");
                                        fprintf(a, "\t\t\t\t\n");
                                }
                                if(DolphinData::data.variables.using_whale)
                                        fprintf(a, "\t\t\t\treturn (%sTerminalError);\n",term_namespace_qual.c_str());
                                else
                                        fprintf(a, "\t\t\t\treturn -1;\n");
                        }
                }
                else
                {
                        int an=i-1; // action number in our arrays.
                        ActionData &action=DolphinData::data.actions[an];
                        if(action.is_special) continue;

                        fprintf(a, "\t\t\tcase %u:\n", i);
                        if(DolphinData::data.variables.generate_verbose_prints)
                                fprintf(a, "\t\t\t\tcout << \x22" "Action %u\\n\x22;\n", i);

                        generate_action(a, "\t\t\t\t", action, true);
                }
        }
        if(DolphinData::data.variables.generate_sanity_checks)
        {
                fprintf(a, "\t\t\tdefault:\n");
                if(DolphinData::data.variables.generate_verbose_prints)
                        fprintf(a, "\t\t\t\tcout << \x22wrong action number \x22 << action_to_call << \x22\\n\x22;\n");
                fprintf(a, "\t\t\t\tthrow logic_error(\x22%s::get_token(): DFA has reached a non-existent state.\x22);\n", DolphinData::data.variables.dolphin_class_name);
        }
        fprintf(a, "\t\t\t}\n");

        fprintf(a, "\t\t\t\n");
        if(DolphinData::data.variables.append_data_member)
        {
                fprintf(a, "\t\t\tif(append)\n");
                if(DolphinData::data.variables.store_lexeme_in_string)
                        fprintf(a, "\t\t\t\tstart_pos=lexeme.size();\n");
                else
                        fprintf(a, "\t\t\t\tstart_pos=number_of_characters_in_lexeme;\n");
                fprintf(a, "\t\t\telse\n");
                fprintf(a, "\t\t\t{\n");
                fprintf(a, "\t\t\t\tclear_lexeme();\n");
                fprintf(a, "\t\t\t\tstart_pos=0;\n");
                fprintf(a, "\t\t\t}\n");
                fprintf(a, "\t\t\t\n");
                fprintf(a, "\t\t\tpos=start_pos-1;\n");
        }
        else
        {
                if(DolphinData::data.variables.store_lexeme_in_string)
                        fprintf(a, "\t\t\tif(lexeme.size())\n");
                else
                        fprintf(a, "\t\t\tif(lexeme)\n");
                fprintf(a, "\t\t\t\tclear_lexeme();\n");
                fprintf(a, "\t\t\tpos=-1;\n");
        }
        fprintf(a, "\t\t\tstate=initial_dfa_state;\n");
        fprintf(a, "\t\t\taccepting_pos=0;\n");
        fprintf(a, "\t\t\taction_to_call=0;\n");
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                fprintf(a, "\t\t\tpossible_lookahead_positions_and_states.clear();\n");
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t}\n");
        fprintf(a, "}\n");
}

void generate_table_of_states(FILE *a)
{
        fprintf(a, "\n%sconst %s::StateData %s::states[%s::number_of_dfa_states+1]={\n",
                cpp_glob_decl.c_str(),
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name);

        // dummy 0th state.
        fprintf(a, "\t{ ");
        if(DolphinData::data.variables.compress_tables)
        {
                if(DolphinData::data.variables.using_layer2)
                {
                        fprintf(a, "0, 0, ");
                }
                fprintf(a, "NULL");
        }
        else
        {
                fprintf(a, "{ ");
                for(int j=0; j<DolphinData::data.number_of_symbol_classes; j++)
                {
                        if(j) fprintf(a, ", ");
                        fprintf(a, "0");
                }
                fprintf(a, " }");
        }
        if(DolphinData::data.variables.start_conditions_enabled)
        {
                fprintf(a, ", { 0");
                for(int j=1; j<DolphinData::data.start_conditions.size(); j++)
                        fprintf(a, ", 0");
                fprintf(a, " }");
        }
        else
                fprintf(a, ", 0");
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                fprintf(a, ", false");
        fprintf(a, " }");

//      cout << DolphinData::data.action_vectors_in_state_groups << "\n";

        // dfa states.
        for(int i=0; i<DolphinData::data.dfa_partition.groups.size(); i++)
        {
                fprintf(a, ",\n" "\t{ ");//, i);

                if(DolphinData::data.variables.compress_tables)
                {
                        int l1_line=DolphinData::data.tables.state_to_layer1[i];
                        int offset;

                        if(!DolphinData::data.variables.using_layer2)
                                offset=DolphinData::data.tables.line_to_offset_in_table_of_lines[l1_line];
                        else
                        {
                                int exc_location=DolphinData::data.tables.layer1_to_exception_location[l1_line];
                                vector<int> &exc_data=DolphinData::data.tables.layer1_to_exception_data[l1_line];
                                if(exc_location==-1)
                                        fprintf(a, "-1, 0, ");
                                else
                                {
                                        assert(exc_data.size()==1);
                                        fprintf(a, "%d, %d, ", exc_location, exc_data[0]);
                                }

                                int l2_line=DolphinData::data.tables.layer1_to_layer2[l1_line];
                                offset=DolphinData::data.tables.line_to_offset_in_table_of_lines[l2_line];
                        }

                        fprintf(a, "table_of_lines%s", printable_increment(offset));
                }
                else
                {
                        fprintf(a, "{ ");
                        for(int j=0; j<DolphinData::data.number_of_symbol_classes; j++)
                        {
                                if(j) fprintf(a, ", ");

                                // all state numbers are incremented by one.
                                fprintf(a, "%d", DolphinData::data.final_transition_table[i][j]+1);
                        }
                        fprintf(a, " }");
                }

                // all action numbers are also incremented by one.
                fprintf(a, ", ");
                int original_group=DolphinData::data.dfa_partition.group_to_original_group[i];
                if(DolphinData::data.variables.start_conditions_enabled)
                {
                        fprintf(a, "{ ");
                        for(int j=0; j<DolphinData::data.start_conditions.size(); j++)
                        {
                                if(j) fprintf(a, ", ");
                                fprintf(a, "%u", DolphinData::data.action_vectors_in_state_groups[original_group][j]+1);
                        }
                        fprintf(a, " }");
                }
                else
                        fprintf(a, "%u", DolphinData::data.action_vectors_in_state_groups[original_group][0]+1);

                if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                        fprintf(a, ", %s", (DolphinData::data.lookahead_states_in_final_dfa[i] ? "true" : "false"));

                fprintf(a, " }");
        }
        fprintf(a, "\n};\n");
}

void generate_table_of_lines(FILE *a)
{
        fprintf(a, "\n%sconst %s %s::table_of_lines[%s::size_of_table_of_lines]={\n",
                cpp_glob_decl.c_str(),
                DolphinData::data.variables.analyzer_state_type,
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name);
        TablePrinter tp(a, "\t", "%3d", 12);
        tp.print(DolphinData::data.tables.compressed_table_of_lines);
        fprintf(a, "\n};\n");
}

void generate_table_of_symbol_classes(FILE *a)
{
        fprintf(a, "\n%sconst int %s::symbol_to_symbol_class[%s::alphabet_cardinality]={\n",
                cpp_glob_decl.c_str(),
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name);
        TablePrinter tp(a, "\t", "%3d", 12);
        tp.print(DolphinData::data.symbol_to_symbol_class);
        fprintf(a, "\n};\n");
}

void generate_table_of_lookahead_states(FILE *a)
{
        fprintf(a, "\n%sconst %s %s::lookahead_states[%s::size_of_table_of_lookahead_states]={\n",
                cpp_glob_decl.c_str(),
                DolphinData::data.variables.analyzer_state_type,
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name);
        TablePrinter tp(a, "\t", "%3d", 12);
        tp.print(DolphinData::data.tables.compressed_table_of_lookahead_states);
        fprintf(a, "\n};\n");
}

void generate_table_of_actions(FILE *a)
{
#if 0
        fprintf(a, "\n%sconst int %s::action_to_recognized_expression[%s::number_of_actions+1]={\n",
                cpp_glob_decl.c_str(),
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name);
        TablePrinter tp(a, "\t", "%3d", 12);
        tp.print(DolphinData::data.tables.action_to_recognized_expression);
        fprintf(a, "\n};\n");
#endif

        const int actions_per_line=2;

        fprintf(a, "\n%sconst %s::ActionData %s::actions[%s::number_of_actions+1]={\n\t",
                cpp_glob_decl.c_str(),
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name,
                DolphinData::data.variables.dolphin_class_name);

        fprintf(a, "{ ");
        if(DolphinData::data.variables.generate_arbitrary_lookahead_support &&
                DolphinData::data.variables.generate_fixed_length_lookahead_support)
        {
                fprintf(a, "0, NULL, 0");
        }
        else if(DolphinData::data.variables.generate_fixed_length_lookahead_support)
        {
                assert(!DolphinData::data.variables.generate_arbitrary_lookahead_support);
                fprintf(a, "0");
        }
        else if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
        {
                assert(!DolphinData::data.variables.generate_fixed_length_lookahead_support);
                fprintf(a, "NULL, 0");
        }
        if(DolphinData::data.variables.generate_eof_lookahead_support)
        {
                if(DolphinData::data.variables.generate_fixed_length_lookahead_support ||
                        DolphinData::data.variables.generate_arbitrary_lookahead_support)
                {
                        fprintf(a, ", ");
                }
                fprintf(a, "0");
        }
        fprintf(a, " }");

        for(int i=0; i<DolphinData::data.actions.size(); i++)
        {
                fprintf(a, ((i+1)%actions_per_line==0 ? ",\n\t" : ", "));

                int re_number=DolphinData::data.actions[i].recognized_expression_number;
                RecognizedExpressionData &re=DolphinData::data.recognized_expressions[re_number];

                fprintf(a, "{ ");

                bool comma_needed=false;

                if(DolphinData::data.variables.generate_fixed_length_lookahead_support)
                {
                        fprintf(a, "%d", re.lookahead_length);
                        comma_needed=true;
                }

                if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
                {
                        if(comma_needed)
                                fprintf(a, ", ");
                        else
                                comma_needed=true;

                        if(re.lookahead_length==-1)
                        {
                                int offset=DolphinData::data.tables.offset_in_table_of_lookahead_states[re_number];
                                int number=DolphinData::data.tables.number_of_lookahead_states[re_number];
                                fprintf(a, "lookahead_states%s, %d", printable_increment(offset), number);
                        }
                        else
                                fprintf(a, "NULL, 0");
                }

                if(DolphinData::data.variables.generate_eof_lookahead_support)
                {
                        if(comma_needed)
                                fprintf(a, ", ");
                        else
                                comma_needed=true;

                        fprintf(a, "%d", re.lookahead_eof);
                }

                //comma_needed;
                fprintf(a, " }");
        }

        fprintf(a, "\n};\n");
}

// this function must be run twice: before DolphinLexicalAnalyzer class with
// mode==1 and after DolphinLexicalAnalyzerClass with mode==2

void print_parametrized_string(FILE *a, const char *s, const char *parameters, const char *values[])
{
        bool after_dollar=false;
        for(int i=0; s[i]; i++)
        {
                char c=s[i];
                if(after_dollar)
                {
                        if(c=='$')
                                fputc('$', a);
                        else if(c<=' ' || c>=128)
                                fprintf(a, "$%c", c);
                        else
                        {
                                bool local_flag=false;
                                for(int j=0; j<parameters[j]; j++)
                                        if(parameters[j]==c)
                                        {
                                                fprintf(a, "%s", values[j]);
                                                local_flag=true;
                                                break;
                                        }
                                assert(local_flag);
                        }
                        after_dollar=false;
                }
                else if(c=='$' && s[i+1])
                        after_dollar=true;
                else
                        fputc(c, a);
        }
}

void generate_action(FILE *a, const char *indent, ActionData &action, bool break_after)
{
        if(typeid(*action.declaration->action)==typeid(NonterminalActionReturn))
        {
                NonterminalActionReturn &a_ret=*dynamic_cast<NonterminalActionReturn *>(action.declaration->action);

                fprintf(a, "%s", indent);
                const char *parameter_values[]={a_ret.return_value->text};
                print_parametrized_string(a, DolphinData::data.variables.how_to_return_token, "t", parameter_values);
                fprintf(a, "\n");
        }
        else if(typeid(*action.declaration->action)==typeid(NonterminalActionSkip))
        {
                if(DolphinData::data.variables.generate_verbose_prints)
                        fprintf(a, "%scout << \x22skipping\\n\x22;\n", indent);
                if(break_after)
                        fprintf(a, "%sbreak;\n", indent);
        }
        else if(typeid(*action.declaration->action)==typeid(NonterminalActionCodeII))
        {
                NonterminalActionCodeII &a_code2=*dynamic_cast<NonterminalActionCodeII *>(action.declaration->action);

                fprintf(a, "%s", indent);
                fprintf(a, "%s", a_code2.s.c_str());
                fprintf(a, "\n");

                if(break_after)
                        fprintf(a, "%sbreak;\n", indent);
        }
}

void generate_actions_for_special_expression(FILE *a, const char *indent, RecognizedExpressionData &re, bool break_after)
{
        assert(re.start_condition_to_action_number.size());

        int an0=re.start_condition_to_action_number[0];
        bool one_action=true;
        for(int i=1; i<re.start_condition_to_action_number.size(); i++)
                if(re.start_condition_to_action_number[i]!=an0)
                {
                        one_action=false;
                        break;
                }

        if(one_action)
                generate_action(a, indent, DolphinData::data.actions[an0], break_after);
        else
        {
                fprintf(a, "%sswitch(start_condition)\n", indent);
                fprintf(a, "%s{\n", indent);

                set<int> action_numbers;
                for(int i=0; i<re.start_condition_to_action_number.size(); i++)
                        action_numbers.insert(re.start_condition_to_action_number[i]);
                action_numbers.erase(-1);

                string further_indent=string(indent)+string("\t");

                for(set<int>::iterator p=action_numbers.begin(); p!=action_numbers.end(); p++)
                {
                        for(int i=0; i<re.start_condition_to_action_number.size(); i++)
                                if(re.start_condition_to_action_number[i]==*p)
                                        fprintf(a, "%scase %s:\n", indent, DolphinData::data.start_conditions[i].name);

                        generate_action(a, further_indent.c_str(), DolphinData::data.actions[*p], true);
                }

                fprintf(a, "%s}\n", indent);

                if(break_after)
                        fprintf(a, "%sbreak;\n", indent);
        }
}

bool type_is_a_pointer(const char *s)
{
        for(int i=strlen(s)-1; i>=0; i++)
                if(s[i]=='*')
                        return true;
                else if(s[i]>' ')
                        return false;
        return false;
}

bool type_should_be_printed_without_space(const char *s)
{
        int l=strlen(s);
        if(l<3) return false;
        return s[l-1]=='*' && s[l-2]==' ';
}

void insert_cpp_code_in_file(FILE *a, const char *indent, Terminal *code)
{
}


void generate_terminals(FILE *a)
{
        fprintf(a, "\n");

        if(DolphinData::data.variables.terminals_namespace){
                fprintf(a, "namespace %s\n", DolphinData::data.variables.terminals_namespace);
                fprintf(a, "{\n");
        }
        fprintf(a, "enum {\n");
        // generating all terminal classes referenced from the file.
        /*set<char *, NullTerminatedStringCompare> terminal_classes;
        for(int i=0; i<DolphinData::data.actions.size(); i++)
        {
                if(typeid(*DolphinData::data.actions[i].declaration->action)==typeid(NonterminalActionReturn))
                        terminal_classes.insert(dynamic_cast<NonterminalActionReturn *>(DolphinData::data.actions[i].declaration->action)->return_value->text);
        }
        terminal_classes.insert("TerminalEOF");
        terminal_classes.insert("TerminalError");
        int counter=0;
        for(set<char *, NullTerminatedStringCompare>::iterator p=terminal_classes.begin(); p!=terminal_classes.end(); p++){
                fprintf(a, "\t%s=%u,\n", *p, counter++);
        }*/
        vector<const char *> terminal_classes;

        terminal_classes.push_back("TerminalEOF");
        terminal_classes.push_back("TerminalError");

        for(int i=0; i<DolphinData::data.actions.size(); i++)
        {
                auto &a = *DolphinData::data.actions[i].declaration->action;
                if(typeid(a)==typeid(NonterminalActionReturn))
                        terminal_classes.push_back(dynamic_cast<NonterminalActionReturn *>(DolphinData::data.actions[i].declaration->action)->return_value->text);
        }
        int counter=0;
        for(vector<const char *>::iterator p=terminal_classes.begin(); p!=terminal_classes.end(); p++){
                if(!DolphinData::data.variables.terminals_namespace && !DolphinData::data.variables.tokens_prefix && counter<2) counter++;
                else {
                        if(DolphinData::data.variables.tokens_prefix) fprintf(a, "\t%s%s=%u,\n",DolphinData::data.variables.tokens_prefix, *p, counter++);
                        else fprintf(a, "\t%s=%u,\n", *p, counter++);
                }
        }
        if(DolphinData::data.variables.code_in_token_enum)
                fprintf(a, "\n%s\n", DolphinData::data.variables.code_in_token_enum);
        fprintf(a, "};\n // enumeration");
        if(DolphinData::data.variables.terminals_namespace)fprintf(a, "\n} // namespace %s\n", DolphinData::data.variables.whale_namespace);
}

