
#ifndef __WHALE__TABLES_H

#define __WHALE__TABLES_H

#include <stdio.h>
#include <vector>
#include <set>
#include <algorithm>

class IndexEntry
{
private:
	IndexEntry() {}
	
public:
	bool it_is_offset;
	
	union
	{
		int offset;
		int value;
	};
	
	// *** Ought to REMOVE this operator! ***
//	operator int() { return offset; }
	
	static IndexEntry single_offset(int supplied_offset)
	{
		IndexEntry ie;
		ie.it_is_offset=true;
		ie.offset=supplied_offset;
		return ie;
	}
	static IndexEntry single_value(int supplied_value)
	{
		IndexEntry ie;
		ie.it_is_offset=false;
		ie.value=supplied_value;
		return ie;
	}
};

class TableMaker
{
	int mode;
	std::vector<int> &table;
	std::vector<IndexEntry> &table_of_indices;
	
	bool neutral_element_specified;
	
	struct Equal
	{
		int neutral_element;
		
		Equal(int neutral_element_supplied) : neutral_element(neutral_element_supplied)
		{
		}
		bool operator ()(int x1, int x2)
		{
			return x1==x2 || x1==neutral_element || x2==neutral_element;
		}
	};
	
	Equal equal;
	
public:
	TableMaker(std::vector<int> &supplied_table, std::vector<IndexEntry> &supplied_table_of_indices, int mode_supplied) :
		table(supplied_table), table_of_indices(supplied_table_of_indices), mode(mode_supplied), equal(0)
	{
		neutral_element_specified=false;
	}
	~TableMaker()
	{
		// in case the table is left empty
		if(table_of_indices.size()>0 && table.size()==0)
			table.push_back(0);
	}
	void set_neutral_element(int neutral_element)
	{
		neutral_element_specified=true;
		equal=Equal(neutral_element);
	}
	void add(const std::vector<int> &v);
	template<class ForwardIterator> void add(ForwardIterator first, ForwardIterator last)
	{
		std::vector<int> temporary;
		copy(first, last, std::back_inserter(temporary));
		add(temporary);
	}
	template<class T> void add(const std::vector<T> &v) { add(v.begin(), v.end()); }
	template<class T> void add(const std::set<int> &s) { add(s.begin(), s.end()); }
};

class ErrorMapMaker
{
	TableMaker table_maker;
	int neutral_element;
	
public:
	ErrorMapMaker(std::vector<int> &supplied_table, std::vector<IndexEntry> &supplied_table_of_indices, int supplied_neutral_element) :
		table_maker(supplied_table, supplied_table_of_indices, 2),
		neutral_element(supplied_neutral_element)
	{
	}
	void add(const std::vector<int> &v);
	template<class ForwardIterator> void add(ForwardIterator first, ForwardIterator last)
	{
		std::vector<int> temporary;
		copy(first, last, std::back_inserter(temporary));
		add(temporary);
	}
	template<class T> void add(const std::vector<T> &v) { add(v.begin(), v.end()); }
	template<class T> void add(const std::set<int> &s) { add(s.begin(), s.end()); }
};

inline const char *determine_optimal_format_field(int n)
{
	if(n<10)
		return "%1d";
	else if(n<100)
		return "%2d";
	else if(n<1000)
		return "%3d";
	else if(n<10000)
		return "%4d";
	else if(n<100000)
		return "%5d";
	else
		return "%d";
}

class TablePrinter
{
	FILE *a;
	const char *indent, *format;
	int numbers_in_line, pos;
	bool it_is_the_first_number;
	
public:
	TablePrinter(FILE *supplied_a, const char *supplied_indent, const char *supplied_format, int supplied_numbers_in_line)
	{
		a=supplied_a;
		indent=supplied_indent;
		format=supplied_format;
		numbers_in_line=supplied_numbers_in_line;
		pos=0;
		it_is_the_first_number=true;
	}
	void print(int x)
	{
		if(it_is_the_first_number)
		{
			fprintf(a, "%s", indent);
			it_is_the_first_number=false;
		}
		else if(pos%numbers_in_line)
			fprintf(a, ", ");
		else
			fprintf(a, ",\n%s", indent);

		fprintf(a, format, x);
		
		pos++;
	}
	void print(std::vector<int> &v)
	{
		for(int i=0; i<v.size(); i++)
			print(v[i]);
	}
};

#endif
