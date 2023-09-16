/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2009 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#if defined(_WIN32) || defined(__i386__)
#define BT_USE_SSE_IN_API
#endif

#include "btConvexShape.h"
#include "btTriangleShape.h"
#include "btSphereShape.h"
#include "btCylinderShape.h"
#include "btConeShape.h"
#include "btCapsuleShape.h"
#include "btConvexHullShape.h"
#include "btConvexPointCloudShape.h"

///not supported on IBM SDK, until we fix the alignment of btVector3
#if defined(_TARGET_C0) && defined(__SPU__)








#endif  //__SPU__

btConvexShape::btConvexShape()
{
}

btConvexShape::~btConvexShape()
{
}

void btConvexShape::project(const btTransform& trans, const btVector3& dir, btScalar& min, btScalar& max, btVector3& witnesPtMin, btVector3& witnesPtMax) const
{
	btVector3 localAxis = dir * trans.getBasis();
	btVector3 vtx1 = trans(localGetSupportingVertex(localAxis));
	btVector3 vtx2 = trans(localGetSupportingVertex(-localAxis));

	min = vtx1.dot(dir);
	max = vtx2.dot(dir);
	witnesPtMax = vtx2;
	witnesPtMin = vtx1;

	if (min > max)
	{
		btScalar tmp = min;
		min = max;
		max = tmp;
		witnesPtMax = vtx1;
		witnesPtMin = vtx2;
	}
}

static btVector3 convexHullSupport(const btVector3& localDirOrg, const btVector3* points, int numPoints, const btVector3& localScaling)
{
	btVector3 vec = localDirOrg * localScaling;

#if defined(_TARGET_C0) && defined(__SPU__)










































#else

	btScalar maxDot;
	long ptIndex = vec.maxDot(points, numPoints, maxDot);
	btAssert((unsigned)ptIndex < numPoints);
	if ((unsigned)ptIndex >= numPoints)
	{
		ptIndex = 0;
	}
	btVector3 supVec = points[ptIndex] * localScaling;
	return supVec;
#endif  //__SPU__
}

