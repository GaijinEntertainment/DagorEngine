
#ifndef __WHALE__LR1_H

#define __WHALE__LR1_H

#include "lr.h"
#include <vector>
#include <set>
#include <map>
#include "sorted_vector.h"

struct LR1
{
	struct Item
	{
		int rule;		// rule number.
		int dot_position;	// dot position. pos=i means 1 2 ... i-1 . i ...
		sorted_vector<int> lookaheads;	// terminal numbers.
		
		Item(int supplied_rule, int supplied_dot_position, int supplied_lookahead)
		{
			rule=supplied_rule;
			dot_position=supplied_dot_position;
			lookaheads.insert(supplied_lookahead);
		}
		Item(int supplied_rule, int supplied_dot_position, const sorted_vector<int> &supplied_lookaheads)
		{
			rule=supplied_rule;
			dot_position=supplied_dot_position;
			lookaheads=supplied_lookaheads;
		}
	};
	struct SortedVectorOfItemsCompare
	{
		bool operator ()(const sorted_vector<Item> *s1, const sorted_vector<Item> *s2) const
		{
			return *s1<*s2;
		}
	};
	struct State
	{
		sorted_vector<Item> kernel_items;
		std::map<int, sorted_vector<int> > nonterminals_added_by_closure;
		
		State()
		{
		}
	};
	
	std::vector<State *> states;
	std::map<sorted_vector<Item> *, int, SortedVectorOfItemsCompare> search_engine;
	
	void build_automaton();
	void print(std::ostream &os);
	void print_statistics(std::ostream &os);
	
private:
	void closure(State &I);
	int transition(int from, SymbolNumber sn);
};

inline bool operator ==(const LR1::Item &item1, const LR1::Item &item2)
{
	return item1.rule==item2.rule &&
		item1.dot_position==item2.dot_position &&
		item1.lookaheads==item2.lookaheads;
}

inline bool operator <(const LR1::Item &item1, const LR1::Item &item2)
{
	if(item1.rule<item2.rule) return true;
	if(item1.rule>item2.rule) return false;
	if(item1.dot_position<item2.dot_position) return true;
	if(item1.dot_position>item2.dot_position) return false;
	if(item1.lookaheads<item2.lookaheads) return true;
	return false;
}

std::ostream &operator <<(std::ostream &os, const LR1::Item &item);

#endif
