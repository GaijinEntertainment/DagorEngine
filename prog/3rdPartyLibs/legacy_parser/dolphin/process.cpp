
#include "dolphin.h"

#include <stdio.h>
#include <iostream>
#include <typeinfo>
using namespace std;

#include "assert.h"
#include "process.h"
#include "matrix.h"
#include "serialize.h"
#include "stl.h"
using namespace Whale;

//#define EXTRACT_FIRST_AND_LAST_TERMINAL_FUNCTIONS
//#define DEBUG_DERIVATION_PATHS_MATRIX
//#define PRINT_SERIALIZED_EXPRESSIONS

bool process_expression_proc(NonterminalExpression *expr, int nn, bool flag);
bool process_expression_proc(NonterminalExpressionS *expr, int nn, bool flag);
bool process_expression_proc(NonterminalExpressionC *expr, int nn);
bool check_whether_terminal_symbol_is_in_range(int tn, Terminal *location);
bool check_whether_terminal_symbol_is_in_range(vector<int> &v, Terminal *location);
bool decode_escape_sequences(const char *s, vector<int> &v);
bool get_character_from_the_string_that_is_expected_to_be_exactly_one_character_long(NonterminalExpressionS *expr_s, int nn, int &result);

void print_a_number_of_terminal_locations(ostream &os, vector<Terminal *> &a, const char *word)
{
	for(int k=0; k<a.size(); k++)
	{
		if(k) os << (k<a.size()-1 ? ", " : " and ");
		os << word << " ";
		print_terminal_location(os, a[k]);
	}
}

void print_derivation_path(ostream &os, PathData &path, int initial,
	PathData::TransitionType bracket_brace_threshold=PathData::IN_EXPRESSION)
{
	int n=path.v.size();
	os << DolphinData::data.nonterminals[initial].name;
	for(int i=0; i<n; i++)
	{
		os << " ";
		os << (path.v[i].second<=bracket_brace_threshold ? "[" : "{");
		print_terminal_location(os, path.v[i].first);
		os << (path.v[i].second<=bracket_brace_threshold ? "]" : "}");
		os << " " << path.v[i].first->text;
	}
}

void print_a_number_of_expressions(ostream &os, set<int> &expressions)
{
	int pos=0;
	for(set<int>::iterator p=expressions.begin(); p!=expressions.end(); p++)
	{
		if(pos)
		{
			if(pos<expressions.size()-1)
				os << ", ";
			else
				os << " and ";
		}
		pos++;

		os << DolphinData::data.recognized_expressions[*p].in_serialized_form;
	}
}

void print_a_number_of_start_conditions(ostream &os, set<int> &start_conditions)
{
	int pos=0;
	for(set<int>::iterator p=start_conditions.begin(); p!=start_conditions.end(); p++)
	{
		if(pos)
		{
			if(pos<start_conditions.size()-1)
				os << ", ";
			else
				os << " and ";
		}
		pos++;
		
		os << DolphinData::data.start_conditions[*p].name;
	}
}

#ifdef EXTRACT_FIRST_AND_LAST_TERMINAL_FUNCTIONS
Terminal *extract_first_terminal(Symbol *s)
{
	vector<Terminal *> v;
	s->extract_terminals(v);
	if(v.size())
		return v[0];
	else
		return NULL;
}

Terminal *extract_last_terminal(Symbol *s)
{
	vector<Terminal *> v;
	s->extract_terminals(v);
	if(v.size())
		return v.back();
	else
		return NULL;
}
#endif

void print_derivation_paths()
{
	int n=DolphinData::data.nonterminals.size();
	for(int i=0; i<n; i++)
		for(int j=0; j<n; j++)
		{
			PathData &m_ij=DolphinData::data.derivation_paths[i][j];
			if(m_ij.v.size())
			{
				cout << "m[" << i << "][" << j << "] (" << m_ij.worst << ") = ";
				print_derivation_path(cout, m_ij, i);
				cout << "\n";
			}
		}
}

