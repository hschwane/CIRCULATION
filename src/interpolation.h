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
 *          position must be valid, i.e. between coordinate system min+offset and max-offset
 * @param position position to read from must be valid i.e between coordinate system min+offset and max-offset
 * @param grid grid to read from
 * @param cs coordinate system to use
 * @param offset offset of the value on the C grid
 * @return interpolated value at position
 */
template <AT Param, typename GridRefT, typename CoordSystemT>
CUDAHOSTDEV auto readInterpolated2D(float2 position, GridRefT& grid, const CoordSystemT& cs, float2 offset = float2{0,0})
{
    static_assert(std::is_same<CoordSystemT,GeographicalCoordinates2D>::value, "Not implemented for cartesian coordinates yet.");

    // compensate for offset when computing cell id
    float2 cellIdf = ((position-offset) - make_float2(cs.getMinCoord())) / make_float2(cs.getCellSize());
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

/**
 * @brief interpolate a value over the pole
 * @param originalPos the position at the boundary from where to interpolate
 * @param valueAtOriginalPos the value at the boundary position
 * @param targetLatitudeExt the latitude of the target position in extended coordinates (defined without discontinuty beyond the pole)
 * @param grid the grid to read values from
 * @param cs the coordinate system used
 * @param negate should the value on the other side of the pole be negated
 * @param offset offset of the parameter on the C grid
 * @return the value at position (originalPos.x, targetLatitudeExt)
 */
template <AT Param, typename GridRefT>
CUDAHOSTDEV auto interpolateNorthPole2D(float2 originalPos, float valueAtOriginalPos,
        float targetLatitudeExt, GridRefT& grid, const GeographicalCoordinates2D& cs, float2 offset=float2{0,0}, bool negate=false)
{
    // find position on the other side of the pole in extended coordinates
    float2 interpolationPos = originalPos;
//    interpolationPos.y += 2*(-originalPos.y);
    interpolationPos.y += originalPos.y + 2*(M_PIf32*0.5f - originalPos.y);

    // find the same position in actual coordinates
    float2 dataLoadPos = originalPos;
    dataLoadPos.x = fmod(originalPos.x + M_PIf32, 2 * M_PIf32);

    // get the value at dataLoadPos, compensate for offset
    float2 cellIdf = ((dataLoadPos-offset) - make_float2(cs.getMinCoord())) / make_float2(cs.getCellSize());
    int2 lowerId = make_int2(cellIdf); // truncate to get lower cell
    int2 upperId = lowerId + int2{1,0}; // upper cell is the one to the right
    float f = cellIdf.x - lowerId.x; // interpolation weight is calculated from the fraction

    // wrap around ids in x direction
    lowerId.x = lowerId.x % cs.getNumGridCells3d().x;
    upperId.x = upperId.x % cs.getNumGridCells3d().x;

    // read values
    auto v1 = ((negate) ? -1 : 1) * grid.read<AT::geopotential>(cs.getCellId(make_int3(lowerId)));
    auto v2 = ((negate) ? -1 : 1) * grid.read<AT::geopotential>(cs.getCellId(make_int3(upperId)));

    // do interpolation
    auto valueAtInterpolationPosition = (v1 * (1-f) + v2 * f);

    // now we interpolate from interpolationPos to the position where the value is actually needed
    // using the extended coordinates
    return linearInterpolate(targetLatitudeExt, originalPos.y, valueAtOriginalPos, interpolationPos.y, valueAtInterpolationPosition);
}

//template <AT Param, typename GridRefT, typename CoordSystemT, std::enable_if< !std::is_same<CoordSystemT, GeographicalCoordinates2D() >::value,int> =0>
//CUDAHOSTDEV auto readInterpolated2D(const float2& position, GridRefT& grid, const CoordSystemT& cs)
//{
//    static_assert(mpu::always_false_v<CoordSystemT>, "Not implementes for cartesian coordinates yet.");
//    static_assert(CoordSystemT::isCartesian, "This overload only works for cartesian coordinates.");
//}




#endif //CIRCULATION_INTERPOLATION_H
