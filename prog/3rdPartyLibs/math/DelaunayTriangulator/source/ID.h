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
/*! \brief Provides ctl::IDGenerator
\author Joshua Anghel
*/
#pragma once

#include <vector>

namespace ctl {

	typedef unsigned int	ID;
	typedef std::vector<ID> IDList;

/*! \brief IDGenerator

Provides a structure to maintain the allocation of unique int ID values to ensure keys are
always unique and to ensure the lowest ID values possible are used, so they can also double as index 
values into an array if needed.

The first ID value recieved when calling getID() for the first time after initialization is 0.
When feeID() is called, it places the newly available ID at the top of the stack and will be returned
on the next call to getID().
*/
	class IDGenerator
	{
	private:
		IDList AvailableIDs;

	public:
		IDGenerator(void);
		~IDGenerator(void) { }

/*!	\brief Return a unique ID from this IDGenerator.
 *	\return Unique ID from this IDGenerator.
 */
		ID getID(void);

/*!	\brief Free an ID to be used by this IDGenerator again.
 *	\param id ID to release for reassignment.
 */
		void freeID(ID id);

		bool isIDUsed(ID id) const;

/*!	\brief Get the number of IDs currently in use.
 *	\return The number of IDs currently in use.
 */
		unsigned int numUsedIDs(void) const;

/*!	\brief Get the number of IDs currently free to the IDGenerator.
 *
 *	Does not count the next (highest available) 
 *	\return The number of IDs that have been manually freed to the IDGenerator.
 */
		unsigned int numFreeIDs(void) const;

		// get the next id (highest available, not a replacement)
		unsigned int peekNextID(void) const;
	};

}