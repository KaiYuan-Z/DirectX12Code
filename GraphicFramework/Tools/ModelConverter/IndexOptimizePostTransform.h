#pragma once

//-----------------------------------------------------------------------------
//  OptimizeFaces
//-----------------------------------------------------------------------------
//  Parameters:
//      indexList
//          input index list
//      indexCount
//          the number of indices in the list
//      newIndexList
//          a pointer to a preallocated buffer the same size as indexList to
//          hold the optimized index list
//      lruCacheSize
//          the size of the simulated post-transform cache (max:64)
//-----------------------------------------------------------------------------
template <typename IndexType>
void OptimizeFaces(const IndexType* indexList, uint32_t indexCount, IndexType* newIndexList, uint16_t lruCacheSize);

template void OptimizeFaces<uint16_t>(const uint16_t* indexList, uint32_t indexCount, uint16_t* newIndexList, uint16_t lruCacheSize);
template void OptimizeFaces<uint32_t>(const uint32_t* indexList, uint32_t indexCount, uint32_t* newIndexList, uint16_t lruCacheSize);
