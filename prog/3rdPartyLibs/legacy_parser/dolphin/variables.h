
#ifndef __DOLPHIN__VARIABLES_H

#define __DOLPHIN__VARIABLES_H

#include "variable.h"

struct Variables
{
	std::map<const char *, variable_base *, NullTerminatedStringCompare> database;
	
	variable<bool> my_new_variable;
	variable<const char *> another_variable;
	
	bool unicode;
	int alphabet_cardinality;
	const char *internal_char_type; // "char", "wchar_t" or some user type.
	const char *internal_string_type; // "std::string", "std::wstring", "std::basic_string<...>" or some user type.
	
	bool start_conditions_enabled;
	bool compress_action_vectors;
	bool generate_verbose_prints;
	bool generate_sanity_checks;
	bool eat_one_character_upon_lexical_error;
	bool store_lexeme_in_string;
	bool append_data_member;
	bool case_insensitive;
	bool line_directives;
	bool allow_inclusion_cycle_between_whale_and_dolphin;
	
	bool compress_tables;
	int table_compression_exception_width;
	
	enum OutputLanguage { LANGUAGE_CPP, LANGUAGE_VINTAGE_CPP };
	OutputLanguage output_language;
	
	const char *dolphin_class_name;
	
	bool using_whale;
	bool whale_emulation_mode;
	const char *whale_namespace;
	const char *whale_file;
	
	int dump_nfa_to_file;
	int dump_dfa_to_file;
	int dump_minimized_dfa_to_file;
	
	const char*analyzer_state_type;
	const char*get_token_function_return_value;
	const char*get_token_function_parameters;
	const char*input_stream_class;
	const char*input_character_class;
	
	const char*std_namespace;
	const char*how_to_get_actual_character;
	const char*how_to_check_eof;
	const char*how_to_check_stream_error;
	const char*how_to_get_character_from_stream;
	const char*how_to_return_token;
	
	const char*code_in_token_enum;
	const char*glob_decl_in_h;
	const char*glob_decl_in_cpp;

	const char*code_in_h_before_all;
	const char*code_in_h;
	const char*code_in_h_after_all;
	const char*code_in_cpp_before_all;
	const char*code_in_cpp;
	const char*code_in_cpp_after_all;
	const char*code_in_class_before_all;
	const char*code_in_class;
	const char*code_in_class_after_all;
	const char*code_in_constructor;

	bool generate_terminals;
	const char*generate_terminals_file;
	const char*terminals_namespace;
	const char*tokens_prefix;
	Variables();
	bool check_database_integrity();
	
	// these variables cannot be directly accessed by assignment statements.
	
	bool generate_fixed_length_lookahead_support;
	bool generate_arbitrary_lookahead_support;
	bool generate_eof_lookahead_support;
	
	bool generate_table_of_actions;
	bool input_stream_is_FILE_asterisk;
	
	bool using_layer2; // should make it accessible
	bool access_transitions_through_a_method;
	
	bool internal_char_type_is_char;
	bool internal_char_type_is_wchar_t;
};

bool variable_exists(const char*s);
bool assign_values_to_variables_stage_zero();
bool assign_values_to_variables_stage_one();
bool assign_values_to_variables_stage_two();

#endif
