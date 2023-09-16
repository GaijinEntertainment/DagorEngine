
#include "whale.h"
#include "lr1.h"
#include "nstl/stl_format.h"
#include "utilities.h"
#include "first.h"
#include <fstream>
using namespace std;

//#define DEBUG_TRANSITIONS
//#define DEBUG_ACTIONS

struct AddedByClosurePrinter
{
	ostream &operator ()(ostream &os, const pair<int, sorted_vector<int> > &p)
	{
		os << WhaleData::data.nonterminals[p.first].name << "(";
		assert(p.second.size());
		custom_print(os, p.second.begin(), p.second.end(), TerminalNamePrinter(), ", ", ", ");
		os << ")";
		return os;
	}
};

template<class T> void union_assign(sorted_vector<T> &dest, const sorted_vector<T> &src)
{
	sorted_vector<int> tmp;
	set_union(dest.begin(), dest.end(), src.begin(), src.end(), back_inserter(tmp));
	dest.swap(tmp);
}

void LR1::build_automaton()
{
//	cout << "LR1::build_automaton()\n";
	
	bool keep_log=WhaleData::data.variables.dump_lr_automaton_to_file;
	ofstream log;
	if(keep_log)
	{
		string log_file_name=WhaleData::data.file_name+string(".lr1");
		log.open(log_file_name.c_str());
		log << "LR(1) automaton.\n";
	}
	
	State &I0=*new State;
	I0.kernel_items.insert(Item(0, 0, WhaleData::data.eof_terminal_number));
	search_engine[&I0.kernel_items]=states.size();
	states.push_back(&I0);
	
	for(int i=0; i<states.size(); i++)
	{
//		cout << "\tProcessing state " << i << "\n";
		closure(*states[i]);
		
		assert(WhaleData::data.lr_automaton.states.size()==i);
		WhaleData::data.lr_automaton.states.push_back(LRAutomaton::State());
		LRAutomaton::State &state_in_final_automaton=WhaleData::data.lr_automaton.states[i];
		
		// reduces.
		vector<set<int> > reduces(WhaleData::data.terminals.size());
		State &I=*states[i];
		for(sorted_vector<Item>::const_iterator p=I.kernel_items.begin(); p!=I.kernel_items.end(); p++)
		{
			RuleData &rule=WhaleData::data.rules[p->rule];
			
			if(p->dot_position==rule.body.size())
				for(sorted_vector<int>::const_iterator p2=p->lookaheads.begin(); p2!=p->lookaheads.end(); p2++)
					reduces[*p2].insert(p->rule);
		}
		for(map<int, sorted_vector<int> >::const_iterator p=I.nonterminals_added_by_closure.begin(); p!=I.nonterminals_added_by_closure.end(); p++)
		{
			NonterminalData &nonterminal=WhaleData::data.nonterminals[p->first];
			
			for(int j=nonterminal.first_rule; j<nonterminal.last_rule; j++)
				if(!WhaleData::data.rules[j].body.size())
				{
					for(sorted_vector<int>::const_iterator p2=p->second.begin(); p2!=p->second.end(); p2++)
						reduces[*p2].insert(j);
					break;
				}
		}
		
		// transitions.
		for(int j=0; j<2; j++) // pass 0, terminals; pass 1, nonterminals.
		{
			int limit=(j ? WhaleData::data.nonterminals.size() : WhaleData::data.terminals.size());
			for(int k=0; k<limit; k++)
			{
				SymbolNumber sn=(j ? SymbolNumber::for_nonterminal(k) : SymbolNumber::for_terminal(k));
				int target_state=transition(i, sn);
			#ifdef DEBUG_TRANSITIONS
				cout << "From " << i << " to " << target_state << " on " << WhaleData::data.full_symbol_name(sn, false) << "\n";
			#endif
				if(sn.is_terminal())
				{
					LRAction action=LRAction::error();
					
					if(target_state>=0 && reduces[k].size()==0)
					{
						action=LRAction::shift(target_state);
					#ifdef DEBUG_ACTIONS
						cout << "(shift " << target_state << ")\n";
					#endif
					}
					else if(target_state==-1 && reduces[k].size()==1)
					{
						action=LRAction::reduce(*reduces[k].begin());
					#ifdef DEBUG_ACTIONS
						cout << "(reduce " << *reduces[k].begin() << ")\n";
					#endif
					}
					else if(target_state==-1 && reduces[k].size()==0)
					{
						action=LRAction::error();
					#ifdef DEBUG_ACTIONS
						cout << "(error)\n";
					#endif
					}
					else
					{
						action=process_lr_conflict(i, k, target_state, vector<int>(reduces[k].begin(), reduces[k].end()));
					#ifdef DEBUG_ACTIONS
						cout << "(conflict: shift " << target_state << ", reduce " << reduces[k] << ")\n";
					#endif
					}
					
					state_in_final_automaton.action_table.push_back(action.get_n());
				}
				else if(sn.is_nonterminal())
					state_in_final_automaton.goto_table.push_back(target_state);
				else
					assert(false);
			}
		}
		
		assert(state_in_final_automaton.action_table.size()==WhaleData::data.terminals.size());
		assert(state_in_final_automaton.goto_table.size()==WhaleData::data.nonterminals.size());
		
		if(keep_log)
		{
			log << "\n";
			log << "\tState " << i << ".\n";
			log << "kernel items: " << states[i]->kernel_items << "\n";
			if(states[i]->nonterminals_added_by_closure.size())
			{
				log << "added by closure: ";
				custom_print(log, states[i]->nonterminals_added_by_closure.begin(), states[i]->nonterminals_added_by_closure.end(), AddedByClosurePrinter(), ", ", ", ");
				log << "\n";
			}
			log << "reduces: " << reduces << "\n";
		}
	}
	
	cout << states.size() << " LR(1) states.\n";
	
	if(keep_log)
		print_statistics(log);
	
	print_conflicts();
}

