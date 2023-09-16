
#include "whale.h"

#include <stdio.h>
#include <iostream>
#include <typeinfo>
using namespace std;

#include "assert.h"
#include "process.h"
//#include "stl.h"
#include "classes.h"
#include "utilities.h"
#include "ancillary.h"
using namespace Whale;

//#define DEBUG
//#define DEBUG_DATA_MEMBERS_IN_BODIES
//#define DEBUG_COMMON_ANCESTOR

void make_built_in_symbols();
bool process_class_declarations();
bool process_expressions();
bool process_expression_proc(NonterminalExpression *expr, ProcessExpressionProcData &proc_data);
bool process_expression_proc_ii(NonterminalExpression *expression, ProcessExpressionProcIIData &proc_data);
bool process_terminal_type_declaration(TerminalData &terminal, NonterminalTypeExpression *type);
bool process_nonterminal_type_declaration(int nn, NonterminalTypeExpression *type);
ClassHierarchy::Class *process_external_type_declaration(NonterminalExternalTypeExpression *external_type, NonterminalData *nonterminal);
bool process_alternative_type_declaration(AlternativeData &alternative, NonterminalData &nonterminal, int alternative_number, NonterminalTypeExpression *type);
ClassHierarchy::Class *process_standalone_type_declaration(NonterminalTypeExpression *type);
string default_naming_proc(const char *s);
string default_terminal_class_name(const char *s);
string default_nonterminal_class_name(const char *s);
string default_alternative_class_name(const string &s, int alternative_number);
void report_undefined_base_class(ostream &os, Terminal *t);
bool identifier_contains_no_hyphens(Terminal *t);
ClassHierarchy::Class *process_scope_in_member_name_specification(Terminal *scope, Terminal *id, const pair<int, int> &location);
ClassHierarchy::DataMember *process_single_data_member_declaration(Terminal *scope, Terminal *name, int nn, int an, int number_of_iterations);
string generate_default_data_member_name(int n);
string serialize_external_type_expression(NonterminalExternalTypeExpression &expr);
bool apply_common_ancestor_determination_procedure(ClassHierarchy::Class *&dest, ClassHierarchy::Class *src, SymbolNumber sn, ClassHierarchy::DataMember *data_member, Terminal *t);
bool process_option_statement(NonterminalOptionStatement &option_statement);
pair<char *, AssignmentData::ValueType> process_single_value_in_option_statement(Terminal *t, bool process_escape_sequences_in_string);
bool high_level_decode_escape_sequences(Terminal *t, string &s);
void make_up_data_member_in_class_symbol();

bool process_grammar()
{
	make_built_in_symbols();
	if(!process_class_declarations()) return false;
	if(!assign_values_to_variables_stage_zero()) return false;
	if(WhaleData::data.variables.make_up_connection)
		make_up_data_member_in_class_symbol();
	if(!process_expressions()) return false;
	
	if(WhaleData::data.terminals.size()<=2)
	{
		cout << "The grammar should contain at least one terminal symbol.\n";
		return false;
	}
	if(WhaleData::data.nonterminals.size()<=1)
	{
		cout << "The grammar should contain at least one nonterminal symbol.\n";
		return false;
	}
	assert(classes.in_global_scope.size());
	
	return true;
}

void make_built_in_symbols()
{
	// S'
	
	NonterminalData start_symbol_of_augmented_grammar;
	
	start_symbol_of_augmented_grammar.name="S'";
	start_symbol_of_augmented_grammar.declaration=NULL;
	start_symbol_of_augmented_grammar.type=0;
	
	start_symbol_of_augmented_grammar.is_ancillary=true;
	start_symbol_of_augmented_grammar.category=NonterminalData::START;
	
	WhaleData::data.nonterminals.push_back(start_symbol_of_augmented_grammar);
	
	// eof
	
	TerminalData eof_terminal;
	eof_terminal.name="eof";
	eof_terminal.type=new ClassHierarchy::Class("TerminalEOF", classes.terminal, NULL);
	eof_terminal.type->is_abstract=false;
	WhaleData::data.eof_terminal_number=WhaleData::data.terminals.size();
	eof_terminal.type->assigned_to=SymbolNumber::for_terminal(WhaleData::data.eof_terminal_number);
	WhaleData::data.terminals.push_back(eof_terminal);
	
	// error
	
	TerminalData error_terminal;
	error_terminal.name="error";
	error_terminal.type=new ClassHierarchy::Class("TerminalError", classes.terminal, NULL);
	error_terminal.type->is_abstract=false;
	WhaleData::data.error_terminal_number=WhaleData::data.terminals.size();
	error_terminal.type->assigned_to=SymbolNumber::for_terminal(WhaleData::data.error_terminal_number);
	WhaleData::data.terminals.push_back(error_terminal);
}

