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
/*!	\file ctl/Vector.h
\headerfile ctl/Vector.h
\brief Provides ctl::Vector.
\author Joshua Anghel
*/
#pragma once

#include <vector>

namespace ctl {

/*!	\class ctl::Vector ctl/Vector.h ctl/Vector.h
\brief Vector

3D coordinate for ctl, with ability to perform normalization and other common vector operations.
*/
	class Vector
	{
	public:
		double		x;
		double		y;
		double		z;

		Vector(double _x = 0, double _y = 0, double _z = 0) : x(_x), y(_y), z(_z) { }
		~Vector(void) { }

		double& operator[](int index);
		double operator[](int index) const;

		bool operator==(const Vector& other) const;
		bool operator!=(const Vector& other) const;
		Vector operator+(const Vector& other) const;
		Vector& operator+=(const Vector& other);
		Vector operator-(const Vector& other) const;
		Vector& operator-=(const Vector& other);

		Vector& operator*=(double a);

/*	Dot Product					*/
		double dot(const Vector& other) const;

/*	Cross Product				*/
		Vector cross(const Vector& other) const;
		Vector operator^(const Vector& other) const;

/*	Length and Normalization	*/
		double length(void) const;
		double length2D(void) const;
		double length2D2(void) const;
		Vector& normalize(void);

/*	Equal check with epsilon	*/
		bool equals(Vector& other, double epsilon) const;

		void set(double x, double y, double z);
	};

	Vector operator*(double a, const Vector& b);
	Vector operator*(const Vector& a, double b);
	double operator*(const Vector& a, const Vector& b);

	typedef Vector Point;
	typedef std::vector<Point> PointList;
}