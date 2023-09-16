
#include "dolphin.h"
#include "dfa.h"
using namespace std;
using namespace Whale;

#include <fstream>
#include "stl.h"
#include "time.h"
#include "process.h"

//#define DEBUG_STATE_GROUP_CREATION
//#define PRINT_SETS_OF_NFA_STATES_CORRESPONDING_TO_DFA_STATES
//#define PRINT_DFA_PARTITION
//#define PRINT_TRANSITION_TABLE

// Initial dfa partition shall consist of the following state groups:
// A group of all nonaccepting live states.
// Groups of accepting live states. Each of these groups shall contain states
//	that accept the same set of recognized expressions.
// (if lookahead is used) A group of lookahead states.
// Other groups shall be created by optimize_dfa(). See nfa-to-dfa.cpp.
// Returns false if some error was found, true otherwise
bool analyze_accepting_states_and_make_initial_partition()
{
	TimeKeeper tk("accepting states");
	
	if(find(DolphinData::data.live_states_in_dfa.begin(), DolphinData::data.live_states_in_dfa.end(),
		true)==DolphinData::data.live_states_in_dfa.end())
	{
		cout << "All states in DFA are dead.\n";
		return false;
	}
	
#ifdef PRINT_SETS_OF_NFA_STATES_CORRESPONDING_TO_DFA_STATES
	for(int j=0; j<DolphinData::data.dfa.states.size(); j++)
	{
		DFA::State &state=DolphinData::data.dfa.states[j];
		cout << "DFA state " << j << ": NFA states " << state.nfa_states << "\n";
	}
#endif
	
	vector<bool> lookahead_states_in_dfa(DolphinData::data.dfa.states.size(), false);
	
	for(int i=0; i<DolphinData::data.recognized_expressions.size(); i++)
	{
		RecognizedExpressionData &re=DolphinData::data.recognized_expressions[i];
		if(re.is_special) continue;
		
		assert(re.final_nfa_state>=0);
		#define implication(x, y) (x || !y)
		assert(implication(re.lookahead, (re.intermediate_nfa_state>=0)));
		#undef implication
		
		for(int j=0; j<DolphinData::data.dfa.states.size(); j++)
		{
			DFA::State &state=DolphinData::data.dfa.states[j];
			if(!DolphinData::data.live_states_in_dfa[j]) continue;
			
			if(/* re.lookahead_length==-1 && */ re.intermediate_nfa_state>=0 &&
				find(state.nfa_states.begin(), state.nfa_states.end(), re.intermediate_nfa_state)!=state.nfa_states.end())
			{
				lookahead_states_in_dfa[j]=true;
				re.intermediate_dfa_states.insert(j);
			}
			if(state.is_accepting && find(state.nfa_states.begin(), state.nfa_states.end(), re.final_nfa_state)!=state.nfa_states.end())
				re.dfa_states.insert(j);
		}
		
	//	cout << i << ": ";
	//	if(re.intermediate_nfa_state>=0)
	//		cout << re.intermediate_dfa_states << " / ";
	//	cout << re.dfa_states << "\n";
	}
	
	assert(!DolphinData::data.action_vectors_in_state_groups.size());
	
	// state_group_assignment_precedents[v]=j means that if the sorted
	// vector of expessions recognized in some dfa state i equals v, then
	// state i should be assigned to state group j.
	map<vector<int>, int> expressions_to_state_group_precedents;
	
	// actions_in_start_conditions_to_state_group_precedents[v]=j means
	// that if the sorted vector of actions in start conditions for some
	// dfa state i equals v, then state i should be assigned to state
	// group j.
	map<vector<int>, int> actions_in_start_conditions_to_state_group_precedents;
	
	// conflicts_found[s1] = { sc | sc is a start condition and in some
	// dfa state i in start condition sc there was a conflict between all
	// actions contained in s1 }
	map<set<int>, set<int> > conflicts_found;
	
	// nonaccepting_state_group=-1 if there is no nonaccepting state
	// group yet. The same with lookahead_state_group.
	int nonaccepting_state_group=-1;
	int lookahead_state_group=-1;
	
	for(int i=0; i<DolphinData::data.dfa.states.size(); i++)
	{
		DFA::State &state=DolphinData::data.dfa.states[i];
		if(!DolphinData::data.live_states_in_dfa[i])
			DolphinData::data.dfa_partition.state_to_group.push_back(-1);
		else if(!state.is_accepting)
		{
			if(!lookahead_states_in_dfa[i])
			{
				if(nonaccepting_state_group==-1)
				{
				#ifdef DEBUG_STATE_GROUP_CREATION
					cout << "Creating nonaccepting state group: " << DolphinData::data.action_vectors_in_state_groups.size() << "\n";
				#endif
					nonaccepting_state_group=DolphinData::data.action_vectors_in_state_groups.size();
					DolphinData::data.action_vectors_in_state_groups.push_back(vector<int>(DolphinData::data.start_conditions.size(), -1));
				}
				DolphinData::data.dfa_partition.state_to_group.push_back(nonaccepting_state_group);
			}
			else
			{
				if(lookahead_state_group==-1)
				{
				#ifdef DEBUG_STATE_GROUP_CREATION
					cout << "Creating lookahead state group: " << DolphinData::data.action_vectors_in_state_groups.size() << "\n";
				#endif
					lookahead_state_group=DolphinData::data.action_vectors_in_state_groups.size();
					DolphinData::data.action_vectors_in_state_groups.push_back(vector<int>(DolphinData::data.start_conditions.size(), -1));
				}
				DolphinData::data.dfa_partition.state_to_group.push_back(lookahead_state_group);
			}
		}
		else
		{
			assert(DolphinData::data.live_states_in_dfa[i] && state.is_accepting);
			
			vector<int> expressions_recognized_here;
			for(int j=0; j<DolphinData::data.recognized_expressions.size(); j++)
			{
				RecognizedExpressionData &re=DolphinData::data.recognized_expressions[j];
				if(!re.is_special && re.dfa_states.count(i))
					expressions_recognized_here.push_back(j);
			}
			assert(expressions_recognized_here.size());
			
			map<vector<int>, int>::iterator p=expressions_to_state_group_precedents.find(expressions_recognized_here);
			if(p!=expressions_to_state_group_precedents.end())
				DolphinData::data.dfa_partition.state_to_group.push_back((*p).second);
			else
			{
				vector<int> actions_recognized_in_start_conditions;
				
				for(int j=0; j<DolphinData::data.start_conditions.size(); j++)
				{
					set<int> rival_actions;
					
					for(int k=0; k<expressions_recognized_here.size(); k++)
					{
						RecognizedExpressionData &re=DolphinData::data.recognized_expressions[expressions_recognized_here[k]];
						int an=re.start_condition_to_action_number[j];
						if(an!=-1) rival_actions.insert(an);
					}
//					cout << "state " << i << ", sc " << j << ": " << rival_actions << "\n";
					
					if(!rival_actions.size())
						actions_recognized_in_start_conditions.push_back(-1);
					else if(rival_actions.size()==1)
						actions_recognized_in_start_conditions.push_back(*(rival_actions.begin()));
					else
					{
						actions_recognized_in_start_conditions.push_back(*(rival_actions.begin()));
						conflicts_found[rival_actions].insert(j);
					}
				}
				
				map<vector<int>, int>::iterator p2=actions_in_start_conditions_to_state_group_precedents.find(actions_recognized_in_start_conditions);
				if(p2!=actions_in_start_conditions_to_state_group_precedents.end())
					DolphinData::data.dfa_partition.state_to_group.push_back((*p2).second);
				else
				{
					int new_state_group=DolphinData::data.action_vectors_in_state_groups.size();
				#ifdef DEBUG_STATE_GROUP_CREATION
					cout << "Creating state group " << new_state_group << " = " << actions_recognized_in_start_conditions << "\n";
				#endif
					DolphinData::data.action_vectors_in_state_groups.push_back(actions_recognized_in_start_conditions);
					
					DolphinData::data.dfa_partition.state_to_group.push_back(new_state_group);
					
					// registering this state group in
 					// our local index structures.
					expressions_to_state_group_precedents[expressions_recognized_here]=new_state_group;
					actions_in_start_conditions_to_state_group_precedents[actions_recognized_in_start_conditions]=new_state_group;
				}
			}
		}
	}
	
	int dfa_initial_state=0;
	bool initial_state_is_accepting=DolphinData::data.dfa.states[dfa_initial_state].is_accepting;
	
	if(nonaccepting_state_group==-1)
	{
		cout << "Warning: all states in DFA, including the initial state, are accepting.\n";
	}
	else if(initial_state_is_accepting)
	{
		cout << "Warning: initial state of DFA is accepting.\n";
	}
	
	// reporting conflicts, if there were any.
	for(map<set<int>, set<int> >::iterator p=conflicts_found.begin(); p!=conflicts_found.end(); p++)
	{
		const set<int> &rival_actions=(*p).first;
		set<int> &start_conditions=(*p).second;
		assert(rival_actions.size()>=2 && start_conditions.size()>=1);
		
		cout << "Ambiguity between ";
		
		set<int> expressions_involved;
		for(set<int>::const_iterator p=rival_actions.begin(); p!=rival_actions.end(); p++)
			expressions_involved.insert(DolphinData::data.actions[*p].recognized_expression_number);
		assert(expressions_involved.size());
		
		print_a_number_of_expressions(cout, expressions_involved);
		
		if(DolphinData::data.start_conditions.size()>1)
		{
			if(DolphinData::data.start_conditions.size()==start_conditions.size())
				cout << " in all start conditions";
			else
			{
				bool more_than_one=(start_conditions.size()>1);
				cout << " in start condition";
				if(more_than_one) cout << "s";
				cout << " ";
				
				print_a_number_of_start_conditions(cout, start_conditions);
			}
		}

		cout << "; conflicting actions defined ";
		vector<Terminal *> action_locations;
		for(set<int>::const_iterator p2=rival_actions.begin(); p2!=rival_actions.end(); p2++)
			action_locations.push_back(DolphinData::data.actions[*p2].declaration->arrow);
		print_a_number_of_terminal_locations(cout, action_locations, "at");
		cout << ".\n";
	}
	
	// reporting actions that will never be executed
	set<int> actions_that_will_never_be_executed;
	for(int i=0; i<DolphinData::data.actions.size(); i++)
		if(!DolphinData::data.actions[i].is_special)
			actions_that_will_never_be_executed.insert(i);
	for(int i=0; i<DolphinData::data.action_vectors_in_state_groups.size(); i++)
		for(int j=0; j<DolphinData::data.start_conditions.size(); j++)
		{
			int an=DolphinData::data.action_vectors_in_state_groups[i][j];
			if(an!=-1)
				actions_that_will_never_be_executed.erase(an);
		}
	if(actions_that_will_never_be_executed.size())
	{
		set<int> expressions_involved;
		for(set<int>::iterator p=actions_that_will_never_be_executed.begin(); p!=actions_that_will_never_be_executed.end(); p++)
			expressions_involved.insert(DolphinData::data.actions[*p].recognized_expression_number);
		assert(expressions_involved.size());
		bool more_than_one_action=(actions_that_will_never_be_executed.size()>1);
		bool more_than_one_expression=(expressions_involved.size()>1);
		
		cout << "Action";
		if(more_than_one_action) cout << "s";
		cout << " for expression";
		if(more_than_one_expression) cout << "s";
		cout << " ";
		
		print_a_number_of_expressions(cout, expressions_involved);
		
		cout << ", defined ";
		
		vector<Terminal *> places_of_definition;
		for(set<int>::iterator p=actions_that_will_never_be_executed.begin(); p!=actions_that_will_never_be_executed.end(); p++)
			places_of_definition.push_back(DolphinData::data.actions[*p].declaration->arrow);
		
		print_a_number_of_terminal_locations(cout, places_of_definition, "at");
		
		cout << ", will never be executed.\n";
	}
	
	// reporting start conditions in which no expression can be recognized
	vector<int> dead_start_conditions;
	for(int j=0; j<DolphinData::data.start_conditions.size(); j++)
	{
		bool flag=false;
		
		for(int i=0; i<DolphinData::data.action_vectors_in_state_groups.size(); i++)
		{
			if(DolphinData::data.action_vectors_in_state_groups[i][j]!=-1)
			{
				flag=true;
				break;
			}
		}
		
		if(!flag)
			dead_start_conditions.push_back(j);
	}
	if(dead_start_conditions.size())
	{
		cout << "No expressions are recognized in start condition";
		if(dead_start_conditions.size()>1) cout << "s";
		cout << " ";
		
		for(int j=0; j<dead_start_conditions.size(); j++)
		{
			if(j) cout << (j<dead_start_conditions.size()-1 ? ", " : " and ");
			cout << DolphinData::data.start_conditions[dead_start_conditions[j]].name;
		}
		
		cout << ".\n";
	}
	
	return true;
}

