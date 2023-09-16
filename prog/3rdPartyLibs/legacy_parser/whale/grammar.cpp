
#include <fstream>
#include "whale.h"
#include "grammar.h"
//#include "nstl/stl.h"
#include "ancillary.h"
#include "utilities.h"
#include "process.h"
using namespace std;
using namespace Whale;

//#define DEBUG_RULE_NUMBERS
//#define DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE

template<class T> set<vector<T> > concatenate_sets_of_vectors(const set<vector<T> > &s1, const set<vector<T> > &s2);
template<class T> inline set<vector<T> > set_consisting_of_a_single_one_element_vector(const T &x);
template<class T> inline set<vector<T> > set_consisting_of_a_single_empty_vector();
set<vector<NonterminalExpression *> > make_rules_proc(NonterminalExpression *expr);
int find_creator_or_updater_in_rule(const RuleData &rule, int pos);
SymbolNumber make_creator_or_updater_or_use_existing_one(const set<RuleData> &requested_rules, RuleData &rule, int last1);
void build_create_and_update_operator_locations_and_source_rules_database();

bool make_rules()
{
	bool flag=true;
	
	set<NonterminalExpressionConnectUp *> bad_connect_up_operators;
	
	for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
	{
		WhaleData::data.nonterminals[i].first_rule=WhaleData::data.rules.size();
		NonterminalData::Category category=WhaleData::data.nonterminals[i].category;
		
		if(category==NonterminalData::NORMAL ||
			category==NonterminalData::EXTERNAL ||
			category==NonterminalData::BODY)
		{
			set<RuleData> requested_rules;
			
			for(int j=0; j<WhaleData::data.nonterminals[i].alternatives.size(); j++)
			{
				AlternativeData &alternative=WhaleData::data.nonterminals[i].alternatives[j];
				
				set<vector<NonterminalExpression *> > rule_bodies=make_rules_proc(alternative.expr);
				
				for(set<vector<NonterminalExpression *> >::iterator p=rule_bodies.begin(); p!=rule_bodies.end(); p++)
				{
					RuleData rule(i, j);
					
					rule.expr_body=*p;
					
					for(int k=rule.expr_body.size()-1; k>=0; k--)
					{
						NonterminalExpression *expr=(*p)[k];
						
						if(typeid(*expr)==typeid(NonterminalExpressionCode))
						{
							NonterminalExpressionCode *expr_code=dynamic_cast<NonterminalExpressionCode *>(expr);
							
							// to be activated upon reduce.
							if(rule.code_to_execute_upon_reduce==NULL)
							{
								rule.code_to_execute_upon_reduce=expr_code;
								rule.expr_body.pop_back();
							}
							else
								break;
						}
						else if(typeid(*expr)==typeid(NonterminalExpressionConnectUp))
						{
							NonterminalExpressionConnectUp *expr_up=dynamic_cast<NonterminalExpressionConnectUp *>(expr);
							
							alternative.connect_up=true;
						//	WhaleData::data.variables.connect_up_operators_are_used=true;
							rule.expr_body.pop_back();
						}
						else
							break;
					}
					
					for(int k=0; k<rule.expr_body.size(); k++)
					{
						NonterminalExpression *expr=rule.expr_body[k];
						
						if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
							rule.body.push_back(dynamic_cast<NonterminalExpressionSymbol *>(expr)->sn);
						else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
							rule.body.push_back(SymbolNumber::for_nonterminal(dynamic_cast<NonterminalExpressionIteration *>(expr)->iterator_nn));
						else if(typeid(*expr)==typeid(NonterminalExpressionIterationPair))
							rule.body.push_back(SymbolNumber::for_nonterminal(dynamic_cast<NonterminalExpressionIterationPair *>(expr)->iterator_nn));
						else if(typeid(*expr)==typeid(NonterminalExpressionCreate) ||
							typeid(*expr)==typeid(NonterminalExpressionUpdate))
						{
							rule.body.push_back(make_creator_or_updater_or_use_existing_one(requested_rules, rule, k));
						}
						else if(typeid(*expr)==typeid(NonterminalExpressionCode))
						{
							NonterminalExpressionCode *expr_code=dynamic_cast<NonterminalExpressionCode *>(expr);
							
							if(expr_code->invoker_nn==-1)
							{
								int local_invoker_number=++WhaleData::data.nonterminals[i].number_of_invokers;
								expr_code->invoker_nn=make_invoker_or_creator_or_updater(i, j, local_invoker_number, 0, NonterminalData::INVOKER, expr_code->code);
								WhaleData::data.nonterminals[expr_code->invoker_nn].code_invoker_declaration=expr_code;
							}
							
							rule.body.push_back(SymbolNumber::for_nonterminal(expr_code->invoker_nn));
						}
						else if(typeid(*expr)==typeid(NonterminalExpressionConnectUp))
						{
							NonterminalExpressionConnectUp *expr_up=dynamic_cast<NonterminalExpressionConnectUp *>(expr);
							
							// in this version, all
							// of them are considered bad.
							bad_connect_up_operators.insert(expr_up);
							rule.expr_body.erase(rule.expr_body.begin()+k, rule.expr_body.begin()+k+1);
							k--;
						}
						else if(typeid(*expr)==typeid(NonterminalExpressionPrecedence))
						{
							NonterminalExpressionPrecedence *expr_pr=dynamic_cast<NonterminalExpressionPrecedence *>(expr);
							rule.precedence_expressions.push_back(expr_pr);
							rule.expr_body.erase(rule.expr_body.begin()+k, rule.expr_body.begin()+k+1);
							k--;
						}
						else
							assert(false);
					}
					
					assert(rule.body.size()==rule.expr_body.size());
					requested_rules.insert(rule);
				}
			}
			
			// note: no autoincrement in this for statement.
			for(set<RuleData>::const_iterator p=requested_rules.begin(); p!=requested_rules.end(); )
			{
				// looking for rules with the same body, they
				// should reside in the close vicinity.
				set<RuleData>::const_iterator p2;
				int count=0;
				map<int, int> how_many_rules_from_alternative;
				for(p2=p; p2!=requested_rules.end(); p2++, count++)
				{
					if(p2->body!=p->body)
						break;
					how_many_rules_from_alternative[p2->an]++;
				}
				assert(count>0);
				
				if(count>1)
				{
					bool more_than_one_from_single_alternative=(count>how_many_rules_from_alternative.size());
					bool different_alternatives=(how_many_rules_from_alternative.size()>1);
					
					cout << "Rule " << p->in_printable_form()
						<< " can be generated in " << english1_itoa(count, false)
						<< " different ways";
					if(different_alternatives || WhaleData::data.nonterminals[i].multiple_alternatives_mode)
					{
						if(different_alternatives)
							cout << ": ";
						
						for(map<int, int>::const_iterator p4=how_many_rules_from_alternative.begin(); p4!=how_many_rules_from_alternative.end(); p4++)
						{
							if(p4!=how_many_rules_from_alternative.begin())
							{
								map<int, int>::const_iterator p5=p4;
								p5++;
								if(p5!=how_many_rules_from_alternative.end())
									cout << "; ";
								else
									cout << " and ";
							}
							
							if(different_alternatives && more_than_one_from_single_alternative)
								cout << "in " << english1_itoa(p4->second, false)
									<< " way" << (p4->second==1 ? "" : "s")
									<< " from the " << english2_itoa(p4->first+1, false)
									<< " alternative";
							else
								cout << "from the " << english2_itoa(p4->first+1, false) << " alternative";
						}
						
						cout << ".\n";
					}
					else
						cout << ".\n";
				}
				
				WhaleData::data.rules.push_back(*p);
				p=p2;
			}
		}
		else if(category==NonterminalData::START)
		{
			assert(!i);
			
			RuleData rule(i, 0);
			rule.body.push_back(SymbolNumber::for_nonterminal(WhaleData::data.start_nonterminal_number));
			WhaleData::data.rules.push_back(rule);
		}
		else if(category==NonterminalData::ITERATOR)
		{
			ClassHierarchy::Class *type=WhaleData::data.nonterminals[i].type;
			assert(type);
			
			if(type->template_parameters.size()==1)
			{
				RuleData rule1(i, 0);
				rule1.body.push_back(type->template_parameters[0]->assigned_to);
				WhaleData::data.rules.push_back(rule1);
				
				RuleData rule2(i, 0);
				rule2.body.push_back(SymbolNumber::for_nonterminal(i));
				rule2.body.push_back(type->template_parameters[0]->assigned_to);
				WhaleData::data.rules.push_back(rule2);
			}
			else if(type->template_parameters.size()==2)
			{
				RuleData rule1(i, 0);
				rule1.body.push_back(type->template_parameters[0]->assigned_to);
				WhaleData::data.rules.push_back(rule1);
				
				RuleData rule2(i, 0);
				rule2.body.push_back(SymbolNumber::for_nonterminal(i));
				rule2.body.push_back(type->template_parameters[1]->assigned_to);
				rule2.body.push_back(type->template_parameters[0]->assigned_to);
				WhaleData::data.rules.push_back(rule2);
			}
			else
				assert(false);
		}
		else if(category==NonterminalData::CREATOR || category==NonterminalData::UPDATER)
		{
			RuleData rule(i, 0);
			
			// rule.body=epsilon
			
			WhaleData::data.rules.push_back(rule);
		}
		else if(category==NonterminalData::INVOKER)
		{
			RuleData rule(i, 0);
			
			// rule.body=epsilon
		//	cout << WhaleData::data.nonterminals[i].name << "\n";
			assert(WhaleData::data.nonterminals[i].code_invoker_declaration);
			rule.code_to_execute_upon_reduce=WhaleData::data.nonterminals[i].code_invoker_declaration;
			
			WhaleData::data.rules.push_back(rule);
		}
		else
			assert(false);
		
		WhaleData::data.nonterminals[i].last_rule=WhaleData::data.rules.size();
	#ifdef DEBUG_RULE_NUMBERS
		cout << WhaleData::data.nonterminals[i].name << ": " << WhaleData::data.nonterminals[i].first_rule << " " << WhaleData::data.nonterminals[i].last_rule << "\n";
	#endif
		assert((WhaleData::data.nonterminals[i].last_rule>WhaleData::data.nonterminals[i].first_rule) ||
			(category==NonterminalData::NORMAL && WhaleData::data.nonterminals[i].alternatives.size()==0));
	}
	
	if(bad_connect_up_operators.size()>0)
	{
		vector<Terminal *> v;
		for(set<NonterminalExpressionConnectUp *>::const_iterator p=bad_connect_up_operators.begin(); p!=bad_connect_up_operators.end(); p++)
			v.push_back((*p)->kw);
		
		cout << "The current version of Whale doesn't support "
			<< "connect_up operators other than in the rightmost "
			<< "position in rule; thus, declaration"
			<< (bad_connect_up_operators.size()==1 ? " " : "s ");
		print_a_number_of_terminal_locations(cout, v, "at");
		cout << (bad_connect_up_operators.size()==1 ? " is " : " are ")
			<< "invalid.\n";
		flag=false;
	}
	
	cout << WhaleData::data.terminals.size() << " terminals, ";
	cout << WhaleData::data.nonterminals.size() << " nonterminals, ";
	cout << classes.total_classes << " classes, ";
	cout << classes.total_data_members << " WhaleData::data members";
	cout << ".\n";
	
	cout << WhaleData::data.rules.size() << " context-free rules created.\n";
	
	if(WhaleData::data.variables.dump_grammar_to_file)
	{
		string grammar_file_name=WhaleData::data.file_name+string(".grammar");
		ofstream grammar_file(grammar_file_name.c_str());
		print_grammar(grammar_file);
	}
	
	build_create_and_update_operator_locations_and_source_rules_database();
	
	return flag;
}

