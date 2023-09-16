
#ifndef __WHALE__CLASSES_H

#define __WHALE__CLASSES_H

#include <string>
#include <set>
#include <map>
#include "whale.h"
#include "tuple.h"

class ClassHierarchy
{
public:
	class Class;
	class DataMember;
	
	Class *symbol, *terminal, *nonterminal;
	Class *single_iterator, *pair_iterator;
	Class *wrapper;
	Class *creator;
	
//	Class *basic_creator, *creator;
	
	std::map<const char *, Class *, NullTerminatedStringCompare> in_global_scope;
	int total_classes, total_data_members;
	
	ClassHierarchy();
	Class *find(const char *name)
	{
		std::map<const char *, Class *, NullTerminatedStringCompare>::iterator p=in_global_scope.find(name);
		if(p!=in_global_scope.end())
			return (*p).second;
		else
			return NULL;
	}
	Class *common_ancestor(Class *c1, Class *c2);
	void put_data_member_to_class(DataMember *data_member, Class *target_class);
	Class *make_wrapper_class(Class *what_do_we_wrap);
	
	class DataMember
	{
	public:
		std::string name;
		Class *type;	// can be either an internal or an external
				// type. Check type->is_internal to learn
				// the truth.
		
		Class *belongs_to;
		int number_within_class;
		int number_of_nested_iterations;
		
		bool was_implicitly_declared;
		Whale::Terminal *declared_at;	// !=NULL
		
		// if this data member has an external type (which is
		// equivalent to !type->is_internal), then this variable
		// points to an internal type that is theoretically capable of
		// storing this data member. Such type always exists, because
		// all internal types are derivatives of Symbol.
		Class *internal_type_if_there_is_an_external_type;
		
		// If this data member belongs to a class assigned to an
		// iteration body, then this variable points to a data member
		// in a class assigned to an outer entity. Data members
		// eventually flush their contents to their seniors.
		DataMember *senior;
		bool senior_rests_in_a_body;
		
		// currently used by Symbol *Symbol::up
		bool better_not_to_use_it_for_ordinary_symbols;
		
		DataMember();
		void print_where_it_was_declared(std::ostream &os);
		std::string get_full_name();
	};
	
	class Class
	{
	public:
		struct Compare
		{
			bool operator ()(const Class *m1, const Class *m2) const
			{
				return m1->name < m2->name;
			}
		};
		
		std::string name;
		bool is_internal;
		bool is_a_built_in_template;
		
		Class *ancestor;
		std::vector<Class *> template_parameters; // for derivatives of
			// Iterator, PairIterator and Creator.
			// _NOT_ for derivates of Wrapper!
		std::set<Class *> descendants;
		
		Class *wraps;
		Class *wrapped_by;
		
		Class *scope_we_are_in;
		std::vector<Class *> in_our_scope;
		
		std::vector<DataMember *> data_members;
		
		bool is_derivative_of_terminal;
		bool is_derivative_of_nonterminal;
		
		bool is_abstract; // has no symbol assigned to it.
		SymbolNumber assigned_to; // implies !is_abstract
		
		bool was_implicitly_declared;
		Whale::Terminal *declared_at; // NULL if it is a built-in class
		
		// For descendants of Nonterminal assigned to specific
		// nonterminals (assigned_to.is_nonterminal() is true).
		enum NonterminalClassCategory
		{
			UNKNOWN,
			SINGLE,		// The only class for the only
					// alternative of a nonterminal.
			WRAPPER,	// A wrapper for an object of an
					// external type. It is an internal
					// class assigned to an EXTERNAL
					// nonterminal.
			BASE,		// Class corresponding to a
					// nonterminal which has multiple
					// alternatives.
			ABSTRACT,	// Class derived from BASE class, which
					// is not assigned to a specific
					// alternative. It can be a base
					// class to other ABSTRACT classes and
					// to SPECIFIC classes
			SPECIFIC	// Class corresponding to one of
					// alternatives of a multiple-alternative
					// nonterminal. It must be a descendant
					// of BASE class corresponding to that
					// nonterminal.
		};
		NonterminalClassCategory nonterminal_class_category;
		int nonterminal_alternative_number; // for SPECIFIC only.
		
		char *code;
		
		Class(const std::string &supplied_name, Class *supplied_ancestor,
			Class *supplied_scope);
		Class(const std::string &supplied_name);
		void print_where_it_was_declared(std::ostream &os);
		int find_data_member(const char *s);
		std::pair<Class *, int> find_data_member_in_ancestors(const char *s);
		std::pair<Class *, int> find_data_member_in_descendants(const char *s);
		triad<Class *, int, int> find_data_member_in_direct_relatives(const char *s);
		int find_class_in_scope(const char *s);
		std::string get_full_name();
	};
};

extern ClassHierarchy classes;

#endif
