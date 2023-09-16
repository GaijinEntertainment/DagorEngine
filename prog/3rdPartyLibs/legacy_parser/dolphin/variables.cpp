
#include "variables.h"
#include "dolphin.h"
#include "process.h"
#include "stl.h"
using namespace std;
using namespace Whale;

void report_wrong_parameters(ostream &os, const char *variable, NonterminalOptionStatement *st, const char *additional_text=NULL);
void report_ignored_parameters(ostream &os, const char *variable, NonterminalOptionStatement *st, const char *additional_text=NULL);
bool assign_value_to_id_or_string_variable(const char *&variable, const char *variable_name);
bool assign_value_to_bool_variable(bool &variable, const char *variable_name);
bool assign_value_to_semibool_variable(int &variable, const char *variable_name, int max_value, int true_value);
bool assign_value_to_int_variable(int &variable, const char *variable_name);
bool assign_value_to_code_variable(const char *&variable, const char *variable_name);
bool process_parametrized_string(const char *&variable, const char *variable_name, const char *parameters);

Variables::Variables() :
	my_new_variable(database, "my_new_variable", TRUE_AND_FALSE_ALLOWED),
	another_variable(database, "another_variable", STRING_ALLOWED)
{
	start_conditions_enabled=true;
	compress_action_vectors=false;
	
	generate_fixed_length_lookahead_support=false;
	generate_arbitrary_lookahead_support=false;
	generate_eof_lookahead_support=false;
	
	generate_table_of_actions=false;
	using_layer2=true;
        generate_terminals=true;
        DolphinData::data.variables.alphabet_cardinality=256;
}

bool variable_exists(const char *s)
{
	static std::set<const char *, NullTerminatedStringCompare> database;
	if(!database.size())
	{
		database.insert("alphabet_cardinality");
		database.insert("unicode");
		database.insert("internal_char_type");
		database.insert("internal_string_type");

		database.insert("glob_decl_in_h");
		database.insert("glob_decl_in_cpp");
		database.insert("code_in_token_enum");

		database.insert("stdnamespace");

		database.insert("generate_tokens");
		database.insert("tokens_file");
		database.insert("tokens_namespace");
		database.insert("tokens_prefix");

		database.insert("whale");
		database.insert("whale_file");
		database.insert("whale_namespace");
		database.insert("language");
		database.insert("eat_character_upon_error");
		database.insert("dolphin_class");
		database.insert("generate_verbose_prints");
		database.insert("generate_sanity_checks");
		database.insert("store_lexeme_in_string");
		database.insert("append_data_member");
		database.insert("case_insensitive");
		database.insert("line_directives");
		database.insert("allow_inclusion_cycle_between_whale_and_dolphin");
		
		database.insert("dump_nfa_to_file");
		database.insert("dump_dfa_to_file");
		
		database.insert("compress_tables");
		database.insert("table_compression_exception_width");
		
		database.insert("input_stream_class");
		database.insert("input_character_class");
		database.insert("how_to_get_actual_character");
		database.insert("how_to_check_eof");
		database.insert("how_to_check_stream_error");
		database.insert("how_to_get_character_from_stream");
		database.insert("how_to_return_token");
		database.insert("analyzer_state_type");
		database.insert("get_token_function_return_value");
		database.insert("get_token_function_parameters");
		
		database.insert("code_in_h_before_all");
		database.insert("code_in_h");
		database.insert("code_in_h_after_all");
		database.insert("code_in_cpp_before_all");
		database.insert("code_in_cpp");
		database.insert("code_in_cpp_after_all");
		database.insert("code_in_class_before_all");
		database.insert("code_in_class");
		database.insert("code_in_class_after_all");
		database.insert("code_in_constructor");
	}
	return database.count(s);
}

bool assign_values_to_variables_stage_zero()
{
	bool flag=true;
	
	if(DolphinData::data.assignments.count("unicode"))
	{
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.unicode,
			"unicode");
	}
	else
		DolphinData::data.variables.unicode=false;

	if(DolphinData::data.assignments.count("alphabet_cardinality"))
	{
		flag=flag & assign_value_to_int_variable(DolphinData::data.variables.alphabet_cardinality,
			"alphabet_cardinality");
	}
	else
		DolphinData::data.variables.alphabet_cardinality=(DolphinData::data.variables.unicode ? 4096 /* those who want more should specify it themselves */ : 256);
	
	return flag;
}

