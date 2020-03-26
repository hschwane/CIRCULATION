/*
 * CIRCULATION
 * interpolation.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */
#ifndef CIRCULATION_INTERPOLATION_H
#define CIRCULATION_INTERPOLATION_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpCuda.h>
#include "GridReference.h"
#include "coordinateSystems/GeographicalCoordinates2D.h"
//--------------------

/**
 * @brief approximate the value at targetPosition using values at position A and B in 1D
 */
template <typename T>
CUDAHOSTDEV T linearInterpolate(float targetPosition, float positionA, T valueA, float positionB, T valueB)
{
    float f = (targetPosition-positionA) / (positionB-positionA);
    return valueA * (1-f) + valueB * f;
}

/**
 * @brief approximate the value at targetPosition using values at four positions around it
 * @param valueAA value at (AX,AY)
 * @param valueBA value at (BX,AY)
 * @param valueAB value at (AX,BY)
 * @param valueBB value at (AY,BY)
 */
template <typename T>
CUDAHOSTDEV T bilinearInterpolate(float2 targetPosition, float positionAX, float positionBX, float positionAY, float positionBY,
                                                        T valueAA, T valueBA, T valueAB, T valueBB)
{
    T vxA = linearInterpolate(targetPosition.x, positionAX, valueAA, positionBX, valueBA);
    T vxB = linearInterpolate(targetPosition.x, positionAX, valueAB, positionAX, valueBB);
    return linearInterpolate(targetPosition.y, positionAY, vxA, positionBY, vxB);
}

/**
 * @brief read value from grid at floating point position using values at neighboring cells and billiniear interpolation
 * @param position position to read from must be valid i.e between coordinate system min and max
 * @param grid grid to read from
 * @param cs coordinate system to use
 * @return interpolated value at position
 */
template <AT Param, typename GridRefT, typename CoordSystemT>
CUDAHOSTDEV auto readInterpolated2D(const float2& position, GridRefT& grid, const CoordSystemT& cs)
{
    static_assert(std::is_same<CoordSystemT,GeographicalCoordinates2D>::value, "Not implemented for cartesian coordinates yet.");

    float2 cellIdf = (position - make_float2(cs.getMinCoord())) / make_float2(cs.getCellSize());
    int2 lowerId = make_int2(cellIdf); // truncate to get lower cell
    int2 upperId = lowerId + 1; // upper cell is just the next one
    float2 f = cellIdf - make_float2(lowerId); // interpolation weight is calculated from the fraction

    // wrap around ids in x direction
    lowerId.x = lowerId.x % cs.getNumGridCells3d().x;
    upperId.x = upperId.x % cs.getNumGridCells3d().x;

    // now read values
    auto v1 = grid.read<Param>(cs.getCellId(make_int3(lowerId)));
    auto v2 = grid.read<Param>(cs.getCellId(int3{lowerId.x,upperId.y,0}));
    auto v3 = grid.read<Param>(cs.getCellId(make_int3(upperId)));
    auto v4 = grid.read<Param>(cs.getCellId(int3{lowerId.y,upperId.x,0}));

    // billinear interpolation
    return (1-f.y) * (v1 * (1-f.x) + v2 * f.x)
            + f.y * (v4 * (1-f.x) + v3 * f.x);
}

//template <AT Param, typename GridRefT, typename CoordSystemT, std::enable_if< !std::is_same<CoordSystemT, GeographicalCoordinates2D() >::value,int> =0>
//CUDAHOSTDEV auto readInterpolated2D(const float2& position, GridRefT& grid, const CoordSystemT& cs)
//{
//    static_assert(mpu::always_false_v<CoordSystemT>, "Not implementes for cartesian coordinates yet.");
//    static_assert(CoordSystemT::isCartesian, "This overload only works for cartesian coordinates.");
//}




#endif //CIRCULATION_INTERPOLATION_H
