
#ifndef __WHALE__VARIABLES_H

#define __WHALE__VARIABLES_H

struct Variables
{
	enum Properties
	{
		DOES_NOT_EXIST=0,
		
		TRUE_AND_FALSE_ALLOWED=1,
		ID_ALLOWED=2,
		NUMBER_ALLOWED=4,
		STRING_ALLOWED=8,
		CODE_ALLOWED=16,
		MULTIPLE_VALUES_ALLOWED=32,
		PARAMETER_ALLOWED=64,
		PARAMETER_REQUIRED=128,
		MULTIPLE_PARAMETERS_ALLOWED=256,
		MULTIPLE_ASSIGNMENTS_ALLOWED=512,
		SINGLE_ASSIGNMENT_FOR_SINGLE_PARAMETER=1024,
		SINGLE_ASSIGNMENT_FOR_SINGLE_PARAMETER_LIST=2048,
		PROCESS_ESCAPE_SEQUENCES_IN_STRING=4096,
		
		ID_OR_STRING_ALLOWED=2+8,
		STRING_OR_CODE_ALLOWED=8+16+4096
	};
	enum Method { LR1, SLR1, LALR1 };
	const char *std_namespace;
	const char *whale_namespace;
	const char *whale_class;
	const char *terminal_file;
	const char *lexical_analyzer_file;
	const char *lexical_analyzer_class;
	const char *tokens_file;
	const char *tokens_namespace;
	const char *tokens_prefix;
	
	Method method;
	bool generate_tokens;
	
	bool generate_verbose_prints;
	bool generate_sanity_checks;
	bool push_null_pointers_to_stack;
	bool rearrange_symbols_between_nested_bodies;
	bool generate_table_of_pointers_to_members;
	int assumed_number_of_bits_in_int;
	bool using_error_map;
	bool default_member_name_is_nothing;
	bool make_up_connection;
	bool individual_up_data_members_in_classes;
	bool reuse_iterators;
	bool input_queue;
	bool line_directives;
	
	bool compress_action_table;
	int action_table_compression_mode;
	bool compress_goto_table;
	int goto_table_compression_mode;
	
	bool dump_grammar_to_file;
	bool dump_first_to_file;
	bool dump_lr_automaton_to_file;
	bool dump_conflicts_to_file;
	bool dump_precedence_to_file;
	
	const char *code_in_parser_class;
	const char *code_in_constructor;
	const char *code_in_h_before_all;
	const char *code_in_h;
	const char *code_in_h_after_all;
	const char *code_in_cpp_before_all;
	const char *code_in_cpp;
	const char *code_in_cpp_after_all;
	const char *error_handler;
	bool gen_sten;
	
	bool error_garbage,garbage;
	// not accessible
	bool connect_up_operators_are_used;
	bool make_creator_lookup_facility;
	bool derive_creators_from_the_abstract_creator;

	
	Variables();
};

Variables::Properties variable_properties(const char *s);
inline bool variable_exists(const char *s)
{
	return variable_properties(s)!=Variables::DOES_NOT_EXIST;
}

bool assign_values_to_variables_stage_zero();
bool assign_values_to_variables_stage_one();
bool assign_values_to_variables_stage_two();

#endif
