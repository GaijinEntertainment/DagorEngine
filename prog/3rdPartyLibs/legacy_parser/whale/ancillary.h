
#ifndef __WHALE__ANCILLARY_H

#define __WHALE__ANCILLARY_H

#include "whale.h"
#include <string>

SymbolNumber make_body_symbol(Whale::NonterminalExpression *expr, int source_nn, int source_an, int local_body_number, int letter, Whale::Terminal *location);
int make_single_iterator_symbol(SymbolNumber body_sn, int source_nn, int source_an, int local_iterator_number, Whale::Terminal *location);
int make_pair_iterator_symbol(SymbolNumber body1_sn, SymbolNumber body2_sn, int source_nn, int source_an, int local_iterator_number, Whale::Terminal *location);
int make_invoker_or_creator_or_updater(int source_nn, int source_an, int local_number, int letter, NonterminalData::Category category, Whale::Terminal *location);
int make_body_or_iterator_proc(int source_nn, int source_an, const std::string &name, const std::string &long_name, const std::string &title, ClassHierarchy::Class *base_class, Whale::Terminal *location);
SymbolNumber if_there_is_exactly_one_symbol_inside_then_extract_it(Whale::NonterminalExpression *expr);

#endif
