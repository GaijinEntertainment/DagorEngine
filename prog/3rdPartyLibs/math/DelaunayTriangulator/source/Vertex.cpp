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
#include "Vertex.h"

namespace ctl {

	ID Vertex::getID(void)
	{
		return id;
	}

	void Vertex::addEdge(Edge* edge)
	{
		this->edge = edge;
	}

	void Vertex::removeEdge(Edge* edge)
	{
		Edge* next = edge->Onext();
		this->edge = (next != edge) ? next : NULL;
	}

	Edge* Vertex::getEdges(void)
	{
		return edge;
	}

	void Vertex::clearEdge(void)
	{
		edge = NULL;
	}

	Vector Vertex::getAreaWeightedNormal(void)
	{
		Vector up(0,0,1);
		if (!edge) return up;
		if (edge->Onext() == edge) return up;
		
		Vector normal;

		Edge* start = edge;
		Edge* next = start;

		do
		{
			if (!next->Dest() || !next->Onext()->Dest()) break;

			Vector u = next->Dest()->point - point;
			Vector v = next->Onext()->Dest()->point - point;

			Vector nextNorm = u.cross(v);
			if (nextNorm.dot(up) > 0)
				normal += nextNorm;

			next = next->Onext();
		}
		while (next != start);

		return normal;
	}

	void Vertex::Reset(Vertex* vert)
	{
		id = vert->id;
		point = vert->point;
		edge = vert->edge;

	//	Replace all links to old vert with this new one
		if (edge)
		{
			Edge* start = edge;
			Edge* next = edge;
			do
			{
				next->setOrg(this);
				next = next->Onext();
			} while( next != start );
		}
	}

}