bool process_class_declarations()
{
	bool flag=true;
	
	// Pass 1. Determining how many rules are there for each nonterminal.
	// It is really vital to do it before all other processing.
	map<const char *, int, NullTerminatedStringCompare> how_many_rules_for_nonterminal;
	for(int i=0; i<WhaleData::data.S->statements.size(); i++)
	{
	    auto &a = *WhaleData::data.S->statements[i];
		if(typeid(a)==typeid(NonterminalRuleStatement))
		{
			NonterminalRuleStatement &rule_statement=*dynamic_cast<NonterminalRuleStatement *>(WhaleData::data.S->statements[i]);
			how_many_rules_for_nonterminal[rule_statement.left->text]++;
		}
	}
#ifdef DEBUG
	cout << how_many_rules_for_nonterminal << "\n";
#endif
	
	// Pass 2: Making the class hierarchy. It is done without looking
	// inside the expressions.
	for(int i=0; i<WhaleData::data.S->statements.size(); i++)
	{
	    auto &a = *WhaleData::data.S->statements[i];
		if(typeid(a)==typeid(NonterminalTerminalStatement))
		{
			NonterminalTerminalStatement &terminal_statement=*dynamic_cast<NonterminalTerminalStatement *>(WhaleData::data.S->statements[i]);
			
			for(int j=0; j<terminal_statement.names.size(); j++)
			{
				Terminal *t=terminal_statement.names[j];
				int tn=WhaleData::data.find_terminal(t->text);
				int nn=WhaleData::data.find_nonterminal(t->text);
				assert(!(tn!=-1 && nn!=-1));
				if(tn!=-1)
				{
					cout << "Terminal " << t->text << ", ";
					WhaleData::data.terminals[tn].print_where_it_was_declared(cout);
					cout << ", cannot be redeclared at ";
					t->print_location(cout);
					cout << ".\n";
					flag=false;
				}
				else if(nn!=-1)
				{
					cout << "Attempt to redeclare nonterminal " << t->text << " (";
					WhaleData::data.nonterminals[nn].print_where_it_was_declared(cout);
					cout << ") as terminal at ";
					t->print_location(cout);
					cout << ".\n";
					flag=false;
				}
				else
				{
					TerminalData terminal;
					terminal.name=t->text;
					terminal.declaration=t;
					
					if(process_terminal_type_declaration(terminal, terminal_statement.types[j]))
						WhaleData::data.terminals.push_back(terminal);
					else
						flag=false;
				}
			}
		}
		else if(typeid(a)==typeid(NonterminalNonterminalStatement))
		{
			NonterminalNonterminalStatement &nonterminal_statement=*dynamic_cast<NonterminalNonterminalStatement *>(WhaleData::data.S->statements[i]);
			
			bool has_external_tag;
			if(!strcmp(nonterminal_statement.kw->text, "nonterminal"))
				has_external_tag=false;
			else if(!strcmp(nonterminal_statement.kw->text, "external"))
				has_external_tag=true;
			else
				assert(false);
			
			for(int j=0; j<nonterminal_statement.names.size(); j++)
			{
				Terminal *t=nonterminal_statement.names[j];
				NonterminalExternalTypeExpression *external_type=nonterminal_statement.external_types[j];
				int tn=WhaleData::data.find_terminal(t->text);
				int nn=WhaleData::data.find_nonterminal(t->text);
				assert(!(tn!=-1 && nn!=-1));
				if(tn!=-1)
				{
					cout << "Attempt to redeclare terminal " << t->text << " (";
					WhaleData::data.terminals[tn].print_where_it_was_declared(cout);
					cout << ") as nonterminal at ";
					t->print_location(cout);
					cout << ".\n";
					flag=false;
				}
				else if(nn!=-1)
				{
					cout << "Nonterminal " << t->text << ", ";
					WhaleData::data.nonterminals[nn].print_where_it_was_declared(cout);
					cout << ", cannot be redeclared at ";
					t->print_location(cout);
					cout << ".\n";
					flag=false;
				}
				else
				{
					int new_nn=WhaleData::data.nonterminals.size();
					WhaleData::data.nonterminals.push_back(NonterminalData());
					
					NonterminalData &nonterminal=WhaleData::data.nonterminals[new_nn];
					nonterminal.name=t->text;
					nonterminal.declaration=t;
					
					if(!has_external_tag)
					{
						nonterminal.category=NonterminalData::NORMAL;
						
						if(!process_nonterminal_type_declaration(new_nn, nonterminal_statement.internal_types[j]))
							flag=false;
						if(external_type && !process_external_type_declaration(external_type, &nonterminal))
							flag=false;
					}
					else
					{
						nonterminal.category=NonterminalData::EXTERNAL;
						
						if(!process_nonterminal_type_declaration(new_nn, NULL))
							flag=false;
						if(!process_external_type_declaration(external_type, &nonterminal))
							flag=false;
						
						nonterminal.type->template_parameters.push_back(nonterminal.external_type);
					}
					
					// if there were any errors (which have
					// already been reported by now),
					// nonterminal.type may be NULL.
					if(nonterminal.type!=NULL)
					{
						int nr=how_many_rules_for_nonterminal[t->text];
						if(has_external_tag)
							nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::WRAPPER;
						else if(nr<=1)
							nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::UNKNOWN;
						else
							nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::BASE;
					}
				}
			}
		}
		else if(typeid(a)==typeid(NonterminalClassStatement))
		{
			NonterminalClassStatement &class_statement=*dynamic_cast<NonterminalClassStatement *>(WhaleData::data.S->statements[i]);
			
			ClassHierarchy::Class *m=process_standalone_type_declaration(class_statement.expr);
			if(!m) flag=false;
		}
		else if(typeid(a)==typeid(NonterminalRuleStatement))
		{
			NonterminalRuleStatement &rule_statement=*dynamic_cast<NonterminalRuleStatement *>(WhaleData::data.S->statements[i]);
			
			Terminal *t=rule_statement.left;
			
			int tn=WhaleData::data.find_terminal(t->text);
			int nn=WhaleData::data.find_nonterminal(t->text);
			assert(!(tn!=-1 && nn!=-1));
			
			int nr=how_many_rules_for_nonterminal[t->text];
			assert(nr>0);
			
			if(tn!=-1)
			{
				cout << "Terminal " << t->text << ", ";
				WhaleData::data.terminals[tn].print_where_it_was_declared(cout);
				cout << ", cannot be used in left part of rule at ";
				t->print_location(cout);
				cout << ".\n";
				flag=false;
				continue;
			}
			else if(nn==-1 && nr==1)
			{
				int new_nn=WhaleData::data.nonterminals.size();
				WhaleData::data.nonterminals.push_back(NonterminalData());
				
				NonterminalData &nonterminal=WhaleData::data.nonterminals[new_nn];
				nonterminal.name=t->text;
				nonterminal.declaration=t;
				nonterminal.category=NonterminalData::NORMAL;
				
				// creating a single class
				bool result=process_nonterminal_type_declaration(new_nn, rule_statement.type);
				if(!result)
				{
					flag=false;
					continue;
				}
				nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::SINGLE;
				
				AlternativeData alternative;
				alternative.declaration=rule_statement.left;
				alternative.type=NULL;
				alternative.expr=rule_statement.right;
			//	cout << "pushing alternative: " << (void *)rule_statement.right << "\n";
				nonterminal.alternatives.push_back(alternative);
				nonterminal.multiple_alternatives_mode=false;
			}
			else if(nn==-1 && nr>1)
			{
				int new_nn=WhaleData::data.nonterminals.size();
				WhaleData::data.nonterminals.push_back(NonterminalData());
				
				NonterminalData &nonterminal=WhaleData::data.nonterminals[new_nn];
				nonterminal.name=t->text;
				nonterminal.declaration=t;
				nonterminal.category=NonterminalData::NORMAL;
				
				// Creating a base class using a generated name.
				bool result=process_nonterminal_type_declaration(new_nn, NULL);
				if(!result)
				{
					flag=false;
					continue;
				}
				nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::BASE;
				
				AlternativeData alternative;
				alternative.declaration=rule_statement.left;
				alternative.expr=rule_statement.right;
				bool result2=process_alternative_type_declaration(alternative, nonterminal, nonterminal.alternatives.size(), rule_statement.type);
				if(!result2)
				{
				//	cout << "pushing alternative: " << (void *)rule_statement.right << "\n";
					nonterminal.alternatives.push_back(alternative);
					flag=false;
					continue;
				}
				assert(!alternative.type->is_abstract);
			//	cout << "pushing alternative: " << (void *)rule_statement.right << "\n";
				nonterminal.alternatives.push_back(alternative);
				nonterminal.multiple_alternatives_mode=true;
			}
			else if(nn!=-1)
			{
				NonterminalData &nonterminal=WhaleData::data.nonterminals[nn];
				assert(nonterminal.category==NonterminalData::NORMAL
					|| nonterminal.category==NonterminalData::EXTERNAL);
				
				if(!nonterminal.type)
				{
					assert(!flag);
				}
				else if(nonterminal.category==NonterminalData::EXTERNAL)
				{
					assert(nonterminal.type->nonterminal_class_category==ClassHierarchy::Class::WRAPPER);
					nonterminal.multiple_alternatives_mode=false;
				}
				else if(nr>1)
				{
					assert(nonterminal.type->nonterminal_class_category==ClassHierarchy::Class::BASE);
					nonterminal.multiple_alternatives_mode=true;
				}
				else if(rule_statement.type || nonterminal.type->nonterminal_class_category==ClassHierarchy::Class::BASE)
				{
					nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::BASE;
					nonterminal.multiple_alternatives_mode=true;
				}
				else if(nonterminal.type->nonterminal_class_category==ClassHierarchy::Class::UNKNOWN)
				{
					nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::SINGLE;
					nonterminal.multiple_alternatives_mode=false;
				}
				else
					assert(false);
				
				AlternativeData alternative;
				alternative.declaration=rule_statement.left;
				alternative.expr=rule_statement.right;
				if(nonterminal.multiple_alternatives_mode)
				{
					bool result=process_alternative_type_declaration(alternative, nonterminal, nonterminal.alternatives.size(), rule_statement.type);
					if(!result)
					{
						nonterminal.alternatives.push_back(alternative);
						flag=false;
						continue;
					}
				}
				else
					alternative.type=NULL;
			//	cout << "pushing alternative: " << (void *)rule_statement.right << "\n";
				nonterminal.alternatives.push_back(alternative);
			}
			else
				assert(false);
		}
		else if(typeid(a)==typeid(NonterminalPrecedenceStatement))
		{
			// precedence statements are dealt with in precedence.cpp
		}
		else if(typeid(a)==typeid(NonterminalOptionStatement))
		{
			bool result=process_option_statement(*dynamic_cast<NonterminalOptionStatement *>(WhaleData::data.S->statements[i]));
			if(!result)
				flag=false;
		}
		else if(typeid(a)==typeid(NonterminalInvalidStatement))
			flag=false;
		else
			assert(false);
	}
	
	if(!flag) return false;
	
	for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
	{
		NonterminalData &nonterminal=WhaleData::data.nonterminals[i];
		if(nonterminal.is_ancillary) continue;
		
		if(!nonterminal.alternatives.size() && !nonterminal.type->descendants.size())
		{
			assert(nonterminal.type->nonterminal_class_category==ClassHierarchy::Class::UNKNOWN);
			cout << "Applying a doubtful fix to nonterminal " << nonterminal.name << "\n";
			nonterminal.type->nonterminal_class_category=ClassHierarchy::Class::SINGLE;
		}
	}
	
	return true;
}

bool process_expressions()
{
	bool flag=true;
	
	for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
	{
		if(WhaleData::data.nonterminals[i].is_ancillary) continue;
		
		for(int j=0; j<WhaleData::data.nonterminals[i].alternatives.size(); j++)
		{
			NonterminalData &nonterminal=WhaleData::data.nonterminals[i]; // this declaration cannot be moved one loop level above, because process_expressions_proc can add nonterminals.
			AlternativeData &alternative=nonterminal.alternatives[j];
		#ifdef DEBUG
			cout << i << ", " <<j << ": alternative.expr = " << (void *)alternative.expr << "\n";
		#endif
			assert(alternative.expr);
			
			ProcessExpressionProcData proc_data(i, j);
			if(!process_expression_proc(alternative.expr, proc_data))
				flag=false;
		}
	}
	
	if(!flag) return false;
	
	map<int, vector<NonterminalExpressionSymbol *> > nonterminals_that_cannot_be_connected_up;
	for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
	{
		NonterminalData &nonterminal=WhaleData::data.nonterminals[i];
		if(nonterminal.is_ancillary) continue;
		
		for(int j=0; j<nonterminal.alternatives.size(); j++)
		{
			AlternativeData &alternative=nonterminal.alternatives[j];
			assert(alternative.expr);
			
			ProcessExpressionProcIIData proc_data(i, j, nonterminals_that_cannot_be_connected_up);
			if(!process_expression_proc_ii(alternative.expr, proc_data))
				flag=false;
		}
	}
	
	// A -> a create b C;
	// B -> a b C;			// create operator required here.
	// C -> c connect_up d;
	for(map<int, vector<NonterminalExpressionSymbol *> >::const_iterator p=nonterminals_that_cannot_be_connected_up.begin(); p!=nonterminals_that_cannot_be_connected_up.end(); p++)
	{
		NonterminalData &nonterminal=WhaleData::data.nonterminals[p->first];
		
		vector<Terminal *> v1, v2;
		for(int i=0; i<nonterminal.requests_to_connect_up.size(); i++)
			v1.push_back(nonterminal.requests_to_connect_up[i]->kw);
		for(int i=0; i<p->second.size(); i++)
			v2.push_back(p->second[i]->symbol);
		
		cout << "Using connect_up operator ";
		print_a_number_of_terminal_locations(cout, v1, "at");
		cout << " requires create operator before any reference to "
			<< "nonterminal " << nonterminal.name
			<< "; appearance" << (v2.size()==1 ? "" : "s")
			<< " of " << nonterminal.name << " ";
		print_a_number_of_terminal_locations(cout, v2, "at");
		cout << " " << (v2.size()==1 ? "is" : "are")
			<< " not preceded by create.\n";
		
		flag=false;
	}
	
	return flag;
}

