/** 
	Numbered set data structure.

	Two most important properties are:
		-	each element in unique, duplicates are not inserted
		-	each item can be accessed by index in constant time

	It has little functionality, but is (probably) faster that 
	combination of (vector, hash_map) and surely outperforms 
	anything using map.

	@version 0.1

	@author Vladimir Prus <ghost@cs.msu.su>
	@file
*/

#ifndef __NUMBERED_SET__
#define __NUMBERED_SET__

#include <vector>
#include <unordered_map>
#include <unordered_set>

// path specified by Alexander Okhotin, 6 March 2001.
//#include "../hash/hash_set"

using namespace std;


/// The numbered set data structure
template<class Item>
class numbered_set
{
public:

	typedef typename vector<Item>::size_type size_type;	
	typedef typename vector<Item>::reference reference;
	typedef typename vector<Item>::const_reference const_reference;

	typedef typename vector<Item>::iterator iterator;
	typedef typename vector<Item>::const_iterator const_iterator;

	vector<Item> items;

	size_type size() { return items.size(); }
	reference operator[](size_type i)
		{ return items[i]; }
	const_reference operator[](size_type i) const
		{ return items[i]; }

	iterator begin() { return items.begin(); }
	iterator end()   { return items.end(); }
	const_iterator begin() const { return items.begin(); }
	const_iterator end()   const { return items.end(); }



	template<class Item2>
	struct Hasher {

		numbered_set<Item>& the_set;
		Hasher(numbered_set<Item>& the_set) : the_set(the_set) {}

		size_t operator()(int i) const {
			return hash<Item>()(the_set.the_item(i));
		}	
	};

	template<class CItem>
	struct Comparer {
		numbered_set<CItem>& the_set;
		Comparer(numbered_set<CItem>& the_set) : the_set(the_set) {}		

		bool operator()(int i, int j) const {
			return the_set.the_item(i) == the_set.the_item(j);
		}
	};


	int bizzare_bcc_bug_workaround;
	unordered_set<int, Hasher<Item>, Comparer<Item> > indices;


	// The core -- insertets item if it isn't present. returns index
	int index_of(const Item& item);

	void insert(const Item& item);

	const_reference the_item(int i) {
		if (i == -1) return *scratch;
		else		 return items[i];
	}
	

	// FIXME: do something about hash table size
	numbered_set()
		:	indices(0, Hasher<Item>(*this), Comparer<Item>(*this)) {}

protected:
	const Item* scratch;

};


template<class Item>
int numbered_set<Item>::index_of(const Item& item)
{
	
	scratch = &item;

	auto p = indices.find(-1);
	if (p != indices.end()) {
		return *p;
	}
	else {

		// FIXME : use equal_range - will increase performance
		items.push_back(item);
		indices.insert(items.size() - 1);
		return items.size() - 1;		
	}
}

template<class Item>
void numbered_set<Item>::insert(const Item& item)
{
	index_of(item);
}



#endif
