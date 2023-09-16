
#define NFA_TO_DFA_VERSION "6"

// NFA to DFA conversion library.
// (C) Alexander Okhotin <whale@aha.ru>, 1999-2000.
// (C) Vladimir Prus <ghost@cs.msu.su>, 2000.

#ifdef __BORLANDC__
#define _RWSTD_CONTAINER_BUFFER_SIZE 1024
#endif

static const int _bubble_threshold = 6;

#include <cstddef>
#include <cstdlib>

using namespace std;

#include "dolphin.h"
#include "nfa-to-dfa.h"
#include "nfa-to-dfa-stl.h"
#include "stl.h"

#include <stack>
#include <queue>
#include <fstream>

//#define DFA_DEBUG_EPSILON_CLOSURE
//#define DFA_DEBUG_SUBSET_CONSTRUCTION
//#define DFA_DEBUG_SUBSET_INDEX_LOOKUP
//#define DFA_DEBUG_DEAD_STATES
//#define DFA_THOROUGHLY_DEBUG_DEAD_STATES
//#define DEBUG_DFA_MINIMIZATION


// (AO) The following comment belongs to VP. I don't understand a single word
// in it.

/*	Internal data structures used to speed up convert_nfa_to_dfa
	I belive that the biggest problem is that 1-2 transitions on
	one symbol exists, thus if we proceess each symbol
	individually, STL algorithm call themself can take more time
	then actuall processing
	*/

struct internal_nfa_state
{
	int first_transition, last_transition;
	int first_epsilon_transition, last_epsilon_transition;
	
	internal_nfa_state(int first_transition, int last_transition) :
		first_transition(first_transition),
		last_transition(last_transition)
	{
	}	
};

struct internal_dfa_state
{
	vector<int> nfa_states;
	
	internal_dfa_state() : nfa_states(10) { nfa_states.clear(); }
	static int hash(const internal_dfa_state &ds)
	{
		return accumulate(ds.nfa_states, 0);
	}
	friend bool operator<(const internal_dfa_state &s1, const internal_dfa_state &s2)
	{
		return s1.nfa_states < s2.nfa_states;
	}
	friend bool operator==(const internal_dfa_state &s1, const internal_dfa_state &s2)
	{
		return s1.nfa_states == s2.nfa_states;
	}
};

