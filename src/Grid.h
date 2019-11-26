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

/**
 * @brief AT = AttributeType different types of attributes that can be stored in the grid
 */
enum class AT
{
    velocity,
    density
};

/**
 * @brief Template to create a grid attribute that stores an array auf data and has am attribute type
 */
template <AT attributeType, typename T>
class GridAttribute
{
public:
    explicit GridAttribute(int numCells) : m_data(numCells) {}

    T read(int cellId) {return m_data[cellId];}
    void write(int cellId, T&& data) {m_data[cellId] = std::forward<T>(data);}

    static constexpr AT type = attributeType;
private:
    std::vector<T> m_data;
};


// define some attributes for the grid
using GridDensity = GridAttribute<AT::density,float>;
using GridVelocity2D = GridAttribute<AT::velocity,float2>;

//-------------------------------------------------------------------
/**
 * @brief buffer object used internally by the grid, can store a arbitrary number of attributes
 * @tparam Attributes grid attributes that needs a compatible read and write function as well as data storage (best use only things from above list)
 */
template <typename ...Attributes>
class GridBuffer : Attributes...
{
public:
    explicit GridBuffer(int numCells) : Attributes(numCells)...{};

    template <AT Param>
    auto read(int cellId);
    template <AT Param, typename T>
    void write(int cellId, T&& data);
};

//!< selects the first attribute with type == param from attributes
template <AT Param, typename First, typename ...Attributes>
struct GridAttributeSelectorImpl
{
    using type = mpu::if_else_t< First::type == Param, First, GridAttributeSelectorImpl<Param,Attributes...> >;
};

//!< selects the first attribute with type == param from attributes
template <AT Param, typename ...Attributes>
using GridAttributeSelector_t = typename GridAttributeSelectorImpl<Param,Attributes...>::type;

// template function definitions of the GridBuffer class
//-------------------------------------------------------------------
template <typename... Attributes>
template <AT Param>
auto GridBuffer<Attributes...>::read(int cellId)
{
    return GridAttributeSelector_t<Param,Attributes...>::read(cellId);
}

template <typename... Attributes>
template <AT Param, typename T>
void GridBuffer<Attributes...>::write(int cellId, T&& data)
{
    GridAttributeSelector_t<Param,Attributes...>::write(cellId,std::forward<T>(data));
}

//-------------------------------------------------------------------
/**
 * class Grid
 *
 * usage:
 * Class to manage memory for simulation data of a grid based simulation.
 * Supports buffer swap and rendering be done from different threads.
 *
 */
template <typename BufferType, typename RenderBufferType>
class Grid
{
public:
    explicit Grid(int numCells);

    void swapBuffer(); //!< swap working buffers, the old write buffer becomes the read buffer
    void swapAndRender(); //!< swap and ready the current buffer for rendering
    void swapAndRenderWait(); //!< swap and ready the current buffer for rendering, make sure no unrendered data is discarded

    bool newRenderDataReady(); //!< the renderbuffer has new data to render
    RenderBufferType* getRenderbuffer(); //!< get access to the renderbuffer. Blocks until readyToRender is true. Data will be valid until renderDone() was called
    void renderDone(); //!< indicates render buffer can be overwritten

    template <AT Param>
    auto read(int cellId); //!< read data from grid cell cellId parameter Param
    template <AT Param, typename T>
    void write(int cellId, T&& data); //!< read data from grid cell cellId parameter Param

private:
    BufferType* m_readBuffer; //!< the buffer data is read from
    BufferType* m_writeBuffer; //!< the buffer data is written to

    BufferType* m_renderAwaitBuffer; //!< data that will be copied to the openGL buffer on the rendering GPU
    BufferType* m_unusedBuffer; //!< when we read from the renderAwait buffer one buffer will be unused

    BufferType m_bufferA; //!< buffer for cuda grid data
    BufferType m_bufferB; //!< buffer for cuda grid data
    BufferType m_bufferC; //!< buffer for cuda grid data
    RenderBufferType m_renderBuffer; //!< openGL buffer to render from

