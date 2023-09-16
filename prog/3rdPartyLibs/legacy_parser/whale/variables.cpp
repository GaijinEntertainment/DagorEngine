
#include "variables.h"
#include "whale.h"
#include "process.h"
#include "utilities.h"
#include "nstl/stl_format.h"
using namespace std;
using namespace Whale;

void report_wrong_parameters(ostream &os, const char *variable, NonterminalOptionStatement *st, const char *additional_text=NULL);
void report_ignored_parameters(ostream &os, const char *variable, NonterminalOptionStatement *st, const char *additional_text=NULL);
bool assign_value_to_id_or_string_variable(const char *&variable, const char *variable_name);
bool assign_value_to_bool_variable(bool &variable, const char *variable_name, bool default_value);
bool assign_value_to_code_variable(const char *&variable, const char *variable_name);
bool process_parametrized_string(const char *&variable, const char *variable_name, const char *parameters);

Variables::Variables()
{
	whale_namespace=NULL;
	whale_class=NULL;
	lexical_analyzer_file=NULL;
	lexical_analyzer_class=NULL;
	terminal_file=NULL;

	code_in_parser_class=NULL;
	code_in_constructor=NULL;
	code_in_h_before_all=NULL;
	code_in_h=NULL;
	code_in_h_after_all=NULL;
	code_in_cpp_before_all=NULL;
	code_in_cpp=NULL;
	code_in_cpp_after_all=NULL;
	error_handler=NULL;
	generate_tokens=false;
	tokens_file=NULL;
	tokens_namespace=NULL;
	tokens_prefix=NULL;

	// these values are not yet accessible from outside.
	push_null_pointers_to_stack=false;
	rearrange_symbols_between_nested_bodies=true;
	generate_table_of_pointers_to_members=false;
	assumed_number_of_bits_in_int=sizeof(int)*8;
	using_error_map=false; // if needed, it will be set to true elsewhere.
	action_table_compression_mode=3;
	goto_table_compression_mode=2;
	connect_up_operators_are_used=false;
	derive_creators_from_the_abstract_creator=false;

        error_garbage=true;
        garbage=true;
}