template<class lookup_structure> struct internal_nfa_dfa
{
	NFA &nfa;
	
//	vector<int> epsilon_transitions;
	lookup_structure cache;
	typedef typename lookup_structure::iterator cache_iterator;
	vector<cache_iterator> dfa_state_aux;
	vector<bool> dfa_accepting;
	
	typedef vector<pair<int, int> > one_state_data;
	typedef vector<pair<int, int> >::iterator one_state_data_iterator;
	
	vector<vector<int> > pred;
	
	vector<bool> accepting_state_is_reachable;
	
	int rejected_states;
	
	internal_nfa_dfa(NFA &nfa);
	void make_epsilon_transitions(NFA &nfa);
	void follow_epsilon(const vector<int> &nfa_states, vector<int> &closure);
	
	int add_dfa_state(vector<int> &nfa_states)
	{
		bool dfa_state_is_accepting(false);
		for(vector<int>::iterator i=nfa_states.begin(); i!=nfa_states.end(); i++)
			if(nfa.states[*i].is_accepting)
			{
				dfa_state_is_accepting = true;
				break;
			}
		
		internal_dfa_state ds;
		ds.nfa_states=nfa_states;
		cache_iterator i=cache.find(ds);
		if(i!=cache.end())
			return i->second;
		else
		{
			pair<cache_iterator, bool> p=cache.insert(make_pair(ds, dfa_state_aux.size()));
			dfa_state_aux.push_back(p.first);
			dfa_accepting.push_back(dfa_state_is_accepting);
			pred.push_back(vector<int>(8));
			pred.back().clear();
			return dfa_state_aux.size()-1;
		}
	}
	const vector<int>& nfa_states_for_dfa_state(int dfa_state)
	{
		return dfa_state_aux[dfa_state]->first.nfa_states;
	}
	void get_dfa_state_data(int dfa_state, one_state_data &osd)
	{
		cache_iterator i=dfa_state_aux[dfa_state];
		for(vector<int>::const_iterator j=i->first.nfa_states.begin(); j!=i->first.nfa_states.end(); j++)
			copy(nfa.states[*j].transitions.begin(), nfa.states[*j].transitions.end(), back_inserter(osd));
		//sort(osd.begin(), osd.end(), greater<pair<int, int> >());
		sort(osd, greater< pair<int, int> >());
		unique_and_erase(osd);
	}
	int get_transitions_for_one_symbol(one_state_data &osd, vector<int> &nstates)
	{
		if(osd.empty()) return -1;
		one_state_data_iterator i=osd.end()-1;
		int sym=i->first;
		while(i->first==sym && i>=osd.begin())
			nstates.push_back((i--)->second);
		i++;
		osd.erase(i, osd.end());
		make_set(nstates);
		return sym;		   
	}
	void mark_accessibility(int dfa_state, int target_state)
	{
		pred[target_state].push_back(dfa_state);
	}
	void find_dead_states()
	{
		// Tells us we have seen this node
		vector<bool> stacked(dfa_accepting);
		// Initial nodes
		vector<int> acc=find_all_indices(stacked, true);
		// Nodes that we've seen but haven't processed yet
		stack<int, vector<int> > s(acc);
		
		while(!s.empty())
		{
			int t=s.top(); s.pop();
			vector<int> &p=pred[t];
			for(vector<int>::iterator i=p.begin(); i!=p.end(); i++)
				if(!stacked[*i])
				{
					stacked[*i]=true;
					s.push(*i);
				}
		}
		accepting_state_is_reachable.swap(stacked);
	}

	// Just the same as partition from 'nfa-to-dfa.h' but uses vectors
	// everywhere. I wanted to retain the original version for
	// testing purposes and redeclared the struct here
	// See comments below for field explanation
	struct Partition
	{
		std::vector<int> state_to_group;
		std::vector<std::vector<int> > groups;
		std::vector<int> group_to_original_group;
		int group_containing_initial_state;

		Partition(const DFA &dfa, vector<bool> &accepting_state_is_reachable) : groups(2)
		{
			// 3 initial groups - acc, nonacc and dead(ignored)
			
			vector<bool>::iterator a = accepting_state_is_reachable.begin();
			for(vector<DFA::State>::const_iterator i=dfa.states.begin(); i!=dfa.states.end(); i++, a++)
			{
				int group_no=(i->is_accepting ? 1 : ((*a) ? 0 : -1));
				// AO: What does _THAT_ mean?
				//dfa.state_to_group.push_back;
				if(group_no!=-1)
					groups[group_no].push_back(distance(dfa.states.begin(), i));
			}		
		}
	};
	void final_fixup(DFA &dfa)
	{
		for(unsigned int i=0; i<dfa_state_aux.size(); i++)
		{
			vector<int> &v=const_cast<vector<int> &>(nfa_states_for_dfa_state(i));
			dfa.states[i].nfa_states.swap(v);
			dfa.states[i].is_accepting=dfa_accepting[i];
		}
		dfa.dead_states=accepting_state_is_reachable;
	}
	void print_dfa_statistics(DFA &dfa, ostream &os)
	{
		os << "DFA has " << dfa.states.size() << " states\n";
		int count=0;
		for (int i=0; i<pred.size(); i++)
			count+=pred[i].size();
		os << "State has " << fixed << float(count)/pred.size() 
			<< " predecessors on average\n";
	}
};

template<class lookup_structure> internal_nfa_dfa<lookup_structure>::internal_nfa_dfa(NFA &nfa) : nfa(nfa), rejected_states(0)
{
}

