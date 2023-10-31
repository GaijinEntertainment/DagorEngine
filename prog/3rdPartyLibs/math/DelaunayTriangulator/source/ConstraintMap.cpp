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
#include "ConstraintMap.h"
#include <algorithm>

//#pragma optimize("", off)

namespace ctl {

	ID ConstraintMap::GetNextConstraintID(void)
	{
		ID id = id_generator.getID();
        Constraint_To_Vertex.resize(std::max<ID>((ID)Constraint_To_Vertex.size(),id+1),-1);
		return id;
	}

	void ConstraintMap::FreeConstraintID(ID constraintID)
	{
		Constraint_To_Vertex[constraintID] = NULL;
		id_generator.freeID(constraintID);
	}

	void ConstraintMap::BindVertex(ID constraintID, Vertex* vert)
	{
		if (vert) 
			Constraint_To_Vertex[constraintID] = vert->getID();
	}

	ID ConstraintMap::GetBoundVertex(ID constraintID)
	{
		return Constraint_To_Vertex[constraintID];
	}

	void ConstraintMap::FreeVertex(ID constraintID)
	{
		Constraint_To_Vertex[constraintID] = -1;
	}

	bool ConstraintMap::IsVertexBound(Vertex* vert)
	{
		if (!vert) return false;
		for (unsigned int i = 0; i < Constraint_To_Vertex.size(); i++)
		{
			if (Constraint_To_Vertex[i] == vert->getID()) return true;
		}
		Edge* start = vert->getEdges();
		Edge* next = start;
		do {
			if (IsEdgeBound(next))
				return true;
			next = next->Onext();
		} while (next != start);
		return false;
	}

	void ConstraintMap::BindEdge(Edge* edge, ID constraintID)
	{
		if (!edge) return;
		
		if(edge->getID()>=Edge_To_Constraint.size())
			Edge_To_Constraint.resize(edge->getID()+1);
		IDList::iterator it = upper_bound(Edge_To_Constraint[edge->getID()].begin(),Edge_To_Constraint[edge->getID()].end(),constraintID);
		Edge_To_Constraint[edge->getID()].insert(it,constraintID);
	}

	void ConstraintMap::BindEdge(Edge* edge, IDList constraints)
	{
		if (edge)
		{
			if(edge->getID()>=Edge_To_Constraint.size())
				Edge_To_Constraint.resize(edge->getID()+1);
			Edge_To_Constraint[edge->getID()] = constraints;
		}
	}

	void ConstraintMap::FreeEdge(Edge* edge, ID constraintID)
	{
		if (!edge) return;
		if(edge->getID()>=Edge_To_Constraint.size())
			Edge_To_Constraint.resize(edge->getID()+1);
		IDList::iterator it = lower_bound(Edge_To_Constraint[edge->getID()].begin(),Edge_To_Constraint[edge->getID()].end(),constraintID);
		if ( (*it) == constraintID) Edge_To_Constraint[edge->getID()].erase(it);
	}

	void ConstraintMap::FreeEdge(Edge* edge)
	{
		if (edge)
		{
			if(edge->getID()>=Edge_To_Constraint.size())
				Edge_To_Constraint.resize(edge->getID()+1);
			Edge_To_Constraint[edge->getID()].clear();
		}
	}

	bool ConstraintMap::IsEdgeBound(Edge* edge)
	{
		if (edge)
		{
			if(edge->getID()>=Edge_To_Constraint.size())
				Edge_To_Constraint.resize(edge->getID()+1);
			return (Edge_To_Constraint[edge->getID()].size() > 0);
		}
		else return false;
	}

	bool ConstraintMap::IsEdgeBound(Edge* edge, ID constraintID)
	{
		if (edge)
		{
			if(edge->getID()>=Edge_To_Constraint.size())
				Edge_To_Constraint.resize(edge->getID()+1);
			return binary_search(Edge_To_Constraint[edge->getID()].begin(),Edge_To_Constraint[edge->getID()].end(),constraintID);
		}
		else return false;
	}

	IDList ConstraintMap::GetConstraints(Edge* edge)
	{
		if(edge->getID()>=Edge_To_Constraint.size())
			Edge_To_Constraint.resize(edge->getID()+1);
		return Edge_To_Constraint[edge->getID()];
	}

	int ConstraintMap::GetNumBoundEdges(Vertex* vert)
	{
		if (IsVertexBound(vert)) return 1;
		if (!vert) return 0;
		int count = 0;
		Edge* start = vert->getEdges();
		if (!start) return 0;
		Edge* next = start;
		do
		{
			if (IsEdgeBound(next)) count++;
			next = next->Onext();
		} while (next != start);
		return count;
	}

}