/*
 * CIRCULATION
 * ShallowWaterModel.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the ShallowWaterModel class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "CosineAdvection.h"

#include <mpUtils/mpUtils.h>
#include <mpUtils/mpGraphics.h>
#include <mpUtils/mpCuda.h>

#include "../GridReference.h"
#include "../finiteDifferences.h"
#include "../boundaryConditions.h"
#include "../interpolation.h"
//--------------------

// function definitions of the ShallowWaterModel class
//-------------------------------------------------------------------

void CosineAdvection::showCreationOptions()
{
    ImGui::Text("Test Case number 1 from David L. Williamson 1992.");
    ImGui::DragFloat("Earth radius (a) in m", &m_earthRadiusSI);
    ImGui::DragFloat("Angular Velocity in rad/m", &m_angularVelocitySI, 0.00001f, 0.00001f, 5.0f, "%.8f");
    ImGui::DragFloat("Internal time unit in s", &m_timeUnit, 0.1f, 1.0);
    ImGui::DragFloat("Wind Angle offset (alpha) in rad", &m_alpha, 0.001f,0.0,M_PI_2);
    ImGui::DragFloat("Wind Velocity (u0) in m/s", &m_u0SI, 0.001f);
    ImGui::DragFloat2("position of cosine bell", &m_cosineBellCenter.x, 0.001);
    ImGui::DragFloat("cosine bell radius (R) in m", &m_cosineBellRadiusSI, 1.0f);
}

void CosineAdvection::showBoundaryOptions(const CoordinateSystem& cs)
{
}

void CosineAdvection::showSimulationOptions()
{
    if(ImGui::DragFloat("Wind Angle offset (alpha) in rad", &m_alpha, 0.001f,0.0,M_PI_2))
        buildWindField();

    if(ImGui::DragFloat("Wind Velocity (u0) in m/s", &m_u0SI, 0.001f))
        buildWindField();

    if(ImGui::DragFloat("Angular Velocity", &m_angularVelocitySI, 0.00001f, 0.00001, 5.0f, "%.5f"))
    {
        m_angularVelocity = m_angularVelocitySI * m_timeUnit;
    }

    ImGui::Checkbox("Use Leapfrog",&m_useLeapfrog);
    ImGui::DragFloat("Timestep",&m_timestep,0.000001,0.000001f,1.0,"%.6f");
    ImGui::Text("Simulated Time units: %f", m_totalSimulatedTime);
}

std::shared_ptr<GridBase> CosineAdvection::recreate(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;
    m_grid = std::make_shared<ShallowWaterGrid>(m_cs->getNumGridCells());

    if(m_cs->getType() != CSType::geographical2d)
    {
        logERROR("CosineAdvection") << "Pole advection test loaded in cartesian cordinates!";
        tinyfd_messageBox("Error","Advection Test only works for geographical coordinates",
                          "ok", "error",1);
        return m_grid;
    }

    // scale units
    m_lengthUnit = m_earthRadiusSI / m_cs->getMinCoord().z;
    logINFO("PoleAdvectionTest") << "Internal length unit: " << m_lengthUnit << " meter";
    logINFO("PoleAdvectionTest") << "Internal time unit: " << m_timeUnit << " seconds";

    m_earthRadius = m_earthRadiusSI / m_lengthUnit;
    m_u0 = m_u0SI / m_lengthUnit * m_timeUnit;
    m_angularVelocity = m_angularVelocitySI * m_timeUnit;
    m_cosineBellRadius = m_cosineBellRadiusSI / m_lengthUnit;
    m_h0 = m_h0SI / m_lengthUnit;
    m_g = m_gSI / m_lengthUnit * m_timeUnit * m_timeUnit;

    logINFO("PoleAdvectionTest") << "Settings in internal units: earth radius: " << m_earthRadius << ", u0: " << m_u0
                                 << ", angular velocity: " << m_angularVelocity
                                 << ", cosine bell radius: " << m_cosineBellRadius
                                 << ", cosine bell heigt: " << m_h0
                                 << ", g: " << m_g
                                 ;
    buildWindField();
    reset();
    return m_grid;
}

void CosineAdvection::buildWindField()
{
    m_grid->cacheOnHost();

    float cosAlpha = cos(m_alpha);
    float sinAlpha = sin(m_alpha);

    float sinLatCenter = sin(m_cosineBellCenter.y);
    float cosLatCenter = cos(m_cosineBellCenter.y);

    // create solid body rotation velocity field
    for(int i : mpu::Range<int>(m_grid->size()))
    {
        float3 cv = m_cs->getCellCoordinate(i) + m_cs->getCellSize()*0.5f;

        float velX = m_u0*( cos(cv.y)*cosAlpha + sin(cv.y)*cos(cv.x)*sinAlpha);
        float velY = -m_u0*sin(cv.x)*sinAlpha;

        m_grid->initialize<AT::velocityX>(i, velX);
        m_grid->initialize<AT::velocityY>(i, velY);
    }
    m_grid->pushCachToDevice();

    // swap buffers and ready for rendering
    m_grid->swapAndRender();
}

void CosineAdvection::reset()
{
    m_grid->cacheOnHost();

    std::default_random_engine rng(mpu::getRanndomSeed());
    std::uniform_real_distribution<float> dist(0.9,1.1);

    float cosAlpha = cos(m_alpha);
    float sinAlpha = sin(m_alpha);

    float sinLatCenter = sin(m_cosineBellCenter.y);
    float cosLatCenter = cos(m_cosineBellCenter.y);

    // create initial conditions using cosine bell
    for(int i : mpu::Range<int>(m_grid->size()))
    {
        float3 cp = m_cs->getCellCoordinate(i);
        float geopotential = 0;
        float r = m_earthRadius * acos( sinLatCenter*sin(cp.y) + cosLatCenter*cos(cp.y)*cos(cp.x - m_cosineBellCenter.x));
        if(r < m_cosineBellRadius)
        {
            float h = dist(rng) * (m_h0/2.0f) * (1.0f + cos( M_PI * r / m_cosineBellRadius ));
            geopotential = m_g*h;
        }

        m_grid->initialize<AT::geopotential>(i, geopotential);
    }
    m_grid->pushCachToDevice();

    // swap buffers and ready for rendering
    m_grid->swapAndRender();

    // reset simulation state
    m_totalSimulatedTime = 0.0f;
    m_firstTimestep = true;
}

std::unique_ptr<Simulation> CosineAdvection::clone() const
{
    return std::make_unique<CosineAdvection>(*this);
}

void CosineAdvection::simulateOnce()
{
    if(m_cs->getType() != CSType::geographical2d)
        return;

    simulateOnceImpl(static_cast<GeographicalCoordinates2D&>( *(this->m_cs)));
}

__global__ void poleAdvectionA(ShallowWaterGrid::ReferenceType grid, GeographicalCoordinates2D coordinateSystem,
                               float timestep, bool useLeapfrog, float angularVelocity)
{
    GeographicalCoordinates2D cs = coordinateSystem;

    // updates geopotential for all non boundary cells
    // also calculates kinetic energy per unit mass
    for(int x : mpu::gridStrideRange( 0, cs.getNumGridCells3d().x))
        for(int y : mpu::gridStrideRangeY( 0, cs.getNumGridCells3d().y))
        {
            int3 cell{x,y,0};
            int cellId = cs.getCellId(cell);
            float2 cellPos = make_float2( cs.getCellCoordinate3d(cell) );

            // read values of quantities
            const float phi = grid.read<AT::geopotential>(cellId);
            const float phiLeft = grid.read<AT::geopotential>(cs.getLeftNeighbor(cellId));
            const float phiRight = grid.read<AT::geopotential>(cs.getRightNeighbor(cellId));

            const float velLeftX  = grid.read<AT::velocityX>(cs.getLeftNeighbor(cellId));
            const float velRightX = grid.read<AT::velocityX>(cellId);
            float velBackY;
            float velForY;

            // compute phi half, meaning phi half way between cells
            const float phiHalfLeft = 0.5f*(phi+phiLeft);
            const float phiHalfRight = 0.5f*(phi+phiRight);
            float phiHalfBack;
            float phiHalfFor;

            // handle lower boundary
            if(cell.y == 0)
            {
                velBackY = 0;
                phiHalfBack = 0;
                continue; // we cant handle this case right now
            } else {
                float phiBack = grid.read<AT::geopotential>(cs.getBackwardNeighbor(cellId));
                velBackY  = grid.read<AT::velocityY>(cs.getBackwardNeighbor(cellId));
                phiHalfBack = 0.5f*(phi+phiBack);
            }

//            bool print = cell.x == 256 || cell.x == 0;
//            bool print = cell.x == 384 || cell.x == 128;
//            bool print = cell.x == 127 || cell.x == 128 || cell.x == 129 || cell.x == 384;
            // handle upper boundary
            if(cell.y == cs.getNumGridCells3d().y-1)
            {
//                continue; // we cant handle this case right now

                // find where we need phiHalfFor in extended coordinates
                float2 neighborPosExt = cellPos;
                neighborPosExt.y += cs.getCellSize().y*0.5f;

                // get the same in position in normal coordinates by wrapping around the pole
                float2 neighborPos = neighborPosExt;
                if(neighborPos.y > M_PIf32*0.5)
                {
                    neighborPos.y = M_PIf32 - neighborPos.y;
                    neighborPos.x = fmod(neighborPos.x+M_PIf32,2*M_PIf32);
                }

//                if(print) printf("cell %i pos: (%f,%f), nbPosExt: (%f,%f), nbPos: (%f,%f)\n",cell.x, cellPos.x, cellPos.y,
//                        neighborPosExt.x, neighborPosExt.y, neighborPos.x, neighborPos.y);

                // if we are in the polar region (outside of the coordinate bounds) special care must be taken
                if( neighborPos.y > cs.getMaxCoord().y)
                {
//                    if(print) printf("cell %i geopotential using interpolatePole\n",cell.x);
                    phiHalfFor = interpolateNorthPole2D<AT::geopotential>(cellPos,phi,neighborPosExt.y,grid,cs,float2{0,0},false);
//                    if(print && phiHalfFor < 0)
//                        printf("cell %i has negative neighbor\n",cell.x);
                } else
                {
//                    if(print) printf("cell %i geopotential using interpolate2d\n",cell.x);
                    phiHalfFor = readInterpolated2D<AT::geopotential>(neighborPos,grid,cs);
                }

                // velocity X has an offset on the c grid, but we need to load it at the same position as phi
                float2 offset{0,cs.getCellSize().y*0.5f};
                if( neighborPos.y > cs.getMaxCoord().y-offset.y)
                {
//                    if(print) printf("cell %i velocity using interpolatePole\n",cell.x);
                    velForY = interpolateNorthPole2D<AT::velocityY>(cellPos-offset, velBackY,neighborPosExt.y,grid,cs,offset,true);
                } else
                {
//                    if(print) printf("cell %i velocity using interpolate2D\n",cell.x);
                    velForY = -readInterpolated2D<AT::velocityY>(neighborPos,grid,cs,offset);
                }

//                if(print) printf("cell %i geo: %f geoBack: %f, geoFor %f\n",cell.x,phi,phiHalfBack,phiHalfFor);
//                if(print) printf("cell %i velBack: %f, velFor %f\n",cell.x,velBackY,velForY);

            } else {
                float phiFor = grid.read<AT::geopotential>(cs.getForwardNeighbor(cellId));
                velForY = grid.read<AT::velocityY>(cellId);
                phiHalfFor = 0.5f*(phi+phiFor);
            }

//            const float velForX  = grid.read<AT::velocityX>(cs.getForwardNeighbor(cellId)); // used for vorticity
//            const float velRightY  = grid.read<AT::velocityY>(cs.getRightNeighbor(cellId)); // used for vorticity

            // calculate vorticity and coriolis parameter
            // if this looks strange consider where values are located on the C grid
//            const float2 vortPos = cellPos + 0.5f * make_float2(cs.getCellSize()); // position where vorticity is computed
//            const float vort = curl2d(velForY, velRightY, velRightX, velForX, vortPos, cs);
//            float cor;
//            if(cs.getType() == CSType::geographical2d)
//                cor = 2*corOrAngvel*sin(vortPos.y);
//            else if(cs.getType() == CSType::cartesian2d)
//                cor = corOrAngvel;
//            else
//                cor = 0.0f;
//            vortPlusCor[cellId] = vort + cor;
//
//            // write potential vorticity
//            grid.write<AT::potentialVort>(cellId, abs(vort+cor) / phi);

            // compute geopotential advection time derivative dPhi/dt
            float dphi_dt = -divergence2d( velLeftX*phiHalfLeft, velRightX*phiHalfRight, velBackY*phiHalfBack, velForY*phiHalfFor, cellPos, cs);

            // compute values at t+1
            float nextPhi;
            if(useLeapfrog)
            {
                const float prevPhi = grid.readPrev<AT::geopotential>(cellId);
                nextPhi = prevPhi + dphi_dt * 2.0f*timestep;
            }
            else
                nextPhi = phi + dphi_dt * timestep;

            grid.write<AT::potentialVort>(cellId, dphi_dt);
            grid.write<AT::geopotential>(cellId, nextPhi);
        }
}

void CosineAdvection::simulateOnceImpl(GeographicalCoordinates2D& cs)
{
    dim3 blocksize{16,16,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().x ,blocksize.x)),
                    static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().y ,blocksize.y)), 1};

    poleAdvectionA<<< numBlocks, blocksize >>>(m_grid->getGridReference(),cs,m_timestep, !m_firstTimestep && m_useLeapfrog, m_angularVelocity);

    m_totalSimulatedTime += m_timestep;
    m_firstTimestep = false;
}

GridBase& CosineAdvection::getGrid()
{
    return *m_grid;
}

std::string CosineAdvection::getDisplayName()
{
    return "Shallow Water Model";
}