bool process_expression_proc(NonterminalExpression *expr, ProcessExpressionProcData &proc_data)
{
	if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
		bool iabc_backup=proc_data.inside_anything_but_concatenation;
		proc_data.inside_anything_but_concatenation=true;
		
		bool result1=process_expression_proc(expr_d.expr1, proc_data);
		bool result2=process_expression_proc(expr_d.expr2, proc_data);
		
		proc_data.inside_anything_but_concatenation=iabc_backup;
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
		
		bool iabc_backup=proc_data.inside_anything_but_concatenation;
		proc_data.inside_anything_but_concatenation=true;
		
		process_expression_proc(expr_c.expr1, proc_data);
		
		cout << "Conjunction, used at ";
		expr_c.op->print_location(cout);
		cout << ", is not supported.\n";
		
		process_expression_proc(expr_c.expr2, proc_data);
		
		proc_data.inside_anything_but_concatenation=iabc_backup;
		return false;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
		bool result1=process_expression_proc(expr_cat.expr1, proc_data);
		bool result2=process_expression_proc(expr_cat.expr2, proc_data);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
	{
		NonterminalExpressionComplement &expr_com=*dynamic_cast<NonterminalExpressionComplement *>(expr);
		cout << "Complement, used at ";
		expr_com.op->print_location(cout);
		cout << ", is not supported.\n";
		
		bool iabc_backup=proc_data.inside_anything_but_concatenation;
		proc_data.inside_anything_but_concatenation=true;
		
		process_expression_proc(expr_com.expr, proc_data);
		
		proc_data.inside_anything_but_concatenation=iabc_backup;
		return false;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
		
		bool iabc_backup=proc_data.inside_anything_but_concatenation;
		proc_data.inside_anything_but_concatenation=true;
		
		bool result=process_expression_proc(expr_om.expr, proc_data);
		
		proc_data.inside_anything_but_concatenation=iabc_backup;
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		bool result=process_expression_proc(expr_p.expr, proc_data);
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
		
		if(proc_data.member_expression)
		{
			cout << "Warning: member name specification at ";
			proc_data.member_expression->t->print_location(cout);
			cout << " has no effect inside iteration at ";
			expr_it.sign->print_location(cout);
			cout << ".\n";
		}
		
		NonterminalMemberExpression *backup=proc_data.member_expression;
		proc_data.member_expression=NULL;
		proc_data.number_of_nested_iterations++;
		bool iabc_backup=proc_data.inside_anything_but_concatenation;
		proc_data.inside_anything_but_concatenation=true;
		
		bool result=process_expression_proc(expr_it.expr, proc_data);
		
		proc_data.inside_anything_but_concatenation=iabc_backup;
		proc_data.number_of_nested_iterations--;
		proc_data.member_expression=backup;
		
		if(!result) return false;
		
		expr_it.local_iterator_number=++proc_data.nonterminal().number_of_iterators;
		expr_it.body_sn=make_body_symbol(expr_it.expr, proc_data.nn, proc_data.an, expr_it.local_iterator_number, 0, expr_it.sign);
		expr_it.iterator_nn=make_single_iterator_symbol(expr_it.body_sn, proc_data.nn, proc_data.an, expr_it.local_iterator_number, expr_it.sign);
		
		if(typeid(*expr_it.sign)==typeid(TerminalAsterisk))
			expr_it.reflexive=true;
		else if(typeid(*expr_it.sign)==typeid(TerminalPlus))
			expr_it.reflexive=false;
		else
			assert(false);
		
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIterationPair))
	{
		NonterminalExpressionIterationPair &expr_it2=*dynamic_cast<NonterminalExpressionIterationPair *>(expr);
		
		if(proc_data.member_expression)
		{
			cout << "Warning: member name specification at ";
			proc_data.member_expression->t->print_location(cout);
			cout << " has no effect inside iteration at ";
			expr_it2.kw->print_location(cout);
			cout << ".\n";
		}
		
		NonterminalMemberExpression *backup=proc_data.member_expression;
		proc_data.member_expression=NULL;
		proc_data.number_of_nested_iterations++;
		bool iabc_backup=proc_data.inside_anything_but_concatenation;
		proc_data.inside_anything_but_concatenation=true;
		
		bool result1=process_expression_proc(expr_it2.expr1, proc_data);
		bool result2=process_expression_proc(expr_it2.expr2, proc_data);
		
		proc_data.inside_anything_but_concatenation=iabc_backup;
		proc_data.number_of_nested_iterations--;
		proc_data.member_expression=backup;
		
		if(!result1 || !result2) return false;
		
		expr_it2.local_iterator_number=++proc_data.nonterminal().number_of_iterators;
		expr_it2.body1_sn=make_body_symbol(expr_it2.expr1, proc_data.nn, proc_data.an, expr_it2.local_iterator_number, 1, expr_it2.kw);
		expr_it2.body2_sn=make_body_symbol(expr_it2.expr2, proc_data.nn, proc_data.an, expr_it2.local_iterator_number, 2, expr_it2.kw);
		expr_it2.iterator_nn=make_pair_iterator_symbol(expr_it2.body1_sn, expr_it2.body2_sn, proc_data.nn, proc_data.an, expr_it2.local_iterator_number, expr_it2.kw);
		
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
		return true;
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		Terminal *t=expr_s.symbol;
		
		int tn=WhaleData::data.find_terminal(t->text);
		int nn=WhaleData::data.find_nonterminal(t->text);
		assert(tn==-1 || nn==-1);
		
		if(tn==-1 && nn==-1)
		{
			cout << "Undefined symbol " << t->text << " at ";
			t->print_location(cout);
			cout << ".\n";
			return false;
		}
		
		proc_data.symbol_count++;
		
		expr_s.number_of_iterations=proc_data.number_of_nested_iterations;
		
		ClassHierarchy::Class *internal_class_for_this_symbol;
		ClassHierarchy::Class *external_class_for_this_symbol;
		if(tn!=-1)
		{
			if(tn==WhaleData::data.eof_terminal_number)
			{
				cout << "Warning: explicit use of end-of-file symbol at ";
				t->print_location(cout);
				cout << ".\n";
			}
			expr_s.sn=SymbolNumber::for_terminal(tn);
			internal_class_for_this_symbol=WhaleData::data.terminals[tn].type;
			external_class_for_this_symbol=NULL;
		}
		else if(nn!=-1)
		{
			NonterminalData &nonterminal=WhaleData::data.nonterminals[nn];
			
			expr_s.sn=SymbolNumber::for_nonterminal(nn);
			if(nonterminal.external_type)
			{
				internal_class_for_this_symbol=nonterminal.type;
				external_class_for_this_symbol=nonterminal.external_type;
			}
			else
			{
				internal_class_for_this_symbol=nonterminal.type;
				external_class_for_this_symbol=NULL;
			}
		}
		assert(internal_class_for_this_symbol);
		
		NonterminalMemberExpression *m_expr=proc_data.member_expression;
		
		if(m_expr)
			m_expr->number_of_affected_symbols++;
		
		if(m_expr && m_expr->is_nothing)
			return true;
		
		if(!m_expr && WhaleData::data.variables.default_member_name_is_nothing)
			return true;
		
		if(!m_expr)
		{
			ClassHierarchy::Class *class_assigned_to_host_nonterminal=WhaleData::data.get_class_assigned_to_alternative(proc_data.nn, proc_data.an);
			string s=generate_default_data_member_name(proc_data.symbol_count);
			triad<ClassHierarchy::Class *, int, int> cnr=class_assigned_to_host_nonterminal->find_data_member_in_direct_relatives(s.c_str());
			
			if(cnr.first)
			{
				cout << "Unable to generate default WhaleData::data member name for ";
				cout << WhaleData::data.full_symbol_name(expr_s.sn, false) << " at ";
				t->print_location(cout);
				cout << ", because identifier " << s << " is already in use.\n";
				return false;
			}
			
			ClassHierarchy::DataMember *new_data_member=new ClassHierarchy::DataMember;
			
			new_data_member->name=s;
			new_data_member->was_implicitly_declared=true;
			new_data_member->declared_at=t;
			new_data_member->number_of_nested_iterations=proc_data.number_of_nested_iterations;
			
			if(external_class_for_this_symbol)
			{
				new_data_member->type=external_class_for_this_symbol;
				new_data_member->internal_type_if_there_is_an_external_type=internal_class_for_this_symbol;
			}
			else
			{
				new_data_member->type=internal_class_for_this_symbol;
				new_data_member->internal_type_if_there_is_an_external_type=NULL;
			}
			
			classes.put_data_member_to_class(new_data_member, class_assigned_to_host_nonterminal);
			expr_s.data_member_i=new_data_member;
			
			return true;
		}
		
		assert(m_expr);
		
		/* What are we going to do is to unify WhaleData::data types.           */
		/* Consider ( ... symbol ... ) = (member expression)         */
		/* We have just read _symbol_, and we have access to the     */
		/* previously read member expression.                        */
		/* internal_class_for_this_symbol and external_class_... are */
		/* properties of symbol that doesn't depend upon this        */
		/* context. m_expr->everything are different properties of   */
		/* the member expression. We have to modify WhaleData::data members     */
		/* created by member expression so that they would become    */
		/* capable of holding our symbol, while retaining all holding*/
		/* capabilities they had before. */
		
		// 6 cases to consider.
		if(m_expr->i_specified && !m_expr->e_specified && !external_class_for_this_symbol)
		{
			// the most simple case: just unify the type WhaleData::data
			// member has now with the type assigned to symbol.
			bool result=apply_common_ancestor_determination_procedure(m_expr->data_member_i->type, internal_class_for_this_symbol, expr_s.sn, m_expr->data_member_i, t);
			if(!result) return false;
			expr_s.data_member_i=m_expr->data_member_i;
		}
		else if(!m_expr->i_specified && m_expr->e_specified && !external_class_for_this_symbol)
		{
			bool result=apply_common_ancestor_determination_procedure(m_expr->data_member_e->internal_type_if_there_is_an_external_type, internal_class_for_this_symbol, expr_s.sn, m_expr->data_member_e, t);
			assert(result);
			expr_s.data_member_e=m_expr->data_member_e;
		}
		else if(m_expr->i_specified && m_expr->e_specified && !external_class_for_this_symbol)
		{
			bool result1=apply_common_ancestor_determination_procedure(m_expr->data_member_i->type, internal_class_for_this_symbol, expr_s.sn, m_expr->data_member_i, t);
			bool result2=apply_common_ancestor_determination_procedure(m_expr->data_member_e->internal_type_if_there_is_an_external_type, internal_class_for_this_symbol, expr_s.sn, m_expr->data_member_e, t);
			if(!result1 || !result2) return false;
			expr_s.data_member_i=m_expr->data_member_i;
			expr_s.data_member_e=m_expr->data_member_e;
		}
		else if(m_expr->i_specified && !m_expr->e_specified && external_class_for_this_symbol)
		{
			bool result1=apply_common_ancestor_determination_procedure(m_expr->data_member_i->type, external_class_for_this_symbol, expr_s.sn, m_expr->data_member_i, t);
			bool result2=apply_common_ancestor_determination_procedure(m_expr->data_member_i->internal_type_if_there_is_an_external_type, internal_class_for_this_symbol, expr_s.sn, m_expr->data_member_i, t);
			if(!result1 || !result2) return false;
			expr_s.data_member_e=m_expr->data_member_i;
		}
		else if(!m_expr->i_specified && m_expr->e_specified && external_class_for_this_symbol)
		{
			bool result1=apply_common_ancestor_determination_procedure(m_expr->data_member_e->type, external_class_for_this_symbol, expr_s.sn, m_expr->data_member_e, t);
			bool result2=apply_common_ancestor_determination_procedure(m_expr->data_member_e->internal_type_if_there_is_an_external_type, internal_class_for_this_symbol, expr_s.sn, m_expr->data_member_e, t);
			if(!result1 || !result2) return false;
			expr_s.data_member_e=m_expr->data_member_e;
		}
		else if(m_expr->i_specified && m_expr->e_specified && external_class_for_this_symbol)
		{
			bool result1=apply_common_ancestor_determination_procedure(m_expr->data_member_i->type, internal_class_for_this_symbol, expr_s.sn, m_expr->data_member_i, t);
			bool result2=apply_common_ancestor_determination_procedure(m_expr->data_member_e->type, external_class_for_this_symbol, expr_s.sn, m_expr->data_member_e, t);
			if(!result1 || !result2) return false;
			expr_s.data_member_i=m_expr->data_member_i;
			expr_s.data_member_e=m_expr->data_member_e;
		}
		else
			assert(false);
		
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionMemberName))
	{
		NonterminalExpressionMemberName &expr_mn=*dynamic_cast<NonterminalExpressionMemberName *>(expr);
		NonterminalMemberExpression *expr_m=expr_mn.expr_m;
		
		if(typeid(*expr_m)==typeid(NonterminalMemberExpressionNothing))
		{
			NonterminalMemberExpressionNothing &em_nothing=*dynamic_cast<NonterminalMemberExpressionNothing *>(expr_m);
			expr_m->is_nothing=true;
			expr_m->t=em_nothing.kw;
		}
		else if(typeid(*expr_m)==typeid(NonterminalMemberExpressionInternal))
		{
			NonterminalMemberExpressionInternal &em_internal=*dynamic_cast<NonterminalMemberExpressionInternal *>(expr_m);
			
			expr_m->scope_i=em_internal.scope;
			expr_m->name_i=em_internal.name;
			
			expr_m->i_specified=true;
			expr_m->t=em_internal.name;
		}
		else if(typeid(*expr_m)==typeid(NonterminalMemberExpressionExternal))
		{
			NonterminalMemberExpressionExternal &em_external=*dynamic_cast<NonterminalMemberExpressionExternal *>(expr_m);
			
			expr_m->type_e=em_external.type;
			expr_m->scope_e=em_external.scope;
			expr_m->name_e=em_external.name;
			
			expr_m->e_specified=true;
			expr_m->t=em_external.name;
		}
		else if(typeid(*expr_m)==typeid(NonterminalMemberExpressionBoth))
		{
			NonterminalMemberExpressionBoth &em_both=*dynamic_cast<NonterminalMemberExpressionBoth *>(expr_m);
			
			expr_m->scope_i=em_both.scope1;
			expr_m->name_i=em_both.name1;
			
			expr_m->type_e=em_both.type2;
			expr_m->scope_e=em_both.scope2;
			expr_m->name_e=em_both.name2;
			
			expr_m->i_specified=true;
			expr_m->e_specified=true;
			expr_m->t=em_both.name1;
		}
		else
			assert(false);
		
		bool local_flag=true;
		
		if(expr_m->i_specified)
		{
			expr_m->data_member_i=process_single_data_member_declaration(expr_m->scope_i, expr_m->name_i, proc_data.nn, proc_data.an, proc_data.number_of_nested_iterations);
			if(!expr_m->data_member_i)
				local_flag=false;
		}
		
		if(expr_m->e_specified)
		{
			expr_m->data_member_e=process_single_data_member_declaration(expr_m->scope_e, expr_m->name_e, proc_data.nn, proc_data.an, proc_data.number_of_nested_iterations);
			if(expr_m->data_member_e)
			{
				assert(expr_m->type_e);
				ClassHierarchy::Class *this_external_type=process_external_type_declaration(expr_m->type_e, NULL);
				if(expr_m->data_member_e->type &&
					expr_m->data_member_e->type!=this_external_type)
				{
					cout << "External class " << this_external_type->get_full_name()
						<< " cannot be assigned to WhaleData::data member "
						<< expr_m->name_e->text << " at ";
					expr_m->name_e->print_location(cout);
					cout << ", because the latter already has incompatible type ";
					cout << expr_m->data_member_e->type->get_full_name() << ".\n";
					local_flag=false;
				}
				else
					expr_m->data_member_e->type=this_external_type;
				
				if(!expr_m->data_member_e->type)
					local_flag=false;
			}
			else
				local_flag=false;
		}
		
		if(!local_flag)
			return false;
		
		NonterminalMemberExpression *backup=proc_data.member_expression;
		proc_data.member_expression=expr_mn.expr_m;
		
		bool result=process_expression_proc(expr_mn.expr, proc_data);
		
		proc_data.member_expression=backup;
		
		// in case no types were assigned to WhaleData::data members
		if(expr_m->i_specified)
		{
			assert(expr_m->data_member_i);
			
			if(!expr_m->data_member_i->type)
				expr_m->data_member_i->type=classes.symbol;
		}
		if(expr_m->e_specified)
		{
			assert(expr_m->data_member_e && expr_m->data_member_e->type);
		}
		
		if(!result) return false;
		
		if(!expr_mn.expr_m->number_of_affected_symbols)
		{
			cout << "Warning: member name declaration ";
			expr_mn.expr_m->t->print_location(cout);
			cout << " doesn't affect any symbols.\n";
		}
		
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCreate))
	{
		NonterminalExpressionCreate &expr_create=*dynamic_cast<NonterminalExpressionCreate *>(expr);
		if(proc_data.nonterminal().category==NonterminalData::EXTERNAL)
		{
			cout << "Usage of create operator at ";
			expr_create.kw->print_location(cout);
			cout << " is illegal, because nonterminal "
				<< proc_data.nonterminal().name << ", ";
			proc_data.nonterminal().print_where_it_was_declared(cout);
			cout << ", is external.\n";
			return false;
		}
		else if(proc_data.inside_anything_but_concatenation)
		{
			cout << "Error at ";
			expr_create.kw->print_location(cout);
			cout << ": create operator can only appear on the top level of expression.\n";
			return false;
		}
		else if(proc_data.make_operator)
		{
			cout << "Create operator at ";
			proc_data.make_operator->kw->print_location(cout);
			cout << " cannot be invoked once more at ";
			expr_create.kw->print_location(cout);
			cout << ".\n";
			return false;
		}
		proc_data.make_operator=&expr_create;
		
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionUpdate))
	{
		NonterminalExpressionUpdate &expr_update=*dynamic_cast<NonterminalExpressionUpdate *>(expr);
		if(proc_data.inside_anything_but_concatenation)
		{
			cout << "Error at ";
			expr_update.kw->print_location(cout);
			cout << ": update operator can only appear on the top level of expression.\n";
			return false;
		}
		else if(!proc_data.make_operator)
		{
			cout << "Invalid use of update operator at ";
			expr_update.kw->print_location(cout);
			cout << ": a previous use of create operator is required.\n";
			return false;
		}
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConnectUp))
	{
		NonterminalExpressionConnectUp &expr_up=*dynamic_cast<NonterminalExpressionConnectUp *>(expr);
		expr_up.used_after_make_operator=(proc_data.make_operator!=NULL);
		proc_data.nonterminal().requests_to_connect_up.push_back(&expr_up);
		WhaleData::data.variables.connect_up_operators_are_used=true;
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCode))
		return true;
	else if(typeid(*expr)==typeid(NonterminalExpressionPrecedence))
		return true;
	else
	{
		assert(false);
		return false; // to p. the c.
	}
}