template<class lookup_structure> void internal_nfa_dfa<lookup_structure>::make_epsilon_transitions(NFA &nfa)
{
	for(unsigned int s=0; s<nfa.states.size(); s++)
	{
		// Weed out non-important states	
		vector<int>::iterator cur = nfa.states[s].epsilon_transitions.begin(), ins = cur;
		while(cur!=nfa.states[s].epsilon_transitions.end())
		{
			if(nfa.states[*cur].is_important) 
				*ins++=*cur;
			++cur;
		}
		
		if(ins!=nfa.states[s].epsilon_transitions.end())
			nfa.states[s].epsilon_transitions.erase(ins, nfa.states[s].epsilon_transitions.end());
	}
}

template<class lookup_structure> void internal_nfa_dfa<lookup_structure>::follow_epsilon(const vector<int> &current_nfa_states, vector<int> &closure)
{
	for(vector<int>::const_iterator i=current_nfa_states.begin(); i!=current_nfa_states.end(); i++)
	{
		copy(nfa.states[*i].epsilon_transitions.begin(),
			nfa.states[*i].epsilon_transitions.end(),
			back_inserter(closure));
	}
	
	make_set(closure);	  
}

void epsilon_closure(NFA &nfa)
{
	for(unsigned int i=0; i<nfa.states.size(); i++)
	{
		nfa.states[i].epsilon_transitions.push_back(i);
		make_set(nfa.states[i].epsilon_transitions);
	}
	
	bool once_more=true;
	vector<int> more_transitions;
	vector<int> tmp;
	while(once_more)
	{
		once_more=false;
		
		for(unsigned int i=0; i<nfa.states.size(); i++)
		{
			NFA::State &state=nfa.states[i];
			
			more_transitions.clear();
			
			for(vector<int>::iterator p = state.epsilon_transitions.begin(); p != state.epsilon_transitions.end(); ++p)
				copy(nfa.states[*p].epsilon_transitions.begin(), 
					 nfa.states[*p].epsilon_transitions.end(),
					 back_inserter(more_transitions));
			
			make_set(more_transitions);
			
			unsigned int cs=state.epsilon_transitions.size();				
			
			tmp.clear();
			set_union(state.epsilon_transitions.begin(), state.epsilon_transitions.begin(),
				more_transitions.begin(), more_transitions.end(),
				back_inserter(tmp));
			
			state.epsilon_transitions.swap(tmp);
			
			once_more|=(state.epsilon_transitions.size()>cs);
		}
	}

}

void DFA::align_transition_table()
{
	for(int i=0; i<states.size(); i++)
	{
		vector<int> &transitions=states[i].transitions;
		
		while(transitions.size()<number_of_symbols)
			transitions.push_back(-1);
	}
}

#if 0
struct is_not_important : public binary_function<NFA, int, bool>
{
	bool accepting;
	is_not_important() : accepting(false)
	{
	}
	bool operator()(const NFA &nfa, int state)
	{
		accepting|=nfa.states[state].is_accepting;
		return nfa.states[state].is_important==false;
	}
};
#endif

