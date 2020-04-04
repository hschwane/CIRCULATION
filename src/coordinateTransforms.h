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

#endif //CIRCULATION_COORDINATETRANSFORMS_H
