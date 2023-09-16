
#ifndef __DOLPHIN__TABLES_H

#define __DOLPHIN__TABLES_H

#include <stdio.h>
#include <vector>
#include <set>

void make_tables();

class TableMaker
{
	std::vector<int> *table;
	
public:
	TableMaker(std::vector<int> &supplied_table)
	{
		table=&supplied_table;
	}
	// returns offset of string in global table
	template<class ForwardIterator> int add(ForwardIterator first, ForwardIterator last)
	{
		int offset=table->size();
		for(ForwardIterator p=first; p!=last; p++)
			table->push_back(*p);
		return offset;
	}
	int add(std::vector<int> &v) { return add(v.begin(), v.end()); }
	int add(std::set<int> &s) { return add(s.begin(), s.end()); }
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