Variables::Properties variable_properties(const char *s)
{
	static std::map<const char *, Variables::Properties, NullTerminatedStringCompare> database;
	if(!database.size())
	{
		database["garbage"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["error_garbage"]=Variables::TRUE_AND_FALSE_ALLOWED;

		database["whale_namespace"]=Variables::ID_OR_STRING_ALLOWED;
		database["whale_class"]=Variables::ID_OR_STRING_ALLOWED;
		database["terminal_file"]=Variables::ID_OR_STRING_ALLOWED;

		database["stdnamespace"]=Variables::ID_OR_STRING_ALLOWED;

		database["tokens_prefix"]=Variables::ID_OR_STRING_ALLOWED;
		database["tokens_file"]=Variables::ID_OR_STRING_ALLOWED;
		database["tokens_namespace"]=Variables::ID_OR_STRING_ALLOWED;
		database["generate_tokens"]=Variables::ID_OR_STRING_ALLOWED;

		database["generate_sten"]=Variables::ID_OR_STRING_ALLOWED;

		database["lexical_analyzer_file"]=Variables::ID_OR_STRING_ALLOWED;
		database["lexical_analyzer_class"]=Variables::ID_OR_STRING_ALLOWED;
		database["method"]=Variables::ID_ALLOWED;
		
		database["generate_verbose_prints"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["generate_sanity_checks"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["push_null_pointers_to_stack"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["default_member_name_is_nothing"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["make_up_connection"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["reuse_iterators"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["input_queue"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["line_directives"]=Variables::TRUE_AND_FALSE_ALLOWED;
		
		database["compress_action_table"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["compress_goto_table"]=Variables::TRUE_AND_FALSE_ALLOWED;
		
		database["dump_grammar_to_file"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["dump_first_to_file"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["dump_lr_automaton_to_file"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["dump_conflicts_to_file"]=Variables::TRUE_AND_FALSE_ALLOWED;
		database["dump_precedence_to_file"]=Variables::TRUE_AND_FALSE_ALLOWED;
		
		database["code_in_class"]=(Variables::Properties)(Variables::CODE_ALLOWED |
			Variables::PARAMETER_ALLOWED | Variables::PARAMETER_REQUIRED |
			Variables::MULTIPLE_ASSIGNMENTS_ALLOWED |
			Variables::SINGLE_ASSIGNMENT_FOR_SINGLE_PARAMETER_LIST);
		
		database["code_in_h_before_all"]=Variables::CODE_ALLOWED;
		database["code_in_h"]=Variables::CODE_ALLOWED;
		database["code_in_h_after_all"]=Variables::CODE_ALLOWED;
		database["code_in_cpp_before_all"]=Variables::CODE_ALLOWED;
		database["code_in_cpp"]=Variables::CODE_ALLOWED;
		database["code_in_cpp_after_all"]=Variables::CODE_ALLOWED;
		database["code_in_constructor"]=Variables::CODE_ALLOWED;
		database["error_handler"]=Variables::CODE_ALLOWED;
	}
	if(database.count(s))
		return database[s];
	else
		return Variables::DOES_NOT_EXIST;
}

bool assign_values_to_variables_stage_zero()
{
	bool flag=true;
	
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.default_member_name_is_nothing,
		"default_member_name_is_nothing", false);
	
	if(WhaleData::data.assignments.count("make_up_connection"))
	{
		if(assign_value_to_bool_variable(WhaleData::data.variables.make_up_connection, "make_up_connection", false))
		{
			if(WhaleData::data.variables.make_up_connection==false && WhaleData::data.variables.connect_up_operators_are_used)
			{
				cout << "Cannot set make_up_connection=false, because connect_up operators are used.\n";
				flag=false;
			//	WhaleData::data.variables.make_up_connection=true;
			}
		}
		else
			flag=false;
	}
	else
		WhaleData::data.variables.make_up_connection=WhaleData::data.variables.connect_up_operators_are_used;
	
	// change this.
	WhaleData::data.variables.individual_up_data_members_in_classes=false;
	WhaleData::data.variables.make_creator_lookup_facility=WhaleData::data.variables.make_up_connection;
	
	// WhaleData::data.variables.derive_creators_from_the_abstract_creator=WhaleData::data.variables.make_up_connection;
	WhaleData::data.variables.derive_creators_from_the_abstract_creator=true;
	
	return flag;
}

bool assign_values_to_variables_stage_one()
{
	bool flag=true;

	if(WhaleData::data.assignments.count("lexical_analyzer_file"))
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.lexical_analyzer_file,
			"lexical_analyzer_file");
	else
		WhaleData::data.variables.lexical_analyzer_file="\x22lexical.h\x22";

	if(WhaleData::data.assignments.count("generate_tokens")){
		AssignmentData &ad=WhaleData::data.assignments["generate_tokens"][0];
		int n=ad.values.size();
                if(n==1){
			if(ad.values[0].second==AssignmentData::VALUE_TRUE)
			{
	        		WhaleData::data.variables.generate_tokens=true;
	                } else WhaleData::data.variables.generate_tokens=false;
	        } else WhaleData::data.variables.generate_tokens=false;
	}

	if(WhaleData::data.assignments.count("stdnamespace"))
	{
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.std_namespace,
			"stdnamespace");
	}
	else
		WhaleData::data.variables.std_namespace="std";
	

	if(WhaleData::data.assignments.count("tokens_file"))
	{
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.tokens_file,
			"tokens_file");
	}
	else
		WhaleData::data.variables.tokens_file=NULL;
	
	if(WhaleData::data.assignments.count("tokens_namespace"))
	{
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.tokens_namespace,
			"tokens_namespace");
	}
	else
		WhaleData::data.variables.tokens_namespace=NULL;

	if(WhaleData::data.assignments.count("tokens_prefix"))
	{
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.tokens_prefix,
			"tokens_prefix");
	}
	else
		WhaleData::data.variables.tokens_prefix=NULL;


	if(WhaleData::data.assignments.count("terminal_file"))
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.terminal_file,
			"terminal_file");
	else
		WhaleData::data.variables.terminal_file=NULL;
	


	if(WhaleData::data.assignments.count("lexical_analyzer_class"))
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.lexical_analyzer_class,
			"lexical_analyzer_class");
	else
		WhaleData::data.variables.lexical_analyzer_class="DolphinLexicalAnalyzer";
	
	if(WhaleData::data.assignments.count("whale_namespace"))
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.whale_namespace,
			"whale_namespace");
	else
		WhaleData::data.variables.whale_namespace="Whale";
	
	if(WhaleData::data.assignments.count("whale_class"))
		flag=flag & assign_value_to_id_or_string_variable(WhaleData::data.variables.whale_class,
			"whale_class");
	else
		WhaleData::data.variables.whale_class="WhaleParser";
	
	if(WhaleData::data.assignments.count("method"))
	{
		AssignmentData &ad=WhaleData::data.assignments["method"][0];
		assert(ad.values.size()==1);
		
		if(!strcmp(ad.values[0].first, "LR1"))
			WhaleData::data.variables.method=Variables::LR1;
		else if(!strcmp(ad.values[0].first, "SLR1"))
			WhaleData::data.variables.method=Variables::SLR1;
		else if(!strcmp(ad.values[0].first, "LALR1"))
			WhaleData::data.variables.method=Variables::LALR1;
		else
		{
			report_wrong_parameters(cout, "method", ad.declaration,
				"expecting 'LR1', 'SLR1' or 'LALR1'");
			flag=false;
		}
	}
	else
		WhaleData::data.variables.method=Variables::LR1;
	
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.gen_sten,
		"generate_sten", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.generate_verbose_prints,
		"generate_verbose_prints", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.generate_sanity_checks,
		"generate_sanity_checks", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.push_null_pointers_to_stack,
		"push_null_pointers_to_stack", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.reuse_iterators,
		"reuse_iterators", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.input_queue,
		"input_queue", false);

	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.garbage,
		"garbage", true);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.error_garbage,
		"error_garbage", true);
	
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.compress_action_table,
		"compress_action_table", true);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.compress_goto_table,
		"compress_goto_table", true);
	
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.dump_grammar_to_file,
		"dump_grammar_to_file", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.dump_first_to_file,
		"dump_first_to_file", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.dump_lr_automaton_to_file,
		"dump_lr_automaton_to_file", false);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.dump_conflicts_to_file,
		"dump_conflicts_to_file", true);
	flag=flag & assign_value_to_bool_variable(WhaleData::data.variables.dump_precedence_to_file,
		"dump_precedence_to_file", false);
	
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.code_in_constructor,
		"code_in_constructor");
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.error_handler,
		"error_handler");
	
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.code_in_h_before_all,
		"code_in_h_before_all");
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.code_in_h,
		"code_in_h");
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.code_in_h_after_all,
		"code_in_h_after_all");
	
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.code_in_cpp_before_all,
		"code_in_cpp_before_all");
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.code_in_cpp,
		"code_in_cpp");
	flag=flag & assign_value_to_code_variable(WhaleData::data.variables.code_in_cpp_after_all,
		"code_in_cpp_after_all");
	
	return flag;
}

