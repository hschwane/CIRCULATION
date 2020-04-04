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

#endif //CIRCULATION_INTERPOLATION_H
