
#include "whale.h"

#include <fstream>
#include "parser.h"
#include "process.h"
#include "assert.h"
#include "utilities.h"
#include "codegen.h"
#include "grammar.h"
#include "first.h"
#include "tables.h"
#include "precedence.h"

using namespace std;
using namespace Whale;

WhaleData WhaleData::data;
//TimeKeeper time_keeper("main");

int main(int argc, char *argv[])
{
	cout << SHORT_COPYRIGHT_NOTICE;
	
	if(argc!=2)
	{
		cout << "\n"
			"Whale is being developed by:\n"
			"\n"
			"Alexander Okhotin <okhotin@aha.ru> - project coordination; transformation of\n"
			"\tsource grammar to context-free; class system; code generation.\n"
			"Vladimir Prus <ghost@cs.msu.su> - construction of parsing automata.\n"
			"\n"
			"For more information, see http://www.aha.ru/~whale/whale/.\n";
		cout << "\n" << COMMAND_LINE_SYNTAX;
		return 1;
	}
	
	ifstream is(argv[1]);
	if(!is) { cout << "Unable to access file '" << argv[1] << "'.\n"; return 1; }
	
	WhaleData::data.file_name=strip_dot(argv[1]);
	
	DolphinLexicalAnalyzer dolphin(is);
	WhaleParser whale(dolphin);
	WhaleData::data.S=whale.parse();
	if(WhaleData::data.S)
	{
		try
		{
			bool result=process_grammar();
			if(result)
			{
				if(!assign_values_to_variables_stage_one()) return 1;
				if(!make_rules()) return 1;
				if(!process_precedence()) return 1;
				if(!make_first()) return 1;
				build_automaton();
				make_tables();
				if(!assign_values_to_variables_stage_two()) return 1;
				generate_code();
				
				return 0;
			}
		//	else
		//		cout << "main(): false received from below.\n";
		}
		catch(FailedAssertion)
		{
			cout << "\nPlease send a bug report to okhotin@aha.ru\n\n";
			return 255;
		}
		catch(QuietException)
		{
			return 1;
		}
		catch(exception &exc)
		{
			cout << "\nmain(): Caught an std::exception '" << exc.what() << "'\n"
				<< "Please send a bug report to okhotin@aha.ru\n\n";
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

WhaleData::WhaleData()
{
	start_nonterminal_number=1;
	first_computed=false;
	first_nonterminals_computed=false;
	follow_computed=false;
}

string RuleData::in_printable_form() const
{
	string s;
	
	s=WhaleData::data.nonterminals[nn].name;
	s+=" -> ";
	if(body.size())
	{
		for(int i=0; i<body.size(); i++)
		{
			if(i) s+=" ";
			s+=WhaleData::data.symbol_name(body[i]);
		}
	}
	else
		s+="e";
	return s;
}

void TerminalData::print_where_it_was_declared(ostream &os)
{
	if(declaration)
	{
		os << "declared at ";
		print_terminal_location(os, declaration);
	}
	else
		os << "a special built-in terminal symbol";
}

void NonterminalData::print_where_it_was_declared(ostream &os)
{
	if(declaration)
	{
		os << "declared at ";
		print_terminal_location(os, declaration);
	}
	else
		os << "a special built-in nonterminal symbol";
}

void PrecedenceSymbolData::print_where_it_was_declared(ostream &os)
{
	if(was_implicitly_declared) os << "implicitly ";
	os << "declared ";
	if(declared_at)
	{
		os << "at ";
		print_terminal_location(os, declared_at);
	}
	else
		os << "somewhere";
}
