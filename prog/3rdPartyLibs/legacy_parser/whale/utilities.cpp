
#include "assert.h"
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include "utilities.h"
using namespace std;

string roman_itoa(int n, bool use_capital_letters)
{
	string s;
	int x=(use_capital_letters ? 0 : 'a'-'A');
	if(n>10000) return use_capital_letters ? "Multae milia" : "multae milia";
	if(n==0) return use_capital_letters ? "Nihil" : "nihil";
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

// mode=1:  one, two, three
// mode=2:  first, second, third
string english_itoa(int n, int mode, bool use_capital_letters)
{
	if(n==0) return use_capital_letters ? (mode==1 ? "Zero" : "Zeroth") :
		(mode==1 ? "zero" : "zeroth");
	else if(n<0) return (use_capital_letters ? "Minus " : "minus") +
			english_itoa(-n, mode, false);
	else if(n/1000>=1000)
		return use_capital_letters ? (mode==1 ? "Many" : "Innumerable") :
			(mode==1 ? "many" : "innumerable");
	else if(n>=1000)
	{
		string s;
		s+=english_itoa(n/1000, 1, use_capital_letters);
		if(n%1000)
			s+=" thousand "+english_itoa(n%1000, mode, false);
		else if(mode==1)
			s+=" thousand";
		else
			s+=" thousandth";
		return s;
	}
	else if(n>=100)
	{
		string s;
		s+=english_itoa(n/100, 1, use_capital_letters);
		s+=" hundred";
		if(n%100)
			s+=" hundred "+english_itoa(n%100, mode, false);
		else if(mode==1)
			s+=" hundred";
		else
			s+=" hundredth";
		return s;
	}
	else if(n>=20)
	{
		if(mode==2 && n%10==0)
			switch(n/10)
			{
			case 2: return use_capital_letters ? "Twentieth" : "twentieth";
			case 3: return use_capital_letters ? "Thirtieth" : "thirtieth";
			case 4: return use_capital_letters ? "Fourtieth" : "fourtieth";
			case 5: return use_capital_letters ? "Fiftieth" : "fiftieth";
			case 6: return use_capital_letters ? "Sixtieth" : "sixtieth";
			case 7: return use_capital_letters ? "Seventieth" : "seventieth";
			case 8: return use_capital_letters ? "Eightieth" : "eightieth";
			case 9: return use_capital_letters ? "Ninetieth" : "ninetieth";
			default: assert(false); return "";
			}
		else
		{
			string s;
			switch(n/10)
			{
			case 2: s=(use_capital_letters ? "Twenty" : "twenty"); break;
			case 3: s=(use_capital_letters ? "Thirty" : "thirty"); break;
			case 4: s=(use_capital_letters ? "Fourty" : "fourty"); break;
			case 5: s=(use_capital_letters ? "Fifty" : "fifty"); break;
			case 6: s=(use_capital_letters ? "Sixty" : "sixty"); break;
			case 7: s=(use_capital_letters ? "Seventy" : "seventy"); break;
			case 8: s=(use_capital_letters ? "Eighty" : "eighty"); break;
			case 9: s=(use_capital_letters ? "Ninety" : "ninety"); break;
			default: assert(false);
			}
			if(n%10)
				s+=" "+english_itoa(n%10, mode, false);
			return s;
		}
	}
	else
		switch(n)
		{
		case 1: return use_capital_letters ?
			(mode==1 ? "One" : "First") :
			(mode==1 ? "one" : "first");
		case 2: return use_capital_letters ?
			(mode==1 ? "Two" : "Second") :
			(mode==1 ? "two" : "second");
		case 3: return use_capital_letters ?
			(mode==1 ? "Three" : "Third") :
			(mode==1 ? "three" : "third");
		case 4: return use_capital_letters ?
			(mode==1 ? "Four" : "Fourth") :
			(mode==1 ? "four" : "fourth");
		case 5: return use_capital_letters ?
			(mode==1 ? "Five" : "Fifth") :
			(mode==1 ? "five" : "fifth");
		case 6: return use_capital_letters ?
			(mode==1 ? "Six" : "Sixth") :
			(mode==1 ? "six" : "sixth");
		case 7: return use_capital_letters ?
			(mode==1 ? "Seven" : "Seventh") :
			(mode==1 ? "seven" : "seventh");
		case 8: return use_capital_letters ?
			(mode==1 ? "Eight" : "Eighth") :
			(mode==1 ? "eight" : "eighth");
		case 9: return use_capital_letters ?
			(mode==1 ? "Nine" : "Ninth") :
			(mode==1 ? "nine" : "ninth");
		case 10: return use_capital_letters ?
			(mode==1 ? "Ten" : "Tenth") :
			(mode==1 ? "ten" : "tenth");
		case 11: return use_capital_letters ?
			(mode==1 ? "Eleven" : "Eleventh") :
			(mode==1 ? "eleven" : "eleventh");
		case 12: return use_capital_letters ?
			(mode==1 ? "Twelve" : "Twelveth") :
			(mode==1 ? "twelve" : "twelveth");
		case 13: return use_capital_letters ?
			(mode==1 ? "Thirteen" : "Thirteenth") :
			(mode==1 ? "thirteen" : "thirteenth");
		case 14: return use_capital_letters ?
			(mode==1 ? "Fourteen" : "Fourteenth") :
			(mode==1 ? "fourteen" : "fourteenth");
		case 15: return use_capital_letters ?
			(mode==1 ? "Fifteen" : "Fifteenth") :
			(mode==1 ? "fifteen" : "fifteenth");
		case 16: return use_capital_letters ?
			(mode==1 ? "Sixteen" : "Sixteenth") :
			(mode==1 ? "sixteen" : "sixteenth");
		case 17: return use_capital_letters ?
			(mode==1 ? "Seventeen" : "Seventeenth") :
			(mode==1 ? "seventeen" : "seventeenth");
		case 18: return use_capital_letters ?
			(mode==1 ? "Eighteen" : "Eighteenth") :
			(mode==1 ? "eighteen" : "eighteenth");
		case 19: return use_capital_letters ?
			(mode==1 ? "Nineteen" : "Nineteenth") :
			(mode==1 ? "nineteen" : "nineteenth");
		default: assert(false);
			return "";
		}
}

string letter_itoa(int n, bool use_capital_letters)
{
	assert(n>0);
	string s;
	while(n)
	{
		n--;
		s+=char(n%26 + (use_capital_letters ? 'A' : 'a'));
		n=n/26;
	}
	reverse(s.begin(), s.end());
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
		snprintf(buf, 20, "%d", x);
	else if(x==0)
		snprintf(buf, 20, "");
	else if(x>0)
		snprintf(buf, 20, "+%u", x);
	else
		assert(false);
	
	return buf;
}

string prepare_for_printf(const string &s)
{
	string result;
	
	bool after_hex_sequence=false;
	for(string::const_iterator p=s.begin(); p!=s.end(); p++)
		if(*p=='"')
			result+="\\x22", after_hex_sequence=true;
		else if(*p=='\\')
			result+="\\\\", after_hex_sequence=false;
		else if(after_hex_sequence && isxdigit(*p))
			result+="\x22 \x22", result+=*p, after_hex_sequence=false;
		else if(*p>=' ')
			result+=*p, after_hex_sequence=false;
		else if(*p=='\n')
			result+="\\n", after_hex_sequence=false;
		else if(*p=='\r')
			result+="\\r", after_hex_sequence=false;
		else if(*p=='\t')
			result+="\\t", after_hex_sequence=false;
		else if(*p=='\b')
			result+="\\b", after_hex_sequence=false;
		else if(*p=='\f')
			result+="\\f", after_hex_sequence=false;
		else
			assert(false);
	
//	cout << "'" << s << "' ----> '" << result << "'\n";
	
	return result;
}

bool decode_escape_sequences(char *s, vector<int> &v)
{
	static const char *escape_sequences="n\nt\tv\vb\br\rf\fa\a\\\\??\'\'\"\"";
	
	int length=strlen(s);
	assert(length>=2 && (s[0]=='\x27' || s[0]=='\x22') && s[0]==s[length-1]);
	s[length-1]=0;
	
	for(int i=1; s[i]; i++)
		if(s[i]!='\\')
			v.push_back(s[i]);
		else
		{
			i++;
			if(!s[i])
				return false;
			
			bool flag=false;
			for(int j=0; escape_sequences[j]; j+=2)
				if(s[i]==escape_sequences[j])
				{
					v.push_back(escape_sequences[j+1]);
					flag=true;
					break;
				}
			if(!flag)
			{
				if(s[i]=='x' || s[i]=='u' || s[i]=='U')
				{
					i++;
					int j;
					for(j=0; isxdigit(s[i+j]); j++);
					if(s[i-1]=='x')
					{
						if(!j || j>8)
							return false;
					}
					else if(s[i-1]=='u')
					{
						if(j>=4)
							j=4;
						else
							return false;
					}
					else if(s[i-1]=='U')
					{
						if(j>=8)
							j=8;
						else
							return false;
					}
					else
						assert(false);
					
					char backup=s[i+j];
					s[i+j]=0;
					int sn;
					sscanf(s+i, "%x", &sn);
					v.push_back(sn);
					s[i+j]=backup;
					i+=j-1;
				}
				else if(is_octal_digit(s[i]))
				{
					i++;
					int j;
					for(j=0; is_octal_digit(s[i+j]); j++);
					if(!j || j>11 || (j==11 && s[i]>='3'))
						return false;
					
					int sn=0;
					int factor=1;
					for(int k=j+i-1; k>=i; k--)
					{
						sn+=(s[k]-'8')*factor;
						factor*=8;
					}
					v.push_back(sn);
					i+=j-1;
				}
				else
					return false;
			}
		}
	
	return true;
}
