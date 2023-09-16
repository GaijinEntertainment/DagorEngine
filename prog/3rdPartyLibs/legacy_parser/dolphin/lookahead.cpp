
#include "lookahead.h"
#include "dolphin.h"
using namespace std;
using namespace Whale;

bool process_lookaheads()
{
	bool flag=true;
	
	for(int i=0; i<DolphinData::data.recognized_expressions.size(); i++)
	{
		RecognizedExpressionData &re=DolphinData::data.recognized_expressions[i];
		
		if(re.lookahead)
		{
			re.lookahead_length=expression_length(re.lookahead);
			
			if(re.lookahead_length>=0)
			{
				DolphinData::data.variables.generate_fixed_length_lookahead_support=true;
				
				if(re.lookahead_length==0)
					cout << "Warning: zero-length lookahead will have no effect (somewhere - write me!).\n";
			}
			else if(re.lookahead_length==-1)
			{
				DolphinData::data.variables.generate_arbitrary_lookahead_support=true;
				
			//	cout << "Support for arbitrary-length lookahead is not yet available.\n";
			//	flag=false;
				
			//	vector<Terminal *> locations;
			//	for(int j=0; j<re.action_numbers.size(); j++)
			//		locations.push_back(DolphinData::data.actions[re.action_numbers[j]]);
			//	cout << "Lookahead expression at ";
			//	print_terminal_location(re.lookahead);
			}
			else
				assert(false);
		}
		
		if(re.lookahead_eof!=0)
			DolphinData::data.variables.generate_eof_lookahead_support=true;
	}
	
	DolphinData::data.variables.generate_table_of_actions|=
		DolphinData::data.variables.generate_fixed_length_lookahead_support |
		DolphinData::data.variables.generate_arbitrary_lookahead_support |
		DolphinData::data.variables.generate_eof_lookahead_support;
	
	return flag;
}

// if the function returns some nonnegative n, then L(expr) consists entirely
// of n character long strings.
// if the function returns -1, then either L(expr) contains strings of
// different length, or the function has just failed to notice the opposite.
int expression_length(NonterminalExpression *expr)
{
	if(expr->expanded)
		return 1;
	else if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
		int result1=expression_length(expr_d.expr1);
		int result2=expression_length(expr_d.expr2);
		if(result1==result2 && result1!=-1)
			return result1;
		else
			return -1;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
		int result1=expression_length(expr_c.expr1);
		int result2=expression_length(expr_c.expr2);
		if(result1==result2 && result1!=-1)
			return result1;
		else
			return -1;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
		int result1=expression_length(expr_cat.expr1);
		int result2=expression_length(expr_cat.expr2);
		if(result1!=-1 && result2!=-1)
			return result1+result2;
		else
			return -1;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
		return -1;
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
		int result=expression_length(expr_om.expr);
		if(!result)
			return 0;
		else
			return -1;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		return expression_length(expr_p.expr);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
		int result=expression_length(expr_it.expr);
		if(!result)
			return 0;
		else
			return -1;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCondition))
		return 1; // this is right even if the condition is always false.
	else if(typeid(*expr)==typeid(NonterminalExpressionRange))
		return 1;
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
		return 0;
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		
		if(expr_s.expr->is_nts)
		{
			int nn=expr_s.expr->nn;
			if(DolphinData::data.derivation_paths[nn][nn].v.size())
				return -1;
			else if(DolphinData::data.nonterminals[nn].expanded)
				return 1;
			else if(DolphinData::data.nonterminals[nn].expression_length_calculated)
				return DolphinData::data.nonterminals[nn].expression_length;
			else
			{
				assert(!DolphinData::data.derivation_paths[nn][nn].v.size() &&
					!DolphinData::data.nonterminals[nn].expression_length_calculated);
				
				DolphinData::data.nonterminals[nn].expression_length=expression_length(DolphinData::data.nonterminals[nn].rules[0]->right);
				assert(("change me!" && false));
				DolphinData::data.nonterminals[nn].expression_length_calculated=true;
				
				return DolphinData::data.nonterminals[nn].expression_length;
			}
		}
		else
			return expr_s.expr->s.size();
	}
	else
	{
		assert(false);
		return false; // to please the compiler
	}
}
