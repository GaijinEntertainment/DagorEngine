/** @file MaxRectsBinPack.h
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the MAXRECTS data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <EASTL/vector.h>

#include "Rect.h"

namespace rbp {

/** MaxRectsBinPack implements the MAXRECTS data structure and different bin packing algorithms that 
	use this structure. */
class MaxRectsBinPack
{
public:
	/// Instantiates a bin of size (0,0). Call Init to create a new bin.
	MaxRectsBinPack();

	/// Instantiates a bin of the given size.
	MaxRectsBinPack(int width, int height);

	/// (Re)initializes the packer to an empty bin of width x height units. Call whenever
	/// you need to restart with a new bin.
	void Init(int width, int height);

	/// Specifies the different heuristic rules that can be used when deciding where to place a new rectangle.
	enum FreeRectChoiceHeuristic
	{
		RectBestShortSideFit, ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
		RectBestLongSideFit, ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
		RectBestAreaFit, ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
		RectBottomLeftRule, ///< -BL: Does the Tetris placement.
		//RectContactPointRule ///< -CP: Choosest the placement where the rectangle touches other rects as much as possible.
        //removed due to requirement to store used rectangles
	};

	/// Inserts the given list of rectangles in an offline/batch mode, possibly rotated.
	/// @param rects The list of rectangles to insert. This vector will be destroyed in the process.
	/// @param dst [out] This list will contain the packed rectangles. The indices will not correspond to that of rects.
	/// @param method The rectangle placement rule to use when packing.
	void Insert(eastl::vector<RectSizeRequest> &rects, eastl::vector<RectResult> &dst, FreeRectChoiceHeuristic method, bool allowFlip);

	/// Inserts a single rectangle into the bin, possibly rotated.
	Rect Insert(int width, int height, FreeRectChoiceHeuristic method, bool allowFlip);

    /// Returns the internal list of disjoint rectangles that track the free area of the bin. You may alter this vector
	/// any way desired, as long as the end result still is a list of disjoint rectangles.
    // however, usedSurfaceArea won't be updated
	eastl::vector<Rect> &GetFreeRectangles() { return freeRectangles; }

    // add one of previously allocated rects to free. beware that it is actually one of allocated!
    void freeUsedRect(const Rect &rect)
    {
      usedSurfaceArea -= rect.width*rect.height;
      freeRectangles.push_back(rect);
    }

	/// Computes the ratio of used surface area to the total bin area.
	float Occupancy() const;

    /// Goes through the free rectangle list and removes any redundant entries.
	void PruneFreeList();
private:
	int binWidth;
	int binHeight;
    unsigned usedSurfaceArea;

	eastl::vector<Rect> freeRectangles;

	/// Computes the placement score for placing the given rectangle with the given method.
	/// @param score1 [out] The primary placement score will be outputted here.
	/// @param score2 [out] The secondary placement score will be outputted here. This isu sed to break ties.
	/// @return This struct identifies where the rectangle would be placed if it were placed.
	Rect ScoreRect(int width, int height, FreeRectChoiceHeuristic method, int &score1, int &score2, bool allowFlip) const;

	/// Places the given rectangle into the bin.
	void PlaceRect(const Rect &node);

	/// Computes the placement score for the -CP variant.
	int ContactPointScoreNode(int x, int y, int width, int height) const;

	Rect FindPositionForNewNodeBottomLeft(int width, int height, int &bestY, int &bestX, bool allowFlip) const;
	Rect FindPositionForNewNodeBestShortSideFit(int width, int height, int &bestShortSideFit, int &bestLongSideFit, bool allowFlip) const;
	Rect FindPositionForNewNodeBestLongSideFit(int width, int height, int &bestShortSideFit, int &bestLongSideFit, bool allowFlip) const;
	Rect FindPositionForNewNodeBestAreaFit(int width, int height, int &bestAreaFit, int &bestShortSideFit, bool allowFlip) const;

	/// @return True if the free node was split.
	bool SplitFreeNode(Rect freeNode, const Rect &usedNode);
};

}
