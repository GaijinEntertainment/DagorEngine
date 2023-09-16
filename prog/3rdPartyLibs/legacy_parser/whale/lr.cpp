
#include <fstream>
#include <stdio.h>
#include "whale.h"
#include "lr.h"
#include "precedence.h"
#include "utilities.h"
using namespace std;

#include "lr0.h"
#include "lalr1.h"
#ifndef _MSC_VER
#include <unistd.h>
#endif

void build_automaton()
{
	if (WhaleData::data.variables.method == Variables::LR1)
		WhaleData::data.lr1_automaton.build_automaton();
	else if (WhaleData::data.variables.method == Variables::SLR1)
		build_slr1_automaton();
	else if (WhaleData::data.variables.method == Variables::LALR1)
		build_lalr1_automaton();	
	else
		assert(false);
}

LRAction process_lr_conflict(int this_state, int shift_terminal, int shift_state, const std::vector<int> &reduce_rules)
{
	if(shift_terminal<0 || shift_state<0)
		shift_terminal=-1, shift_state=-1;
	LRConflictResolutionPrecedent lr_crp(shift_terminal, reduce_rules);
	set<LRConflictResolutionPrecedent>::iterator p=WhaleData::data.lr_conflict_resolution_precedents.find(lr_crp);
	int action_number;
	if(p!=WhaleData::data.lr_conflict_resolution_precedents.end())
	{
		p->states_where_this_conflict_is_present.insert(this_state);
		action_number=p->action_we_took;
	}
	else
	{
		pair<bool, int> result=resolve_lr_conflict(shift_terminal, shift_state, reduce_rules);
		
		lr_crp.action_we_took=result.second;
		lr_crp.are_we_sure=result.first;
		lr_crp.states_where_this_conflict_is_present.insert(this_state);
		
		WhaleData::data.lr_conflict_resolution_precedents.insert(lr_crp);
		action_number=result.second;
	}
	
	if(action_number==-1)
		return LRAction::shift(shift_state);
	else if(action_number>=0)
		return LRAction::reduce(reduce_rules[action_number]);
	else
	{
		assert(false);
		return LRAction::error();
	}
}

void print_conflicts()
{
	string log_file_name=WhaleData::data.file_name+string(".conflicts");
	
	if(WhaleData::data.lr_conflict_resolution_precedents.size()==0)
	{
		unlink(log_file_name.c_str());
		return;
	}
	
	bool keep_log=WhaleData::data.variables.dump_conflicts_to_file;
	ofstream log;
	if(keep_log)
	{
		log.open(log_file_name.c_str());
		log << "LR conflicts.\n\n";
	}
	
	for(int pass=1; pass<=2; pass++)
	{
		if(keep_log) log << (pass==1 ? "Unresolved conflicts:\n" : "Resolved conflicts:\n");
		
		for(set<LRConflictResolutionPrecedent>::const_iterator p=WhaleData::data.lr_conflict_resolution_precedents.begin(); p!=WhaleData::data.lr_conflict_resolution_precedents.end(); p++)
		{
			if((pass==1 && p->are_we_sure) || (pass==2 && !p->are_we_sure))
				continue;
			bool print_to_screen=!p->are_we_sure;
			
			if(print_to_screen) cout << "Conflict between ";
			if(keep_log) log << "Between ";
			
			bool flag=false;
			for(int i=(p->shift_symbol==-1 ? 1 : 0); i<=p->reduce_rules.size(); i++) // stay calm
			{
				if(flag)
				{
					const char *s=(i<p->reduce_rules.size() ? ", " : " and ");
					if(print_to_screen) cout << s;
					if(keep_log) log << s;
				}
				else
					flag=true;
				
				if(i==0)
				{
					const char *s=WhaleData::data.terminals[p->shift_symbol].name;
					if(print_to_screen) cout << "Shift " << s;
					if(keep_log) log << "Shift " << s;
				}
				else
				{
					RuleData &rule=WhaleData::data.rules[p->reduce_rules[i-1]];
					if(print_to_screen) cout << "Reduce " << rule.in_printable_form();
					if(keep_log) log << "Reduce " << rule.in_printable_form();
				}
			}
			
			if(print_to_screen)
			{
				if(p->states_where_this_conflict_is_present.size()==1)
					cout << " in state " << *p->states_where_this_conflict_is_present.begin();
				else
					cout << " in states " << p->states_where_this_conflict_is_present;
			}
			if(keep_log)
			{
				if(p->states_where_this_conflict_is_present.size()==1)
					log << " in state " << *p->states_where_this_conflict_is_present.begin();
				else
					log << " in states " << p->states_where_this_conflict_is_present;
			}
			
			if(p->are_we_sure)
			{
				if(print_to_screen) cout << ", resolved in favour of ";
				if(keep_log) log << ", resolved in favour of ";
				
				if(p->action_we_took==-1)
				{
					const char *s=WhaleData::data.terminals[p->shift_symbol].name;
					if(print_to_screen) cout << "Shift " << s;
					if(keep_log) log << "Shift " << s;
				}
				else
				{
					RuleData &rule=WhaleData::data.rules[p->reduce_rules[p->action_we_took]];
					if(print_to_screen) cout << "Reduce " << rule.in_printable_form();
					if(keep_log) log << "Reduce " << rule.in_printable_form();
				}
				
				if(print_to_screen) cout << ".\n";
				if(keep_log) log << ".\n";
			}
			else
			{
				if(print_to_screen) cout << ".\n";
				if(keep_log) log << ".\n";
			}
		}
	}
}
