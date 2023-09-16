
#include <stdio.h>
#include "dolphin.h"
using namespace std;
using namespace Whale;

//#define LISP_MODE

string serialize_expression(NonterminalExpression *expr);
string serialize_logical_expression(NonterminalExpressionC *expr);
string serialize_single_symbol(int t);
string serialize_string(vector<int> &v);

string serialize_expression(NonterminalExpression *expr)
{
	if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
	#ifndef LISP_MODE
		return serialize_expression(expr_d.expr1) + " | " + serialize_expression(expr_d.expr2);
	#else
		return string("(| ") + serialize_expression(expr_d.expr1) + " " + serialize_expression(expr_d.expr2) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
	#ifndef LISP_MODE
		return serialize_expression(expr_c.expr1) + " & " + serialize_expression(expr_c.expr2);
	#else
		return string("(& ") + serialize_expression(expr_c.expr1) + " " + serialize_expression(expr_c.expr2) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
	#ifndef LISP_MODE
		return serialize_expression(expr_cat.expr1) + " " + serialize_expression(expr_cat.expr2);
	#else
		return string("(. ") + serialize_expression(expr_cat.expr1) + " " + serialize_expression(expr_cat.expr2) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
	{
		NonterminalExpressionComplement &expr_com=*dynamic_cast<NonterminalExpressionComplement *>(expr);
	#ifndef LISP_MODE
		return "~" + serialize_expression(expr_com.expr);
	#else
		return string("(~ ") + serialize_expression(expr_com.expr) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
	#ifndef LISP_MODE
		return "[" + serialize_expression(expr_om.expr) + "]";
	#else
		return string("([] ") + serialize_expression(expr_om.expr) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		return "(" + serialize_expression(expr_p.expr) + ")";
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
	#ifndef LISP_MODE
		return serialize_expression(expr_it.expr) + expr_it.sign->text;
	#else
		return string(expr_it.reflexive ? "(* " : "(+ ") + serialize_expression(expr_it.expr) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCondition))
	{
		NonterminalExpressionCondition &expr_cond=*dynamic_cast<NonterminalExpressionCondition *>(expr);
	#ifndef LISP_MODE
		return string("condition(") + serialize_logical_expression(expr_cond.condition) + ")";
	#else
		return string("(condition ") + serialize_logical_expression(expr_cond.condition) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionRange))
	{
		NonterminalExpressionRange &expr_r=*dynamic_cast<NonterminalExpressionRange *>(expr);
		
	#ifndef LISP_MODE
		return "range(" + serialize_single_symbol(expr_r.first) +
			", " + serialize_single_symbol(expr_r.last) + ")";
	#else
		return string("(range ") + serialize_single_symbol(expr_r.first) +
			" " + serialize_single_symbol(expr_r.last) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionContains))
	{
		NonterminalExpressionContains &expr_cont=*dynamic_cast<NonterminalExpressionContains *>(expr);
	#ifndef LISP_MODE
		return "contains(" + serialize_expression(expr_cont.expr) + ")";
	#else
		return "(contains " + serialize_expression(expr_cont.expr) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
	{
		return "epsilon";
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionSharpSign))
	{
		return "#";
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		vector<int> &v=expr_s.expr->s;
		if(expr_s.expr->is_nts)
			return DolphinData::data.nonterminals[expr_s.expr->nn].name;
		else if(!v.size())
			return "\x22\x22";
		else if(v.size()==1)
			return serialize_single_symbol(v[0]);
		else
			return serialize_string(v);
	}
	else
	{
		assert(false);
		return ""; // to please the compiler
	}
}

string serialize_logical_expression(NonterminalExpressionC *expr)
{
//	return "chaos"; // Hail RSL!
	
	if(typeid(*expr)==typeid(NonterminalExpressionC_Disjunction))
	{
		NonterminalExpressionC_Disjunction &expr_d=*dynamic_cast<NonterminalExpressionC_Disjunction *>(expr);
	#ifndef LISP_MODE
		return serialize_logical_expression(expr_d.expr1) + " | " +
			serialize_logical_expression(expr_d.expr2);
	#else
		return "(| " + serialize_logical_expression(expr_d.expr1) +
			serialize_logical_expression(expr_d.expr2) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Conjunction))
	{
		NonterminalExpressionC_Conjunction &expr_c=*dynamic_cast<NonterminalExpressionC_Conjunction *>(expr);
	#ifndef LISP_MODE
		return serialize_logical_expression(expr_c.expr1) + " & " +
			serialize_logical_expression(expr_c.expr2);
	#else
		return "(& " + serialize_logical_expression(expr_c.expr1) +
			serialize_logical_expression(expr_c.expr2) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Complement))
	{
		NonterminalExpressionC_Complement &expr_com=*dynamic_cast<NonterminalExpressionC_Complement *>(expr);
	#ifndef LISP_MODE
		return "~" + serialize_logical_expression(expr_com.expr);
	#else
		return "(~ " + serialize_logical_expression(expr_com.expr) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_InParentheses))
	{
		NonterminalExpressionC_InParentheses &expr_p=*dynamic_cast<NonterminalExpressionC_InParentheses *>(expr);
		return "(" + serialize_logical_expression(expr_p.expr) + ")";
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Comparison))
	{
		NonterminalExpressionC_Comparison &expr_cmp=*dynamic_cast<NonterminalExpressionC_Comparison *>(expr);
	#ifndef LISP_MODE
		return string("c") + NonterminalExpressionC_Comparison::operation_to_s(expr_cmp.actual_operation) +
			serialize_single_symbol(expr_cmp.symbol);
	#else
		return string("(") + NonterminalExpressionC_Comparison::operation_to_s(expr_cmp.actual_operation) +
			" c " + serialize_single_symbol(expr_cmp.symbol) + ")";
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_In))
	{
		NonterminalExpressionC_In &expr_in=*dynamic_cast<NonterminalExpressionC_In *>(expr);
	#ifndef LISP_MODE
		return string("c in ") + string(DolphinData::data.nonterminals[expr_in.nn].name);
	#else
		return string("(in c ") + string(DolphinData::data.nonterminals[expr_in.nn].name) + string(")");
	#endif
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionC_Constant))
	{
		NonterminalExpressionC_Constant &expr_const=*dynamic_cast<NonterminalExpressionC_Constant *>(expr);
		return string(expr_const.value ? "true" : "false");
	}
	else
	{
		assert(false);
		return ""; // to please the compiler
	}
}

string serialize_single_symbol(int t)
{
	if(t>=0x20 && t<=0x7f && t!=39)
		return "'"+string(1, char(t))+"'";
	else if(t=='\n')
		return "'\\n'";
	else if(t=='\r')
		return "'\\r'";
	else if(t=='\t')
		return "'\\t'";
	else
	{
		char buf[20];
		sprintf(buf, "0x%x", t);
		return string(buf);
	}
}

string serialize_string(vector<int> &v)
{
	string s;
	for(int i=0; i<v.size(); i++)
	{
		unsigned int t=v[i];
		if(t>=0x20 && t<=0x7f && t!=34)
			s+=char(t);
		else if(t=='\n')
			s+="\\n";
		else if(t=='\r')
			s+="\\r";
		else if(t=='\t')
			s+="\\t";
		else if(t<256 && i<v.size()-1 && !isxdigit(v[i+1]))
		{
			char buf[10];
			sprintf(buf, "\\x%x", t);
			s+=string(buf);
		}
		else
		{
			char buf[20];
			sprintf(buf, (t<=0xffff ? "\\u%04x" : "\\U%08x"), t);
			s+=string(buf);
		}
	}
	return "\x22" + s + "\x22";
}
