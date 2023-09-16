
#include <iostream>
#include "whale.h"
#include "process.h"
using namespace std;

//#define DEBUG_CLASS_REGISTRATION

ClassHierarchy classes;

ClassHierarchy::ClassHierarchy()
{
//	cout << "ClassHierarchy::ClassHierarchy(): here, sizeof(ClassHierarchy::Class)=" << sizeof(ClassHierarchy::Class) << "\n";
	total_classes=0;
	total_data_members=0;
	
	symbol=new Class("Symbol", NULL, NULL);
	symbol->is_abstract=true;
	
	terminal=new Class("Terminal", symbol, NULL);
	terminal->is_derivative_of_terminal=true; // let it be reflexive.
	terminal->is_abstract=true;
	
	nonterminal=new Class("Nonterminal", symbol, NULL);
	nonterminal->is_derivative_of_nonterminal=true;
	nonterminal->is_abstract=true;
	
	single_iterator=NULL;
	pair_iterator=NULL;
	wrapper=NULL;
	creator=NULL;
	
//	basic_creator=NULL;
//	creator=NULL;
}

ClassHierarchy::Class::Class(const string &supplied_name, Class *supplied_ancestor, Class *supplied_scope)
	: name(supplied_name), assigned_to(SymbolNumber::error())
{
	if(supplied_scope)
		assert(supplied_scope->find_class_in_scope(supplied_name.c_str())==-1);
	else
		assert(!classes.find(name.c_str()));
	
	classes.total_classes++;
	
	is_internal=true;
	is_a_built_in_template=false;
	ancestor=supplied_ancestor;
	declared_at=NULL;
	was_implicitly_declared=false;
	scope_we_are_in=supplied_scope;
	code=NULL;
	wraps=NULL;
	wrapped_by=NULL;
	
#ifdef DEBUG_CLASS_REGISTRATION
	cout << "Registering " << this->get_full_name();
	if(ancestor)
		cout << " : " << ancestor->get_full_name() << "\n";
	else
		cout << "\n";
#endif
	
	if(ancestor)
	{
		assert(ancestor->is_internal);
		
		ancestor->descendants.insert(this);
		is_derivative_of_terminal=ancestor->is_derivative_of_terminal;
		is_derivative_of_nonterminal=ancestor->is_derivative_of_nonterminal;
	}
	else
	{
		is_derivative_of_terminal=false;
		is_derivative_of_nonterminal=false;
	}
	
	// assuming there is a single ClassHierarchy object...
	if(!scope_we_are_in)
		classes.in_global_scope[const_cast<char *>(name.c_str())]=this;
	else
		scope_we_are_in->in_our_scope.push_back(this);
	
	assert(!(is_derivative_of_terminal && is_derivative_of_nonterminal));
}

ClassHierarchy::Class::Class(const string &supplied_name)
	: name(supplied_name), assigned_to(SymbolNumber::error())
{
	assert(!classes.find(name.c_str()));
	
	classes.total_classes++;
	
	is_internal=false;
	is_a_built_in_template=false;
	
#ifdef DEBUG_CLASS_REGISTRATION
	cout << "Registering " << name << " (external)\n";
#endif

#if 0
	cout << "External type statement at ";
	nonterminal.external_type_declaration->print_location(cout);
	cout << ": " << nonterminal.external_type << "\n";
#endif
	
	classes.in_global_scope[const_cast<char *>(name.c_str())]=this;
}

ClassHierarchy::DataMember::DataMember()
{
	type=NULL;
	belongs_to=NULL;
	number_within_class=-1;
	number_of_nested_iterations=-1;
	declared_at=NULL;
	internal_type_if_there_is_an_external_type=NULL;
	senior=NULL;
	better_not_to_use_it_for_ordinary_symbols=false;
	
	classes.total_data_members++;
}

void ClassHierarchy::Class::print_where_it_was_declared(ostream &os)
{
	if(name=="Symbol" || name=="Terminal" || name=="Nonterminal")
		os << "a built-in abstract class";
	else if(name=="TerminalEOF")
		os << "assigned to eof marker";
	else if(name=="TerminalError")
		os << "assigned to error marker";
	else if(is_a_built_in_template)
		os << "a built-in class template";
	else
	{
		if(was_implicitly_declared) os << "implicitly ";
		os << "declared ";
		if(declared_at)
		{
			os << "at ";
			declared_at->print_location(os);
		}
		else
			os << "somewhere";
		if(assigned_to)
		{
			os << " and assigned to ";
			os << WhaleData::data.full_symbol_name(assigned_to, false);
		}
	}
}