bool process_expression_proc_ii(NonterminalExpression *expr, ProcessExpressionProcIIData &proc_data)
{
	if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
		bool result1=process_expression_proc_ii(expr_d.expr1, proc_data);
		bool result2=process_expression_proc_ii(expr_d.expr2, proc_data);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
		bool result1=process_expression_proc_ii(expr_c.expr1, proc_data);
		bool result2=process_expression_proc_ii(expr_c.expr2, proc_data);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
		bool result1=process_expression_proc_ii(expr_cat.expr1, proc_data);
		bool result2=process_expression_proc_ii(expr_cat.expr2, proc_data);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
	{
		NonterminalExpressionComplement &expr_com=*dynamic_cast<NonterminalExpressionComplement *>(expr);
		return process_expression_proc_ii(expr_com.expr, proc_data);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
		return process_expression_proc_ii(expr_om.expr, proc_data);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		return process_expression_proc_ii(expr_p.expr, proc_data);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
		
		proc_data.bodies_in_our_way.push_back(expr_it.body_sn);
		bool result=process_expression_proc_ii(expr_it.expr, proc_data);
		proc_data.bodies_in_our_way.pop_back();
		
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIterationPair))
	{
		NonterminalExpressionIterationPair &expr_it2=*dynamic_cast<NonterminalExpressionIterationPair *>(expr);
		
		proc_data.bodies_in_our_way.push_back(expr_it2.body1_sn);
		bool result1=process_expression_proc_ii(expr_it2.expr1, proc_data);
		proc_data.bodies_in_our_way.pop_back();
		
		proc_data.bodies_in_our_way.push_back(expr_it2.body2_sn);
		bool result2=process_expression_proc_ii(expr_it2.expr2, proc_data);
		proc_data.bodies_in_our_way.pop_back();
		
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
		return true;
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		Terminal *t=expr_s.symbol;
		
		/* checking validity of connect_up operators */
		if(expr_s.sn.is_nonterminal())
		{
			int nn=expr_s.sn.nonterminal_number();
			NonterminalData &nonterminal=WhaleData::data.nonterminals[nn];
			
			if(nonterminal.requests_to_connect_up.size() && !proc_data.make_operator)
				proc_data.nonterminals_that_cannot_be_connected_up[nn].push_back(&expr_s);
		}
		
		/* making WhaleData::data members in iteration bodies */
		
		int iteration_depth=proc_data.bodies_in_our_way.size();
		
	#ifdef DEBUG_DATA_MEMBERS_IN_BODIES
		cout << "process_expression_proc_ii(), at ";
		t->print_location(cout);
		if(iteration_depth)
		{
			cout << ": ";
			for(int i=0; i<proc_data.bodies_in_our_way.size(); i++)
				cout << (i ? ", " : "")
					<< WhaleData::data.full_symbol_name(proc_data.bodies_in_our_way[i], !i);
			cout << ".\n";
		}
		else
			cout << ".\n";
	#endif
		
		ClassHierarchy::DataMember *prototype_data_member;
		ClassHierarchy::Class *type_we_shall_use;
		string name_we_shall_use;
		if(expr_s.data_member_i)
		{
			prototype_data_member=expr_s.data_member_i;
			if(expr_s.data_member_i->internal_type_if_there_is_an_external_type)
				type_we_shall_use=expr_s.data_member_i->internal_type_if_there_is_an_external_type;
			else
				type_we_shall_use=expr_s.data_member_i->type;
			name_we_shall_use=expr_s.data_member_i->name;
		}
		else if(expr_s.data_member_e)
		{
			assert(expr_s.data_member_e->internal_type_if_there_is_an_external_type);
			prototype_data_member=expr_s.data_member_e;
			type_we_shall_use=expr_s.data_member_e->internal_type_if_there_is_an_external_type;
			name_we_shall_use=expr_s.data_member_e->name;
		}
		else
		{
			// it is 'nothing'.
			return true;
		}
		
		bool flag=true;
		for(int i=0; i<iteration_depth; i++)
		{
			SymbolNumber sn=proc_data.bodies_in_our_way[i];
			
		#ifdef DEBUG_DATA_MEMBERS_IN_BODIES
			cout << "\t" << WhaleData::data.full_symbol_name(sn, true) << ": ";
		#endif
			if(sn.is_nonterminal() && WhaleData::data.nonterminals[sn.nonterminal_number()].is_ancillary)
			{
				NonterminalData &nonterminal=WhaleData::data.nonterminals[sn.nonterminal_number()];
				assert(nonterminal.category==NonterminalData::BODY);
				
				if(proc_data.data_members_already_processed.count(make_pair(prototype_data_member, sn.nonterminal_number())))
				{
					bool flag=false;
					std::vector<ClassHierarchy::DataMember *> &v=proc_data.data_members_already_processed[make_pair(prototype_data_member, sn.nonterminal_number())]->data_members_in_bodies;
					for(int j=0; j<v.size(); j++)
						if(v[j]->belongs_to==nonterminal.type)
						{
						#ifdef DEBUG_DATA_MEMBERS_IN_BODIES
							cout << "reusing " << v[j]->get_full_name() << ".\n";
						#endif
							assert(!flag);
							expr_s.data_members_in_bodies.push_back(v[j]);
							flag=true;
						}
					assert(flag);
				}
				else
				{
					triad<ClassHierarchy::Class *, int, int> cnr=nonterminal.type->find_data_member_in_direct_relatives(name_we_shall_use.c_str());
					
					if(cnr.first)
					{
						// I think this can be replaced by
						// 'already_processed' WhaleData::data member
						// of type vector<DataMember *> in
						// ProcDataII.
						cout << "Unable to generate WhaleData::data member " << name_we_shall_use
							<< " in class " << nonterminal.type->get_full_name()
							<< " for " << WhaleData::data.full_symbol_name(expr_s.sn, false) << " at ";
						t->print_location(cout);
						cout << ", because identifier " << name_we_shall_use << " is already in use.\n";
						flag=false;
						continue;
					}
					
					ClassHierarchy::DataMember *new_data_member=new ClassHierarchy::DataMember;
					
					new_data_member->name=name_we_shall_use;
					new_data_member->was_implicitly_declared=prototype_data_member->was_implicitly_declared;
					new_data_member->declared_at=prototype_data_member->declared_at;
					new_data_member->number_of_nested_iterations=iteration_depth-i-1;
					new_data_member->type=type_we_shall_use;
					
					if(i>0)
					{
						new_data_member->senior=expr_s.data_members_in_bodies[i-1];
						new_data_member->senior_rests_in_a_body=true;
					}
					else
					{
						new_data_member->senior=prototype_data_member;
						new_data_member->senior_rests_in_a_body=false;
					}
					
				#ifdef DEBUG_DATA_MEMBERS_IN_BODIES
					cout << "\n\t\tmaking dm " <<  new_data_member->name << " in " << nonterminal.type->get_full_name()
						<< " (senior " << new_data_member->senior->get_full_name() << ").\n";
				#endif
					
					classes.put_data_member_to_class(new_data_member, nonterminal.type);
					expr_s.data_members_in_bodies.push_back(new_data_member);
					proc_data.data_members_already_processed[make_pair(prototype_data_member, sn.nonterminal_number())]=&expr_s;
				}
			}
			else
				assert(i==iteration_depth-1);
		}
		
		return flag;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionMemberName))
	{
		NonterminalExpressionMemberName &expr_mn=*dynamic_cast<NonterminalExpressionMemberName *>(expr);
		return process_expression_proc_ii(expr_mn.expr, proc_data);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCreate))
	{
		NonterminalExpressionCreate &expr_create=*dynamic_cast<NonterminalExpressionCreate *>(expr);
		proc_data.make_operator=&expr_create;
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionUpdate))
		return true;
	else if(typeid(*expr)==typeid(NonterminalExpressionConnectUp))
		return true;
	else if(typeid(*expr)==typeid(NonterminalExpressionCode))
		return true;
	else if(typeid(*expr)==typeid(NonterminalExpressionPrecedence))
		return true;
	else
	{
		assert(false);
		return false;
	}
}

