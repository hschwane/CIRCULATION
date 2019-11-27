/*
 * CIRCULATION
 * main.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#include <mpUtils/mpUtils.h>
#include "Application.h"
#include "Grid.h"

int main()
{
    // setup logging
    mpu::Log myLog( mpu::LogLvl::ALL, mpu::ConsoleSink());
#if defined(NDEBUG)
    myLog.printHeader("CIRCULATION", CIRCULATION_VERSION, "", "Release");
#else
    myLog.printHeader("CIRCULATION", CIRCULATION_VERSION, CIRCULATION_VERSION_SHA, "Debug");
#endif

//    // create app
    Application myApp(600,600);

//    // run app
//    while (myApp.run());


    GridBuffer<GridDensity,GridVelocity2D> buffer(20);
    RenderBuffer<GridDensity::RenderType, GridVelocity2D::RenderType> rb(20);


    buffer.write<AT::density>(2,0.45451f);


    rb.write(buffer);
    rb.bind(0,GL_SHADER_STORAGE_BUFFER);

    mpu::gph::Buffer<float> vbo(30);

    return 0;
}