void build_final_transition_table()
{
	// building an uncompressed transition table first.
	int n=DolphinData::data.dfa_partition.groups.size();
	matrix<int> uncompressed_transition_table(n, DolphinData::data.variables.alphabet_cardinality);
	
	for(int i=0; i<n; i++)
	{
		int representative_state_number=*DolphinData::data.dfa_partition.groups[i].begin();
//		cout << "group " << i << ": representative_state " << representative_state_number << "\n";
		DFA::State &state=DolphinData::data.dfa.states[representative_state_number];
		
		for(int j=0; j<DolphinData::data.variables.alphabet_cardinality; j++)
		{
			int target_dfa_state=state.transitions[j];
			
			if(target_dfa_state!=-1)
			{
				uncompressed_transition_table[i][j]=DolphinData::data.dfa_partition.state_to_group[target_dfa_state];
//				cout << "(" << i << ", " << j << "): " << uncompressed_transition_table[i][j] << "\n";
			}
			else
				uncompressed_transition_table[i][j]=-1;
		}
	}
	
	DolphinData::data.number_of_symbol_classes=0;
	assert(!DolphinData::data.symbol_to_symbol_class.size());
	for(int j=0; j<DolphinData::data.variables.alphabet_cardinality; j++)
		DolphinData::data.symbol_to_symbol_class.push_back(-1);
	vector<int> symbol_class_to_symbol;
	
	for(int j=0; j<DolphinData::data.variables.alphabet_cardinality; j++)
		if(DolphinData::data.symbol_to_symbol_class[j]==-1)
		{
			DolphinData::data.symbol_to_symbol_class[j]=DolphinData::data.number_of_symbol_classes;
			
			for(int k=j+1; k<DolphinData::data.variables.alphabet_cardinality; k++)
			{
				bool flag=true;
				for(int i=0; i<n; i++)
					if(uncompressed_transition_table[i][k]!=uncompressed_transition_table[i][j])
					{
						flag=false;
						break;
					}
				
				if(flag)
					DolphinData::data.symbol_to_symbol_class[k]=DolphinData::data.number_of_symbol_classes;
			}
			
			symbol_class_to_symbol.push_back(j);
			DolphinData::data.number_of_symbol_classes++;
		}
	
	DolphinData::data.final_transition_table.destructive_resize(n, DolphinData::data.number_of_symbol_classes);
	for(int i=0; i<n; i++)
		for(int j=0; j<DolphinData::data.number_of_symbol_classes; j++)
			DolphinData::data.final_transition_table[i][j]=uncompressed_transition_table[i][symbol_class_to_symbol[j]];
	
#ifdef PRINT_TRANSITION_TABLE
	cout << "Uncompressed transition table:\n";
	for(int i=0; i<n; i++)
	{
		cout << i << "\t";
		for(int j=0; j<DolphinData::data.variables.alphabet_cardinality; j++)
			cout << uncompressed_transition_table[i][j] << " ";
		cout << "\n";
	}
	cout << "Symbol to symbol class table:\n";
	cout << DolphinData::data.symbol_to_symbol_class << "\n";
	cout << "Final transition table:\n";
	for(int i=0; i<n; i++)
	{
		cout << i << "\t";
		for(int j=0; j<DolphinData::data.number_of_symbol_classes; j++)
			cout << DolphinData::data.final_transition_table[i][j] << " ";
		cout << "\n";
	}
	cout << "build_final_transition_table(): done.\n";
#endif
}

