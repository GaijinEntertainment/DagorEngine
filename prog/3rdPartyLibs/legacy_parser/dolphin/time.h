
#ifndef __DOLPHIN__TIME_H

#define __DOLPHIN__TIME_H

#include <iostream>

class TimeKeeper
{
	const char *s;
	double start_time;
	
public:
	static double get_time();
	double get_absolute_time()
	{
		return get_time();
	}
	double get_relative_time()
	{
		return get_time()-start_time;
	}
	TimeKeeper(const char *id_string=NULL)
	{
		if(id_string)
			s=id_string;
		else
			s="";
//		std::cout << "TimeKeeper(" << s << "): here.\n";
		start_time=get_time();
	}
	~TimeKeeper();
};

#endif