btVector3 btConvexShape::localGetSupportVertexWithoutMarginNonVirtual(const btVector3& localDir) const
{
	switch (m_shapeType)
	{
		case SPHERE_SHAPE_PROXYTYPE:
		{
			return btVector3(0, 0, 0);
		}
		case BOX_SHAPE_PROXYTYPE:
		{
			btBoxShape* convexShape = (btBoxShape*)this;
			const btVector3& halfExtents = convexShape->getImplicitShapeDimensions();

#if defined(__APPLE__) && (defined(BT_USE_SSE) || defined(BT_USE_NEON))
#if defined(BT_USE_SSE)
			return btVector3(_mm_xor_ps(_mm_and_ps(localDir.mVec128, (__m128){-0.0f, -0.0f, -0.0f, -0.0f}), halfExtents.mVec128));
#elif defined(BT_USE_NEON)
			return btVector3((float32x4_t)(((uint32x4_t)localDir.mVec128 & (uint32x4_t){0x80000000, 0x80000000, 0x80000000, 0x80000000}) ^ (uint32x4_t)halfExtents.mVec128));
#else
#error unknown vector arch
#endif
#else
			return btVector3(btFsels(localDir.x(), halfExtents.x(), -halfExtents.x()),
							 btFsels(localDir.y(), halfExtents.y(), -halfExtents.y()),
							 btFsels(localDir.z(), halfExtents.z(), -halfExtents.z()));
#endif
		}
		case TRIANGLE_SHAPE_PROXYTYPE:
		{
			btTriangleShape* triangleShape = (btTriangleShape*)this;
			return chooseMaxAxis(localDir, &triangleShape->m_vertices1[0]);
		}
		case CYLINDER_SHAPE_PROXYTYPE:
		{
			btCylinderShape* cylShape = (btCylinderShape*)this;
			//mapping of halfextents/dimension onto radius/height depends on how cylinder local orientation is (upAxis)

			btVector3 halfExtents = cylShape->getImplicitShapeDimensions();
			btVector3 v(localDir);
			int cylinderUpAxis = cylShape->getUpAxis();
			int XX(1), YY(0), ZZ(2);

			switch (cylinderUpAxis)
			{
				case 0:
				{
					XX = 1;
					YY = 0;
					ZZ = 2;
				}
				break;
				case 1:
				{
					XX = 0;
					YY = 1;
					ZZ = 2;
				}
				break;
				case 2:
				{
					XX = 0;
					YY = 2;
					ZZ = 1;
				}
				break;
				default:
					btAssert(0);
					break;
			};

			btScalar radius = halfExtents[XX];
			btScalar halfHeight = halfExtents[cylinderUpAxis];

			btVector3 tmp;

			btScalar s = btSqrt(v[XX] * v[XX] + v[ZZ] * v[ZZ]);
			if (s != btScalar(0.0))
			{
				btScalar d = radius / s;
				tmp[XX] = v[XX] * d;
				tmp[ZZ] = v[ZZ] * d;
			}
			else
			{
				tmp[XX] = radius;
				tmp[ZZ] = btScalar(0.0);
			}
			tmp[YY] = v[YY] < 0.0 ? -halfHeight : halfHeight;
			return tmp;
		}
		case CAPSULE_SHAPE_PROXYTYPE:
		{
			btVector3 vec0(localDir);

			btCapsuleShape* capsuleShape = (btCapsuleShape*)this;
			btScalar halfHeight = capsuleShape->getHalfHeight();
			int capsuleUpAxis = capsuleShape->getUpAxis();

			vec0.normalize();

			btVector3 vtx = vec0 * (capsuleShape->getRadius() - capsuleShape->getMarginNV());
			btVector3 pos(halfHeight * (capsuleUpAxis == 0),halfHeight * (capsuleUpAxis == 1), halfHeight * (capsuleUpAxis == 2));
			if (vec0[capsuleUpAxis] > 0)
				return vtx + pos;
			return vtx - pos;
		}
		case CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE:
		{
			btConvexPointCloudShape* convexPointCloudShape = (btConvexPointCloudShape*)this;
			btVector3* points = convexPointCloudShape->getUnscaledPoints();
			int numPoints = convexPointCloudShape->getNumPoints();
			return convexHullSupport(localDir, points, numPoints, convexPointCloudShape->getLocalScalingNV());
		}
		case CONVEX_HULL_SHAPE_PROXYTYPE:
		{
			btConvexHullShape* convexHullShape = (btConvexHullShape*)this;
			btVector3* points = convexHullShape->getUnscaledPoints();
			int numPoints = convexHullShape->getNumPoints();
			return convexHullSupport(localDir, points, numPoints, convexHullShape->getLocalScalingNV());
		}
		default:
#ifndef __SPU__
			return this->localGetSupportingVertexWithoutMargin(localDir);
#else
			btAssert(0);
#endif
	}

	// should never reach here
	btAssert(0);
	return btVector3(btScalar(0.0f), btScalar(0.0f), btScalar(0.0f));
}

btVector3 btConvexShape::localGetSupportVertexNonVirtual(const btVector3& localDir) const
{
	btVector3 localDirNorm = localDir;
	if (localDirNorm.length2() < (SIMD_EPSILON * SIMD_EPSILON))
	{
		localDirNorm.setValue(btScalar(-1.), btScalar(-1.), btScalar(-1.));
	}
	localDirNorm.normalize();

	return localGetSupportVertexWithoutMarginNonVirtual(localDirNorm) + getMarginNonVirtual() * localDirNorm;
}