bool assign_values_to_variables_stage_two()
{
	if(WhaleData::data.assignments.count("code_in_class"))
	{
		vector<AssignmentData> &v_ad=WhaleData::data.assignments["code_in_class"];
		for(int i=0; i<v_ad.size(); i++)
		{
			AssignmentData &ad=v_ad[i];
			assert(ad.values.size()==1);
			assert(ad.parameters.size()<=1);
			
			if(ad.parameters.size()==1)
			{
				assert(ad.declaration->middle.size()==1);
				
				ClassHierarchy::Class *target_class=classes.find(ad.parameters[0].first);
				if(target_class==NULL)
				{
					cout << "Unknown class " << ad.parameters[0].first << " at ";
					ad.declaration->middle[0]->print_location(cout);
					cout << ".\n";
					continue;
				}
				else if(!target_class->is_internal)
				{
					cout << "Class " << target_class->name << ", ";
					target_class->print_where_it_was_declared(cout);
					cout << " is an external class and, therefore, assignment at ";
				//	cout << ad.parameters[0].first << " at ";
					ad.declaration->middle[0]->print_location(cout);
					cout << " is invalid.\n";
					continue;
				}
				assert(target_class->code==NULL);
				target_class->code=ad.values[0].first;
			}
			else
				WhaleData::data.variables.code_in_parser_class=ad.values[0].first;
		}
	}
	
	return true;
}

void report_wrong_parameters(ostream &os, const char *variable, NonterminalOptionStatement *st, const char *additional_text)
{
	os << "Wrong value assigned to variable '" << variable << "'";
	if(st)
	{
		cout << " at ";
		st->left->print_location(os);
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
		st->left->print_location(os);
	}
	else
		cout << " in command line";
	
	if(additional_text)
		os << ", " << additional_text;
	os << ".\n";
}

bool assign_value_to_id_or_string_variable(const char *&variable, const char *variable_name)
{
	if(!WhaleData::data.assignments.count(variable_name))
	{
		variable=NULL;
		return true;
	}
	assert(WhaleData::data.assignments[variable_name].size()==1);
	AssignmentData &ad=WhaleData::data.assignments[variable_name][0];
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
bool assign_value_to_bool_variable(bool &variable, const char *variable_name, bool default_value)
{
	if(!WhaleData::data.assignments.count(variable_name))
	{
		variable=default_value;
		return true;
	}
	assert(WhaleData::data.assignments[variable_name].size()==1);
	AssignmentData &ad=WhaleData::data.assignments[variable_name][0];
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

bool assign_value_to_code_variable(const char *&variable, const char *variable_name)
{
	if(!WhaleData::data.assignments.count(variable_name))
	{
		variable=NULL;
		return true;
	}
	assert(WhaleData::data.assignments[variable_name].size()==1);
	AssignmentData &ad=WhaleData::data.assignments[variable_name][0];
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
	if(!WhaleData::data.assignments.count(variable_name))
		return true;
	assert(WhaleData::data.assignments[variable_name].size()==1);
	AssignmentData &ad=WhaleData::data.assignments[variable_name][0];
	int n=ad.values.size();

	if(n==1 && ad.values[0].second==AssignmentData::VALUE_STRING)
	{
		int np=strlen(parameters);
		char *s=ad.values[0].first;
		
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
			ad.declaration->right[0]->print_location(cout);
			cout << ".\n";
		}
		if(invalid_parameters.size())
		{
			cout << "Invalid parameter" << (invalid_parameters.size()==1 ? " " : "s ");
			print_with_and(cout, invalid_parameters.begin(), invalid_parameters.end());
			cout << " in string at ";
			ad.declaration->right[0]->print_location(cout);
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
