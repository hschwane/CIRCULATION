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
template <AT attributeType, typename T> class GridAttributeReference;
template <typename ...AttribRefs> class GridBufferReference;
template <typename ...AttribRefs> class GridReference;

//-------------------------------------------------------------------
/**
 * @brief Template to create a grid attribute that stores an array auf data and has am attribute type
 */
template <AT attributeType, typename T>
class GridAttribute
{
public:
    GridAttribute() : m_data() {}
    explicit GridAttribute(int numCells) : m_data(numCells) {}

    T read(int cellId) {return m_data[cellId];}
    template <typename Tin>
    void write(int cellId, Tin&& data)
    {
        m_data[cellId] = std::move<Tin>(data);
    }

    static constexpr AT type = attributeType;
    using RenderType = RenderAttribute<attributeType, T>;
    using ReferenceType = GridAttributeReference<attributeType, T>;
    friend class RenderAttribute<attributeType,T>;
    friend class GridAttributeReference<attributeType,T>;

    friend void swap(GridAttribute& first, GridAttribute& second)
    {
        using std::swap;
        swap(first.m_data,second.m_data);
    }

private:
    mpu::DeviceVector<T> m_data;
};

//-------------------------------------------------------------------
/**
 * @brief Like GridAttribute but uses opengl buffer and can be used for rendering does only work for float and floatN types righ now
 */
template <AT attributeType, typename T>
class RenderAttribute
{
public:
    RenderAttribute() : m_data(), m_bufferMapper() {}
    explicit RenderAttribute(int numCells) : m_data(numCells), m_bufferMapper()
    {
        if(m_data.size()>0)
            m_bufferMapper = mpu::mapBufferToCuda(m_data);
    }

    // copy constructor
    RenderAttribute(const RenderAttribute& other) : m_data(other.m_data), m_bufferMapper()
    {
        if(m_data.size())
            m_bufferMapper = mpu::mapBufferToCuda(m_data);
    }

    void write(const GridAttribute<attributeType,T> & source)
    {
        m_bufferMapper.map();
        assert_true(m_bufferMapper.size() == source.m_data.size(), "Grid", "Render Attribute does not have same size as GridAttribute");
        mpu::cudaCopy(m_bufferMapper.data(),source.m_data.data(),m_bufferMapper.size());
        m_bufferMapper.unmap();
    }

    void bind(GLuint binding, GLenum target) {m_data.bindBase(binding,target);}
    void addToVao(mpu::gph::VertexArray& vao, int binding) {vao.addAttributeBufferArray(binding,binding,m_data,0,sizeof(T),
                                                                                        sizeof(T)/sizeof(float),0);}
    static constexpr AT type = attributeType;

    friend void swap(RenderAttribute& first, RenderAttribute& second)
    {
        using std::swap;
        swap(first.m_data,second.m_data);
        swap(first.m_bufferMapper,second.m_bufferMapper);
    }

private:
    mpu::gph::Buffer<T,true> m_data;
    mpu::GlBufferMapper<T> m_bufferMapper;
};


//-------------------------------------------------------------------
// define some attributes for the grid

/**
 * @brief AT = AttributeType different types of attributes that can be stored in the grid
 */
enum class AT
{
    density,
    velocityX,
    velocityY,
    densityGradX,
    densityGradY,
    densityLaplace,
    velocityDiv,
    velocityCurl,
    temperature,
    temperatureGradX,
    temperatureGradY,
};


using GridDensity = GridAttribute<AT::density,float>;
using GridVelocityX = GridAttribute<AT::velocityX,float>;
using GridVelocityY = GridAttribute<AT::velocityY,float>;
using GridDensityGradX = GridAttribute<AT::densityGradX,float>;
using GridDensityGradY = GridAttribute<AT::densityGradY,float>;
using GridDensityLaplace = GridAttribute<AT::densityLaplace,float>;
using GridVelocityDiv = GridAttribute<AT::velocityDiv,float>;
using GridVelocityCurl = GridAttribute<AT::velocityCurl,float>;
using GridTemperature = GridAttribute<AT::temperature,float>;
using GridTemperatureGradX = GridAttribute<AT::temperatureGradX,float>;
using GridTemperatureGradY = GridAttribute<AT::temperatureGradY,float>;


