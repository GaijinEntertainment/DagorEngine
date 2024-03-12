/** @file SkylineBinPack.cpp
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the SKYLINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#include <EASTL/algorithm.h>

#include <SkylineBinPack.h>

namespace rbp {

SkylineBinPack::SkylineBinPack()
:binWidth(0),
binHeight(0),
useWasteMap(false)
{
}

SkylineBinPack::SkylineBinPack(int width, int height, bool useWasteMap)
{
	Init(width, height, useWasteMap);
}

void SkylineBinPack::Init(int width, int height, bool useWasteMap_)
{
	binWidth = width;
	binHeight = height;

	useWasteMap = useWasteMap_;

#ifdef _DEBUG
	disjointRects.Clear();
#endif

	usedSurfaceArea = 0;
	skyLine.clear();
	SkylineNode node;
	node.x = 0;
	node.y = 0;
	node.width = binWidth;
	skyLine.push_back(node);

	if (useWasteMap)
	{
		wasteMap.Init(width, height);
		wasteMap.GetFreeRectangles().clear();
	}
}

void SkylineBinPack::Insert(eastl::vector<RectSizeRequest> &rects, eastl::vector<RectResult> &dst, LevelChoiceHeuristic method, bool allowFlip)
{
	dst.clear();

	while(rects.size() > 0)
	{
		Rect bestNode;
		int bestScore1 = eastl::numeric_limits<int>::max();
		int bestScore2 = eastl::numeric_limits<int>::max();
		int bestSkylineIndex = -1;
		int bestRectIndex = -1;
		for(unsigned i = 0; i < rects.size(); ++i)
		{
			Rect newNode;
			int score1;
			int score2;
			int index;
			switch(method)
			{
			case LevelBottomLeft:
				newNode = FindPositionForNewNodeBottomLeft(rects[i].width, rects[i].height, score1, score2, index, allowFlip);
				debug_assert(disjointRects.Disjoint(newNode));
				break;
			case LevelMinWasteFit:
				newNode = FindPositionForNewNodeMinWaste(rects[i].width, rects[i].height, score2, score1, index, allowFlip);
				debug_assert(disjointRects.Disjoint(newNode));
				break;
			default: debug_assert(false); break;
			}
			if (newNode.height != 0)
			{
				if (score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2))
				{
					bestNode = newNode;
					bestScore1 = score1;
					bestScore2 = score2;
					bestSkylineIndex = index;
					bestRectIndex = i;
				}
			}
		}

		if (bestRectIndex == -1)
			return;

		// Perform the actual packing.
#ifdef _DEBUG
		debug_assert(disjointRects.Disjoint(bestNode));
		disjointRects.Add(bestNode);
#endif
		AddSkylineLevel(bestSkylineIndex, bestNode, allowFlip);
		usedSurfaceArea += rects[bestRectIndex].width * rects[bestRectIndex].height;
    	dst.push_back(RectResult(bestNode, rects[bestRectIndex].id));
		rects.erase(rects.begin() + bestRectIndex);
	}
}

Rect SkylineBinPack::Insert(int width, int height, LevelChoiceHeuristic method, bool allowFlip)
{
	// First try to pack this rectangle into the waste map, if it fits.
	Rect node = wasteMap.Insert(width, height, true, GuillotineBinPack::RectBestShortSideFit,GuillotineBinPack::SplitMinimizeArea, allowFlip);
	debug_assert(disjointRects.Disjoint(node));

	if (node.height != 0)
	{
		Rect newNode;
		newNode.x = node.x;
		newNode.y = node.y;
		newNode.width = node.width;
		newNode.height = node.height;
		usedSurfaceArea += width * height;
#ifdef _DEBUG
		debug_assert(disjointRects.Disjoint(newNode));
		disjointRects.Add(newNode);
#endif
		return newNode;
	}
	
	switch(method)
	{
	case LevelBottomLeft: return InsertBottomLeft(width, height, allowFlip);
	case LevelMinWasteFit: return InsertMinWaste(width, height, allowFlip);
	default: debug_assert(false); return node;
	}
}

bool SkylineBinPack::RectangleFits(int skylineNodeIndex, int width, int height, int &y) const
{
	int x = skyLine[skylineNodeIndex].x;
	if (x + width > binWidth)
		return false;
	int widthLeft = width;
	int i = skylineNodeIndex;
	y = skyLine[skylineNodeIndex].y;
	while(widthLeft > 0)
	{
		y = eastl::max(y, skyLine[i].y);
		if (y + height > binHeight)
			return false;
		widthLeft -= skyLine[i].width;
		++i;
		debug_assert(i < (int)skyLine.size() || widthLeft <= 0);
	}
	return true;
}

int SkylineBinPack::ComputeWastedArea(int skylineNodeIndex, int width, int height, int y) const
{
	int wastedArea = 0;
	const int rectLeft = skyLine[skylineNodeIndex].x;
	const int rectRight = rectLeft + width;
	for(; skylineNodeIndex < (int)skyLine.size() && skyLine[skylineNodeIndex].x < rectRight; ++skylineNodeIndex)
	{
		if (skyLine[skylineNodeIndex].x >= rectRight || skyLine[skylineNodeIndex].x + skyLine[skylineNodeIndex].width <= rectLeft)
			break;

		int leftSide = skyLine[skylineNodeIndex].x;
		int rightSide = eastl::min(rectRight, leftSide + skyLine[skylineNodeIndex].width);
		debug_assert(y >= skyLine[skylineNodeIndex].y);
		wastedArea += (rightSide - leftSide) * (y - skyLine[skylineNodeIndex].y);
	}
	return wastedArea;
}

bool SkylineBinPack::RectangleFits(int skylineNodeIndex, int width, int height, int &y, int &wastedArea) const
{
	bool fits = RectangleFits(skylineNodeIndex, width, height, y);
	if (fits)
		wastedArea = ComputeWastedArea(skylineNodeIndex, width, height, y);
	
	return fits;
}

void SkylineBinPack::AddWasteMapArea(int skylineNodeIndex, int width, int height, int y)
{
	// int wastedArea = 0; // unused
	const int rectLeft = skyLine[skylineNodeIndex].x;
	const int rectRight = rectLeft + width;
	for(; skylineNodeIndex < (int)skyLine.size() && skyLine[skylineNodeIndex].x < rectRight; ++skylineNodeIndex)
	{
		if (skyLine[skylineNodeIndex].x >= rectRight || skyLine[skylineNodeIndex].x + skyLine[skylineNodeIndex].width <= rectLeft)
			break;

		int leftSide = skyLine[skylineNodeIndex].x;
		int rightSide = eastl::min(rectRight, leftSide + skyLine[skylineNodeIndex].width);
		debug_assert(y >= skyLine[skylineNodeIndex].y);

		Rect waste;
		waste.x = leftSide;
		waste.y = skyLine[skylineNodeIndex].y;
		waste.width = rightSide - leftSide;
		waste.height = y - skyLine[skylineNodeIndex].y;

		debug_assert(disjointRects.Disjoint(waste));
		wasteMap.GetFreeRectangles().push_back(waste);
	}
}

void SkylineBinPack::AddSkylineLevel(int skylineNodeIndex, const Rect &rect, bool allowFlip)
{
	// First track all wasted areas and mark them into the waste map if we're using one.
	if (useWasteMap)
		AddWasteMapArea(skylineNodeIndex, rect.width, rect.height, rect.y);

	SkylineNode newNode;
	newNode.x = rect.x;
	newNode.y = rect.y + rect.height;
	newNode.width = rect.width;
	skyLine.insert(skyLine.begin() + skylineNodeIndex, newNode);

	debug_assert(newNode.x + newNode.width <= binWidth);
	debug_assert(newNode.y <= binHeight);

	for(unsigned i = skylineNodeIndex+1; i < skyLine.size(); ++i)
	{
		debug_assert(skyLine[i-1].x <= skyLine[i].x);

		if (skyLine[i].x < skyLine[i-1].x + skyLine[i-1].width)
		{
			int shrink = skyLine[i-1].x + skyLine[i-1].width - skyLine[i].x;

			skyLine[i].x += shrink;
			skyLine[i].width -= shrink;

			if (skyLine[i].width <= 0)
			{
				skyLine.erase(skyLine.begin() + i);
				--i;
			}
			else
				break;
		}
		else
			break;
	}
	MergeSkylines();
}

void SkylineBinPack::MergeSkylines()
{
	for(unsigned i = 0; i < skyLine.size()-1; ++i)
		if (skyLine[i].y == skyLine[i+1].y)
		{
			skyLine[i].width += skyLine[i+1].width;
			skyLine.erase(skyLine.begin() + (i+1));
			--i;
		}
}

Rect SkylineBinPack::InsertBottomLeft(int width, int height, bool allowFlip)
{
	int bestHeight;
	int bestWidth;
	int bestIndex;
	Rect newNode = FindPositionForNewNodeBottomLeft(width, height, bestHeight, bestWidth, bestIndex, allowFlip);

	if (bestIndex != -1)
	{
		debug_assert(disjointRects.Disjoint(newNode));

		// Perform the actual packing.
		AddSkylineLevel(bestIndex, newNode, allowFlip);

		usedSurfaceArea += width * height;
#ifdef _DEBUG
		disjointRects.Add(newNode);
#endif
	}
	else
		memset(&newNode, 0, sizeof(Rect));

	return newNode;
}

Rect SkylineBinPack::FindPositionForNewNodeBottomLeft(int width, int height, int &bestHeight, int &bestWidth, int &bestIndex, bool allowFlip) const
{
	bestHeight = eastl::numeric_limits<int>::max();
	bestIndex = -1;
	// Used to break ties if there are nodes at the same level. Then pick the narrowest one.
	bestWidth = eastl::numeric_limits<int>::max();
	Rect newNode;
	memset(&newNode, 0, sizeof(newNode));
	for(unsigned i = 0; i < skyLine.size(); ++i)
	{
		int y;
		if (RectangleFits(i, width, height, y))
		{
			if (y + height < bestHeight || (y + height == bestHeight && skyLine[i].width < bestWidth))
			{
				bestHeight = y + height;
				bestIndex = i;
				bestWidth = skyLine[i].width;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = width;
				newNode.height = height;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
#if !DEFINED_NO_FLIPPING
		if (allowFlip && RectangleFits(i, height, width, y))
		{
			if (y + width < bestHeight || (y + width == bestHeight && skyLine[i].width < bestWidth))
			{
				bestHeight = y + width;
				bestIndex = i;
				bestWidth = skyLine[i].width;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = height;
				newNode.height = width;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
#endif
	}

	return newNode;
}

Rect SkylineBinPack::InsertMinWaste(int width, int height, bool allowFlip)
{
	int bestHeight;
	int bestWastedArea;
	int bestIndex;
	Rect newNode = FindPositionForNewNodeMinWaste(width, height, bestHeight, bestWastedArea, bestIndex, allowFlip);

	if (bestIndex != -1)
	{
		debug_assert(disjointRects.Disjoint(newNode));

		// Perform the actual packing.
		AddSkylineLevel(bestIndex, newNode, allowFlip);

		usedSurfaceArea += width * height;
#ifdef _DEBUG
		disjointRects.Add(newNode);
#endif
	}
	else
		memset(&newNode, 0, sizeof(newNode));

	return newNode;
}

Rect SkylineBinPack::FindPositionForNewNodeMinWaste(int width, int height, int &bestHeight, int &bestWastedArea, int &bestIndex, bool allowFlip) const
{
	bestHeight = eastl::numeric_limits<int>::max();
	bestWastedArea = eastl::numeric_limits<int>::max();
	bestIndex = -1;
	Rect newNode;
	memset(&newNode, 0, sizeof(newNode));
	for(unsigned i = 0; i < skyLine.size(); ++i)
	{
		int y;
		int wastedArea;

		if (RectangleFits(i, width, height, y, wastedArea))
		{
			if (wastedArea < bestWastedArea || (wastedArea == bestWastedArea && y + height < bestHeight))
			{
				bestHeight = y + height;
				bestWastedArea = wastedArea;
				bestIndex = i;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = width;
				newNode.height = height;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
#if !DEFINED_NO_FLIPPING
		if (allowFlip && RectangleFits(i, height, width, y, wastedArea))
		{
			if (wastedArea < bestWastedArea || (wastedArea == bestWastedArea && y + width < bestHeight))
			{
				bestHeight = y + width;
				bestWastedArea = wastedArea;
				bestIndex = i;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = height;
				newNode.height = width;
				debug_assert(disjointRects.Disjoint(newNode));
			}
		}
#endif
	}

	return newNode;
}

/// Computes the ratio of used surface area.
float SkylineBinPack::Occupancy() const
{
	return (float)usedSurfaceArea / (binWidth * binHeight);
}

}