// This code is in the public domain -- castanyo@yahoo.es

#include "plane.h"
#include "matrix.h"

namespace nv
{
	Plane transformPlane(const Matrix& m, Plane::Arg p)
	{
		Vector3 newVec = transformVector(m, p.vector());

		Vector3 ptInPlane = p.offset() * p.vector();
		ptInPlane = transformPoint(m, ptInPlane);

		return Plane(newVec, ptInPlane);
	}
}
