/*
 * CIRCULATION
 * boundaryConditions.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */
#ifndef CIRCULATION_BOUNDARYCONDITIONS_H
#define CIRCULATION_BOUNDARYCONDITIONS_H

// includes
//--------------------
#include "enums.h"
#include "Grid.h"
//--------------------

/**
 * @brief initializes the boundaries on the grid to fixed scalar values
 */
template < AT attributeType, typename T, typename csT, typename gridT>
void initializeFixedValueBoundaries(bool boundX, bool boundY, const T& valueX, const T& valueY, const csT& cs, gridT& grid)
{
    if(boundY)
    {
        int numBoundCellsY = 2 * cs.hasBoundary().y * cs.getNumGridCells3d().x;
        for(int i : mpu::Range<int>(numBoundCellsY))
        {
            // transform boundary cell id into actual cell id
            int3 cellId3d{i % cs.getNumGridCells3d().x, 0, 0};
            if(i >= cs.getNumGridCells3d().x)
                cellId3d.y = cs.getNumGridCells3d().y - 1;
            int cellId = cs.getCellId(cellId3d);

            grid.template initialize<attributeType>(cellId, valueX);
        }
    }

    if(boundX)
    {
        int numBoundCellsX = 2 * cs.hasBoundary().x * cs.getNumGridCells3d().y - 4;
        for(int i : mpu::Range<int>(numBoundCellsX))
        {
            // transform boundary cell id into actual cell id
            int3 cellId3d{(i % 2) * (cs.getNumGridCells3d().x - 1), 1 + i / 2, 0};
            int cellId = cs.getCellId(cellId3d);

            grid.template initialize<attributeType>(cellId, valueY);
        }
    }
}

template < AT attributeType, typename csT, typename gridRefT>
__global__ void mirrorBoundariesGPU(int numBoundCellsY, int  numBoundCellsX, csT coordinateSystem, gridRefT grid, bool isOffset)
{
    csT cs = coordinateSystem;
    int offset = isOffset ? 2 : 1;

    for(int i : mpu::gridStrideRange(numBoundCellsY))
    {
        // transform boundary cell id into actual cell id
        int3 cellId3d{i % cs.getNumGridCells3d().x, 0, 0};
        if(i >= cs.getNumGridCells3d().x)
            cellId3d.y = cs.getNumGridCells3d().y - offset;
        int cellId = cs.getCellId(cellId3d);

        int neighbourId = (cellId3d.y == 0) ? cs.getForwardNeighbor(cellId) : cs.getBackwardNeighbor(cellId);

        auto value = grid.template read<attributeType>(neighbourId);
        grid.template write<attributeType>(cellId,value);
    }

    for(int i : mpu::gridStrideRange(numBoundCellsX))
    {
        // transform boundary cell id into actual cell id
        int3 cellId3d{(i % 2) * (cs.getNumGridCells3d().x - offset), 1 + i / 2, 0};
        int cellId = cs.getCellId(cellId3d);

        int neighbourId = (cellId3d.x == 0) ? cs.getRightNeighbor(cellId) : cs.getLeftNeighbor(cellId);

        auto value = grid.template read<attributeType>(neighbourId);
        grid.template write<attributeType>(cellId, value);
    }
}

/**
 * @brief update the boundaries on the grid to mirror the closest valid value
 */
template < AT attributeType, typename csT, typename gridT>
void handleMirroredBoundaries(bool boundX, bool boundY, csT& cs, gridT& grid, bool isOffset = false)
{
    int numBoundCellsY = boundY ? 2 * cs.hasBoundary().y * cs.getNumGridCells3d().x : 0;
    int numBoundCellsX = boundX ? 2 * cs.hasBoundary().x * cs.getNumGridCells3d().y - 4 : 0;

    dim3 blocksize{128,1,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( min(numBoundCellsX,numBoundCellsY) ,blocksize.x)),
                    1, 1};

    mirrorBoundariesGPU<attributeType><<<numBlocks, blocksize>>>(numBoundCellsY,numBoundCellsX,cs,grid.getGridReference(), isOffset);
}

#endif //CIRCULATION_BOUNDARYCONDITIONS_H
