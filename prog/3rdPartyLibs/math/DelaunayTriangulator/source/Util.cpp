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
#include "Util.h"
#include <cmath>

using std::abs;

namespace ctl {

/******************************************************************************************************
	POINT OPERATIONS
*******************************************************************************************************/
	
	PointLineLocation LocatePointOnLine(const Point& p, const Point& a, const Point& b, double epsilon)
	{
		Vector u = b - a;
		Vector v = p - a;

		double du = abs(u.x) + abs(u.y); // Absolute value norm for distance
		// Avoid the division and handle gracefully aligned points
		double area = (v.x*u.y - v.y*u.x);

		// if area/du < -epsilon return PL_LEFT_OF_LINE
		if (area<-du*epsilon)
			return PL_LEFT_OF_LINE;

		// if (area/du > epsilon) return PL_RIGHT_OF_LINE
		if (area>du*epsilon)
			return PL_RIGHT_OF_LINE;
		{
			double da = abs(v.x) + abs(v.y);
			Vector w = p - b;
			double db = abs(w.x) + abs(w.y);

			if (da < epsilon) return PL_BEGINS_LINE;
			else if (db < epsilon) return PL_ENDS_LINE;
			else if (da > du) return PL_AFTER_LINE;
			else if (db > du) return PL_BEFORE_LINE;
			else return PL_ON_LINE;
		}
	}

	bool IsLeft(const Point& p, const Point& a, const Point& b, double epsilon)
	{
		Vector u = b - a;
		Vector v = p - a;

		double du = abs(u.x) + abs(u.y); // Absolute value norm for distance
		// Avoid the division and handle gracefully aligned points
		double area = (v.x*u.y - v.y*u.x);
		return area < -du*epsilon;

//        return area/du < -epsilon;
	}

	bool IsOn(const Point& p, const Point& a, const Point& b, double epsilon)
	{
		Vector u = b - a;
		Vector v = p - a;

		double du = abs(u.x) + abs(u.y); // Absolute value norm for distance

		// Avoid the division to handle gracefully aligned points
		double area = (v.x*u.y - v.y*u.x);

		return abs(area) <= du*epsilon;
		// return abs(area/du) <= epsilon;
	}

	bool IsRight(const Point& p, const Point& a, const Point& b, double epsilon)
	{
		Vector u = b - a;
		Vector v = p - a;

		double du = abs(u.x) + abs(u.y); // Absolute value norm for distance

		// Avoid the division and handle gracefully aligned points
		double area = (v.x*u.y - v.y*u.x);
		return area > du*epsilon;

//        return (area/du) > epsilon;
	}

/******************************************************************************************************
	TRIANGLE OPERATIONS
*******************************************************************************************************/
	Point TCentroid2D(const Point& a, const Point& b, const Point& c)
	{
		Point temp = (1.0/3.0)*(a+b+c);
		temp.z = 0;
		return temp;
	}

	Point TCentroid3D(const Point& a, const Point& b, const Point& c)
	{
		return (1.0/3.0)*(a+b+c);
	}

	double TArea2D(const Point& a, const Point& b, const Point& c)
	{
		return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
	}

	double TArea3D(const Point& a, const Point& b, const Point& c)
	{
		return 0.5*((b-a).cross(c-a)).length();
	}


/******************************************************************************************************
	POLYGON OPERATIONS
*******************************************************************************************************/
	bool PointInPolygon(Point p, const PointList &polygon, double epsilon)
	{
		int Rcross = 0;
		int Lcross = 0;

		// If the polygon is not closed, add a new edge from the last to the first point
		int n = int(polygon.size());
		if ((n>0) && !(polygon[0]==polygon.back()))
			n++;
		for (int i = 1; i < n; i++)
		{
			Point p1 = polygon[i-1];
			int pointIndex = i < polygon.size() ? i : 0;
			Point p2 = polygon[pointIndex];

			if ( p.x == p1.x && p.y == p1.y) return true;

			double	height = p.y;
			double	x;
			bool	Rstrad, Lstrad;

			//Rstrad is false if the point only touches ray, but is mostly below it				
			//Lstrad is false if the point only touches ray, but is mostly above it
			Rstrad = ( p2.y > height - epsilon ) != ( p1.y > height - epsilon );				
			Lstrad = ( p2.y < height + epsilon ) != ( p1.y < height + epsilon );

			if (Rstrad || Lstrad)
			{
				x = ( (p2.x - p.x)*(p1.y - height) - (p1.x - p.x)*(p2.y - height) ) 
					/ ( p1.y - p2.y );
				if ( Rstrad && x > epsilon ) Rcross++;
				if ( Lstrad && x < -epsilon ) Lcross++;
			//	Point is on polygons edge
				if (abs(x) < epsilon)
					return true;
			}
		}

	//check cross countings to determine intersection
		if ( (Rcross % 2) != (Lcross % 2) )		return true;
		else if ( (Rcross % 2) == 1 )			return true;
		else									return false;
	}

