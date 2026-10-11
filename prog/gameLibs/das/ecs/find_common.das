options indenting = 2
module find_common shared
options no_aot

require ecs
require strings
require daslib/strings_boost
require math.base


def get_max_mismatches_by_filter_length(filter_length : int)
  let allowedMismatchesByFilterLength <- fixed_array(3, 3, 5, 8, 10, 13)
  var maxMismatches = 0
  for m in allowedMismatchesByFilterLength
    if filter_length > m
      maxMismatches ++
    else
      break
  return maxMismatches


def build_shingles(filter_str : string; var out_shingles : array<string>)
  let filterStringLength = length(filter_str)
  for i in range(filterStringLength - 1)
    out_shingles |> push(slice(filter_str, i, i + 2))
  return filterStringLength


def compute_fuzzy_distance(searched : string; shingles : array<string>; total_filter_length : int) : float
  let maxMismatches = get_max_mismatches_by_filter_length(total_filter_length)
  let maxMisNeighbourCount = 100
  var misNeighbourCount = 0
  var mismatchCount = 0
  var prevIdx = -1
  var lastFindIdx = 0
  var shingleDistance = 0
  for shingle in shingles
    let idx = find(searched, shingle, lastFindIdx)
    if idx == -1
      mismatchCount++
      if mismatchCount > maxMismatches
        break
    elif prevIdx != -1 && prevIdx + 1 != idx
      misNeighbourCount++
      if misNeighbourCount > maxMisNeighbourCount
        break
    prevIdx = idx
    if idx != -1
      shingleDistance += abs(idx - lastFindIdx)
      lastFindIdx = idx
  if mismatchCount > maxMismatches || misNeighbourCount > maxMisNeighbourCount
    return -1.0
  return float(mismatchCount) + float(misNeighbourCount) * 0.5 + float(shingleDistance) * 0.05