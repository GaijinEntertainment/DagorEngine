
#include "dolphin.h"
#include "nfa.h"
#include <fstream>
using namespace std;
using namespace Whale;

#include "nfa-to-dfa.h"
#include "stl.h"
#include "time.h"

//#define DEBUG_EXPANDED
//#define DEBUG_CONJUNCTION
//#define DEBUG_COMPLEMENT
//#define DEBUG_DISJUNCTION
//#define DEBUG_CONCATENATION
//#define DEBUG_OMITTABLE
//#define DEBUG_ITERATION
//#define DEBUG_CONDITION
//#define DEBUG_RANGE
//#define DEBUG_CONTAINS
//#define DEBUG_EPSILON
//#define DEBUG_NONTERMINAL_SYMBOL
//#define DEBUG_TERMINAL_SYMBOL

//#define PRINT_WHERE_EXPRESSIONS_ARE_RECOGNIZED

/* Some explanation is required. The function constructs an NFA for          */
/* subexpression expr and adds it to the main NFA. Main NFA states           */
/* local_initial_state and local_final_state, which numbers are passed from  */
/* above as formal parameters, are used by the function as the initial and   */
/* the final state of the sub-NFA being created.                             */
/* recursive_path stores initial state numbers for each nonterminal in the   */
/* recursive path. Each recursive use of the nonterminal shall produce an    */
/* epsilon transition to the corresponding initial state.                    */
void construct_nfa_proc(NFA &nfa, Whale::NonterminalExpression *expr,
	int local_initial_state, int local_final_state,
	map<int, int> &recursive_path)
{
	if(expr->expanded)
	{
	#ifdef DEBUG_EXPANDED
		cout << "expanded " << typeid(*expr).name() << " (from " << local_initial_state << " to "
			<< local_final_state << "): " << expr->expanded << "\n";
	#endif
		set<int> actual_symbols=set<int>(*expr->expanded & UnionOfIntervals<int>(0, DolphinData::data.variables.alphabet_cardinality-1));
		
		for(set<int>::const_iterator p=actual_symbols.begin(); p!=actual_symbols.end(); p++)
			nfa.add_transition(local_initial_state, local_final_state, *p);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
	#ifdef DEBUG_DISJUNCTION
		cout << "disjunction: from " << local_initial_state << " to " << local_final_state << "\n";
	#endif
		construct_nfa_proc(nfa, expr_d.expr1, local_initial_state,
			local_final_state, recursive_path);
		construct_nfa_proc(nfa, expr_d.expr2, local_initial_state,
			local_final_state, recursive_path);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
		
	#ifdef DEBUG_CONJUNCTION
		cout << "construct_nfa_proc(): processing conjunction.\n";
	#endif
		
		NFA nfa_for_expr1(nfa.number_of_symbols);
		int nfa1_initial_state=nfa_for_expr1.add_state();
		int nfa1_final_state=nfa_for_expr1.add_state();
		map<int, int> nfa1_recursive_path;
		construct_nfa_proc(nfa_for_expr1, expr_c.expr1,
			nfa1_initial_state, nfa1_final_state, nfa1_recursive_path);
		nfa_for_expr1.mark_as_accepting(nfa1_final_state);
		assert(!nfa1_recursive_path.size());
		
		NFA nfa_for_expr2(nfa.number_of_symbols);
		int nfa2_initial_state=nfa_for_expr2.add_state();
		int nfa2_final_state=nfa_for_expr2.add_state();
		map<int, int> nfa2_recursive_path;
		construct_nfa_proc(nfa_for_expr2, expr_c.expr2,
			nfa2_initial_state, nfa2_final_state, nfa2_recursive_path);
		nfa_for_expr2.mark_as_accepting(nfa2_final_state);
		assert(!nfa2_recursive_path.size());
		
	#ifdef DEBUG_CONJUNCTION
		cout << "construct_nfa_proc(): made two nfas ("
			<< nfa_for_expr1.states.size() << " and "
			<< nfa_for_expr2.states.size() << " states), converting them to dfas\n";
		print_nfa(nfa_for_expr1);
		print_nfa(nfa_for_expr2);
	#endif
		
		DFA dfa_for_expr1;
		convert_nfa_to_dfa(nfa_for_expr1, dfa_for_expr1);
		
		DFA dfa_for_expr2;
		convert_nfa_to_dfa(nfa_for_expr2, dfa_for_expr2);
		
	#ifdef DEBUG_CONJUNCTION
		cout << "construct_nfa_proc(): made two dfas ("
			<< dfa_for_expr1.states.size() << " and "
			<< dfa_for_expr2.states.size() << " states), now adding them to the main nfa.\n";
		print_dfa(dfa_for_expr1);
		print_dfa(dfa_for_expr2);
	#endif
		
//		int dfa1_size=dfa_for_expr1.states.size();
//		int dfa2_size=dfa_for_expr2.states.size();
		
		map<pair<int, int>, int> state_mapping;
		set<pair<int, int> > left_to_process;
		
		int our_initial_state=nfa.add_state();
		nfa.add_epsilon_transition(local_initial_state, our_initial_state);
		state_mapping[make_pair(0, 0)]=our_initial_state;
		left_to_process.insert(make_pair(0, 0));
		
		while(left_to_process.size())
		{
			pair<int, int> state_pair=*left_to_process.begin();
			int nfa_state_corresponding_to_this_state_pair=state_mapping[state_pair];
		#ifdef DEBUG_CONJUNCTION
			cout << "processing " << state_pair.first << ", " << state_pair.second << "\n";
		#endif
			
			DFA::State &dfa1_state=dfa_for_expr1.states[state_pair.first];
			DFA::State &dfa2_state=dfa_for_expr2.states[state_pair.second];
			
			for(int j=0; j<nfa.number_of_symbols; j++)
			{
				int target1=dfa1_state.transitions[j];
				int target2=dfa2_state.transitions[j];
				if(target1!=-1 && target2!=-1)
				{
					// target state is (target1, target2)
					map<pair<int, int>, int>::iterator p_target=state_mapping.find(make_pair(target1, target2));
					if(p_target!=state_mapping.end())
						nfa.add_transition(nfa_state_corresponding_to_this_state_pair, (*p_target).second, j);
					else
					{
						int new_nfa_state=nfa.add_state();
						state_mapping[make_pair(target1, target2)]=new_nfa_state;
						left_to_process.insert(make_pair(target1, target2));
						nfa.add_transition(nfa_state_corresponding_to_this_state_pair, new_nfa_state, j);
					}
				}
			}
			
			if(dfa1_state.is_accepting && dfa2_state.is_accepting)
				nfa.add_epsilon_transition(nfa_state_corresponding_to_this_state_pair, local_final_state);
			
			left_to_process.erase(state_pair);
		}
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
		int state_in_the_middle=nfa.add_state();
	#ifdef DEBUG_CONCATENATION
		cout << "concatenation: " << local_initial_state << " -> " << state_in_the_middle << " -> " << local_final_state << "\n";
	#endif
		construct_nfa_proc(nfa, expr_cat.expr1, local_initial_state,
			state_in_the_middle, recursive_path);
		construct_nfa_proc(nfa, expr_cat.expr2, state_in_the_middle,
			local_final_state, recursive_path);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
	{
		NonterminalExpressionComplement &expr_com=*dynamic_cast<NonterminalExpressionComplement *>(expr);
		
	#ifdef DEBUG_COMPLEMENT
		cout << "construct_nfa_proc(): processing complement.\n";
	#endif
		
		NFA nfa_for_expr(nfa.number_of_symbols);
		int nfa2_initial_state=nfa_for_expr.add_state();
		int nfa2_final_state=nfa_for_expr.add_state();
		map<int, int> nfa2_recursive_path;
		construct_nfa_proc(nfa_for_expr, expr_com.expr,
			nfa2_initial_state, nfa2_final_state, nfa2_recursive_path);
		nfa_for_expr.mark_as_accepting(nfa2_final_state);
		assert(!nfa2_recursive_path.size());
		
	#ifdef DEBUG_COMPLEMENT
		cout << "construct_nfa_proc(): made an nfa (" << nfa_for_expr.states.size() << " states), converting it to dfa\n";
		print_nfa(nfa_for_expr);
	#endif
		
		DFA dfa_for_expr;
		convert_nfa_to_dfa(nfa_for_expr, dfa_for_expr);
		
	#ifdef DEBUG_COMPLEMENT
		cout << "construct_nfa_proc(): made a dfa (" << dfa_for_expr.states.size() << " states), now adding it to the main nfa.\n";
		print_dfa(dfa_for_expr);
	#endif
		
		vector<int> state_mapping; // 'dfa_for_expr' -> 'nfa'.
		for(int i=0; i<dfa_for_expr.states.size(); i++)
			state_mapping.push_back(nfa.add_state());
		nfa.add_epsilon_transition(local_initial_state, state_mapping[0]);
		
		int omniaccepting_state=nfa.add_state();
	#ifdef DEBUG_COMPLEMENT
		cout << "omniaccepting state: " << omniaccepting_state << "\n";
	#endif
		for(int j=0; j<dfa_for_expr.number_of_symbols; j++)
			nfa.add_transition(omniaccepting_state, omniaccepting_state, j);
		nfa.add_epsilon_transition(omniaccepting_state, local_final_state);
		
		for(int i=0; i<dfa_for_expr.states.size(); i++)
		{
			DFA::State &state=dfa_for_expr.states[i];
			
			if(!state.is_accepting) // then make it accepting.
				nfa.add_epsilon_transition(state_mapping[i], local_final_state);
			
			for(int j=0; j<dfa_for_expr.number_of_symbols; j++)
				if(state.transitions[j]>=0)
					nfa.add_transition(state_mapping[i], state_mapping[state.transitions[j]], j);
				else
					nfa.add_transition(state_mapping[i], omniaccepting_state, j);
		}
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
	#ifdef DEBUG_OMITTABLE
		cout << "omittable: from " << local_initial_state << " to " << local_final_state << "\n";
	#endif
		construct_nfa_proc(nfa, expr_om.expr, local_initial_state,
			local_final_state, recursive_path);
		nfa.add_epsilon_transition(local_initial_state, local_final_state);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		construct_nfa_proc(nfa, expr_p.expr, local_initial_state,
			local_final_state, recursive_path);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
		
		int new_state_i=nfa.add_state();
		int new_state_ii=nfa.add_state();
		nfa.add_epsilon_transition(local_initial_state, new_state_i);
		nfa.add_epsilon_transition(new_state_ii, local_final_state);
	#ifdef DEBUG_ITERATION
		cout << "iteration " << (expr_it.reflexive ? "*" : "+")
			<< ": " << local_initial_state << " -> "
			<< new_state_i << " -> " << new_state_ii << " -> "
			<< local_final_state << "\n";
	#endif
		construct_nfa_proc(nfa, expr_it.expr, new_state_i, new_state_ii, recursive_path);
		if(expr_it.reflexive)
			nfa.add_epsilon_transition(new_state_i, new_state_ii);
		nfa.add_epsilon_transition(new_state_ii, new_state_i);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCondition))
		assert(false); // it should have been expanded.
	else if(typeid(*expr)==typeid(NonterminalExpressionRange))
	{
		NonterminalExpressionRange &expr_r=*dynamic_cast<NonterminalExpressionRange *>(expr);
	#ifdef DEBUG_RANGE
		cout << "range (state " << local_initial_state << " -> "
			<< local_final_state << "): from " << expr_r.first
			<< " to " << expr_r.last << "\n";
	#endif
		for(int i=expr_r.first; i<=expr_r.last; i++)
			nfa.add_transition(local_initial_state, local_final_state, i);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionContains))
	{
		NonterminalExpressionContains &expr_cont=*dynamic_cast<NonterminalExpressionContains *>(expr);
		
		int new_state_i=nfa.add_state();
		int new_state_ii=nfa.add_state();
		int new_state_iii=nfa.add_state();
		int new_state_iv=nfa.add_state();
		
		nfa.add_epsilon_transition(local_initial_state, new_state_i);
		nfa.add_epsilon_transition(new_state_i, new_state_ii);
		nfa.add_epsilon_transition(new_state_iii, new_state_iv);
		nfa.add_epsilon_transition(new_state_iv, local_final_state);
		for(int j=0; j<nfa.number_of_symbols; j++)
		{
			nfa.add_transition(new_state_i, new_state_i, j);
			nfa.add_transition(new_state_iv, new_state_iv, j);
		}
		
	#ifdef DEBUG_CONTAINS
		cout << "contains: " << local_initial_state << " -> "
			<< new_state_i << " -> " << new_state_ii << " -> "
			<< new_state_iii << " -> " << new_state_iv << " -> "
			<< local_final_state << "\n";
	#endif
		construct_nfa_proc(nfa, expr_cont.expr, new_state_ii, new_state_iii, recursive_path);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
	{
	#ifdef DEBUG_EPSILON
		cout << "epsilon: " << local_initial_state << " -> "
			<< local_final_state << "\n";
	#endif
		nfa.add_epsilon_transition(local_initial_state, local_final_state);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionSharpSign))
	{
	#ifdef DEBUG_SHARP
		cout << "sharp: " << local_initial_state << " -> "
			<< local_final_state << "\n";
	#endif
		for(int i=0; i<=DolphinData::data.variables.alphabet_cardinality; i++)
			nfa.add_transition(local_initial_state, local_final_state, i);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		if(expr_s.expr->is_nts)
		{
			int nn=expr_s.expr->nn;
			assert(nn>=0);
			
			if(DolphinData::data.nonterminals[nn].expanded)
			{
				set<int> actual_symbols=set<int>(*(DolphinData::data.nonterminals[nn].expanded) & UnionOfIntervals<int>(0, DolphinData::data.variables.alphabet_cardinality-1));
				for(set<int>::const_iterator p=actual_symbols.begin(); p!=actual_symbols.end(); p++)
					nfa.add_transition(local_initial_state, local_final_state, *p);
			}
			else if(recursive_path.count(nn))
				nfa.add_epsilon_transition(local_initial_state, recursive_path[nn]);
			else
			{
				// if this usage is not recursive, then
				// i. if it starts a recursion, then mark it as
				//   recursive for the descendants.
				// ii. insert its whole body of rules.
				// iii. if it has been marked, then unmark it.
				
				bool here_starts_a_recursion=DolphinData::data.derivation_paths[nn][nn].v.size();
				int state_used_as_initial;
				
				if(here_starts_a_recursion)
				{
					// introduction of an ancillary state
					// helps to avoid a really bad error.
					state_used_as_initial=nfa.add_state();
					nfa.add_epsilon_transition(local_initial_state, state_used_as_initial);
					recursive_path[nn]=state_used_as_initial;
				}
				else
					state_used_as_initial=local_initial_state;
				
				NonterminalData &nonterminal=DolphinData::data.nonterminals[nn];
				assert(nonterminal.rules.size()==1);
				construct_nfa_proc(nfa, nonterminal.rules[0]->right,
					state_used_as_initial, local_final_state,
					recursive_path);
				
				if(here_starts_a_recursion)
				{
					assert(recursive_path.count(nn) && recursive_path[nn]==state_used_as_initial);
					recursive_path.erase(nn);
				}
			}
		}
		else
		{
		#ifdef DEBUG_TERMINAL_SYMBOL
			cout << "symbol: ";
		#endif
			vector<int> &v=expr_s.expr->s;
			int previous_state=local_initial_state;
			for(int i=0; i<v.size(); i++)
			{
				int next_state=(i<v.size()-1 ? nfa.add_state() : local_final_state);
			#ifdef DEBUG_TERMINAL_SYMBOL
				cout << "(from " << previous_state << " to " << next_state << " on " << v[i] << ")";
			#endif
				nfa.add_transition(previous_state, next_state, v[i]);
				
				if(DolphinData::data.variables.case_insensitive && isalpha(v[i]))
				{
					// the most straightforward method to
					// implement case insensitivity.
					
					if(v[i]!=tolower(v[i]))
						nfa.add_transition(previous_state, next_state, tolower(v[i]));
					if(v[i]!=toupper(v[i]))
						nfa.add_transition(previous_state, next_state, toupper(v[i]));
				}
				
				previous_state=next_state;
			}
		#ifdef DEBUG_TERMINAL_SYMBOL
			cout << "\n";
		#endif
		}
	}
	else
		assert(false);
}