	PointList ClipToLine(const PointList &polygon, Point a, Point b, double epsilon)
	{
		PointList result;
		if(polygon.size() < 3) return result;
		
		bool inside = !IsRight(a,b,polygon[0],epsilon);
		if (inside) result.push_back(polygon[0]);
		for (int i = 1; i < int(polygon.size()); i++)
		{
			bool next = !IsRight(a,b,polygon[i],epsilon);
			if (next != inside)
			{
			/*	Test if an edge (b-a) intersects a line (d-c). Finds the intersection by computing the normalized distance along the edge (b-a) the intersection
				point is located at. This allows not only faster computation, but implicite linear interpolation of the z value.

				Assumes that b-a crosses d-c and is not collinear.
			*/
				double dx = b.x - a.x;
				double dy = b.y - a.y;
				double dist = ( dx*(polygon[i-1].y - a.y) - dy*(polygon[i-1].x - a.x) )/( dy*(polygon[i].x - polygon[i-1].x) - dx*(polygon[i].y - polygon[i-1].y) );
				if (dist > epsilon && dist < (1-epsilon)) result.push_back( Point( polygon[i-1].x + (polygon[i].x - polygon[i-1].x)*dist, polygon[i-1].y + (polygon[i].y - polygon[i-1].y)*dist, polygon[i-1].z + (polygon[i].z - polygon[i-1].z)*dist ) );
			}
			if (next)
			{
				if (result.empty())
					result.push_back(polygon[i]);
				else if ( (polygon[i] - result.back()).length() > epsilon)
					result.push_back(polygon[i]);
			}
			inside = next;
		}

	//	At this point, the polygon might not be closed if it did not start off inside the polygon, so close it if not.
		if (!result.empty())
		{
			if (!(result.front() == result.back())) result.push_back(result.front());
		}

		return result;
	}

	PointList ClipToPolygon(PointList polygon, const PointList &convex_region, double epsilon)
	{
		int n = int(convex_region.size());
		for (int i = 1; i < n; i++)
			polygon = ClipToLine(polygon,convex_region[i-1],convex_region[i],epsilon);

		// If the convex region is open, just add one additional point
		if ((n>1) && !(convex_region[0]==convex_region.back()))
			polygon = ClipToLine(polygon, convex_region[n-1], convex_region[0], epsilon);
		return polygon;
	}

	double PArea2D(const PointList& contour)
	{
		double area = 0;
		int n = int(contour.size());
		for (int i=0; i<n-1; i++)
			area += (contour[i][0] + contour[i+1][0])*(contour[i][1] - contour[i+1][1]);
		return area / 2.0;
	}

	double PArea3D(const PointList& contour)
	{
		int n = int(contour.size());
		Vector N;
		double area = 0;
		double ax, ay, az;
		int coord;

		for (int i = 0; i < n-1; i++)
		{
			Point v0 = contour[i];
			Point v1 = contour[i+1];
			N[0] += (v0.y - v1.y)*(v0.z + v1.z);
			N[1] += (v0.z - v1.z)*(v0.x + v1.x);
			N[2] += (v0.x - v1.x)*(v0.y + v1.y);
		}

		N.normalize();
		ax = abs(N.x); ay = fabs(N.y); az = fabs(N.z);

		if (ax > ay && ax > az)
			coord = 1;
		else if (ay > ax) 
			coord = 2;
		else
			coord = 3;

		int prev, current, next;
		for (prev = n-1, current = 0, next = 1; current < n; prev=(prev+1)%n, current++, next=(next+1)%n)
		{
			switch(coord)
			{
			case 1:
				area += (contour[current].y * (contour[next].z - contour[prev].z) );
				break;
			case 2:
				area += (contour[current].x * (contour[next].z - contour[prev].z) );
				break;
			case 3:
				area += (contour[current].x * (contour[next].y - contour[prev].y) );
				break;
			default:
				break;
			}
		}

		switch(coord)
		{
		case 1:
			return area / (2.0*ax);
		case 2:
			return area / (2.0*ay);
		case 3:
			return area / (2.0*az);
		default:
			return 0;
		}
	}

}