bool assign_values_to_variables_stage_one()
{
	// We have the following sources to consider (arranged from the highest
	// priority to the lowest):
	// i. Command line options (not implemented yet)
	// ii. Assignment statements within the source file.
	// iii. Default values.
	
	bool flag=true;
	
	if(DolphinData::data.assignments.count("internal_char_type"))
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.internal_char_type,
			"internal_char_type");
	else
		DolphinData::data.variables.internal_char_type=(DolphinData::data.variables.unicode ? "wchar_t" : "char");
	
	DolphinData::data.variables.internal_char_type_is_char=(strcmp(DolphinData::data.variables.internal_char_type, "char")==0);
	DolphinData::data.variables.internal_char_type_is_wchar_t=(strcmp(DolphinData::data.variables.internal_char_type, "wchar_t")==0);

	if(DolphinData::data.assignments.count("stdnamespace"))
	{
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.std_namespace,
			"stdnamespace");
	}
	else
		DolphinData::data.variables.std_namespace="std";
	
	if(DolphinData::data.assignments.count("internal_string_type"))
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.internal_string_type,
			"internal_string_type");
	else
	{
	    char *globstr = nullptr;
		if(DolphinData::data.variables.internal_char_type_is_char){
    		globstr = new char[30+strlen(DolphinData::data.variables.std_namespace)];
			sprintf(globstr, "%s::string",DolphinData::data.variables.std_namespace);
		}else if(DolphinData::data.variables.internal_char_type_is_wchar_t){
    		globstr = new char[30+strlen(DolphinData::data.variables.std_namespace)];
			sprintf(globstr, "%s::wstring",DolphinData::data.variables.std_namespace);
		}else
		{
			globstr=new char[30+strlen(DolphinData::data.variables.std_namespace)+strlen(DolphinData::data.variables.internal_char_type)];
			sprintf(globstr, "%s::basic_string<%s>",DolphinData::data.variables.std_namespace ,DolphinData::data.variables.internal_char_type);
		}
  		DolphinData::data.variables.internal_string_type = globstr;
	}
	
	if(DolphinData::data.assignments.count("glob_decl_in_h"))
	{
		flag=flag & assign_value_to_id_or_string_variable(
		  DolphinData::data.variables.glob_decl_in_h,"glob_decl_in_h");
	}
	else
		DolphinData::data.variables.glob_decl_in_h=NULL;

	if(DolphinData::data.assignments.count("glob_decl_in_cpp"))
	{
		flag=flag & assign_value_to_id_or_string_variable(
		  DolphinData::data.variables.glob_decl_in_cpp,"glob_decl_in_cpp");
	}
	else
		DolphinData::data.variables.glob_decl_in_cpp=NULL;

	if(DolphinData::data.assignments.count("generate_tokens")){
		AssignmentData &ad=DolphinData::data.assignments["generate_tokens"];
		int n=ad.values.size();
                if(n==1){
			if(ad.values[0].second==AssignmentData::VALUE_TRUE)
			{
	        		DolphinData::data.variables.generate_terminals=true;
	                } else DolphinData::data.variables.generate_terminals=false;
	        } else DolphinData::data.variables.generate_terminals=false;
	}

	if(DolphinData::data.assignments.count("tokens_file"))
	{
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.generate_terminals_file,
			"tokens_file");
	}
	else
		DolphinData::data.variables.generate_terminals_file=NULL;
	
	if(DolphinData::data.assignments.count("tokens_namespace"))
	{
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.terminals_namespace,
			"tokens_namespace");
	}
	else
		DolphinData::data.variables.terminals_namespace=NULL;

	
	if(DolphinData::data.assignments.count("tokens_prefix"))
	{
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.tokens_prefix,
			"tokens_prefix");
	}
	else
		DolphinData::data.variables.tokens_prefix=NULL;

        DolphinData::data.variables.using_whale=true;
        DolphinData::data.variables.whale_emulation_mode=false;
	/*if(DolphinData::data.assignments.count("whale"))
	{
		AssignmentData &ad=DolphinData::data.assignments["whale"];
		int n=ad.values.size();
		
		if(n==1 && ad.values[0].second==AssignmentData::VALUE_TRUE)
		{
			DolphinData::data.variables.using_whale=true;
			DolphinData::data.variables.whale_emulation_mode=false;
		}
		else if(n==1 && ad.values[0].second==AssignmentData::VALUE_FALSE)
		{
			DolphinData::data.variables.using_whale=false;
			DolphinData::data.variables.whale_emulation_mode=false;
		}
		else if(n==1 && !strcmp(ad.values[0].first, "emulate"))
		{
			DolphinData::data.variables.using_whale=true;
			DolphinData::data.variables.whale_emulation_mode=true;
		}
		else
		{
			report_wrong_parameters(cout, "whale", ad.declaration,
				"expecting 'true', 'false' or 'emulate'");
			flag=false;
		}
	}
	else
	{
		DolphinData::data.variables.using_whale=true;
		DolphinData::data.variables.whale_emulation_mode=false;
	}
	*/
	if(DolphinData::data.assignments.count("whale_file"))
	{
		if(DolphinData::data.variables.using_whale)
			flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.whale_file,
				"whale_file");
		else
			report_ignored_parameters(cout, "whale_file",
				DolphinData::data.assignments["whale_file"].declaration);
	}
	else
		DolphinData::data.variables.whale_file="parser.h";
	
	if(DolphinData::data.assignments.count("whale_namespace"))
	{
		if(DolphinData::data.variables.using_whale)
			flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.whale_namespace,
				"whale_namespace");
		else
			report_ignored_parameters(cout, "whale_namespace",
				DolphinData::data.assignments["whale_namespace"].declaration);
	}
	else
		DolphinData::data.variables.whale_namespace="Whale";
	
	if(DolphinData::data.assignments.count("language"))
	{
		AssignmentData &ad=DolphinData::data.assignments["language"];
		int n=ad.values.size();
		
		if(n==1 && !strcmp(ad.values[0].first, "cpp"))
			DolphinData::data.variables.output_language=Variables::LANGUAGE_CPP;
		else if(n==1 && !strcmp(ad.values[0].first, "vintage_cpp"))
			DolphinData::data.variables.output_language=Variables::LANGUAGE_VINTAGE_CPP;
		else
		{
			report_wrong_parameters(cout, "language", ad.declaration,
				"expecting 'cpp' or 'vintage_cpp'");
			flag=false;
		}
	}
	else
		DolphinData::data.variables.output_language=Variables::LANGUAGE_CPP;
	
	if(DolphinData::data.assignments.count("dolphin_class"))
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.dolphin_class_name,
			"dolphin_class");
	else
		DolphinData::data.variables.dolphin_class_name="DolphinLexicalAnalyzer";
	
	if(DolphinData::data.assignments.count("get_token_function_return_value"))
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.get_token_function_return_value,
			"get_token_function_return_value");
	else
	{
		if(DolphinData::data.variables.using_whale)
		{
			string str=string(DolphinData::data.variables.whale_namespace)+string("::Terminal *");
			DolphinData::data.variables.get_token_function_return_value=strdup(str.c_str());
		}
		else
			DolphinData::data.variables.get_token_function_return_value="int";
	}
	
	if(DolphinData::data.assignments.count("get_token_function_parameters"))
	{
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.get_token_function_parameters,
			"get_token_function_parameters");
		if(!strcmp(DolphinData::data.variables.get_token_function_parameters, "void"))
			DolphinData::data.variables.get_token_function_parameters="";
	}
	else
		DolphinData::data.variables.get_token_function_parameters="";
	
	if(DolphinData::data.assignments.count("generate_verbose_prints"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.generate_verbose_prints,
			"generate_verbose_prints");
	else
		DolphinData::data.variables.generate_verbose_prints=false;
	
	if(DolphinData::data.assignments.count("generate_sanity_checks"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.generate_sanity_checks,
			"generate_sanity_checks");
	else
		DolphinData::data.variables.generate_sanity_checks=false;
	
	if(DolphinData::data.assignments.count("eat_character_upon_error"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.eat_one_character_upon_lexical_error,
			"eat_character_upon_error");
	else
		DolphinData::data.variables.eat_one_character_upon_lexical_error=true;
	
	if(DolphinData::data.assignments.count("append_data_member"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.append_data_member,
			"append_data_member");
	else
		DolphinData::data.variables.append_data_member=false;
	
	if(DolphinData::data.assignments.count("case_insensitive"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.case_insensitive,
			"case_insensitive");
	else
		DolphinData::data.variables.case_insensitive=false;
	
	if(DolphinData::data.assignments.count("compress_tables"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.compress_tables,
			"compress_tables");
	else
		DolphinData::data.variables.compress_tables=true;
	
	if(DolphinData::data.assignments.count("line_directives"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.line_directives,
			"line_directives");
	else
		DolphinData::data.variables.line_directives=false;
	
	if(DolphinData::data.assignments.count("allow_inclusion_cycle_between_whale_and_dolphin"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.allow_inclusion_cycle_between_whale_and_dolphin,
			"allow_inclusion_cycle_between_whale_and_dolphin");
	else
		DolphinData::data.variables.allow_inclusion_cycle_between_whale_and_dolphin=true;
	
	if(DolphinData::data.assignments.count("dump_nfa_to_file"))
		flag=flag & assign_value_to_semibool_variable(DolphinData::data.variables.dump_nfa_to_file,
			"dump_nfa_to_file", 2, 2);
	else
		DolphinData::data.variables.dump_nfa_to_file=0;
	
	if(DolphinData::data.assignments.count("dump_dfa_to_file"))
		flag=flag & assign_value_to_semibool_variable(DolphinData::data.variables.dump_dfa_to_file,
			"dump_dfa_to_file", 2, 2);
	else
		DolphinData::data.variables.dump_dfa_to_file=0;
	
	if(DolphinData::data.assignments.count("table_compression_exception_width"))
	{
		flag=flag & assign_value_to_int_variable(DolphinData::data.variables.table_compression_exception_width,
			"table_compression_exception_width");
	}
	else
		DolphinData::data.variables.table_compression_exception_width=1;
	
	if(DolphinData::data.assignments.count("store_lexeme_in_string"))
		flag=flag & assign_value_to_bool_variable(DolphinData::data.variables.store_lexeme_in_string,
			"store_lexeme_in_string");
	else
	{
		if(DolphinData::data.variables.append_data_member)
			DolphinData::data.variables.store_lexeme_in_string=true;
		else
			DolphinData::data.variables.store_lexeme_in_string=false;
	}
	
	if(DolphinData::data.variables.append_data_member && !DolphinData::data.variables.store_lexeme_in_string)
	{
		cout << "Warning: append_data_member=true together with "
			"store_lexeme_in_string=false can potentially lead "
			"to quadratic time complexity.\n";
	//	cout << "The current version of Dolphin requires "
	//		"store_lexeme_in_string to be true if "
	//		"append_data_member=true.\n";
	//	flag=false;
	}
	
	if(DolphinData::data.assignments.count("input_stream_class"))
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.input_stream_class,
			"input_stream_class");
	else
	{
		if(DolphinData::data.variables.internal_char_type_is_char)
			DolphinData::data.variables.input_stream_class="std::istream";
		else if(DolphinData::data.variables.internal_char_type_is_wchar_t)
			DolphinData::data.variables.input_stream_class="std::wistream";
		else
		{
			char *str = new char[30+strlen(DolphinData::data.variables.internal_char_type)];
			sprintf(str, "std::basic_ifstream<%s>", DolphinData::data.variables.internal_char_type);
			DolphinData::data.variables.input_stream_class=str;
		}
	}
	
	DolphinData::data.variables.input_stream_is_FILE_asterisk=(!strcmp(DolphinData::data.variables.input_stream_class, "FILE *") ||
		!strcmp(DolphinData::data.variables.input_stream_class, "FILE*"));
	
	if(DolphinData::data.assignments.count("input_character_class"))
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.input_character_class,
			"input_character_class");
	else
		DolphinData::data.variables.input_character_class=DolphinData::data.variables.internal_char_type;

	string term_namespace_qual="";
	if(DolphinData::data.variables.terminals_namespace){
        	term_namespace_qual=DolphinData::data.variables.terminals_namespace;
        	term_namespace_qual+="::";
	}
	if(DolphinData::data.variables.tokens_prefix){
        	term_namespace_qual+=DolphinData::data.variables.tokens_prefix;
	}
	
	if(DolphinData::data.assignments.count("how_to_return_token"))
		flag=flag & process_parametrized_string(DolphinData::data.variables.how_to_return_token,
			"how_to_return_token", "t");
	else
	{
		if(DolphinData::data.variables.using_whale)
		{
			string str=string("return (")+term_namespace_qual+string("$t);");
			DolphinData::data.variables.how_to_return_token=strdup(str.c_str());
		}
		else
			DolphinData::data.variables.how_to_return_token="return $t;";
	}
	
	if(DolphinData::data.assignments.count("how_to_get_actual_character"))
		flag=flag & process_parametrized_string(DolphinData::data.variables.how_to_get_actual_character,
			"how_to_get_actual_character", "c");
	else
		DolphinData::data.variables.how_to_get_actual_character="$c";
	
	if(DolphinData::data.assignments.count("how_to_check_eof"))
		flag=flag & process_parametrized_string(DolphinData::data.variables.how_to_check_eof,
			"how_to_check_eof", "s");
	else if(DolphinData::data.variables.input_stream_is_FILE_asterisk)
		DolphinData::data.variables.how_to_check_eof="feof($s)";
	else
		DolphinData::data.variables.how_to_check_eof="$s.eof()";
	
	if(DolphinData::data.assignments.count("how_to_check_stream_error"))
		flag=flag & process_parametrized_string(DolphinData::data.variables.how_to_check_stream_error,
			"how_to_check_stream_error", "s");
	else if(DolphinData::data.variables.input_stream_is_FILE_asterisk)
		DolphinData::data.variables.how_to_check_stream_error="$s==NULL";
	else
		DolphinData::data.variables.how_to_check_stream_error="$s.bad()";
	
	if(DolphinData::data.assignments.count("how_to_get_character_from_stream"))
		flag=flag & process_parametrized_string(DolphinData::data.variables.how_to_get_character_from_stream,
			"how_to_get_character_from_stream", "cs");
	else if(DolphinData::data.variables.input_stream_is_FILE_asterisk)
		DolphinData::data.variables.how_to_get_character_from_stream="$c=fgetc($s)";
	else
		DolphinData::data.variables.how_to_get_character_from_stream="$s.get($c)";
	
        //DolphinData::data.variables.how_to_check_eof="$s.length()==get_cur_offset()";
        //DolphinData::data.variables.how_to_get_character_from_stream="$c=$s[get_cur_offset()]";
	if(DolphinData::data.assignments.count("analyzer_state_type"))
		flag=flag & assign_value_to_id_or_string_variable(DolphinData::data.variables.analyzer_state_type,
			"analyzer_state_type");

	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_token_enum,
		"code_in_token_enum");
	
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_class_before_all,
		"code_in_class_before_all");
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_class,
		"code_in_class");
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_class_after_all,
		"code_in_class_after_all");
	
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_constructor,
		"code_in_constructor");
	
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_h_before_all,
		"code_in_h_before_all");
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_h,
		"code_in_h");
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_h_after_all,
		"code_in_h_after_all");
	
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_cpp_before_all,
		"code_in_cpp_before_all");
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_cpp,
		"code_in_cpp");
	flag=flag & assign_value_to_code_variable(DolphinData::data.variables.code_in_cpp_after_all,
		"code_in_cpp_after_all");
	
	return flag;
}

