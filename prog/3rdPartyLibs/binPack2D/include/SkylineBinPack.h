/** @file SkylineBinPack.h
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the SKYLINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <EASTL/vector.h>

#include "Rect.h"
#include "GuillotineBinPack.h"

namespace rbp {

/** Implements bin packing algorithms that use the SKYLINE data structure to store the bin contents. Uses
	GuillotineBinPack as the waste map. */
class SkylineBinPack
{
public:
	/// Instantiates a bin of size (0,0). Call Init to create a new bin.
	SkylineBinPack();

	/// Instantiates a bin of the given size.
	SkylineBinPack(int binWidth, int binHeight, bool useWasteMap);

	/// (Re)initializes the packer to an empty bin of width x height units. Call whenever
	/// you need to restart with a new bin.
	void Init(int binWidth, int binHeight, bool useWasteMap);

	/// Defines the different heuristic rules that can be used to decide how to make the rectangle placements.
	enum LevelChoiceHeuristic
	{
		LevelBottomLeft,
		LevelMinWasteFit
	};

	/// Inserts the given list of rectangles in an offline/batch mode, possibly rotated.
	/// @param rects The list of rectangles to insert. This vector will be destroyed in the process.
	/// @param dst [out] This list will contain the packed rectangles. The indices will not correspond to that of rects.
	/// @param method The rectangle placement rule to use when packing.
	void Insert(eastl::vector<RectSizeRequest> &rects, eastl::vector<RectResult> &dst, LevelChoiceHeuristic method, bool allowFlip);

	/// Inserts a single rectangle into the bin, possibly rotated.
	Rect Insert(int width, int height, LevelChoiceHeuristic method, bool allowFlip);

	/// Computes the ratio of used surface area to the total bin area.
	float Occupancy() const;

    void freeUsedRect(const Rect &rect)
    {
      usedSurfaceArea -= rect.width*rect.height;
      wasteMap.freeUsedRect(rect, true);
    }

private:
	int binWidth;
	int binHeight;

#ifdef _DEBUG
	DisjointRectCollection disjointRects;
#endif

	/// Represents a single level (a horizontal line) of the skyline/horizon/envelope.
	struct SkylineNode
	{
		/// The starting x-coordinate (leftmost).
		int x;

		/// The y-coordinate of the skyline level line.
		int y;

		/// The line width. The ending coordinate (inclusive) will be x+width-1.
		int width;
	};

	eastl::vector<SkylineNode> skyLine;

	unsigned long usedSurfaceArea;

	/// If true, we use the GuillotineBinPack structure to recover wasted areas into a waste map.
	bool useWasteMap;
	GuillotineBinPack wasteMap;

	Rect InsertBottomLeft(int width, int height, bool allowFlip);
	Rect InsertMinWaste(int width, int height, bool allowFlip);

	Rect FindPositionForNewNodeMinWaste(int width, int height, int &bestHeight, int &bestWastedArea, int &bestIndex, bool allowFlip) const;
	Rect FindPositionForNewNodeBottomLeft(int width, int height, int &bestHeight, int &bestWidth, int &bestIndex, bool allowFlip) const;

	bool RectangleFits(int skylineNodeIndex, int width, int height, int &y) const;
	bool RectangleFits(int skylineNodeIndex, int width, int height, int &y, int &wastedArea) const;
	int ComputeWastedArea(int skylineNodeIndex, int width, int height, int y) const;

	void AddWasteMapArea(int skylineNodeIndex, int width, int height, int y);

	void AddSkylineLevel(int skylineNodeIndex, const Rect &rect, bool allowFlip);

	/// Merges all skyline nodes that are at the same level.
	void MergeSkylines();
};

}