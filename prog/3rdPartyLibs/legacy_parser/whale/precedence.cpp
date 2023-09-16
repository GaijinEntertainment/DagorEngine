
#include "whale.h"
#include "precedence.h"
#include "process.h"
//#include "stl.h"
#include "utilities.h"

#include <fstream>

using namespace std;
using namespace Whale;

// NB: comparisons with asterisk are not transitive.

//#define DEBUG_CONFLICT_RESOLUTION

bool acquire_new_precedence_symbols();
int acquire_single_precedence_symbol(Terminal *t_name, Terminal *t_assoc, bool can_create_new_symbols, bool can_use_existing_symbols);
bool acquire_relations_between_precedence_symbols(vector<triad<int, Terminal *, int> > &relation_statements);
void set_default_associativity();

bool process_precedence()
{
	bool flag=true;
	
	bool keep_log=WhaleData::data.variables.dump_precedence_to_file;
	ofstream log;
	if(keep_log)
	{
		string log_file_name=WhaleData::data.file_name+string(".prec");
		log.open(log_file_name.c_str());
		log << "Precedence.\n\n";
	}
	
	vector<triad<int, Terminal *, int> > relation_statements;
	
	if(!acquire_new_precedence_symbols())
		return false;
	
	if(!acquire_relations_between_precedence_symbols(relation_statements))
		return false;
	
	set_default_associativity();
	
	if(WhaleData::data.precedence_symbols.size() || relation_statements.size()>0)
		cout << WhaleData::data.precedence_symbols.size() << " precedence symbols, "
			<< relation_statements.size() << " relations.\n";
	if(keep_log)
	{
		log << WhaleData::data.precedence_symbols.size() << " precedence symbols, "
			<< relation_statements.size() << " relations.\n\n";
		
		for(int i=0; i<WhaleData::data.precedence_symbols.size(); i++)
			log << WhaleData::data.precedence_symbols[i].name << "\n";
		log << "\n";
	}
	
	WhaleData::data.precedence_database.initialize(WhaleData::data.precedence_symbols.size());
	
	for(int i=0; i<relation_statements.size(); i++)
	{
		triad<int, Terminal *, int> t=relation_statements[i];
		
		// should deal with asteriskness in some other way!
		if(t.first==-1 || t.third==-1) continue;
		
		if(typeid(*t.second)==typeid(TerminalEqual) ||
			typeid(*t.second)==typeid(TerminalAssign))
		{
			WhaleData::data.precedence_database.access_equivalence_relation(t.first, t.third)=true;
		}
		else if(typeid(*t.second)==typeid(TerminalLessThan))
			WhaleData::data.precedence_database.access_partial_order_relation(t.first, t.third)=true;
		else if(typeid(*t.second)==typeid(TerminalGreaterThan))
			WhaleData::data.precedence_database.access_partial_order_relation(t.third, t.first)=true;
		else
			assert(false);
	}
	
	if(keep_log)
	{
		for(int i=0; i<WhaleData::data.rules.size(); i++)
		{
			RuleData &rule=WhaleData::data.rules[i];
			
			if(rule.precedence_symbol_number>=0)
				log << "Rule " << rule.in_printable_form()
					<< " has precedence "
					<< WhaleData::data.precedence_symbols[rule.precedence_symbol_number].name
					<< ".\n";
		}
		
		log << "\n\tBefore closure.\n";
		WhaleData::data.precedence_database.print_tables(log);
	}
	
	WhaleData::data.precedence_database.closure();
	
	if(keep_log)
	{
		log << "\n\tAfter closure.\n";
		WhaleData::data.precedence_database.print_tables(log);
		log << "\n";
		
		for(int i=0; i<WhaleData::data.precedence_database.n; i++)
			for(int j=0; j<WhaleData::data.precedence_database.n; j++)
				if(WhaleData::data.precedence_database.partial_order_relation[i][j])
					log << WhaleData::data.precedence_symbols[i].name << " < " << WhaleData::data.precedence_symbols[j].name << "\n";
	}
	
	set<int> precedence_symbols_we_have_problems_with;
	for(int i=0; i<WhaleData::data.precedence_symbols.size(); i++)
	{
		bool less_than_self=WhaleData::data.precedence_database.access_partial_order_relation(i, i);
		if(less_than_self)
			precedence_symbols_we_have_problems_with.insert(i);
	}
	if(precedence_symbols_we_have_problems_with.size())
	{
		cout << "Inconsistent precedence information: something is "
			"wrong somewhere around symbol"
			<< (precedence_symbols_we_have_problems_with.size()==1 ? " " : "s ");
		custom_print(cout, precedence_symbols_we_have_problems_with.begin(), precedence_symbols_we_have_problems_with.end(), PrecedenceNamePrinter(), ", ", " and ");
		cout << ".\n";
		flag=false;
	}
	
	return flag;
}

