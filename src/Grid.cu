/*
 * CIRCULATION
 * Grid.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the Grid class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "Grid.h"
//--------------------

// template instantiations for faster compiling
//-------------------------------------------------------------------
template class Grid<GridDensity,GridVelocityX,GridVelocityY>;
template class Grid<GridDensity, GridVelocityX, GridVelocityY, GridDensityGradX, GridDensityGradY, GridDensityLaplace, GridVelocityDiv, GridVelocityCurl, GridTemperature, GridTemperatureGradX, GridTemperatureGradY>;
template class Grid<GridVelocityX,GridVelocityY,GridGeopotential,GridPotentialVort>;