// expr points to a tree containing a subexpression.
// nn is the number of nonterminal in the left side of the rule that contains
// expr as a subexpression. If we are processing an expression that belongs to
//   a recognized expression, not a grammar rule, than nn==-1.
// flag==true means that a rightmost recursion may take place through this
//   subexpression.
// returns true if no errors were found, false otherwise.
bool process_expression_proc(NonterminalExpression *expr, int nn, bool flag)
{
	if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
		bool result1=process_expression_proc(expr_d.expr1, nn, flag);
		bool result2=process_expression_proc(expr_d.expr2, nn, flag);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
		bool result1=process_expression_proc(expr_c.expr1, nn, false);
		bool result2=process_expression_proc(expr_c.expr2, nn, false);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
		bool result1=process_expression_proc(expr_cat.expr1, nn, false);
		bool result2=process_expression_proc(expr_cat.expr2, nn, flag);
		if(nn>=0)
			DolphinData::data.nonterminals[nn].can_be_used_in_IN_expressions=false;
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
	{
		NonterminalExpressionComplement &expr_com=*dynamic_cast<NonterminalExpressionComplement *>(expr);
		bool result=process_expression_proc(expr_com.expr, nn, false);
		if(nn>=0)
			DolphinData::data.nonterminals[nn].can_be_used_in_IN_expressions=false;
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
		bool result=process_expression_proc(expr_om.expr, nn, flag);
		if(nn>=0)
			DolphinData::data.nonterminals[nn].can_be_used_in_IN_expressions=false;
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		bool result=process_expression_proc(expr_p.expr, nn, flag);
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
		bool result=process_expression_proc(expr_it.expr, nn, false);
		if(nn>=0)
			DolphinData::data.nonterminals[nn].can_be_used_in_IN_expressions=false;
		if(!strcmp(expr_it.sign->text, "*"))
			expr_it.reflexive=true;
		else if(!strcmp(expr_it.sign->text, "+"))
			expr_it.reflexive=false;
		else
			assert(false);
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCondition))
	{
		NonterminalExpressionCondition &expr_c=*dynamic_cast<NonterminalExpressionCondition *>(expr);
		bool result=process_expression_proc(expr_c.condition, nn);
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionRange))
	{
		NonterminalExpressionRange &expr_r=*dynamic_cast<NonterminalExpressionRange *>(expr);
		bool result1=get_character_from_the_string_that_is_expected_to_be_exactly_one_character_long(expr_r.first_expr, nn, expr_r.first);
		bool result2=get_character_from_the_string_that_is_expected_to_be_exactly_one_character_long(expr_r.last_expr, nn, expr_r.last);
		if(result1 && result2)
		{
			if(expr_r.first>expr_r.last)
			{
				cout << "Invalid 'range' expression at ";
				print_terminal_location(cout, expr_r.range_kw);
				cout << ": symbol at ";
				print_terminal_location(cout, expr_r.first_expr->symbol);
				cout << " is greater than symbol at ";
				print_terminal_location(cout, expr_r.last_expr->symbol);
				cout << ".\n";
				return false;
			}
			else
			{
			//	***** REMOVE IT: *****
			//	for(int j=expr_r.first; j<=expr_r.last; j++)
			//		DolphinData::data.charset.new_symbol(j);
				
				return true;
			}
		}
		else
			return false;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionContains))
	{
		NonterminalExpressionContains &expr_c=*dynamic_cast<NonterminalExpressionContains *>(expr);
		bool result=process_expression_proc(expr_c.expr, nn, false);
		if(nn>=0)
			DolphinData::data.nonterminals[nn].can_be_used_in_IN_expressions=false;
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
	{
//		NonterminalExpressionEpsilon &expr_eps=*dynamic_cast<NonterminalExpressionEpsilon *>(expr);
		if(nn>=0)
			DolphinData::data.nonterminals[nn].can_be_used_in_IN_expressions=false;
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionSharpSign))
		return true;
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		bool result=process_expression_proc(expr_s.expr, nn, flag);
		return result;
	}
	else
	{
		assert(false);
		return false; // to please the compiler
	}
}

bool process_expression_proc(NonterminalExpressionS *expr_s, int nn, bool flag)
{
	Terminal *t=expr_s->symbol;
	const char *s=t->text;
	if(typeid(*t)==typeid(TerminalId))
	{
		int nn2=DolphinData::data.find_nonterminal(s);
		expr_s->is_nts=true;
		expr_s->nn=nn2;
		if(nn2==-1)
		{
			cout << "Reference to undefined nonterminal '" << s << "' at ";
			print_terminal_location(cout, t);
			cout << ".\n";
			return false;
		}
		
		if(nn>=0)
		{
			PathData &path=DolphinData::data.derivation_paths[nn][nn2];
			if(!path.v.size())
			{
			#ifdef DEBUG_DERIVATION_PATHS_MATRIX
				cout << "creating\n";
			#endif
				path.worst=(flag ? PathData::RIGHT : PathData::NORMAL);
				path.v.push_back(make_pair(t, path.worst));
			}
			else if(path.v.size() && path.worst==PathData::RIGHT && !flag)
			{
			#ifdef DEBUG_DERIVATION_PATHS_MATRIX
				cout << "replacing\n";
			#endif
				path.v.clear();
				path.worst=PathData::NORMAL;
				path.v.push_back(make_pair(t, path.worst));
			}
		#ifdef DEBUG_DERIVATION_PATHS_MATRIX
			else
				cout << "ignoring\n";
		#endif
		}
		
		return true;
	}
	else if(typeid(*t)==typeid(TerminalString))
	{
		expr_s->is_nts=false;
		bool result=decode_escape_sequences(s, expr_s->s);
		if(!result)
		{
			cout << "Ill-formed escape sequences in string at ";
			print_terminal_location(cout, t);
			cout << ".\n";
		}
		else
		{
			if(nn>=0)
			{
				if(expr_s->s.size()!=1)
					DolphinData::data.nonterminals[nn].can_be_used_in_IN_expressions=false;
			}
		}
		result=(check_whether_terminal_symbol_is_in_range(expr_s->s, t) && result);
		return result;
	}
	else if(typeid(*t)==typeid(TerminalNumber))
	{
		int tn=atoi(s);
		
		expr_s->is_nts=false;
		expr_s->s.push_back(tn);
		
		return check_whether_terminal_symbol_is_in_range(tn, t);
	}
	else if(typeid(*t)==typeid(TerminalHexNumber))
	{
		assert(s[0]=='0' && tolower(s[1])=='x' && s[2]);
		int tn;
		sscanf(s+2, "%x", &tn);
		
		expr_s->is_nts=false;
		expr_s->s.push_back(tn);
		
		return check_whether_terminal_symbol_is_in_range(tn, t);
	}
	else
	{
		assert(false);
		return false; // to please the compiler
	}
}

