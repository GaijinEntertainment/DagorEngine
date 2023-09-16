
// create ancillary symbols: iterators, iteration bodies, invokers.

#include "whale.h"
#include "ancillary.h"
#include "utilities.h"
#include "process.h"
#include "classes.h"
using namespace std;
using namespace Whale;

#define CREATE_BODIES_FOR_SINGLE_SYMBOLS

SymbolNumber make_body_symbol(NonterminalExpression *expr, int source_nn, int source_an, int local_body_number, int letter, Terminal *location)
{
#ifndef CREATE_BODIES_FOR_SINGLE_SYMBOLS
	SymbolNumber what_is_inside_expr=if_there_is_exactly_one_symbol_inside_then_extract_it(expr);
	if(what_is_inside_expr) return what_is_inside_expr;
#endif
	
	NonterminalData &host_nonterminal=WhaleData::data.nonterminals[source_nn];
	string name="Body"+roman_itoa(local_body_number, true);
	if(letter>0) name+=letter_itoa(letter, false);
	string long_name=string(host_nonterminal.name)+"-"+name;
	
	int body_nn=make_body_or_iterator_proc(source_nn, source_an, name, long_name, "body", classes.nonterminal, location);
	WhaleData::data.nonterminals[body_nn].category=NonterminalData::BODY;
	
	AlternativeData sole_alternative;
	sole_alternative.expr=expr;
	sole_alternative.declaration=NULL;
	sole_alternative.type=NULL;
	WhaleData::data.nonterminals[body_nn].alternatives.push_back(sole_alternative);
	
	return SymbolNumber::for_nonterminal(body_nn);
}

int make_single_iterator_symbol(SymbolNumber body_sn, int source_nn, int source_an, int local_iterator_number, Terminal *location)
{
	NonterminalData &host_nonterminal=WhaleData::data.nonterminals[source_nn];
	string name="Iterator"+roman_itoa(local_iterator_number, true);
	string long_name=string(host_nonterminal.name)+"-"+name;
	
	if(!classes.single_iterator)
	{
		classes.single_iterator=new ClassHierarchy::Class("Iterator", classes.nonterminal, NULL);
		classes.single_iterator->is_derivative_of_nonterminal=true;
		classes.single_iterator->is_abstract=true;
		classes.single_iterator->is_a_built_in_template=true;
	}
	
	int iterator_nn=make_body_or_iterator_proc(source_nn, source_an, name, long_name, "iterator", classes.single_iterator, location);
	NonterminalData &newly_created_iterator=WhaleData::data.nonterminals[iterator_nn];
	newly_created_iterator.category=NonterminalData::ITERATOR;
	newly_created_iterator.type->template_parameters.push_back(WhaleData::data.symbol_type(body_sn));
	
	return iterator_nn;
}

int make_pair_iterator_symbol(SymbolNumber body1_sn, SymbolNumber body2_sn, int source_nn, int source_an, int local_iterator_number, Terminal *location)
{
	NonterminalData &host_nonterminal=WhaleData::data.nonterminals[source_nn];
	string name="Iterator"+roman_itoa(local_iterator_number, true);
	string long_name=string(host_nonterminal.name)+"-"+name;
	
	if(!classes.pair_iterator)
	{
		classes.pair_iterator=new ClassHierarchy::Class("PairIterator", classes.nonterminal, NULL);
		classes.pair_iterator->is_derivative_of_nonterminal=true;
		classes.pair_iterator->is_abstract=true;
		classes.pair_iterator->is_a_built_in_template=true;
	}
	
	int iterator_nn=make_body_or_iterator_proc(source_nn, source_an, name, long_name, "iterator", classes.pair_iterator, location);
	NonterminalData &newly_created_iterator=WhaleData::data.nonterminals[iterator_nn];
	newly_created_iterator.category=NonterminalData::ITERATOR;
	newly_created_iterator.type->template_parameters.push_back(WhaleData::data.symbol_type(body1_sn));
	newly_created_iterator.type->template_parameters.push_back(WhaleData::data.symbol_type(body2_sn));
	
	return iterator_nn;
}