bool acquire_new_precedence_symbols()
{
	bool flag=true;
	
	for(int i=0; i<WhaleData::data.S->statements.size(); i++)
	{
	    auto &a = *WhaleData::data.S->statements[i];
		if(typeid(a)==typeid(NonterminalPrecedenceStatement))
		{
			NonterminalPrecedenceStatement &precedence_statement=*dynamic_cast<NonterminalPrecedenceStatement *>(WhaleData::data.S->statements[i]);
			
			for(int j=0; j<precedence_statement.expr.size(); j++)
			{
				int m=precedence_statement.expr[j]->operands.size();
				
				if(m==1)
				{
					Terminal *t=precedence_statement.expr[j]->operands[0];
					Terminal *t_assoc=precedence_statement.expr[j]->associativity[0];
					int psn=acquire_single_precedence_symbol(t, t_assoc, true, false);
					if(psn==-1) flag=false;
				}
			}
		}
	}

	for(int i=0; i<WhaleData::data.rules.size(); i++)
	{
		RuleData &rule=WhaleData::data.rules[i];
		assert(rule.precedence_symbol_number==-1);
		
		bool there_are_comparisons=false;
		set<int> lone_precedence_symbols;
		
		for(int j=0; j<rule.precedence_expressions.size(); j++)
			for(int k=0; k<rule.precedence_expressions[j]->operands.size(); k++)
				if(rule.precedence_expressions[j]->comparison_operators[k]==NULL)
				{
					Terminal *t=rule.precedence_expressions[j]->operands[k];
					int psn=acquire_single_precedence_symbol(t, NULL, true, true);
					if(psn==-1)
						flag=false;
					else
						lone_precedence_symbols.insert(psn);
				//	cout << "\t" << t->text << "\n";
				}
				else
				{
					Terminal *t=rule.precedence_expressions[j]->operands[k];
					int psn=acquire_single_precedence_symbol(t, NULL, true, true);
					if(psn==-1) flag=false;
					there_are_comparisons=true;
				}
		
		if(lone_precedence_symbols.size()==0)
		{
			if(there_are_comparisons)
			{
				// Create a new precedence symbol for this rule
				string s=string("Rule")+roman_itoa(i+1, true);
				
				PrecedenceSymbolData precedence_symbol;
				precedence_symbol.name=strdup(s.c_str());
				// *** change ***
				precedence_symbol.declared_at=NULL;
				precedence_symbol.was_implicitly_declared=true;
				
				int new_psn=WhaleData::data.precedence_symbols.size();
				WhaleData::data.precedence_symbols.push_back(precedence_symbol);
				
				rule.precedence_symbol_number=new_psn;
			}
		}
		else if(lone_precedence_symbols.size()==1)
		{
			// everything's clear: just assign this precedence symbol
			// to the rule
			
			rule.precedence_symbol_number=*lone_precedence_symbols.begin();
		}
		else if(lone_precedence_symbols.size()>1)
		{
			cout << "Warning (*** change me ***): unexpected artistry "
				"with precedence symbols somewhere around ";
			rule.precedence_expressions[0]->operands[0]->print_location(cout);
			cout << ".\n";
			rule.precedence_symbol_number=*lone_precedence_symbols.begin();
		}
	}
	
	return flag;
}