void convert_nfa_to_dfa(NFA &nfa, DFA &dfa)
{
	if(!nfa.states.size()) return;
	
	epsilon_closure(nfa);
	
	internal_nfa_dfa<map<internal_dfa_state, int> > internal(nfa);
	internal.make_epsilon_transitions(nfa);
	
	vector<int> epsilon_scratch;
	
	dfa.number_of_symbols=nfa.number_of_symbols;
	
	DFA::State dstate0;
	
	epsilon_scratch.clear();	
	internal.follow_epsilon(vector<int>(1, 0), epsilon_scratch);
	internal.add_dfa_state(epsilon_scratch);
	
	dfa.states.push_back(dstate0);
	
	map< vector<int>, int > nfa_states_to_dfa_state;
	vector<int> target_nfa_states;
	internal_nfa_dfa< map<internal_dfa_state, int> > :: one_state_data osd;
		
	for(int i=0; i<dfa.states.size(); i++)
	{
		osd.clear();
		internal.get_dfa_state_data(i, osd);

		nfa_states_to_dfa_state.clear();

		while(true)
		{
			target_nfa_states.clear();
			int sym=internal.get_transitions_for_one_symbol(osd, target_nfa_states);
			
			if(sym==0) continue;
			if(sym==-1) break;
			if(target_nfa_states.empty()) continue;
			
			map<vector<int>, int>::iterator s=nfa_states_to_dfa_state.find(target_nfa_states);
			if(s!=nfa_states_to_dfa_state.end())
			{
				dfa.add_transition(i, s->second, sym);
				continue;
			}
			
			epsilon_scratch.clear();
			internal.follow_epsilon(target_nfa_states, epsilon_scratch);
			
			int cs = dfa.states.size();
			int target_state_number=internal.add_dfa_state(epsilon_scratch /*new_index_entry.nfa_states*/);
			if(target_state_number>=cs)
				dfa.states.push_back(DFA::State());
			internal.mark_accessibility(i, target_state_number);
			
			nfa_states_to_dfa_state[target_nfa_states]=target_state_number;
			
			dfa.add_transition(i, target_state_number, sym);
		}	
	}
	
	internal.find_dead_states();
	internal.final_fixup(dfa);
	
	dfa.align_transition_table();
}

// a state is considered dead if either it is unreachable from the initial
// state, or no final state can be reached from it.
// result[i]==false <==> i-th state is dead.
vector<bool> find_dead_states_in_dfa(DFA &dfa)
{
	if (dfa.dead_states.size()) return dfa.dead_states;

#ifdef DFA_DEBUG_DEAD_STATES
	cout << "find_dead_states_in_dfa()\n";
#endif
	int n=dfa.states.size();
	
	vector<bool> reachable_from_initial, can_lead_to_final;
#ifdef DFA_THOROUGHLY_DEBUG_DEAD_STATES
	cout << "0 is reachable from initial because it IS the initial state.\n";
#endif
	for(int i=0; i<n; i++)
	{
		reachable_from_initial.push_back(i==0);
		can_lead_to_final.push_back(dfa.states[i].is_accepting);
	#ifdef DFA_THOROUGHLY_DEBUG_DEAD_STATES
		if(dfa.states[i].is_accepting)
			cout << i << " can lead to final because it IS final.\n";
	#endif
	}
	
	for(;;)
	{
		bool flag=false;
		
		for(int i=0; i<n; i++)
		{
			vector<int> &transitions=dfa.states[i].transitions;
			if(reachable_from_initial[i])
			{
				for(int j=0; j<dfa.number_of_symbols; j++)
					if(transitions[j]!=-1 && !reachable_from_initial[transitions[j]])
					{
					#ifdef DFA_THOROUGHLY_DEBUG_DEAD_STATES
						cout << i << " is reachable from initial ==> " << transitions[j] << " also is.\n";
					#endif
						reachable_from_initial[transitions[j]]=true;
						flag=true;
					}
			}
			if(!can_lead_to_final[i])
			{
				for(int j=0; j<dfa.number_of_symbols; j++)
					if(transitions[j]!=-1 && can_lead_to_final[transitions[j]])
					{
					#ifdef DFA_THOROUGHLY_DEBUG_DEAD_STATES
						cout << transitions[j] << " can lead to final ==> " << i << " also can.\n";
					#endif
						can_lead_to_final[i]=true;
						flag=true;
						break;
					}
			}
		}
		
		if(!flag) break;
	}
	
	vector<bool> result;
	for(int i=0; i<n; i++)
		result.push_back(reachable_from_initial[i] && can_lead_to_final[i]);
	
#ifdef DFA_DEBUG_DEAD_STATES
	for(int i=0; i<n; i++)
		if(!result[i])
		{
			cout << "state " << i << " is dead because it ";
			if(!reachable_from_initial[i])
				cout << "is unreachable from initial state";
			if(!reachable_from_initial[i] && !can_lead_to_final[i])
				cout << " and ";
			if(!can_lead_to_final[i])
				cout << "cannot lead to a final state";
			cout << ".\n";
		}
#endif
	return result;
}