//-------------------------------------------------------------------
/**
 * @brief buffer object used internally by the grid, can store a arbitrary number of attributes
 * @tparam Attributes grid attributes that needs a compatible read and write function as well as data storage (best use only things from above list)
 */
template <typename ...Attributes>
class GridBuffer : Attributes...
{
public:
    GridBuffer() : Attributes()...{};
    explicit GridBuffer(int numCells) : Attributes(numCells)...{};

    template <AT Param>
    auto read(int cellId);
    template <AT Param, typename T>
    void write(int cellId, T&& data);

    friend class RenderBuffer<typename Attributes::RenderType...>;
    friend class GridBufferReference<typename Attributes::ReferenceType...>;

    friend void swap(GridBuffer& first, GridBuffer& second)
    {
        using std::swap;
        int t[] = {0, ( swap(static_cast<Attributes&>(first) , static_cast<Attributes&>(second) ) ,1)...};
        (void)t[0];
    }
};

//!< selects the first attribute with type == param from attributes
template <AT Param, typename First, typename ...Attributes>
struct GridAttributeSelectorImpl
{
    using type = mpu::if_else_t< First::type == Param, First, typename GridAttributeSelectorImpl<Param,Attributes...>::type >;
};

template <AT Param, typename First>
struct GridAttributeSelectorImpl<Param,First>
{
    using type = mpu::if_else_t< First::type == Param, First, nullptr_t >;
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
    GridAttributeSelector_t<Param,Attributes...>::write(cellId, std::forward<T>(data));
}


//-------------------------------------------------------------------
/**
 * @brief buffer object used internally by the grid to store data to be rendered
 */
template <typename ...Attributes>
class RenderBuffer : Attributes...
{
public:
    explicit RenderBuffer(int numCells=1) : Attributes(numCells)...{};

    template<typename ...SourceAttribs>
    void write(GridBuffer<SourceAttribs...>& source);
    void bind(GLuint binding, GLenum target);
    void addToVao(mpu::gph::VertexArray& vao, int binding);

    friend void swap(RenderBuffer& first, RenderBuffer& second)
    {
        using std::swap;
        int t[] = {0, ( swap(static_cast<Attributes&>(first) , static_cast<Attributes&>(second) ) ,1)...};
        (void)t[0];
    }

private:
    template<size_t ... I>
    void addToVaoImpl(mpu::gph::VertexArray& vao, int binding, std::index_sequence<I ...>);
    template<size_t ... I>
    void bindImpl(GLuint binding, GLenum target, std::index_sequence<I ...>);
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
template <size_t... I>
void RenderBuffer<Attributes...>::bindImpl(GLuint binding, GLenum target, std::index_sequence<I...>)
{
    int t[] = {0, ((void)Attributes::bind(binding + I,target),1)...};
    (void)t[0]; // silence compiler warning abut t being unused
}

template <typename... Attributes>
void RenderBuffer<Attributes...>::bind(GLuint binding, GLenum target)
{
    bindImpl(binding,target, std::make_index_sequence<sizeof...(Attributes)>{});
}

template <typename... Attributes>
template <size_t... I>
void RenderBuffer<Attributes...>::addToVaoImpl(mpu::gph::VertexArray& vao, int binding, std::index_sequence<I ...>)
{
    int t[] = {0, ((void)Attributes::addToVao(vao,binding + I ),1)...};
    (void)t[0]; // silence compiler warning abut t being unused
}

template <typename... Attributes>
void RenderBuffer<Attributes...>::addToVao(mpu::gph::VertexArray& vao, int binding)
{
    addToVaoImpl(vao, binding, std::make_index_sequence<sizeof...(Attributes)>{});
}

//-------------------------------------------------------------------
/**
 * class GridBase
 *
 * base class to store and access grids of different types
 *
 */
class GridBase
{
public:
    virtual ~GridBase()=default;

    virtual void swapBuffer()=0; //!< swap working buffers, the old write buffer becomes the read buffer
    virtual void swapAndRender()=0; //!< swap and ready the current buffer for rendering
    virtual void swapAndRenderWait()=0; //!< swap and ready the current buffer for rendering, make sure no unrendered data is discarded