bool process_terminal_type_declaration(TerminalData &terminal, NonterminalTypeExpression *type)
{
	if(type)
	{
		Terminal *t1=type->name;
		if(!identifier_contains_no_hyphens(t1))
			return false;
		Terminal *t2=type->base_name;
		ClassHierarchy::Class *previous_declaration=classes.find(t1->text);
		ClassHierarchy::Class *base_class=(t2 ? classes.find(t2->text) : classes.terminal);
		if(previous_declaration)
		{
			cout << "Class " << t1->text << ", ";
			previous_declaration->print_where_it_was_declared(cout);
			cout << ", cannot be used as a class for terminal " << terminal.name << " at ";
			t1->print_location(cout);
			cout << ".\n";
			return false;
		}
		
		if(!base_class)
		{
			report_undefined_base_class(cout, t2);
			return false;
		}
		
		if(!base_class->is_abstract || !base_class->is_derivative_of_terminal || base_class->is_a_built_in_template)
		{
			cout << "Class " << t2->text << ", ";
			base_class->print_where_it_was_declared(cout);
			cout << ", cannot be used as a base class for " << t1->text << " at ";
			t2->print_location(cout);
			cout << ": either Terminal or an abstract derivative of Terminal is required.\n";
			return false;
		}
		
		terminal.type=new ClassHierarchy::Class(t1->text, base_class, NULL);
		terminal.type->declared_at=t1;
	}
	else
	{
		string class_name=default_terminal_class_name(terminal.name);
		if(!class_name.size())
		{
			cout << "Unable to generate default class name for terminal " << terminal.name << " at ";
			terminal.declaration->print_location(cout);
			cout << ".\n";
			return false;
		}
		ClassHierarchy::Class *previous_declaration=classes.find(class_name.c_str());
		
		if(previous_declaration)
		{
			cout << "Unable to generate default class name for terminal " << terminal.name << " at ";
			terminal.declaration->print_location(cout);
			cout << ", because the name " << class_name << " is already in use";
			if(previous_declaration->declared_at)
			{
				cout << " (it was ";
				previous_declaration->print_where_it_was_declared(cout);
				cout << ")";
			}
			cout << ".\n";
			return false;
		}
		
		terminal.type=new ClassHierarchy::Class(class_name, classes.terminal, NULL);
		terminal.type->was_implicitly_declared=true;
		terminal.type->declared_at=terminal.declaration;
	}
	
	terminal.type->is_abstract=false;
	terminal.type->assigned_to=SymbolNumber::for_terminal(WhaleData::data.terminals.size());
	
	return true;
}

