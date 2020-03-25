/*
 * CIRCULATION
 * enums.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */
#ifndef CIRCULATION_ENUMS_H
#define CIRCULATION_ENUMS_H

/**
 * Types of simulation model available
 */
enum class SimModel : int
{
    renderDemo = 0,
    testSimulation = 1,
    shallowWaterModel = 2,
    poleAdvectionTest = 3
};

/**
 * Types of coordinate systems available
 */
enum class CSType : int
{
    cartesian2d = 0,
    geographical2d = 1
};


#endif //CIRCULATION_ENUMS_H