    virtual bool newRenderDataReady()=0; //!< the renderbuffer has new data to render
    virtual void startRendering()=0; //!< lock access to the renderbuffer. Blocks until readyToRender is true. Data in the renderbuffer will be valid until renderDone() was called
    virtual void renderDone()=0; //!< indicates render buffer can be overwritten

    virtual void bindRenderBuffer(GLuint binding, GLenum target)=0; //!< bind the renderbuffer to target starting with binding id binding
    virtual void addRenderBufferToVao(mpu::gph::VertexArray& vao, int binding)=0; //!< adds the renderbuffer buffers onto the vao starting with binding id binding

    virtual void cacheOnHost()=0; //!< cache the current buffers data on the host
    virtual void pushCachToDevice()=0; //!< write changes from the local cache back to the device
};

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
class Grid : public GridBase
{
public:
    using BufferType = GridBuffer<GridAttribs...>;
    using RenderBufferType = RenderBuffer<typename GridAttribs::RenderType ...>;
    using ReferenceType = GridReference<typename GridAttribs::ReferenceType...>;

    explicit Grid(int numCells=1);

    // copy and move constructor (copy swap idom)
    Grid(const Grid& other);
    Grid(Grid&& other) noexcept;
    Grid& operator=(Grid other) noexcept;

    void swapBuffer() override; //!< swap working buffers, the old write buffer becomes the read buffer
    void swapAndRender() override; //!< swap and ready the current buffer for rendering
    void swapAndRenderWait() override; //!< swap and ready the current buffer for rendering, make sure no unrendered data is discarded

    bool newRenderDataReady() override; //!< the renderbuffer has new data to render
    void startRendering() override; //!< lock access to the renderbuffer. Blocks until readyToRender is true. Data in the renderbuffer will be valid until renderDone() was called
    void renderDone() override; //!< indicates render buffer can be overwritten

    void bindRenderBuffer(GLuint binding, GLenum target) override; //!< bind the renderbuffer to target starting with binding id binding
    void addRenderBufferToVao(mpu::gph::VertexArray& vao, int binding) override; //!< adds the renderbuffer buffers onto the vao starting with binding id binding

    void cacheOnHost() override; //!< cache the current buffers data on the host
    void pushCachToDevice() override; //!< write changes from the local cache back to the device

    template <AT Param>
    auto read(int cellId); //!< read data from grid cell cellId parameter Param at time t
    template <AT Param>
    auto readNext(int cellId); //!< read data from grid cell cellId parameter Param at time t +1. Beware of possible race conditions when also writing to the time t+1 buffer!
    template <AT Param>
    auto readPrev(int cellId); //!< read data from grid cell cellId parameter Param at time t-1
    template <AT Param, typename T>
    void write(int cellId, T&& data); //!< write data to grid cell cellId parameter Param at time t+1
    template <AT Param, typename T>
    void writeCurrent(int cellId, T&& data); //!< write data to grid cell cellId parameter Param at time t. Beware of possible race conditions when also reading from the time t buffer!
    template <AT Param>
    void copy(int cellId); //!< copy data from the read to the write grid
    template <AT Param, typename T>
    void initialize(int cellId, T&& data); //!< write data to grid cell cellId parameter Param in all used buffers (t-1, t, t+1, renderAwait). Beware of possible race conditions when also reading from the time t or t-1 buffer!

    int size() const; //!< returns the number of available grid cells

    ReferenceType getGridReference(); //!< get a grid reference object for use in device code

    // for copy swap idom
    friend void swap(Grid& first, Grid& second) //!< swap two instances of buffer
    {
        using std::swap;

        swap(first.m_numCells,second.m_numCells);

        swap(first.m_buffers[0],second.m_buffers[0]);
        swap(first.m_buffers[1],second.m_buffers[1]);
        swap(first.m_buffers[2],second.m_buffers[2]);
        swap(first.m_buffers[3],second.m_buffers[3]);
        swap(first.m_renderBuffer,second.m_renderBuffer);

        swap(first.m_readBuffer , second.m_readBuffer );
        swap(first.m_writeBuffer , second.m_writeBuffer );
        swap(first.m_previousBuffer , second.m_previousBuffer );
        swap(first.m_renderAwaitBuffer , second.m_renderAwaitBuffer );
        swap(first.m_unusedBuffer , second.m_unusedBuffer );

        bool b = first.m_renderbufferNotRendered;
        first.m_renderbufferNotRendered = second.m_renderbufferNotRendered.load();
        second.m_renderbufferNotRendered = b;
        b = first.m_newRenderdataWaiting;
        first.m_newRenderdataWaiting = second.m_newRenderdataWaiting.load();
        second.m_newRenderdataWaiting = b;
    }

