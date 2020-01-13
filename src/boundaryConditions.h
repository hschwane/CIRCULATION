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

/**
 * @brief update the boundaries on the grid to mirror the closest valid value
 */
template < AT attributeType, typename csT, typename gridT>
void handleMirroredBoundaries(bool boundX, bool boundY, const csT& cs, gridT& grid)
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

            int neighbourId = (cellId3d.y == 0) ? cs.getForwardNeighbor(cellId) : cs.getBackwardNeighbor(cellId);

            auto value = grid.template read<attributeType>(neighbourId);
            grid.template initialize<attributeType>(cellId,value);
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

            int neighbourId = (cellId3d.x == 0) ? cs.getRightNeighbor(cellId) : cs.getLeftNeighbor(cellId);

            auto value = grid.template read<attributeType>(neighbourId);
            grid.template initialize<attributeType>(cellId,value);
        }
    }
}

#endif //CIRCULATION_BOUNDARYCONDITIONS_H
