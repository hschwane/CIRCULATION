/*
 * CIRCULATION
 * TestSimulation.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the TestSimulation class
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "TestSimulation.h"
#include "../GridReference.h"
#include "../coordinateSystems/CartesianCoordinates2D.h"
#include "../coordinateSystems/GeographicalCoordinates2D.h"
#include "../finiteDifferences.h"
#include "../boundaryConditions.h"
//--------------------

// function definitions of the TestSimulation class
//-------------------------------------------------------------------

void TestSimulation::showCreationOptions()
{
    ImGui::Checkbox("Random velocity vectors", &m_randomVectors);
    if(!m_randomVectors)
        ImGui::DragFloat2("Vector", &m_vectorValue.x);
}

void TestSimulation::showBoundaryOptions(const CoordinateSystem& cs)
{
    if(cs.hasBoundary().x)
    {
        ImGui::Text("X-Axis Boundary:");
        if(ImGui::RadioButton("Isolated##X",m_boundaryIsolatedX))
            m_boundaryIsolatedX = true;

        ImGui::SameLine();
        if(ImGui::RadioButton("Const. temperature##X", !m_boundaryIsolatedX))
        {
            m_boundaryIsolatedX = false;
            m_needUpdateBoundaries = true;
        }

        if(!m_boundaryIsolatedX)
            if(ImGui::DragFloat("Temperature on boundary##X", &m_boundaryTemperatureX, 0.1))
                m_needUpdateBoundaries = true;
    }

    if(cs.hasBoundary().y)
    {
        ImGui::Text("Y-Axis Boundary:");
        if(ImGui::RadioButton("Isolated##Y",m_boundaryIsolatedY))
            m_boundaryIsolatedY = true;

        ImGui::SameLine();
        if(ImGui::RadioButton("Const. temperature##Y", !m_boundaryIsolatedY))
        {
            m_boundaryIsolatedY = false;
            m_needUpdateBoundaries = true;
        }

        if(!m_boundaryIsolatedY)
            if(ImGui::DragFloat("Temperature on boundary##Y", &m_boundaryTemperatureY, 0.1))
                m_needUpdateBoundaries = true;
    }
}

void TestSimulation::showSimulationOptions()
{
    ImGui::Checkbox("diffuse heat",&m_diffuseHeat);
    ImGui::Checkbox("use divergence of gradient instead of laplacian",&m_useDivOfGrad);
    ImGui::Checkbox("use leapfrog (unstable)",&m_leapfrogIntegrattion);
    ImGui::Checkbox("advect heat",&m_advectHeat);
    ImGui::DragFloat("Heat Coefficient",&m_heatCoefficient,0.0001,0.0001f,1.0,"%.4f");
    ImGui::DragFloat("Timestep",&m_timestep,0.0001,0.0001f,1.0,"%.4f");
    ImGui::Text("Biggest maybe stable timestep is %f.",
                (fmin(m_cs->getCellSize().x,m_cs->getCellSize().y) * fmin(m_cs->getCellSize().x,m_cs->getCellSize().y) / (2*m_heatCoefficient) ) );
    ImGui::Text("Simulated Time units: %f", m_totalSimulatedTime);

    if( ImGui::CollapsingHeader("Boundaries"))
        showBoundaryOptions(*m_cs);
}

std::shared_ptr<GridBase> TestSimulation::recreate(std::shared_ptr<CoordinateSystem> cs)
{
    m_cs = cs;
    m_grid = std::make_shared<TestSimGrid>(m_cs->getNumGridCells());
    m_offsettedCurl.resize(m_cs->getNumGridCells());

    // select coordinate system
    switch(m_cs->getType())
    {
        case CSType::cartesian2d:
            m_simOnceFunc = [this](){ this->simulateOnceImpl( static_cast<CartesianCoordinates2D&>( *(this->m_cs)) ); };
            break;
        case CSType::geographical2d:
            m_simOnceFunc = [this](){ this->simulateOnceImpl( static_cast<GeographicalCoordinates2D&>( *(this->m_cs)) ); };
            break;
    }

    reset();
    return m_grid;
}

void TestSimulation::reset()
{
    // generate some data
    std::default_random_engine rng(mpu::getRanndomSeed());
    std::normal_distribution<float> dist(10,4);
    std::normal_distribution<float> vdist(0,4);

    m_grid->cacheOverwrite();
    for(int i : mpu::Range<int>(m_grid->size()))
    {
        float density = fmax(0,dist(rng));
        float temperature = fmax(0,dist(rng));
        float velX = vdist(rng);
        float velY = vdist(rng);

        m_grid->initialize<AT::density>(i,density);
        m_grid->initialize<AT::temperature>(i,temperature);
        if(m_randomVectors)
        {
            m_grid->initialize<AT::velocityX>(i, velX);
            m_grid->initialize<AT::velocityY>(i, velY);
        }
        else {
            m_grid->initialize<AT::velocityX>(i, m_vectorValue.x);
            m_grid->initialize<AT::velocityY>(i, m_vectorValue.y);
        }
    }

    // initialize boundary
    initializeFixedValueBoundaries<AT::temperature>(!m_boundaryIsolatedX && m_cs->hasBoundary().x,
                                                    !m_boundaryIsolatedY && m_cs->hasBoundary().y,
                                                    m_boundaryTemperatureX, m_boundaryTemperatureY, *m_cs, *m_grid);

    // swap buffers and ready for rendering
    m_grid->pushCachToDevice();
    m_grid->swapAndRender();

    // reset simulation state
    m_totalSimulatedTime = 0;
    m_firstTimestep = true;
    m_needUpdateBoundaries = false;
}

std::unique_ptr<Simulation> TestSimulation::clone() const
{
    return std::make_unique<TestSimulation>(*this);
}

void TestSimulation::simulateOnce()
{
    m_simOnceFunc(); // calls correct template specialization
}

template <typename csT>
__global__ void testSimulationA(TestSimGrid::ReferenceType grid, csT coordinateSystem, mpu::VectorReference<float> offsettedCurl,
        bool diffuseHeat, bool advectHeat, float heatCoefficient, bool useDivOfGrad, float timestep)
{
    csT cs = coordinateSystem;

    for(int x : mpu::gridStrideRange( cs.hasBoundary().x, cs.getNumGridCells3d().x-cs.hasBoundary().x ))
        for(int y : mpu::gridStrideRangeY( cs.hasBoundary().y, cs.getNumGridCells3d().y-cs.hasBoundary().y ))
    {
        int3 cell{x,y,0};
        int cellId = cs.getCellId(cell);
        float2 cellPos = make_float2( cs.getCellCoordinate3d(cell) );

        // do bounds checking
        int3 leftNeigbour = cs.getCellId3d(cs.getRightNeighbor(cellId));
        int3 rightNeibor = cs.getCellId3d(cs.getLeftNeighbor(cellId));
        int3 backwardNeigbor = cs.getCellId3d(cs.getBackwardNeighbor(cellId));
        int3 forwardNeigbor = cs.getCellId3d(cs.getForwardNeighbor(cellId));

        auto oob = [&](int3 c)->bool
        {
            return (c.x >= cs.getNumGridCells3d().x) || (c.x < 0) || (c.y >= cs.getNumGridCells3d().y) || (c.y < 0);
        };

        if(oob(leftNeigbour))
            printf("Left neighbor out of bounds! cell (%i,%i) \n",x,y);
        if(oob(rightNeibor))
            printf("Right neighbor out of bounds! cell (%i,%i) \n",x,y);
        if(oob(backwardNeigbor))
            printf("Backward neighbor out of bounds! cell (%i,%i) \n",x,y);
        if(oob(forwardNeigbor))
            printf("Forward neighbor out of bounds! cell (%i,%i) \n",x,y);

        float rho = grid.read<AT::density>(cellId);
        float velX = grid.read<AT::velocityX>(cellId);
        float velY = grid.read<AT::velocityY>(cellId);

        // calculate gradient using central difference
        // since we use the density at at i and i+1 we get the gradient halfway in between the cells,
        // on the edge between cell i and i+1
        float rhoRight     = grid.read<AT::density>(cs.getRightNeighbor(cellId));
        float rhoForward   = grid.read<AT::density>(cs.getForwardNeighbor(cellId));

        float2 gradRho = gradient2d(rho, rhoRight, rho, rhoForward, cellPos, cs);

        grid.write<AT::densityGradX>(cellId, gradRho.x);
        grid.write<AT::densityGradY>(cellId, gradRho.y);

        // calculate divergence of the velocity field
        // remember, velocities are defined half way between the nodes,
        // we want the divergence at the node, so we get a central difference by looking at the velocities left and backwards from us
        // and compare them to our velocities
        float velLeftX = grid.read<AT::velocityX>(cs.getLeftNeighbor(cellId));
        float velBackwardY = grid.read<AT::velocityY>(cs.getBackwardNeighbor(cellId));

        float velDiv = divergence2d(velLeftX,velX,velBackwardY,velY,cellPos,cs);

        grid.write<AT::velocityDiv>(cellId, velDiv);

        // laplace
        float rhoLeft     = grid.read<AT::density>(cs.getLeftNeighbor(cellId));
        float rhoBackward   = grid.read<AT::density>(cs.getBackwardNeighbor(cellId));

        float laplace = laplace2d(rhoLeft,rhoRight,rhoBackward,rhoForward,rho,cellPos,cs);

        grid.write<AT::densityLaplace>(cellId, laplace);

        // curl is more difficult, as we can only compute it at cell corners
        // offsetted from where we want to visualize it
        // so we need to compute 4 curls and average them

        // forward right quadrant
        float velRightY = grid.read<AT::velocityY>(cs.getRightNeighbor(cellId));
        float velForwardX = grid.read<AT::velocityX>(cs.getForwardNeighbor(cellId));

        float forwardRightCurl = curl2d(velY,velRightY, velX, velForwardX,cellPos,cs);
        // averaging is done in the next kernel
        offsettedCurl[cellId] = forwardRightCurl;

        // temperature gradient

        float temp = grid.read<AT::temperature>(cellId);
        float tempRight = grid.read<AT::temperature>(cs.getRightNeighbor(cellId));
        float tempForward = grid.read<AT::temperature>(cs.getForwardNeighbor(cellId));

        float2 tempGrad = gradient2d(temp,tempRight,temp,tempForward,cellPos,cs);

        grid.write<AT::temperatureGradX>(cellId,tempGrad.x);
        grid.write<AT::temperatureGradY>(cellId,tempGrad.y);
    }
}

template <typename csT>
__global__ void testSimulationB(TestSimGrid::ReferenceType grid, csT coordinateSystem, mpu::VectorReference<const float> offsettedCurl,
                                bool useLeapfrog, bool diffuseHeat, bool advectHeat, float heatCoefficient, bool useDivOfGrad, float timestep)
{
    csT cs = coordinateSystem;

    for(int x : mpu::gridStrideRange( cs.hasBoundary().x, cs.getNumGridCells3d().x-cs.hasBoundary().x ))
        for(int y : mpu::gridStrideRangeY( cs.hasBoundary().y, cs.getNumGridCells3d().y-cs.hasBoundary().y ))
        {
            int3 cell{x,y,0};
            int cellId = cs.getCellId(cell);
            float2 cellPos = make_float2( cs.getCellCoordinate3d(cell) );

            // only forward right curl was computed above, so now curl must be interpolated
            float curlForwardRight = offsettedCurl[cellId];
            float curlForwardLeft = offsettedCurl[cs.getLeftNeighbor(cellId)];
            float curlBackwardsRight = offsettedCurl[cs.getBackwardNeighbor(cellId)];
            float curlBackwardsLeft = offsettedCurl[cs.getLeftNeighbor(cs.getBackwardNeighbor(cellId))];

            float averageCurl = curlForwardRight + curlForwardLeft + curlBackwardsRight + curlBackwardsLeft;
            averageCurl *= 0.25;

            grid.write<AT::velocityCurl>(cellId, averageCurl);

            // solve the heat equation
            if(diffuseHeat || advectHeat)
            {
                float temp_dt =0;
                float temp = grid.read<AT::temperature>(cellId);

                if(diffuseHeat)
                {
                    float tempGradX = grid.readNext<AT::temperatureGradX>(cellId);
                    float tempGradY = grid.readNext<AT::temperatureGradY>(cellId);
                    float tempGradXLeft = grid.readNext<AT::temperatureGradX>(cs.getLeftNeighbor(cellId));
                    float tempGradYBack = grid.readNext<AT::temperatureGradY>(cs.getBackwardNeighbor(cellId));

                    float heatDivGrad = divergence2d(tempGradXLeft, tempGradX, tempGradYBack, tempGradY, cellPos, cs);

                    float tempLeft = grid.read<AT::temperature>(cs.getLeftNeighbor(cellId));
                    float tempRight = grid.read<AT::temperature>(cs.getRightNeighbor(cellId));
                    float tempForward = grid.read<AT::temperature>(cs.getForwardNeighbor(cellId));
                    float tempBackward = grid.read<AT::temperature>(cs.getBackwardNeighbor(cellId));

                    float heatLaplace = laplace2d(tempLeft,tempRight,tempBackward,tempForward,temp,cellPos,cs);

                    if(useDivOfGrad)
                        temp_dt += heatCoefficient * heatDivGrad;
                    else
                        temp_dt += heatCoefficient *heatLaplace;
                }

                if(advectHeat)
                {
                    temp_dt -= grid.readNext<AT::velocityDiv>(cellId) * temp;
                }

                float previousTemp;
                if(useLeapfrog)
                {
                    previousTemp = grid.readPrev<AT::temperature>(cellId);
                    timestep *=2.0f;
                }
                else
                {
                    previousTemp = temp;
                }

                float nextTemp =  previousTemp + temp_dt * timestep;
                grid.write<AT::temperature>(cellId,nextTemp);
            }
            else
                grid.copy<AT::temperature>(cellId);
        }
}

template <typename csT>
void TestSimulation::simulateOnceImpl(csT& cs)
{
    dim3 blocksize{16,16,1};
    dim3 numBlocks{ static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().x ,blocksize.x)),
                    static_cast<unsigned int>(mpu::numBlocks( cs.getNumGridCells3d().y ,blocksize.y)), 1};

    if(m_needUpdateBoundaries)
    {
//        m_grid->cacheOnHost();
        initializeFixedValueBoundaries<AT::temperature>(!m_boundaryIsolatedX && m_cs->hasBoundary().x,
                                                        !m_boundaryIsolatedY && m_cs->hasBoundary().y,
                                                        m_boundaryTemperatureX, m_boundaryTemperatureY, *m_cs, *m_grid);
//        m_grid->pushCachToDevice();
    }

    handleMirroredBoundaries<AT::temperature>(m_boundaryIsolatedX && cs.hasBoundary().x,
                                              m_boundaryIsolatedY && cs.hasBoundary().y,
                                              cs, *m_grid);

    if(m_diffuseHeat)
        m_totalSimulatedTime += m_timestep;

    testSimulationA<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_offsettedCurl.getVectorReference(),
            m_diffuseHeat,m_advectHeat,m_heatCoefficient,m_useDivOfGrad,m_timestep);
    testSimulationB<<< numBlocks, blocksize>>>(m_grid->getGridReference(),cs,m_offsettedCurl.getVectorReference(),
            !m_firstTimestep && m_leapfrogIntegrattion,m_diffuseHeat,m_advectHeat,m_heatCoefficient,m_useDivOfGrad,m_timestep);

    m_firstTimestep = false;
}

GridBase& TestSimulation::getGrid()
{
    return *m_grid;
}

std::string TestSimulation::getDisplayName()
{
    return "Test Simulation";
}