void LR1::closure(LR1::State &I)
{
//	cout << "LR1::closure()\n";
	
	for(sorted_vector<Item>::const_iterator p=I.kernel_items.begin(); p!=I.kernel_items.end(); p++)
	{
		RuleData &rule=WhaleData::data.rules[p->rule];
		
		if(p->dot_position<rule.body.size() && rule.body[p->dot_position].is_nonterminal())
		{
			int nn=rule.body[p->dot_position].nonterminal_number();
			bool nullable=first(I.nonterminals_added_by_closure[nn],
				rule.body.data()+p->dot_position+1,
				rule.body.size()-p->dot_position-1);
			if(nullable)
				union_assign(I.nonterminals_added_by_closure[nn], p->lookaheads);
		}
	}
	
	bool repeat_flag=true;
	while(repeat_flag)
	{
		repeat_flag=false;
		for(map<int, sorted_vector<int> >::iterator p=I.nonterminals_added_by_closure.begin(); p!=I.nonterminals_added_by_closure.end(); p++)
		{
			NonterminalData &nonterminal=WhaleData::data.nonterminals[p->first];
			
			for(int i=nonterminal.first_rule; i<nonterminal.last_rule; i++)
			{
				RuleData &rule=WhaleData::data.rules[i];
				
				if(rule.body.size()>0 && rule.body[0].is_nonterminal())
				{
					int nn=rule.body[0].nonterminal_number();
					sorted_vector<int> &lookaheads=I.nonterminals_added_by_closure[nn];
					int old_size=lookaheads.size();
					bool nullable=first(lookaheads,
						rule.body.data()+1,
						rule.body.size()-1);
					if(nullable)
						union_assign(lookaheads, p->second);
					if(lookaheads.size()>old_size)
						repeat_flag=true;
				}
			}
		}
	}
}

int LR1::transition(int from, SymbolNumber sn)
{
	State &I=*states[from];
	
	State &new_I=*new State;
	sorted_vector<Item> &new_kernel_items=new_I.kernel_items;
	
	// i. moving dot in kernel items further.
	for(sorted_vector<Item>::const_iterator p=I.kernel_items.begin(); p!=I.kernel_items.end(); p++)
	{
		RuleData &rule=WhaleData::data.rules[p->rule];
		if(p->dot_position<rule.body.size() && rule.body[p->dot_position]==sn)
		{
			Item item(*p);
			item.dot_position++;
			new_kernel_items.insert(item);
		}
	}
	
	// ii. moving dot in nonkernel items to position 1.
	for(map<int, sorted_vector<int> >::iterator p=I.nonterminals_added_by_closure.begin(); p!=I.nonterminals_added_by_closure.end(); p++)
	{
		NonterminalData &nonterminal=WhaleData::data.nonterminals[p->first];
		
		for(int i=nonterminal.first_rule; i<nonterminal.last_rule; i++)
			if(WhaleData::data.rules[i].body.size()>=1 && WhaleData::data.rules[i].body[0]==sn)
				new_kernel_items.insert(Item(i, 1, p->second));
	}
	
	if(!new_kernel_items.size())
		return -1;
	
	map<sorted_vector<Item> *, int, SortedVectorOfItemsCompare>::const_iterator p=search_engine.find(&new_kernel_items);
	int to;
	if(p!=search_engine.end())
	{
		to=p->second;
		delete &new_I;
	}
	else
	{
		to=states.size();
		search_engine[&new_kernel_items]=to;
		states.push_back(&new_I);
	}
	
	return to;
}