bool construct_nfa()
{
	cout << "construct_nfa()\n";
	TimeKeeper tk("nfa");
	
	DolphinData::data.nfa=NFA(DolphinData::data.variables.alphabet_cardinality);
	
	NFA &nfa=DolphinData::data.nfa;
	
	int initial_state=nfa.add_state();
	assert(!initial_state);
	for(int i=0; i<DolphinData::data.recognized_expressions.size(); i++)
	{
		RecognizedExpressionData &re=DolphinData::data.recognized_expressions[i];
		if(re.is_special) continue;
	#ifdef PRINT_WHERE_EXPRESSIONS_ARE_RECOGNIZED
		cout << "Processing " << re.in_serialized_form << "\n";
	#endif
		
		map<int, int> empty_recursive_path;
		if(re.lookahead) re.intermediate_nfa_state=nfa.add_state();
		re.final_nfa_state=nfa.add_state();
		
		int state_after_expr=(re.lookahead ? re.intermediate_nfa_state :
			re.final_nfa_state);
		construct_nfa_proc(nfa, re.expr, initial_state,
			state_after_expr, empty_recursive_path);
		assert(empty_recursive_path.size()==0);
		
		if(re.lookahead)
		{
			int additional_state=nfa.add_state();
			nfa.add_epsilon_transition(re.intermediate_nfa_state, additional_state);
			construct_nfa_proc(nfa, re.lookahead,
				additional_state, re.final_nfa_state,
				empty_recursive_path);
			assert(empty_recursive_path.size()==0);
			
			if(re.lookahead_length==-1)
				nfa.mark_as_important(re.intermediate_nfa_state);
			
		//	if(re.lookahead_length==-1)
		//		cout << "********** lookahead **********\n";
		}
		
		nfa.mark_as_accepting(re.final_nfa_state);
		
	#ifdef PRINT_WHERE_EXPRESSIONS_ARE_RECOGNIZED
		if(re.lookahead)
			cout << "Intermediate state " << re.intermediate_nfa_state
				<< "; recognized ";
		else
			cout << "Recognized ";
		cout << "in state " << re.final_nfa_state << "\n";
	#endif
	}
 	
	cout << nfa.states.size() << " NFA states.\n";
	
	if(DolphinData::data.variables.dump_nfa_to_file>0)
	{
		ofstream log;
		string log_file_name=DolphinData::data.file_name+string(".nfa");
		log.open(log_file_name.c_str());
		log << "NFA.\n\n";
		if(DolphinData::data.variables.dump_nfa_to_file==1)
			; //	dfa.raw_print(log);
		else if(DolphinData::data.variables.dump_nfa_to_file==2)
			nfa.print(log);
		else
			assert(false);
	}
	
	return true;
}
