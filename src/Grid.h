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
    velocityX,
    velocityY,
    density
};


using GridDensity = GridAttribute<AT::density,float>;
using GridVelocityX = GridAttribute<AT::velocityX,float>;
using GridVelocityY = GridAttribute<AT::velocityY,float>;


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
 * Supports buffer swap and rendering to be done from two different threads.
 *
 * usage:
 * Use variadic template to define attribute types from above list (e.g. GridDensity, GridVelocity, usw)
 * Only copy/move/create in the render thread in single threaded contex! (because openGL buffers are part of this and the context need to be valid)
 * Also threading might break when copying while other thread is still working on the grid as copy / move / swap are NOT thread safe.
 *
 */
template <typename ...GridAttribs>
class Grid
{
public:
    using BufferType = GridBuffer<GridAttribs...>;
    using RenderBufferType = RenderBuffer<typename GridAttribs::RenderType ...>;

    explicit Grid(int numCells=0);

    // copy and move constructor (copy swap idom)
    Grid(const Grid& other);
    Grid(Grid&& other) noexcept;
    Grid& operator=(Grid other) noexcept;

    void swapBuffer(); //!< swap working buffers, the old write buffer becomes the read buffer
    void swapAndRender(); //!< swap and ready the current buffer for rendering
    void swapAndRenderWait(); //!< swap and ready the current buffer for rendering, make sure no unrendered data is discarded

    bool newRenderDataReady(); //!< the renderbuffer has new data to render
    void startRendering(); //!< lock access to the renderbuffer. Blocks until readyToRender is true. Data in the renderbuffer will be valid until renderDone() was called
    void renderDone(); //!< indicates render buffer can be overwritten

    void bindRenderBuffer(GLuint binding, GLenum target); //!< bind the renderbuffer to target starting with binding id binding
    void addRenderBufferToVao(mpu::gph::VertexArray& vao, int binding); //!< adds the renderbuffer buffers onto the vao starting with binding id binding

    void cacheOnHost(); //!< cache the current buffers data on the host
    void pushCachToDevice(); //!< write changes from the local cache back to the device

    template <AT Param>
    auto read(int cellId); //!< read data from grid cell cellId parameter Param
    template <AT Param, typename T>
    void write(int cellId, T&& data); //!< read data from grid cell cellId parameter Param

    int size(); //!< returns the number of available grid cells

    // for copy swap idom
    friend void swap(Grid& first, Grid& second) //!< swap two instances of buffer
    {
        using std::swap;

        swap(first.m_numCells,second.m_numCells);

        swap(first.m_readBuffer,second.m_readBuffer);
        swap(first.m_writeBuffer,second.m_writeBuffer);
        swap(first.m_renderAwaitBuffer,second.m_renderAwaitBuffer);
        swap(first.m_unusedBuffer,second.m_unusedBuffer);

        swap(first.m_bufferA,second.m_bufferA);
        swap(first.m_bufferB,second.m_bufferB);
        swap(first.m_bufferC,second.m_bufferC);
        swap(first.m_renderBuffer,second.m_renderBuffer);

        bool b = first.m_renderbufferNotRendered;
        first.m_renderbufferNotRendered = second.m_renderbufferNotRendered.load();
        second.m_renderbufferNotRendered = b;
        b = first.m_newRenderdataWaiting;
        first.m_newRenderdataWaiting = second.m_newRenderdataWaiting.load();
        second.m_newRenderdataWaiting = b;
    }

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
    m_bufferC(numCells), m_numCells(numCells),
    m_renderBuffer(numCells)
{
    m_readBuffer  = &m_bufferA;
    m_writeBuffer = &m_bufferB;
    m_unusedBuffer = nullptr;
    m_renderAwaitBuffer = &m_bufferC;
}

template <typename... GridAttribs>
Grid<GridAttribs...>::Grid(const Grid& other)
    : m_bufferA(other.m_bufferA),
      m_bufferB(other.m_bufferB),
      m_bufferC(other.m_bufferC),
      m_renderBuffer(other.m_renderBuffer),
      m_renderbufferNotRendered(other.m_renderbufferNotRendered.load()),
      m_newRenderdataWaiting(other.m_newRenderdataWaiting.load()),
      m_rbuMtx(),
      m_rabuMtx(),
      m_numCells(other.m_numCells)
{
    if(other.m_readBuffer == &other.m_bufferA)
        m_readBuffer = &m_bufferA;
    else if(other.m_readBuffer == &other.m_bufferB)
        m_readBuffer = &m_bufferB;
    else if(other.m_readBuffer == &other.m_bufferC)
        m_readBuffer = &m_bufferC;
    else
        m_readBuffer = nullptr;

    if(other.m_writeBuffer == &other.m_bufferA)
        m_writeBuffer = &m_bufferA;
    else if(other.m_writeBuffer == &other.m_bufferB)
        m_writeBuffer = &m_bufferB;
    else if(other.m_writeBuffer == &other.m_bufferC)
        m_writeBuffer = &m_bufferC;
    else
        m_writeBuffer = nullptr;

    if(other.m_renderAwaitBuffer == &other.m_bufferA)
        m_renderAwaitBuffer = &m_bufferA;
    else if(other.m_renderAwaitBuffer == &other.m_bufferB)
        m_renderAwaitBuffer = &m_bufferB;
    else if(other.m_renderAwaitBuffer == &other.m_bufferC)
        m_renderAwaitBuffer = &m_bufferC;
    else
        m_renderAwaitBuffer = nullptr;

    if(other.m_unusedBuffer == &other.m_bufferA)
        m_unusedBuffer = &m_bufferA;
    else if(other.m_unusedBuffer == &other.m_bufferB)
        m_unusedBuffer = &m_bufferB;
    else if(other.m_unusedBuffer == &other.m_bufferC)
        m_unusedBuffer = &m_bufferC;
    else
        m_unusedBuffer = nullptr;
}

template <typename... GridAttribs>
Grid<GridAttribs...>::Grid(Grid&& other) noexcept
    : Grid()
{
    swap(*this,other);
}

template <typename... GridAttribs>
Grid<GridAttribs...>& Grid<GridAttribs...>::operator=(Grid other) noexcept
{
    swap(*this,other);
    return *this;
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

template <typename... GridAttribs>
void Grid<GridAttribs...>::cacheOnHost()
{
    logWARNING("Grid") << "host cache is not implemented jet";
}

template <typename... GridAttribs>
void Grid<GridAttribs...>::pushCachToDevice()
{
    logWARNING("Grid") << "host cache is not implemented jet";
}

// template function definitions of the Grid class
//-------------------------------------------------------------------

// declare and precompile some grid types

using RenderDemoGrid = Grid<GridDensity,GridVelocityX,GridVelocityY>;

extern template class Grid<GridDensity,GridVelocityX,GridVelocityY>;

#endif //CIRCULATION_GRID_H
