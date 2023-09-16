
#include <fstream>
#include <list>
#include "whale.h"
#include "codegen.h"
#include "utilities.h"
#include "first.h"
#include "grammar.h"
#include "tables.h" // should move table making stuff to some other module
using namespace std;
using namespace Whale;

//#define ANNOUNCE_REDUCE_CODE_GENERATION

/* It seems to me that this table is completely unused. */
//#define MAKE_TABLE_OF_RULE_BODIES

void generate_tokens(FILE *a);
void generate_whale_class(FILE *a);
void generate_scalar_constants(FILE *a, const char *indent, bool it_is_cpp);
void generate_table_of_states(FILE *a);
void generate_table_of_pointers_to_members(FILE *a);
void generate_table_of_rules(FILE *a);
void generate_index_entry_to_access_compressed_table(FILE *a, IndexEntry &ie, int mode);
void generate_raw_tables(FILE *a, const char *indent, bool it_is_cpp);
void generate_classes(FILE *a, const char *indent, int mode,int mode2);
void generate_parse_function(FILE *a);
void generate_reduce_code(FILE *a, const char *indent, int rn);
ClassHierarchy::DataMember *generate_code_that_copies_single_symbol_from_stack_to_one_or_more_data_members(FILE *a, const char *indent, ClassHierarchy::Class *type, NonterminalExpression *expr, const char *access_to_nonterminal_object, const char *location_in_stack_base, int location_in_stack_offset);
void generate_code_that_puts_symbol_to_data_member(FILE *a, const char *indent, const char *access_to_nonterminal_object, ClassHierarchy::DataMember *data_member, int number_of_iterations_in_source, const string &access_to_symbol);
void generate_code_that_fills_up_field(FILE *a, const char *indent, const string &access_to_symbol, const string &pointer_to_nonterminal_object, bool null_pointer_possible);
void generate_code_that_disposes_of_symbol(FILE *a, const char *indent, const string &access_to_symbol);
string expression_that_gets_single_symbol_from_stack(ClassHierarchy::Class *symbol_type, const char *location_in_stack_base, int location_in_stack_offset);
void generate_expression_that_gets_single_symbol_from_stack(FILE *a, ClassHierarchy::Class *symbol_type, const char *location_in_stack_base, int location_in_stack_offset);
void generate_code_that_initializes_all_remaining_data_members(FILE *a, const char *indent, ClassHierarchy::Class *type, const char *access_to_object, set<ClassHierarchy::DataMember *> &initialized_data_members);
int find_creator_somewhere_around_rule(int rn);
ClassHierarchy::Class *generate_code_to_establish_connection_to_creator(FILE *a, const char *indent, int creator_nn);
void generate_code_to_establish_connection_to_unknown_creator(FILE *a, const char *indent, int nn, int an);

void generate_code()
{
        const char *parser_type;
        if(WhaleData::data.variables.method==Variables::LR1)
                parser_type="An LR(1) parser";
        else if(WhaleData::data.variables.method==Variables::SLR1)
                parser_type="An SLR(1) parser";
        else if(WhaleData::data.variables.method==Variables::LALR1)
                parser_type="An LALR(1) parser";
        else
                assert(false);
        string h_file_name=WhaleData::data.file_name+string(".h");
        string h_file_name_short = h_file_name;
        std::size_t s_found = h_file_name_short.rfind('/');
        if (s_found != std::string::npos)
          h_file_name_short.erase(0, s_found+1);
        s_found = h_file_name_short.rfind('\\');
        if (s_found != std::string::npos)
          h_file_name_short.erase(0, s_found+1);

        string cpp_file_name=WhaleData::data.file_name+string(".cpp");
        string th_file_name;
        FILE *th_file=NULL;
        if(WhaleData::data.variables.terminal_file){
                th_file_name=string(WhaleData::data.variables.terminal_file)+string(".h");
                th_file=fopen(th_file_name.c_str(), "wt");
                //printf("\nusing terminal file %s %p\n",WhaleData::data.variables.terminal_file,th_file);
        }
        FILE *h_file=fopen(h_file_name.c_str(), "w");
        if(!h_file) h_file=th_file;
        else if(!th_file) th_file=h_file;

        bool term_h_equal_to_h=false;
        if(h_file==th_file) term_h_equal_to_h=true;

        FILE *cpp_file=fopen(cpp_file_name.c_str(), "w");

        string inclusion_indicator="__PARSER_GENERATED_BY_WHALE__"+convert_file_name_to_identifier(h_file_name_short, 1);
        string tinclusion_indicator;
        if(WhaleData::data.variables.terminal_file){
                tinclusion_indicator="__PARSER_GENERATED_BY_WHALE__"+convert_file_name_to_identifier(th_file_name, 1);
        }

        fprintf(h_file, FIRST_LINE_FOR_GENERATED_FILES, parser_type);
        fprintf(cpp_file, FIRST_LINE_FOR_GENERATED_FILES, parser_type);


        /* .h */
        if(!term_h_equal_to_h){
                fprintf(th_file, "\n#ifndef %s\n", tinclusion_indicator.c_str());
                fprintf(th_file, "\n#define %s\n", tinclusion_indicator.c_str());
        }

        fprintf(h_file, "\n#ifndef %s\n", inclusion_indicator.c_str());
        fprintf(h_file, "\n#define %s\n", inclusion_indicator.c_str());


        //fprintf(th_file, "\n#include <iostream>\n");
        //fprintf(th_file, "#include <vector>\n");
        fprintf(h_file, "#include <str.h>\n");
        if(WhaleData::data.variables.code_in_h_before_all)
                fprintf(th_file, "\n%s\n", WhaleData::data.variables.code_in_h_before_all);
        if(WhaleData::data.variables.input_queue)
                fprintf(th_file, "#include <queue>\n");
        if(WhaleData::data.variables.generate_verbose_prints)
                fprintf(th_file, "#include <typeinfo>\n");
        if(WhaleData::data.variables.generate_sanity_checks)
                fprintf(th_file, "#include <stdexcept>\n");
        if(WhaleData::data.variables.generate_tokens){
                if(WhaleData::data.variables.tokens_file){
                        string ht_folder=WhaleData::data.file_name.c_str();
                        std::size_t s_found = ht_folder.rfind('/'), bs_found = ht_folder.rfind('\\');
                        if (s_found == std::string::npos)
                          s_found = bs_found;
                        if (bs_found != std::string::npos && bs_found > s_found)
                          s_found = bs_found;
                        if (s_found != std::string::npos)
                          ht_folder.erase(s_found+1);
                        else
                          ht_folder = "";
                        string ht_file_name = WhaleData::data.variables.tokens_file+string(".h");
                        FILE *ht_file=fopen((ht_folder+ht_file_name).c_str(),"wt");
                        if(!ht_file){
                                cout<<"Could not write token file" << endl;
                        }else{
                                string inclusion_indicator="__TERMINALS_FOR_LEXICAL_ANALYZER_GENERATED_BY_WHALE__"+convert_file_name_to_identifier(ht_file_name, 1);
                                fprintf(ht_file, FIRST_LINE_FOR_GENERATED_FILES, parser_type);
                                fprintf(ht_file, "\n#ifndef %s\n", inclusion_indicator.c_str());
                                fprintf(ht_file, "\n#define %s\n", inclusion_indicator.c_str());
                                generate_tokens(ht_file);
                                fprintf(ht_file, "\n#endif\n");
                                fclose(ht_file);
                        }
                        fprintf(th_file, "#include \"%s\"\n",ht_file_name.c_str());
                }else{
                    generate_tokens(th_file);
                }
        }

        fprintf(th_file, "\nnamespace %s\n"
                "{\n", WhaleData::data.variables.whale_namespace);

        if(!term_h_equal_to_h){
          generate_classes(th_file, "\t", 2,1);
        }else{
          generate_classes(h_file, "\t", 2,1);
          generate_classes(h_file, "\t", 1,2);
          generate_classes(h_file, "\t", 2,2);
        }

        fprintf(th_file, "\n} // namespace %s\n", WhaleData::data.variables.whale_namespace);

        // change this! Escape sequences should be decoded.
        //fprintf(th_file, "\n#include %s\n", WhaleData::data.variables.lexical_analyzer_file);

        if(WhaleData::data.variables.code_in_h)
                fprintf(th_file, "\n%s\n", WhaleData::data.variables.code_in_h);
        if(!term_h_equal_to_h){
                fprintf(h_file, "\n#include \"%s\"\n", th_file_name.c_str());
                fprintf(th_file, "\n#endif\n");
                fclose(th_file);th_file=NULL;
        }

        fprintf(h_file, "\nnamespace %s\n"
                "{\n", WhaleData::data.variables.whale_namespace);

        if(!term_h_equal_to_h){
          generate_classes(h_file, "\t", 1,2);
          generate_classes(h_file, "\t", 2,2);
        }
        //generate_classes(h_file, "\t", 2);

        generate_whale_class(h_file);

        /*fprintf(h_file, "\t\n");
        fprintf(h_file, "\ttemplate<class T> std::vector<T> deepen(const T &x)\n");
        fprintf(h_file, "\t{\n");
        fprintf(h_file, "\t\treturn std::vector<T>(1, x);\n");
        fprintf(h_file, "\t}\n");*/

        fprintf(h_file, "\t\n");
        //fprintf(h_file, "\tstd::ostream &print_terminal_location(std::ostream &os, const Terminal *t);\n");

        fprintf(h_file, "\t\n"
                "} // namespace %s\n", WhaleData::data.variables.whale_namespace);

        //fprintf(h_file, "\n"
        //        "typedef %s::Parser %s;\n", WhaleData::data.variables.whale_namespace, WhaleData::data.variables.whale_class);

        if(WhaleData::data.variables.code_in_h_after_all)
                fprintf(h_file, "\n%s\n", WhaleData::data.variables.code_in_h_after_all);

        fprintf(h_file, "\n#endif\n");
        fclose(h_file);


        /* .cpp */

        if(WhaleData::data.variables.code_in_cpp_before_all)
                fprintf(cpp_file, "\n%s\n", WhaleData::data.variables.code_in_cpp_before_all);

        fprintf(cpp_file, "\n#include \x22%s\x22\n", h_file_name_short.c_str());
        fprintf(cpp_file, "\n#include %s\n", WhaleData::data.variables.lexical_analyzer_file);

        //fprintf(cpp_file, "\nconst char *%s::Parser::whale_copyright_notice=\n" COPYRIGHT_NOTICE_FOR_GENERATED_FILES ";\n",
        //      WhaleData::data.variables.whale_namespace, parser_type);

        if(WhaleData::data.variables.code_in_cpp)
                fprintf(cpp_file, "\n%s\n", WhaleData::data.variables.code_in_cpp);

        fprintf(cpp_file, "using namespace %s;\n", WhaleData::data.variables.std_namespace);
        fprintf(cpp_file, "using namespace %s;\n", WhaleData::data.variables.whale_namespace);

//      fprintf(cpp_file, "\n");
//      generate_scalar_constants(cpp_file, "", true);

        fprintf(cpp_file, "\n");
        generate_parse_function(cpp_file);

        fprintf(cpp_file, "\n");

        /*fprintf(cpp_file, "Symbol *%s::_newalloc(Symbol *s){\n",WhaleData::data.variables.whale_class);
        fprintf(cpp_file, "\tsymbols.append(1,&s);\n");
        fprintf(cpp_file, "\treturn s;\n");
        fprintf(cpp_file, "}\n");*/

        fprintf(cpp_file, "void %s::%s::initialize()\n", WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class);
        fprintf(cpp_file, "{\n");
        fprintf(cpp_file, "\tstate_stack.push_back(0);\n");
        if(!WhaleData::data.variables.input_queue)
                fprintf(cpp_file, "\tinput_symbol=(Terminal*)lexical_analyzer.get_terminal();\n");
        else
                fprintf(cpp_file, "\tinput_symbols.push((Terminal*)lexical_analyzer.get_terminal());\n");
        fprintf(cpp_file, "}\n");

        fprintf(cpp_file, "\n");
        //fprintf(cpp_file, "void debug(char *,...);");
        fprintf(cpp_file, "void %s::%s::report_error(const Terminal *t) const\n", WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class);
        fprintf(cpp_file, "{\n");
        fprintf(cpp_file, "\tString er(64,\"syntax: unexpected <%%s>\",t->text);\n");
        fprintf(cpp_file, "\tlexical_analyzer.set_error(t->file_start,t->line_start,t->col_start,er);\n");
        //fprintf(cpp_file, "\tos << \x22Syntax error at \x22;\n");
        //fprintf(cpp_file, "\tt->print_location(os);\n");
        //fprintf(cpp_file, "\tos << \x22.\\n\x22;\n");
        //fprintf(cpp_file, "\tdebug(\"Syntax error %%s at line %%d col %%d\",t->text,t->line,t->column);");
        fprintf(cpp_file, "}\n");

        const char *ref_to_input_symbol=(WhaleData::data.variables.input_queue ? "input_symbols.front()" : "input_symbol");

        fprintf(cpp_file, "\n");
        fprintf(cpp_file, "bool %s::%s::recover_from_error()\n", WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class);
        fprintf(cpp_file, "{\n");
        if(WhaleData::data.variables.generate_verbose_prints)
        {
                fprintf(cpp_file, "\tcout << \x22" "Error recovery\\n\x22;\n");
                fprintf(cpp_file, "\t\n");
        }
        fprintf(cpp_file, "\tint stack_pos;\n");
        fprintf(cpp_file, "\tint new_state=-1;\n");
        fprintf(cpp_file, "\tint number_of_symbols_discarded=0;\n");
        fprintf(cpp_file, "\tfor(stack_pos=state_stack.size()-1; stack_pos>=0; stack_pos--)\n");
        fprintf(cpp_file, "\t{\n");
        fprintf(cpp_file, "\t\tint state=state_stack[stack_pos];\n");
        fprintf(cpp_file, "\t\tLRAction lr_action=access_action_table(state, error_terminal_number);\n");
        fprintf(cpp_file, "\t\tif(lr_action.is_shift())\n");
        fprintf(cpp_file, "\t\t{\n");
        fprintf(cpp_file, "\t\t\tnew_state=lr_action.shift_state();\n");
        fprintf(cpp_file, "\t\t\tbreak;\n");
        fprintf(cpp_file, "\t\t}\n");
        if(WhaleData::data.variables.generate_verbose_prints)
                fprintf(cpp_file, "\t\tcout << \x22" "Discard stack\\n\x22;\n");
        fprintf(cpp_file, "\t\tnumber_of_symbols_discarded++;\n");
        fprintf(cpp_file, "\t}\n");
        fprintf(cpp_file, "\tif(new_state==-1) return false;\n");
        fprintf(cpp_file, "\t\n");
        fprintf(cpp_file, "\tTerminalError *error_token=(TerminalError *)(Symbol*)(new TerminalError);\n");
        fprintf(cpp_file, "\terror_token->file_start=%s->file_start;\n", ref_to_input_symbol);
        fprintf(cpp_file, "\terror_token->line_start=%s->line_start;\n", ref_to_input_symbol);
        fprintf(cpp_file, "\terror_token->col_start=%s->col_start;\n", ref_to_input_symbol);
        fprintf(cpp_file, "\t\n");
        if(WhaleData::data.variables.error_garbage){
          fprintf(cpp_file, "\tfor(unsigned int stack_pos_ii=stack_pos; stack_pos_ii<symbol_stack.size(); stack_pos_ii++)\n");
          fprintf(cpp_file, "\t\terror_token->garbage.push_back(symbol_stack[stack_pos_ii]);\n");
          fprintf(cpp_file, "\terror_token->error_position=error_token->garbage.size();\n");
          fprintf(cpp_file, "\t\n");
        }
        fprintf(cpp_file, "\t{for(int j=0; j<number_of_symbols_discarded; j++)\n");
        fprintf(cpp_file, "\t{\n");
        fprintf(cpp_file, "\t\tstate_stack.pop_back();\n");
        fprintf(cpp_file, "\t\tsymbol_stack.pop_back();\n");
        fprintf(cpp_file, "\t}}\n");
        fprintf(cpp_file, "\t\n");
        fprintf(cpp_file, "\t// shifting error token.\n");
        fprintf(cpp_file, "\tsymbol_stack.push_back(error_token);\n");
        fprintf(cpp_file, "\tstate_stack.push_back(new_state);\n");
        fprintf(cpp_file, "\t\n");
        fprintf(cpp_file, "\tfor(;;)\n");
        fprintf(cpp_file, "\t{\n");
        fprintf(cpp_file, "\t\tint state=state_stack.back();\n");
        fprintf(cpp_file, "\t\tLRAction lr_action(access_action_table(state, %s->number()));\n", ref_to_input_symbol);
        if(WhaleData::data.variables.generate_verbose_prints)
        {
                fprintf(cpp_file, "\t\tif(!lr_action.is_error())\n");
                fprintf(cpp_file, "\t\t{\n");
                fprintf(cpp_file, "\t\t\tcout << \x22Recovery complete\\n\x22;\n");
                fprintf(cpp_file, "\t\t\treturn true;\n");
                fprintf(cpp_file, "\t\t}\n");
        }
        else
                fprintf(cpp_file, "\t\tif(!lr_action.is_error()) return true;\n");
        fprintf(cpp_file, "\t\t\n");
        if(WhaleData::data.variables.generate_verbose_prints)
                fprintf(cpp_file, "\t\tcout << \x22" "Discard input\\n\x22;\n");
        if(WhaleData::data.variables.error_garbage){
          fprintf(cpp_file, "\t\terror_token->garbage.push_back(%s);\n", ref_to_input_symbol);
        }
        fprintf(cpp_file, "\t\tif(%s->number()==eof_terminal_number) return false;\n", ref_to_input_symbol);
        if(!WhaleData::data.variables.input_queue)
                fprintf(cpp_file, "\t\tinput_symbol=(Terminal*)lexical_analyzer.get_terminal();\n");
        else
        {
                fprintf(cpp_file, "\t\tinput_symbols.pop();\n");
                fprintf(cpp_file, "\t\tif(input_symbols.empty()) input_symbols.push((Terminal*)lexical_analyzer.get_terminal());\n");
        }
        fprintf(cpp_file, "\t}\n");

        fprintf(cpp_file, "\t\n");
#if 0
        fprintf(cpp_file, "\tTerminal *t=dynamic_cast<Terminal *>(error_token->garbage[error_token->error_position]);\n");
        if(WhaleData::data.variables.generate_sanity_checks)
                fprintf(cpp_file, "\tif(t==NULL) throw \x22Whale: internal malfunction in error recovery\x22;\n");
        fprintf(cpp_file, "\terror_token->file_start=t->file_start;\n");
        fprintf(cpp_file, "\terror_token->line_start=t->line_start;\n");
        fprintf(cpp_file, "\terror_token->col_start=t->col_start;\n");
        fprintf(cpp_file, "\t\n");
        fprintf(cpp_file, "\treturn true;\n");
#endif
        fprintf(cpp_file, "}\n");

        if(classes.creator!=NULL)
        {
                fprintf(cpp_file, "\n");
                fprintf(cpp_file, "int %s::%s::find_some_creator_in_stack(int pos) const\n", WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class);
                fprintf(cpp_file, "{\n");
                if(WhaleData::data.variables.generate_verbose_prints)
                {
                        fprintf(cpp_file, "\tcout << \x22" "find_some_creator_in_stack(): starting from \x22 << pos << \x22, \x22;\n");
                        fprintf(cpp_file, "\tcout.flush();\n");
                }
                fprintf(cpp_file, "\t{for(int i=pos; i>=0; i--)\n");
                fprintf(cpp_file, "\t\tif(dynamic_cast<Creator *>(symbol_stack[i])!=NULL)\n");
                if(WhaleData::data.variables.generate_verbose_prints)
                {
                        fprintf(cpp_file, "\t\t{\n");
                        fprintf(cpp_file, "\t\t\tcout << \x22" "found at \x22 << i << \x22\\n\x22;\n");
                        fprintf(cpp_file, "\t\t\treturn i;\n");
                        fprintf(cpp_file, "\t\t}\n");
                }
                else
                        fprintf(cpp_file, "\t\t\treturn i;\n");
                fprintf(cpp_file, "\t\n");
                if(WhaleData::data.variables.generate_verbose_prints)
                        fprintf(cpp_file, "\tcout << \x22" "found nothing.\\n\x22;\n");
                fprintf(cpp_file, "\treturn -1;\n");
                fprintf(cpp_file, "}}\n");
        }

        fprintf(cpp_file, "\n");
        /*fprintf(cpp_file, "void %s::%s::print_stack(ostream &os) const\n", WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class);
        fprintf(cpp_file, "{\n");
        fprintf(cpp_file, "\t{for(int i=0; i<state_stack.size(); i++)\n");
        fprintf(cpp_file, "\t{\n");
        fprintf(cpp_file, "\t\tos << \x22[\x22 << state_stack[i] << \x22]\x22;\n");
        fprintf(cpp_file, "\t\tif(i==symbol_stack.size()) break;\n");
        fprintf(cpp_file, "//\t\tos << \x22 \x22 << typeid(*symbol_stack[i]).name() << \x22 \x22;\n");
        fprintf(cpp_file, "\t}}\n");
        fprintf(cpp_file, "\tos << \x22\\n\x22;\n");
        fprintf(cpp_file, "}\n");*/

        fprintf(cpp_file, "\n");
        /*fprintf(cpp_file, "ostream &%s::print_terminal_location(ostream &os, const Terminal *t)\n", WhaleData::data.variables.whale_namespace);
        fprintf(cpp_file, "{\n");
        fprintf(cpp_file, "\treturn os << \x22line \x22 << t->line << \x22 column \x22 << t->column;\n");
        fprintf(cpp_file, "}\n");*/

        fprintf(cpp_file, "\n");
        generate_table_of_states(cpp_file);

        fprintf(cpp_file, "\n");
        generate_table_of_rules(cpp_file);

        if(WhaleData::data.variables.generate_table_of_pointers_to_members)
        {
                fprintf(cpp_file, "\n");
                generate_table_of_pointers_to_members(cpp_file);
        }

        fprintf(cpp_file, "\n");
        generate_raw_tables(cpp_file, "", true);

        if(WhaleData::data.variables.code_in_cpp_after_all)
                fprintf(cpp_file, "\n%s\n", WhaleData::data.variables.code_in_cpp_after_all);

        fclose(cpp_file);
}

