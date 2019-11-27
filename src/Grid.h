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

// forward declaration
enum class AT;
template <AT attributeType, typename T> class RenderAttribute;
template <typename ...Atrribs> class RenderBuffer;

//-------------------------------------------------------------------
/**
 * @brief Template to create a grid attribute that stores an array auf data and has am attribute type
 */
template <AT attributeType, typename T>
class GridAttribute
{
public:
    explicit GridAttribute(int numCells=0) : m_data(numCells) {}

    T read(int cellId) {return m_data[cellId];}
    void write(int cellId, T&& data) {m_data[cellId] = std::forward<T>(data);}

    static constexpr AT type = attributeType;
    using RenderType = RenderAttribute<attributeType, T>;
    friend class RenderAttribute<attributeType,T>;

private:
    std::vector<T> m_data;
};

//-------------------------------------------------------------------
/**
 * @brief Like GridAttribute but uses opengl buffer and can be used for rendering does only work for float and floatN types righ now
 */
template <AT attributeType, typename T>
class RenderAttribute
{
public:
    explicit RenderAttribute(int numCells=0) : m_data(numCells) {}

    void write(const GridAttribute<attributeType,T> & source) {m_data.write(source.m_data);}
    void bind(GLuint binding, GLenum target) {m_data.bindBase(binding,target);}
    void addToVao(mpu::gph::VertexArray& vao, int binding) {vao.addAttributeBufferArray(binding,binding,m_data,0,sizeof(T),
                                                                                        sizeof(T)/sizeof(float),0);}
    static constexpr AT type = attributeType;

private:
    mpu::gph::Buffer<T,true> m_data;
};


//-------------------------------------------------------------------
// define some attributes for the grid

/**
 * @brief AT = AttributeType different types of attributes that can be stored in the grid
 */
enum class AT
{
    velocity,
    density
};

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
    explicit GridBuffer(int numCells=0) : Attributes(numCells)...{};

    template <AT Param>
    auto read(int cellId);
    template <AT Param, typename T>
    void write(int cellId, T&& data);

    friend class RenderBuffer<typename Attributes::RenderType...>;
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
 * @brief buffer object used internally by the grid to store data to be rendered
 */
template <typename ...Attributes>
class RenderBuffer : Attributes...
{
public:
    explicit RenderBuffer(int numCells=0) : Attributes(numCells)...{};

    template<typename ...SourceAttribs>
    void write(GridBuffer<SourceAttribs...>& source);
    void bind(GLuint binding, GLenum target);
    void addToVao(mpu::gph::VertexArray& vao, int binding);
};

// template function definitions of the RenderBuffer class
//-------------------------------------------------------------------
template <typename... Attributes>
template <typename... SourceAttribs>
void RenderBuffer<Attributes...>::write(GridBuffer<SourceAttribs...>& source)
{
    int t[] = {0, ((void)Attributes::write( static_cast<SourceAttribs>(source) ),1)...};
    (void)t[0];
}

template <typename... Attributes>
void RenderBuffer<Attributes...>::bind(GLuint binding, GLenum target)
{
    int t[] = {0, ((void)Attributes::bind(binding,target),1)...};
    (void)t[0]; // silence compiler warning abut t being unused
}

template <typename... Attributes>
void RenderBuffer<Attributes...>::addToVao(mpu::gph::VertexArray& vao, int binding)
{
    int t[] = {0, ((void)Attributes::addToVao(vao,binding),1)...};
    (void)t[0]; // silence compiler warning abut t being unused
}

//-------------------------------------------------------------------
/**
 * class Grid
 *
 * Class to manage memory for simulation data of a grid based simulation.
 * Supports buffer swap and rendering be done from different threads.
 *
 * usage:
 * Use variadic template to define attribute types from above list (e.g. GridDensity, GridVelocity, usw)
 *
 */
template <typename ...GridAttribs>
class Grid
{
public:
    using BufferType = GridBuffer<GridAttribs...>;
    using RenderBufferType = RenderBuffer<typename GridAttribs::RenderType ...>;

    explicit Grid(int numCells=0);

