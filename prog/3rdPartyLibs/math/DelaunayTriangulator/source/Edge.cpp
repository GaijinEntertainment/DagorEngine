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
#include "Edge.h"
#include "Vertex.h"

namespace ctl {

	void Edge::Splice(Edge* a, Edge* b)
	{
		Edge* alpha = a->Onext()->Rot();
		Edge* beta	= b->Onext()->Rot();

		Edge* temp1 = b->Onext();
		Edge* temp2 = a->Onext();
		Edge* temp3 = beta->Onext();
		Edge* temp4 = alpha->Onext();

		a->next		= temp1;
		b->next		= temp2;
		alpha->next = temp3;
		beta->next	= temp4;
	}

	ID Edge::getID(void) const
	{
		return id;
	}

	void Edge::setOrg(Vertex* vert)
	{
		point = vert;
		if (vert != (Vertex*)0 && vert != (Vertex*)1) vert->addEdge(this);
	}

	void Edge::setDest(Vertex* vert)
	{
		Sym()->setOrg(vert);
	}

	void Edge::setEndPoints(Vertex* org, Vertex* dest)
	{
		setOrg(org);
		setDest(dest);
	}

}