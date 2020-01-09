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
    return (right - 2*center + left) / delta*delta;
}

/**
 * @brief calculates the gradient of a 2d scalar field using 2nd order central differences
 * @param left the value left (-X axis) of the location where the gradient is computed
 * @param right the calue right (+X axis) of the location where the gradient is computed
 * @param backward the value backward (-Y axis) of the location where the gradient is computed
 * @param forward the value forward (+Y axis) of the location where the gradient is computed
 * @param delta distance between the locations where left - right (x-component) and backward - forward (y-component) where taken (z is ignored)
 * @return the gradient calculate at the center between left right backward and forward
 */
CUDAHOSTDEV inline float2 gradient2d(float left, float right, float backward, float forward, const float3& delta)
{
    return make_float2( centralDeriv(left,right,delta.x), centralDeriv(backward,forward,delta.y) );
}

/**
 * @brief calculates the divergence of a 2d vector field
 * @param leftX the X component of the vector taken to the left (-X axis) of the location where the divergence is computed
 * @param rightX the X component of the vector taken to the right (+X axis) of the location where the divergence is computed
 * @param downY the Y component of the vector taken backward (-Y axis) of the location where the divergence is computed
 * @param upY the Y component of the vector taken forward (+Y axis) of the location where the divergence is computed
 * @param delta distance between the locations where left - right (x-component) and backward - forward (y-component) where taken (z is ignored)
 * @return the divergence calculate at the center between left, right, backward and forward
 */
CUDAHOSTDEV inline float divergence2d(float leftX, float rightX, float backwardY, float forwardY, const float3& delta)
{
    return centralDeriv(leftX,rightX,delta.x) + centralDeriv(backwardY,forwardY,delta.y);
}

/**
 * @brief calculates the curl of a 2d vector field
 * @param leftY the Y component of the vector field taken to the left (-X axis) of the location where the curl is computed
 * @param rightY the Y component of the vector field taken to the right (+X axis) of the location where the curl is computed
 * @param downX the X component of the vector taken backward (-Y axis) of the location where the curl is computed
 * @param upX the X component of the vector taken forward (+Y axis) of the location where the curl is computed
 * @param delta distance between the locations where left - right (x-component) and backward - forward (y-component) where taken (z is ignored)
 * @return the curl calculated in the center of left, right, backward and forward
 */
CUDAHOSTDEV inline float curl2d(float leftY, float rightY, float backwardX, float forwardX, const float3& delta)
{
    return centralDeriv(leftY,rightY,delta.x) - centralDeriv(backwardX,forwardX,delta.y);
}

/**
 * @brief calculates the laplace operator on a 2d scalar field
 * @param left the value left (-X axis) of the location where the laplace operator is computed
 * @param right the value right (+X axis) of the location where the laplace operator is computed
 * @param backward the value backward (-Y axis) of the location where the laplace operator is computed
 * @param forward the value forward (+Y axis) of the location where the laplace operator is computed
 * @param center the value at the location where the laplace operator is computed
 * @param delta distance between the locations where left - right (x-component) and backward - forward (y-component) where taken (z is ignored)
 * @return the laplace operator calculated at the location where center was taken, in the center of left, right, backward and forward
 */
CUDAHOSTDEV inline float laplace2d(float left, float right, float backward, float forward, float center, const float3& delta)
{
    return central2ndDeriv(left,center,right,delta.x) + central2ndDeriv(backward,center,forward,delta.y);
}

#endif //CIRCULATION_FINITEDIFFERENCES_H
