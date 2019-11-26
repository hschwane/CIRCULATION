/*
 * CIRCULATION
 * Grid.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the Grid class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_GRID_H
#define CIRCULATION_GRID_H

// includes
//--------------------
#include <mpUtils/mpUtils.h>
#include <mpUtils/mpGraphics.h>
#include <mpUtils/mpCuda.h>
//--------------------

///**
// * @brief Grid precision, allows to select different
// */
//enum class GridPrecision
//{
//    fp64,
//    fp32,
//    fp16
//};
//template <GridPrecision precision, typename ReturnType>
//class GridBufferScalar
//{
//};
//
//template <typename ReturnType>
//class GridBufferScalar<GridPrecision::fp32,ReturnType>
//{
//    std::vector<float> m_data;
//};

//-------------------------------------------------------------------
/**
 * class Grid
 *
 * usage:
 * Class to manage memory for simulation data of a grid based simulation.
 * Supports buffer swap and rendering be done from different threads.
 *
 */
class Grid
{
public:
    using BufferType = std::vector<float>;

    explicit Grid(int numCells);

    void swapBuffer(); //!< swap working buffers, the old write buffer becomes the read buffer
    void swapAndRender(); //!< swap and ready the current buffer for rendering
    void swapAndRenderWait(); //!< swap and ready the current buffer for rendering, make sure no unrendered data is discarded

    bool newRenderDataReady(); //!< the renderbuffer has new data to render
    BufferType* getRenderbuffer(); //!< get access to the renderbuffer. Blocks until readyToRender is true. Data will be valid until renderDone() was called
    void renderDone(); //!< indicates render buffer can be overwritten

    BufferType& read();
    BufferType& write();

private:
    BufferType* m_readBuffer; //!< the buffer data is read from
    BufferType* m_writeBuffer; //!< the buffer data is written to

    BufferType* m_renderAwaitBuffer; //!< data that will be copied to the openGL buffer on the rendering GPU
    BufferType* m_unsuedBuffer; //!< when we read from the renderAwait buffer one buffer will be unused

    BufferType m_bufferA;
    BufferType m_bufferB;
    BufferType m_bufferC;
    BufferType m_renderBuffer;

    std::atomic_bool m_renderbufferNotRendered{false}; //!< indicates that renderbuffer contains data that have not been rendered yet
    std::atomic_bool m_newRenderdataWaiting{false}; //!< indicate new renderdata are ready to be written to the renderbuffer

    std::mutex m_rbuMtx; //!< renderbuffer mutex
    std::mutex m_rabuMtx; //!< renderAwaitBuffer mutex

    void prepareForRendering();
};

#endif //CIRCULATION_GRID_H
