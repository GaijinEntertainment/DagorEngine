
#ifndef __DOLPHIN__CHARSET_H

#define __DOLPHIN__CHARSET_H

#define __DOLPHIN__CHARSET_H__SANITY_CHECKS

#ifdef __DOLPHIN__CHARSET_H__SANITY_CHECKS
	#include "assert.h"
#endif

#include <vector>
#include <map>

class CharacterSet
{
	std::vector<int> internal_symbol_to_external_symbol_table;
	std::vector<int> one_byte_external_symbol_to_internal_symbol_table;
	std::map<int, int> multibyte_external_symbol_to_internal_symbol_table;
	
	int unconditionally_create_new_symbol(int e)
	{
		int i=internal_symbol_to_external_symbol_table.size();
		internal_symbol_to_external_symbol_table.push_back(e);
		
		if(e>=0 && e<=255)
		{
		#ifdef __DOLPHIN__CHARSET_H__SANITY_CHECKS
			assert(one_byte_external_symbol_to_internal_symbol_table[e]==0);
		#endif
			one_byte_external_symbol_to_internal_symbol_table[e]=i;
		}
		else
		{
		#ifdef __DOLPHIN__CHARSET_H__SANITY_CHECKS
			assert(multibyte_external_symbol_to_internal_symbol_table.find(e)==multibyte_external_symbol_to_internal_symbol_table.end());
		#endif
			multibyte_external_symbol_to_internal_symbol_table[e]=i;
		}
		
		return i;
	}
	
public:
	CharacterSet()
	{
		// internal symbol 0 corresponds to all unknown external symbols.
		internal_symbol_to_external_symbol_table.push_back(-1);
		
		for(int i=0; i<256; i++)
			one_byte_external_symbol_to_internal_symbol_table.push_back(0);
	}
	int internal_symbol_to_external_symbol(int i)
	{
		return internal_symbol_to_external_symbol_table[i];
	}
	int external_symbol_to_internal_symbol(int e)
	{
		if(e>=0 && e<=255)
			return one_byte_external_symbol_to_internal_symbol_table[e];
		
		std::map<int, int>::iterator p=multibyte_external_symbol_to_internal_symbol_table.find(e);
		if(p!=multibyte_external_symbol_to_internal_symbol_table.end())
			return p->second;
		else
			return 0;
	}
	int new_symbol(int e)
	{
		int i=external_symbol_to_internal_symbol(e);
		
		if(i!=0)
			return i;
		else
			return unconditionally_create_new_symbol(i);
	}
	int get_number_of_internal_symbols()
	{
		return internal_symbol_to_external_symbol_table.size();
	}
};

#endif