void generate_whale_class(FILE *a)
{
        fprintf(a, "\t\n"
                "\tclass %s\n",WhaleData::data.variables.whale_class);
        fprintf(a, "\t{\n");
        //fprintf(a, "\t\tstatic const char *whale_copyright_notice;\n");

        fprintf(a, "\t\t\n"
                "\tpublic:\n");

        generate_scalar_constants(a, "\t\t", false);

        fprintf(a, "\t\t\n");
        fprintf(a, "\tprotected:\n");
        fprintf(a, "\t\tclass LRAction\n");
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\tint n;\n");
        fprintf(a, "\t\t\t\n");
        fprintf(a, "\t\tpublic:\n");
        fprintf(a, "\t\t\tLRAction(int supplied_n=0) { n=supplied_n; }\n");
        fprintf(a, "\t\t\t\n");
        fprintf(a, "\t\t\tbool is_error() const { return !n; }\n");
        fprintf(a, "\t\t\tbool is_accept() const { return n==1; }\n");
        fprintf(a, "\t\t\tbool is_shift() const { return n<0; }\n");
        fprintf(a, "\t\t\tbool is_reduce() const { return n>=2; }\n");
        fprintf(a, "\t\t\tint shift_state() const { return -n-1; }\n");
        fprintf(a, "\t\t\tint reduce_rule() const { return n-1; }\n");
        fprintf(a, "\t\t\tint get_n() const { return n; }\n");
        fprintf(a, "\t\t\tstatic LRAction error() { return LRAction(0); }\n");
        fprintf(a, "\t\t\tstatic LRAction accept() { return LRAction(1); }\n");
        fprintf(a, "\t\t\tstatic LRAction shift(int state) { return LRAction(-state-1); }\n");
        fprintf(a, "\t\t\tstatic LRAction reduce(int rule) { return LRAction(rule+1); }\n");
        fprintf(a, "\t\t};\n");

        fprintf(a, "\t\tstruct StateData\n");
        fprintf(a, "\t\t{\n");

        if(!WhaleData::data.variables.compress_action_table)
                fprintf(a, "\t\t\tint action_table[number_of_terminals];\n");
        else if(WhaleData::data.variables.action_table_compression_mode<=2)
                fprintf(a, "\t\t\tint index_in_action_table;\n");
        else if(WhaleData::data.variables.action_table_compression_mode==3)
        {
                fprintf(a, "\t\t\tbool sole_action;\n");
                fprintf(a, "\t\t\tunion { int index_in_action_table, action_value; };\n");
        }
        else
                assert(false);

        if(WhaleData::data.variables.using_error_map)
                fprintf(a, "\t\t\tint index_in_action_error_map;\n");

        if(!WhaleData::data.variables.compress_goto_table)
                fprintf(a, "\t\t\tint goto_table[number_of_nonterminals];\n");
        else if(WhaleData::data.variables.goto_table_compression_mode<=2)
                fprintf(a, "\t\t\tint index_in_goto_table;\n");
        else if(WhaleData::data.variables.goto_table_compression_mode==3)
        {
                fprintf(a, "\t\t\tbool sole_goto;\n");
                fprintf(a, "\t\t\tunion { int index_in_goto_table, goto_value; };\n");
        }
        else
                assert(false);

        fprintf(a, "\t\t};\n");

        fprintf(a, "\t\tstruct RuleData\n");
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\tint nn;\n");
#ifdef MAKE_TABLE_OF_RULE_BODIES
        fprintf(a, "\t\t\tconst int *body;\n");
#endif
        fprintf(a, "\t\t\tint length;\n");
        fprintf(a, "\t\t};\n");
        fprintf(a, "\t\t\n");

        fprintf(a, "\t\tstatic const StateData states[number_of_lr_states];\n");
        fprintf(a, "\t\tstatic const RuleData rules[number_of_rules];\n");
        generate_raw_tables(a, "\t\t", false);

        fprintf(a, "\t\t\n");
        fprintf(a, "\tpublic:\n");
        //fprintf(a, "\t\tTab<struct Symbol *> symbols;\n");
        //fprintf(a, "\t\t~Parser()\n");
        //fprintf(a, "\t\t{\n");
        /*fprintf(a, "\t\t~%s() {\n", WhaleData::data.variables.whale_class);
        fprintf(a, "\t\t\tfor(int i=0;i<symbols.count();++i) delete symbols[i];\n");
        fprintf(a, "\t\t\tsymbols.clear();\n");
        fprintf(a, "\t\t}\n");*/
        //fprintf(a, "\t\t}\n");
        fprintf(a, "\t\t%s(class %s &lexical_analyzer_supplied);\n", WhaleData::data.variables.whale_class,WhaleData::data.variables.lexical_analyzer_class);
        /*
        fprintf(a, "\t\t%s(%s &lexical_analyzer_supplied) : lexical_analyzer(lexical_analyzer_supplied),symbols(tmpmem)\n", WhaleData::data.variables.whale_class,WhaleData::data.variables.lexical_analyzer_class);
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\tinitialize();\n");
        if(WhaleData::data.variables.code_in_constructor)
                fprintf(a, "\t\t\t\n\t\t\t%s\n", WhaleData::data.variables.code_in_constructor);
        fprintf(a, "\t\t}\n");
        */
        fprintf(a, "\t\tvoid initialize();\n");
        const char *S_name=WhaleData::data.nonterminals[WhaleData::data.start_nonterminal_number].type->name.c_str();
        fprintf(a, "\t\t%s *parse();\n", S_name);
        fprintf(a, "\t\tvoid report_error(const Terminal *t) const;\n");
        fprintf(a, "\t\tbool recover_from_error();\n");
        if(classes.creator!=NULL)
                fprintf(a, "\t\tint find_some_creator_in_stack(int pos) const;\n");
        //fprintf(a, "\t\tvoid print_stack(std::ostream &os) const;\n");

        fprintf(a, "\t\t\n");
        fprintf(a, "\tprotected:\n");
        fprintf(a, "\t\tclass %s &lexical_analyzer;\n", WhaleData::data.variables.lexical_analyzer_class);
        if(!WhaleData::data.variables.input_queue)
                fprintf(a, "\t\tTerminal *input_symbol;\n");
        else
                fprintf(a, "\t\t%s::queue<Terminal *> input_symbols;\n",WhaleData::data.variables.std_namespace);
        fprintf(a, "\t\t%s::vector<int> state_stack;\n",WhaleData::data.variables.std_namespace);
        fprintf(a, "\t\t%s::vector<Symbol *> symbol_stack;\n",WhaleData::data.variables.std_namespace);
        if(WhaleData::data.variables.garbage){
          fprintf(a, "\t\t%s::vector<Symbol *> garbage;\n",WhaleData::data.variables.std_namespace);
        }
        fprintf(a, "\t\t\n");
        fprintf(a, "\t\tLRAction access_action_table(int state, int tn) const\n");
        fprintf(a, "\t\t{\n");
        if(WhaleData::data.variables.using_error_map)
        {
                fprintf(a, "\t\t\tif(!access_error_map(compressed_action_error_map + states[state].index_in_action_error_map, tn))\n");
                fprintf(a, "\t\t\t\treturn LRAction::error();\n");
        }
        if(!WhaleData::data.variables.compress_action_table)
                fprintf(a, "\t\t\treturn LRAction(states[state].action_table[tn]);\n");
        else
        {
                if(WhaleData::data.variables.action_table_compression_mode==3)
                {
                        fprintf(a, "\t\t\tif(states[state].sole_action)\n");
                        fprintf(a, "\t\t\t\treturn states[state].action_value;\n");
                }

                fprintf(a, "\t\t\treturn LRAction(compressed_action_table[states[state].index_in_action_table+tn]);\n");
        }
        fprintf(a, "\t\t}\n");

        if(WhaleData::data.variables.using_error_map)
        {
                fprintf(a, "\t\tbool access_error_map(const unsigned int *map, int n) const\n");
                fprintf(a, "\t\t{\n");
                fprintf(a, "\t\t\treturn map[n/assumed_number_of_bits_in_int] & (1 << (n%%assumed_number_of_bits_in_int));\n");
                fprintf(a, "\t\t}\n");
        }

        fprintf(a, "\t\tint access_goto_table(int state, int nn) const\n");
        fprintf(a, "\t\t{\n");
        if(!WhaleData::data.variables.compress_action_table)
                fprintf(a, "\t\t\treturn states[state].goto_table[nn];\n");
        else
        {
                if(WhaleData::data.variables.goto_table_compression_mode==3)
                {
                        fprintf(a, "\t\t\tif(states[state].sole_goto)\n");
                        fprintf(a, "\t\t\t\treturn states[state].goto_value;\n");
                }

                fprintf(a, "\t\t\treturn compressed_goto_table[states[state].index_in_goto_table + nn];\n");
        }
        fprintf(a, "\t\t}\n");

        //fprintf(a, "\t\tSymbol *_newalloc(Symbol *s);\n");

        fprintf(a, "\t\tint find_nonterminal_in_stack(int nn) const\n");
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\t{for(int i=symbol_stack.size()-1; i>=0; i--)\n");
        fprintf(a, "\t\t\t\tif(symbol_stack[i]->is_nonterminal())\n");
        fprintf(a, "\t\t\t\t{\n");
        fprintf(a, "\t\t\t\t\tNonterminal *n=(Nonterminal *)symbol_stack[i];\n");
        fprintf(a, "\t\t\t\t\tif(n->number()==nn)\n");
        fprintf(a, "\t\t\t\t\t\treturn i;\n");
        fprintf(a, "\t\t\t\t}}\n");
        fprintf(a, "\t\t\treturn -1;\n");
        fprintf(a, "\t\t}\n");

#if 0
        if(WhaleData::data.variables.make_creator_lookup_facility)
        {
                fprintf(a, "\t\tbool is_creator(int nn) const\n");
                fprintf(a, "\t\t{\n");
                fprintf(a, "\t\t\t// Ought to rewrite it using a table.\n");
                fprintf(a, "\t\t\t\n");

                fprintf(a, "\t\t\treturn ");
                vector<int> creators;
                for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
                        if(WhaleData::data.nonterminals[i].category==NonterminalData::CREATOR)
                                creators.push_back(i);
                if(creators.size()==0)
                        fprintf(a, "false");
                else
                        for(int i=0; i<creators.size(); i++)
                        {
                                if(i>0) fprintf(a, " || ");
                                fprintf(a, "nn==%d", creators[i]);
                        }
                fprintf(a, ";\n");

                fprintf(a, "\t\t}\n");
        }
#endif

        if(WhaleData::data.variables.code_in_parser_class)
                fprintf(a, "\t\t\n\t\t%s\n", WhaleData::data.variables.code_in_parser_class);

        fprintf(a, "\t};\n");
}

template<class T> void generate_single_scalar_constant(FILE *a, const char *indent, const char *type, const char *id, T value, const char *format, bool it_is_cpp)
{
        if(!it_is_cpp)
        {
                //string s=string("%sstatic const %s %s=")+string(format)+string(";\n");
                //fprintf(a, s.c_str(), indent, type, id, value);
                fprintf(a, "%senum {%s=%d};\n", indent, id, (int)value);
        }
        else {
//                fprintf(a, "%sconst %s %s::%s::%s;\n", indent, type, WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class,id);
             }
}

void generate_scalar_constants(FILE *a, const char *indent, bool it_is_cpp)
{
        generate_single_scalar_constant(a, indent, "int", "assumed_number_of_bits_in_int", WhaleData::data.variables.assumed_number_of_bits_in_int, "%u", it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "number_of_terminals", WhaleData::data.terminals.size(), "%u", it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "number_of_nonterminals", WhaleData::data.nonterminals.size(), "%u", it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "number_of_lr_states", WhaleData::data.lr_automaton.states.size(), "%u", it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "number_of_rules", WhaleData::data.rules.size(), "%u", it_is_cpp);
//      generate_single_scalar_constant(a, indent, "int", "rule_body_table_size", WhaleData::data.tables.rule_bodies.size(), "%u", it_is_cpp);
        if(WhaleData::data.variables.compress_action_table)
                generate_single_scalar_constant(a, indent, "int", "compressed_action_table_size", WhaleData::data.tables.lr_action.size(), "%u", it_is_cpp);
        if(WhaleData::data.variables.using_error_map)
                generate_single_scalar_constant(a, indent, "int", "compressed_action_error_map_size", WhaleData::data.tables.lr_action_error_map.size(), "%u", it_is_cpp);
        if(WhaleData::data.variables.compress_goto_table)
                generate_single_scalar_constant(a, indent, "int", "compressed_goto_table_size", WhaleData::data.tables.lr_goto.size(), "%u", it_is_cpp);
        if(WhaleData::data.variables.generate_table_of_pointers_to_members)
                generate_single_scalar_constant(a, indent, "int", "table_of_pointers_to_members_size", WhaleData::data.tables.all_data_members.size(), "%u", it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "eof_terminal_number", WhaleData::data.eof_terminal_number, "%u", it_is_cpp);
        generate_single_scalar_constant(a, indent, "int", "error_terminal_number", WhaleData::data.error_terminal_number, "%u", it_is_cpp);
}

void generate_table_of_states(FILE *a)
{
        fprintf(a, "const %s::%s::StateData %s::%s::states[%s::%s::number_of_lr_states]={\n",
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_class,
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_class,
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_class);

        // LR automaton states.
        for(int i=0; i<WhaleData::data.lr_automaton.states.size(); i++)
        {
                LRAutomaton::State &state=WhaleData::data.lr_automaton.states[i];

                if(i) fprintf(a, ",\n");
                fprintf(a, "\t{ ");

                if(!WhaleData::data.variables.compress_action_table)
                {
                        fprintf(a, "{ ");
                        for(int j=0; j<WhaleData::data.terminals.size(); j++)
                        {
                                if(j) fprintf(a, ", ");
                                fprintf(a, "%d", state.action_table[j]);
                        }
                        fprintf(a, " }");
                }
                else
                        generate_index_entry_to_access_compressed_table(a, WhaleData::data.tables.lr_action_indices[i], WhaleData::data.variables.action_table_compression_mode);

                if(WhaleData::data.variables.using_error_map)
                {
                        fprintf(a, ", ");
                        generate_index_entry_to_access_compressed_table(a, WhaleData::data.tables.lr_action_error_map_indices[i], 1);
                }

                fprintf(a, ", ");

                if(!WhaleData::data.variables.compress_goto_table)
                {
                        fprintf(a, "{ ");
                        for(int j=0; j<WhaleData::data.nonterminals.size(); j++)
                        {
                                if(j) fprintf(a, ", ");
                                if(state.goto_table[j]!=-1)
                                        fprintf(a, "%u", state.goto_table[j]);
                                else
                                        fprintf(a, "0");
                        }
                        fprintf(a, " }");
                }
                else
                        generate_index_entry_to_access_compressed_table(a, WhaleData::data.tables.lr_goto_indices[i], WhaleData::data.variables.goto_table_compression_mode);

                fprintf(a, " }");
        }
        fprintf(a, "\n};\n");
}

void generate_index_entry_to_access_compressed_table(FILE *a, IndexEntry &ie, int mode)
{
        if(mode<=2)
                fprintf(a, "%d", ie.offset);
        else if(mode==3)
        {
                if(ie.it_is_offset)
                        fprintf(a, "false, %d", ie.offset);
                else
                        fprintf(a, "true, %d", ie.value);
        }
}

void generate_table_of_pointers_to_members(FILE *a)
{
        fprintf(a, "%s::Symbol *%s::Nonterminal::*pointers_to_members[%s::%s::table_of_pointers_to_members_size]={\n",
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_class);

        for(int i=0; i<WhaleData::data.tables.all_data_members.size(); i++)
        {
                ClassHierarchy::DataMember *dm=WhaleData::data.tables.all_data_members[i];
                if(!dm->type->is_internal) continue;

                if(i) fprintf(a, ",\n");
        //      fprintf(a, "\t(%s *%s::*)&%s", dm->type->get_full_name().c_str(), dm->belongs_to->get_full_name().c_str(), dm->name.c_str());
                fprintf(a, "\t(%s::Symbol *%s::Nonterminal::*)&%s::%s::%s",
                        WhaleData::data.variables.whale_namespace,
                        WhaleData::data.variables.whale_namespace,
                        WhaleData::data.variables.whale_namespace,
                        dm->belongs_to->get_full_name().c_str(),
                        dm->name.c_str());
        }

        fprintf(a, "\n};\n");
}

void generate_table_of_rules(FILE *a)
{
        fprintf(a, "const %s::%s::RuleData %s::%s::rules[%s::%s::number_of_rules]={\n",
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_class,
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_class,
                WhaleData::data.variables.whale_namespace,
                WhaleData::data.variables.whale_class);

        for(int i=0; i<WhaleData::data.rules.size(); i++)
        {
        #ifdef MAKE_TABLE_OF_RULE_BODIES
                if(i) fprintf(a, ",\n");
        #else
                if(i) fprintf(a, (i%4==0 ? ",\n" : ","));
        #endif

                fprintf(a, "\t{ %u", WhaleData::data.rules[i].nn);

        #ifdef MAKE_TABLE_OF_RULE_BODIES
                if(WhaleData::data.rules[i].body.size())
                        fprintf(a, ", rule_bodies%s, %u", printable_increment(WhaleData::data.tables.rule_body_indices[i]), WhaleData::data.rules[i].body.size());
                else
                        fprintf(a, ", NULL, 0");
        #else
                if(WhaleData::data.rules[i].body.size())
                        fprintf(a, ", %u", unsigned(WhaleData::data.rules[i].body.size()));
                else
                        fprintf(a, ", 0");
        #endif
                fprintf(a, " }");
        }

        fprintf(a, "\n};\n");
}

void generate_raw_tables(FILE *a, const char *indent, bool it_is_cpp)
{
#ifdef MAKE_TABLE_OF_RULE_BODIES
        if(!it_is_cpp)
                fprintf(a, "%sstatic const int rule_bodies[rule_body_table_size];\n", indent);
        else
        {
                fprintf(a, "\nconst int %s::%s::rule_bodies[%s::%s::rule_body_table_size]={\n",
                        WhaleData::data.variables.whale_namespace,
                        WhaleData::data.variables.whale_class,
                        WhaleData::data.variables.whale_namespace,
                        WhaleData::data.variables.whale_class
                        );
                TablePrinter rule_bodies_printer(a, "\t", "%3d", 12);
                rule_bodies_printer.print(WhaleData::data.tables.rule_bodies);
                fprintf(a, "\n};\n");
        }
#endif
        if(WhaleData::data.variables.compress_action_table)
        {
                if(!it_is_cpp)
                        fprintf(a, "%sstatic const int compressed_action_table[compressed_action_table_size];\n", indent);
                else
                {
                        fprintf(a, "\nconst int %s::%s::compressed_action_table[%s::%s::compressed_action_table_size]={\n",
                                WhaleData::data.variables.whale_namespace,
                                WhaleData::data.variables.whale_class,
                                WhaleData::data.variables.whale_namespace,
                                WhaleData::data.variables.whale_class);
                        TablePrinter action_table_printer(a, "\t", "%3d", 12);
                        action_table_printer.print(WhaleData::data.tables.lr_action);
                        fprintf(a, "\n};\n");
                }
        }
        if(WhaleData::data.variables.using_error_map)
        {
                if(!it_is_cpp)
                        fprintf(a, "%sstatic const unsigned int compressed_action_error_map[compressed_action_error_map_size];\n", indent);
                else
                {
                        fprintf(a, "\nconst unsigned int %s::%s::compressed_action_error_map[%s::%s::compressed_action_error_map_size]={\n",
                                WhaleData::data.variables.whale_namespace,
                                WhaleData::data.variables.whale_class,
                                WhaleData::data.variables.whale_namespace,
                                WhaleData::data.variables.whale_class);
                        TablePrinter action_table_printer(a, "\t", "0x%08x", 6);
                        action_table_printer.print(WhaleData::data.tables.lr_action_error_map);
                        fprintf(a, "\n};\n");
                }
        }
        if(WhaleData::data.variables.compress_goto_table)
        {
                if(!it_is_cpp)
                        fprintf(a, "%sstatic const int compressed_goto_table[compressed_goto_table_size];\n", indent);
                else
                {
                        fprintf(a, "\nconst int %s::%s::compressed_goto_table[%s::%s::compressed_goto_table_size]={\n",
                                WhaleData::data.variables.whale_namespace,
                                WhaleData::data.variables.whale_class,
                                WhaleData::data.variables.whale_namespace,
                                WhaleData::data.variables.whale_class);
                        TablePrinter goto_table_printer(a, "\t", "%3d", 12);
                        goto_table_printer.print(WhaleData::data.tables.lr_goto);
                        fprintf(a, "\n};\n");
                }
        }
}

void generate_data_member(ClassHierarchy::DataMember *m, FILE *a, const char *indent)
{
        assert(m && m->type);
        int number_of_iterations=m->number_of_nested_iterations;

        string type_name("");

        for(int i=0; i<number_of_iterations; i++){
                type_name+=WhaleData::data.variables.std_namespace;
                type_name+="::vector<";
        }

//      cout << "(" << m->type << ", " << m << ") TRYING...";
        type_name+=m->type->name;
//      cout << " Ok.\n";

        if(m->type->is_internal)
                type_name+=" *";

        if(type_name.back()=='>')
                type_name+=" ";

        for(int i=0; i<number_of_iterations; i++)
                type_name+="> ";

        char c=type_name.back();
        if(c!='*' && c!='&' && c!=' ')
                type_name+=" ";

        fprintf(a, "%s%s%s;", indent, type_name.c_str(), m->name.c_str());
        if(m->internal_type_if_there_is_an_external_type)
        {
                fprintf(a, " // internal type: %s", m->internal_type_if_there_is_an_external_type->name.c_str());
        }
        fprintf(a, "\n");
}

void generate_single_class(ClassHierarchy::Class *m, FILE *a, const char *indent, int mode)
{
        if(m->is_a_built_in_template)
                return;

        if(mode==1)
        {
                fprintf(a, "%sstruct %s;\n", indent, m->name.c_str());
        }
        else if(mode==2)
        {
                if(m->ancestor)
                {
                        fprintf(a, "%sstruct %s : %s", indent, m->name.c_str(), m->ancestor->name.c_str());
                        if(m->template_parameters.size())
                        {
                                for(int i=0; i<m->template_parameters.size(); i++)
                                {
                                        fprintf(a, (!i ? "<" : ", "));
                                        fprintf(a, "%s", m->template_parameters[i]->get_full_name().c_str());
                                }
                                fprintf(a, ">\n");
                        }
                        else
                                fprintf(a, "\n");
                }
                else
                {
                        assert(!m->template_parameters.size());
                        fprintf(a, "%sstruct %s\n", indent, m->name.c_str());
                }
                fprintf(a, "%s{\n", indent);

                string fi_string=string(indent)+string("\t");
                const char *further_indent=fi_string.c_str();

                if(m->in_our_scope.size())
                {
                        for(int i=0; i<m->in_our_scope.size(); i++)
                                generate_single_class(m->in_our_scope[i], a, further_indent, mode);

                        fprintf(a, "%s\n", further_indent);
                }

                if(m->name=="Symbol")
                {
                        fprintf(a, "%svirtual bool is_terminal() const =0;\n", further_indent);
                        fprintf(a, "%svirtual bool is_nonterminal() const =0;\n", further_indent);
                //      if(WhaleData::data.variables.make_up_connection)
                //              fprintf(a, "%s", further_indent);
                }
                else if(m->name=="Terminal")
                {
                        fprintf(a, "%sint line, column;\n", further_indent);
                        fprintf(a, "%schar *text;\n", further_indent);
                        fprintf(a, "%s\n", further_indent);
                        /*fprintf(a, "%svoid print_location(std::ostream &os) const\n", further_indent);
                        fprintf(a, "%s{\n", further_indent);
                        fprintf(a, "%s\tos << \x22line \x22 << line << \x22 column \x22 << column;\n", further_indent);
                        fprintf(a, "%s}\n", further_indent);*/
                        fprintf(a, "%sbool is_terminal() const { return true; }\n", further_indent);
                        fprintf(a, "%sbool is_nonterminal() const { return false; }\n", further_indent);
                        //fprintf(a, "%svirtual int number() const =0;\n", further_indent);
                }
                else if(m->name=="Nonterminal")
                {
                        fprintf(a, "%sbool is_terminal() const { return false; }\n", further_indent);
                        fprintf(a, "%sbool is_nonterminal() const { return true; }\n", further_indent);
                        fprintf(a, "%svirtual int number() const =0;\n", further_indent);
                }
                else if(m->name=="Creator")
                {
                        fprintf(a, "%svirtual Nonterminal *object()=0;\n", further_indent);
                }
                else if(m->is_abstract)
                {
                //      fprintf(a, "/* here */\n");
                }
                else if(m->assigned_to.is_terminal())
                {
                        if(WhaleData::data.variables.generate_tokens){
                                std::string ss="";
                                if(WhaleData::data.variables.tokens_namespace){
                                  ss=WhaleData::data.variables.tokens_namespace;
                                  ss+="::";
                                }
                                if(WhaleData::data.variables.tokens_prefix){
                                  ss+=WhaleData::data.variables.tokens_prefix;
                                }
                                //fprintf(a, "%sstatic const int number=%s%s;\n", further_indent,ss.c_str(),m->name.c_str());
                                fprintf(a, "%senum{number=%s%s};\n", further_indent,ss.c_str(),m->name.c_str());
                        } else {
                                //fprintf(a, "%sstatic const int number=%d;\n", further_indent, m->assigned_to.terminal_number());
                                fprintf(a, "%senum{number=%d};\n", further_indent, m->assigned_to.terminal_number());
                        }
                        fprintf(a, "%s%s(){num=number;};\n", further_indent, m->name.c_str());

                        if(m->assigned_to.terminal_number()==WhaleData::data.error_terminal_number)
                        {
                                fprintf(a, "%s\n", further_indent);
                                if(WhaleData::data.variables.error_garbage){
                                  fprintf(a, "%sstd::vector<Symbol *> garbage;\n", further_indent);
                                  fprintf(a, "%sint error_position;\n", further_indent);
                                }
                        }
                }
                else if(m->assigned_to.is_nonterminal() &&
                        (m->nonterminal_class_category==ClassHierarchy::Class::SINGLE
                        || m->nonterminal_class_category==ClassHierarchy::Class::WRAPPER
                        || m->nonterminal_class_category==ClassHierarchy::Class::BASE))
                {
                        bool it_is_the_only_class=m->nonterminal_class_category==ClassHierarchy::Class::SINGLE;
                        int nn=m->assigned_to.nonterminal_number();
                        NonterminalData &nonterminal=WhaleData::data.nonterminals[nn];
                        //fprintf(a, "%sstatic const int number=%d;\n", further_indent, nn);
                        fprintf(a, "%senum{number=%d};\n", further_indent, nn);
                        if(WhaleData::data.variables.gen_sten){
                                fprintf(a, "%s%s(int sf,int sl,int sc,int ef,int el,int ec):Nonterminal(sf,sl,sc,ef,el,ec){num=number;}\n", further_indent, m->name.c_str());
                        } else
                                //fprintf(a, "%s%s():Nonterminal(){num=number;}\n", further_indent, m->name.c_str());
                                fprintf(a, "%s%s(){num=number;}\n", further_indent, m->name.c_str());
                        //fprintf(a, "%sint number() const { return %d; }\n", further_indent, nn);
                        //fprintf(a, "%sint number() const { return %d; }\n", further_indent, nn);

                        if(nonterminal.category==NonterminalData::CREATOR)
                        {
                        //      fprintf(a, "%sNonterminal *object() { return static_cast<Nonterminal *>(n); }\n", further_indent);
                                fprintf(a, "%sNonterminal *object() { return (Nonterminal *)n; }\n", further_indent);
                        }

                        if(m->nonterminal_class_category==ClassHierarchy::Class::BASE)
                                fprintf(a, "%svirtual int alternative_number() const =0;\n", further_indent);
                        else if(m->nonterminal_class_category==ClassHierarchy::Class::WRAPPER)
                        {
                                assert(nonterminal.external_type);
                        //      fprintf(a, "%s\n", further_indent);
                        //      fprintf(a, "%s%s contents;\n", further_indent, nonterminal.external_type->name.c_str());
                        //      fprintf(a, "%s// [ WRAPPER ]\n", further_indent);
                        }
                }
                else if(m->assigned_to.is_nonterminal() && m->nonterminal_class_category==ClassHierarchy::Class::SPECIFIC)
                {
                        fprintf(a, "%sint alternative_number() const { return %d; }\n", further_indent, m->nonterminal_alternative_number);
                }
                else
                        assert(false);

//              fprintf(a, "%s// Assigned to %s, nonterminal_class_category=%d\n", further_indent, WhaleData::data.full_symbol_name(m->assigned_to, false).c_str(), m->nonterminal_class_category);

                if(m->data_members.size() &&
                        (!m->assigned_to.is_nonterminal() ||
                        m->nonterminal_class_category!=ClassHierarchy::Class::WRAPPER))
                {
                        fprintf(a, "%s\n", further_indent);

                        for(int i=0; i<m->data_members.size(); i++)
                                generate_data_member(m->data_members[i], a, further_indent);
                }

                if(m->code)
                        fprintf(a, "%s%s\n", further_indent, m->code);

                fprintf(a, "%s};\n", indent);
        }
        else
                assert(false);
}

void generate_class_proc(ClassHierarchy::Class *m, FILE *a, const char *indent, int mode, int submode, bool generate_top_class)
{
        assert(submode==1 || submode==2);
        if((submode==1 && m->assigned_to) || (submode==2 && m->nonterminal_class_category==ClassHierarchy::Class::SPECIFIC))
                return;

        if(generate_top_class)
                generate_single_class(m, a, indent, mode);

        for(set<ClassHierarchy::Class *>::const_iterator p=m->descendants.begin(); p!=m->descendants.end(); p++)
                generate_class_proc(*p, a, indent, mode, submode, true);
}

void generate_symbol_class_templates(FILE *a, const char *indent)
{
        if(classes.single_iterator)
        {
                fprintf(a, "%stemplate<class Body> struct Iterator : Nonterminal\n", indent);
                fprintf(a, "%s{\n", indent);
                fprintf(a, "%s\t%s::vector<Body *> body;\n", indent,WhaleData::data.variables.std_namespace);
                fprintf(a, "%s};\n", indent);
        }
        if(classes.pair_iterator)
        {
                fprintf(a, "%stemplate<class BodyA, class BodyB> struct PairIterator : Nonterminal\n", indent);
                fprintf(a, "%s{\n", indent);
                fprintf(a, "%s\t%s::vector<BodyA *> body_a;\n", indent,WhaleData::data.variables.std_namespace);
                fprintf(a, "%s\t%s::vector<BodyB *> body_b;\n", indent,WhaleData::data.variables.std_namespace);
                fprintf(a, "%s};\n", indent);
        }
        if(classes.wrapper)
        {
                fprintf(a, "%stemplate<class T> struct Wrapper : Nonterminal\n", indent);
                fprintf(a, "%s{\n", indent);
                fprintf(a, "%s\tT x;\n", indent);
                fprintf(a, "%s};\n", indent);
        }
#if 0
        if(classes.basic_creator)
        {
                fprintf(a, "%sstruct BasicCreator : Nonterminal\n", indent);
                fprintf(a, "%s{\n", indent);
                fprintf(a, "%s\tvirtual Nonterminal *object()=0;\n", indent);
                fprintf(a, "%s};\n", indent);
        }
        if(classes.creator)
        {
        //      assert(classes.basic_creator);
        //      fprintf(a, "%stemplate<class NonterminalType> struct Creator : BasicCreator\n", indent);

                fprintf(a, "%stemplate<class NonterminalType> struct Creator : Nonterminal\n", indent);
                fprintf(a, "%s{\n", indent);
                fprintf(a, "%s\tNonterminalType *n;\n", indent);
        //      fprintf(a, "%s\t\n", indent);
        //      fprintf(a, "%s\tNonterminal *object() { return n; }\n", indent);
                fprintf(a, "%s};\n", indent);
        }
#endif
}

void generate_classes(FILE *a, const char *indent, int mode,int mode2)
{
        assert(mode==1 || mode==2);

        // i. Symbol, Terminal and Nonterminal.
        // ii. Abstract terminal classes
        // iii. Terminal classes.
        // iv. Abstract nonterminal classes.
        // v. For each nonterminal: base class, then abstract classes, then
        //      alternatives.

        //generate_class_proc(classes.terminal, a, indent, mode, 1, false);
        if(mode2&1)
            for(int i=2; i<WhaleData::data.terminals.size(); i++)
                    generate_single_class(WhaleData::data.terminals[i].type, a, indent, mode);

        if(mode2&2){
            fprintf(a, "%s\n", indent);
            //generate_class_proc(classes.nonterminal, a, indent, mode, 1, false);

            for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
            {
                    NonterminalData &nonterminal=WhaleData::data.nonterminals[i];

                    if(!nonterminal.is_ancillary)
                    {
                            generate_class_proc(nonterminal.type, a, indent, mode, 2, true);

                            for(int j=0; j<nonterminal.alternatives.size(); j++)
                            {
                                    AlternativeData &alternative=nonterminal.alternatives[j];
                                    if(alternative.type)
                                            generate_single_class(alternative.type, a, indent, mode);
                            }
                    }
            }

            if(mode==1)
            {
                    //if(WhaleData::data.variables.make_up_connection)
                    //      generate_single_class(classes.nonterminal, a, indent, 1);
                    //generate_single_class(classes.symbol, a, indent, 2);
                    //generate_single_class(classes.terminal, a, indent, 2);
                    //generate_single_class(classes.nonterminal, a, indent, 2);
            //      if(classes.creator)
            //              generate_single_class(classes.creator, a, indent, 2);
                    generate_symbol_class_templates(a, indent);
                    fprintf(a, "%s\n", indent);
            }
        }
}

void generate_parse_function(FILE *a)
{
        const char *S_name=WhaleData::data.nonterminals[WhaleData::data.start_nonterminal_number].type->name.c_str();
//,symbols(tmpmem)
        fprintf(a, "%s::%s::%s(%s &lexical_analyzer_supplied) : lexical_analyzer(lexical_analyzer_supplied)\n", WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class,WhaleData::data.variables.whale_class,WhaleData::data.variables.lexical_analyzer_class);
        fprintf(a, "{\n");
        fprintf(a, "\tinitialize();\n");
        if(WhaleData::data.variables.code_in_constructor)
                fprintf(a, "\t\n\t%s\n", WhaleData::data.variables.code_in_constructor);
        fprintf(a, "}\n");


        fprintf(a, "%s *%s::%s::parse()\n", S_name, WhaleData::data.variables.whale_namespace,WhaleData::data.variables.whale_class);
        fprintf(a, "{\n");
        fprintf(a, "\tfor(;;)\n");
        fprintf(a, "\t{\n");
        fprintf(a, "\t\tint state=state_stack.back();\n");
        if(WhaleData::data.variables.input_queue)
                fprintf(a, "\t\tif(input_symbols.empty()) input_symbols.push((Terminal*)lexical_analyzer.get_terminal());\n");

        const char *ref_to_input_symbol=(WhaleData::data.variables.input_queue ? "input_symbols.front()" : "input_symbol");

        fprintf(a, "\t\tLRAction lr_action=access_action_table(state, %s->number());\n", ref_to_input_symbol);
        fprintf(a, "\t\tif(lr_action.is_shift() && %s->number()!=error_terminal_number)\n", ref_to_input_symbol);
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\tint new_state=lr_action.shift_state();\n");
        if(WhaleData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << \x22Shift \x22 << new_state << \x22\\n\x22;\n");
        fprintf(a, "\t\t\tsymbol_stack.push_back(%s);\n", ref_to_input_symbol);
        fprintf(a, "\t\t\tstate_stack.push_back(new_state);\n");
        if(!WhaleData::data.variables.input_queue)
                fprintf(a, "\t\t\tinput_symbol=(Terminal*)lexical_analyzer.get_terminal();\n");
        else
                fprintf(a, "\t\t\tinput_symbols.pop();\n");
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t\telse if(lr_action.is_reduce())\n");
        fprintf(a, "\t\t{\n");
        fprintf(a, "\t\t\tint rule_number=lr_action.reduce_rule();\n");
        if(WhaleData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << \x22Reduce \x22 << rule_number << \x22\\n\x22;\n");
        fprintf(a, "\t\t\tint rule_length=rules[rule_number].length;\n");
        fprintf(a, "\t\t\tint rule_start=symbol_stack.size()-rule_length;\n");
        if(WhaleData::data.variables.generate_sanity_checks)
                fprintf(a, "\t\t\tif(rule_start<0) throw logic_error(\x22Whale: rule_start<0\x22);\n");
        fprintf(a, "\t\t\tNonterminal *new_symbol;\n");
        fprintf(a, "\t\t\t\n");
        //fprintf(a, "\t\t\t{\n");
        if(WhaleData::data.variables.gen_sten){
          fprintf(a, "\t\t\tint start_file,start_line,start_col,end_file,end_line,end_col;\n");
          fprintf(a, "\t\t\tif(rule_length!=0){\n");
          fprintf(a, "\t\t\t\tstart_file=symbol_stack[rule_start]->file_start;\n");
          fprintf(a, "\t\t\t\tstart_line=symbol_stack[rule_start]->line_start;\n");
          fprintf(a, "\t\t\t\tstart_col=symbol_stack[rule_start]->col_start;\n");

          fprintf(a, "\t\t\t\tend_file=symbol_stack[rule_start+rule_length-1]->file_end;\n");
          fprintf(a, "\t\t\t\tend_line=symbol_stack[rule_start+rule_length-1]->line_end;\n");
          fprintf(a, "\t\t\t\tend_col=symbol_stack[rule_start+rule_length-1]->col_end;\n");
          fprintf(a, "\t\t\t}else{\n");
          fprintf(a, "\t\t\t\tstart_file=input_symbol->file_start;\n");
          fprintf(a, "\t\t\t\tstart_line=input_symbol->line_start;\n");
          fprintf(a, "\t\t\t\tstart_col=input_symbol->col_start;\n");

          fprintf(a, "\t\t\t\tend_file=start_file;\n");
          fprintf(a, "\t\t\t\tend_line=start_line;\n");
          fprintf(a, "\t\t\t\tend_col=start_col;\n");
          fprintf(a, "\t\t\t}\n");
        }
        //fprintf(a, "\t\t\t}\n");

        fprintf(a, "\t\t\tswitch(rule_number)\n");
        fprintf(a, "\t\t\t{\n");
        for(int i=1 /* S->S' excluded */; i<WhaleData::data.rules.size(); i++)
                generate_reduce_code(a, "\t\t\t", i);
        if(WhaleData::data.variables.generate_sanity_checks)
                fprintf(a, "\t\t\tdefault:\n"
                        "\t\t\t\tthrow logic_error(\x22Whale: Invalid rule number.\x22);\n");
        fprintf(a, "\t\t\t}\n");

        fprintf(a, "\t\t\t\n");
        fprintf(a, "\t\t\t{for(int i=0; i<rule_length; i++)\n");
        fprintf(a, "\t\t\t{\n");
        fprintf(a, "\t\t\t\tsymbol_stack.pop_back();\n");
        fprintf(a, "\t\t\t\tstate_stack.pop_back();\n");
        fprintf(a, "\t\t\t}}\n");
        fprintf(a, "\t\t\tint nn=rules[rule_number].nn;\n");
        fprintf(a, "\t\t\tsymbol_stack.push_back(new_symbol);\n");
        fprintf(a, "\t\t\tstate_stack.push_back(access_goto_table(state_stack.back(), nn));\n");
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t\telse if(lr_action.is_accept())\n");
        fprintf(a, "\t\t{\n");
        if(WhaleData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << \x22" "Accept\\n\x22;\n");
        if(WhaleData::data.variables.generate_sanity_checks)
                fprintf(a, "\t\t\tif(symbol_stack.size()!=1)\n"
                        "\t\t\t\tthrow logic_error(\x22Whale: Stack size !=1 while accepting.\x22);\n");
        fprintf(a, "\t\t\treturn (%s *)(symbol_stack.back());\n", S_name);
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t\telse\n");
        fprintf(a, "\t\t{\n");
        if(WhaleData::data.variables.generate_verbose_prints)
                fprintf(a, "\t\t\tcout << \x22" "Error\\n\x22;\n");
        if(WhaleData::data.variables.error_handler)
        {
                fprintf(a, "\t\t\t// User-specified error handler.\n");
                fprintf(a, "\t\t\t%s\n", WhaleData::data.variables.error_handler);
        }
        else
        {
                fprintf(a, "\t\t\tif(!%s->is_terminal() || %s->number())\n", ref_to_input_symbol, ref_to_input_symbol);
                fprintf(a, "\t\t\t\treport_error(%s);\n", ref_to_input_symbol);
                fprintf(a, "\t\t\tif(!recover_from_error())\n");
                fprintf(a, "\t\t\t\treturn NULL;\n");
        }
        fprintf(a, "\t\t}\n");
        fprintf(a, "\t}\n");
        fprintf(a, "}\n");
}

void generate_reduce_code(FILE *a, const char *indent, int rn)
{
        RuleData &rule=WhaleData::data.rules[rn];
        NonterminalData &nonterminal=WhaleData::data.nonterminals[rule.nn];
        SymbolNumber sn=SymbolNumber::for_nonterminal(rule.nn);

        string fi=string(indent)+string("\t");
        const char *further_indent=fi.c_str();

        ClassHierarchy::Class *type=WhaleData::data.get_class_assigned_to_alternative(rule.nn, rule.an);

#ifdef ANNOUNCE_REDUCE_CODE_GENERATION
        cout << "Generating reduce code for nonterminal " << nonterminal.name << "\n";
#endif

        fprintf(a, "%scase %d: {\t// %s\n", indent, rn, rule.in_printable_form().c_str());

        if(nonterminal.category==NonterminalData::NORMAL ||
                nonterminal.category==NonterminalData::BODY)
        {
                AlternativeData &alternative=nonterminal.alternatives[rule.an];

                if(WhaleData::data.variables.generate_verbose_prints)
                        fprintf(a, "%scout << \x22Reducing by rule %s\\n\x22;\n", further_indent, prepare_for_printf(rule.in_printable_form()).c_str());

        //      const char *n_name=((nonterminal.category==NonterminalData::BODY &&
        //              rule.code_to_execute_upon_reduce) ? "body_n" : "n");

                const char *n_name=(nonterminal.category==NonterminalData::BODY ?
                        "body_n" : "n");
        //      string n_with_arrow=string(n_name)+string("->");

                int first_position;
                if(rule.create_and_update_operator_locations.size()==0)
                {
                        std::string params_string="";
                        if(WhaleData::data.variables.gen_sten){
                                params_string+="(start_file,start_line,start_col,end_file,end_line,end_col)";
                        }
                        fprintf(a, "%s%s *%s=(%s *)(new %s%s);\n", further_indent, type->get_full_name().c_str(), n_name, type->get_full_name().c_str(), type->get_full_name().c_str(),params_string.c_str());
                        first_position=0;
                }
                else
                {
                        SymbolNumber creator_sn=rule.body[rule.create_and_update_operator_locations[0]];
                        assert(creator_sn.is_nonterminal());
                        NonterminalData &creator=WhaleData::data.nonterminals[creator_sn.nonterminal_number()];

                        fprintf(a, "%s%s *%s=(", further_indent, type->get_full_name().c_str(), n_name);
                        generate_expression_that_gets_single_symbol_from_stack(a, creator.type, "rule_start", rule.create_and_update_operator_locations[0]);
                        fprintf(a, ")->n;\n");

                        first_position=rule.create_and_update_operator_locations.back()+1;
                }

                if(alternative.connect_up && rule.creator_nn==-1)
                {
                        /* here we have to look for a creator of our parent. */
                        /* (a remarkable situation: a completely born child  */
                        /* is looking for his parent, while the latter is    */
                        /* still in his egg, waiting to be born. This is     */
                        /* bottom-up parsing...)                             */

                //      if(WhaleData::data.variables.generate_sanity_checks)
                //      else

                //      assert(false);

                //      ***BUOY***

                        fprintf(a, "%s%s->up=dynamic_cast<Creator *>(symbol_stack[find_some_creator_in_stack(rule_start-1)])->object();\n", further_indent, n_name);
                }

                set<ClassHierarchy::DataMember *> initialized_data_members;

                if(first_position<rule.body.size())
                {
                        fprintf(a, "%s\n", further_indent);

                        for(int i=first_position; i<rule.expr_body.size(); i++)
                        {
                                ClassHierarchy::DataMember *dm=generate_code_that_copies_single_symbol_from_stack_to_one_or_more_data_members(a, further_indent, type, rule.expr_body[i], n_name, "rule_start", i);
                                if(dm!=NULL) initialized_data_members.insert(dm);
                        }

                        fprintf(a, "%s\n", further_indent);
                }

                if(rule.create_and_update_operator_locations.size()==0)
                        generate_code_that_initializes_all_remaining_data_members(a, further_indent, type, n_name, initialized_data_members);

                fprintf(a, "%snew_symbol=%s;\n", further_indent, n_name);

                // if it is an iteration body and there is a code to execute
                // upon reduce, then it's possible that there is a Creator to
                // connect to.
                if(nonterminal.category==NonterminalData::BODY &&
                        rule.code_to_execute_upon_reduce)
                {
                        fprintf(a, "%s\n", further_indent);
                        int creator_nn=find_creator_somewhere_around_rule(rn);
                        if(creator_nn==-1)
                                fprintf(a, "%s/* Code operates without an object */\n", further_indent);
                        else if(creator_nn==0)
                                generate_code_to_establish_connection_to_unknown_creator(a, further_indent, nonterminal.master_nn, nonterminal.master_an);
                        else
                        {
                                fprintf(a, "%s/* Establishing connection to the object */\n", further_indent);
                                generate_code_to_establish_connection_to_creator(a, further_indent, creator_nn);
                        }
                }
        }
        else if(nonterminal.category==NonterminalData::EXTERNAL)
        {
                fprintf(a, "%s/* Reduce to an external nonterminal. */\n", further_indent);
        }
        else if(nonterminal.category==NonterminalData::ITERATOR)
        {
                assert(rule.code_to_execute_upon_reduce==NULL);

                vector<ClassHierarchy::Class *> body_types;
                assert(nonterminal.type->template_parameters.size());
                for(int i=0; i<nonterminal.type->template_parameters.size(); i++)
                        body_types.push_back(nonterminal.type->template_parameters[i]);

                if(rule.body.size()==1)
                {
                        const char *v_name;
                        if(body_types.size()==1)
                                v_name="body";
                        else if(body_types.size()==2)
                                v_name="body_a";
                        else
                                assert(false);

                        if(WhaleData::data.variables.generate_verbose_prints)
                                fprintf(a, "%scout << \x22" "Creating iterator %s\\n\x22;\n", further_indent, nonterminal.name);
                        std::string params_string="";
                        if(WhaleData::data.variables.gen_sten){
                                params_string+="(start_file,start_line,start_col,end_file,end_line,end_col)";
                        }
                        fprintf(a, "%s%s *it=(%s *)(new %s%s);\n", further_indent, type->get_full_name().c_str(), type->get_full_name().c_str(), type->get_full_name().c_str(),params_string.c_str());

                        generate_code_that_fills_up_field(a, further_indent, expression_that_gets_single_symbol_from_stack(NULL, "rule_start", 0), "it", false);

                        fprintf(a, "%sit->%s.push_back(", further_indent, v_name);
                        generate_expression_that_gets_single_symbol_from_stack(a, body_types[0], "rule_start", 0);
                        fprintf(a, ");\n");

                        fprintf(a, "%snew_symbol=it;\n", further_indent);
                }
                else if(rule.body.size()==2)
                {
                        assert(body_types.size()==1);

                        if(WhaleData::data.variables.generate_verbose_prints)
                                fprintf(a, "%scout << \x22" "Updating iterator %s\\n\x22;\n", further_indent, nonterminal.name);

                        fprintf(a, "%s%s *it=", further_indent, type->get_full_name().c_str());
                        generate_expression_that_gets_single_symbol_from_stack(a, type, "rule_start", 0);
                        fprintf(a, ";\n");

                        generate_code_that_fills_up_field(a, further_indent, expression_that_gets_single_symbol_from_stack(NULL, "rule_start", 1), "it", false);

                        fprintf(a, "%sit->body.push_back(", further_indent);
                        generate_expression_that_gets_single_symbol_from_stack(a, body_types[0], "rule_start", 1);
                        fprintf(a, ");\n");

                        fprintf(a, "%snew_symbol=it;\n", further_indent);
                }
                else if(rule.body.size()==3)
                {
                        assert(body_types.size()==2);

                        if(WhaleData::data.variables.generate_verbose_prints)
                                fprintf(a, "%scout << \x22" "Updating pair iterator %s\\n\x22;\n", further_indent, nonterminal.name);

                        fprintf(a, "%s%s *it=", further_indent, type->get_full_name().c_str());
                        generate_expression_that_gets_single_symbol_from_stack(a, type, "rule_start", 0);
                        fprintf(a, ";\n");

                        generate_code_that_fills_up_field(a, further_indent, expression_that_gets_single_symbol_from_stack(NULL, "rule_start", 1), "it", false);

                        fprintf(a, "%sit->body_b.push_back(", further_indent);
                        generate_expression_that_gets_single_symbol_from_stack(a, body_types[1], "rule_start", 1);
                        fprintf(a, ");\n");

                        generate_code_that_fills_up_field(a, further_indent, expression_that_gets_single_symbol_from_stack(NULL, "rule_start", 2), "it", false);

                        fprintf(a, "%sit->body_a.push_back(", further_indent);
                        generate_expression_that_gets_single_symbol_from_stack(a, body_types[0], "rule_start", 2);
                        fprintf(a, ");\n");

                        fprintf(a, "%snew_symbol=it;\n", further_indent);
                }
                else
                        assert(false);
        }
        else if(nonterminal.category==NonterminalData::INVOKER)
        {
                assert(rule.code_to_execute_upon_reduce);
                std::string params_string="";
                if(WhaleData::data.variables.gen_sten){
                        params_string+="(start_file,start_line,start_col,end_file,end_line,end_col)";
                }
                fprintf(a, "%s%s *invoker=(%s *)(new %s%s);\n", further_indent, type->get_full_name().c_str(), type->get_full_name().c_str(), type->get_full_name().c_str(),params_string.c_str());
                fprintf(a, "%snew_symbol=invoker;\n", further_indent);

                int creator_nn=find_creator_somewhere_around_rule(rn);
                if(creator_nn==-1)
                        fprintf(a, "%s/* Code operates without an object */\n", further_indent);
                else if(creator_nn==0)
                        generate_code_to_establish_connection_to_unknown_creator(a, further_indent, nonterminal.master_nn, nonterminal.master_an);
                else
                {
                        fprintf(a, "%s/* Establishing connection to the object */\n", further_indent);
                        generate_code_to_establish_connection_to_creator(a, further_indent, creator_nn);
                }
        }
        else if(nonterminal.category==NonterminalData::CREATOR ||
                nonterminal.category==NonterminalData::UPDATER)
        {
                assert(rule.code_to_execute_upon_reduce==NULL);
                bool it_is_creator=(nonterminal.category==NonterminalData::CREATOR);

                assert(nonterminal.rules_where_ancillary_symbol_is_used.size()>0);
                int parent_rule_number=nonterminal.rules_where_ancillary_symbol_is_used[0];
                RuleData &parent_rule=WhaleData::data.rules[parent_rule_number];
                NonterminalData &parent_nonterminal=WhaleData::data.nonterminals[parent_rule.nn];
                AlternativeData &parent_alternative=parent_nonterminal.alternatives[parent_rule.an];

                ClassHierarchy::Class *parent_type=WhaleData::data.get_class_assigned_to_alternative(parent_rule.nn, parent_rule.an);

                assert(parent_rule.create_and_update_operator_locations.size()>=(it_is_creator ? 1 : 2));
                int first_position_in_this_rule; // Important note: for
                        // creators it may differ from rule to rule.
                int number_of_positions_to_process;
                if(it_is_creator)
                {
                        int creator_pos=parent_rule.create_and_update_operator_locations[0];
                        assert(parent_rule.body[creator_pos]==sn);
                        first_position_in_this_rule=0;
                        number_of_positions_to_process=creator_pos;
                }
                else
                {
                        int first_pos=-1, last_pos=-1;
                        for(int i=1; i<parent_rule.create_and_update_operator_locations.size(); i++)
                                if(parent_rule.body[parent_rule.create_and_update_operator_locations[i]]==sn)
                                {
                                        first_pos=parent_rule.create_and_update_operator_locations[i-1]+1;
                                        last_pos=parent_rule.create_and_update_operator_locations[i];
                                }
                        assert(first_pos>0 && last_pos>0 && first_pos<=last_pos);
                        first_position_in_this_rule=first_pos;
                        number_of_positions_to_process=last_pos-first_pos;
                }

                if(it_is_creator)
                {
                        std::string params_string="";
                        if(WhaleData::data.variables.gen_sten){
                                params_string+="(start_file,start_line,start_col,end_file,end_line,end_col)";
                        }
                        fprintf(a, "%s%s *creator=(%s *)(new %s%s);\n", further_indent, type->get_full_name().c_str(), type->get_full_name().c_str(), type->get_full_name().c_str(),params_string.c_str());
                        fprintf(a, "%screator->n=new %s;\n", further_indent, parent_type->get_full_name().c_str());

                        if(parent_alternative.connect_up)
                        {
                                // ***BUOY*** II

                                fprintf(a, "%screator->n->up=dynamic_cast<Creator *>(symbol_stack[find_some_creator_in_stack(rule_start-1)])->object();\n", further_indent);
                        }
                }
                else{
                        std::string params_string="";
                        if(WhaleData::data.variables.gen_sten){
                                params_string+="(start_file,start_line,start_col,end_file,end_line,end_col)";
                        }
                        fprintf(a, "%s%s *updater=(%s *)(new %s%s);\n", further_indent, type->get_full_name().c_str(), type->get_full_name().c_str(), type->get_full_name().c_str(),params_string.c_str());
                    }

                set<ClassHierarchy::DataMember *> initialized_data_members;

                if(number_of_positions_to_process)
                {
                        const char *base_pointer_name=(it_is_creator ? "parent_rule_start" : "initial_position");
                        fprintf(a, "%sint %s=symbol_stack.size()-%d;\n", further_indent, base_pointer_name, number_of_positions_to_process);
                        if(WhaleData::data.variables.generate_sanity_checks)
                                fprintf(a, "%sif(%s<0) throw logic_error(\x22Whale: %s<0\x22);\n", further_indent, base_pointer_name, base_pointer_name);

                        // updaters should establish connection to creators.
                        if(!it_is_creator)
                        {
                                SymbolNumber creator_sn=parent_rule.body[parent_rule.create_and_update_operator_locations[0]];
                                assert(creator_sn.is_nonterminal());
                                NonterminalData &creator_nonterminal=WhaleData::data.nonterminals[creator_sn.nonterminal_number()];
                                assert(creator_nonterminal.category==NonterminalData::CREATOR);

                                fprintf(a, "%sint creator_pos=find_nonterminal_in_stack(%d);\n", further_indent, creator_sn.nonterminal_number());
                                if(WhaleData::data.variables.generate_sanity_checks)
                                        fprintf(a, "%sif(creator_pos<0) throw logic_error(\x22Whale: Updater failed to find Creator in stack.\x22);\n", further_indent);
                                fprintf(a, "%s%s *creator=(%s *)symbol_stack[creator_pos];\n", further_indent, creator_nonterminal.type->get_full_name().c_str(), creator_nonterminal.type->get_full_name().c_str());
                        }

                        fprintf(a, "%s\n", further_indent);
                        for(int i=0; i<number_of_positions_to_process; i++)
                        {
                                ClassHierarchy::DataMember *dm=generate_code_that_copies_single_symbol_from_stack_to_one_or_more_data_members(a, further_indent, parent_type, parent_rule.expr_body[first_position_in_this_rule+i], "creator->n", base_pointer_name, i);
                                if(dm!=NULL) initialized_data_members.insert(dm);
                        }
                        fprintf(a, "%s\n", further_indent);
                }

                if(it_is_creator)
                {
                        generate_code_that_initializes_all_remaining_data_members(a, further_indent, parent_type, "creator->n", initialized_data_members);
                        fprintf(a, "%snew_symbol=creator;\n", further_indent);
                }
                else
                        fprintf(a, "%snew_symbol=updater;\n", further_indent);
        }
        else
                assert(false);

        if(rule.code_to_execute_upon_reduce)
        {
                const char *s=rule.code_to_execute_upon_reduce->code->text;
                int k=(s[0]=='{' ? 1 : 2);
                string str(s+k, s+strlen(s)-k);

                if(WhaleData::data.variables.generate_verbose_prints)
                        fprintf(a, "%scout << \x22" "Executing code invoked by %s\\n\x22;\n", further_indent, nonterminal.name);
                fprintf(a, "%s%s\n", further_indent, str.c_str());
        }

        fprintf(a, "%s} break;\n", further_indent);
}

// Returns one of WhaleData::data members initialized, preferrably of an internal type,
// preferrably vectorless.
ClassHierarchy::DataMember *generate_code_that_copies_single_symbol_from_stack_to_one_or_more_data_members(FILE *a, const char *indent, ClassHierarchy::Class *type, NonterminalExpression *expr, const char *access_to_nonterminal_object, const char *location_in_stack_base, int location_in_stack_offset)
{
        string nonterminal_with_arrow=string(access_to_nonterminal_object)+"->";

        if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
        {
                NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);

                ClassHierarchy::Class *type_we_expect_to_find_in_stack;
                ClassHierarchy::DataMember *target_data_member_i=NULL;
                ClassHierarchy::DataMember *target_data_member_e=NULL;
                for(int i=0; i<expr_s.data_members_in_bodies.size(); i++)
                        if(expr_s.data_members_in_bodies[i]->belongs_to==type)
                        {
                                target_data_member_i=expr_s.data_members_in_bodies[i];
                                break;
                        }
                if(!target_data_member_i)
                {
                        target_data_member_i=expr_s.data_member_i;
                        target_data_member_e=expr_s.data_member_e;
                }
                if(target_data_member_i)
                        type_we_expect_to_find_in_stack=target_data_member_i->type;
                else if(target_data_member_e)
                        type_we_expect_to_find_in_stack=target_data_member_e->internal_type_if_there_is_an_external_type;
                else
                        type_we_expect_to_find_in_stack=NULL;

                string how_to_get_symbol_from_stack=expression_that_gets_single_symbol_from_stack(type_we_expect_to_find_in_stack, location_in_stack_base, location_in_stack_offset);
                string how_to_get_symbol_from_stack_without_typecast=expression_that_gets_single_symbol_from_stack(NULL, location_in_stack_base, location_in_stack_offset);

                if(target_data_member_i)
                {
                        generate_code_that_fills_up_field(a, indent, how_to_get_symbol_from_stack_without_typecast, access_to_nonterminal_object, false);
                        generate_code_that_puts_symbol_to_data_member(a, indent, nonterminal_with_arrow.c_str(), target_data_member_i, 0, how_to_get_symbol_from_stack);
                }
                if(target_data_member_e)
                        generate_code_that_puts_symbol_to_data_member(a, indent, nonterminal_with_arrow.c_str(), target_data_member_e, 0, how_to_get_symbol_from_stack);
                if(!target_data_member_i && !target_data_member_e) // 'nothing'
                        generate_code_that_disposes_of_symbol(a, indent, how_to_get_symbol_from_stack);

                if(target_data_member_i)
                        return target_data_member_i;
                else
                        return target_data_member_e;
        }
        else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
        {
                NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
                NonterminalData &it_nonterminal=WhaleData::data.nonterminals[expr_it.iterator_nn];

                string iterator_name_string=string("it_")+roman_itoa(expr_it.local_iterator_number, false);
                const char *iterator_name=iterator_name_string.c_str();

                fprintf(a, "%s%s *%s=", indent, it_nonterminal.type->get_full_name().c_str(), iterator_name);
                generate_expression_that_gets_single_symbol_from_stack(a, it_nonterminal.type, location_in_stack_base, location_in_stack_offset);
                fprintf(a, ";\n");
                fprintf(a, "%s{for(int i=0; i<%s->body.size(); i++)\n", indent, iterator_name);
                fprintf(a, "%s{\n", indent);

                string further_indent_string=string(indent)+string("\t");

                // further behaviour depends on whether there is a special
                // body nonterminal or a single normal symbol is used instead.
                if(expr_it.body_sn.is_nonterminal() && WhaleData::data.nonterminals[expr_it.body_sn.nonterminal_number()].category==NonterminalData::BODY)
                {
                        NonterminalData &body_nonterminal=WhaleData::data.nonterminals[expr_it.body_sn.nonterminal_number()];

                        for(int i=0; i<body_nonterminal.type->data_members.size(); i++)
                        {
                                ClassHierarchy::DataMember *source_dm=body_nonterminal.type->data_members[i];
                                assert(source_dm->senior);

                                string how_to_get_symbol_from_iterator=string(iterator_name) +
                                        string("->body[i]->") + string(source_dm->name.c_str());
                        //      cout << how_to_get_symbol_from_iterator << "\n";

                                generate_code_that_fills_up_field(a, further_indent_string.c_str(), how_to_get_symbol_from_iterator, access_to_nonterminal_object, true);
                                generate_code_that_puts_symbol_to_data_member(a, further_indent_string.c_str(), nonterminal_with_arrow.c_str(), source_dm->senior, source_dm->number_of_nested_iterations, how_to_get_symbol_from_iterator);
                        }
                }
                else
                        assert(false);

                fprintf(a, "%s}}\n", indent);

                return NULL;
        }
        else if(typeid(*expr)==typeid(NonterminalExpressionIterationPair))
        {
                NonterminalExpressionIterationPair &expr_it2=*dynamic_cast<NonterminalExpressionIterationPair *>(expr);
                NonterminalData &it2_nonterminal=WhaleData::data.nonterminals[expr_it2.iterator_nn];

                string iterator_name_string=string("it2_")+roman_itoa(expr_it2.local_iterator_number, false);
                const char *iterator_name=iterator_name_string.c_str();

                fprintf(a, "%s%s *%s=", indent, it2_nonterminal.type->get_full_name().c_str(), iterator_name);
                generate_expression_that_gets_single_symbol_from_stack(a, it2_nonterminal.type, location_in_stack_base, location_in_stack_offset);
                fprintf(a, ";\n");
                fprintf(a, "%s{for(int i=0; i<%s->body_a.size(); i++)\n", indent, iterator_name);
                fprintf(a, "%s{\n", indent);

                string further_indent_string=string(indent)+string("\t");

                if(expr_it2.body1_sn.is_nonterminal() && WhaleData::data.nonterminals[expr_it2.body1_sn.nonterminal_number()].category==NonterminalData::BODY)
                {
                        NonterminalData &body1_nonterminal=WhaleData::data.nonterminals[expr_it2.body1_sn.nonterminal_number()];

                        for(int i=0; i<body1_nonterminal.type->data_members.size(); i++)
                        {
                                ClassHierarchy::DataMember *source_dm=body1_nonterminal.type->data_members[i];
                                assert(source_dm->senior);

                                string how_to_get_symbol_from_iterator=string(iterator_name) +
                                        string("->body_a[i]->") + string(source_dm->name.c_str());
                        //      cout << how_to_get_symbol_from_iterator << "\n";

                                generate_code_that_fills_up_field(a, further_indent_string.c_str(), how_to_get_symbol_from_iterator, access_to_nonterminal_object, true);
                                generate_code_that_puts_symbol_to_data_member(a, further_indent_string.c_str(), nonterminal_with_arrow.c_str(), source_dm->senior, source_dm->number_of_nested_iterations, how_to_get_symbol_from_iterator);
                        }
                }
                else
                        assert(false);

                fprintf(a, "%s\tif(i==%s->body_b.size()) break;\n", indent, iterator_name);

                if(expr_it2.body2_sn.is_nonterminal() && WhaleData::data.nonterminals[expr_it2.body2_sn.nonterminal_number()].category==NonterminalData::BODY)
                {
                        NonterminalData &body2_nonterminal=WhaleData::data.nonterminals[expr_it2.body2_sn.nonterminal_number()];

                        for(int i=0; i<body2_nonterminal.type->data_members.size(); i++)
                        {
                                ClassHierarchy::DataMember *source_dm=body2_nonterminal.type->data_members[i];
                                assert(source_dm->senior);

                                string how_to_get_symbol_from_iterator=string(iterator_name) +
                                        string("->body_b[i]->") + string(source_dm->name.c_str());
                        //      cout << how_to_get_symbol_from_iterator << "\n";

                                generate_code_that_fills_up_field(a, further_indent_string.c_str(), how_to_get_symbol_from_iterator, access_to_nonterminal_object, true);
                                generate_code_that_puts_symbol_to_data_member(a, further_indent_string.c_str(), nonterminal_with_arrow.c_str(), source_dm->senior, source_dm->number_of_nested_iterations, how_to_get_symbol_from_iterator);
                        }
                }
                else
                        assert(false);

                fprintf(a, "%s}}\n", indent);

                return NULL;
        }
        else if(typeid(*expr)==typeid(NonterminalExpressionCode))
        {
                return NULL;
        }
        else
        {
                // other: creators, updaters, precedence expressions.
                assert(false);
                return NULL;
        }
}

void generate_code_that_puts_symbol_to_data_member(FILE *a, const char *indent, const char *access_to_nonterminal_object, ClassHierarchy::DataMember *data_member, int number_of_iterations_in_source, const string &access_to_symbol)
{
        int number_of_iterations_to_add=data_member->number_of_nested_iterations - number_of_iterations_in_source;

//      if(WhaleData::data.variables.make_up_connection)
//              fprintf(a, "%s%sup=%s;\n", indent, access_to_symbol.c_str(), access_to_nonterminal_object);


        fprintf(a, "%s%s%s", indent, access_to_nonterminal_object, data_member->name.c_str());

        if(number_of_iterations_to_add==0)
                fprintf(a, "=");
        else
        {
                fprintf(a, ".push_back(");
                for(int i=1; i<number_of_iterations_to_add; i++)
                        fprintf(a, "deepen(");
        }

        if(!data_member->type->is_internal)
                fprintf(a, "convert(*");

        fprintf(a, "%s", access_to_symbol.c_str());

        if(!data_member->type->is_internal)
                fprintf(a, ")");

        if(number_of_iterations_to_add>0)
        {
                for(int i=1; i<number_of_iterations_to_add; i++)
                        fprintf(a, ")");
                fprintf(a, ")"); // push_back method call.
        }

        fprintf(a, ";\n");
}

// Fill "Symbol *Symbol::up". If make_up_connection=false, then this function
// generates no code.
void generate_code_that_fills_up_field(FILE *a, const char *indent, const string &access_to_symbol, const string &pointer_to_nonterminal_object, bool null_pointer_possible)
{
        if(!WhaleData::data.variables.make_up_connection) return;

        if(null_pointer_possible)
                fprintf(a, "%sif(%s) ", indent, access_to_symbol.c_str());
        else
        {
                if(WhaleData::data.variables.generate_sanity_checks)
                        fprintf(a, "%sif(%s==NULL) throw logic_error(\x22Whale: unexpected NULL pointer\x22);\n", indent, access_to_symbol.c_str());

                fprintf(a, "%s", indent);
        }
        fprintf(a, "%s->up=%s;\n", access_to_symbol.c_str(), pointer_to_nonterminal_object.c_str());
}

void generate_code_that_disposes_of_symbol(FILE *a, const char *indent, const string &access_to_symbol)
{
        if(WhaleData::data.variables.garbage){
          fprintf(a, "%sgarbage.push_back(%s);\n", indent, access_to_symbol.c_str());
        }
}

// symbol_type==NULL means no typecast;
string expression_that_gets_single_symbol_from_stack(ClassHierarchy::Class *symbol_type, const char *location_in_stack_base, int location_in_stack_offset)
{
        return (symbol_type ? string("(") + symbol_type->get_full_name() +
                string(" *)") : string("")) +
                string("symbol_stack[") + string(location_in_stack_base) +
                string(printable_increment(location_in_stack_offset)) +
                string("]");
}

void generate_expression_that_gets_single_symbol_from_stack(FILE *a, ClassHierarchy::Class *symbol_type, const char *location_in_stack_base, int location_in_stack_offset)
{
        fprintf(a, "%s", expression_that_gets_single_symbol_from_stack(symbol_type, location_in_stack_base, location_in_stack_offset).c_str());
}

void generate_code_that_initializes_all_remaining_data_members(FILE *a, const char *indent, ClassHierarchy::Class *type, const char *access_to_object, set<ClassHierarchy::DataMember *> &initialized_data_members)
{
        for(int i=0; i<type->data_members.size(); i++)
        {
                ClassHierarchy::DataMember *dm=type->data_members[i];

                if(dm->type->is_internal && dm->number_of_nested_iterations==0 &&
                        initialized_data_members.count(dm)==0)
                {
                        fprintf(a, "%s%s->%s=NULL;\n", indent, access_to_object, dm->name.c_str());
                }
        }
}

// returns -1, if there is no creator;
// 0, if the case is complicated (there is more than one suitable creator);
// the number of creator otherwise.
int find_creator_somewhere_around_rule(int rn)
{
        RuleData &rule=WhaleData::data.rules[rn];

        if(rule.creator_nn!=-1)
                return rule.creator_nn;

//      if(rule.create_and_update_operator_locations.size())
//      {
//              SymbolNumber sn=rule.body[rule.create_and_update_operator_locations[0]];
//              assert(sn.is_nonterminal());
//              return sn.nonterminal_number();
//      }

        NonterminalData &nonterminal=WhaleData::data.nonterminals[rule.nn];
        if(nonterminal.is_ancillary)
        {
                assert(nonterminal.master_nn>=0 && nonterminal.master_an>=0);
                AlternativeData &alternative=WhaleData::data.access_alternative(nonterminal.master_nn, nonterminal.master_an);

                if(alternative.creator_nn.size()==0)
                        return -1;
                else if(alternative.creator_nn.size()==1)
                        return alternative.creator_nn[0];
                else
                        return 0;
        }
        else
                return -1;
}

// returns type of object the creator holds.
ClassHierarchy::Class *generate_code_to_establish_connection_to_creator(FILE *a, const char *indent, int creator_nn)
{
        NonterminalData &creator=WhaleData::data.nonterminals[creator_nn];
        assert(creator.category==NonterminalData::CREATOR);
        assert(creator.rules_where_ancillary_symbol_is_used.size()>0);
        RuleData &parent_rule=WhaleData::data.rules[creator.rules_where_ancillary_symbol_is_used[0]];

        ClassHierarchy::Class *parent_type=WhaleData::data.get_class_assigned_to_alternative(parent_rule.nn, parent_rule.an);

        fprintf(a, "%sint creator_pos=find_nonterminal_in_stack(%d);\n", indent, creator_nn);
        if(WhaleData::data.variables.generate_sanity_checks)
                fprintf(a, "%sif(creator_pos<0) throw logic_error(\x22Whale: failed to find Creator in stack.\x22);\n", indent);
        fprintf(a, "%s%s *creator=(%s *)symbol_stack[creator_pos];\n", indent, creator.type->get_full_name().c_str(), creator.type->get_full_name().c_str());
        fprintf(a, "%s%s *n=creator->n;\n", indent, parent_type->get_full_name().c_str());

        return parent_type;
}

void generate_code_to_establish_connection_to_unknown_creator(FILE *a, const char *indent, int nn, int an)
{
        AlternativeData &alternative=WhaleData::data.access_alternative(nn, an);
        ClassHierarchy::Class *parent_type=WhaleData::data.get_class_assigned_to_alternative(nn, an);
//      cout << "It is " << parent_type->get_full_name() << "\n";

        fprintf(a, "%sint creator_pos=find_some_creator_in_stack(rule_start-1);\n", indent);
        if(WhaleData::data.variables.generate_sanity_checks)
                fprintf(a, "%sif(creator_pos<0) throw logic_error(\x22Whale: failed to find Creator in stack.\x22);\n", indent);
        fprintf(a, "%sCreator *creator=(Creator *)symbol_stack[creator_pos];\n", indent);
        fprintf(a, "%s%s *n=(%s *)creator->object();\n", indent, parent_type->get_full_name().c_str(), parent_type->get_full_name().c_str());
}

void make_tables()
{
#ifdef MAKE_TABLE_OF_RULE_BODIES
        TableMaker rule_bodies_maker(WhaleData::data.tables.rule_bodies, WhaleData::data.tables.rule_body_indices, 1);
        for(int i=0; i<WhaleData::data.rules.size(); i++)
        {
                RuleData &rule=WhaleData::data.rules[i];
                if(rule.body.size())
                        rule_bodies_maker.add(rule.body);
        }
#endif

        if(WhaleData::data.variables.compress_action_table)
        {
                WhaleData::data.variables.using_error_map=true;
                TableMaker action_table_maker(WhaleData::data.tables.lr_action, WhaleData::data.tables.lr_action_indices, WhaleData::data.variables.action_table_compression_mode);
                action_table_maker.set_neutral_element(0);
                ErrorMapMaker error_map_maker(WhaleData::data.tables.lr_action_error_map, WhaleData::data.tables.lr_action_error_map_indices, 0);
                for(int i=0; i<WhaleData::data.lr_automaton.states.size(); i++)
                {
                        LRAutomaton::State &state=WhaleData::data.lr_automaton.states[i];
                        action_table_maker.add(state.action_table);
                        error_map_maker.add(state.action_table);
                }
        }
        if(WhaleData::data.variables.compress_action_table)
        {
                // after ~TableMaker()
                cout << "Compressed action table: " << WhaleData::data.tables.lr_action.size() << " entries ";
                cout << "(" << WhaleData::data.lr_automaton.states.size()*WhaleData::data.terminals.size() << " uncompressed)";
                cout << " + " << WhaleData::data.tables.lr_action_error_map.size() << " error map entries.\n";
        }

        if(WhaleData::data.variables.compress_goto_table)
        {
                TableMaker goto_table_maker(WhaleData::data.tables.lr_goto, WhaleData::data.tables.lr_goto_indices, WhaleData::data.variables.goto_table_compression_mode);
                goto_table_maker.set_neutral_element(-1);
                for(int i=0; i<WhaleData::data.lr_automaton.states.size(); i++)
                {
                        LRAutomaton::State &state=WhaleData::data.lr_automaton.states[i];
                        goto_table_maker.add(state.goto_table);
                }
        }
        if(WhaleData::data.variables.compress_goto_table)
        {
                cout << "Compressed goto table: " << WhaleData::data.tables.lr_goto.size() << " entries ";
                cout << "(" << WhaleData::data.lr_automaton.states.size()*WhaleData::data.nonterminals.size() << " uncompressed).\n";
        }

        if(WhaleData::data.variables.generate_table_of_pointers_to_members)
        {
                /************* REWRITE *************/
                for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
                {
                        NonterminalData &nonterminal=WhaleData::data.nonterminals[i];

                        for(int j=0; j<nonterminal.alternatives.size(); j++)
                        {
                        //      AlternativeData &alternative=nonterminal.alternatives[j];

                                ClassHierarchy::Class *c=WhaleData::data.get_class_assigned_to_alternative(i, j);
                                for(int k=0; k<c->data_members.size(); k++)
                                        WhaleData::data.tables.all_data_members.push_back(c->data_members[k]);
                        }
                }
        }
}



void generate_tokens(FILE *a)
{
        fprintf(a, "\n");

        if(WhaleData::data.variables.tokens_namespace){
                fprintf(a, "namespace %s\n", WhaleData::data.variables.tokens_namespace);
                fprintf(a, "{\n");
        }
        fprintf(a, "enum {\n");
        // generating all terminal classes referenced from the file.
        /*set<char *, NullTerminatedStringCompare> terminal_classes;
        for(int i=0; i<WhaleData::data.actions.size(); i++)
        {
                if(typeid(*WhaleData::data.actions[i].declaration->action)==typeid(NonterminalActionReturn))
                        terminal_classes.insert(dynamic_cast<NonterminalActionReturn *>(WhaleData::data.actions[i].declaration->action)->return_value->text);
        }
        terminal_classes.insert("TerminalEOF");
        terminal_classes.insert("TerminalError");
        int counter=0;
        for(set<char *, NullTerminatedStringCompare>::iterator p=terminal_classes.begin(); p!=terminal_classes.end(); p++){
                fprintf(a, "\t%s=%u,\n", *p, counter++);
        }*/
        vector<char *> terminal_classes;
        //terminal_classes.push_back("TerminalEOF");
        //terminal_classes.push_back("TerminalError");
        for(int i=0; i<WhaleData::data.terminals.size(); i++)
        {
        char *s=(char*)WhaleData::data.terminals[i].type->name.c_str();
                terminal_classes.push_back(s);
        }
        int counter=0;
        for(vector<char *>::iterator p=terminal_classes.begin(); p!=terminal_classes.end(); p++){
                if(!WhaleData::data.variables.tokens_namespace && !WhaleData::data.variables.tokens_prefix && counter<2) counter++;
                else {
                        if(WhaleData::data.variables.tokens_prefix)fprintf(a, "\t%s%s=%u,\n",WhaleData::data.variables.tokens_prefix, *p, counter++);
                        else fprintf(a, "\t%s=%u,\n", *p, counter++);
                }
        }
        fprintf(a, "};\n // enumerution");
        if(WhaleData::data.variables.tokens_namespace)fprintf(a, "\n} // namespace %s\n", WhaleData::data.variables.whale_namespace);
}

