/** @file MaxRectsBinPack.cpp
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the MAXRECTS data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#include <EASTL/algorithm.h>

#include <MaxRectsBinPack.h>

namespace rbp {

MaxRectsBinPack::MaxRectsBinPack()
:binWidth(0),
binHeight(0),
usedSurfaceArea(0)
{
}

MaxRectsBinPack::MaxRectsBinPack(int width, int height)
{
	Init(width, height);
}

void MaxRectsBinPack::Init(int width, int height)
{
	binWidth = width;
	binHeight = height;
    usedSurfaceArea = 0;

	Rect n;
	n.x = 0;
	n.y = 0;
	n.width = width;
	n.height = height;

	freeRectangles.clear();
	freeRectangles.push_back(n);
}

Rect MaxRectsBinPack::Insert(int width, int height, FreeRectChoiceHeuristic method, bool allowFlip)
{
	Rect newNode;
	// Unused in this function. We don't need to know the score after finding the position.
	int score1 = eastl::numeric_limits<int>::max();
	int score2 = eastl::numeric_limits<int>::max();
	switch(method)
	{
		case RectBestShortSideFit: newNode = FindPositionForNewNodeBestShortSideFit(width, height, score1, score2, allowFlip); break;
		case RectBottomLeftRule: newNode = FindPositionForNewNodeBottomLeft(width, height, score1, score2, allowFlip); break;
		case RectBestLongSideFit: newNode = FindPositionForNewNodeBestLongSideFit(width, height, score2, score1, allowFlip); break;
		case RectBestAreaFit: newNode = FindPositionForNewNodeBestAreaFit(width, height, score1, score2, allowFlip); break;
	}
		
	if (newNode.height == 0)
		return newNode;

	size_t numRectanglesToProcess = freeRectangles.size();
	for(unsigned i = 0; i < numRectanglesToProcess; ++i)
	{
		if (SplitFreeNode(freeRectangles[i], newNode))
		{
			freeRectangles.erase(freeRectangles.begin() + i);
			--i;
			--numRectanglesToProcess;
		}
	}

	PruneFreeList();
    usedSurfaceArea += newNode.width*newNode.height;
	return newNode;
}

void MaxRectsBinPack::Insert(eastl::vector<RectSizeRequest> &rects, eastl::vector<RectResult> &dst, FreeRectChoiceHeuristic method, bool allowFlip)
{
	dst.clear();

	while(rects.size() > 0)
	{
		int bestScore1 = eastl::numeric_limits<int>::max();
		int bestScore2 = eastl::numeric_limits<int>::max();
		int bestRectIndex = -1;
		Rect bestNode;

		for(unsigned i = 0; i < rects.size(); ++i)
		{
			int score1;
			int score2;
			Rect newNode = ScoreRect(rects[i].width, rects[i].height, method, score1, score2, allowFlip);

			if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2))
			{
				bestScore1 = score1;
				bestScore2 = score2;
				bestNode = newNode;
				bestRectIndex = i;
			}
		}

		if (bestRectIndex == -1)
			return;

		PlaceRect(bestNode);
        dst.push_back(RectResult(bestNode, rects[bestRectIndex].id));
		rects.erase(rects.begin() + bestRectIndex);
	}
}

void MaxRectsBinPack::PlaceRect(const Rect &node)
{
	size_t numRectanglesToProcess = freeRectangles.size();
	for(unsigned i = 0; i < numRectanglesToProcess; ++i)
	{
		if (SplitFreeNode(freeRectangles[i], node))
		{
			freeRectangles.erase(freeRectangles.begin() + i);
			--i;
			--numRectanglesToProcess;
		}
	}

	PruneFreeList();

    usedSurfaceArea += node.width*node.height;
}

Rect MaxRectsBinPack::ScoreRect(int width, int height, FreeRectChoiceHeuristic method, int &score1, int &score2, bool allowFlip) const
{
	Rect newNode;
	score1 = eastl::numeric_limits<int>::max();
	score2 = eastl::numeric_limits<int>::max();
	switch(method)
	{
	case RectBestShortSideFit: newNode = FindPositionForNewNodeBestShortSideFit(width, height, score1, score2, allowFlip); break;
	case RectBottomLeftRule: newNode = FindPositionForNewNodeBottomLeft(width, height, score1, score2, allowFlip); break;
	case RectBestLongSideFit: newNode = FindPositionForNewNodeBestLongSideFit(width, height, score2, score1, allowFlip); break;
	case RectBestAreaFit: newNode = FindPositionForNewNodeBestAreaFit(width, height, score1, score2, allowFlip); break;
	}

	// Cannot fit the current rectangle.
	if (newNode.height == 0)
	{
		score1 = eastl::numeric_limits<int>::max();
		score2 = eastl::numeric_limits<int>::max();
	}

	return newNode;
}

/// Computes the ratio of used surface area.
float MaxRectsBinPack::Occupancy() const
{
	return (float)usedSurfaceArea / (binWidth * binHeight);
}

Rect MaxRectsBinPack::FindPositionForNewNodeBottomLeft(int width, int height, int &bestY, int &bestX, bool allowFlip) const
{
	Rect bestNode;
	memset(&bestNode, 0, sizeof(Rect));

	bestY = eastl::numeric_limits<int>::max();
	bestX = eastl::numeric_limits<int>::max();

	for(unsigned i = 0; i < freeRectangles.size(); ++i)
	{
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
		{
			int topSideY = freeRectangles[i].y + height;
			if (topSideY < bestY || (topSideY == bestY && freeRectangles[i].x < bestX))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestY = topSideY;
				bestX = freeRectangles[i].x;
			}
		}
#if !DEFINED_NO_FLIPPING
		if (allowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
		{
			int topSideY = freeRectangles[i].y + width;
			if (topSideY < bestY || (topSideY == bestY && freeRectangles[i].x < bestX))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestY = topSideY;
				bestX = freeRectangles[i].x;
			}
		}
#endif
	}
	return bestNode;
}

Rect MaxRectsBinPack::FindPositionForNewNodeBestShortSideFit(int width, int height, 
	int &bestShortSideFit, int &bestLongSideFit, bool allowFlip) const
{
	Rect bestNode;
	memset(&bestNode, 0, sizeof(Rect));

	bestShortSideFit = eastl::numeric_limits<int>::max();
	bestLongSideFit = eastl::numeric_limits<int>::max();

	for(unsigned i = 0; i < freeRectangles.size(); ++i)
	{
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
		{
			int leftoverHoriz = abs(freeRectangles[i].width - width);
			int leftoverVert = abs(freeRectangles[i].height - height);
			int shortSideFit = eastl::min(leftoverHoriz, leftoverVert);
			int longSideFit = eastl::max(leftoverHoriz, leftoverVert);

			if (shortSideFit < bestShortSideFit || (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}

#if !DEFINED_NO_FLIPPING
		if (allowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
		{
			int flippedLeftoverHoriz = abs(freeRectangles[i].width - height);
			int flippedLeftoverVert = abs(freeRectangles[i].height - width);
			int flippedShortSideFit = eastl::min(flippedLeftoverHoriz, flippedLeftoverVert);
			int flippedLongSideFit = eastl::max(flippedLeftoverHoriz, flippedLeftoverVert);

			if (flippedShortSideFit < bestShortSideFit || (flippedShortSideFit == bestShortSideFit && flippedLongSideFit < bestLongSideFit))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestShortSideFit = flippedShortSideFit;
				bestLongSideFit = flippedLongSideFit;
			}
		}
#endif
	}
	return bestNode;
}

Rect MaxRectsBinPack::FindPositionForNewNodeBestLongSideFit(int width, int height, 
	int &bestShortSideFit, int &bestLongSideFit, bool allowFlip) const
{
	Rect bestNode;
	memset(&bestNode, 0, sizeof(Rect));

	bestShortSideFit = eastl::numeric_limits<int>::max();
	bestLongSideFit = eastl::numeric_limits<int>::max();

	for(unsigned i = 0; i < freeRectangles.size(); ++i)
	{
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
		{
			int leftoverHoriz = abs(freeRectangles[i].width - width);
			int leftoverVert = abs(freeRectangles[i].height - height);
			int shortSideFit = eastl::min(leftoverHoriz, leftoverVert);
			int longSideFit = eastl::max(leftoverHoriz, leftoverVert);

			if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}

#if !DEFINED_NO_FLIPPING
		if (allowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
		{
			int leftoverHoriz = abs(freeRectangles[i].width - height);
			int leftoverVert = abs(freeRectangles[i].height - width);
			int shortSideFit = eastl::min(leftoverHoriz, leftoverVert);
			int longSideFit = eastl::max(leftoverHoriz, leftoverVert);

			if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestShortSideFit = shortSideFit;
				bestLongSideFit = longSideFit;
			}
		}
#endif
	}
	return bestNode;
}

Rect MaxRectsBinPack::FindPositionForNewNodeBestAreaFit(int width, int height, 
	int &bestAreaFit, int &bestShortSideFit, bool allowFlip) const
{
	Rect bestNode;
	memset(&bestNode, 0, sizeof(Rect));

	bestAreaFit = eastl::numeric_limits<int>::max();
	bestShortSideFit = eastl::numeric_limits<int>::max();

	for(unsigned i = 0; i < freeRectangles.size(); ++i)
	{
		int areaFit = freeRectangles[i].width * freeRectangles[i].height - width * height;

		// Try to place the rectangle in upright (non-flipped) orientation.
		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
		{
			int leftoverHoriz = abs(freeRectangles[i].width - width);
			int leftoverVert = abs(freeRectangles[i].height - height);
			int shortSideFit = eastl::min(leftoverHoriz, leftoverVert);

			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestShortSideFit = shortSideFit;
				bestAreaFit = areaFit;
			}
		}

#if !DEFINED_NO_FLIPPING
		if (allowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
		{
			int leftoverHoriz = abs(freeRectangles[i].width - height);
			int leftoverVert = abs(freeRectangles[i].height - width);
			int shortSideFit = eastl::min(leftoverHoriz, leftoverVert);

			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestShortSideFit = shortSideFit;
				bestAreaFit = areaFit;
			}
		}
#endif
	}
	return bestNode;
}

/// Returns 0 if the two intervals i1 and i2 are disjoint, or the length of their overlap otherwise.
static inline int CommonIntervalLength(int i1start, int i1end, int i2start, int i2end)
{
	if (i1end < i2start || i2end < i1start)
		return 0;
	return eastl::min(i1end, i2end) - eastl::max(i1start, i2start);
}

bool MaxRectsBinPack::SplitFreeNode(Rect freeNode, const Rect &usedNode)
{
	// Test with SAT if the rectangles even intersect.
	if (usedNode.x >= freeNode.x + freeNode.width || usedNode.x + usedNode.width <= freeNode.x ||
		usedNode.y >= freeNode.y + freeNode.height || usedNode.y + usedNode.height <= freeNode.y)
		return false;

	if (usedNode.x < freeNode.x + freeNode.width && usedNode.x + usedNode.width > freeNode.x)
	{
		// New node at the top side of the used node.
		if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.height)
		{
			Rect newNode = freeNode;
			newNode.height = usedNode.y - newNode.y;
			freeRectangles.push_back(newNode);
		}

		// New node at the bottom side of the used node.
		if (usedNode.y + usedNode.height < freeNode.y + freeNode.height)
		{
			Rect newNode = freeNode;
			newNode.y = usedNode.y + usedNode.height;
			newNode.height = freeNode.y + freeNode.height - (usedNode.y + usedNode.height);
			freeRectangles.push_back(newNode);
		}
	}

	if (usedNode.y < freeNode.y + freeNode.height && usedNode.y + usedNode.height > freeNode.y)
	{
		// New node at the left side of the used node.
		if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.width)
		{
			Rect newNode = freeNode;
			newNode.width = usedNode.x - newNode.x;
			freeRectangles.push_back(newNode);
		}

		// New node at the right side of the used node.
		if (usedNode.x + usedNode.width < freeNode.x + freeNode.width)
		{
			Rect newNode = freeNode;
			newNode.x = usedNode.x + usedNode.width;
			newNode.width = freeNode.x + freeNode.width - (usedNode.x + usedNode.width);
			freeRectangles.push_back(newNode);
		}
	}

	return true;
}

void MaxRectsBinPack::PruneFreeList()
{
	/* 
	///  Would be nice to do something like this, to avoid a Theta(n^2) loop through each pair.
	///  But unfortunately it doesn't quite cut it, since we also want to detect containment. 
	///  Perhaps there's another way to do this faster than Theta(n^2).

	if (freeRectangles.size() > 0)
		clb::sort::QuickSort(&freeRectangles[0], freeRectangles.size(), NodeSortCmp);

	for(unsigned i = 0; i < freeRectangles.size()-1; ++i)
		if (freeRectangles[i].x == freeRectangles[i+1].x &&
		    freeRectangles[i].y == freeRectangles[i+1].y &&
		    freeRectangles[i].width == freeRectangles[i+1].width &&
		    freeRectangles[i].height == freeRectangles[i+1].height)
		{
			freeRectangles.erase(freeRectangles.begin() + i);
			--i;
		}
	*/

	/// Go through each pair and remove any rectangle that is redundant.
	for(unsigned i = 0; i < freeRectangles.size(); ++i)
		for(unsigned j = i+1; j < freeRectangles.size(); ++j)
		{
			if (IsContainedIn(freeRectangles[i], freeRectangles[j]))
			{
				freeRectangles.erase(freeRectangles.begin()+i);
				--i;
				break;
			}
			if (IsContainedIn(freeRectangles[j], freeRectangles[i]))
			{
				freeRectangles.erase(freeRectangles.begin()+j);
				--j;
			}
		}
}

}
