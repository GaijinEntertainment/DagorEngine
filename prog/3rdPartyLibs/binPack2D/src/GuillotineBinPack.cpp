/** @file GuillotineBinPack.cpp
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the GUILLOTINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#include <EASTL/algorithm.h>

#include <GuillotineBinPack.h>

namespace rbp {

GuillotineBinPack::GuillotineBinPack():usedSurfaceArea(0)
{
}

GuillotineBinPack::GuillotineBinPack(int width, int height)
{
	Init(width, height);
}

void GuillotineBinPack::Init(int width, int height)
{
    usedSurfaceArea = 0;

#ifdef _DEBUG
	disjointRects.Clear();
#endif

	// We start with a single big free rectangle that spans the whole bin.
	Rect n;
	n.x = 0;
	n.y = 0;
	n.width = width;
	n.height = height;
    maxFreeW = width;
    maxFreeH = height;

	freeRectangles.clear();
	freeRectangles.push_back(n);
}

void GuillotineBinPack::Insert(eastl::vector<RectSizeRequest> &rects, eastl::vector<RectResult> &usedRectangles, bool merge, 
	FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod, bool allowFlip)
{
	// Remember variables about the best packing choice we have made so far during the iteration process.
	int bestFreeRect = 0;
	int bestRect = 0;
	bool bestFlipped = false;

	// Pack rectangles one at a time until we have cleared the rects array of all rectangles.
	// rects will get destroyed in the process.
	while(rects.size() > 0)
	{
		// Stores the penalty score of the best rectangle placement - bigger=worse, smaller=better.
		int bestScore = eastl::numeric_limits<int>::max();

		for(unsigned i = 0; i < freeRectangles.size(); ++i)
		{
			for(unsigned j = 0; j < rects.size(); ++j)
			{
				// If this rectangle is a perfect match, we pick it instantly.
				if (rects[j].width == freeRectangles[i].width && rects[j].height == freeRectangles[i].height)
				{
					bestFreeRect = i;
					bestRect = j;
					bestFlipped = false;
					bestScore = eastl::numeric_limits<int>::min();
					i = (unsigned)freeRectangles.size(); // Force a jump out of the outer loop as well - we got an instant fit.
					break;
				}
#if !DEFINED_NO_FLIPPING
				// If flipping this rectangle is a perfect match, pick that then.
				else if (allowFlip && rects[j].height == freeRectangles[i].width && rects[j].width == freeRectangles[i].height)
				{
					bestFreeRect = i;
					bestRect = j;
					bestFlipped = true;
					bestScore = eastl::numeric_limits<int>::min();
					i = (unsigned)freeRectangles.size(); // Force a jump out of the outer loop as well - we got an instant fit.
					break;
				}
#endif
				// Try if we can fit the rectangle upright.
				else if (rects[j].width <= freeRectangles[i].width && rects[j].height <= freeRectangles[i].height)
				{
					int score = ScoreByHeuristic(rects[j].width, rects[j].height, freeRectangles[i], rectChoice);
					if (score < bestScore)
					{
						bestFreeRect = i;
						bestRect = j;
						bestFlipped = false;
						bestScore = score;
					}
				}
#if !DEFINED_NO_FLIPPING
				// If not, then perhaps flipping sideways will make it fit?
				else if (allowFlip && rects[j].height <= freeRectangles[i].width && rects[j].width <= freeRectangles[i].height)
				{
					int score = ScoreByHeuristic(rects[j].height, rects[j].width, freeRectangles[i], rectChoice);
					if (score < bestScore)
					{
						bestFreeRect = i;
						bestRect = j;
						bestFlipped = true;
						bestScore = score;
					}
				}
#endif
			}
		}

		// If we didn't manage to find any rectangle to pack, abort.
		if (bestScore == eastl::numeric_limits<int>::max())
			return;

		// Otherwise, we're good to go and do the actual packing.
		RectResult newNode;
		newNode.x = freeRectangles[bestFreeRect].x;
		newNode.y = freeRectangles[bestFreeRect].y;
		newNode.width = rects[bestRect].width;
		newNode.height = rects[bestRect].height;
    	newNode.id = rects[bestRect].id;

		if (bestFlipped)
			eastl::swap(newNode.width, newNode.height);

		// Remove the free space we lost in the bin.
		SplitFreeRectByHeuristic(freeRectangles[bestFreeRect], newNode, splitMethod);
		freeRectangles.erase(freeRectangles.begin() + bestFreeRect);

		// Remove the rectangle we just packed from the input list.
		rects.erase(rects.begin() + bestRect);

		// Perform a Rectangle Merge step if desired.
		if (merge)
			MergeFreeList();

		// Remember the new used rectangle.
        usedSurfaceArea += newNode.width*newNode.height;
		usedRectangles.push_back(newNode);

		// Check that we're really producing correct packings here.
		debug_assert(disjointRects.Add(newNode) == true);
	}
}

/// @return True if r fits inside freeRect (possibly rotated).
static inline bool Fits(const RectSize &r, const Rect &freeRect)
{
	return (r.width <= freeRect.width && r.height <= freeRect.height) ||
		(r.height <= freeRect.width && r.width <= freeRect.height);
}

/// @return True if r fits perfectly inside freeRect, i.e. the leftover area is 0.
static inline bool FitsPerfectly(const RectSize &r, const Rect &freeRect)
{
	return (r.width == freeRect.width && r.height == freeRect.height) ||
		(r.height == freeRect.width && r.width == freeRect.height);
}


Rect GuillotineBinPack::Insert(int width, int height, bool merge, FreeRectChoiceHeuristic rectChoice, 
	GuillotineSplitHeuristic splitMethod, bool allowFlip)
{
	// Find where to put the new rectangle.
    int maxFree = eastl::max(maxFreeW, maxFreeH);
    if ((allowFlip && (width > maxFree || height > maxFree)) || (!allowFlip && (width > maxFreeW || height > maxFreeH)))
    {
       Rect bestNode;
       bestNode.x = bestNode.y = bestNode.width = bestNode.height = 0;
       return bestNode;
    }
	int freeNodeIndex = 0;
	Rect newRect = FindPositionForNewNode(width, height, rectChoice, &freeNodeIndex, allowFlip);

	// Abort if we didn't have enough space in the bin.
	if (newRect.height == 0)
		return newRect;

	// Remove the space that was just consumed by the new rectangle.
	SplitFreeRectByHeuristic(freeRectangles[freeNodeIndex], newRect, splitMethod);
	freeRectangles.erase(freeRectangles.begin() + freeNodeIndex);

	// Perform a Rectangle Merge step if desired.
	if (merge)
		MergeFreeList();

	// Remember the new used rectangle.
    usedSurfaceArea += newRect.width*newRect.height;

	// Check that we're really producing correct packings here.
	debug_assert(disjointRects.Add(newRect) == true);

	return newRect;
}

/// Returns the heuristic score value for placing a rectangle of size width*height into freeRect. Does not try to rotate.
inline int GuillotineBinPack::ScoreByHeuristic(int width, int height, const Rect &freeRect, FreeRectChoiceHeuristic rectChoice)
{
	switch(rectChoice)
	{
	case RectBestAreaFit: return ScoreBestAreaFit(width, height, freeRect);
	case RectBestShortSideFit: return ScoreBestShortSideFit(width, height, freeRect);
	case RectBestLongSideFit: return ScoreBestLongSideFit(width, height, freeRect);
	case RectWorstAreaFit: return ScoreWorstAreaFit(width, height, freeRect);
	case RectWorstShortSideFit: return ScoreWorstShortSideFit(width, height, freeRect);
	case RectWorstLongSideFit: return ScoreWorstLongSideFit(width, height, freeRect);
	default: rbp_assert(false); return eastl::numeric_limits<int>::max();
	}
}

inline int GuillotineBinPack::ScoreBestAreaFit(int width, int height, const Rect &freeRect)
{
	return freeRect.width * freeRect.height - width * height;
}

inline int GuillotineBinPack::ScoreBestShortSideFit(int width, int height, const Rect &freeRect)
{
	int leftoverHoriz = abs(freeRect.width - width);
	int leftoverVert = abs(freeRect.height - height);
	int leftover = eastl::min(leftoverHoriz, leftoverVert);
	return leftover;
}

inline int GuillotineBinPack::ScoreBestLongSideFit(int width, int height, const Rect &freeRect)
{
	int leftoverHoriz = abs(freeRect.width - width);
	int leftoverVert = abs(freeRect.height - height);
	int leftover = eastl::max(leftoverHoriz, leftoverVert);
	return leftover;
}

inline int GuillotineBinPack::ScoreWorstAreaFit(int width, int height, const Rect &freeRect)
{
	return -ScoreBestAreaFit(width, height, freeRect);
}

inline int GuillotineBinPack::ScoreWorstShortSideFit(int width, int height, const Rect &freeRect)
{
	return -ScoreBestShortSideFit(width, height, freeRect);
}

inline int GuillotineBinPack::ScoreWorstLongSideFit(int width, int height, const Rect &freeRect)
{
	return -ScoreBestLongSideFit(width, height, freeRect);
}

Rect GuillotineBinPack::FindPositionForNewNode(int width, int height, FreeRectChoiceHeuristic rectChoice, int *nodeIndex, bool allowFlip)
{
	Rect bestNode;
    bestNode.x = bestNode.y = bestNode.width = bestNode.height = 0;

	int bestScore = eastl::numeric_limits<int>::max();
    int maxFreeLocalW = 0, maxFreeLocalH = 0;

	/// Try each free rectangle to find the best one for placement.
	for(unsigned i = 0; i < freeRectangles.size(); ++i)
	{
		// If this is a perfect fit upright, choose it immediately.
        maxFreeLocalW = eastl::max(maxFreeLocalW, freeRectangles[i].width), maxFreeLocalH = eastl::max(maxFreeLocalH, freeRectangles[i].height);
		if (width == freeRectangles[i].width && height == freeRectangles[i].height)
		{
			bestNode.x = freeRectangles[i].x;
			bestNode.y = freeRectangles[i].y;
			bestNode.width = width;
			bestNode.height = height;
			bestScore = eastl::numeric_limits<int>::min();
			*nodeIndex = i;
			debug_assert(disjointRects.Disjoint(bestNode));
			return bestNode;
		}
#if !DEFINED_NO_FLIPPING
		// If this is a perfect fit sideways, choose it.
		else if (allowFlip && height == freeRectangles[i].width && width == freeRectangles[i].height)
		{
			bestNode.x = freeRectangles[i].x;
			bestNode.y = freeRectangles[i].y;
			bestNode.width = height;
			bestNode.height = width;
			bestScore = eastl::numeric_limits<int>::min();
			*nodeIndex = i;
			debug_assert(disjointRects.Disjoint(bestNode));
			return bestNode;
		}
#endif
		// Does the rectangle fit upright?
		else if (width <= freeRectangles[i].width && height <= freeRectangles[i].height)
		{
			int score = ScoreByHeuristic(width, height, freeRectangles[i], rectChoice);

			if (score < bestScore)
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = width;
				bestNode.height = height;
				bestScore = score;
				*nodeIndex = i;
				debug_assert(disjointRects.Disjoint(bestNode));
			}
		}
#if !DEFINED_NO_FLIPPING
		// Does the rectangle fit sideways?
		else if (allowFlip && height <= freeRectangles[i].width && width <= freeRectangles[i].height)
		{
			int score = ScoreByHeuristic(height, width, freeRectangles[i], rectChoice);

			if (score < bestScore)
			{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
				bestNode.width = height;
				bestNode.height = width;
				bestScore = score;
				*nodeIndex = i;
				debug_assert(disjointRects.Disjoint(bestNode));
			}
		}
#endif
	}
    maxFreeW = maxFreeLocalW; maxFreeH = maxFreeLocalH;
	return bestNode;
}

void GuillotineBinPack::SplitFreeRectByHeuristic(const Rect &freeRect, const Rect &placedRect, GuillotineSplitHeuristic method)
{
	// Compute the lengths of the leftover area.
	const int w = freeRect.width - placedRect.width;
	const int h = freeRect.height - placedRect.height;

	// Placing placedRect into freeRect results in an L-shaped free area, which must be split into
	// two disjoint rectangles. This can be achieved with by splitting the L-shape using a single line.
	// We have two choices: horizontal or vertical.	

	// Use the given heuristic to decide which choice to make.

	bool splitHorizontal;
	switch(method)
	{
	case SplitShorterLeftoverAxis:
		// Split along the shorter leftover axis.
		splitHorizontal = (w <= h);
		break;
	case SplitLongerLeftoverAxis:
		// Split along the longer leftover axis.
		splitHorizontal = (w > h);
		break;
	case SplitMinimizeArea:
		// Maximize the larger area == minimize the smaller area.
		// Tries to make the single bigger rectangle.
		splitHorizontal = (placedRect.width * h > w * placedRect.height);
		break;
	case SplitMaximizeArea:
		// Maximize the smaller area == minimize the larger area.
		// Tries to make the rectangles more even-sized.
		splitHorizontal = (placedRect.width * h <= w * placedRect.height);
		break;
	case SplitShorterAxis:
		// Split along the shorter total axis.
		splitHorizontal = (freeRect.width <= freeRect.height);
		break;
	case SplitLongerAxis:
		// Split along the longer total axis.
		splitHorizontal = (freeRect.width > freeRect.height);
		break;
	default:
		splitHorizontal = true;
		rbp_assert(false);
	}

	// Perform the actual split.
	SplitFreeRectAlongAxis(freeRect, placedRect, splitHorizontal);
}

/// This function will add the two generated rectangles into the freeRectangles array. The caller is expected to
/// remove the original rectangle from the freeRectangles array after that.
void GuillotineBinPack::SplitFreeRectAlongAxis(const Rect &freeRect, const Rect &placedRect, bool splitHorizontal)
{
	// Form the two new rectangles.
	Rect bottom;
	bottom.x = freeRect.x;
	bottom.y = freeRect.y + placedRect.height;
	bottom.height = freeRect.height - placedRect.height;

	Rect right;
	right.x = freeRect.x + placedRect.width;
	right.y = freeRect.y;
	right.width = freeRect.width - placedRect.width;

	if (splitHorizontal)
	{
		bottom.width = freeRect.width;
		right.height = placedRect.height;
	}
	else // Split vertically
	{
		bottom.width = placedRect.width;
		right.height = freeRect.height;
	}

	// Add the new rectangles into the free rectangle pool if they weren't degenerate.
	if (bottom.width > 0 && bottom.height > 0)
		freeRectangles.push_back(bottom);
	if (right.width > 0 && right.height > 0)
		freeRectangles.push_back(right);

	debug_assert(disjointRects.Disjoint(bottom));
	debug_assert(disjointRects.Disjoint(right));
}

void GuillotineBinPack::MergeFreeList()
{
#ifdef _DEBUG
	DisjointRectCollection test;
	for(unsigned i = 0; i < freeRectangles.size(); ++i)
		assert(test.Add(freeRectangles[i]) == true);
#endif

	// Do a Theta(n^2) loop to see if any pair of free rectangles could me merged into one.
	// Note that we miss any opportunities to merge three rectangles into one. (should call this function again to detect that)
    int maxFreeLocalH = 0, maxFreeLocalW = 0;
	for(unsigned i = 0; i < freeRectangles.size(); ++i)
    {
    	for(unsigned j = i+1; j < freeRectangles.size(); ++j)
		{
			if (freeRectangles[i].width == freeRectangles[j].width && freeRectangles[i].x == freeRectangles[j].x)
			{
				if (freeRectangles[i].y == freeRectangles[j].y + freeRectangles[j].height)
				{
					freeRectangles[i].y -= freeRectangles[j].height;
					freeRectangles[i].height += freeRectangles[j].height;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
				else if (freeRectangles[i].y + freeRectangles[i].height == freeRectangles[j].y)
				{
					freeRectangles[i].height += freeRectangles[j].height;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
			}
			else if (freeRectangles[i].height == freeRectangles[j].height && freeRectangles[i].y == freeRectangles[j].y)
			{
				if (freeRectangles[i].x == freeRectangles[j].x + freeRectangles[j].width)
				{
					freeRectangles[i].x -= freeRectangles[j].width;
					freeRectangles[i].width += freeRectangles[j].width;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
				else if (freeRectangles[i].x + freeRectangles[i].width == freeRectangles[j].x)
				{
					freeRectangles[i].width += freeRectangles[j].width;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
			}
		}
        maxFreeLocalW = eastl::max(freeRectangles[i].width, maxFreeLocalW);
        maxFreeLocalH = eastl::max(freeRectangles[i].height, maxFreeLocalH);
    }
    maxFreeW = maxFreeLocalW; maxFreeH = maxFreeLocalH;

#ifdef _DEBUG
	test.Clear();
	for(unsigned i = 0; i < freeRectangles.size(); ++i)
		assert(test.Add(freeRectangles[i]) == true);
#endif
}

void GuillotineBinPack::freeUsedRect(const Rect &rect, bool merge_it)
{
  int area = rect.width*rect.height;
  if (area<=0)
    return;
  maxFreeW = maxFreeH = 65535;
  usedSurfaceArea -= area;
  if (!merge_it)
  {
    freeRectangles.push_back(rect);
    return;
  }

  for(unsigned i = 0; i < freeRectangles.size(); ++i)
  {
  	if (freeRectangles[i].width == rect.width && freeRectangles[i].x == rect.x)
  	{
  		if (freeRectangles[i].y == rect.y + rect.height)
  		{
  			freeRectangles[i].y -= rect.height;
  			freeRectangles[i].height += rect.height;
            return;
  		}
  		else if (freeRectangles[i].y + freeRectangles[i].height == rect.y)
  		{
  			freeRectangles[i].height += rect.height;
            return;
  		}
  	}
  	else if (freeRectangles[i].height == rect.height && freeRectangles[i].y == rect.y)
  	{
  		if (freeRectangles[i].x == rect.x + rect.width)
  		{
  			freeRectangles[i].x -= rect.width;
  			freeRectangles[i].width += rect.width;
            return;
  		}
  		else if (freeRectangles[i].x + freeRectangles[i].width == rect.x)
  		{
  			freeRectangles[i].width += rect.width;
            return;
  		}
  	}
  }
  freeRectangles.push_back(rect);
}


int GuillotineBinPack::freeSpaceArea() const
{
  int freeArea = 0;
  for (int i = 0; i < freeRectangles.size(); ++i)
    freeArea += freeRectangles[i].width*freeRectangles[i].height;
  return freeArea;
}

}