// executed just before code generation.
bool assign_values_to_variables_stage_two()
{
	bool flag=true;
	
	if(!DolphinData::data.assignments.count("analyzer_state_type"))
	{
		int n=DolphinData::data.dfa_partition.groups.size();
		
		if(n>60000)
			DolphinData::data.variables.analyzer_state_type="int";
		else if(n>30000)
			DolphinData::data.variables.analyzer_state_type="unsigned short";
		else if(n>250)
			DolphinData::data.variables.analyzer_state_type="short";
		else
			DolphinData::data.variables.analyzer_state_type="unsigned char";
	}
	
	DolphinData::data.variables.access_transitions_through_a_method=(DolphinData::data.variables.compress_tables && DolphinData::data.variables.using_layer2);
	
	DolphinData::data.variables.my_new_variable=false;
	DolphinData::data.variables.another_variable=NULL;
	
	return flag;
}

void report_wrong_parameters(ostream &os, const char *variable, NonterminalOptionStatement *st, const char *additional_text)
{
	os << "Wrong value assigned to variable '" << variable << "'";
	if(st)
	{
		cout << " at ";
		print_terminal_location(os, st->left);
	}
	else
		cout << " in command line";
	
	if(additional_text)
		os << ", " << additional_text;
	os << ".\n";
}