bool process_expression_proc(NonterminalExpressionC *expr, int nn)
{
	if(typeid(*expr)==typeid(NonterminalExpressionC_Disjunction))
	{
		NonterminalExpressionC_Disjunction &expr_d=*dynamic_cast<NonterminalExpressionC_Disjunction *>(expr);
		bool result1=process_expression_proc(expr_d.expr1, nn);
		bool result2=process_expression_proc(expr_d.expr2, nn);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Conjunction))
	{
		NonterminalExpressionC_Conjunction &expr_c=*dynamic_cast<NonterminalExpressionC_Conjunction *>(expr);
		bool result1=process_expression_proc(expr_c.expr1, nn);
		bool result2=process_expression_proc(expr_c.expr2, nn);
		return result1 && result2;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Complement))
	{
		NonterminalExpressionC_Complement &expr_com=*dynamic_cast<NonterminalExpressionC_Complement *>(expr);
		bool result=process_expression_proc(expr_com.expr, nn);
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_InParentheses))
	{
		NonterminalExpressionC_InParentheses &expr_p=*dynamic_cast<NonterminalExpressionC_InParentheses *>(expr);
		bool result=process_expression_proc(expr_p.expr, nn);
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Comparison))
	{
		NonterminalExpressionC_Comparison &expr_cmp=*dynamic_cast<NonterminalExpressionC_Comparison *>(expr);
		
		NonterminalExpressionS &left_symbol=*dynamic_cast<NonterminalExpressionS *>(expr_cmp.left);
		Terminal *left_location=left_symbol.symbol;
		bool left_is_c=!strcmp(left_location->text, "c");
		
		NonterminalExpressionS &right_symbol=*dynamic_cast<NonterminalExpressionS *>(expr_cmp.right);
		Terminal *right_location=right_symbol.symbol;
		bool right_is_c=!strcmp(right_location->text, "c");
		
		if(left_is_c && right_is_c)
		{
			cout << "Expressions at ";
			print_terminal_location(cout, left_location);
			cout << " and at ";
			print_terminal_location(cout, right_location);
			cout << " cannot both be 'c'.\n";
			return false;
		}
		else if(!left_is_c && !right_is_c)
		{
			cout << "Either the expression at ";
			print_terminal_location(cout, left_location);
			cout << ", or the expression at ";
			print_terminal_location(cout, right_location);
			cout << " should be 'c'.\n";
			return false;
		}
		
		if(!strcmp(expr_cmp.comparison_operator->text, "=="))
			expr_cmp.actual_operation=NonterminalExpressionC_Comparison::EQ;
		else if(!strcmp(expr_cmp.comparison_operator->text, "!="))
			expr_cmp.actual_operation=NonterminalExpressionC_Comparison::NE;
		else if(!strcmp(expr_cmp.comparison_operator->text, "<"))
			expr_cmp.actual_operation=NonterminalExpressionC_Comparison::LT;
		else if(!strcmp(expr_cmp.comparison_operator->text, "<="))
			expr_cmp.actual_operation=NonterminalExpressionC_Comparison::LE;
		else if(!strcmp(expr_cmp.comparison_operator->text, ">"))
			expr_cmp.actual_operation=NonterminalExpressionC_Comparison::GT;
		else if(!strcmp(expr_cmp.comparison_operator->text, ">="))
			expr_cmp.actual_operation=NonterminalExpressionC_Comparison::GE;
		else
			assert(false);
		
		if(left_is_c)
			return get_character_from_the_string_that_is_expected_to_be_exactly_one_character_long(&right_symbol, nn, expr_cmp.symbol);
		else if(right_is_c)
		{
			expr_cmp.actual_operation=NonterminalExpressionC_Comparison::swap_operands(expr_cmp.actual_operation);
			return get_character_from_the_string_that_is_expected_to_be_exactly_one_character_long(&left_symbol, nn, expr_cmp.symbol);
		}
		else
		{
			assert(false);
			return false;
		}
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_In))
	{
		NonterminalExpressionC_In &expr_in=*dynamic_cast<NonterminalExpressionC_In *>(expr);
		if(strcmp(expr_in.c->symbol->text, "c"))
		{
			cout << "Expecting 'c' at ";
			print_terminal_location(cout, expr_in.c->symbol);
			cout << ".\n";
			return false;
		}
		expr_in.nn=DolphinData::data.find_nonterminal(expr_in.symbol->text);
		if(expr_in.nn==-1)
		{
			cout << "Reference to undefined nonterminal '" << expr_in.symbol->text << "' at ";
			print_terminal_location(cout, expr_in.symbol);
			cout << ".\n";
			return false;
		}
		NonterminalData &nonterminal=DolphinData::data.nonterminals[expr_in.nn];
		
		nonterminal.is_used_in_IN_expressions=true;
		nonterminal.where_it_is_used_in_IN_expressions.push_back(expr_in.symbol);
		
		if(nn>=0)
		{
			PathData &path=DolphinData::data.derivation_paths[nn][expr_in.nn];
			if(!path.v.size() || path.worst!=PathData::IN_EXPRESSION)
			{
				path.v.clear();
				path.v.push_back(make_pair(expr_in.symbol, PathData::IN_EXPRESSION));
				path.worst=PathData::IN_EXPRESSION;
			}
		}
		
		return true;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Constant))
	{
		NonterminalExpressionC_Constant &expr_const=*dynamic_cast<NonterminalExpressionC_Constant *>(expr);
		if(!strcmp(expr_const.true_or_false->text, "true"))
			expr_const.value=true;
		else if(!strcmp(expr_const.true_or_false->text, "false"))
			expr_const.value=false;
		else
			assert(false);
		return true;
	}
	else
	{
		assert(false);
		return false; // to please the compiler
	}
}

bool check_whether_terminal_symbol_is_in_range(int tn, Terminal *location)
{
	if(tn>=0 && tn<DolphinData::data.variables.alphabet_cardinality)
		return true;
	else
	{
		cout << "Out-of-range symbol " << tn << " at ";
		print_terminal_location(cout, location);
		cout << ".\n";
		return false;
	}
}

