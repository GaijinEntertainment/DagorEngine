
#include "dolphin.h"

#include <fstream>
#include "parser.h"
#include "process.h"
#include "expand.h"
#include "codegen.h"
#include "assert.h"
#include "tables.h"
#include "time.h"
#include "nfa.h"
#include "dfa.h"
#include "lookahead.h"
#include "alphabet.h"
#include "utilities.h"

using namespace std;
using namespace Whale;

DolphinData DolphinData::data;
TimeKeeper time_keeper("main");

int main(int argc, char *argv[])
{
	cout << SHORT_COPYRIGHT_NOTICE;
	if(argc!=2) { cout << COMMAND_LINE_SYNTAX; return 1; }
	
	ifstream is(argv[1]);
	if(!is) { cout << "Unable to access file '" << argv[1] << "'.\n"; return 1; }
	
	DolphinData::data.file_name=strip_dot(argv[1]);
	
	DolphinLexicalAnalyzer dolphin(is);
	WhaleParser whale(dolphin);
	NonterminalS *S;
	{
		TimeKeeper tk("parser");
		S=whale.parse();
	}
	if(S)
	{
		try
		{
			bool result=process_grammar(S);
			if(result)
			{
				if(!assign_values_to_variables_stage_one()) return 1;
				if(!expand_what_can_be_expanded()) return 1;
				if(!process_lookaheads()) return 1;
				if(!process_alphabet()) return 1;
				if(!construct_nfa()) return 1;
				if(!construct_dfa()) return 1;
				make_tables();
				if(!assign_values_to_variables_stage_two()) return 1;
				assert(DolphinData::data.variables.check_database_integrity());
				generate_code();
				
				return 0;
			}
		}
		catch(FailedAssertion)
		{
			cout << "\nPlease send a bug report to okhotin@aha.ru\n\n";
			return 255;
		}
		catch(exception &exc)
		{
			cout << "\nmain(): Caught an std::exception '" << exc.what() << "'\n"
				"Please send a bug report to okhotin@aha.ru\n\n";
			return 255;
		}
		catch(...)
		{
			cout << "\nmain(): caught an unknown exception.\n"
				"Please send a bug report to okhotin@aha.ru\n\n";
			return 255;
		}
	}
	return 1;
}

DolphinData::DolphinData() : derivation_paths(0, 0),
	final_transition_table(0, 0),
	nfa(0)
{
	return_keywords_used=false;
	skip_keywords_used=false;
}
