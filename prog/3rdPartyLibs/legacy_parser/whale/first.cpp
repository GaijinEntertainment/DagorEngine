
#include "whale.h"
#include "first.h"
#include "utilities.h"
#include "nstl/stl_format.h"
#include <fstream>
#include <iterator>
using namespace std;

bool make_first()
{
//	cout << "make_first()\n";
	
	for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
		WhaleData::data.nonterminals[i].is_nullable=false;
	
	bool repeat_flag=true;
	while(repeat_flag)
	{
		repeat_flag=false;
		
		for(int i=0; i<WhaleData::data.rules.size(); i++)
		{	
			RuleData &rule=WhaleData::data.rules[i];
			NonterminalData &nonterminal=WhaleData::data.nonterminals[rule.nn];
			
			bool end_of_rule_not_reached=false;
			for(int j=0; j<rule.body.size(); j++)
			{
				if(rule.body[j].is_terminal())
				{
					int tn=rule.body[j].terminal_number();
					if(!nonterminal.first.count(tn))
					{
						nonterminal.first.insert(tn);
						repeat_flag=true;
					}
					end_of_rule_not_reached=true;
					break;
				}
				else
				{
					NonterminalData &that_nonterminal=WhaleData::data.nonterminals[rule.body[j].nonterminal_number()];
					for(set<int>::const_iterator p=that_nonterminal.first.begin(); p!=that_nonterminal.first.end(); p++)
						if(!nonterminal.first.count(*p))
						{
							nonterminal.first.insert(*p);
							repeat_flag=true;
						}
					if(!that_nonterminal.is_nullable)
					{
						end_of_rule_not_reached=true;
						break;
					}
				}
			}
			if(!end_of_rule_not_reached && !nonterminal.is_nullable)
			{
				nonterminal.is_nullable=true;
				repeat_flag=true;
			}
		}
	}
	
	bool flag=true;
	vector<const char *> bad_nonterminal_names;
	for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
		if(WhaleData::data.nonterminals[i].first.empty() && !WhaleData::data.nonterminals[i].is_nullable)
			bad_nonterminal_names.push_back(WhaleData::data.nonterminals[i].name);
	if(bad_nonterminal_names.size())
	{
		cout << "No terminal strings can be derived from nonterminal"
			<< (bad_nonterminal_names.size()==1 ? " " : "s ");
		print_with_and(cout, bad_nonterminal_names.begin(), bad_nonterminal_names.end());
		cout << ".\n";
		flag=false;
	}
	
	WhaleData::data.first_computed=true;
	
	make_first_nonterminals();
	make_follow();
	
	if(WhaleData::data.variables.dump_first_to_file)
	{
		string first_file_name=WhaleData::data.file_name+string(".first");
		ofstream first_file(first_file_name.c_str());
		print_first(first_file);
	}
	
	return flag;
}

// calculates first(s[0]...s[n-1]) and adds it to 'result'.
// returns true if string is nullable.
bool first(sorted_vector<int> &result, SymbolNumber *s, int n)
{
	assert(WhaleData::data.first_computed);
	for(int i=0; i<n; i++)
	{
		if(s[i].is_terminal())
		{
			int tn=s[i].terminal_number();
			result.insert(tn);
			return false;
		}
		else if(s[i].is_nonterminal())
		{
			NonterminalData &nonterminal=WhaleData::data.nonterminals[s[i].nonterminal_number()];
			for(set<int>::const_iterator p=nonterminal.first.begin(); p!=nonterminal.first.end(); p++)
				result.insert(*p);
			if(!nonterminal.is_nullable)
				return false;
		}
		else
			assert(false);
	}
	return true;
}

sorted_vector<int> first(SymbolNumber *s, int n)
{
	sorted_vector<int> result;
	first(result, s, n);
	return result;
}

void print_first(ostream &os)
{
	os << "\tfirst: N -> 2^(Sigma U {e})";
	if(!WhaleData::data.first_computed) { os << ": no WhaleData::data available.\n"; return; }
	if(WhaleData::data.first_nonterminals_computed)
		os << ";  first_nonterminals: N -> 2^N\n";
	else
		os << "\n";
	
	for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
	{
		NonterminalData &nonterminal=WhaleData::data.nonterminals[i];
		
		os << nonterminal.name << ": ";
		
		if(!nonterminal.is_nullable && !nonterminal.first.size())
			os << "empty set";
		else if(nonterminal.is_nullable && !nonterminal.first.size())
			os << "{e}";
		else
		{
			os << "{";
			if(nonterminal.is_nullable) os << "e, ";
			custom_print(os, nonterminal.first.begin(), nonterminal.first.end(), TerminalNamePrinter(), ", ", ", ");
			os << "}";
		}
		
		if(WhaleData::data.first_nonterminals_computed)
		{
			os << " / ";
			
			if(!nonterminal.first_nonterminals.size())
				os << "empty set";
			else
			{
				os << "{";
				custom_print(os, nonterminal.first_nonterminals.begin(), nonterminal.first_nonterminals.end(), NonterminalNamePrinter(), ", ", ", ");
				os << "}";
			}
		}
		
		os << "\n";
	}
}

