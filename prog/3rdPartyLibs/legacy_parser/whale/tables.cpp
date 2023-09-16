
#include "whale.h"
#include "tables.h"
//#include "stl.h"
using namespace std;

//#define DEBUG

void TableMaker::add(const vector<int> &v)
{
//	cout << "TableMaker::add(): " << v << "\n";
	
	if(mode==1)
	{
		table_of_indices.push_back(IndexEntry::single_offset(table.size()));
		copy(v.begin(), v.end(), back_inserter(table));
	}
	else if(mode==2 && !neutral_element_specified)
	{
		vector<int>::const_iterator p=search(table.begin(), table.end(), v.begin(), v.end());
		if(p!=table.end())
			table_of_indices.push_back(IndexEntry::single_offset(p-table.begin()));
		else
		{
			table_of_indices.push_back(IndexEntry::single_offset(table.size()));
			copy(v.begin(), v.end(), back_inserter(table));
		}
	}
	else if((mode==2 || mode==3) && neutral_element_specified)
	{
	#ifdef DEBUG
		cout << "[here: ";
	#endif
		// looking for first and last non-neutral positions in vector:
		int first_pos=-1, last_pos=-1;
		for(int i=0; i<v.size(); i++)
			if(v[i]!=equal.neutral_element)
			{
				first_pos=i;
				break;
			}
		if(first_pos==-1)
		{
		#ifdef DEBUG
			cout << "empty]";
		#endif
			table_of_indices.push_back(IndexEntry::single_offset(0));
			return;
		}
		for(int i=v.size()-1; i>=0; i--)
			if(v[i]!=equal.neutral_element)
			{
				last_pos=i;
				break;
			}
	#ifdef DEBUG
		cout << first_pos << ", " << last_pos << "]";
	#endif
		if(mode>=3)
		{
			int value=v[first_pos];
			bool different_values_present=false;
			for(int i=first_pos+1; i<=last_pos; i++)
				if(v[i]!=equal.neutral_element && v[i]!=value)
				{
					different_values_present=true;
					break;
				}
			if(!different_values_present)
			{
				table_of_indices.push_back(IndexEntry::single_value(value));
				return;
			}
		}
		
		vector<int>::const_iterator p=search(table.begin(), table.end(), v.begin()+first_pos, v.begin()+last_pos+1);
		if(p!=table.end())
			table_of_indices.push_back(IndexEntry::single_offset(p-table.begin()-first_pos));
		else
		{
			table_of_indices.push_back(IndexEntry::single_offset(table.size()-first_pos));
			copy(v.begin()+first_pos, v.begin()+last_pos+1, back_inserter(table));
		}
		
	#if 0
		vector<int>::iterator p=search(table.begin(), table.end(), v.begin(), v.end(), equal);
		if(p!=table.end())
		{
			int pos=p-table.begin();
			for(vector<int>::const_iterator pv=v.begin(); pv!=v.end(); pv++, p++)
				if(*p==equal.neutral_element)
					*p=*pv;
			table_of_indices.push_back(pos);
		}
		else
		{
			table_of_indices.push_back(table.size());
			copy(v.begin(), v.end(), back_inserter(table));
		}
	#endif
	}
	else
		assert(false);
}

void ErrorMapMaker::add(const vector<int> &v)
{
	int n=WhaleData::data.variables.assumed_number_of_bits_in_int;
	
	int number_of_extra_elements;
	if(v.size()%n)
	{
		number_of_extra_elements=n-v.size()%n;
		for(int i=0; i<number_of_extra_elements; i++)
			const_cast<vector<int> &>(v).push_back(neutral_element);
	}
	else
		number_of_extra_elements=0;
	
	vector<int> error_map;
	int size_of_error_map=v.size()/n;
	for(int i=0; i<size_of_error_map; i++)
	{
		unsigned int x=0;
		for(int j=0; j<n; j++)
			if(v[i*n+j]!=neutral_element)
				x|=(1 << j);
		error_map.push_back(x);
	}
	table_maker.add(error_map);
	
	for(int i=0; i<number_of_extra_elements; i++)
		const_cast<vector<int> &>(v).pop_back();
}
