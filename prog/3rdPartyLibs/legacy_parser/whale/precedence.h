
#ifndef __WHALE__PRECEDENCE_H

#define __WHALE__PRECEDENCE_H

#include <iostream>
#include <vector>
#include "matrix.h"

bool process_precedence();
std::pair<bool, int> resolve_lr_conflict(int shift_terminal, int shift_state, const std::vector<int> &reduce_rules);

class PrecedenceDatabase
{
	int n;
	matrix<bool> equivalence_relation;
	matrix<bool> partial_order_relation; // should be antisymmetric
	
public:
	PrecedenceDatabase() : n(0), equivalence_relation(0, 0), partial_order_relation(0, 0)
	{
	}
	void initialize(int supplied_n);
	void closure();
	void print_tables(std::ostream &os);
	bool &access_equivalence_relation(int i, int j) { return equivalence_relation[i][j]; }
	bool &access_partial_order_relation(int i, int j) { return partial_order_relation[i][j]; }
	
	friend bool process_precedence();
};

#endif