bool check_whether_terminal_symbol_is_in_range(vector<int> &v, Terminal *location)
{
	vector<int> v2;
	for(int i=0; i<v.size(); i++)
		if(!(v[i]>=0 && v[i]<DolphinData::data.variables.alphabet_cardinality))
			v2.push_back(v[i]);
	if(v2.size())
	{
		cout << "Out-of-range symbol" << (v2.size()==1 ? " " : "s ");
		for(int i=0; i<v2.size(); i++)
		{
			if(i)
			{
				if(i<v2.size()-1)
					cout << ", ";
				else
					cout << " and ";
			}
			cout << v2[i];
		}
		cout << " in string at ";
		print_terminal_location(cout, location);
		cout << ".\n";
		return false;
	}
	return true;
}

inline bool is_octal_digit(char c) { return c>='0' && c<='7'; }

bool decode_escape_sequences(const char *str, vector<int> &v)
{
	static const char *escape_sequences="n\nt\tv\vb\br\rf\fa\a\\\\??\'\'\"\"";
	string s_ = str;
	char *s = &s_[0];
	
	const int length=s_.length();
	assert(length>=2 && (s[0]=='\x27' || s[0]=='\x22') && s[0]==s[length-1]);
	s[length-1]=0;
	
	for(int i=1; s[i]; i++)
		if(s[i]!='\\')
			v.push_back(s[i]);
		else
		{
			i++;
			if(!s[i])
				return false;
			
			bool flag=false;
			for(int j=0; escape_sequences[j]; j+=2)
				if(s[i]==escape_sequences[j])
				{
					v.push_back(escape_sequences[j+1]);
					flag=true;
					break;
				}
			if(!flag)
			{
				if(s[i]=='x' || s[i]=='u' || s[i]=='U')
				{
					i++;
					int j;
					for(j=0; isxdigit(s[i+j]); j++);
					if(s[i-1]=='x')
					{
						if(!j || j>8)
							return false;
					}
					else if(s[i-1]=='u')
					{
						if(j>=4)
							j=4;
						else
							return false;
					}
					else if(s[i-1]=='U')
					{
						if(j>=8)
							j=8;
						else
							return false;
					}
					else
						assert(false);
					
					char backup=s[i+j];
					s[i+j]=0;
					int sn;
					sscanf(s+i, "%x", &sn);
					v.push_back(sn);
					s[i+j]=backup;
					i+=j-1;
				}
				else if(is_octal_digit(s[i]))
				{
					i++;
					int j;
					for(j=0; is_octal_digit(s[i+j]); j++);
					if(!j || j>11 || (j==11 && s[i]>='3'))
						return false;
					
					int sn=0;
					int factor=1;
					for(int k=j+i-1; k>=i; k--)
					{
						sn+=(s[k]-'8')*factor;
						factor*=8;
					}
					v.push_back(sn);
					i+=j-1;
				}
				else
					return false;
			}
		}
	
	return true;
}

bool high_level_decode_escape_sequences(Terminal *t, string &s)
{
	vector<int> v;
	bool result=decode_escape_sequences(t->text, v);
	if(!result)
	{
		cout << "Ill-formed escape sequences in string at ";
		print_terminal_location(cout, t);
		cout << ".\n";
		return false;
	}
	bool flag=true;
	for(int k=0; k<v.size(); k++)
		if(!(v[k]>=0 && v[k]<DolphinData::data.variables.alphabet_cardinality))
		{
                        cout << v[k] <<" cmp "<< DolphinData::data.variables.alphabet_cardinality;
			cout << " Out-of-range characters in string at ";
			print_terminal_location(cout, t);
			cout << ".\n";
			flag=false;
		}
	if(!flag)
		return false;
	
	s.clear();
	for(int k=0; k<v.size(); k++)
		s+=char(v[k]);
	return true;
}

bool get_character_from_the_string_that_is_expected_to_be_exactly_one_character_long(NonterminalExpressionS *expr_s, int nn, int &result)
{
	if(!process_expression_proc(expr_s, nn, false))
		return false;
	
	if(expr_s->is_nts)
	{
		cout << "Expecting a string at ";
		print_terminal_location(cout, expr_s->symbol);
		cout << ".\n";
		return false;
	}
	
	if(expr_s->s.size()!=1)
	{
		cout << "The string should be exactly one character long at ";
		print_terminal_location(cout, expr_s->symbol);
		cout << ".\n";
		return false;
	}
	
	result=expr_s->s[0];
	
	return true;
}