    friend class GridReference<typename GridAttribs::ReferenceType...>; //!< reference type needs to be friends

private:
    int m_numCells; //!< number of grid cells

    int m_readBuffer; //!< the buffer data is read from (stores values at t)
    int m_writeBuffer; //!< the buffer data is written to (stores values at t+1)
    int m_previousBuffer; //!< the buffer data was read from previously (stores values at t-1)

    int m_renderAwaitBuffer; //!< data that will be copied to the openGL buffer on the rendering GPU
    int m_unusedBuffer; //!< when render await buffer == previous buffer one buffer is unused

    BufferType m_buffers[4]; //!< buffers for cuda grid data
    RenderBufferType m_renderBuffer; //!< openGL buffer to render from

    std::atomic_bool m_renderbufferNotRendered{false}; //!< indicates that renderbuffer contains data that have not been rendered yet
    std::atomic_bool m_newRenderdataWaiting{false}; //!< indicate new renderdata are ready to be written to the renderbuffer

    std::mutex m_rbuMtx; //!< renderbuffer mutex
    std::mutex m_rabuMtx; //!< renderAwaitBuffer mutex

    void prepareForRendering(); //!< copies data from renderAwaitBuffer to renderBuffer
};

// include forward defined classes
#include "GridReference.h"

// template function definitions of the Grid class
//-------------------------------------------------------------------

template <typename ...GridAttribs>
template <AT Param>
auto Grid<GridAttribs...>::read(int cellId)
{
    return m_buffers[m_readBuffer].read<Param>(cellId);
}

template <typename... GridAttribs>
template <AT Param>
auto Grid<GridAttribs...>::readNext(int cellId)
{
    return m_buffers[m_writeBuffer].read<Param>(cellId);
}

template <typename... GridAttribs>
template <AT Param>
auto Grid<GridAttribs...>::readPrev(int cellId)
{
    return m_buffers[m_previousBuffer].read<Param>(cellId);
}

template <typename ...GridAttribs>
template <AT Param, typename T>
void Grid<GridAttribs...>::write(int cellId, T&& data)
{
    m_buffers[m_writeBuffer].write<Param>(cellId, std::forward<T>(data));
}

template <typename... GridAttribs>
template <AT Param, typename T>
void Grid<GridAttribs...>::writeCurrent(int cellId, T&& data)
{
    m_buffers[m_readBuffer].write<Param>(cellId, std::forward<T>(data));
}

template <typename... GridAttribs>
template <AT Param>
void Grid<GridAttribs...>::copy(int cellId)
{
    auto data = read<Param>(cellId);
    write<Param>(cellId,data);
}

template <typename... GridAttribs>
template <AT Param, typename T>
void Grid<GridAttribs...>::initialize(int cellId, T&& data)
{
    m_buffers[0].write<Param>(cellId, std::forward<T>(data));
    m_buffers[1].write<Param>(cellId, std::forward<T>(data));
    m_buffers[2].write<Param>(cellId, std::forward<T>(data));
    m_buffers[3].write<Param>(cellId, std::forward<T>(data));
}

template <typename ...GridAttribs>
Grid<GridAttribs...>::Grid(int numCells)
    : m_buffers{ Grid<GridAttribs...>::BufferType(numCells), Grid<GridAttribs...>::BufferType(numCells),
                 Grid<GridAttribs...>::BufferType(numCells), Grid<GridAttribs...>::BufferType(numCells)},
    m_numCells(numCells), m_renderBuffer(numCells)
{
    assert_critical(numCells>0,"Grid","Number of cells must be at least one");
    m_writeBuffer = 3;
    m_readBuffer  = 2;
    m_previousBuffer = 1;
    m_renderAwaitBuffer = 0;
    m_unusedBuffer = -1;
}

template <typename... GridAttribs>
Grid<GridAttribs...>::Grid(const Grid& other)
    : m_buffers{ other.m_buffers[0], other.m_buffers[1],
                 other.m_buffers[2], other.m_buffers[3]},
      m_readBuffer(other.m_readBuffer),
      m_writeBuffer(other.m_writeBuffer),
      m_previousBuffer(other.m_previousBuffer),
      m_renderAwaitBuffer(other.m_renderAwaitBuffer),
      m_unusedBuffer(other.m_unusedBuffer),
      m_renderBuffer(other.m_renderBuffer),
      m_renderbufferNotRendered(other.m_renderbufferNotRendered.load()),
      m_newRenderdataWaiting(other.m_newRenderdataWaiting.load()),
      m_rbuMtx(),
      m_rabuMtx(),
      m_numCells(other.m_numCells)
{
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
    int tmp = m_writeBuffer;

    // make sure not to overwrite the render await buffer
    if(m_previousBuffer == m_renderAwaitBuffer)
    {
        m_writeBuffer = m_unusedBuffer;
        m_unusedBuffer = -1;
    } else
    {
        m_writeBuffer = m_previousBuffer;
    }
    m_previousBuffer = m_readBuffer;
    m_readBuffer = tmp;
}

template <typename ...GridAttribs>
void Grid<GridAttribs...>::swapAndRender()
{
    std::unique_lock<std::mutex> lck(m_rabuMtx);

    int tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_previousBuffer == m_renderAwaitBuffer)
        m_writeBuffer = m_unusedBuffer;
    else
        m_writeBuffer = m_previousBuffer;