ostream &operator <<(ostream &os, const LR1::Item &item)
{
	RuleData &rule=WhaleData::data.rules[item.rule];

	os << "[" << WhaleData::data.nonterminals[rule.nn].name << " -> ";
	if(rule.body.size())
	{
		for(int i=0; i<rule.body.size(); i++)
		{
			if(i) os << " ";
			if(i==item.dot_position) os << ". ";
			os << WhaleData::data.symbol_name(rule.body[i]);
		}
		if(item.dot_position==rule.body.size()) os << " .";
	}
	else
		os << ".";
	
	os << ", ";
	os << "{";
	assert(item.lookaheads.size());
	custom_print(os, item.lookaheads.begin(), item.lookaheads.end(), TerminalNamePrinter(), ", ", ", ");
	os << "}";
	os << "]";
	return os;
}

#if 0
void LR1::print(ostream &os)
{
	os << "LR(1) automaton.\n";
	
	for(int i=0; i<states.size(); i++)
	{
		State &I=*states[i];
		os << "\n";
		os << "\tState " << i << ".\n";
		os << "kernel items: " << I.kernel_items << "\n";
		if(I.nonterminals_added_by_closure.size())
		{
			os << "added by closure: ";
			bool flag=false;
			for(map<int, sorted_vector<int> >::const_iterator p=I.nonterminals_added_by_closure.begin(); p!=I.nonterminals_added_by_closure.end(); p++)
			{
				if(flag)
					os << ", ";
				else
					flag=true;
				os << WhaleData::data.nonterminals[p->first].name << "(";
				assert(p->second.size());
				custom_print(os, p->second.begin(), p->second.end(), TerminalNamePrinter(), ", ", ", ");
				os << ")";
			}
			os << "\n";
		}
	}
	os << "\n\n";
	print_statistics(os);
}
#endif

void LR1::print_statistics(ostream &os)
{
	os << "\n\n\tLR(1) automaton statistics.\n";
	
	os << states.size() << " states.\n";
	
	int total_kernel_items=0, total_nonterminals_added_by_closure=0;
	int states_that_have_nonterminals_added_by_closure=0;
	int total_lookaheads=0;
	for(int i=0; i<states.size(); i++)
	{
		State &state=*states[i];
		total_kernel_items+=state.kernel_items.size();
		total_nonterminals_added_by_closure+=state.nonterminals_added_by_closure.size();
		if(state.nonterminals_added_by_closure.size()>0)
			states_that_have_nonterminals_added_by_closure++;
		for(sorted_vector<Item>::iterator p=state.kernel_items.begin(); p!=state.kernel_items.end(); p++)
		{
			total_lookaheads+=p->lookaheads.size();
		}
	}
	os << "Total: " << total_kernel_items << " kernel items,\n\t"
		<< total_nonterminals_added_by_closure << " n.a.b.c.,\n\t"
		<< total_lookaheads << " lookaheads.\n";
	os << states_that_have_nonterminals_added_by_closure << " have n.a.b.c.\n";
	os << "Average: " << double(total_kernel_items)/states.size()
		<< " k.i. per state,\n\t"
		<< double(total_nonterminals_added_by_closure)/states.size()
		<< " n.a.b.c. per state,\n\t"
		<< double(total_nonterminals_added_by_closure)/states_that_have_nonterminals_added_by_closure
		<< " n.a.b.c. per state involved,\n\t"
		<< double(total_lookaheads)/states.size()
		<< " lookaheads per state,\n\t"
		<< double(total_lookaheads)/total_kernel_items
		<< " lookaheads per kernel item.\n";
}