bool process_option_statement(NonterminalOptionStatement &option_statement)
{
	const char *s=option_statement.left->text; // make me const
	
#if 0
	map<const char *, variable_base *, NullTerminatedStringCompare>::iterator p=DolphinData::data.variables.database.find(s);
	if(p==DolphinData::data.variables.database.end())
	{
		cout << "Assignment to unknown variable '" << s << "' at ";
		print_terminal_location(cout, option_statement.left);
		cout << ".\n";
		return false;
	}
	return true;
#endif
	
	map<const char *, AssignmentData, NullTerminatedStringCompare>::iterator p=DolphinData::data.assignments.find(s);
	if(p!=DolphinData::data.assignments.end())
	{
		cout << "Duplicate assignment to variable '" << s << "' at ";
		print_terminal_location(cout, option_statement.left);
		cout << " (earlier assignment at ";
		print_terminal_location(cout, (*p).second.declaration->left);
		cout << ").\n";
		return false;
	}
	
	if(!variable_exists(s))
	{
		cout << "Warning: assignment to unknown variable '" << s << "' at ";
		print_terminal_location(cout, option_statement.left);
		cout << " will probably have no effect.\n";
	}
	
	AssignmentData ad;
	ad.declaration=&option_statement;
	
	bool flag=true;
	for(int j=0; j<option_statement.right.size(); j++)
	{
		const char *s1=option_statement.right[j]->text;
		const char *final_s;
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
		else if(typeid(*option_statement.right[j])==typeid(Whale::TerminalId))
		{
			vt=AssignmentData::VALUE_ID;
			final_s=s1;
		}
		else if(typeid(*option_statement.right[j])==typeid(Whale::TerminalString))
		{
			vt=AssignmentData::VALUE_STRING;
			string str;
			if(!high_level_decode_escape_sequences(option_statement.right[j], str))
			{
				flag=false;
				continue;
			}
			final_s=strdup(str.c_str());
		}
		else if(typeid(*option_statement.right[j])==typeid(Whale::TerminalNumber))
		{
			vt=AssignmentData::VALUE_NUMBER;
			final_s=s1;
		}
		else if(typeid(*option_statement.right[j])==typeid(Whale::TerminalHexNumber))
		{
			vt=AssignmentData::VALUE_HEX_NUMBER;
			assert(s1[0]=='0' && tolower(s1[1])=='x' && s1[2]);
			final_s=s1+2;
		}
		else if(typeid(*option_statement.right[j])==typeid(Whale::TerminalCode))
		{
			vt=AssignmentData::VALUE_CODE;
			const char *s=option_statement.right[j]->text;
			int k=(s[0]=='{' ? 1 : 2);
			string str(s+k, s+strlen(s)-k);
			final_s=strdup(str.c_str());
		}
		else
			assert(false);
		
		ad.values.push_back(std::make_pair(final_s, vt));
	}
	
	DolphinData::data.assignments[s]=ad;
	
	return flag;
}

