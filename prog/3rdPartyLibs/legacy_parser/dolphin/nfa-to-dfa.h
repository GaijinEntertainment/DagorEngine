
// NFA to DFA conversion library.
// (C) Alexander Okhotin <whale@aha.ru>, 1999-2000.
// (C) Vladimir Prus <ghost@cs.msu.su>, 2000.

#ifndef __DOLPHIN__NFA_TO_DFA_H

#define __DOLPHIN__NFA_TO_DFA_H

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>

#define NUMBER_OF_TRANSITIONS_TO_STORE_IN_LINEAR_MEMORY 2

//#define DEBUG_NFA
//#define DEBUG_NFA_DESTRUCTOR
//#define DEBUG_DFA
//#define NFA_TO_DFA_THROW_EXCEPTIONS


class NFA
{
	void state_should_exist(int n)
	{
		while(states.size()<=n)
			states.push_back(State(number_of_symbols));
	}
	
public:
	/** Initial size of epsilon_transitions array */
	static const int def_epsilon_transitions = 16;

	struct State
	{
		std::vector<std::pair<int, int> > transitions;

		std::vector<int> epsilon_transitions;
		
		bool is_accepting;
		bool is_important; // see Aho/Sethi/Ullman 1986, p. 134.
		
		State(int number_of_symbols) :
			epsilon_transitions(def_epsilon_transitions),
			is_accepting(false), is_important(false)
		{ epsilon_transitions.clear(); }
		
		void add_transition(int target, int symbol)
		{
			is_important=true;
			transitions.push_back(std::make_pair(symbol, target));
		}
		void add_epsilon_transition(int target)
		{
			epsilon_transitions.push_back(target);
		}
	};
	
	std::vector<State> states;
	int number_of_symbols;
	
	NFA(int supplied_number_of_symbols)
	{
		number_of_symbols=supplied_number_of_symbols;
	}

	void add_transition(int from, int to, int symbol)
	{
		state_should_exist(from);
		state_should_exist(to);
	#ifdef NFA_TO_DFA_THROW_EXCEPTIONS
		if(symbol>=number_of_symbols)
			throw std::logic_error("NFA::add_transition(): Symbol number out of range");
	#endif
		states[from].add_transition(to, symbol);
	#ifdef DEBUG_NFA
		std::cout << "NFA: Adding transition from " << from << " to " << to << " upon " << symbol << "\n";
	#endif
	}
	void add_epsilon_transition(int from, int to)
	{
		state_should_exist(from);
		state_should_exist(to);
		states[from].add_epsilon_transition(to);
	#ifdef DEBUG_NFA
		std::cout << "NFA: Adding transition from " << from << " to " << to << " upon epsilon\n";
	#endif
	}
	void mark_as_accepting(int n)
	{
		state_should_exist(n);
		states[n].is_accepting=true;
		states[n].is_important=true;
	}
	void mark_as_important(int n)
	{
		state_should_exist(n);
		states[n].is_important=true;
	}
	int add_state(void)
	{
		int n=states.size();
		states.push_back(State(number_of_symbols));
		return n;
	}
	void raw_print(std::ostream &os);
	void print(std::ostream &os);
};

class DFA
{
public:
	struct State
	{
		std::vector<int> nfa_states;
		std::vector<int> transitions;
		bool is_accepting;
		
		State() : is_accepting(false) {}
		
		void add_transition(int target, int symbol)
		{
			while(transitions.size()<=symbol)
				transitions.push_back(-1);
			transitions[symbol]=target;
		}
	};
	
	struct Partition
	{
		std::vector<int> state_to_group;
		std::vector<std::set<int> > groups;
		std::vector<int> group_to_original_group;
		int group_containing_initial_state;
	};
	
	std::vector<State> states;
	int number_of_symbols;
	
	std::vector<bool> dead_states;
	
	void add_transition(int from, int to, int symbol)
	{
		states[from].add_transition(to, symbol);
	#ifdef DEBUG_DFA
		std::cout << "DFA: Adding transition from " << from << " to " << to << " upon " << symbol << "\n";
	#endif
	}
	void align_transition_table();
	void raw_print(std::ostream &os);
	void print(std::ostream &os);
};


void epsilon_closure(NFA &nfa);
void convert_nfa_to_dfa(NFA &nfa, DFA &dfa);
std::vector<bool> find_dead_states_in_dfa(DFA &dfa);
int minimize_dfa(DFA &dfa, DFA::Partition &partition);

#undef DEBUG_NFA
#undef DEBUG_DFA

#endif
