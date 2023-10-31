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

#include "Edge.h"
#include <vector>

namespace ctl {

//////////////////////////////////////////////////////////////////////
//!	Wrapper to a simple array of data of type T.
//////////////////////////////////////////////////////////////////////
	template <typename T>
	class Block
	{
	private:


	public:
	  Block(size_t num)
		{
		  size = num;
		  data = new T[size];
		}
		~Block(void) { delete [] data; }

		size_t	size;
		T*		data;
	};

//////////////////////////////////////////////////////////////////////
//!	Container that stores a list of consecutive Blocks. Also allows 
//!	access to compute the total size of all Blocks as well as access
//!	individual memebers.
//////////////////////////////////////////////////////////////////////
	template <typename T>
	class BlockList
	{
	private:
		std::vector<Block<T>*> blocks;
		size_t totalSize = 0;



	public:
		BlockList(void) { }
		~BlockList(void)
		{
			while (!blocks.empty())
				PopBlock();
		}

		void PushBlock(Block<T>* block)
		{
			if (block)
			{
				blocks.push_back(block); 
				totalSize += block->size;
			 }
		}

		void PopBlock(void)
		{
			if (blocks.size())
			{
			Block<T>* t = blocks.back();
				totalSize = totalSize - t->size;
			blocks.pop_back();
			delete t;
			 }
		}

	//	Get data at location i
		T* operator[](const unsigned int& i)
		{
			size_t current = 0;
			typedef typename std::vector<Block<T>*>::iterator block_iterator;
			for (block_iterator next = blocks.begin(); next != blocks.end(); ++next)
			{
				if ( i < (*next)->size + current )
					return &(*next)->data[ i - current ];
				else
					current += (*next)->size;
			}
			return NULL;
		}

		size_t size(void) const
		{
			return totalSize;
		}
	};

}
