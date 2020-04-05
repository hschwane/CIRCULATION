/*
 * CIRCULATION
 * coordinateTransforms.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */
#ifndef CIRCULATION_COORDINATETRANSFORMS_H
#define CIRCULATION_COORDINATETRANSFORMS_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpCuda.h>
//--------------------

/**
 * @brief convert between coordinate systems X is longitude 0<long<2pi, Y is latitude -pi/2 < lat < pi/2, Z is radius
 */
template <typename T>
T geoToCartPoint(const T& spherical)
{
    float sinPhi = cos(spherical.y);
    return T{spherical.z * cos(spherical.x) * sinPhi, spherical.z * sin(spherical.x) * sinPhi, spherical.z * sin(spherical.y)};
}

/**
 * @brief convert between coordinate systems X is longitude 0<long<2pi, Y is latitude -pi/2 < lat < pi/2, Z is radius
 */
template <typename T>
T cartToGeoPoint(const T& cartesian)
{
    float r = sqrt( cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z );
    float phi = acos(cartesian.z / r);
    return T{  atan2(cartesian.y,cartesian.x), M_PI_2f32 - phi, r};
}

/**
 * @brief compute the great circle arclength between point A and B
 */
 template <typename T>
 float greatCircleDistance(const T& A, const T& B)
{
     return acos( sin(A.y) * sin(B.y) + cos(A.y)*cos(B.y)*cos(fabs(B.x-A.x)) );
}

/**
 * @brief compute the position in geographical coordinates at a fraction of f along the great-circle-arc between A and B
 *          X is longitude 0<long<2pi, Y is latitude -pi/2 < lat < pi/2
 *          points can not be directly opposite of each other!
 *          http://www.movable-type.co.uk/scripts/latlong.html
 */
template <typename T>
T fractionalPointOnArc(const T& A, const T& B, float f)
{
    assert_true(A.z == B.z, "CoordinateTransformation", "cannot find fractional point for points with different radii");
    float d = greatCircleDistance(A,B);
    float a = sin((1.0f-f) *d) /sin(d);

    if(isnan(a))
        return A;

    float b = sin(f*d) /sin(d);
    float x = a*cos(A.y)*cos(A.x) + b*cos(B.y)*cos(B.x);
    float y = a*cos(A.y)*sin(A.x) + b*cos(B.y)*sin(B.x);
    float z = a*sin(A.y) + b*sin(B.y);

    return {atan2(y,x),atan2(z,sqrt(x*x+y*y)),A.z};
}

#endif //CIRCULATION_COORDINATETRANSFORMS_H