// returns precedence symbol number upon success, -1 upon failure
int acquire_single_precedence_symbol(Terminal *t_name, Terminal *t_assoc, bool can_create_new_symbols, bool can_use_existing_symbols)
{
	assert(can_create_new_symbols || can_use_existing_symbols);
	
	if(!strcmp(t_name->text, "*")) // by now, all uses of the asterisk are invalid
	{
		cout << "Invalid use of asterisk at ";
		t_name->print_location(cout);
		cout << ".\n";
		return -1;
	}
	
	PrecedenceSymbolData::Associativity associativity_given;
	if(t_assoc)
	{
		if(!strcmp(t_assoc->text, "left"))
			associativity_given=PrecedenceSymbolData::ASSOC_LEFT;
		else if(!strcmp(t_assoc->text, "none"))
			associativity_given=PrecedenceSymbolData::ASSOC_NONE;
		else if(!strcmp(t_assoc->text, "right"))
			associativity_given=PrecedenceSymbolData::ASSOC_RIGHT;
		else
		{
			cout << "Invalid associativity at ";
			t_assoc->print_location(cout);
			cout << ": expecting 'left', 'right' or 'none'.\n";
			return -1;
		}
	}
	else
		associativity_given=PrecedenceSymbolData::ASSOC_UNDEFINED;
	
	int psn=WhaleData::data.find_precedence_symbol(t_name->text);
	if(psn!=-1)
	{
		if(can_use_existing_symbols)
		{
			if(t_assoc && WhaleData::data.precedence_symbols[psn].associativity!=PrecedenceSymbolData::ASSOC_UNDEFINED &&
				associativity_given!=PrecedenceSymbolData::ASSOC_UNDEFINED &&
				WhaleData::data.precedence_symbols[psn].associativity!=associativity_given)
			{
				cout << "Associativity declaration for ";
				cout << "precedence symbol " << t_name->text << " at ";
				t_assoc->print_location(cout);
				cout << " doesn't match previous declaration at ";
				WhaleData::data.precedence_symbols[psn].declared_at->print_location(cout);
				cout << ".\n";
				return -1;
			}
			return psn;
		}
		else
		{
			PrecedenceSymbolData &precedence_symbol=WhaleData::data.precedence_symbols[psn];
			cout << "Precedence symbol " << precedence_symbol.name << ", ";
			precedence_symbol.print_where_it_was_declared(cout);
			cout << ", cannot be redeclared at ";
			t_name->print_location(cout);
			cout << ".\n";
			return -1;
		}
	}
	
	int nn=WhaleData::data.find_nonterminal(t_name->text);
	if(nn!=-1)
	{
		cout << "Symbol already in use (WRITE ME!) at ";
		t_name->print_location(cout);
		cout << ".\n";
		return -1;
	}
	
	int tn=WhaleData::data.find_terminal(t_name->text);
	
	if(tn==-1 && !can_create_new_symbols)
	{
		cout << "Reference to undefined precedence symbol "
			<< t_name->text << " at ";
		t_name->print_location(cout);
		cout << ".\n";
		return -1;
	}
	
	PrecedenceSymbolData precedence_symbol;
	precedence_symbol.name=t_name->text;
	precedence_symbol.associativity=associativity_given;
	
	if(tn!=-1 && !can_create_new_symbols)
	{
		TerminalData &terminal=WhaleData::data.terminals[tn];
		
		precedence_symbol.declared_at=terminal.declaration;
		precedence_symbol.was_implicitly_declared=true;
	}
	else
		precedence_symbol.declared_at=t_name;
	
	int new_psn=WhaleData::data.precedence_symbols.size();
	WhaleData::data.precedence_symbols.push_back(precedence_symbol);
	
	return new_psn;
}

