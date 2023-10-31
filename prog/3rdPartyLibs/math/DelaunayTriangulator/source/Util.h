/****************************************************************************
Copyright 2017, Cognitics Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is 
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
THE SOFTWARE.
****************************************************************************/
#pragma once

#include "Vector.h"

namespace ctl {

/******************************************************************************************************
	POINT OPERATIONS
*******************************************************************************************************/
	enum PointLineLocation 
	{
		PL_BEFORE_LINE = 0,
		PL_BEGINS_LINE = 1,
		PL_ON_LINE = 2,
		PL_ENDS_LINE = 3,
		PL_AFTER_LINE = 4,
		PL_LEFT_OF_LINE = 5,
		PL_RIGHT_OF_LINE = 6
	};

	PointLineLocation LocatePointOnLine(const Point& p, const Point& a, const Point& b, double epsilon);
	bool	IsLeft(const Point& p, const Point& a, const Point& b, double epsilon);
	bool	IsOn(const Point& p, const Point& a, const Point& b, double epsilon);
	bool	IsRight(const Point& p, const Point& a, const Point& b, double epsilon);
	inline bool IsOnLine(PointLineLocation loc) { return loc <= PL_AFTER_LINE; }

/******************************************************************************************************
	TRIANGLE OPERATIONS
*******************************************************************************************************/
	Point	TCentroid2D(const Point& a, const Point& b, const Point& c);
	Point	TCentroid3D(const Point& a, const Point& b, const Point& c);
	double	TArea2D(const Point& a, const Point& b, const Point& c);
	double	TArea3D(const Point& a, const Point& b, const Point& c);

/******************************************************************************************************
	POLYGON OPERATIONS
*******************************************************************************************************/
	bool    PointInPolygon(Point p, const PointList &polygon, double epsilon = 1e-10);
	PointList ClipToLine(const PointList &polygon, Point a, Point b, double epsilon);

/*
	This implementation actually supports clipping any polygon - convex or concave (so long as its simple...and in fact it might even work for non-
	simple polygons, but I'm not sure). The only constraint is that the clipping region be a closed convex polygon (ie triangle). It handles any case
	of verticle lines while correctly finding all z values through linear interpolation along edges.

	Effectively, the following function will allow clipping an arbitrarily defined 3D polygon defined as a closed (first and last point equal) vector 
	of Points to any convex 2D region extruded upwards to infinity.

	Note that the convex_region MUST be defined CCW.
	The polygon doesn't have to be a particular orientation CCW or CW are fine, but it will return the orientation it was given.
		
	If the convex_region is defined CW this algorithm will actually result in computing the intersection of the diference of the polygon with each half-plane.
	Mathematically, since the convex_region is convex, that means that the funcation will always return nothing (empty vector). Its possible to prove this
	mathematically and is actually the basis for many point in convex polygon detection systems.
*/
	PointList ClipToPolygon(PointList polygon, const PointList &convex_region, double epsilon);

	double PArea2D(const PointList& contour);
	double PArea3D(const PointList& contour);

}