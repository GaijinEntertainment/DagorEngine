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
#include "CGrid.h"

namespace ctl {

	CGrid::CGrid(Point o, Vector u, Vector v, int xsize, int ysize)
	{
		origin = o;
		U = u;
		V = v;
		XSize = xsize;
		YSize = ysize;

		data.resize(xsize);
		for (int i = 0; i < xsize; i++)
			data[i].resize(ysize);
		
		for (int i = 0; i < xsize; i++)
			for (int j = 0; j < ysize; j++)
				data[i][j] = 0;
	}

	Point CGrid::getPoint(int x, int y)
	{
		return origin + U*x + V*y;
	}

	PointList CGrid::getEnvelope(void)
	{
		PointList points;
		points.resize(4);
		points[0] = getPoint(0,0) + Point(0,0,1)*getValue(0,0);
		points[1] = getPoint(XSize-1,0)  + Point(0,0,1)*getValue(XSize-1,0);
		points[2] = getPoint(XSize-1,YSize-1)  + Point(0,0,1)*getValue(XSize-1,YSize-1);
		points[3] = getPoint(0,YSize-1)  + Point(0,0,1)*getValue(0,YSize-1);
		return points;
	}

	double CGrid::getValue(int x, int y)
	{
		return data[x][y];
	}

	void CGrid::setValue(int x, int y, double value)
	{
		data[x][y] = value;
	}

	Point CGrid::getOrigin(void)
	{
		return origin;
	}

	void CGrid::setOrigin(Point org)
	{
		origin = org;
	}

	int CGrid::getXSize(void)
	{
		return XSize;
	}

	int CGrid::getYSize(void)
	{
		return YSize;
	}

	Vector CGrid::getU(void)
	{
		return U;
	}

	Vector CGrid::getV(void)
	{
		return V;
	}

	void CGrid::setU(Vector u)
	{
		U = u;
	}

	void CGrid::setV(Vector v)
	{
		V = v;
	}

}