void print_grammar(ostream &os)
{
	os << "\tContext-free grammar:\n";
	for(int i=0; i<WhaleData::data.rules.size(); i++)
		os << WhaleData::data.rules[i].in_printable_form() << "\n";
}

template<class T> set<vector<T> > concatenate_sets_of_vectors(const set<vector<T> > &s1, const set<vector<T> > &s2)
{
	set<vector<T> > result;
	
	for(typename set<vector<T> >::const_iterator p1=s1.begin(); p1!=s1.end(); p1++)
		for(typename set<vector<T> >::const_iterator p2=s2.begin(); p2!=s2.end(); p2++)
		{
			vector<T> v(*p1);
			copy((*p2).begin(), (*p2).end(), back_inserter(v));
			result.insert(v);
		}
	
	return result;
}

template<class T> inline set<vector<T> > set_consisting_of_a_single_one_element_vector(const T &x)
{
	set<vector<T> > result;
	vector<T> v;
	v.push_back(x);
	result.insert(v);
	return result;
}

template<class T> inline set<vector<T> > set_consisting_of_a_single_empty_vector()
{
	set<vector<T> > result;
	result.insert(vector<T>());
	return result;
}

set<vector<NonterminalExpression *> > make_rules_proc(NonterminalExpression *expr)
{
	if(typeid(*expr)==typeid(NonterminalExpressionDisjunction))
	{
		NonterminalExpressionDisjunction &expr_d=*dynamic_cast<NonterminalExpressionDisjunction *>(expr);
		set<vector<NonterminalExpression *> > result=make_rules_proc(expr_d.expr1);
		union_assign(result, make_rules_proc(expr_d.expr2));
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConjunction))
	{
		NonterminalExpressionConjunction &expr_c=*dynamic_cast<NonterminalExpressionConjunction *>(expr);
		assert(false);
		return set<vector<NonterminalExpression *> >();
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConcatenation))
	{
		NonterminalExpressionConcatenation &expr_cat=*dynamic_cast<NonterminalExpressionConcatenation *>(expr);
		return concatenate_sets_of_vectors(make_rules_proc(expr_cat.expr1), make_rules_proc(expr_cat.expr2));
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionComplement))
	{
		NonterminalExpressionComplement &expr_com=*dynamic_cast<NonterminalExpressionComplement *>(expr);
		assert(false);
		return set<vector<NonterminalExpression *> >();
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionOmittable))
	{
		NonterminalExpressionOmittable &expr_om=*dynamic_cast<NonterminalExpressionOmittable *>(expr);
		set<vector<NonterminalExpression *> > result=make_rules_proc(expr_om.expr);
		result.insert(vector<NonterminalExpression *>()); // {epsilon}
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		return make_rules_proc(expr_p.expr);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIteration))
	{
		NonterminalExpressionIteration &expr_it=*dynamic_cast<NonterminalExpressionIteration *>(expr);
		set<vector<NonterminalExpression *> > result;
		
		vector<NonterminalExpression *> v;
		v.push_back(&expr_it);
		result.insert(v);
		
		if(expr_it.reflexive)
			result.insert(vector<NonterminalExpression *>()); // {epsilon}
		
		return result;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionIterationPair))
	{
		NonterminalExpressionIterationPair &expr_it2=*dynamic_cast<NonterminalExpressionIterationPair *>(expr);
		return set_consisting_of_a_single_one_element_vector<NonterminalExpression *>(&expr_it2);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionEpsilon))
	{
		return set_consisting_of_a_single_empty_vector<NonterminalExpression *>();
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		return set_consisting_of_a_single_one_element_vector<NonterminalExpression *>(&expr_s);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionMemberName))
	{
		NonterminalExpressionMemberName &expr_mn=*dynamic_cast<NonterminalExpressionMemberName *>(expr);
		return make_rules_proc(expr_mn.expr);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCreate))
	{
		NonterminalExpressionCreate &expr_create=*dynamic_cast<NonterminalExpressionCreate *>(expr);
		return set_consisting_of_a_single_one_element_vector<NonterminalExpression *>(&expr_create);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionUpdate))
	{
		NonterminalExpressionUpdate &expr_update=*dynamic_cast<NonterminalExpressionUpdate *>(expr);
		return set_consisting_of_a_single_one_element_vector<NonterminalExpression *>(&expr_update);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionConnectUp))
	{
		NonterminalExpressionConnectUp &expr_up=*dynamic_cast<NonterminalExpressionConnectUp *>(expr);
		return set_consisting_of_a_single_one_element_vector<NonterminalExpression *>(&expr_up);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionCode))
	{
		NonterminalExpressionCode &expr_code=*dynamic_cast<NonterminalExpressionCode *>(expr);
		return set_consisting_of_a_single_one_element_vector<NonterminalExpression *>(&expr_code);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionPrecedence))
	{
		NonterminalExpressionPrecedence &expr_pr=*dynamic_cast<NonterminalExpressionPrecedence *>(expr);
		return set_consisting_of_a_single_one_element_vector<NonterminalExpression *>(&expr_pr);
	}
	else
	{
		assert(false);
		return set<vector<NonterminalExpression *> >();
	}
}

