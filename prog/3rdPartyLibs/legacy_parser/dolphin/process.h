
#ifndef __DOLPHIN__PROCESS_H

#define __DOLPHIN__PROCESS_H

#include "dolphin.h"

bool process_grammar(Whale::NonterminalS *S);
void print_a_number_of_terminal_locations(std::ostream &os, std::vector<Whale::Terminal *> &a, const char *word);
void print_a_number_of_expressions(std::ostream &os, std::set<int> &expressions);
void print_a_number_of_start_conditions(std::ostream &os, std::set<int> &start_conditions);
bool decode_escape_sequences(const char *s, std::vector<int> &v);

#endif