    void swapBuffer(); //!< swap working buffers, the old write buffer becomes the read buffer
    void swapAndRender(); //!< swap and ready the current buffer for rendering
    void swapAndRenderWait(); //!< swap and ready the current buffer for rendering, make sure no unrendered data is discarded

    bool newRenderDataReady(); //!< the renderbuffer has new data to render
    void startRendering(); //!< lock access to the renderbuffer. Blocks until readyToRender is true. Data in the renderbuffer will be valid until renderDone() was called
    void renderDone(); //!< indicates render buffer can be overwritten

    void bindRenderBuffer(GLuint binding, GLenum target); //!< bind the renderbuffer to target starting with binding id binding
    void addRenderBufferToVao(mpu::gph::VertexArray& vao, int binding); //!< adds the renderbuffer buffers onto the vao starting with binding id binding

    template <AT Param>
    auto read(int cellId); //!< read data from grid cell cellId parameter Param
    template <AT Param, typename T>
    void write(int cellId, T&& data); //!< read data from grid cell cellId parameter Param

    int size(); //!< returns the number of available grid cells

private:
    int m_numCells; //!< number of grid cells

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

template <typename ...GridAttribs>
template <AT Param>
auto Grid<GridAttribs...>::read(int cellId)
{
    return m_readBuffer->read<Param>(cellId);
}

template <typename ...GridAttribs>
template <AT Param, typename T>
void Grid<GridAttribs...>::write(int cellId, T&& data)
{
    return m_writeBuffer->write<Param>(cellId, std::forward<T>(data));
}

template <typename ...GridAttribs>
Grid<GridAttribs...>::Grid(int numCells)
    : m_bufferA(numCells), m_bufferB(numCells),
    m_bufferC(numCells), m_numCells(numCells)
{
    m_readBuffer  = &m_bufferA;
    m_writeBuffer = &m_bufferB;
    m_unusedBuffer = nullptr;
    m_renderAwaitBuffer = &m_bufferC;
}

template <typename ...GridAttribs>
void Grid<GridAttribs...>::swapBuffer()
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

template <typename ...GridAttribs>
void Grid<GridAttribs...>::swapAndRender()
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
        lck.unlock();
        m_rbuMtx.unlock();
    } else
        m_newRenderdataWaiting = true;
}

template <typename ...GridAttribs>
void Grid<GridAttribs...>::swapAndRenderWait()
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

template <typename ...GridAttribs>
void Grid<GridAttribs...>::renderDone()
{
    if(m_newRenderdataWaiting)
    {
        std::lock_guard<std::mutex> lck(m_rabuMtx);
        prepareForRendering();
    }

    m_renderbufferNotRendered = false;
    m_rbuMtx.unlock();
}

template <typename ...GridAttribs>
void Grid<GridAttribs...>::startRendering()
{
    m_rbuMtx.lock();
}

template <typename ...GridAttribs>
bool Grid<GridAttribs...>::newRenderDataReady()
{
    return m_renderbufferNotRendered;
}

template <typename ...GridAttribs>
void Grid<GridAttribs...>::prepareForRendering()
{
    m_renderBuffer.write( *m_renderAwaitBuffer);
    m_newRenderdataWaiting = false;
    m_renderbufferNotRendered = true;
}

template <typename ...GridAttribs>
void Grid<GridAttribs...>::bindRenderBuffer(GLuint binding, GLenum target)
{
    m_renderBuffer.bind(binding,target);
}

template <typename ...GridAttribs>
void Grid<GridAttribs...>::addRenderBufferToVao(mpu::gph::VertexArray& vao, int binding)
{
    m_renderBuffer.addToVao(vao,binding);
}

template <typename ...GridAttribs>
int Grid<GridAttribs...>::size()
{
    return m_numCells;
}

// template function definitions of the Grid class
//-------------------------------------------------------------------

// declare and precompile some grid types

using TestGrid = Grid<GridDensity,GridVelocity2D>;

extern template class Grid<GridDensity,GridVelocity2D>;

#endif //CIRCULATION_GRID_H
