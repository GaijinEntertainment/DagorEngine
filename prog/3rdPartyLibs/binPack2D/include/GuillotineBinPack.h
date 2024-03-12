/** @file GuillotineBinPack.h
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the GUILLOTINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <EASTL/vector.h>

#include "Rect.h"

namespace rbp {

/** GuillotineBinPack implements different variants of bin packer algorithms that use the GUILLOTINE data structure
	to keep track of the free space of the bin where rectangles may be placed. */
class GuillotineBinPack
{
public:
	/// The initial bin size will be (0,0). Call Init to set the bin size.
	GuillotineBinPack();

	/// Initializes a new bin of the given size.
	GuillotineBinPack(int width, int height);

	/// (Re)initializes the packer to an empty bin of width x height units. Call whenever
	/// you need to restart with a new bin.
	void Init(int width, int height);

	/// Specifies the different choice heuristics that can be used when deciding which of the free subrectangles
	/// to place the to-be-packed rectangle into.
	enum FreeRectChoiceHeuristic
	{
		RectBestAreaFit, ///< -BAF
		RectBestShortSideFit, ///< -BSSF
		RectBestLongSideFit, ///< -BLSF
		RectWorstAreaFit, ///< -WAF
		RectWorstShortSideFit, ///< -WSSF
		RectWorstLongSideFit ///< -WLSF
	};

	/// Specifies the different choice heuristics that can be used when the packer needs to decide whether to
	/// subdivide the remaining free space in horizontal or vertical direction.
	enum GuillotineSplitHeuristic
	{
		SplitShorterLeftoverAxis, ///< -SLAS
		SplitLongerLeftoverAxis, ///< -LLAS
		SplitMinimizeArea, ///< -MINAS, Try to make a single big rectangle at the expense of making the other small.
		SplitMaximizeArea, ///< -MAXAS, Try to make both remaining rectangles as even-sized as possible.
		SplitShorterAxis, ///< -SAS
		SplitLongerAxis ///< -LAS
	};

	/// Inserts a single rectangle into the bin. The packer might rotate the rectangle, in which case the returned
	/// struct will have the width and height values swapped.
	/// @param merge If true, performs free Rectangle Merge procedure after packing the new rectangle. This procedure
	///		tries to defragment the list of disjoint free rectangles to improve packing performance, but also takes up 
	///		some extra time.
	/// @param rectChoice The free rectangle choice heuristic rule to use.
	/// @param splitMethod The free rectangle split heuristic rule to use.
	Rect Insert(int width, int height, bool merge, FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod, bool allowFlip);

	/// Inserts a list of rectangles into the bin.
	/// @param rects The list of rectangles to add. This list will be destroyed in the packing process.
	/// @param merge If true, performs Rectangle Merge operations during the packing process.
	/// @param rectChoice The free rectangle choice heuristic rule to use.
	/// @param splitMethod The free rectangle split heuristic rule to use.
    /// @param usedRects result rects
	void Insert(eastl::vector<RectSizeRequest> &rects, eastl::vector<RectResult> &usedRects, bool merge, 
		FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod, bool allowFlip);

// Implements GUILLOTINE-MAXFITTING, an experimental heuristic that's really cool but didn't quite work in practice.
//	void InsertMaxFitting(eastl::vector<RectSize> &rects, eastl::vector<Rect> &dst, bool merge, 
//		FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod);

	/// Computes the ratio of used/total surface area. 0.00 means no space is yet used, 1.00 means the whole bin is used.
	float Occupancy(int totalArea) const {return usedSurfaceArea/float(totalArea);}
    
    /// Computes the ratio of used/total surface area. 0.00 means no space is yet used, 1.00 means the whole bin is used.
	float Occupancy() const
    {
      int freeArea = freeSpaceArea();
      if (freeArea + usedSurfaceArea == 0)
          return 0.f;
      return usedSurfaceArea/float(freeArea + usedSurfaceArea);
    }

    //counts! O(N) of freeRects
    int freeSpaceArea() const;
    unsigned occupiedArea() const {return usedSurfaceArea;}

	/// Returns the internal list of disjoint rectangles that track the free area of the bin. You may alter this vector
	/// any way desired, as long as the end result still is a list of disjoint rectangles.
    // however, usedSurfaceArea won't be updated
	eastl::vector<Rect> &GetFreeRectangles() { return freeRectangles; }

    // add one of previously allocated rects to free. beware that it is actually one of allocated!
    void freeUsedRect(const Rect &rect, bool try_merge_it);

	/// Performs a Rectangle Merge operation. This procedure looks for adjacent free rectangles and merges them if they
	/// can be represented with a single rectangle. Takes up Theta(|freeRectangles|^2) time.
	void MergeFreeList();

private:
    unsigned usedSurfaceArea;
    unsigned short maxFreeW, maxFreeH;

	/// Stores a list of rectangles that represents the free area of the bin. This rectangles in this list are disjoint.
	eastl::vector<Rect> freeRectangles;

#ifdef _DEBUG
	/// Used to track that the packer produces proper packings.
	DisjointRectCollection disjointRects;
#endif

	/// Goes through the list of free rectangles and finds the best one to place a rectangle of given size into.
	/// Running time is Theta(|freeRectangles|).
	/// @param nodeIndex [out] The index of the free rectangle in the freeRectangles array into which the new
	///		rect was placed.
	/// @return A Rect structure that represents the placement of the new rect into the best free rectangle.
	Rect FindPositionForNewNode(int width, int height, FreeRectChoiceHeuristic rectChoice, int *nodeIndex, bool allowFlip);

	static int ScoreByHeuristic(int width, int height, const Rect &freeRect, FreeRectChoiceHeuristic rectChoice);
	// The following functions compute (penalty) score values if a rect of the given size was placed into the 
	// given free rectangle. In these score values, smaller is better.

	static int ScoreBestAreaFit(int width, int height, const Rect &freeRect);
	static int ScoreBestShortSideFit(int width, int height, const Rect &freeRect);
	static int ScoreBestLongSideFit(int width, int height, const Rect &freeRect);

	static int ScoreWorstAreaFit(int width, int height, const Rect &freeRect);
	static int ScoreWorstShortSideFit(int width, int height, const Rect &freeRect);
	static int ScoreWorstLongSideFit(int width, int height, const Rect &freeRect);

	/// Splits the given L-shaped free rectangle into two new free rectangles after placedRect has been placed into it.
	/// Determines the split axis by using the given heuristic.
	void SplitFreeRectByHeuristic(const Rect &freeRect, const Rect &placedRect, GuillotineSplitHeuristic method);

	/// Splits the given L-shaped free rectangle into two new free rectangles along the given fixed split axis.
	void SplitFreeRectAlongAxis(const Rect &freeRect, const Rect &placedRect, bool splitHorizontal);
};

}
