/*
 * CIRCULATION
 * icosphere.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */
#ifndef CIRCULATION_ICOSPHERE_H
#define CIRCULATION_ICOSPHERE_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpCuda.h>
#include "coordinateTransforms.h"
//--------------------

namespace icosphere {

/**
 * @brief number of cells in memory, including halo region
 */
CUDAHOSTDEV inline int memorySize(int n)
{
    return 10*(n+2)*(n+2);
}

/**
 * @brief id of a grid point in memory from its 3d id
 */
CUDAHOSTDEV inline int getPointId(const int3& pointId3d, int n)
{
    int N = n+2;
    return pointId3d.x * N*N + pointId3d.y*N + pointId3d.z;
}

/**
 * @brief 3d id of a grid point from its id in memory
 */
CUDAHOSTDEV inline int3 getPointId3d(int pointId, int n)
{
    int N = n+2;
    int3 id3d;
    id3d.x = pointId / (N*N);
    pointId -= id3d.x * (N*N);
    id3d.y = pointId / N;
    id3d.z = pointId % N;
    return id3d;
}

/**
 * @brief computes the spherical coordinates of the triangle corners of the original icosahedron this boint lives in
 */
CUDAHOSTDEV inline void getSurroundingTriangle(int3 pointId, float2& A, float2& B, float2& C)
{
    bool upperTri = pointId.y <= pointId.z;

    float lat = atan(0.5);
    float offset = glm::radians(72.0f);

    if(pointId.x<5)
    {
        if(upperTri)
        {
            A = float2{ 0, M_PI_2f32};
            B = float2{ pointId.x*offset, lat};
            C = float2{ ((pointId.x+1)%5)*offset, lat};
        } else {
            A = float2{ (0.5f+pointId.x)*offset, -lat};
            B = float2{ ((pointId.x+1)%5)*offset, lat};
            C = float2{ pointId.x*offset, lat};
        }
    } else {
        pointId.x -= 5;
        if(upperTri)
        {
            A = float2{ pointId.x*offset, lat};
            B = float2{ fmod(pointId.x-0.5f,5.0f)*offset, -lat};
            C = float2{ (0.5f+pointId.x)*offset, -lat};
        } else {
            A = float2{ 0, -M_PI_2f32};
            B = float2{ (0.5f+pointId.x)*offset, -lat};
            C = float2{ fmod(pointId.x-0.5f,5.0f)*offset, -lat};
        }
    }
}

inline void generateIcosphere(int n, std::vector<float2>& geoCoord, std::vector<float3>& cartCoord)
{
    // allocate some memory for positions
    geoCoord = std::vector<float2>(memorySize(n));
    cartCoord = std::vector<float3>(memorySize(n));

    // some constants used later
    const int N = n+2;
    const float lat = atan(0.5);
    const float offset = glm::radians(72.0f);

    // generate subdivided positions
    int3 pointId3d;
    for(pointId3d.x=0; pointId3d.x<10; pointId3d.x++)
        for(pointId3d.y=1; pointId3d.y<N; pointId3d.y++)
            for(pointId3d.z=1; pointId3d.z<N; pointId3d.z++)
            {
                float3 pGeo;
                float3 pCart;

                // compute the id of the diagonal
                int diagonalId = pointId3d.y - pointId3d.z;

                if(diagonalId >= 0)
                {
                    // this is the upper half of the thrombus
                    // find surrounding triangle in spherical coordinates
                    float3 A;
                    float3 B;
                    float3 C;
                    int thrombus = pointId3d.x;
                    if(thrombus<5)
                    {
                        A = float3{ 0, M_PI_2f32,1};
                        B = float3{ thrombus*offset, lat,1};
                        C = float3{ ((thrombus+1)%5)*offset, lat,1};
                    } else
                    {
                        thrombus -= 5;
                        A = float3{ thrombus*offset, lat,1};
                        B = float3{ fmod(thrombus-0.5f,5.0f)*offset, -lat,1};
                        C = float3{ (0.5f+thrombus)*offset, -lat,1};
                    }

                    // get the fraction we need from the AB and AC arc
                    float f1 = float(diagonalId)/float(n);

                    float3 pAB = fractionalPointOnArc(B,A,f1);
                    float3 pAC = fractionalPointOnArc(C,A,f1);

                    // get the fraction we need on the resulting arc
                    float f2 = float(pointId3d.y-1)/float(n-diagonalId);

                    // compute position
                    pGeo =  fractionalPointOnArc(pAB,pAC,f2);
                    pCart = geoToCartPoint(pGeo);

                } else {
                    // this is the lower half of the thrombus
                    // find surrounding triangle in spherical coordinates
                    float3 A;
                    float3 B;
                    float3 C;
                    int thrombus = pointId3d.x;
                    if(thrombus<5)
                    {
                        A = float3{ (0.5f+thrombus)*offset, -lat,1};
                        B = float3{ ((thrombus+1)%5)*offset, lat,1};
                        C = float3{ thrombus*offset, lat,1};
                    } else
                    {
                        thrombus -= 5;
                        A = float3{ 0, -M_PI_2f32,1};
                        B = float3{ (0.5f+thrombus)*offset, -lat,1};
                        C = float3{ fmod(thrombus-0.5f,5.0f)*offset, -lat,1};
                    }

                    diagonalId *= -1;

                    // get the fraction we need from the AB and AC arc
                    float f1 = float(diagonalId)/float(n);

                    float3 pAB = fractionalPointOnArc(B,A,f1);
                    float3 pAC = fractionalPointOnArc(C,A,f1);

                    // get the fraction we need on the resulting arc
                    float f2 = float(pointId3d.z-1)/float(n-diagonalId);

                    // compute position
                    pGeo =  fractionalPointOnArc(pAB,pAC,f2);
                    pCart = geoToCartPoint(pGeo);

                }
                int memId = getPointId(pointId3d, n);
                geoCoord[memId] = float2{pGeo.x,pGeo.y};
                cartCoord[memId] = pCart;
            }
}

inline void generateIcosphereIndices(int n, std::vector<int>& indices)
{
    indices.clear();

    const int N = n+2;

    int3 pointId3d;
    for(pointId3d.x=0; pointId3d.x<10; pointId3d.x++)
        for(pointId3d.y=2; pointId3d.y<N; pointId3d.y++)
            for(pointId3d.z=2; pointId3d.z<N; pointId3d.z++)
            {
                // lower triangle
                indices.push_back(getPointId({pointId3d.x, pointId3d.y-1, pointId3d.z-1}));
                indices.push_back(getPointId({pointId3d.x, pointId3d.y, pointId3d.z-1}));
                indices.push_back(getPointId({pointId3d.x, pointId3d.y, pointId3d.z}));

                // upper triangle
                indices.push_back(getPointId({pointId3d.x, pointId3d.y, pointId3d.z}));
                indices.push_back(getPointId({pointId3d.x, pointId3d.y-1, pointId3d.z}));
                indices.push_back(getPointId({pointId3d.x, pointId3d.y-1, pointId3d.z-1}));
            }
}

}

#endif //CIRCULATION_ICOSPHERE_H