void report_ignored_parameters(ostream &os, const char *variable, NonterminalOptionStatement *st, const char *additional_text)
{
	os << "Warning: assignment to '" << variable << "' ignored";
	if(st)
	{
		cout << " at ";
		print_terminal_location(os, st->left);
	}
	else
		cout << " in command line";
	
	if(additional_text)
		os << ", " << additional_text;
	os << ".\n";
}

bool assign_value_to_id_or_string_variable(const char *&variable, const char *variable_name)
{
	if(!DolphinData::data.assignments.count(variable_name))
	{
		variable=NULL;
		return true;
	}
	AssignmentData &ad=DolphinData::data.assignments[variable_name];
	int n=ad.values.size();
	
	if(n==1 && (ad.values[0].second==AssignmentData::VALUE_ID || ad.values[0].second==AssignmentData::VALUE_STRING))
	{
		variable=ad.values[0].first;
		return true;
	}
	else
	{
		report_wrong_parameters(cout, variable_name, ad.declaration,
			"expecting either an identifier or a string");
		return false;
	}
}

// returns true if no error is found, false otherwise.
// *** The return value has nothing to do with the value of the variable! ***
bool assign_value_to_bool_variable(bool &variable, const char *variable_name)
{
	if(!DolphinData::data.assignments.count(variable_name))
		return true;
	AssignmentData &ad=DolphinData::data.assignments[variable_name];
	int n=ad.values.size();
	
	if(n==1 && ad.values[0].second==AssignmentData::VALUE_TRUE)
	{
		variable=true;
		return true;
	}
	else if(n==1 && ad.values[0].second==AssignmentData::VALUE_FALSE)
	{
		variable=false;
		return true;
	}
	else
	{
		report_wrong_parameters(cout, variable_name, ad.declaration,
			"expecting either 'true' or 'false'");
		return false;
	}
}