int find_creator_or_updater_in_rule(const RuleData &rule, int pos)
{
	assert(rule.body.size()>pos);
	for(int i=pos; i>=0; i--)
		if(rule.body[i].is_nonterminal())
		{
			NonterminalData &nonterminal=WhaleData::data.nonterminals[rule.body[i].nonterminal_number()];
			if(nonterminal.category==NonterminalData::CREATOR || nonterminal.category==NonterminalData::UPDATER)
				return i;
		}
	return -1;
}

// 'requested_rules' is a set of all rules already made for this alternative of
// this nonterminal. 'rule' is the current rule; its body is incomplete. The
// symbol returned by this function shall be appended to the end or rule body
// by the caller.
// TO DO: if there are no symbols for this creator or updater to process, then
// remove it.
SymbolNumber make_creator_or_updater_or_use_existing_one(const set<RuleData> &requested_rules, RuleData &rule, int last1)
{
	NonterminalExpressionCreate *expr_create=dynamic_cast<NonterminalExpressionCreate *>(rule.expr_body[last1]);
	NonterminalExpressionUpdate *expr_update=dynamic_cast<NonterminalExpressionUpdate *>(rule.expr_body[last1]);
	assert((expr_create!=NULL) ^ (expr_update!=NULL));
	
#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
	cout << "make_creator_or_updater_or_use_existing_one(): " << rule.in_printable_form() << "\n";
#endif
	
	// Looking for an existing creator or updater that could be used here.
	
	int first1;
	if(expr_create)
		first1=0;
	else if(expr_update)
	{
		first1=find_creator_or_updater_in_rule(rule, last1-1);
		assert(first1!=-1);
	}
	else
		assert(false);
	
#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
	cout << "\tfirst1=" << first1 << ", last1=" << last1 << "\n";
#endif
	
	for(set<RuleData>::const_iterator p=requested_rules.begin(); p!=requested_rules.end(); p++)
	{
		if(rule.an!=p->an)
			continue;
		
		int last2=-1;
		for(int i=0; i<p->expr_body.size(); i++)
			if(p->expr_body[i]==rule.expr_body[last1])
			{
				last2=i;
				break;
			}
		if(last2==-1)
			continue;
		
	#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
		cout << "\tConsidering " << WhaleData::data.full_symbol_name(p->body[last2], false)
			<< " in rule " << p->in_printable_form() << "\n";
	#endif
		
		int first2;
		if(expr_create)
			first2=0;
		else if(expr_update)
		{
			first2=find_creator_or_updater_in_rule(*p, last2-1);
			assert(first2!=-1);
		}
		else
			assert(false);
	#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
		cout << "\t\tfirst2=" << first2 << ", last2=" << last2 << "\n";
	#endif
		
		if(last1-first1 != last2-first2)
		{
		#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
			cout << "\t\tLength mismatch.\n";
		#endif
			continue;
		}
		
		bool ok_flag=true;
		for(int i=last1-1, j=last2-1; i>=first1; i--, j--)
			if(rule.expr_body[i]!=p->expr_body[j])
			{
			#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
				cout << "\t\tMismatch in position (" << i << ", " << j << ")\n";
			#endif
				ok_flag=false;
				break;
			}
		
		if(ok_flag)
		{
			#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
				cout << "\t\tReusing " << WhaleData::data.full_symbol_name(p->body[last2], false) << "\n";
			#endif
			return p->body[last2];
		}
	}
	
	// Creating a new creator or updater.
	
	int symbol_nn;
	if(expr_create)
	{
		if(expr_create->creator_nn.size()==0)
			expr_create->local_creator_number=++WhaleData::data.nonterminals[rule.nn].number_of_creators;
		symbol_nn=make_invoker_or_creator_or_updater(rule.nn, rule.an, expr_create->local_creator_number, expr_create->creator_nn.size()+1, NonterminalData::CREATOR, expr_create->kw);
		expr_create->creator_nn.push_back(symbol_nn);
	}
	else if(expr_update)
	{
		if(expr_update->updater_nn.size()==0)
			expr_update->local_updater_number=++WhaleData::data.nonterminals[rule.nn].number_of_updaters;
		symbol_nn=make_invoker_or_creator_or_updater(rule.nn, rule.an, expr_update->local_updater_number, expr_update->updater_nn.size()+1, NonterminalData::UPDATER, expr_update->kw);
		expr_update->updater_nn.push_back(symbol_nn);
	}
	else
		assert(false);
	
#ifdef DEBUG_MAKE_CREATOR_OR_UPDATER_OR_USE_EXISTING_ONE
	cout << "\t\tCreating nonterminal " << WhaleData::data.nonterminals[symbol_nn].name << "\n";
#endif
	
	return SymbolNumber::for_nonterminal(symbol_nn);
}

