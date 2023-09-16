
// In order to be able to work with huge alphabets, Dolphin uses internal
// representations of symbols instead of their real numerical values.
// Two such representations are used:
// i. UnionOfIntervals<int> is used to store sets of symbols. Individual
//	symbols and expanded (see expand.cpp) expressions are stored as sets
//	of this kind. For example, a declaration
//		A -> range(1000, 100000) & ~5000 | 1;
//	would result in nonterminal A being expanded and stored as
//		{ (1, 1), (1000, 4999), (5001, 10000) }.
// ii. Symbols are grouped into classes of equivalence and every such class
//	of equivalence is assigned its own number. These numbers are used
//	by NFA and DFA construction routines as "internal symbol numbers".
//	For instance, if this is the whole of the grammar
//		range(1, 1000) ==> { first(); };
//		range(300, 500) | range(900, 1100) ==> { second(); };
//	then we shall create three classes of equivalence:
//		{ (1, 299), (501, 899) },
//		{ (300, 500), (900, 1000) } and
//		{ (1001, 1100) }
//	and, if we choose to call them I, II and III, we get the following
//	equivalent grammar:
//		I | II ==> { first(); };
//		II | III ==> { second(); };
//
// gather_sets_of_symbols() assumes that the set of expanded nodes of all
// expressions has already been computed, and uses it to build the mentioned
// classes of equivalence.

#include "dolphin.h"
using namespace std;
using namespace Whale;

bool gather_sets_of_symbols();
void gather_sets_of_symbols_proc(NonterminalExpression *expr, vector<UnionOfIntervals<int> *> &sets_of_symbols);

bool process_alphabet()
{
	if(!gather_sets_of_symbols()) return false;
	
	return true;
}

bool gather_sets_of_symbols()
{
	return true; // not implemented yet.
	
#if 0
	// sets of symbols are in dynamic memory and are unlikely to be
	// moved or deleted. Thus we shall reference them by pointers.
	vector<UnionOfIntervals<int> *> sets_of_symbols;
//	map<UnionOfIntervals<int> *, int> sets_of_symbols_index;
	
	// assert(nonterminal.rules.size()==1);
	// construct_nfa_proc(nfa, nonterminal.rules[0]->right,
	for(int i=0; i<data.recognized_expressions.size(); i++)
	{
		RecognizedExpressionData &re=data.recognized_expressions[i];
		if(re.is_special) continue;
		
		gather_sets_of_symbols_proc(re.expr, sets_of_symbols);
		if(re.lookahead)
			gather_sets_of_symbols_proc(re.lookahead, sets_of_symbols);
	}
	cout << "Gathered " << sets_of_symbols.size() << " sets.\n";
	
	return true;
#endif
}

void gather_sets_of_symbols_proc(NonterminalExpression *expr, vector<UnionOfIntervals<int> *> &sets_of_symbols)
{
	if(expr->expanded)
	{
		sets_of_symbols.push_back(expr->expanded);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
		gather_sets_of_symbols_proc(expr_d.expr1, sets_of_symbols);
		gather_sets_of_symbols_proc(expr_d.expr2, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
		gather_sets_of_symbols_proc(expr_c.expr1, sets_of_symbols);
		gather_sets_of_symbols_proc(expr_c.expr2, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
		gather_sets_of_symbols_proc(expr_cat.expr1, sets_of_symbols);
		gather_sets_of_symbols_proc(expr_cat.expr2, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
	{
		NonterminalExpressionComplement &expr_com=*dynamic_cast<NonterminalExpressionComplement *>(expr);
		gather_sets_of_symbols_proc(expr_com.expr, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
		gather_sets_of_symbols_proc(expr_om.expr, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		gather_sets_of_symbols_proc(expr_p.expr, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
		gather_sets_of_symbols_proc(expr_it.expr, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCondition))
		assert(false);
	else if(typeid(*expr)==typeid(NonterminalExpressionRange))
		assert(false);
	else if(typeid(*expr)==typeid(NonterminalExpressionContains))
	{
		NonterminalExpressionContains &expr_cont=*dynamic_cast<NonterminalExpressionContains *>(expr);
		gather_sets_of_symbols_proc(expr_cont.expr, sets_of_symbols);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
	{
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionSharpSign))
		assert(false);
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
	}
	else
		assert(false);
}
