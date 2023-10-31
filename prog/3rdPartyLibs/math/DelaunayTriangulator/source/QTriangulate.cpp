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
#include "QTriangulate.h"
#include "Util.h"

namespace ctl {

	bool QTriangulate::snip(PointList contour, int u, int v, int w, int n, const std::vector<int>& V, const Point& normal)
	{
		Point A = contour[V[u]];
		Point B = contour[V[v]];
		Point C = contour[V[w]];

		if ( (B-A).cross(C-A).dot(normal) < 1e-10 )
			return false;
		//TArea3D(A,B,C) < 1e-10) return false;

		for (int p = 0; p < n; p++)
		{
			if ( (p==u) || (p==v) || (p==w) ) continue;
			Point P = contour[V[p]];

			Vector v0 = C - A;
			Vector v1 = B - A;
			Vector v2 = P - A;

			double dot00 = v0 * v0;
			double dot01 = v0 * v1;
			double dot02 = v0 * v2;
			double dot11 = v1 * v1;
			double dot12 = v1 * v2;

			double invDenom = 1.0/ (dot00 * dot11 - dot01 * dot01);
			double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
			double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

			if ((u >= 0) && (v >= 0) && (u + v <= 1))
				return false;
		}

		return true;
	}

	PointList QTriangulate::apply(PointList contour)
	{
		int n = int(contour.size());
		if (n < 3) return PointList();

		PointList result;
		std::vector<int> V(n);

		Point normal;
		for (int i = 0; i < n; i++)
		{
			Point v0 = contour[i];
			Point v1 = contour[(i+1)%n];
			normal[0] += (v0.y - v1.y)*(v0.z + v1.z);
			normal[1] += (v0.z - v1.z)*(v0.x + v1.x);
			normal[2] += (v0.x - v1.x)*(v0.y + v1.y);
		}

		for (int v = 0; v < n; v++) V[v] = v;

		int nv = n;
		int count = 2*nv;

		for (int v = nv-1; nv>2;)
		{
			if (0 >= (count--))	//	Bad polygon
				return PointList();

			int u = v; if (nv <= u) u = 0;
			v = u+1; if (nv <= v) v = 0;
			int w = v+1; if (nv <= w) w = 0;

			if (snip(contour,u,v,w,nv,V,normal))
			{
				result.push_back(contour[V[u]]);
				result.push_back(contour[V[v]]);
				result.push_back(contour[V[w]]);
				for (int s = v, t=v+1; t<nv; s++, t++) 
					V[s] = V[t]; 
				nv--;
				count = 2*nv;
			}
		}

		return result;
	}

}