int make_invoker_or_creator_or_updater(int source_nn, int source_an, int local_number, int letter, NonterminalData::Category category, Terminal *location)
{
	NonterminalData &host_nonterminal=WhaleData::data.nonterminals[source_nn];
	
	string name, message;
	ClassHierarchy::Class *base_class;
	
	if(category==NonterminalData::INVOKER)
		name="Invoker", message="invoker", base_class=classes.nonterminal;
	else if(category==NonterminalData::CREATOR)
	{
		name="Creator", message="creator";
		
		if(WhaleData::data.variables.derive_creators_from_the_abstract_creator)
		{
		//	classes.basic_creator=new ClassHierarchy::Class("BasicCreator", classes.nonterminal, NULL);
		//	classes.basic_creator->is_derivative_of_nonterminal=true;
		//	classes.basic_creator->is_abstract=true;
		//	classes.creator=new ClassHierarchy::Class("Creator", classes.basic_creator, NULL);
			
			if(!classes.creator)
			{
				classes.creator=new ClassHierarchy::Class("Creator", classes.nonterminal, NULL);
				classes.creator->is_derivative_of_nonterminal=true;
				classes.creator->is_abstract=true;
				classes.creator->is_a_built_in_template=false;
			}
			
			base_class=classes.creator;
		}
		else
			base_class=classes.nonterminal;
	}
	else if(category==NonterminalData::UPDATER)
		name="Updater", message="updater", base_class=classes.nonterminal;
	else
		assert(false);
	
	name+=roman_itoa(local_number, true);
	if(letter>0)
		name+=letter_itoa(letter, false);
	string long_name=string(host_nonterminal.name)+"-"+name;
	
	int new_symbol_nn=make_body_or_iterator_proc(source_nn, source_an, name, long_name, message.c_str(), base_class, location);
	NonterminalData &newly_created_symbol=WhaleData::data.nonterminals[new_symbol_nn];
	newly_created_symbol.category=category;
	
#if 0
	if(category==NonterminalData::CREATOR)
		newly_created_symbol.type->template_parameters.push_back(WhaleData::data.symbol_type(SymbolNumber::for_nonterminal(1)));
#endif
	
	if(category==NonterminalData::CREATOR)
	{
		ClassHierarchy::DataMember *new_data_member=new ClassHierarchy::DataMember;
		
	//	cout << long_name << ": WhaleData::data member at " << new_data_member << "\n";
		
		new_data_member->name="n";
		new_data_member->was_implicitly_declared=true;
		new_data_member->declared_at=location;
		new_data_member->number_of_nested_iterations=0;
		new_data_member->type=WhaleData::data.get_class_assigned_to_alternative(source_nn, source_an);
	//	cout << new_data_member->type->name << "\n";
	//	cout << "(" << source_nn << ", " << source_an << ")\n";
		
		classes.put_data_member_to_class(new_data_member, newly_created_symbol.type);
		
	//	cout << long_name << ": in fact at " << newly_created_symbol.type->data_members[0] << "\n";
	//	cout << long_name << ": type at " << new_data_member->type << "\n";
	}
	
	return new_symbol_nn;
}

int make_body_or_iterator_proc(int source_nn, int source_an, const string &name, const string &long_name, const string &title, ClassHierarchy::Class *base_class, Terminal *location)
{
	NonterminalData &host_nonterminal=WhaleData::data.nonterminals[source_nn];
	if(base_class && host_nonterminal.type->find_class_in_scope(name.c_str())!=-1)
	{
		cout << "Internal error at ";
		location->print_location(cout);
		cout << ": class " << name << " already exists in scope ";
		cout << host_nonterminal.type->get_full_name() << ".\n";
		assert(false);
	}
	if(WhaleData::data.find_terminal(long_name.c_str())!=-1 || WhaleData::data.find_nonterminal(long_name.c_str())!=-1)
	{
		cout << "Unable to create " << title << " symbol at ";
		location->print_location(cout);
		cout << " because the name '" << long_name << "' is already in use.\n";
		throw QuietException();
	}
	
	int new_nn=WhaleData::data.nonterminals.size();
	
	ClassHierarchy::Class *new_type;
	assert(base_class);
	new_type=new ClassHierarchy::Class(name.c_str(), base_class, host_nonterminal.type);
	new_type->is_abstract=false;
	new_type->assigned_to=SymbolNumber::for_nonterminal(new_nn);
	new_type->nonterminal_class_category=ClassHierarchy::Class::SINGLE;
	
//	cout << "Making ancillary symbol: " << long_name << "\n";
	
	NonterminalData new_nonterminal;
	
	new_nonterminal.name=strdup(long_name.c_str());
	new_nonterminal.declaration=NULL;
	new_nonterminal.type=new_type;
	
	new_nonterminal.is_ancillary=true;
	new_nonterminal.multiple_alternatives_mode=false;
	
	new_nonterminal.master_nn=source_nn;
	new_nonterminal.master_an=source_an;
	
	WhaleData::data.nonterminals.push_back(new_nonterminal);
	return new_nn;
}

SymbolNumber if_there_is_exactly_one_symbol_inside_then_extract_it(NonterminalExpression *expr)
{
	if(typeid(*expr)==typeid(NonterminalExpressionSymbol))
	{
		NonterminalExpressionSymbol &expr_s=*dynamic_cast<NonterminalExpressionSymbol *>(expr);
		return expr_s.sn;
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionInParentheses))
	{
		NonterminalExpressionInParentheses &expr_p=*dynamic_cast<NonterminalExpressionInParentheses *>(expr);
		return if_there_is_exactly_one_symbol_inside_then_extract_it(expr_p.expr);
	}
	else if(typeid(*expr)==typeid(NonterminalExpressionMemberName))
	{
		NonterminalExpressionMemberName &expr_mn=*dynamic_cast<NonterminalExpressionMemberName *>(expr);
		return if_there_is_exactly_one_symbol_inside_then_extract_it(expr_mn.expr);
	}
	else
		return SymbolNumber::error();
}
