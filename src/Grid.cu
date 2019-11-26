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

// function definitions of the Grid class
//-------------------------------------------------------------------

Grid::Grid(int numCells)
{
    m_readBuffer  = &m_bufferA;
    m_writeBuffer = &m_bufferB;
    m_unsuedBuffer = nullptr;
    m_renderAwaitBuffer = &m_bufferC;
}

void Grid::swapBuffer()
{
    BufferType* tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_readBuffer == m_renderAwaitBuffer)
    {
        m_writeBuffer = m_unsuedBuffer;
        m_unsuedBuffer = nullptr;
    } else
    {
        m_writeBuffer = m_readBuffer;
    }
    m_readBuffer = tmp;
}

void Grid::swapAndRender()
{
    std::unique_lock<std::mutex> lck(m_rabuMtx);

    BufferType* tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_readBuffer == m_renderAwaitBuffer)
        m_writeBuffer = m_unsuedBuffer;
    else
        m_writeBuffer = m_readBuffer;

    m_unsuedBuffer = m_renderAwaitBuffer;
    m_renderAwaitBuffer = tmp;
    m_readBuffer = m_renderAwaitBuffer;

    lck.unlock();

    if(m_rbuMtx.try_lock())
    {
        lck.lock();
        prepareForRendering();
        lck.lock();
        m_rbuMtx.unlock();
    } else
        m_newRenderdataWaiting = true;
}

void Grid::swapAndRenderWait()
{
    while(m_newRenderdataWaiting) mpu::yield(); // don't oververwrite await buffer if data is still waiting

    std::unique_lock<std::mutex> lck(m_rabuMtx);

    BufferType* tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_readBuffer == m_renderAwaitBuffer)
        m_writeBuffer = m_unsuedBuffer;
    else
        m_writeBuffer = m_readBuffer;

    m_unsuedBuffer = m_renderAwaitBuffer;
    m_renderAwaitBuffer = tmp;
    m_readBuffer = m_renderAwaitBuffer;

    lck.unlock();

    while(m_renderbufferNotRendered) mpu::yield(); // don't copy to renderbuffer bevore is is rendered

    if(m_rbuMtx.try_lock())
    {
        lck.lock();
        prepareForRendering();
        lck.lock();
        m_rbuMtx.unlock();
    } else
        m_newRenderdataWaiting = true;
}

void Grid::renderDone()
{
    if(m_newRenderdataWaiting)
    {
        std::lock_guard<std::mutex> lck(m_rabuMtx);
        prepareForRendering();
    }

    m_renderbufferNotRendered = false;
    m_rbuMtx.unlock();
}

Grid::BufferType& Grid::read()
{
    return *m_readBuffer;
}

Grid::BufferType& Grid::write()
{
    return *m_writeBuffer;
}

Grid::BufferType* Grid::getRenderbuffer()
{
    m_rbuMtx.lock();
    return &m_renderBuffer;
}

bool Grid::newRenderDataReady()
{
    return m_renderbufferNotRendered;
}

void Grid::prepareForRendering()
{
    m_renderBuffer = *m_renderAwaitBuffer;
    m_newRenderdataWaiting = false;
    m_renderbufferNotRendered = true;
}