bool acquire_relations_between_precedence_symbols(vector<triad<int, Terminal *, int> > &relation_statements)
{
	bool flag=true;
	
	for(int i=0; i<WhaleData::data.S->statements.size(); i++)
	{
		auto &a = *WhaleData::data.S->statements[i];
		if(typeid(a)==typeid(NonterminalPrecedenceStatement))
		{
			NonterminalPrecedenceStatement &precedence_statement=*dynamic_cast<NonterminalPrecedenceStatement *>(WhaleData::data.S->statements[i]);
			
			for(int j=0; j<precedence_statement.expr.size(); j++)
			{
				int m=precedence_statement.expr[j]->operands.size();
				
				if(m>1)
				{
					vector<int> these_psn;
					bool local_flag=true;
					for(int k=0; k<m; k++)
					{
						Terminal *t=precedence_statement.expr[j]->operands[k];
						Terminal *t_assoc=precedence_statement.expr[j]->associativity[k];
						if(!strcmp(t->text, "*"))
							these_psn.push_back(-1);
						else
						{
							int psn=acquire_single_precedence_symbol(t, t_assoc, false, true);
							if(psn!=-1)
								these_psn.push_back(psn);
							else
								local_flag=false;
						}
					}
					if(local_flag==false)
					{
						flag=false;
						continue;
					}
					
					for(int k=0; k<m-1; k++)
						relation_statements.push_back(make_triad(these_psn[k], precedence_statement.expr[j]->comparison_operators[k], these_psn[k+1]));
				}
			}
		}
  	}
	
	for(int i=0; i<WhaleData::data.rules.size(); i++)
	{
		RuleData &rule=WhaleData::data.rules[i];
		
		if(rule.precedence_symbol_number==-1)
		{
			int tn=-1;
			for(int j=rule.body.size()-1; j>=0; j--)
				if(rule.body[j].is_terminal())
				{
					tn=rule.body[j].terminal_number();
					break;
				}
			
			if(tn!=-1)
				rule.precedence_symbol_number=WhaleData::data.find_precedence_symbol(WhaleData::data.terminals[tn].name);
		}
		
		for(int j=0; j<rule.precedence_expressions.size(); j++)
			for(int k=0; k<rule.precedence_expressions[j]->operands.size(); k++)
				if(rule.precedence_expressions[j]->comparison_operators[k])
				{
					assert(rule.precedence_symbol_number>=0);
					
					Terminal *t_x=rule.precedence_expressions[j]->operands[k];
					Terminal *t_sign=rule.precedence_expressions[j]->comparison_operators[k];
					
					int psn=WhaleData::data.find_precedence_symbol(t_x->text);
					if(psn==-1)
					{
						// if I'm not mistaken, the
						// error message have already
						// been printed out.
						flag=false;
						continue;
					}
					
					relation_statements.push_back(make_triad(rule.precedence_symbol_number, t_sign, psn));
				}
	}
	
	return flag;
}

void set_default_associativity()
{
	for(int i=0; i<WhaleData::data.precedence_symbols.size(); i++)
	{
		PrecedenceSymbolData &ps=WhaleData::data.precedence_symbols[i];
		
		if(ps.associativity==PrecedenceSymbolData::ASSOC_UNDEFINED)
			ps.associativity=PrecedenceSymbolData::ASSOC_LEFT;
	}
}

