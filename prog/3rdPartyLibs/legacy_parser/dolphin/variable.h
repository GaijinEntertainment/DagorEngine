
#ifndef __DOLPHIN__VARIABLE_H

#define __DOLPHIN__VARIABLE_H

#include <map>
#include <stdexcept>
#include <string>
#include "utilities.h"

enum VariableProperties
{
	DOES_NOT_EXIST=0,
	
	TRUE_AND_FALSE_ALLOWED=1,
	ID_ALLOWED=2,
	NUMBER_ALLOWED=4,
	STRING_ALLOWED=8,
	CODE_ALLOWED=16,
	MULTIPLE_VALUES_ALLOWED=32,
	PARAMETER_ALLOWED=64,
	PARAMETER_REQUIRED=128,
	MULTIPLE_PARAMETERS_ALLOWED=256,
	MULTIPLE_ASSIGNMENTS_ALLOWED=512,
	SINGLE_ASSIGNMENT_FOR_SINGLE_PARAMETER=1024,
	SINGLE_ASSIGNMENT_FOR_SINGLE_PARAMETER_LIST=2048,
	PROCESS_ESCAPE_SEQUENCES_IN_STRING=4096,
	
	ID_OR_STRING_ALLOWED=2+8,
	STRING_OR_CODE_ALLOWED=8+16+4096
};

class variable_base
{
protected:
	const char *name;
	VariableProperties properties;
	bool has_value_assigned_to_it;
	
public:
	variable_base(std::map<const char *, variable_base *, NullTerminatedStringCompare> &database,
		const char *name_supplied,
		VariableProperties properties_supplied)
	{
		name=name_supplied;
		properties=properties_supplied;
		has_value_assigned_to_it=false;
		database[name]=this;
	}
	
	friend struct Variables;
};

template<class T> class variable : public variable_base
{
//	vector<AssignmentData> assignments;
	T value;
	
private:
	std::string make_exception() const
	{
		return std::string("Attempt to read an uninitialized variable ") +
			typeid(T).name() + std::string(" ") +
			std::string(name) + std::string(".");
	}
	
public:
	variable(std::map<const char *, variable_base *, NullTerminatedStringCompare> &database, const char *name, VariableProperties properties)
		: variable_base(database, name, properties)
	{
	}
	const T &operator *() const
	{
		if(!has_value_assigned_to_it)
			throw std::logic_error(make_exception());
		return value;
	}
	operator T() const
	{
		if(!has_value_assigned_to_it)
			throw std::logic_error(make_exception());
		return value;
	}
	T operator =(const T &new_value)
	{
		value=new_value;
		has_value_assigned_to_it=true;
		return value;
	}
};

#endif