    // do not change the unused buffer if we render muliple times in a row
    if(m_renderAwaitBuffer != m_readBuffer)
        m_unusedBuffer = m_renderAwaitBuffer;

    m_previousBuffer = m_readBuffer;
    m_renderAwaitBuffer = tmp;
    m_readBuffer = tmp;

    /*      i s s r s s r r r s r
    write   3 1 2 3 1 0 3 1 0 3 2
    Read    2 3 1 2 3 1 0 3 1 0 3
    prev    1 2 3 1 2 3 1 0 3 1 0

    render  0 0 0 2 2 2 0 3 1 1 3
    unused  - - - 0 0 - 2 2 2 2 1

    */

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

    int tmp = m_writeBuffer;
    // make sure not to overwrite the render await buffer
    if(m_previousBuffer == m_renderAwaitBuffer)
        m_writeBuffer = m_unusedBuffer;
    else
        m_writeBuffer = m_previousBuffer;

    // do not change the unused buffer if we render muliple times in a row
    if(m_renderAwaitBuffer != m_readBuffer)
        m_unusedBuffer = m_renderAwaitBuffer;

    m_previousBuffer = m_readBuffer;
    m_renderAwaitBuffer = tmp;
    m_readBuffer = tmp;

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
    m_renderBuffer.write( m_buffers[m_renderAwaitBuffer]);

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
int Grid<GridAttribs...>::size() const
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

template <typename... GridAttribs>
Grid<GridAttribs...>::ReferenceType Grid<GridAttribs...>::getGridReference()
{
    return Grid::ReferenceType(*this);
}

// template function definitions of the Grid class
//-------------------------------------------------------------------

// declare and precompile some grid types

using RenderDemoGrid = Grid<GridDensity,GridVelocityX,GridVelocityY>;
extern template class Grid<GridDensity,GridVelocityX,GridVelocityY>;

using TestSimGrid = Grid<GridDensity, GridVelocityX, GridVelocityY, GridDensityGradX, GridDensityGradY, GridDensityLaplace, GridVelocityDiv, GridVelocityCurl, GridTemperature, GridTemperatureGradX, GridTemperatureGradY>;
extern template class Grid<GridDensity, GridVelocityX, GridVelocityY, GridDensityGradX, GridDensityGradY, GridDensityLaplace, GridVelocityDiv, GridVelocityCurl, GridTemperature, GridTemperatureGradX, GridTemperatureGradY>;


#endif //CIRCULATION_GRID_H