int ClassHierarchy::Class::find_data_member(const char *s)
{
	for(int i=0; i<data_members.size(); i++)
		if(!strcmp(data_members[i]->name.c_str(), s))
			return i;
	return -1;
}

pair<ClassHierarchy::Class *, int> ClassHierarchy::Class::find_data_member_in_ancestors(const char *s)
{
	for(Class *c=this->ancestor; c; c=c->ancestor)
	{
		int n=c->find_data_member(s);
		if(n!=-1) return make_pair(c, n);
	}
	return make_pair((Class *)NULL, -1);
}

pair<ClassHierarchy::Class *, int> ClassHierarchy::Class::find_data_member_in_descendants(const char *s)
{
	for(set<Class *>::iterator p=descendants.begin(); p!=descendants.end(); p++)
	{
		int n=(*p)->find_data_member(s);
		if(n!=-1) return make_pair(*p, n);
		
		pair<Class *, int> cn=(*p)->find_data_member_in_descendants(s);
		if(cn.first) return cn;
	}
	return make_pair((Class *)NULL, -1);
}

triad<ClassHierarchy::Class *, int, int> ClassHierarchy::Class::find_data_member_in_direct_relatives(const char *s)
{
	int n1=find_data_member(s);
	if(n1!=-1) return make_triad(this, n1, 0);
	
	pair<Class *, int> cn1=find_data_member_in_ancestors(s);
	if(cn1.first) return make_triad(cn1.first, cn1.second, -1);
	
	pair<Class *, int> cn2=find_data_member_in_descendants(s);
	if(cn2.first) return make_triad(cn2.first, cn2.second, 1);
	
	return make_triad((Class *)NULL, -1, 0);
}

int ClassHierarchy::Class::find_class_in_scope(const char *s)
{
	for(int i=0; i<in_our_scope.size(); i++)
		if(!strcmp(s, in_our_scope[i]->name.c_str()))
			return i;
	return -1;
}

string ClassHierarchy::Class::get_full_name()
{
	if(scope_we_are_in)
		return scope_we_are_in->get_full_name() + string("::") + name;
	else
		return name;
}

string ClassHierarchy::DataMember::get_full_name()
{
	if(belongs_to)
		return belongs_to->get_full_name() + string("::") + name;
	else
		return name;
}

void ClassHierarchy::DataMember::print_where_it_was_declared(ostream &os)
{
	if(was_implicitly_declared) os << "implicitly ";
	os << "declared ";
	if(declared_at)
	{
		os << "at ";
		declared_at->print_location(os);
	}
	else
		os << "elsewhere";
}

ClassHierarchy::Class *ClassHierarchy::common_ancestor(ClassHierarchy::Class *c1, ClassHierarchy::Class *c2)
{
	// returns NULL if no common type exists.
	
	if(c1->is_internal!=c2->is_internal)
		return NULL;
	else if(c1->is_internal && c2->is_internal)
	{
		std::set<Class *> ancestors_of_c1;
		for(Class *c=c1; c; c=c->ancestor)
			if(!c->is_a_built_in_template)
				ancestors_of_c1.insert(c);
		
		for(Class *c=c2; c; c=c->ancestor)
			if(ancestors_of_c1.count(c))
				return c;
		
		return NULL;
	}
	else
	{
		assert(!c1->is_internal && !c2->is_internal);
		
		if(c1==c2)
			return c1;
		else
			return NULL;
	}
}

void ClassHierarchy::put_data_member_to_class(ClassHierarchy::DataMember *data_member, ClassHierarchy::Class *target_class)
{
	data_member->belongs_to=target_class;
	data_member->number_within_class=target_class->data_members.size();
	target_class->data_members.push_back(data_member);
}

ClassHierarchy::Class *ClassHierarchy::make_wrapper_class(ClassHierarchy::Class *what_do_we_wrap)
{
	if(what_do_we_wrap->wrapped_by)
		return what_do_we_wrap->wrapped_by;
	if(!wrapper)
	{
		wrapper=new Class("Wrapper", nonterminal, NULL);
		wrapper->is_derivative_of_nonterminal=true;
		wrapper->is_abstract=true;
		wrapper->is_a_built_in_template=true;
	}
	
	string name=string("Wrapper<") + string(what_do_we_wrap->name) + string(">");
	
	Class *c=new Class(name, wrapper, NULL);
	what_do_we_wrap->wrapped_by=c;
	c->wraps=what_do_we_wrap;
	
	return c;
}