bool process_grammar(NonterminalS *S)
{
	bool flag=true;
	
	for(int i=0; i<S->statements.size(); i++)
	{
		if(typeid(*S->statements[i])==typeid(NonterminalRuleStatement))
		{
			NonterminalRuleStatement &rule_statement=*dynamic_cast<NonterminalRuleStatement *>(S->statements[i]);
			
			const char *s=rule_statement.left->text;
			int nn=DolphinData::data.find_nonterminal(s);
			if(nn==-1)
			{
				NonterminalData new_nonterminal;
				new_nonterminal.name=s; // it's not an error.
				nn=DolphinData::data.nonterminals.size();
				DolphinData::data.nonterminals.push_back(new_nonterminal);
			}
			
			NonterminalData &nonterminal=DolphinData::data.nonterminals[nn];
			nonterminal.rules.push_back(&rule_statement);
		}
		else if(typeid(*S->statements[i])==typeid(NonterminalActionStatement))
		{
			// doing nothing by now
		}
		else if(typeid(*S->statements[i])==typeid(NonterminalStartConditionsStatement))
		{
			NonterminalStartConditionsStatement &start_conditions_statement=*dynamic_cast<NonterminalStartConditionsStatement *>(S->statements[i]);
			
			for(int j=0; j<start_conditions_statement.names.size(); j++)
			{
				const char *s=start_conditions_statement.names[j]->text;
				int ssn=DolphinData::data.find_start_condition(s);
				
				if(ssn==-1)
				{
					StartConditionData new_start_condition;
					new_start_condition.name=s;
					new_start_condition.declaration=start_conditions_statement.names[j];
					ssn=DolphinData::data.start_conditions.size();
					DolphinData::data.start_conditions.push_back(new_start_condition);
				}
				else
				{
					StartConditionData &start_condition=DolphinData::data.start_conditions[ssn];
					cout << "Start condition ";
					cout << "'" << start_condition.name << "'";
					cout << ", declared at ";
					print_terminal_location(cout, start_condition.declaration);
					cout << ", is redeclared at ";
					print_terminal_location(cout, start_conditions_statement.names[j]);
					cout << "; redeclaration ignored.\n";
				}
			}
		}
		else if(typeid(*S->statements[i])==typeid(NonterminalOptionStatement))
		{
			bool result=process_option_statement(*dynamic_cast<NonterminalOptionStatement *>(S->statements[i]));
			if(!result)
				flag=false;
		}
		else if(typeid(*S->statements[i])==typeid(NonterminalInvalidStatement))
			flag=false; // there was a parse error.
		else
			assert(false);
	}
	
	if(!DolphinData::data.start_conditions.size())
	{
		// if no start conditions were defined, a single start
		// condition named 'INITIAL' should be created.
		
		StartConditionData initial;
		initial.name="INITIAL";
		initial.declaration=NULL;
		DolphinData::data.start_conditions.push_back(initial);
		
		DolphinData::data.variables.start_conditions_enabled=false;
	}
	else
		DolphinData::data.variables.start_conditions_enabled=true;
	
	if(!assign_values_to_variables_stage_zero()) return false;
	cout << "Alphabet cardinality is " << DolphinData::data.variables.alphabet_cardinality << ".\n";
	
	int n=DolphinData::data.nonterminals.size();
	
	DolphinData::data.derivation_paths.destructive_resize(n, n);
	for(int i=0; i<n; i++)
	{
		NonterminalData &nonterminal=DolphinData::data.nonterminals[i];
		
		for(int j=0; j<nonterminal.rules.size(); j++)
		{
			// Prof. Zhogolev would have killed me for this trick:
			bool old_value=nonterminal.can_be_used_in_IN_expressions;
			nonterminal.can_be_used_in_IN_expressions=true;
			
			if(!process_expression_proc(nonterminal.rules[j]->right, i, true))
			{
				cout << nonterminal.name << " " << j << "\n";
				flag=false;
			}
			
			if(!nonterminal.can_be_used_in_IN_expressions)
				nonterminal.locations_of_offending_rules.push_back(nonterminal.rules[j]->arrow);
			else
				nonterminal.can_be_used_in_IN_expressions=old_value;
			// I do feel ashamed, honestly!
		}
	}
	
	for(int i=0; i<S->statements.size(); i++)
	{
		if(typeid(*S->statements[i])==typeid(NonterminalActionStatement))
		{
			NonterminalActionStatement &action_statement=*dynamic_cast<NonterminalActionStatement *>(S->statements[i]);
			
			char *s;
			bool is_special;
			if(typeid(*action_statement.expr)==typeid(NonterminalPairOfExpressions))
			{
				NonterminalPairOfExpressions &poe=*dynamic_cast<NonterminalPairOfExpressions *>(action_statement.expr);
				
				if(!process_expression_proc(poe.expr, -1, true)) { flag=false; continue; }
				string str=serialize_expression(poe.expr);
				
				if(poe.lookahead)
				{
					if(!process_expression_proc(poe.lookahead, -1, true)) { flag=false; continue; }
					str+=" / "+serialize_expression(poe.lookahead);
				}
				if(poe.eof)
				{
					if(!poe.lookahead)
						str+=" / ";
					else
						str+=" ";
					if(poe.not_eof) str+="~";
					str+="eof";
				}
				
				s=strdup(str.c_str());
				is_special=false;
			}
			else if(typeid(*action_statement.expr)==typeid(TerminalKwEof))
			{
				s=strdup("eof");
				is_special=true;
			}
			else if(typeid(*action_statement.expr)==typeid(TerminalKwError))
			{
				s=strdup("error");
				is_special=true;
			}
			
			int ren;
			if(!DolphinData::data.recognized_expression_search.count(s))
			{
				ren=DolphinData::data.recognized_expressions.size();
				
				RecognizedExpressionData re;
				if(!is_special)
				{
					NonterminalPairOfExpressions &poe=*dynamic_cast<NonterminalPairOfExpressions *>(action_statement.expr);
					re.expr=poe.expr;
					re.lookahead=poe.lookahead;
					if(poe.eof)
						re.lookahead_eof=(poe.not_eof ? -1 : 1);
					else
						re.lookahead_eof=0;
				}
				else
				{
					re.expr=NULL;
					re.lookahead=NULL;
					re.lookahead_eof=0;
				}
				re.is_special=is_special;
				re.in_serialized_form=s;
				DolphinData::data.recognized_expressions.push_back(re);
				DolphinData::data.recognized_expression_search[re.in_serialized_form]=ren;
				
			#ifdef PRINT_SERIALIZED_EXPRESSIONS
				cout << s << "\n";
			#endif
			}
			else
			{
				ren=DolphinData::data.recognized_expression_search[s];
				delete s;
			}
			
			ActionData action;
			action.is_special=is_special;
			action.declaration=&action_statement;
			action.recognized_expression_number=ren;
			
			if(action_statement.start_conditions)
			{
				NonterminalStartConditionsExpression *sce=action_statement.start_conditions;
				NonterminalStartConditionsExpressionList *scel=dynamic_cast<NonterminalStartConditionsExpressionList *>(sce);
				NonterminalStartConditionsExpressionAsterisk *scea=dynamic_cast<NonterminalStartConditionsExpressionAsterisk *>(sce);
				assert(!scel || !scea);
				if(scel)
				{
					for(int k=0; k<scel->names.size(); k++)
					{
						Terminal *t=scel->names[k];
						int sc=DolphinData::data.find_start_condition(t->text);
						if(sc==-1)
						{
							cout << "Reference to undefined start condition '";
							cout << t->text << "' at ";
							print_terminal_location(cout, t);
							cout << ".\n";
							flag=false;
						}
						else if(action.start_condition_numbers.count(sc))
						{
							// duplicate declaration.
							// looking back to report error.
							for(int l=0; l<k; l++)
								if(!strcmp(scel->names[l]->text, t->text))
								{
									cout << "Warning: duplicate start condition ignored at ";
									print_terminal_location(cout, t);
									cout << " (earlier occurence at ";
									print_terminal_location(cout, scel->names[l]);
									cout << ").\n";
								}
						}
						else
							action.start_condition_numbers.insert(sc);
					}
				}
				else if(scea)
					for(int j=0; j<DolphinData::data.start_conditions.size(); j++)
						action.start_condition_numbers.insert(j);
				else
					assert(false);
			}
			else
			{
				// using default set of start conditions.
				
			#if 0
				// <INITIAL>
				action.start_condition_numbers.insert(0);
			#else
				// <*>
				for(int j=0; j<DolphinData::data.start_conditions.size(); j++)
					action.start_condition_numbers.insert(j);
			#endif
			}
			
			int action_number=DolphinData::data.actions.size();
			DolphinData::data.actions.push_back(action);
			
			DolphinData::data.recognized_expressions[ren].action_numbers.push_back(action_number);
			
			if(typeid(*action_statement.action)==typeid(NonterminalActionCodeII))
			{
				NonterminalActionCodeII &a_code2=*dynamic_cast<NonterminalActionCodeII *>(action_statement.action);
				const char *s=a_code2.code->text;
				int k=(s[0]=='{' ? 1 : 2);
				a_code2.s=string(s+k, s+strlen(s)-k);
			}
			else if(typeid(*action_statement.action)==typeid(NonterminalActionReturn))
			{
				Terminal *kw=dynamic_cast<NonterminalActionReturn *>(action_statement.action)->kw;
				if(kw) DolphinData::data.return_keywords_used=true;
			}
			else if(typeid(*action_statement.action)==typeid(NonterminalActionSkip))
				DolphinData::data.skip_keywords_used=true;
		}
	}
	
	if(!flag) return false;
	
	if(DolphinData::data.return_keywords_used)
	{
		cout << "Error: 'return' keyword is no more. Use 'expr ==> value;'.\n";
		flag=false;
	}
	if(DolphinData::data.skip_keywords_used)
	{
		cout << "Error: 'skip' keyword is no more. Use 'expr ==> { };'.\n";
		flag=false;
	}
	
	// at this point we have information on all one-step derivations stored
	// in DolphinData::data.derivation_paths. Performing a transitive closure of this
	// matrix.
	for(;;)
	{
		bool repeat_flag=false;
		
		for(int i=0; i<n; i++)
			for(int j=0; j<n; j++)
			{
				if(i==j) continue;
				PathData &m_ij=DolphinData::data.derivation_paths[i][j];
				if(!m_ij.v.size()) continue;
				
				for(int k=0; k<n; k++)
				{
					if(k==j) continue;
					PathData &m_jk=DolphinData::data.derivation_paths[j][k];
					if(!m_jk.v.size()) continue;
					
					PathData &m_ik=DolphinData::data.derivation_paths[i][k];
					
					if(!m_ik.v.size())
					{
						// Case 1. m_ik is empty ==>
						// just merge m_ij and m_jk.
						
					#ifdef DEBUG_DERIVATION_PATHS_MATRIX
						cout << "Case 1: " << i << ", " << j << ", " << k << "\n";
					#endif
						copy(m_ij.v.begin(), m_ij.v.end(), back_inserter(m_ik.v));
						copy(m_jk.v.begin(), m_jk.v.end(), back_inserter(m_ik.v));
						m_ik.worst=max(m_ij.worst, m_jk.worst);
						repeat_flag=true;
					}
					else if(m_ik.v.size() && m_ik.worst<max(m_ij.worst, m_jk.worst))
					{
						// Case 2. m_ik already contains
						// a recursion path, but a worse
						// one is actually possible ==>
						// replace a better path with a
						// worse.
						
					#ifdef DEBUG_DERIVATION_PATHS_MATRIX
						cout << "Case 2: " << i << ", " << j << ", " << k << "\n";
					#endif
						m_ik.v.clear();
						copy(m_ij.v.begin(), m_ij.v.end(), back_inserter(m_ik.v));
						copy(m_jk.v.begin(), m_jk.v.end(), back_inserter(m_ik.v));
						m_ik.worst=max(m_ij.worst, m_jk.worst);
						repeat_flag=true;
					}
				}
			}
		if(!repeat_flag)
			break;
	#ifdef DEBUG_DERIVATION_PATHS_MATRIX
		cout << "repeat\n";
	#endif
	}
	
#ifdef DEBUG_DERIVATION_PATHS_MATRIX
	print_derivation_paths();
#endif
	
	for(int i=0; i<n; i++)
	{
		PathData &m_ii=DolphinData::data.derivation_paths[i][i];
		if(!m_ii.v.size()) continue;
		
		if(m_ii.v[0].second==PathData::IN_EXPRESSION)
		{
			PathData path=m_ii;
			cout << "Recursive derivation path (";
			print_derivation_path(cout, path, i);
			cout << ") contains 'in' expression.\n";
			flag=false;
		}
	}
	
	if(!flag) return false;
	
	for(int i=0; i<n; i++)
	{
		NonterminalData &nonterminal=DolphinData::data.nonterminals[i];
		if(!nonterminal.is_used_in_IN_expressions) continue;
		
		int nn2;
		if(DolphinData::data.derivation_paths[i][i].v.size())
		{
			nn2=i;
		}
		else
		{
			nn2=-1;
//			int l=(1 << (sizeof(int)*8-2));
			for(int j=0; j<n; j++)
				if(DolphinData::data.derivation_paths[j][j].v.size()
					&& DolphinData::data.derivation_paths[i][j].v.size())
				{
					nn2=j;
					break;
				}
			if(nn2==-1) continue;
		}
		
		int k=nonterminal.where_it_is_used_in_IN_expressions.size();
		assert(k);
		cout << "Nonterminal " << nonterminal.name << ", used in 'in' expression" << (k==1 ? " " : "s ");
		print_a_number_of_terminal_locations(cout, nonterminal.where_it_is_used_in_IN_expressions, "at");
		if(i!=nn2)
		{
			cout << ", can expand (the derivation path is ";
			print_derivation_path(cout, DolphinData::data.derivation_paths[i][nn2], i);
			cout << ") to nonterminal " << DolphinData::data.nonterminals[nn2].name;
			cout << ", which ";
		}
		else
			cout << ", ";
		cout << "is self-embedding (consider a recursive derivation path ";
		print_derivation_path(cout, DolphinData::data.derivation_paths[nn2][nn2], nn2);
		cout << ").\n";
		flag=false;
	}
	
	if(!flag) return false;
	
	for(int i=0; i<n; i++)
	{
		NonterminalData &nonterminal=DolphinData::data.nonterminals[i];
		if(!nonterminal.is_used_in_IN_expressions) continue;
		
		int nn2;
		if(!nonterminal.can_be_used_in_IN_expressions)
			nn2=i;
		else
		{
			nn2=-1;
			int l=(1 << (sizeof(int)*8-2));
			for(int j=0; j<n; j++)
			{
				int this_l=DolphinData::data.derivation_paths[i][j].v.size();
				if(!DolphinData::data.nonterminals[j].is_used_in_IN_expressions
					&& !DolphinData::data.nonterminals[j].can_be_used_in_IN_expressions
					&& this_l>0 && this_l<l)
				{
					nn2=j;
					l=this_l;
				}
			}
			if(nn2==-1) continue;
		}
		
		int k=nonterminal.where_it_is_used_in_IN_expressions.size();
		assert(k);
		cout << "Nonterminal " << nonterminal.name << ", used in 'in' expression" << (k==1 ? " " : "s ");
		print_a_number_of_terminal_locations(cout, nonterminal.where_it_is_used_in_IN_expressions, "at");
		if(i!=nn2)
		{
			cout << ", can expand (the derivation path is ";
			print_derivation_path(cout, DolphinData::data.derivation_paths[i][nn2], i);
			cout << ") to nonterminal " << DolphinData::data.nonterminals[nn2].name;
			cout << ", which ";
		}
		else
			cout << ", ";
		
		int k2=DolphinData::data.nonterminals[nn2].locations_of_offending_rules.size();
		assert(k2);
		cout << "can possibly expand to a string that is other than one character long (offending rule" << (k2==1 ? " " : "s ");
		print_a_number_of_terminal_locations(cout, DolphinData::data.nonterminals[nn2].locations_of_offending_rules, "at");
		cout << ").\n";
		
		flag=false;
	}
	
	if(!flag) return false;
	
	// ensuring that there are no recursive symbols except strictly
	// right recursive.
	set<int> already_reported;
	for(int i=0; i<n; i++)
	{
		PathData &m_ii=DolphinData::data.derivation_paths[i][i];
		
		if(m_ii.v.size() && m_ii.worst>PathData::RIGHT &&
			!already_reported.count(i))
		{
			cout << "Recursion made up by derivation path (";
			print_derivation_path(cout, m_ii, i, PathData::RIGHT);
			cout << ") is rendered invalid by transitions marked "
				"with braces.\n";
			
			for(int j=0; j<m_ii.v.size(); j++)
			{
				int nn=DolphinData::data.find_nonterminal(m_ii.v[j].first->text);
				assert(nn!=-1);
				already_reported.insert(nn);
			}
			
			flag=false;
		}
	}
	
	if(!flag) return false;
	
	// looking for collisions between different actions for one expression.
	for(int i=0; i<DolphinData::data.recognized_expressions.size(); i++)
	{
		RecognizedExpressionData &re=DolphinData::data.recognized_expressions[i];
//		cout << "Recognized expression " << re.in_serialized_form << ": "
//			<< re.action_numbers << "\n";
		
		bool conflicts_found=false;
		set<int> conflicting_actions;
		set<int> start_conditions_involved;
		
		for(int j=0; j<DolphinData::data.start_conditions.size(); j++)
		{
			set<int> local_conflicting_actions;
			for(int k=0; k<re.action_numbers.size(); k++)
				if(DolphinData::data.actions[re.action_numbers[k]].start_condition_numbers.count(j))
					local_conflicting_actions.insert(re.action_numbers[k]);
			
			if(!local_conflicting_actions.size())
				re.start_condition_to_action_number.push_back(-1);
			else if(local_conflicting_actions.size()==1)
				re.start_condition_to_action_number.push_back(*local_conflicting_actions.begin());
			else
			{
				conflicts_found=true;
				union_assign(conflicting_actions, local_conflicting_actions);
				start_conditions_involved.insert(j);
				re.start_condition_to_action_number.push_back(-1); // doesn't matter
			}
		}
		
		if(conflicts_found)
		{
			assert(conflicting_actions.size()>1);
			bool in_all_start_conditions=(start_conditions_involved.size()==DolphinData::data.start_conditions.size());
			
			if(in_all_start_conditions)
				cout << "Multiple actions ";
			else
				cout << "Actions ";
			cout << "for expression ";
			cout << re.in_serialized_form;
			cout << ", defined at ";
			
			vector<Terminal *> v;
			for(set<int>::iterator p=conflicting_actions.begin(); p!=conflicting_actions.end(); p++)
				v.push_back(DolphinData::data.actions[*p].declaration->arrow);
			print_a_number_of_terminal_locations(cout, v, "at");
			
                        if(in_all_start_conditions)
                        	cout << ".\n";
			else
			{
				bool more_than_one_sc=(start_conditions_involved.size()>1);
				cout << " overlap in start condition";
				if(more_than_one_sc) cout << "s";
				cout << " ";
				print_a_number_of_start_conditions(cout, start_conditions_involved);
				cout << ".\n";
			}
			
			flag=false;
		}
	}
	
	if(!flag) return false;
	
	// The grammar seems to be correct. For each nonterminal, merging all
	// its rules into one.
	for(int i=0; i<n; i++)
	{
		NonterminalData &nonterminal=DolphinData::data.nonterminals[i];
		while(nonterminal.rules.size()>1)
		{
			int p2=nonterminal.rules.size()-1;
			
			NonterminalExpressionDisjunction *new_node=new NonterminalExpressionDisjunction;
			new_node->expr1=nonterminal.rules[0]->right;
			new_node->expr2=nonterminal.rules[p2]->right;
			nonterminal.rules[0]->right=new_node;
			
			// A few bytes of memory left hanging won't do any harm.
			nonterminal.rules.pop_back();
		}
	}
	
	return true;
}