void make_first_nonterminals()
{
	// for each nonterminal A determine a set of nonterminals N': B is in
	// N' if and only if (A ==>+ B alpha) for some alpha.
//	cout << "make_first_nonterminals()\n";
	
	// I. Computing all one-step derivations: (A ==> beta B alpha) for
	// some nullable string beta.
	for(int i=0; i<WhaleData::data.rules.size(); i++)
	{
		RuleData &rule=WhaleData::data.rules[i];
		NonterminalData &nonterminal=WhaleData::data.nonterminals[rule.nn];
		
		for(int j=0; j<rule.body.size(); j++)
			if(rule.body[j].is_terminal())
				break;
			else
			{
				assert(rule.body[j].is_nonterminal());
				int nn=rule.body[j].nonterminal_number();
				nonterminal.first_nonterminals.insert(nn);
				if(!WhaleData::data.nonterminals[nn].is_nullable)
					break;
			}
	}
	
	// II. Performing a transitive closure: if (A ==>+ beta_1 B alpha) and
	// (B ==>+ beta_2 C gamma) then (A ==>+ beta_3 C delta) where beta_i
	// are some nullable strings.
	bool repeat_flag=true;
	while(repeat_flag)
	{
		repeat_flag=false;
		for(int i=0; i<WhaleData::data.nonterminals.size(); i++)
		{
			NonterminalData &nonterminal=WhaleData::data.nonterminals[i];
			for(set<int>::const_iterator p=nonterminal.first_nonterminals.begin(); p!=nonterminal.first_nonterminals.end(); p++)
				if(*p!=i)
				{
					NonterminalData &nonterminal2=WhaleData::data.nonterminals[*p];
					for(set<int>::iterator p2=nonterminal2.first_nonterminals.begin(); p2!=nonterminal2.first_nonterminals.end(); p2++)
						if(!nonterminal.first_nonterminals.count(*p2))
						{
							nonterminal.first_nonterminals.insert(*p2);
							repeat_flag=true;
						}
				}
		}
	}
	
	WhaleData::data.first_nonterminals_computed=true;
}

template<class T> void union_assign(set<T> &dest, const sorted_vector<T> &src)
{
	set<int> tmp;
	set_union(dest.begin(), dest.end(), src.begin(), src.end(), inserter(tmp, tmp.begin()));
	dest.swap(tmp);
}

void make_follow()
{
	assert(WhaleData::data.first_computed);
	
	WhaleData::data.nonterminals[0].follow.insert(0); // follow(S')={$};
	
	for(int i=0; i<WhaleData::data.rules.size(); i++)
	{	
		RuleData &rule=WhaleData::data.rules[i];
		int l=rule.body.size();
		
		for(int j=0; j<l-1; j++)
			if(rule.body[j].is_nonterminal())
			{
				NonterminalData &nonterminal=WhaleData::data.nonterminals[rule.body[j].nonterminal_number()];
				sorted_vector<int> f=first(&rule.body[j+1], l-(j+1));
				union_assign(nonterminal.follow, f);
			}
	}
	
	bool repeat_flag=true;
	while(repeat_flag)
	{
		repeat_flag=false;
		
		for(int i=0; i<WhaleData::data.rules.size(); i++)
		{
			RuleData &rule=WhaleData::data.rules[i];
			int l=rule.body.size();
			NonterminalData &nonterminal1=WhaleData::data.nonterminals[rule.nn];
			
			for(int j=l-1; j>=0; j--)
			{
				if(rule.body[j].is_terminal())
					break;
				else if(rule.body[j].is_nonterminal())
				{
					NonterminalData &nonterminal2=WhaleData::data.nonterminals[rule.body[j].nonterminal_number()];
					
					if(union_assign(nonterminal2.follow, nonterminal1.follow))
						repeat_flag=true;
					
					if(!nonterminal2.is_nullable)
						break;
				}
				else
					assert(false);
			}
		}
	}
	
	WhaleData::data.follow_computed=true;
}
