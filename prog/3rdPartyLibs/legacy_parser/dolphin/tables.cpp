
// Experimental table compression routines.

#include "dolphin.h"
#include "tables.h"
#include "stl.h"
#include <fstream>
using namespace std;

//#define DUMP_SIMILARITY_RELATION

struct DifferenceBetweenLines
{
	bool identical;
	int region_begin;
	int region_end;
};

void compress_transition_table();
void compress_tables_layer_1();
void compress_tables_layer_2();
DifferenceBetweenLines find_difference_between_layer1_lines(int line1, int line2);

void make_table_of_lookahead_states();

void make_tables()
{
	if(DolphinData::data.variables.compress_tables)
		compress_transition_table();
	if(DolphinData::data.variables.generate_arbitrary_lookahead_support)
		make_table_of_lookahead_states();
}

void compress_transition_table()
{
	int m=DolphinData::data.number_of_symbol_classes;
	
	cout << "Compressing transition table: ";
	
	if(DolphinData::data.variables.using_layer2)
	{
		cout << "layer 1";
		cout.flush();
	}
	
	compress_tables_layer_1();
	
	if(DolphinData::data.variables.using_layer2) cout << " (";
	cout << DolphinData::data.tables.layer1_to_state.size() << " lines";
	if(DolphinData::data.variables.using_layer2)
		cout << ")";
	else
		cout << ".\n";
        
        if(DolphinData::data.variables.using_layer2)
	{
		cout << ", layer 2";
		cout.flush();
		
		compress_tables_layer_2();
		
		cout << " (" << DolphinData::data.tables.layer2_to_layer1.size() << " lines).\n";
	}
	
	TableMaker tm(DolphinData::data.tables.compressed_table_of_lines);
	if(!DolphinData::data.variables.using_layer2)
		for(int i=0; i<DolphinData::data.tables.layer1_to_state.size(); i++)
		{
			int i_state=DolphinData::data.tables.layer1_to_state[i];
			std::vector<int> v;
			
			/* INCREMENT IS HERE! */
			for(int j=0; j<m; j++)
				v.push_back(DolphinData::data.final_transition_table[i_state][j]+1);
			
			int offset=tm.add(v);
			DolphinData::data.tables.line_to_offset_in_table_of_lines.push_back(offset);
		}
	else
		for(int i=0; i<DolphinData::data.tables.layer2_to_layer1.size(); i++)
		{
			int i_layer1=DolphinData::data.tables.layer2_to_layer1[i];
			int i_state=DolphinData::data.tables.layer1_to_state[i_layer1];
			std::vector<int> v;
			
			/* INCREMENT IS HERE! */
			for(int j=0; j<m; j++)
				v.push_back(DolphinData::data.final_transition_table[i_state][j]+1);
			
			int offset=tm.add(v);
			DolphinData::data.tables.line_to_offset_in_table_of_lines.push_back(offset);
		}
}

void compress_tables_layer_1()
{
	int n=DolphinData::data.dfa_partition.groups.size();
	int m=DolphinData::data.number_of_symbol_classes;
	
	// It could be done in n*log(n) time, but I'm too lazy to do that.
	
	DolphinData::data.tables.state_to_layer1=vector<int>(n, -2);
	
	for(int i=0; i<n; i++)
		if(DolphinData::data.tables.state_to_layer1[i]==-2)
		{
			int this_line_number=DolphinData::data.tables.layer1_to_state.size();
			DolphinData::data.tables.layer1_to_state.push_back(i);
			DolphinData::data.tables.state_to_layer1[i]=this_line_number;
			
			bool it_is_null=true;
			for(int j=0; j<m; j++)
				if(DolphinData::data.final_transition_table[i][j]!=-1)
				{
					it_is_null=false;
					break;
				}
			if(it_is_null)
				DolphinData::data.tables.null_line_number=i;
			
			for(int k=i+1; k<n; k++)
				if(DolphinData::data.tables.state_to_layer1[k]==-2)
				{
					bool all_equal=true;
					for(int j=0; j<m; j++)
						if(DolphinData::data.final_transition_table[i][j]!=DolphinData::data.final_transition_table[k][j])
						{
							all_equal=false;
							break;
						}
					if(all_equal)
						DolphinData::data.tables.state_to_layer1[k]=this_line_number;
				}
		}
}