bool process_nonterminal_type_declaration(int nn, NonterminalTypeExpression *type)
{
	NonterminalData &nonterminal=WhaleData::data.nonterminals[nn];
	
	ClassHierarchy::Class *default_base_class;
	if(nonterminal.category==NonterminalData::EXTERNAL)
	{
		assert(("fix me!" && false));
		default_base_class=classes.wrapper;
	}
	else
		default_base_class=classes.nonterminal;
	
	if(type)
	{
		Terminal *t1=type->name;
		if(!identifier_contains_no_hyphens(t1))
			return false;
		Terminal *t2=type->base_name;
		ClassHierarchy::Class *previous_declaration=classes.find(t1->text);
		ClassHierarchy::Class *base_class=(t2 ? classes.find(t2->text) : default_base_class);
		if(previous_declaration)
		{
			cout << "Class " << t1->text << ", ";
			previous_declaration->print_where_it_was_declared(cout);
			cout << ", cannot be used as a class for nonterminal " << nonterminal.name << " at ";
			t1->print_location(cout);
			cout << ".\n";
			return false;
		}
		
		if(!base_class)
		{
			report_undefined_base_class(cout, t2);
			return false;
		}
		
		if(!base_class->is_abstract || !base_class->is_derivative_of_nonterminal || base_class->is_a_built_in_template)
		{
			cout << "Class " << t2->text << ", ";
			base_class->print_where_it_was_declared(cout);
			cout << ", cannot be used as a base class for " << t1->text << " at ";
			t2->print_location(cout);
			cout << ": either Nonterminal or an abstract derivative of Nonterminal is required.\n";
			return false;
		}
		
		nonterminal.type=new ClassHierarchy::Class(t1->text, base_class, NULL);
		nonterminal.type->declared_at=t1;
	}
	else
	{
		string class_name=default_nonterminal_class_name(nonterminal.name);
		assert(class_name.size());
		ClassHierarchy::Class *previous_declaration=classes.find(class_name.c_str());
		
		if(previous_declaration)
		{
			cout << "Unable to generate default class name for nonterminal " << nonterminal.name << " at ";
			nonterminal.declaration->print_location(cout);
			cout << ", because the name " << class_name << " is already in use";
			if(previous_declaration->declared_at)
			{
				cout << " (it was ";
				previous_declaration->print_where_it_was_declared(cout);
				cout << ")";
			}
			cout << ".\n";
			return false;
		}
		
		nonterminal.type=new ClassHierarchy::Class(class_name, default_base_class, NULL);
		nonterminal.type->was_implicitly_declared=true;
		nonterminal.type->declared_at=nonterminal.declaration;
	}
	
	nonterminal.type->is_abstract=false;
	nonterminal.type->assigned_to=SymbolNumber::for_nonterminal(nn);
	
	return true;
}

ClassHierarchy::Class *process_external_type_declaration(NonterminalExternalTypeExpression *external_type, NonterminalData *nonterminal)
{
	// if external_type!=NULL and nonterminal!=NULL:
	//	explicit declaration of an external type for a nonterminal:
	//	can appear both in 'nonterminal' and 'external' statements.
	// if external_type!=NULL and nonterminal==NULL:
	//	when does _that_ happen, I wonder?!
	// if external_type==NULL and nonterminal!=NULL:
	//	implicit declaration of an external type for a nonterminal:
	//	currently only from withing 'external' statement.
	assert(external_type!=NULL || nonterminal!=NULL);
	
	if(external_type!=NULL && nonterminal==NULL)
	{
		// remove me!
		
		cout << "Finally it happened!\n"
			"If you ever see this message printed, please report the fact to the author!\n";
		throw QuietException();
	}
	
	string s;
	
	if(external_type)
	{
		try
		{
			s=serialize_external_type_expression(*external_type);
		}
		catch(TerminalId *)
		{
			return NULL;
		}
	}
	else
	{
		cout << "process_external_type_declaration(): got NULL, using " << nonterminal->name << " as class name.\n";
		s=nonterminal->name;
	}
	
	ClassHierarchy::Class *previous_declaration=classes.find(s.c_str());
	
	if(!previous_declaration)
	{
		ClassHierarchy::Class *new_class=new ClassHierarchy::Class(s);
		if(external_type)
			new_class->declared_at=external_type->id;
		else
		{
			new_class->declared_at=nonterminal->declaration;
			new_class->was_implicitly_declared=true;
		}
		if(nonterminal)
			nonterminal->external_type=new_class;
		return new_class;
	}
	else if(!previous_declaration->is_internal)
	{
		if(nonterminal)
			nonterminal->external_type=previous_declaration;
		return previous_declaration;
	}
	else
	{
		cout << "Class " << s << ", ";
		previous_declaration->print_where_it_was_declared(cout);
		if(nonterminal)
			cout << ", cannot be used as an external class for nonterminal " << nonterminal->name << " at ";
		else
			cout << ", cannot be used as external class at ";
		external_type->id->print_location(cout);
		cout << ".\n";
		return NULL;
	}
}

bool process_alternative_type_declaration(AlternativeData &alternative, NonterminalData &nonterminal, int alternative_number, NonterminalTypeExpression *type)
{
	if(!nonterminal.type) return false;
	assert(nonterminal.type->nonterminal_class_category==ClassHierarchy::Class::BASE);
	
	if(type)
	{
		Terminal *t1=type->name;
		if(!identifier_contains_no_hyphens(t1))
			return false;
		Terminal *t2=type->base_name;
		ClassHierarchy::Class *previous_declaration=classes.find(t1->text);
		ClassHierarchy::Class *base_class=(t2 ? classes.find(t2->text) : nonterminal.type);
		if(previous_declaration)
		{
			cout << "Class " << t1->text << ", ";
			previous_declaration->print_where_it_was_declared(cout);
			cout << ", cannot be used as a class for the "
				<< english2_itoa(alternative_number+1, false) << " alternative ";
			cout << "of nonterminal " << nonterminal.name << " at ";
			t1->print_location(cout);
			cout << ".\n";
			return false;
		}
		
		if(!base_class)
		{
			report_undefined_base_class(cout, t2);
			return false;
		}
		
		// checking whether base_class is a derivative of nonterminal.type
		bool flag=false;
		for(ClassHierarchy::Class *m=base_class; m!=NULL; m=m->ancestor)
			if(m==nonterminal.type)
			{
				flag=true;
				break;
			}
		
		if(!flag || (base_class->nonterminal_class_category!=ClassHierarchy::Class::ABSTRACT
			&& base_class->nonterminal_class_category!=ClassHierarchy::Class::BASE))
		{
			cout << "Class " << t2->text << ", ";
			base_class->print_where_it_was_declared(cout);
			cout << ", cannot be used as a base class for " << t1->text << " at ";
			t2->print_location(cout);
			cout << ": either " << nonterminal.type->name << " or an abstract derivative of ";
			cout << nonterminal.type->name << " is required.\n";
			return false;
		}
		
		alternative.type=new ClassHierarchy::Class(t1->text, base_class, NULL);
		alternative.type->declared_at=t1;
	}
	else
	{
		string class_name=default_alternative_class_name(nonterminal.type->name, alternative_number);
		ClassHierarchy::Class *previous_declaration=classes.find(class_name.c_str());
		
		if(previous_declaration)
		{
			cout << "Unable to generate default class name for the ";
			cout << english2_itoa(alternative_number+1, false)
				<< " alternative of nonterminal "
				<< nonterminal.name << " at ";
			nonterminal.declaration->print_location(cout);
			cout << ", because the name " << class_name << " is already in use";
			if(previous_declaration->declared_at)
			{
				cout << " (it was ";
				previous_declaration->print_where_it_was_declared(cout);
				cout << ")";
			}
			cout << ".\n";
			return false;
		}
		
		alternative.type=new ClassHierarchy::Class(class_name, nonterminal.type, NULL);
		alternative.type->was_implicitly_declared=true;
		alternative.type->declared_at=alternative.declaration;
	}
	
	alternative.type->is_abstract=false;
	alternative.type->assigned_to=nonterminal.type->assigned_to;
	alternative.type->nonterminal_class_category=ClassHierarchy::Class::SPECIFIC;
	alternative.type->nonterminal_alternative_number=alternative_number;
	
	return true;
}

ClassHierarchy::Class *process_standalone_type_declaration(NonterminalTypeExpression *type)
{
	assert(type);
	
	if(!identifier_contains_no_hyphens(type->name))
		return NULL;
	
	ClassHierarchy::Class *previous_declaration=classes.find(type->name->text);
	if(previous_declaration)
	{
		cout << "Class " << previous_declaration->name << ", ";
		previous_declaration->print_where_it_was_declared(cout);
		cout << ", cannot be redeclared at ";
		type->name->print_location(cout);
		cout << ".\n";
		return NULL;
	}
	if(!type->base_name)
	{
		cout << "Class " << type->name->text << " requires an explicit base class at ";
		type->name->print_location(cout);
		cout << ".\n";
		return NULL;
	}
	
	ClassHierarchy::Class *base_class=classes.find(type->base_name->text);
	if(!base_class)
	{
		report_undefined_base_class(cout, type->base_name);
		return NULL;
	}
	
	if(!base_class->assigned_to && base_class->is_internal && !base_class->is_a_built_in_template)
	{
		ClassHierarchy::Class *m=new ClassHierarchy::Class(type->name->text, base_class, NULL);
		m->declared_at=type->name;
		m->is_abstract=true;
		return m;
	}
	else if(base_class->assigned_to.is_nonterminal() &&
		(base_class->nonterminal_class_category==ClassHierarchy::Class::UNKNOWN ||
		base_class->nonterminal_class_category==ClassHierarchy::Class::BASE ||
		base_class->nonterminal_class_category==ClassHierarchy::Class::ABSTRACT))
	{
//		NonterminalData &nonterminal=WhaleData::data.nonterminals[base_class->assigned_to.nonterminal_number()];
		if(base_class->nonterminal_class_category==ClassHierarchy::Class::UNKNOWN)
			base_class->nonterminal_class_category=ClassHierarchy::Class::BASE;
		ClassHierarchy::Class *m=new ClassHierarchy::Class(type->name->text, base_class, NULL);
		m->nonterminal_class_category=ClassHierarchy::Class::ABSTRACT;
		m->declared_at=type->name;
		m->assigned_to=base_class->assigned_to;
	//	m->is_abstract=false; // ???????????????????????????????
		m->is_abstract=true;
		return m;
	}
	else
	{
		cout << "Class " << base_class->name << ", ";
		base_class->print_where_it_was_declared(cout);
		cout << ", cannot be used as a base class for abstract class " << type->name->text << " at ";
		type->name->print_location(cout);
		cout << ".\n";
		return NULL;
	}
}