    std::atomic_bool m_renderbufferNotRendered{false}; //!< indicates that renderbuffer contains data that have not been rendered yet
    std::atomic_bool m_newRenderdataWaiting{false}; //!< indicate new renderdata are ready to be written to the renderbuffer

    std::mutex m_rbuMtx; //!< renderbuffer mutex
    std::mutex m_rabuMtx; //!< renderAwaitBuffer mutex

    void prepareForRendering(); //!< copies data from renderAwaitBuffer to renderBuffer
};

// template function definitions of the Grid class
//-------------------------------------------------------------------

template <typename BufferType, typename RenderBufferType>
template <AT Param>
auto Grid<BufferType,RenderBufferType>::read(int cellId)
{
    return m_readBuffer->read<Param>(cellId);
}

template <typename BufferType, typename RenderBufferType>
template <AT Param, typename T>
void Grid<BufferType,RenderBufferType>::write(int cellId, T&& data)
{
    return m_writeBuffer->write<Param>(cellId, std::forward<T>(data));
}

template <typename BufferType, typename RenderBufferType>
Grid<BufferType,RenderBufferType>::Grid(int numCells) : m_bufferA(numCells), m_bufferB(numCells), m_bufferC(numCells)
{
    m_readBuffer  = &m_bufferA;
    m_writeBuffer = &m_bufferB;
    m_unusedBuffer = nullptr;
    m_renderAwaitBuffer = &m_bufferC;
}

template <typename BufferType, typename RenderBufferType>
void Grid<BufferType,RenderBufferType>::swapBuffer()
{
    BufferType* tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_readBuffer == m_renderAwaitBuffer)
    {
        m_writeBuffer = m_unusedBuffer;
        m_unusedBuffer = nullptr;
    } else
    {
        m_writeBuffer = m_readBuffer;
    }
    m_readBuffer = tmp;
}

template <typename BufferType, typename RenderBufferType>
void Grid<BufferType,RenderBufferType>::swapAndRender()
{
    std::unique_lock<std::mutex> lck(m_rabuMtx);

    BufferType* tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_readBuffer == m_renderAwaitBuffer)
        m_writeBuffer = m_unusedBuffer;
    else
        m_writeBuffer = m_readBuffer;

    m_unusedBuffer = m_renderAwaitBuffer;
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

template <typename BufferType, typename RenderBufferType>
void Grid<BufferType,RenderBufferType>::swapAndRenderWait()
{
    while(m_newRenderdataWaiting) mpu::yield(); // don't oververwrite await buffer if data is still waiting

    std::unique_lock<std::mutex> lck(m_rabuMtx);

    BufferType* tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_readBuffer == m_renderAwaitBuffer)
        m_writeBuffer = m_unusedBuffer;
    else
        m_writeBuffer = m_readBuffer;

    m_unusedBuffer = m_renderAwaitBuffer;
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

template <typename BufferType, typename RenderBufferType>
void Grid<BufferType,RenderBufferType>::renderDone()
{
    if(m_newRenderdataWaiting)
    {
        std::lock_guard<std::mutex> lck(m_rabuMtx);
        prepareForRendering();
    }

    m_renderbufferNotRendered = false;
    m_rbuMtx.unlock();
}

template <typename BufferType, typename RenderBufferType>
RenderBufferType* Grid<BufferType,RenderBufferType>::getRenderbuffer()
{
    m_rbuMtx.lock();
    return &m_renderBuffer;
}

template <typename BufferType, typename RenderBufferType>
bool Grid<BufferType,RenderBufferType>::newRenderDataReady()
{
    return m_renderbufferNotRendered;
}

template <typename BufferType, typename RenderBufferType>
void Grid<BufferType,RenderBufferType>::prepareForRendering()
{
    m_renderBuffer = *m_renderAwaitBuffer;
    m_newRenderdataWaiting = false;
    m_renderbufferNotRendered = true;
}


#endif //CIRCULATION_GRID_H
