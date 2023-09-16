
#ifndef __DOLPHIN__UTILITIES_H

#define __DOLPHIN__UTILITIES_H

#include <string.h>
#include <string>
#include <functional>

class NullTerminatedStringCompare
{
public:
	bool operator ()(const char *s1, const char *s2) const
	{
		return strcmp(s1, s2)<0;
	}
};

std::string roman_itoa(int n, bool use_capital_letters);
std::string strip_dot(const std::string &s);

// style=1: "IDENTIFIER", style=2: "Identifier".
std::string convert_file_name_to_identifier(const std::string &s, int style);

const char *printable_increment(int x);

#endif