void compress_tables_layer_2()
{
	int n=DolphinData::data.tables.layer1_to_state.size();
	
	/* similarity_relation[i] is a sorted vector of numbers of those    */
	/* lines that are similar to i-th line. Lines should be distinct.   */
	vector<vector<int> > similarity_relation(n, vector<int>());
	
	for(int i=0; i<n; i++)
		for(int k=0; k<n; k++)
		{
			if(i==k) continue;
			
			DifferenceBetweenLines difference=find_difference_between_layer1_lines(i, k);
			assert(!difference.identical);
			int width=difference.region_end-difference.region_begin;
			assert(width>0);
			
			if(width<=DolphinData::data.variables.table_compression_exception_width)
			{
				similarity_relation[i].push_back(k);
				continue;
			}
		}
	
#ifdef DUMP_SIMILARITY_RELATION
	ofstream a("similarity_relation");
	
	for(int i=0; i<n; i++)
	{
		a << i << ": " << similarity_relation[i] << "\n";
	}
	
	a << "\n";
#endif
	
	set<int> lines_left;
	for(int i=0; i<n; i++) // how to write it in C*n time?
	{
		lines_left.insert(i);
		DolphinData::data.tables.layer1_to_layer2.push_back(-1);
		DolphinData::data.tables.layer1_to_exception_location.push_back(-1);
		DolphinData::data.tables.layer1_to_exception_data.push_back(vector<int>());
	}
	
	while(lines_left.size())
	{
		// The line that has the greatest number of neighbours.
		int the_line=*lines_left.begin();
		for(set<int>::const_iterator p=lines_left.begin(); p!=lines_left.end(); p++)
			if(similarity_relation[*p].size()>similarity_relation[the_line].size())
				the_line=*p;
		
		int layer_2_line=DolphinData::data.tables.layer2_to_layer1.size();
		DolphinData::data.tables.layer2_to_layer1.push_back(the_line);
		
	#ifdef DUMP_SIMILARITY_RELATION
		a << "Processing line " << the_line << " (" << similarity_relation[the_line].size() << " neighbours).\n";
	#endif
		
		// Store information about "exceptions" - those fragments in
		// neighbouring lines that differ from the current line.
		
		for(int i=0; i<similarity_relation[the_line].size(); i++)
		{
			int current_line=similarity_relation[the_line][i];
			int cl_state=DolphinData::data.tables.layer1_to_state[current_line];
			
			DifferenceBetweenLines d=find_difference_between_layer1_lines(the_line, current_line);
			assert(!d.identical);
			assert(d.region_end-d.region_begin<=DolphinData::data.variables.table_compression_exception_width);
			
			DolphinData::data.tables.layer1_to_exception_location[current_line]=d.region_begin;
		//	DolphinData::data.tables.layer1_to_exception_data[current_line]=vector<int>(DolphinData::data.final_transition_table.at(cl_state, d.region_begin), DolphinData::data.final_transition_table.at(cl_state, d.region_end));
			
			// INCREMENT HERE. Ought to bring a uniform enumeration.
			for(int i=d.region_begin; i<d.region_end; i++)
				DolphinData::data.tables.layer1_to_exception_data[current_line].push_back(DolphinData::data.final_transition_table[cl_state][i] + 1);
		}
		
		// Now we shall
		// i. Remove this line together with its neighbours from the
		//	list of lines to process.
		// ii. Remove all references to all these lines from the vectors.
		
		set<int> lines_to_erase(similarity_relation[the_line].begin(), similarity_relation[the_line].end());
		lines_to_erase.insert(the_line);
		assert(lines_to_erase.size()==similarity_relation[the_line].size()+1);
		
		for(set<int>::const_iterator p=lines_to_erase.begin(); p!=lines_to_erase.end(); p++)
		{
			DolphinData::data.tables.layer1_to_layer2[*p]=layer_2_line;
			lines_left.erase(*p);
		}
		
		for(int i=0; i<n; i++)
		{
			vector<int> temp;
			set_difference(similarity_relation[i].begin(), similarity_relation[i].end(),
				lines_to_erase.begin(), lines_to_erase.end(), back_inserter(temp));
			similarity_relation[i]=temp;
		}
	}
}