bool assign_value_to_semibool_variable(int &variable, const char *variable_name,
	int max_value, int true_value)
{
	if(!DolphinData::data.assignments.count(variable_name))
		return true;
	AssignmentData &ad=DolphinData::data.assignments[variable_name];
	int n=ad.values.size();

	if(n==1 && ad.values[0].second==AssignmentData::VALUE_TRUE)
	{
		variable=true_value;
		return true;
	}
	else if(n==1 && ad.values[0].second==AssignmentData::VALUE_FALSE)
	{
		variable=0;
		return true;
	}
	else if(n==1 && ad.values[0].second==AssignmentData::VALUE_NUMBER &&
		atoi(ad.values[0].first)>=0 && atoi(ad.values[0].first)<=max_value)
	{
		variable=atoi(ad.values[0].first);
		return true;
	}
	else
	{
		char buf[100];
		sprintf(buf, "expecting 'true', 'false' or a number between 0 "
			"and %d", max_value);
		report_wrong_parameters(cout, variable_name, ad.declaration,
			buf);
		return false;
	}
}

bool assign_value_to_int_variable(int &variable, const char *variable_name)
{
	if(!DolphinData::data.assignments.count(variable_name))
		return true;
	AssignmentData &ad=DolphinData::data.assignments[variable_name];
	int n=ad.values.size();
	
	if(n==1 && ad.values[0].second==AssignmentData::VALUE_NUMBER)
	{
		variable=atoi(ad.values[0].first);
		return true;
	}
	else if(n==1 && ad.values[0].second==AssignmentData::VALUE_HEX_NUMBER)
	{
		sscanf(ad.values[0].first, "%x", &variable);
		return true;
	}
	else
	{
		report_wrong_parameters(cout, variable_name, ad.declaration,
			"expecting a number");
		return false;
	}
}

