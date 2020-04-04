/*
 * CIRCULATION
 * finiteDifferences.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */
#ifndef CIRCULATION_FINITEDIFFERENCES_H
#define CIRCULATION_FINITEDIFFERENCES_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpCuda.h>
//--------------------

/**
 * @brief calculates a derivative using the 2nd order central finite difference
 *          see eg: https://www.mathematik.uni-dortmund.de/~kuzmin/cfdintro/lecture4.pdf
 * @param left value to the left/backward/down of the spot where the derivative is calculated
 * @param right value to the right/forward/up of the spot where the derivative is calculated
 * @param delta distance in space between the locations where left and write are taken from
 * @return the derivative at the location in the middle between left and right
 */
CUDAHOSTDEV inline float centralDeriv(float left, float right, float delta)
{
    return (right-left) / delta;
}

/**
 * @brief calculates the second derivative using the 2nd order central finite difference
 *          see eg: https://www.mathematik.uni-dortmund.de/~kuzmin/cfdintro/lecture4.pdf
 * @param left value to the left/backward/down of the spot where the derivative is calculated
 * @param right value to the right/forward/up of the spot where the derivative is calculated
 * @param center value at the spot where the derivative is calculated
 * @param delta distance in space between the locations where left and write are taken from
 * @return the second derivative at the location center, in the middle between left and right
 */
CUDAHOSTDEV inline float central2ndDeriv(float left, float center, float right, float delta)
{
    return (right - 2*center + left) / (delta*delta);
}


#endif //CIRCULATION_FINITEDIFFERENCES_H
