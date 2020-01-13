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
#include "coordinateSystems/GeographicalCoordinates2D.h"
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

/**
 * @brief calculates the gradient of a 2d scalar field using 2nd order central differences
 * @param left the value left (-X axis) of the location where the gradient is computed
 * @param right the calue right (+X axis) of the location where the gradient is computed
 * @param backward the value backward (-Y axis) of the location where the gradient is computed
 * @param forward the value forward (+Y axis) of the location where the gradient is computed
 * @param location the location where the gradient should be taken (the center between left, right, backward and forward)
 * @param cs the coordinate system to be used when calculating the gradient
 * @return the gradient calculate at the center between left, right, backward and forward
 */
template <typename csT>
CUDAHOSTDEV inline float2 gradient2d(float left, float right, float backward, float forward, const float2& location, const csT& cs)
{
    static_assert(csT::isCartesian, "This overload only works for cartesian coordinates.");
    return make_float2( centralDeriv(left,right,cs.getCellSize().x), centralDeriv(backward,forward,cs.getCellSize().y) );
}

template <>
CUDAHOSTDEV inline float2 gradient2d<GeographicalCoordinates2D>(float left, float right, float backward, float forward, const float2& location, const GeographicalCoordinates2D& cs)
{
    float rinv = 1.0f / cs.getMinCoord().z;
    return make_float2( rinv /cos(location.y) * centralDeriv(left,right,cs.getCellSize().x), rinv * centralDeriv(backward,forward,cs.getCellSize().y) );
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
template <typename csT>
CUDAHOSTDEV inline float divergence2d(float leftX, float rightX, float backwardY, float forwardY, const float2& location, const csT& cs)
{
    static_assert(csT::isCartesian, "This overload only works for cartesian coordinates.");
    return centralDeriv(leftX,rightX,cs.getCellSize().x) + centralDeriv(backwardY,forwardY,cs.getCellSize().y);
}

template <>
CUDAHOSTDEV inline float divergence2d<GeographicalCoordinates2D>(float leftX, float rightX, float backwardY, float forwardY, const float2& location, const GeographicalCoordinates2D& cs)
{
    float rSinePhiInv = 1.0f / ( cs.getMinCoord().z * cos(location.y)); // remember location.y is not phi but phi = pi/2 - location.y

    float locationBackward = location.y - cs.getCellSize().y*0.5f;
    float locationForward = location.y + cs.getCellSize().y*0.5f;

    return rSinePhiInv * ( centralDeriv(leftX,rightX,cs.getCellSize().x) + centralDeriv( cos(locationBackward) * backwardY, cos(locationForward) * forwardY,cs.getCellSize().y) );
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
template <typename csT>
CUDAHOSTDEV inline float curl2d(float leftY, float rightY, float backwardX, float forwardX, const float2& location, const csT& cs)
{
    static_assert(csT::isCartesian, "This overload only works for cartesian coordinates.");
    return centralDeriv(leftY,rightY,cs.getCellSize().x) - centralDeriv(backwardX,forwardX,cs.getCellSize().y);
}

template <>
CUDAHOSTDEV inline float curl2d<GeographicalCoordinates2D>(float leftY, float rightY, float backwardX, float forwardX, const float2& location, const GeographicalCoordinates2D& cs)
{
    float rSinePhiInv = 1.0f / ( cs.getMinCoord().z * cos(location.y)); // remember location.y is not phi but phi = pi/2 - location.y

    float locationBackward = location.y - cs.getCellSize().y*0.5f;
    float locationForward = location.y + cs.getCellSize().y*0.5f;

    // there seems to be a typo on the wolfram math side where a -1 is missing
    return rSinePhiInv * ( centralDeriv(leftY,rightY,cs.getCellSize().x) - centralDeriv( cos(locationBackward) * backwardX, cos(locationForward) * forwardX,cs.getCellSize().y) );
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
template <typename csT>
CUDAHOSTDEV inline float laplace2d(float left, float right, float backward, float forward, float center, const float2& location, const csT& cs)
{
    static_assert(csT::isCartesian, "This overload only works for cartesian coordinates.");
    return central2ndDeriv(left,center,right,cs.getCellSize().x) + central2ndDeriv(backward,center,forward,cs.getCellSize().y);
}

template <>
CUDAHOSTDEV inline float laplace2d<GeographicalCoordinates2D>(float left, float right, float backward, float forward, float center, const float2& location, const GeographicalCoordinates2D& cs)
{
    float sinPhiInv = 1.0f/cos(location.y); // location y is (pi/2-phi)
    float cosPhi = sin(location.y);
    float rinv2 = 1.0f / (cs.getMinCoord().z * cs.getMinCoord().z);

    return rinv2*sinPhiInv*sinPhiInv * central2ndDeriv(left,center,right,cs.getCellSize().x) + cosPhi*rinv2*sinPhiInv * centralDeriv(backward,forward,2*cs.getCellSize().y) + rinv2 * central2ndDeriv(backward,center,forward,cs.getCellSize().y);
}

#endif //CIRCULATION_FINITEDIFFERENCES_H
