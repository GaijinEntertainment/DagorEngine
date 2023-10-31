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

#include "ID.h"
#include "Vertex.h"
#include "Edge.h"

namespace ctl {

/*!	\class ctl::ConstraintMap ctl/ConstraintMap.h ctl/ConstraintMap.h
	\brief CosntraintMap

	Provides an interface for binding Edges to Constraints. A Constraint is not an actual class or object, only an ID value
	that represents some constraining factor in a DelaunayTriangulation.

	Vertex Binding -

	A Vertex can be bound\

	There are two forms of binding possible to a Vertex. Active and Passive.
	
	Active binding is when a Vertex is bonded directly to a Constraint. Each Constraint can only have ONE actively bonded Vertex.
	Active binding is used to bind a series of Constrained Edges together by identifying at least one of the Vertices of those connected
	Edges (usually the first Vertex of the Constraint). It can also be used by active bind a single Vertex to a single Constraint in the
	case of inserting Constrained Points.

	Passive binding is when a Vertex is bonded indirectly to a Constraint through an Edge. This is equivalent to an Edge being 
	bound to a Constraint and having a Vertex as its origin or destination. Passive binding is a result of tracking Edge binding.


	Edge Binding -

	Only Active Edge binding is supported. There is no way for an Edge to be "Passively" bound. Each Edge can be bound to any number of
	Constraints and all Constraints binding each Edge are tracked (unlike Vertex binding tracking).
*/
	class ConstraintMap
	{
	private:
		IDGenerator			id_generator;				/*	ID Generator in charge of assigning ID values to new Constraints.		*/

		IDList				Constraint_To_Vertex;
		std::vector<IDList>		Edge_To_Constraint;
	public:
		ConstraintMap(void) { }
		~ConstraintMap(void) { }

/******************************************************************************************************
	CONSTRAINT ID ASSIGNMENT
*******************************************************************************************************/

//!	\return Returns a newly reserved ID (unique amost this ConstraintMap).
		ID		GetNextConstraintID(void);	

//!	\brief Free an ID to be used by another constraint.
		void	FreeConstraintID(ID constraintID);

/******************************************************************************************************
	VERTEX BINDING
*******************************************************************************************************/

//!	\brief Actively Bind a Vertex to a constraint.
		void	BindVertex(ID constraintID, Vertex* vert);

//!	\return Returns the ID of the Vertex bound to the given constraint.
		ID		GetBoundVertex(ID constraintID);

//!	\brief Frees the Vertex bound by the constraint.
		void	FreeVertex(ID constraintID);

//!	\return Returns TRUE if the Vertex is bound to at least one constraint.
		bool	IsVertexBound(Vertex* vert);

/******************************************************************************************************
	EDGE BINDING
*******************************************************************************************************/

//!	\brief Bind an Edge to a constraint.
		void	BindEdge(Edge* edge, ID constraintID);

//!	\brief Set the constraint list of an Edge.
		void	BindEdge(Edge* edge, IDList constraintIDs);

//!	\brief Frees the Edge bound by the cosntraint.
		void	FreeEdge(Edge* edge, ID constraintID);

//!	\brief Frees the Edge from all constraint binding.
		void	FreeEdge(Edge* edge);

//!	\return Returns TRUE if Edge is bound by at least one constraint.
		bool	IsEdgeBound(Edge* edge);

//!	\return Returns TRUE if Edge is bound by the constraint.
		bool	IsEdgeBound(Edge* edge, ID constraintID);

//!	\return Returns the constraints binding the given Edge.
		IDList	GetConstraints(Edge* edge);

//!	\return Returns the number of bound Edges about the Vertex.
		int		GetNumBoundEdges(Vertex* vert);

	};

}