bool operator <(const RuleData &r1, const RuleData &r2)
{
	// the order of comparison is really important.
	if(r1.nn<r2.nn) return true;
	if(r1.nn>r2.nn) return false;
	if(r1.body<r2.body) return true;
	if(r1.body>r2.body) return false;
	if(r1.an<r2.an) return true;
	if(r1.an>r2.an) return false;
	if(r1.expr_body<r2.expr_body) return true;
	if(r1.expr_body>r2.expr_body) return false;
	return r1.precedence_expressions<r2.precedence_expressions;
}

void build_create_and_update_operator_locations_and_source_rules_database()
{
//	cout << "build_creator_and_updater_source_rules_database()\n";
	for(int i=0; i<WhaleData::data.rules.size(); i++)
	{
		RuleData &rule=WhaleData::data.rules[i];
		
		for(int j=0; j<rule.body.size(); j++)
		{
			SymbolNumber sn=rule.body[j];
			
			if(sn.is_nonterminal())
			{
				NonterminalData::Category category=WhaleData::data.nonterminals[sn.nonterminal_number()].category;
				
				if(category==NonterminalData::CREATOR || category==NonterminalData::UPDATER)
				{
				//	cout << WhaleData::data.symbol_name(sn) << " in " << rule.in_printable_form() << "\n";
					rule.create_and_update_operator_locations.push_back(j);
				}
				if(category==NonterminalData::CREATOR)
				{
					int nn=sn.nonterminal_number();
					
					AlternativeData &alternative=WhaleData::data.access_alternative(rule.nn, rule.an);
					if(find(alternative.creator_nn.begin(), alternative.creator_nn.end(), nn)==alternative.creator_nn.end())
						alternative.creator_nn.push_back(nn);
					
					if(rule.creator_nn==-1)
						rule.creator_nn=nn;
					else if(rule.creator_nn!=nn)
					{
						cout << "\n\n\t\tUnexpected internal error:\n\n";
						cout << "Found " << WhaleData::data.nonterminals[nn].name
							<< " in rule " << rule.in_printable_form()
							<< ", " << WhaleData::data.nonterminals[rule.creator_nn].name
							<< " expected.\n";
						assert(false);
					}
				}
				if(WhaleData::data.nonterminals[sn.nonterminal_number()].is_ancillary)
				{
					WhaleData::data.nonterminals[sn.nonterminal_number()].rules_where_ancillary_symbol_is_used.push_back(i);
				}
			}
		}
	}
}
