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
/*!	\file ctl/Vertex.h
\headerfile ctl/Vertex.h
\brief Provides ctl::Vertex.
\author Joshua Anghel
*/
#pragma once

#include "Edge.h"
#include "Vector.h"

namespace ctl {

/*!	\class ctl::Vertex ctl/Vertex.h ctl/Vertex.h
	\brief Vertex
 
	ID		Unique ID value for this Vertex.
	Point	Location of this Vertex.
	Edges	Edges radiating outword from this Vertex.
 
*/
	class Vertex
	{
		friend class Subdivision;

	private:
		ID			id;
		Edge*		edge;

	public:
		Vertex(void) { }
		~Vertex(void) { }
	
		Vector		point;

/*!	\brief Get the ID of this Vertex.
 *	\return The unique ID assigned to this Vertex.
 */
		ID getID(void);

/*!	\brief Link an Edge to this Vertex.
 *	\param edge Edge to link to this Vertex.
 */
		void addEdge(Edge* edge);

/*!	\brief Remove an Edge linked to this Vertex.
 *	\param edge Edge to unlink from this Vertex.
 */
		void removeEdge(Edge* edge);

		void clearEdge(void);

/*!	\brief Returns an arbitrary Edge linked to this Vertex.
 *
 *	All Edges linked to this Vertex can be found by iterating around the returned Edge using Edge::Onext().
 *	\return An arbitrary Edge linked to this Vertex.
 */
		Edge* getEdges(void);
		
/*!	\brief Returns the area weighted normal of this Vertex.
 *	
 *	Uses the linked Edges of this Vertex to compute the normals of all adjacent triangles. If there are no
 *	Adjacent Edges, the normal will be (0,0,0).
 *	\return The area weighted normal of this Vertex.
 */
		Vector getAreaWeightedNormal(void);

//!	\brief Replace the other Vertex with this
		void Reset(Vertex* vert);
	};

}