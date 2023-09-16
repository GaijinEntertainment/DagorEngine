/** @file Rect.h
	@author Jukka Jylänki

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <EASTL/vector.h>
#include <cassert>
//#include <debug/dag_assert.h>

#ifdef _DEBUG
/// debug_assert is an assert that also requires debug mode to be defined.
#define debug_assert(x) assert(x)
#else
#define debug_assert(x)
#endif
#define rbp_assert(x) assert(x)
//#define rbp_assert(x) G_ASSERT(x)

//using namespace std;

namespace rbp {

struct RectSize
{
	int width;
	int height;
};

struct Rect
{
	int x;
	int y;
	int width;
	int height;
};

struct RectSizeRequest:public RectSize //for multiple add
{
  int id;
  RectSizeRequest(){}
  RectSizeRequest(const RectSize &a, int id_):RectSize(a), id(id_) {}
};
struct RectResult:public Rect //for multiple add
{
  int id;
  RectResult(){}
  RectResult(const Rect &a, int id_):Rect(a), id(id_) {}
};

/// Performs a lexicographic compare on (rect short side, rect long side).
/// @return -1 if the smaller side of a is shorter than the smaller side of b, 1 if the other way around.
///   If they are equal, the larger side length is used as a tie-breaker.
///   If the rectangles are of same size, returns 0.
int CompareRectShortSide(const Rect &a, const Rect &b);

/// Performs a lexicographic compare on (x, y, width, height).
int NodeSortCmp(const Rect &a, const Rect &b);

/// Returns true if a is contained in b.
bool IsContainedIn(const Rect &a, const Rect &b);

class DisjointRectCollection
{
public:
	eastl::vector<Rect> rects;

	bool Add(const Rect &r)
	{
		// Degenerate rectangles are ignored.
		if (r.width == 0 || r.height == 0)
			return true;

		if (!Disjoint(r))
			return false;
		rects.push_back(r);
		return true;
	}

	void Clear()
	{
		rects.clear();
	}

	bool Disjoint(const Rect &r) const
	{
		// Degenerate rectangles are ignored.
		if (r.width == 0 || r.height == 0)
			return true;

		for(unsigned i = 0; i < rects.size(); ++i)
			if (!Disjoint(rects[i], r))
				return false;
		return true;
	}

	static bool Disjoint(const Rect &a, const Rect &b)
	{
		if (a.x + a.width <= b.x ||
			b.x + b.width <= a.x ||
			a.y + a.height <= b.y ||
			b.y + b.height <= a.y)
			return true;
		return false;
	}
};

inline bool IsContainedIn(const Rect &a, const Rect &b)
{
	return a.x >= b.x && a.y >= b.y 
		&& a.x+a.width <= b.x+b.width 
		&& a.y+a.height <= b.y+b.height;
}

}