// partition.state_to_group[i]==j means that state i belongs to state group j.
// i \in partition.groups[j] means the same.
// if partition.state_to_group[i]==-1, then state i belongs to no state group
// and is ignored. In this case for all j not(i \in partition_groups[j]).
// partition.size() returns number of subsets in partition.
// minimize_dfa() takes an initial partition from partition.state_to_group
// (partition.groups is ignored) and completes the job of dividing the set of
// states into classes of equivalence.
// The function returns the number of those classes.
// If something is wrong with the source data, -1 is returned.
int minimize_dfa(DFA &dfa, DFA::Partition &partition)
{
#ifdef DEBUG_DFA_MINIMIZATION
	cout << "minimize_dfa(): arrived here.\n";
#endif
	if(!partition.state_to_group.size())
	{
		// if no initial partition is supplied, we shall divide the
		// states into accepting, nonaccepting and dead.
		vector<bool> live_states=find_dead_states_in_dfa(dfa);
		for(int i=0; i<dfa.states.size(); i++)
			if(!live_states[i])
				partition.state_to_group.push_back(-1);
			else if(dfa.states[i].is_accepting)
				partition.state_to_group.push_back(1);
			else
				partition.state_to_group.push_back(0);
	}
	if(partition.state_to_group.size()!=dfa.states.size()) return -1;
	
#ifdef DEBUG_DFA_MINIMIZATION
	cout << "checkpoint 1\n";
#endif
	
	for(int i=0; i<dfa.states.size(); i++)
		if(partition.state_to_group[i]!=-1)
		{
			while(partition.state_to_group[i]>=partition.groups.size())
				partition.groups.push_back(set<int>());
			partition.groups[partition.state_to_group[i]].insert(i);
		}
	
#ifdef DEBUG_DFA_MINIMIZATION
	cout << "checkpoint 2\n";
#endif
	
	partition.group_to_original_group.clear();
	for(int i=0; i<partition.groups.size(); i++)
		partition.group_to_original_group.push_back(i);
	
#ifdef DEBUG_DFA_MINIMIZATION
	cout << "checkpoint 3\n";
#endif
	
	for(;;)
	{
		bool repeat_from_the_beginning=false;
		
		// note: vector partition.groups shall grow.
		for(int i=0; i<partition.groups.size(); i++)
		{
		#ifdef DEBUG_DFA_MINIMIZATION
			cout << i << ": " << partition.groups[i] << "\n";
		#endif
			if(partition.groups[i].size()<=1) continue;
			
			for(;;)
			{
				set<int> &group=partition.groups[i];
				bool repeat_flag=false;
				
				// trying to split this group
				for(int j=0; j<dfa.number_of_symbols; j++)
				{
					// all states in this group should have transitions
					// on j to states in target_group.
					int target_state=dfa.states[*group.begin()].transitions[j];
					int target_group_number=(target_state!=-1 ? partition.state_to_group[target_state] : -1);
					int new_target_group_number=-1;
					
					set<int>::iterator p;
					bool split_flag=false;
					for(p=group.begin(); p!=group.end(); p++)
					{
						int this_target_state=dfa.states[*p].transitions[j];
						new_target_group_number=(this_target_state!=-1 ? partition.state_to_group[this_target_state] : -1);
						if(new_target_group_number!=target_group_number)
						{
						#ifdef DEBUG_DFA_MINIMIZATION
							cout << "symbol " << j << ": target group is " << new_target_group_number << " (" << target_group_number << " expected).\n";
						#endif
							split_flag=true;
							break;
						}
					}
					
					if(!split_flag) continue;
					
					// splitting.
					set<int> new_group;
					int new_group_number=partition.groups.size();
					for(; p!=group.end(); p++)
					{
						int this_target_state=dfa.states[*p].transitions[j];
						int this_target_group=(this_target_state!=-1 ? partition.state_to_group[this_target_state] : -1);
						if(this_target_group==new_target_group_number)
							new_group.insert(*p);
					}
					for(set<int>::iterator p2=new_group.begin(); p2!=new_group.end(); p2++)
					{
						group.erase(*p2);
						partition.state_to_group[*p2]=new_group_number;
					}
					
					partition.groups.push_back(new_group);
					partition.group_to_original_group.push_back(partition.group_to_original_group[i]);
//					assert(partition.groups.size()==partition.group_to_original_group.size());
					
					repeat_flag=true;
				#ifdef DEBUG_DFA_MINIMIZATION
					cout << "setting repeat flag.\n";
				#endif
					break;
				}
				
				if(!repeat_flag) break;
			#ifdef DEBUG_DFA_MINIMIZATION
				cout << "repeat\n";
				cout << i << ": " << partition.groups[i] << "\n";
			#endif
				repeat_from_the_beginning=true;
			}
		}
		if(!repeat_from_the_beginning) break;
	#ifdef DEBUG_DFA_MINIMIZATION
		cout << "repeat from the beginning\n";
	#endif
	}
	
	partition.group_containing_initial_state=partition.state_to_group[0];
	
#ifdef DEBUG_DFA_MINIMIZATION
	cout << "minimize_dfa(): about to return.\n";
#endif
	return partition.groups.size();
}

