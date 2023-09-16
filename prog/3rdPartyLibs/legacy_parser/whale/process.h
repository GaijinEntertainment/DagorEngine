
#ifndef __WHALE__PROCESS_H

#define __WHALE__PROCESS_H

#include "whale.h"
#include <exception>

bool process_grammar();
//void print_terminal_location(std::ostream &os, Whale::Terminal *a);
void print_a_number_of_terminal_locations(std::ostream &os, std::vector<Whale::Terminal *> &a, const char *word);

struct ProcessExpressionProcData
{
	int nn, an;
	Whale::NonterminalMemberExpression *member_expression;
	int number_of_nested_iterations;
	int symbol_count;
	
	bool inside_anything_but_concatenation; // parentheses and symbol names are also allowed
	Whale::NonterminalExpressionCreate *make_operator;
	
	NonterminalData &nonterminal() { return WhaleData::data.nonterminals[nn]; }
	AlternativeData &alternative() { return nonterminal().alternatives[an]; }
	
	friend bool process_expressions();
	
private:
	ProcessExpressionProcData(int supplied_nn, int supplied_an)
	{
		nn=supplied_nn, an=supplied_an;
		member_expression=NULL;
		number_of_nested_iterations=0;
		symbol_count=0;
		inside_anything_but_concatenation=false;
		make_operator=NULL;
	};
};

struct ProcessExpressionProcIIData
{
	int nn, an;
	std::vector<SymbolNumber> bodies_in_our_way;
	
	// data_members_already_processed[(x, n)]==y means that data member
	// x has already been expanded to iteration body (nonterminal number n),
	// and that y is one of positions where this data member is defined.
	std::map<std::pair<ClassHierarchy::DataMember *, int>, Whale::NonterminalExpressionSymbol *> data_members_already_processed;
	
	Whale::NonterminalExpressionCreate *make_operator;
	std::map<int, std::vector<Whale::NonterminalExpressionSymbol *> > &nonterminals_that_cannot_be_connected_up;
	
	NonterminalData &nonterminal() { return WhaleData::data.nonterminals[nn]; }
	AlternativeData &alternative() { return nonterminal().alternatives[an]; }
	
	friend bool process_expressions();

private:
	ProcessExpressionProcIIData(int supplied_nn, int supplied_an,
		std::map<int, std::vector<Whale::NonterminalExpressionSymbol *> > &that_map)
		: nonterminals_that_cannot_be_connected_up(that_map)
	{
		nn=supplied_nn, an=supplied_an;
		make_operator=NULL;
	};
};

#endif
