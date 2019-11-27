/*
 * CIRCULATION
 * CoordinateSystem.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the CoordinateSystem class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_COORDINATESYSTEM_H
#define CIRCULATION_COORDINATESYSTEM_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpCuda.h>
//--------------------

//-------------------------------------------------------------------
/**
 * class CoordinateSystem
 *
 * usage:
 * Base class for all coordinate systems. Provides coordinate conversions to/from cartesian coordinates and access to grid cells and face.
 * Also provides information about adjacent cells and faces.
 *
 */
class CoordinateSystem
{
public:

    virtual ~CoordinateSystem() = default;

    // convert
    virtual float3 getCartesian(float3 coord)=0; //!< converts a coordinate into cartesian coordinates
    virtual float3 getCoord(float3 cartesian)=0; //!< converts cartesian coordinate into this coordinate system

    // for quantities stored at cell center
    virtual float3 getCellCoordinate(int cellId)=0; //!< get the coordinates of a specific cell
    virtual int getCellId(float3 coord)=0; //!< get the the cell id that belongs coordinates "coord"

    // for quantities stored at cel faces
    virtual float3 getFaceCoordinate(int faceId)=0; //!< get the coordinate of a cell face
    virtual int getFaceId(float3 coord)=0; //!< get the cell face closest to coordinates "coord"

    // adjacency
    virtual int getRightFace(int cellId, int dimension)=0; //!< get face ids for a given cell id along axis dimension
    virtual int getLeftFace(int cellId, int dimension)=0; //!< get face ids for a given cell id along axis dimension
    virtual void getFaces(int cellId, int dimension, int& faceId1, int& faceId2)=0; //!< get face ids for a given cell id along axis dimension
    virtual int getRightNeighbor(int cellId, int dimension)=0; //!< get neighbors for given cell along axis
    virtual int getLightNeighbor(int cellId, int dimension)=0; //!< get neighbors for given cell along axis
    virtual void getNeighbors(int cellId, int dimension, int& cellId1, int& cellId2)=0; //!< get neighbors for given cell along axis

    // boundaries
    virtual float3 getMinCoord()=0; //!< get the lower bound for all dimensions
    virtual float3 getMaxCoord()=0; //!< get the upper bound for all dimensions
    virtual float3 getDimension()=0; //!< get the number of dimensions
    virtual float3 getCartesianDimension()=0; //!< get the number of dimensions in cartesian coordinates (eg surface of sphere dim=2 cartesian_dim = 3)

    // bounding box
    virtual float3 getAABBMin()=0; //!< get the lower left  bounding box corner in cartesian coords
    virtual float3 getAABBMax()=0; //!< get the upper right bounding box corner in cartesian coords
};


#endif //CIRCULATION_COORDINATESYSTEM_H
