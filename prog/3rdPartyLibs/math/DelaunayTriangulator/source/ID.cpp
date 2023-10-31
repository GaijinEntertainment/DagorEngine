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
#include "ID.h"

namespace ctl {

	IDGenerator::IDGenerator(void)
	{
		AvailableIDs.push_back(0);
	}

	ID IDGenerator::getID(void)
	{
		if (AvailableIDs.size() > 1)
		{
			ID id = AvailableIDs.back();
			AvailableIDs.pop_back();
			return id;
		}
		else
			return (AvailableIDs.front()++);
	}

	void IDGenerator::freeID(ID id)
	{
		AvailableIDs.push_back(id);
	}

	bool IDGenerator::isIDUsed(ID id) const
	{
		for (int i = int(AvailableIDs.size())-1; i > 0; i--)
		{
			if (id == AvailableIDs[i]) return false;
		}
		if (id < AvailableIDs[0]) return true;
		else return false;
	}

	unsigned int IDGenerator::numUsedIDs(void) const
	{
	  return AvailableIDs.front() - ((unsigned int)AvailableIDs.size()) + 1;
	}

	unsigned int IDGenerator::numFreeIDs(void) const
	{
	  return (unsigned int)(AvailableIDs.size()) - 1;
	}

	unsigned int IDGenerator::peekNextID(void) const
	{
		return AvailableIDs[0];
	}

}