void print_a_number_of_terminal_locations(ostream &os, vector<Terminal *> &a, const char *word)
{
	for(int k=0; k<a.size(); k++)
	{
		if(k) os << (k<a.size()-1 ? ", " : " and ");
		os << word << " ";
		a[k]->print_location(os);
	}
}

// id -> TerminalId, "for" -> TerminalFor, ";" -> TerminalSemicolon etc.
string default_terminal_class_name(const char *s)
{
	static std::map<const char *, const char *, NullTerminatedStringCompare> database;
	if(database.begin()==database.end())
	{
//		cout << "default_terminal_class_name(): called for the first time, building database.\n";
		
		database["!"]="ExclamationMark";
		database["@"]="CommercialAt";
		database["#"]="Sharp";
		database["$"]="Dollar";
		database["%"]="Percent";
		database["^"]="Circumflex";
		database["&"]="Ampersand";
		database["*"]="Asterisk";
		database["("]="LeftParenthesis";
		database[")"]="RightParenthesis";
		database["["]="LeftBracket";
		database["]"]="RightBracket";
		database["{"]="LeftBrace";
		database["}"]="RightBrace";
		database["+"]="Plus";
		database["-"]="Minus";
		database["++"]="Increment";
		database["--"]="Decrement";
		database[":"]="Colon";
		database[";"]="Semicolon";
		database[","]="Comma";
		database["."]="Period";
		database["..."]="Ellipsis";
		database["<"]="LessThan";
		database[">"]="GreaterThan";
		database["<="]="LessOrEqual";
		database[">="]="GreaterOrEqual";
		database["="]="Equal";
		database["!="]="NotEqual";
		database[":="]="Assign";
		database["+="]="SumAssign";
		database["-="]="DifferenceAssign";
		database["*="]="ProductAssign";
		database["/="]="QuotientAssign";
		database["/"]="Slash";
		database["\\"]="Backslash";
		database["?"]="QuestionMark";
		database["\x22"]="QuotationMark";
		database["'"]="Apostrophe";
		database["_"]="Underscore";
		database["~"]="Tilde";
		database["->"]="Arrow";
	}
	
	if(s[0]=='"')
	{
		int l=strlen(s);
		assert(s[l-1]=='"');
		if(l==2)
			return string("");
		string contents(s+1, s+l-1);
		
		map<const char *, const char *, NullTerminatedStringCompare>::iterator p=database.find(contents.c_str());
		if(p!=database.end())
			return string("Terminal")+string((*p).second);
		
		for(string::iterator p=contents.begin(); p!=contents.end(); p++)
			if(!(isalpha(*p) || isdigit(*p) || *p=='_' || *p=='-'))
				return string("");
		
		return string("Terminal")+default_naming_proc(contents.c_str());
	}
	else
		return string("Terminal")+default_naming_proc(s);
}

string default_nonterminal_class_name(const char *s)
{
	return string("Nonterminal")+default_naming_proc(s);
}

string default_alternative_class_name(const string &s, int alternative_number)
{
	char c=s.back();
	if((c>='a' && c<='z') || c=='_')
		return s+roman_itoa(alternative_number+1, true);
	else
		return s+string("_")+roman_itoa(alternative_number+1, true);
}

string default_naming_proc(const char *s)
{
	string result;
	bool capital_flag=true;
	for(int i=0; s[i]; i++)
		if((s[i]>='A' && s[i]<='Z') || isdigit(s[i]))
		{
			result+=s[i];
			capital_flag=false;
		}
		else if(s[i]>='a' && s[i]<='z')
		{
			result+=char(s[i]-(capital_flag ? 'a'-'A' : 0));
			capital_flag=false;
		}
		else if(s[i]=='_' || s[i]=='-')
			capital_flag=true;
	return result;
}

void report_undefined_base_class(ostream &os, Terminal *t)
{
	assert(t);
	os << "Undefined base class " << t->text << " at ";
	t->print_location(os);
	os << ".\n";
}

bool identifier_contains_no_hyphens(Terminal *t)
{
	for(int i=0; t->text[i]; i++)
		if(t->text[i]=='-')
		{
			cout << "Identifier at ";
			t->print_location(cout);
			cout << " may not contain hyphens.\n";
			return false;
		}
	return true;
}

struct ClassNamePrinter
{
	ostream &operator ()(ostream &os, const ClassHierarchy::Class *c)
	{
		return os << c->name;
	}
};

ClassHierarchy::Class *process_scope_in_member_name_specification(Terminal *scope, Terminal *id, const pair<int, int> &location)
{
	ClassHierarchy::Class *class_assigned_to_host_nonterminal=WhaleData::data.get_class_assigned_to_alternative(location.first, location.second);
	assert(class_assigned_to_host_nonterminal);
	
	if(!scope) return class_assigned_to_host_nonterminal;
	
	bool strange_coincidence=!strcmp(scope->text, WhaleData::data.nonterminals[location.first].name);
	
	ClassHierarchy::Class *this_class;
	
	if(strange_coincidence)
		this_class=WhaleData::data.nonterminals[location.first].type;
	else
		this_class=classes.find(scope->text);
	
	if(!this_class)
	{
		cout << "Undefined class " << scope->text << " at ";
		scope->print_location(cout);
		cout << ".\n";
		return NULL;
	}
	
	// this_class should be an ancestor of class_assigned_to_host_nonterminal.
	bool found=false;
	for(ClassHierarchy::Class *c=class_assigned_to_host_nonterminal; c; c=c->ancestor)
		if(c==this_class)
		{
			found=true;
			break;
		}
	
	if(found)
		return this_class;
	else
	{
		vector<ClassHierarchy::Class *> v;
		for(ClassHierarchy::Class *c=class_assigned_to_host_nonterminal; c; c=c->ancestor)
			v.push_back(c);
		cout << "Class " << this_class->name << " cannot be used to store variable ";
		cout << id->text << " at ";
		scope->print_location(cout);
		cout << ": expecting ";
		custom_print(cout, v.begin(), v.end(), ClassNamePrinter(), ", ", " or ");
		cout << ".\n";
		return NULL;
	}
}

ClassHierarchy::DataMember *process_single_data_member_declaration(Terminal *scope, Terminal *name, int nn, int an, int number_of_iterations)
{
	ClassHierarchy::Class *scope_class=process_scope_in_member_name_specification(scope, name, make_pair(nn, an));
	if(!scope_class) return NULL;
	
	assert(name);
	char *s=name->text;
	
	triad<ClassHierarchy::Class *, int, int> cnr=scope_class->find_data_member_in_direct_relatives(s);
	if(!cnr.first)
	{
		// creating a new WhaleData::data member.
		ClassHierarchy::DataMember *new_data_member=new ClassHierarchy::DataMember;
		
		new_data_member->name=s;
		new_data_member->type=NULL;
		new_data_member->was_implicitly_declared=false;
		new_data_member->declared_at=name;
		new_data_member->internal_type_if_there_is_an_external_type=NULL;
		new_data_member->number_of_nested_iterations=number_of_iterations;
		
		classes.put_data_member_to_class(new_data_member, scope_class);
		return new_data_member;
	}
	else if(!cnr.third || !scope)
	{
		// attempting to use an existing WhaleData::data member.
		ClassHierarchy::DataMember *data_member=cnr.first->data_members[cnr.second];
		
		if(data_member->better_not_to_use_it_for_ordinary_symbols)
		{
			cout << "Warning: usage of built-in WhaleData::data member "
				<< data_member->name << " at ";
			name->print_location(cout);
			cout << " would most probably lead to parser malfunction.\n";
		}
		
	#if 0
		if(data_member->number_of_nested_iterations!=number_of_iterations)
		{
		
			cout << "Data member ";
			cout << data_member->get_full_name() << ", ";
			data_member->print_where_it_was_declared(cout);
			cout << ", cannot be used at ";
			name->print_location(cout);
			cout << ", because " << data_member->number_of_nested_iterations << ", " << number_of_iterations << "\n";
			return NULL;
		}
		else
	#endif
		data_member->number_of_nested_iterations=max(data_member->number_of_nested_iterations, number_of_iterations);
		return data_member;
	}
	else
	{
		cout << "Unable to create WhaleData::data member ";
		cout << scope_class->name << "::" << s;
		cout << " at ";
		name->print_location(cout);
		cout << ", because a there is a member bearing the same name (";
		cnr.first->data_members[cnr.second]->print_where_it_was_declared(cout);
		cout << ") in class " << cnr.first->name << ", which is ";
		if(cnr.third==-1)
			cout << "an ancestor of ";
		else
			cout << "a descendant of ";
		cout << scope_class->name << ".\n";
		return NULL;
	}
}

string generate_default_data_member_name(int n)
{
	char s[20];
	snprintf(s, 20, "a%u", n);
	return string(s);
}

