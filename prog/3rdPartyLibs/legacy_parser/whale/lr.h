
#ifndef __WHALE__LR_H

#define __WHALE__LR_H

#include <vector>
#include <set>

void build_automaton();

class LRAction
{
	// up to -1 - Shift to -n-1, 0 - Error, 1 - Accept,
	// 2 an up - Reduce using rule n-1.
	int n;
	
	LRAction(int supplied_n=0) { n=supplied_n; }
	
public:
	static LRAction error() { return LRAction(0); }
	static LRAction accept() { return LRAction(1); }
	static LRAction shift(int state) { return LRAction(-state-1); }
	static LRAction reduce(int rule) { return LRAction(rule+1); }
	static LRAction set_n(int supplied_n) { return LRAction(supplied_n); }
	
	bool is_error() const { return !n; }
	bool is_accept() const { return n==1; }
	bool is_shift() const { return n<0; }
	bool is_reduce() const { return n>=2; }
//	bool is_shift_or_reduce() const { return is_shift() || is_reduce(); }
	int shift_state() const { return -n-1; }
	int reduce_rule() const { return n-1; }
	int get_n() const { return n; }
};

LRAction process_lr_conflict(int this_state, int shift_terminal, int shift_state, const std::vector<int> &reduce_rules);
void print_conflicts();

inline bool operator ==(const LRAction lra_i, const LRAction lra_ii)
{
	return lra_i.get_n()==lra_ii.get_n();
}
inline bool operator <(const LRAction lra_i, const LRAction lra_ii)
{
	return lra_i.get_n()<lra_ii.get_n();
}

// Abstract LR automaton. All *LR*(*) construction algorithms shall use this
// structure to store their final results.
struct LRAutomaton
{
	struct State
	{
		std::vector<int> action_table; // LRActions stored as ints.
		std::vector<int> goto_table;
	};
	
	std::vector<State> states;
};

// note: it would be better to implement it using a single map of pairs to
// pairs, instead of set of these structures.
struct LRConflictResolutionPrecedent
{
	int shift_symbol;
	std::vector<int> reduce_rules;
	int action_we_took; // -1 if shift, 0+ if reduce.
	bool are_we_sure;
	
	mutable std::set<int> states_where_this_conflict_is_present;
	
	LRConflictResolutionPrecedent(int supplied_shift_symbol, const std::vector<int> &supplied_reduce_rules) :
		shift_symbol(supplied_shift_symbol), reduce_rules(supplied_reduce_rules),
		action_we_took(-2)
	{
	}
};

inline bool operator <(const LRConflictResolutionPrecedent &lr_crp_1, const LRConflictResolutionPrecedent &lr_crp_2)
{
	if(lr_crp_1.shift_symbol<lr_crp_2.shift_symbol) return true;
	if(lr_crp_1.shift_symbol>lr_crp_2.shift_symbol) return false;
	return lr_crp_1.reduce_rules<lr_crp_2.reduce_rules;
}

#endif
