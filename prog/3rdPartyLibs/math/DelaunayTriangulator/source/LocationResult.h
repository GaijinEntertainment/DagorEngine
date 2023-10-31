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
/*!	\file ctl/LocationResult.h
\headerfile ctl/LocationResult.h
\brief Provides ctl::LocationResult.
\author Joshua Anghel
*/
#pragma once

#include "Edge.h"

namespace ctl {

	enum LocationResultType
	{
		LR_UNKNOWN			=  -1,
		LR_VERTEX			=	0,
		LR_EDGE				=	1,
		LR_FACE				=	2
	};

/*!	\class ctl::LocationResult ctl/LocationResult.h ctl/LocationResult.h
\brief LocationResult

A LocationResult provides information on the location of a Point in a QuadEdge and is used to quickly pass this 
information between function calls in various algorithms.
*/
	class LocationResult 
	{
	protected:
		Edge*				edge;
		LocationResultType	type;

	public:
		LocationResult(Edge* e, LocationResultType t) : edge(e), type(t) { }
		LocationResult(void);
		LocationResult(const LocationResult &rhs);
		LocationResult &operator=(const LocationResult &rhs);
		~LocationResult(void) { }

/*!	\brief Return the LocationResultType of this LocationResult.
\return LocationResultType of this LocationResult.
*/
		LocationResultType getType(void);

/*!	\brief Return the Edge representing the location of this LocationResult.

If the LocationResultType is LR_VERTEX, the location is the origin of the Edge.
If the LocationResultType is LR_EDGE, the Location is the Edge.
If the LocationResultType is LR_FACE, the Location is the face to the left of the Edge.
\return Edge representing the Location of this LocationResult.
*/
		Edge* getEdge(void);
	};

}