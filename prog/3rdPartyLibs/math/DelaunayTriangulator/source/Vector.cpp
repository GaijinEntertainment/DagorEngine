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
#include "Vector.h"
#include <cmath>
#include <exception>
#include <stdexcept>
namespace ctl {

	double& Vector::operator[](int index)
	{
		switch(index)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		default:
			throw std::runtime_error("Vector::operator[] Index out of bounds!");
		}
	}

	double Vector::operator[](int index) const
	{
		switch(index)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		default:
			throw std::runtime_error("Vector::operator[] Index out of bounds!");
		}
	}

	bool Vector::operator==(const Vector& other) const
	{
		if (x!=other.x) return false;
		else if (y!=other.y) return false;
		//else if (z!=other.z) return false;
		else return true;
	}

	bool Vector::operator!=(const Vector& other) const
	{
		return !((*this)==other);
	}

	Vector Vector::operator+(const Vector &other) const
	{
		Vector vect = *this;
		vect.x += other.x;
		vect.y += other.y;
		vect.z += other.z;
		return vect;
	}

	Vector& Vector::operator+=(const Vector& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	Vector Vector::operator-(const Vector& other) const
	{
		Vector vect = *this;
		vect.x -= other.x;
		vect.y -= other.y;
		vect.z -= other.z;
		return vect;
	}

	Vector& Vector::operator-=(const Vector& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	Vector& Vector::operator*=(double a)
	{
		x *= a;
		y *= a;
		z *= a;
		return *this;
	}

	double Vector::dot(const Vector& other) const
	{
		return x*other.x + y*other.y + z*other.z;
	}

	Vector Vector::cross(const Vector& other) const
	{
		return (*this)^other;
	}

	Vector Vector::operator^(const Vector& other) const
	{
		return Vector
		(
			y*other.z - z*other.y,
			z*other.x - x*other.z,
			x*other.y - y*other.x
		);
	}

	double Vector::length(void) const
	{
		return sqrt(x*x + y*y + z*z);
	}

	double Vector::length2D(void) const
	{
		return sqrt(x*x + y*y);
	}

	double Vector::length2D2(void) const
	{
		return x*x + y*y;
	}

	Vector& Vector::normalize(void)
	{
		double l = 1.0/length();
		x *= l;
		y *= l;
		z *= l;
		return *this;
	}

	bool Vector::equals(Vector& other, double epsilon) const
	{
		if (epsilon == 0)
		{
			if (x != other.x) 
				return false;
			else if (y != other.y) 
				return false;
			else
				return true;
		}
		else
		{
			double dist = std::abs(x-other.x) + std::abs(y-other.y); // Use L1 norm
			return dist < epsilon;
		}
	}

	Vector operator*(double a, const Vector& b)
	{
		Vector v = b;
		v.x *= a;
		v.y *= a;
		v.z *= a;
		return v;
	}

	Vector operator*(const Vector& a, double b)
	{
		return (b*a);
	}

	double operator*(const Vector& a, const Vector& b)
	{
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}

	void Vector::set(double x, double y, double z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
}