string serialize_external_type_expression(NonterminalExternalTypeExpression &expr)
{
	string result;
	
	if(typeid(*expr.id)==typeid(TerminalId))
	{
		if(!identifier_contains_no_hyphens(expr.id))
			throw expr.id;
		result=expr.id->text;
	}
	else if(typeid(*expr.id)==typeid(TerminalString))
		result=string(expr.id->text+1, expr.id->text+strlen(expr.id->text)-1);
	else
		assert(false);
	
	if(expr.template_arguments.size())
	{
		result+="<";
		
		for(int i=0; i<expr.template_arguments.size(); i++)
		{
			if(i) result+=", ";
			result+=serialize_external_type_expression(*expr.template_arguments[i]);
		}
		
		if(result.back()!='>')
			result+=">";
		else
			result+=" >";
	}
	
	if(typeid(*expr.expr1)==typeid(NonterminalExternalTypeExpressionOneI))
	{
		NonterminalExternalTypeExpressionOneI &expr_i=*dynamic_cast<NonterminalExternalTypeExpressionOneI *>(expr.expr1);
		if(expr_i.asterisks.size() || expr_i.ampersand)
			result+=" ";
		for(int i=0; i<expr_i.asterisks.size(); i++)
			result+="*";
		if(expr_i.ampersand)
			result+="&";
	}
	else if(typeid(*expr.expr1)==typeid(NonterminalExternalTypeExpressionOneII))
	{
		NonterminalExternalTypeExpressionOneII &expr_ii=*dynamic_cast<NonterminalExternalTypeExpressionOneII *>(expr.expr1);
		result+="::";
		result+=serialize_external_type_expression(*expr_ii.expr);
	}
	else
		assert(false);
	
	return result;
}

bool apply_common_ancestor_determination_procedure(ClassHierarchy::Class *&dest, ClassHierarchy::Class *src, SymbolNumber sn, ClassHierarchy::DataMember *data_member, Terminal *t)
{
#ifdef DEBUG_COMMON_ANCESTOR
	cout << "apply_common_ancestor_determination_procedure(): ";
	if(dest)
		cout << dest->get_full_name();
	else
		cout << "NULL";
	cout << ", ";
	if(src)
		cout << src->get_full_name();
	else
		cout << "NULL";
	cout << "\n";
#endif
	
	assert(src);
	if(!dest)
	{
		dest=src;
		return true;
	}
	
	ClassHierarchy::Class *result=classes.common_ancestor(dest, src);
	
	if(result)
	{
		dest=result;
		return true;
	}
	else
	{
		cout << WhaleData::data.full_symbol_name(sn, true);
		cout << " cannot be stored in WhaleData::data member ";
		cout << data_member->get_full_name() << " at ";
		t->print_location(cout);
		cout << ", because class " << dest->name << ", ";
		dest->print_where_it_was_declared(cout);
		cout << ", and class " << src->name << ", ";
		src->print_where_it_was_declared(cout);
		cout << ", have no common ancestors.\n";
		return false;
	}
}

bool process_option_statement(NonterminalOptionStatement &option_statement)
{
	const char *s=option_statement.left->text;
	
	Variables::Properties properties=variable_properties(s);
	bool process_escape_sequences_in_strings=((properties & Variables::PROCESS_ESCAPE_SEQUENCES_IN_STRING)!=0);
	
	map<const char *, vector<AssignmentData>, NullTerminatedStringCompare>::iterator p=WhaleData::data.assignments.find(s);
	if(!(properties & Variables::MULTIPLE_ASSIGNMENTS_ALLOWED) && p!=WhaleData::data.assignments.end())
	{
		cout << "Duplicate assignment to variable '" << s << "' at ";
		option_statement.left->print_location(cout);
		cout << " (earlier assignment at ";
		p->second[0].declaration->left->print_location(cout);
		cout << ").\n";
		return false;
	}
	
	if(properties==Variables::DOES_NOT_EXIST)
	{
		cout << "Warning: assignment to unknown variable '" << s << "' at ";
		option_statement.left->print_location(cout);
		cout << " will probably have no effect.\n";
	}
	
	AssignmentData ad;
	ad.declaration=&option_statement;
	
	bool flag=true;
	
	if(option_statement.middle.size()>0 && !(properties & Variables::PARAMETER_ALLOWED))
	{
		cout << "Parameters are not allowed in assignment to variable '" << s;
		cout << "' at ";
		option_statement.left->print_location(cout);
		cout << ".\n";
		flag=false;
	}
	else if(option_statement.middle.size()>1 && !(properties & Variables::MULTIPLE_PARAMETERS_ALLOWED))
	{
		cout << "Too many parameters in assignment to variable '" << s;
		cout << "' at ";
		option_statement.left->print_location(cout);
		cout << ".\n";
		flag=false;
	}
	
	for(int j=0; j<option_statement.middle.size(); j++)
	{
		pair<char *, AssignmentData::ValueType> result=process_single_value_in_option_statement(option_statement.middle[j], process_escape_sequences_in_strings);
		
		if(result.first!=NULL)
			ad.parameters.push_back(result);
		else
			flag=false;
	}
	
	if((properties & Variables::SINGLE_ASSIGNMENT_FOR_SINGLE_PARAMETER_LIST) && p!=WhaleData::data.assignments.end())
	{
		for(int i=0; i<p->second.size(); i++)
		{
			AssignmentData &prev_ad=p->second[i];
			if(prev_ad.parameters.size()==ad.parameters.size())
			{
				bool all_equal=true;
				for(int j=0; j<ad.parameters.size(); j++)
					if(strcmp(prev_ad.parameters[j].first, ad.parameters[j].first) ||
						prev_ad.parameters[j].second!=ad.parameters[j].second)
					{
						all_equal=false;
						break;
					}
				if(all_equal)
				{
					cout << "Duplicate assignment to variable '" << s << "' at ";
					option_statement.left->print_location(cout);
					cout << " (earlier assignment with the same parameter list at ";
					prev_ad.declaration->left->print_location(cout);
					cout << ").\n";
					return false;
				}
			}
		}
	}
	
	if(properties & Variables::SINGLE_ASSIGNMENT_FOR_SINGLE_PARAMETER)
	{
		assert(false);
	}
	
	if(option_statement.right.size()>1 && !(properties & Variables::MULTIPLE_VALUES_ALLOWED))
	{
		cout << "Too many values in assignment to variable '" << s;
		cout << "' at ";
		option_statement.left->print_location(cout);
		cout << ".\n";
		flag=false;
	}
	
	for(int j=0; j<option_statement.right.size(); j++)
	{
		pair<char *, AssignmentData::ValueType> result=process_single_value_in_option_statement(option_statement.right[j], process_escape_sequences_in_strings);
		
		if(result.first!=NULL)
			ad.values.push_back(result);
		else
			flag=false;
	}
	
	WhaleData::data.assignments[s].push_back(ad);
	
	return flag;
}

// returns (NULL, undefined) upon any error.
pair<char *, AssignmentData::ValueType> process_single_value_in_option_statement(Terminal *t, bool process_escape_sequences_in_string)
{
	char *s1=t->text;
	char *final_s;
	AssignmentData::ValueType vt;
	if(!strcmp(s1, "true") || !strcmp(s1, "yes") || !strcmp(s1, "on"))
	{
		vt=AssignmentData::VALUE_TRUE;
		final_s=s1;
	}
	else if(!strcmp(s1, "false") || !strcmp(s1, "no") || !strcmp(s1, "off"))
	{
		vt=AssignmentData::VALUE_FALSE;
		final_s=s1;
	}
	else if(typeid(*t)==typeid(Whale::TerminalId))
	{
		vt=AssignmentData::VALUE_ID;
		final_s=s1;
	}
	else if(typeid(*t)==typeid(Whale::TerminalNumber) ||
		typeid(*t)==typeid(Whale::TerminalHexNumber))
	{
		vt=AssignmentData::VALUE_NUMBER;
		final_s=s1;
	}
	else if(typeid(*t)==typeid(Whale::TerminalString))
	{
		vt=AssignmentData::VALUE_STRING;
		if(process_escape_sequences_in_string)
		{
			string str;
			if(!high_level_decode_escape_sequences(t, str))
				return make_pair((char *)NULL, vt);
			final_s=strdup(str.c_str());
		}
		else
			final_s=s1;
	}
	else if(typeid(*t)==typeid(Whale::TerminalCode))
	{
		vt=AssignmentData::VALUE_CODE;
		const char *s=t->text;
		int k=(s[0]=='{' ? 1 : 2);
		string str(s+k, s+strlen(s)-k);
		final_s=strdup(str.c_str());
	}
	else
		assert(false);
	
	return make_pair(final_s, vt);
}

bool high_level_decode_escape_sequences(Terminal *t, string &s)
{
	vector<int> v;
	bool result=decode_escape_sequences(t->text, v);
	if(!result)
	{
		cout << "Ill-formed escape sequences in string at ";
		t->print_location(cout);
		cout << ".\n";
		return false;
	}
	bool flag=true;
	for(int k=0; k<v.size(); k++)
		if(!(v[k]>0 && v[k]<256))
		{
			cout << "Out-of-range characters in string at ";
			t->print_location(cout);
			cout << ".\n";
			flag=false;
		}
	if(!flag)
		return false;
	
	s.erase(s.begin(), s.end()); // s.clear();
	for(int k=0; k<v.size(); k++)
		s+=char(v[k]);
	return true;
}

void make_up_data_member_in_class_symbol()
{
	ClassHierarchy::DataMember *up_data_member=new ClassHierarchy::DataMember;
	
	up_data_member->name="up";
	up_data_member->type=classes.nonterminal;
	up_data_member->better_not_to_use_it_for_ordinary_symbols=true;
	classes.put_data_member_to_class(up_data_member, classes.symbol);
}
