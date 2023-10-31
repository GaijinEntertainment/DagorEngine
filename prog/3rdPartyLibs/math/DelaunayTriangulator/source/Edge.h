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
/*!	\file ctl/Edge.h
\headerfile ctl/Edge.h
\brief Provides ctl::Edge.
\author Joshua Anghel
*/
#pragma once

#include "ID.h"

namespace ctl {

	class Vertex;
	class Edge;
	class Subdivision;

/*!	\class ctl::Edge ctl/Edge.h ctl.Edge.h
 *	\brief Edge
 *
 *	A single directed Edge from one Vertex to another. Stores minimalistic information about adjacent Edges to allow
 *	Edge traversal in constant time. (Next and Previous Edge about the same Origin.
*/
	class Edge
	{
		friend class Subdivision;

	private:
		ID				id;	
		unsigned char	num;
		Vertex*			point;
		Edge*			next;

	public:
		Edge(void) { }
		~Edge(void) { }

	public:

/*!	\brief Splice two Edges together or apart.
 *
 *	This operator affects the two edge rings around the origins of a and b, and , independently, the two edge
 *	rings around the left faces of a and b. In each case, (i) if the two rings are distinct, Splice will combine
 *	them into one; (ii) if the two are the same ring, Splice will break it into two separate pieces.
 *	Thus, Splice can be used both to attach the two edges together and to break them apart.
 *
 *	\sa Guibus and Stolfi (1985 p.96)
 *	\param a First non-null Edge to Splice.
 *	\param b Second non-null Edge to Splice.
 */
		static void Splice(Edge* a, Edge* b);

/*!	\brief Get the ID value shared by this Edge and all its sibling QuadEdge Edges.
 *	\return ID value of this Edge.
 */
		ID getID(void) const;

/*!	\brief Set the origin Vertex of this Edge.
 *	\param vert Vertex to set Org to.
 */
		void setOrg(Vertex* vert);

/*!	\brief Set the destination Vertex of this Edge.
 *	\param vert Vertex to set Dest to.
 */
		void setDest(Vertex* vert);

/*!	\brief Simultaneously set the origin and destination Vertices of this Edge.
 *	\param org Vertex to set Org to.
 *	\param dest Vertex to set Dest to.
 */
		void setEndPoints(Vertex* org, Vertex* dest);

/*!	\brief Returns the origin of this Edge.
 *	\return Origin of this Edge.
 */
		inline Vertex* Org(void) { return point; }

/*!	\brief Returns the destination of this Edge.
 *
 *	Equivalent to Sym()->Org()
 *	\return Destination of this Edge.
 */
		inline Vertex* Dest(void) { return Sym()->point; }

/* ================================================================================ */
/*	Edge Traversal Operators														*/
/* ================================================================================ */

/*!	\brief Returns the dual-edge pointing to the right.
 *	\return Dual-Edge pointing from the left face to the right face.
 */
		Edge* Rot(void) { return (num < 3) ? this + 1 : this - 3; }

/*!	\brief Returns the Dual-Edge pointing to the left.
 *	\return Dual-Edge pointing from the right face to the left face.
 */
		inline Edge* invRot(void) { return (num > 0) ? this - 1 : this + 3; }

/*!	\brief Returns the Edge symmetric to this one.
 *	\return Edge symmetric to this one.
 */
		inline Edge* Sym(void) { return (num < 2) ? this + 2 : this - 2; }

/*!	\brief Returns the next Edge about the origin with the same origin.
 *	\return Next Edge about the origin with the same origin.
 */
		inline Edge* Onext(void) { return next; }

/*!	\brief Returns the previous Edge about the origin with the same origin.
 *	\return Previous Edge about the origin with the same origin.
 */
		inline Edge* Oprev(void) { return Rot()->Onext()->Rot(); }

/*!	\brief Returns the next Edge about the Right face with the same right face.
 *	\return Next Edge about the right face with the same right face.
 */
		inline Edge* Rnext(void) { return Rot()->Onext()->invRot(); }

/*!	\brief Returns the previous Edge about the Right face with the same right face.
 *	\return Previous Edge about the right face with the same right face.
 */
		inline Edge* Rprev(void) { return Sym()->Onext(); }

/*!	\brief Returns the next Edge about the destination with the same desination.
 *	\return Next Edge about the destination with the same destination.
 */
		inline Edge* Dnext(void) { return Sym()->Onext()->Sym(); }

/*!	\brief Returns the previous Edge about the destination with the same destination.
 *	\return Previous Edge about the destination with the same destination.
 */
		inline Edge* Dprev(void) { return invRot()->Onext()->invRot(); }

/*!	\brief Returns the next Edge about the left face with the same left face.
 *	\return Next Edge about the left face with the same left face.
 */
		inline Edge* Lnext(void) { return invRot()->Onext()->Rot(); }

/*!	\brief Returns the previous Edge about the left face with the same left face.
 *	\return Previous Edge about the left face with the same left face.
 */
		inline Edge* Lprev(void) { return Onext()->Sym(); }

	};

}