// checking the final transition table for obvious errors, such as
// i. transitions to non-existent states.
// ii. states that no transition leads to (those besides the initial state).
void dfa_sanity_check()
{
	int number_of_states=DolphinData::data.dfa_partition.groups.size();
	int number_of_symbol_classes=DolphinData::data.number_of_symbol_classes;
	matrix<int> &table=DolphinData::data.final_transition_table;
	
	vector<bool> used_somewhere(number_of_states, false);
	used_somewhere[DolphinData::data.dfa_partition.group_containing_initial_state]=true;
	for(int i=0; i<number_of_states; i++)
		for(int j=0; j<number_of_symbol_classes; j++)
		{
			if(table[i][j]<-1 || table[i][j]>=number_of_states)
			{
				cout << "\n\n\tdfa_sanity_check():\n";
				cout << "Something went while constructing the DFA.\n";
				cout << "State group " << i << " has a transition upon symbol class " << j << " to a\n";
				cout << "non-existent state group " << table[i][j] << "\n\n";
				assert(false);
			}
			if(table[i][j]!=-1)
				used_somewhere[table[i][j]]=true;
		}
	
	set<int> unused_states;
	for(int i=0; i<number_of_states; i++)
		if(!used_somewhere[i])
			unused_states.insert(i);
	if(unused_states.size())
	{
		cout << "\n\n\tdfa_sanity_check():\n";
		cout << "Something went wrong while constructing the DFA.\n";
		cout << "State group" << (unused_states.size()==1 ? " " : "s ");
		cout << unused_states << " cannot be reached at all.\n\n";
		assert(false);
	}
}