bool assign_value_to_code_variable(const char *&variable, const char *variable_name)
{
	if(!DolphinData::data.assignments.count(variable_name))
	{
		variable=NULL;
		return true;
	}
	AssignmentData &ad=DolphinData::data.assignments[variable_name];
	int n=ad.values.size();
	
	if(n==1 && (ad.values[0].second==AssignmentData::VALUE_STRING || ad.values[0].second==AssignmentData::VALUE_CODE ))
	{
		variable=ad.values[0].first;
		return true;
	}
	else
	{
		report_wrong_parameters(cout, variable_name, ad.declaration,
			"expecting C++ code");
		return false;
	}
}

struct CharWithDollar
{
	char c;
	CharWithDollar(char c1) { c=c1; }
	operator char() const { return c; }
};

ostream &operator <<(ostream &os, CharWithDollar cwd)
{
	return os << '$' << char(cwd);
}

bool process_parametrized_string(const char *&variable, const char *variable_name, const char *parameters)
{
	if(!DolphinData::data.assignments.count(variable_name))
		return true;
	AssignmentData &ad=DolphinData::data.assignments[variable_name];
	int n=ad.values.size();

	if(n==1 && ad.values[0].second==AssignmentData::VALUE_STRING)
	{
		int np=strlen(parameters);
		const char *s=ad.values[0].first;
		
		set<CharWithDollar> unused_parameters, invalid_parameters;
		for(int j=0; j<np; j++)
			unused_parameters.insert(CharWithDollar(parameters[j]));
		
		bool after_dollar=false;
		for(int i=0; s[i]; i++)
		{
			char c=s[i];
			if(after_dollar)
			{
				if(c!='$' && c>' ' && c<128)
				{
					bool local_flag=false;
					for(int j=0; j<np; j++)
						if(parameters[j]==c)
						{
							unused_parameters.erase(CharWithDollar(c));
							local_flag=true;
							break;
						}
					if(!local_flag)
						invalid_parameters.insert(CharWithDollar(c));
				}
				after_dollar=false;
			}
			else if(c=='$')
				after_dollar=true;
		}
		
		assert(ad.declaration);
		if(unused_parameters.size())
		{
			cout << "Warning: unused parameter" << (unused_parameters.size()==1 ? " " : "s ");
			print_with_and(cout, unused_parameters.begin(), unused_parameters.end());
			cout << " in string at ";
			print_terminal_location(cout, ad.declaration->right[0]);
			cout << ".\n";
		}
		if(invalid_parameters.size())
		{
			cout << "Invalid parameter" << (invalid_parameters.size()==1 ? " " : "s ");
			print_with_and(cout, invalid_parameters.begin(), invalid_parameters.end());
			cout << " in string at ";
			print_terminal_location(cout, ad.declaration->right[0]);
			cout << ".\n";
			return false;
		}
		
		variable=s;
	}
	else
	{
		report_wrong_parameters(cout, variable_name, ad.declaration,
			"expecting a string");
		return false;
	}
	
	return true;
}

bool Variables::check_database_integrity()
{
	bool flag=true;
	for(std::map<const char *, variable_base *, NullTerminatedStringCompare>::iterator p=database.begin(); p!=database.end(); p++)
		if(!p->second->has_value_assigned_to_it)
		{
			cout << "Due to internal error, no value was assigned to variable " << p->first << ".\n";
			flag=false;
		}
	return flag;
}