/* TODO: This should be bumped up to btCollisionShape () */
btScalar btConvexShape::getMarginNonVirtual() const
{
	switch (m_shapeType)
	{
		case SPHERE_SHAPE_PROXYTYPE:
		{
			btSphereShape* sphereShape = (btSphereShape*)this;
			return sphereShape->getRadius();
		}
		case BOX_SHAPE_PROXYTYPE:
		{
			btBoxShape* convexShape = (btBoxShape*)this;
			return convexShape->getMarginNV();
		}
		case TRIANGLE_SHAPE_PROXYTYPE:
		{
			btTriangleShape* triangleShape = (btTriangleShape*)this;
			return triangleShape->getMarginNV();
		}
		case CYLINDER_SHAPE_PROXYTYPE:
		{
			btCylinderShape* cylShape = (btCylinderShape*)this;
			return cylShape->getMarginNV();
		}
		case CONE_SHAPE_PROXYTYPE:
		{
			btConeShape* conShape = (btConeShape*)this;
			return conShape->getMarginNV();
		}
		case CAPSULE_SHAPE_PROXYTYPE:
		{
			btCapsuleShape* capsuleShape = (btCapsuleShape*)this;
			return capsuleShape->getMarginNV();
		}
		case CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE:
		/* fall through */
		case CONVEX_HULL_SHAPE_PROXYTYPE:
		{
			btPolyhedralConvexShape* convexHullShape = (btPolyhedralConvexShape*)this;
			return convexHullShape->getMarginNV();
		}
		default:
#ifndef __SPU__
			return this->getMargin();
#else
			btAssert(0);
#endif
	}

	// should never reach here
	btAssert(0);
	return btScalar(0.0f);
}
#ifndef __SPU__
void btConvexShape::getAabbNonVirtual(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	switch (m_shapeType)
	{
		case SPHERE_SHAPE_PROXYTYPE:
		{
			btSphereShape* sphereShape = (btSphereShape*)this;
			btScalar radius = sphereShape->getImplicitShapeDimensions().getX();  // * convexShape->getLocalScaling().getX();
			btScalar margin = radius + sphereShape->getMarginNonVirtual();
			const btVector3& center = t.getOrigin();
			btVector3 extent(margin, margin, margin);
			aabbMin = center - extent;
			aabbMax = center + extent;
		}
		break;
		case CYLINDER_SHAPE_PROXYTYPE:
		/* fall through */
		case BOX_SHAPE_PROXYTYPE:
		{
			btBoxShape* convexShape = (btBoxShape*)this;
			btScalar margin = convexShape->getMarginNonVirtual();
			btVector3 halfExtents = convexShape->getImplicitShapeDimensions();
			halfExtents += btVector3(margin, margin, margin);
			btMatrix3x3 abs_b = t.getBasis().absolute();
			btVector3 center = t.getOrigin();
			btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);

			aabbMin = center - extent;
			aabbMax = center + extent;
			break;
		}
		case TRIANGLE_SHAPE_PROXYTYPE:
		{
			btTriangleShape* triangleShape = (btTriangleShape*)this;
			btScalar margin = triangleShape->getMarginNonVirtual();
			for (int i = 0; i < 3; i++)
			{
				btVector3 vec(btScalar(0.), btScalar(0.), btScalar(0.));
				vec[i] = btScalar(1.);

				btVector3 sv = localGetSupportVertexWithoutMarginNonVirtual(vec * t.getBasis());

				btVector3 tmp = t(sv);
				aabbMax[i] = tmp[i] + margin;
				vec[i] = btScalar(-1.);
				tmp = t(localGetSupportVertexWithoutMarginNonVirtual(vec * t.getBasis()));
				aabbMin[i] = tmp[i] - margin;
			}
		}
		break;
		case CAPSULE_SHAPE_PROXYTYPE:
		{
			btCapsuleShape* capsuleShape = (btCapsuleShape*)this;
			btVector3 halfExtents(capsuleShape->getRadius(), capsuleShape->getRadius(), capsuleShape->getRadius());
			int m_upAxis = capsuleShape->getUpAxis();
			halfExtents[m_upAxis] = capsuleShape->getRadius() + capsuleShape->getHalfHeight();
			btMatrix3x3 abs_b = t.getBasis().absolute();
			btVector3 center = t.getOrigin();
			btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);
			aabbMin = center - extent;
			aabbMax = center + extent;
		}
		break;
		case CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE:
		case CONVEX_HULL_SHAPE_PROXYTYPE:
		{
			btPolyhedralConvexAabbCachingShape* convexHullShape = (btPolyhedralConvexAabbCachingShape*)this;
			btScalar margin = convexHullShape->getMarginNonVirtual();
			convexHullShape->getNonvirtualAabb(t, aabbMin, aabbMax, margin);
		}
		break;
		default:
#ifndef __SPU__
			this->getAabb(t, aabbMin, aabbMax);
#else
			btAssert(0);
#endif
			break;
	}

	// should never reach here
	btAssert(0);
}

#endif  //__SPU__