bool construct_dfa()
{
	cout << "construct_dfa()\n";
	
	NFA &nfa=DolphinData::data.nfa;
	DFA &dfa=DolphinData::data.dfa;
	
	cout << "calling convert_nfa_to_dfa()\n";
	{
		TimeKeeper tc("convert_nfa_to_dfa");
		convert_nfa_to_dfa(nfa, dfa);
	}
	
	cout << DolphinData::data.dfa.states.size() << " DFA states.\n";
	assert(dfa.states.size());
	
	if(DolphinData::data.variables.dump_dfa_to_file>0)
	{
		ofstream log;
		string log_file_name=DolphinData::data.file_name+string(".dfa");
		log.open(log_file_name.c_str());
		log << "DFA.\n\n";
		if(DolphinData::data.variables.dump_dfa_to_file==1)
			dfa.raw_print(log);
		else if(DolphinData::data.variables.dump_dfa_to_file==2)
			dfa.print(log);
		else
			assert(false);
	}
	
	{
		TimeKeeper tc("dead states");
		DolphinData::data.live_states_in_dfa=find_dead_states_in_dfa(DolphinData::data.dfa);
	}
	
	if(!analyze_accepting_states_and_make_initial_partition())
		return false;
	
	{
		TimeKeeper tc("minimize_dfa");
		int number_of_subsets=minimize_dfa(dfa, DolphinData::data.dfa_partition);
		cout << number_of_subsets << " subsets.\n";
	}
	
#ifdef PRINT_DFA_PARTITION
	cout << "state_to_group = " << DolphinData::data.dfa_partition.state_to_group << "\n";
	cout << "groups = " << DolphinData::data.dfa_partition.groups << "\n";
#endif
	
	{
		TimeKeeper tc("build_final_transition_table");
		build_final_transition_table();
	}
	
	dfa_sanity_check(); // a postcondition
	
	return true;
}