void NFA::print(ostream &os)
{
	for(int i=0; i<states.size(); i++)
	{
		State &state=states[i];
	#if 0
		std::vector<std::pair<int, int> > transitions;

		std::vector<int> epsilon_transitions;

		bool is_accepting;
		bool is_important; // see Aho/Sethi/Ullman 1986, p. 134.
	#endif
		os << "State " << i;
		if(state.is_accepting)
			os << " (Accepting)";
		else if(state.is_important)
			os << " (Important)";
		os << ": ";
		
		bool written_smth=false;
		/*if(state.transitions.size())
		{
			os << "transitions " << state.transitions;
			written_smth=true;
		}*/
		if(state.epsilon_transitions.size())
		{
			if(written_smth) os << "; ";
			os << "epsilon transitions to " << state.epsilon_transitions;
			written_smth=true;
		}
		if(!written_smth)
			os << "No outgoing transitions";
		os << ".\n";
	}
}

void DFA::raw_print(ostream &os)
{
	for(int i=0; i<states.size(); i++)
	{
		State &state=states[i];
		
		os << i;
		if(state.is_accepting) os << "(Acc)";
		os << "\t";
		os << state.transitions << "\n";
	}
}
void DFA::print(ostream &os)
{
	for(int i=0; i<states.size(); i++)
	{
		State &state=states[i];
		
		os << "State " << i;
		if(state.is_accepting)
			os << " (Accepting)";
		
		if(state.nfa_states.size())
			os << " (NFA states " << state.nfa_states << ")";
		
		std::map<int, vector<int> > target_states;
		for(int j=0; j<state.transitions.size(); j++)
			if(state.transitions[j]>=0)
				target_states[state.transitions[j]].push_back(j);
		
		if(target_states.size()==0)
			os << ": No outgoing transitions.\n";
		else
		{
			os << ": ";
			
			for(std::map<int, vector<int> >::iterator p=target_states.begin(); p!=target_states.end(); p++)
			{
				if(p!=target_states.begin())
					os << ", ";
				
				if(p->second.size()==1)
					os << p->second[0];
				else
					os << p->second;
				
				os << " -> " << p->first;
			}
			os << "\n";
		}
	}
}	
