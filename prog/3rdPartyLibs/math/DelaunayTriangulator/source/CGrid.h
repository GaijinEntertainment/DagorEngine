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
#include <vector>

namespace ctl {

/*! \brief Cartesean Grid

	Provides an interface for a cartesean grid, may be used to represent a 3D surface, or the isovalues of a volume
	along a plane. Origin must be at the bottom left.
	
	U and V are vectors representing the two directions of the grid (traditionally x and y). Their lengths are the 
	dimensions of a single grid cell. By customizing which U and V to use, a non square grid can be generated with
	minimal effort.
*/
	class CGrid
	{
	protected:
		Point	origin;
		Vector	U;
		Vector	V;
		int		XSize, YSize;

		std::vector<std::vector<double> >	data;

	public:
		CGrid(Point o, Point u, Point v, int xsize, int ysize);
		~CGrid(void) { }

		Point getPoint(int x, int y);

		PointList getEnvelope(void);

		double getValue(int x, int y);
		void setValue(int x, int y, double value);

		Point getOrigin(void);
		void setOrigin(Point org);

		int getXSize(void);
		int getYSize(void);

		Vector getU(void);
		Vector getV(void);

		void setU(Vector u);
		void setV(Vector v);

	};

}