// rv.first: are we sure, rv.second: -1 for shift, 0+ for reduce.
pair<bool, int> resolve_lr_conflict(int shift_terminal, int shift_state, const vector<int> &reduce_rules)
{
	// ought to rewrite the whole function: it was a bad idea to
	// unload all input values to those vectors.
	
	assert(reduce_rules.size()>0 || shift_terminal!=-1);
	
#ifdef DEBUG_CONFLICT_RESOLUTION
	cout << "Processing LR conflict: shift " << shift_terminal
		<< ", reduce " << reduce_rules << "\n";
#endif
	
	vector<int> priorities;
	vector<LRAction> actions;
	if(shift_terminal!=-1)
	{
		TerminalData &terminal=WhaleData::data.terminals[shift_terminal];
		int psn=WhaleData::data.find_precedence_symbol(terminal.name);
	#ifdef DEBUG_CONFLICT_RESOLUTION
		cout << "For Shift: " << psn << "\n";
	#endif
		priorities.push_back(psn); // possibly -1.
		actions.push_back(LRAction::shift(shift_state));
	}
	for(int i=0; i<reduce_rules.size(); i++)
	{
		priorities.push_back(WhaleData::data.rules[reduce_rules[i]].precedence_symbol_number);
	#ifdef DEBUG_CONFLICT_RESOLUTION
		cout << "For Reduce: " << priorities.back() << "\n";
	#endif
		actions.push_back(LRAction::reduce(reduce_rules[i]));
	}
	
	// How does 'associativity' influence conflict resolution:
	// If a is left-associative, it means that for each b: a~b
	//		Shift a   <   Reduce R (precedence b)
	// If a is right-associative, it means that for each b: a~b
	//		Shift a   >   Reduce R (precedence b)
	// Only shift-reduce conflicts may be resolved using associativity.
	
	// Note: if a~b, then a and b should have the same associativity.
	
	// Trying to find an action with highest precedence.
	for(int i=0; i<priorities.size(); i++)
	{
		// Suppose action[i] has the highest precedence.
	#ifdef DEBUG_CONFLICT_RESOLUTION
		cout << "Suppose " << priorities[i] << "\n";
	#endif
		
		bool success_flag=true;
		for(int j=0; j<priorities.size(); j++)
		{
			if(i==j) continue;
		#ifdef DEBUG_CONFLICT_RESOLUTION
			cout << "\tConsider " << priorities[j] << "\n";
		#endif
			
			if(priorities[i]==-1 || priorities[j]==-1)
			{
			//	cout << "conflict resolution: the bad thing.\n";
				success_flag=false;
				break;
			}
			else if(WhaleData::data.precedence_database.access_partial_order_relation(priorities[j], priorities[i]))
			{
				// action[i] > action[j] - quite coherent with
				// our hypothesis.
			}
			else if(WhaleData::data.precedence_database.access_equivalence_relation(priorities[i], priorities[j]))
			{
				// action[i] ~ action[j]. Depends upon
				// associativity.
				
				if(actions[i].is_shift() && actions[j].is_reduce() &&
					WhaleData::data.precedence_symbols[priorities[i]].associativity==PrecedenceSymbolData::ASSOC_RIGHT)
				{
					// right associativity ==> shift
					// has greater precedence ==> satisfies
					// the hypothesis.
				}
				else if(actions[i].is_reduce() && actions[j].is_shift() &&
					WhaleData::data.precedence_symbols[priorities[j]].associativity==PrecedenceSymbolData::ASSOC_LEFT)
				{
					// left associativity ==> shift has
					// less precedence ==> satisfies the
					// hypothesis.
				}
				else
				{
					success_flag=false;
					break;
				}
			}
			else
			{
				// This example disproves our hypothesis.
				success_flag=false;
				break;
			}
		}
		
		if(success_flag)
		{
			if(actions[i].is_shift())
				return make_pair(true, -1);
			else if(shift_state==-1)
				return make_pair(true, i);
			else
				return make_pair(true, i-1);
		}
	}
	
	return make_pair(false, (shift_state>=0 ? -1 : 0));
}

void PrecedenceDatabase::initialize(int supplied_n)
{
	n=supplied_n;
	equivalence_relation.destructive_resize(n, n);
	partial_order_relation.destructive_resize(n, n);
	for(int i=0; i<n; i++)
		for(int j=0; j<n; j++)
		{
			equivalence_relation[i][j]=false;
			partial_order_relation[i][j]=false;
		}
}

void PrecedenceDatabase::closure()
{
	for(int i=0; i<n; i++)
		equivalence_relation[i][i]=true;
	
	bool flag=true;
	
	while(flag)
	{
		flag=false;
		
		for(int i=0; i<n; i++)
			for(int j=0; j<n; j++)
			{
				if(equivalence_relation[i][j] &&
					!equivalence_relation[j][i])
				{
					equivalence_relation[j][i]=true;
					flag=true;
				}
				
				for(int k=0; k<n; k++)
				{
					if(!equivalence_relation[i][k] &&
						equivalence_relation[i][j] &&
						equivalence_relation[j][k])
					{
						equivalence_relation[i][k]=true;
						flag=true;
					}
					if(!partial_order_relation[i][k] &&
						partial_order_relation[i][j] &&
						partial_order_relation[j][k])
					{
						partial_order_relation[i][k]=true;
						flag=true;
					}
					if(!partial_order_relation[i][k] &&
						partial_order_relation[i][j] &&
						equivalence_relation[j][k])
					{
						partial_order_relation[i][k]=true;
						flag=true;
					}
					if(!partial_order_relation[i][k] &&
						equivalence_relation[i][j] &&
						partial_order_relation[j][k])
					{
						partial_order_relation[i][k]=true;
						flag=true;
					}
				}
			}
	}
}

void PrecedenceDatabase::print_tables(ostream &os)
{
	os << "Equivalence relation:\n";
	for(int i=0; i<n; i++)
	{
		for(int j=0; j<n; j++)
			os << equivalence_relation[i][j] << " ";
		os << "\n";
	}
	os << "Partial order relation:\n";
	for(int i=0; i<n; i++)
	{
		for(int j=0; j<n; j++)
			os << partial_order_relation[i][j] << " ";
		os << "\n";
	}
}
