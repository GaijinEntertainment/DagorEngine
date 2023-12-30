
#include "assert.h"
#include <iostream>
#include "utilities.h"
using namespace std;

string roman_itoa(int n, bool use_capital_letters)
{
	string s;
	int x=(use_capital_letters ? 0 : 'a'-'A');
	if(n>10000) { s+=(use_capital_letters ? "Multae milia" : "multae milia"); return s; }
	if(n==0) { s=(use_capital_letters ? "Nihil" : "nihil"); return s; }
	if(n<0) { s=(use_capital_letters ? "Minus " : "minus "); n=-n; }
	while(n>=1000) s+=char('M'+x), n-=1000;
	if(n>=900) s+=(use_capital_letters ? "CM" : "cm"), n-=900;
	if(n>=500) s+=char('D'+x), n-=500;
	while(n>=100) s+=char('C'+x), n-=100;
	if(n>=90) s+=(use_capital_letters ? "XC" : "xc"), n-=90;
	if(n>=50) s+=char('L'+x), n-=50;
	if(n>=40) s+=char('X'+x), s+=char('L'+x), n-=40;
	while(n>=10) s+=char('X'+x), n-=10;
	switch(n)
	{
	case 0: break;
	case 1: s+=(use_capital_letters ? "I" : "i"); break;
	case 2: s+=(use_capital_letters ? "II" : "ii"); break;
	case 3: s+=(use_capital_letters ? "III" : "iii"); break;
	case 4: s+=(use_capital_letters ? "IV" : "iv"); break;
	case 5: s+=(use_capital_letters ? "V" : "v"); break;
	case 6: s+=(use_capital_letters ? "VI" : "vi"); break;
	case 7: s+=(use_capital_letters ? "VII" : "vii"); break;
	case 8: s+=(use_capital_letters ? "VIII" : "viii"); break;
	case 9: s+=(use_capital_letters ? "IX" : "ix"); break;
	default: assert(false);
	}
	return s;
}

string strip_dot(const string &s)
{
	for(int i=s.size()-1; i>=0; i--)
		if(s[i]=='.')
			return string(s.begin(), s.begin()+i);
		else if(s[i]=='/' || s[i]=='\\')
			break;
	return s;
}

// style=1: "IDENTIFIER", style=2: "Identifier".
string convert_file_name_to_identifier(const string &s, int style)
{
	int first=0;
	for(int i=s.size()-1; i>=0; i--)
		if(s[i]=='/' || s[i]=='\\')
		{
			first=i;
			break;
		}
	
	string result;
	for(int i=first; i<s.size(); i++)
		if(isalpha(s[i]))
		{
			if(style==1 || (style==2 && i==first))
				result+=char(toupper(s[i]));
			else
				result+=char(tolower(s[i]));
		}
		else if(isdigit(s[i]))
			result+=s[i];
		else
			result+='_';
	
	return result;
}

const char *printable_increment(int x)
{
	static char buf[20];
	
	if(x<0)
		sprintf(buf, "%d", x);
	else if(x==0)
		strcpy(buf, "");
	else if(x>0)
		sprintf(buf, "+%u", x);
	else
		assert(false);
	
	return buf;
}
