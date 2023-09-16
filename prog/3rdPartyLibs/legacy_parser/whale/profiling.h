
#ifndef __PROFILING_H__
#define __PROFILING_H__

#define __PROFILING_H__TURN_IT_OFF

#include <iostream>
using namespace std;

#ifndef __PROFILING_H__TURN_IT_OFF

class ticks
{
#ifdef __WIN32__
	union {
		unsigned __int64 val;
		unsigned __int64 _ticks_val_489508349581424;
	};
	ticks(__int64 v) : val(v) {}
#else
	#error Need windows still
#endif

public:

	static ticks now();
	static float compute_scale();

	static float scale;

	ticks() : val(0) {}

	ticks& operator+=(const ticks &a)
	{ val += a.val; return *this; }

	ticks& operator-=(const ticks &a)
	{  val -= a.val; return *this; }

	float seconds() const {
		if (scale == 0.0) scale = compute_scale();
		return scale*val;
	}

	friend ticks operator+(const ticks &t1, const ticks &t2)
	{ return ticks(t1.val + t2.val); }

	friend ticks operator-(const ticks &t1, const ticks &t2)
	{ return ticks(t1.val - t2.val); }

	friend ostream& operator<<(ostream &os, const ticks &t)
	{
		if (t.val > 0xFFFFFFFF) os << (t.val >> 32);
		os << int(t.val & 0xFFFFFFFF) << " CPU cycles ("
			<< t.seconds() << " secs)";
		return os;
	}

};

class ticks_counter
{
	ticks _val;
	ticks last;
public:
	void reset() { _val = ticks(); }
	void start() { last = ticks::now(); }
	void stop()  { _val += ticks::now() - last; }
	ticks val()	 const { return _val; }
	friend ostream& operator<<(ostream &os, const ticks_counter &tc)
	{
		return os << tc.val();
	}
};

#endif

class ticks_keeper
{
	string name;
#ifndef __PROFILING_H__TURN_IT_OFF
	ticks_counter counter;
#endif
	ostream &os;
public:
	ticks_keeper(const string &name = "", ostream &os = cout)
		: name(name), os(os)
	{
	#ifndef __PROFILING_H__TURN_IT_OFF
		counter.start();
	#endif
	}
	~ticks_keeper() {
	#ifndef __PROFILING_H__TURN_IT_OFF
		counter.stop();
	#endif
		os << "tick counter(" << name << ") : ";
	#ifdef __PROFILING_H__TURN_IT_OFF
		os << "hell knows.";
	#else
		os << counter;
	#endif
		os << endl;
	}
};

#endif