DifferenceBetweenLines find_difference_between_layer1_lines(int line1, int line2)
{
	DifferenceBetweenLines difference;
	
	int m=DolphinData::data.number_of_symbol_classes;
	
	// Using the simpliest method: two vectors are similar if there exist
	// p1 and p2, such that
	//	p1 < p2,
	//	p2-p1 < some predefined d,
	//	v1 and v2 are identical everywhere except the region between
	//		p1 and p2.
	// It is possible to use any other relation of similarity instead. It
	// should not necessarily be reflexive.
	
	int state1=DolphinData::data.tables.layer1_to_state[line1];
	int state2=DolphinData::data.tables.layer1_to_state[line2];
	
	bool before_region=true;
	difference.region_begin=0;
	difference.region_end=m;
	for(int j=0; j<m; j++)
	{
		if(before_region &&
			DolphinData::data.final_transition_table[state1][j]==
			DolphinData::data.final_transition_table[state2][j])
		{
			difference.region_begin=j+1;
		}
		if(DolphinData::data.final_transition_table[state1][j]!=
			DolphinData::data.final_transition_table[state2][j])
		{
			before_region=false;
			difference.region_end=j+1;
		}
	}
	
//	cout << "[" << difference.region_begin << ", " << difference.region_end << "]\n";
	difference.identical=before_region;
	
	return difference;
}

#if 0
void make_action_to_recognized_expression_table()
{
	DolphinData::data.tables.action_to_recognized_expression.push_back(0); // I'm tired of this increment.
	for(int i=0; i<DolphinData::data.actions.size(); i++)
		DolphinData::data.tables.action_to_recognized_expression.push_back(DolphinData::data.actions[i].recognized_expression_number);
}
#endif

void make_table_of_lookahead_states()
{
	cout << "make_table_of_lookahead_states()\n";
	
	DolphinData::data.lookahead_states_in_final_dfa=vector<bool>(DolphinData::data.dfa_partition.groups.size(), false);
	
//	for(int i=1; i<=31; i++)
//		DolphinData::data.tables.compressed_table_of_lookahead_states.push_back(i*i);
	
	TableMaker tm(DolphinData::data.tables.compressed_table_of_lookahead_states);
	for(int i=0; i<DolphinData::data.recognized_expressions.size(); i++)
	{
		RecognizedExpressionData &re=DolphinData::data.recognized_expressions[i];
		if(re.lookahead_length!=-1)
		{
			DolphinData::data.tables.offset_in_table_of_lookahead_states.push_back(0);
			DolphinData::data.tables.number_of_lookahead_states.push_back(0);
			continue;
		}
		
		assert(re.intermediate_dfa_states.size());
		
		set<int> states;
		for(set<int>::iterator p=re.intermediate_dfa_states.begin(); p!=re.intermediate_dfa_states.end(); p++)
		{
			int this_state=DolphinData::data.dfa_partition.state_to_group[*p];
			states.insert(this_state+1); // damn this increment!
			DolphinData::data.lookahead_states_in_final_dfa[this_state]=true;
		}
		
		int offset=tm.add(states);
		
		DolphinData::data.tables.offset_in_table_of_lookahead_states.push_back(offset);
		DolphinData::data.tables.number_of_lookahead_states.push_back(states.size());
	}
	
	//	std::vector<int> offset_in_table_of_lookahead_states;
	//	std::vector<int> number_of_lookahead_states;
	//	std::vector<int> compressed_table_of_lookahead_states;
}
