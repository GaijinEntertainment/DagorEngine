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
/*!	\file ctl/Subdivision.h
\headerfile ctl/Subdivision.h
\brief Provides ctl::Subdivision.
\author Joshua Anghel
*/
#pragma once

#include "Edge.h"
#include "Vertex.h"
#include "BlockList.h"

namespace ctl {

/*!	\class ctl::Subdivision ctl/Subdivision.h ctl/Subdivision.h
	\brief Subdivision
 
	Provides a class for storing Edges and Vertex points assossiated with a quad-edge system. 
	Assigns unique IDs to all Vertices and Edges  and stores them in lists using they ID values as index values into the lists. 
	Automatically cleans up and used memory by all Vertices and Edges owned by it upon destruction.
	Stores Edges and Vertices as blocks of data. When an Edge or Vertex is "deleted" it it instead flagged as inactive
	and that memory will be used to construct future Edges/Vertices. This avoids constant calls to new/delete and avoids
	fragmentation.

	Based on Guibus and Stolfi's quad-edge stucture and the code presented in Graphics Gems IV (Paul S. Heckbert).
	\sa Guibus and Stolfi (1985 p.96)
	\sa Graphics Gems IV (Paul S. Heckbert pg. 51)
 */
	class Subdivision
	{
	private:
		typedef BlockList<Edge>		EdgeList;
		typedef BlockList<Vertex>	VertexList;

		size_t				resizeIncriment;		//!< amount to resize a BlockList by on growth

		IDGenerator			vertIDGen;				//!< IDGenerator tracking ID's of active vertices
		std::vector<char>	vertFlags;				//!< flags indicating which vertices are active
		VertexList			verts;					//!< container to store all vertices (active and non-active)

		IDGenerator			edgeIDGen;				//!< IDGenerator tracking ID's of active edges
		std::vector<char>	edgeFlags;				//!< flags indicating which edges are active
		EdgeList			edges;					//!< container to store all edges (active and non-active)

		void allocVerts(void);
		void allocEdges(void);

	public:
		Subdivision(int resizeIncriment = 100);
		~Subdivision(void) { }

/******************************************************************************************************
	VERTICES
*******************************************************************************************************/

//!	\return Returns the maximum number of Vertices stored by this Subdivision.
		int getMaxVerts(void) const;

//!	\return Returns the number of active Vertices in this Subdivision.
		int getNumVerts(void) const;

/*!	\brief Return the Vertex with ID id in this Subdivision.
	\param id ID value of Vertex to retrieve.
	\return Vertex with ID value of id, NULL if that Vertex doesn't exist
*/
		Vertex* getVertex(ID id);

//!	\return Returns a random Vertex in this Subdivision.
		Vertex* getRandomVertex(void);

/*!	\brief Create a Vertex within this Subdivision.
	\param point Point to create Vertex at.
	\return Pointer to new Vertex inside this Subdivision.
*/
		Vertex*	CreateVertex(Point point = Point());

/*!	\brief Free the memory used by a Vertex in this Subdivision.
	\param vert Vertex to free.
*/
		void RemoveVertex(Vertex* vert);

/******************************************************************************************************
	EDGES
*******************************************************************************************************/

//!	\return Returns the maxmimum number of Edges stored by this Subdivision.
		int getMaxEdges(void) const;

//!	\return Returns the number of active Edges in this Subdivision.
		int getNumEdges(void) const;

/*!	\brief Return the Edge with ID id in this Subdivision.
	\param id ID value of Edge to retrieve.
	\return Edge with ID value of id, NULL if that Edge doesn't exist.
*/
		Edge* getEdge(ID id);

//!	\return A random Edge within this Subdivision.
		Edge* getRandomEdge(void);

/*!	\brief Create a Edge in this Subdivision.
	\param a Origin of new Edge.
	\param b Destination of new Edge.
	\return New Edge pointer.
*/
		Edge* CreateEdge(Vertex* a = NULL, Vertex* b = NULL);

/*!	\brief Free the memory used by an Edge in this Subdivision.
	\param edge Edge to free.
*/
		void RemoveEdge(Edge* edge);

/*!	\brief Connects the destination of a to the origin of b.
	Add a new Edge connecting the destination of a to the origin of b, in such a way that all three
	have the same left face after the connection is complete.
	\param a Destination of which will be the origin of the new Edge.
	\param b Origin of which will be the destination of the new Edge.
	\return A new Edge from the desination of a to the origin of b, NULL if not enough memory.
*/
		Edge* Connect(Edge* a, Edge* b);

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
		void Splice(Edge* a, Edge* b);
	};

}