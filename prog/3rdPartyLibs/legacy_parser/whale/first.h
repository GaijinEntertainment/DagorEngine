
#ifndef __WHALE__FIRST_H

#define __WHALE__FIRST_H

#include <iostream>
#include "sorted_vector.h"

class SymbolNumber;

bool make_first();
void print_first(std::ostream &os);
bool first(sorted_vector<int> &result, SymbolNumber *s, int n);
sorted_vector<int> first(SymbolNumber *s, int n);

void make_first_nonterminals();

void make_follow();

#endif
