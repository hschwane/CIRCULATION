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
    GridBuffer() : Attributes()...{};
    explicit GridBuffer(int numCells) : Attributes(numCells)...{};

    template <AT Param>
    auto read(int cellId);
    template <AT Param, typename T>
    void write(int cellId, T&& data);

    friend class RenderBuffer<typename Attributes::RenderType...>;

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
    auto read(int cellId); //!< read data from grid cell cellId parameter Param
    template <AT Param, typename T>
    void write(int cellId, T&& data); //!< read data from grid cell cellId parameter Param

    int size(); //!< returns the number of available grid cells

    // for copy swap idom
    friend void swap(Grid& first, Grid& second) //!< swap two instances of buffer
    {
        using std::swap;

        // swap owned things
        swap(first.m_numCells,second.m_numCells);
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

        // figure out buffer pointer of first element from current values of second
        BufferType* firstUnused;
        BufferType* firstRead;
        BufferType* firstWrite;
        BufferType* firstRenderAwait;

        if(second.m_readBuffer == &second.m_bufferA)
            firstRead = &first.m_bufferA;
        else if(second.m_readBuffer == &second.m_bufferB)
            firstRead = &first.m_bufferB;
        else if(second.m_readBuffer == &second.m_bufferC)
            firstRead = &first.m_bufferC;
        else
            firstRead = nullptr;

        if(second.m_writeBuffer == &second.m_bufferA)
            firstWrite = &first.m_bufferA;
        else if(second.m_writeBuffer == &second.m_bufferB)
            firstWrite = &first.m_bufferB;
        else if(second.m_writeBuffer == &second.m_bufferC)
            firstWrite = &first.m_bufferC;
        else
            firstWrite = nullptr;

        if(second.m_renderAwaitBuffer == &second.m_bufferA)
            firstRenderAwait = &first.m_bufferA;
        else if(second.m_renderAwaitBuffer == &second.m_bufferB)
            firstRenderAwait = &first.m_bufferB;
        else if(second.m_renderAwaitBuffer == &second.m_bufferC)
            firstRenderAwait = &first.m_bufferC;
        else
            firstRenderAwait = nullptr;

        if(second.m_unusedBuffer == &second.m_bufferA)
            firstUnused = &first.m_bufferA;
        else if(second.m_unusedBuffer == &second.m_bufferB)
            firstUnused = &first.m_bufferB;
        else if(second.m_unusedBuffer == &second.m_bufferC)
            firstUnused = &first.m_bufferC;
        else
            firstUnused = nullptr;

        // set new values of second from values of first
        if(first.m_readBuffer == &first.m_bufferA)
            second.m_readBuffer = &second.m_bufferA;
        else if(first.m_readBuffer == &first.m_bufferB)
            second.m_readBuffer = &second.m_bufferB;
        else if(first.m_readBuffer == &first.m_bufferC)
            second.m_readBuffer = &second.m_bufferC;
        else
            second.m_readBuffer = nullptr;

        if(first.m_writeBuffer == &first.m_bufferA)
            second.m_writeBuffer = &second.m_bufferA;
        else if(first.m_writeBuffer == &first.m_bufferB)
            second.m_writeBuffer = &second.m_bufferB;
        else if(first.m_writeBuffer == &first.m_bufferC)
            second.m_writeBuffer = &second.m_bufferC;
        else
            second.m_writeBuffer = nullptr;

        if(first.m_renderAwaitBuffer == &first.m_bufferA)
            second.m_renderAwaitBuffer = &second.m_bufferA;
        else if(first.m_renderAwaitBuffer == &first.m_bufferB)
            second.m_renderAwaitBuffer = &second.m_bufferB;
        else if(first.m_renderAwaitBuffer == &first.m_bufferC)
            second.m_renderAwaitBuffer = &second.m_bufferC;
        else
            second.m_renderAwaitBuffer = nullptr;

        if(first.m_unusedBuffer == &first.m_bufferA)
            second.m_unusedBuffer = &second.m_bufferA;
        else if(first.m_unusedBuffer == &first.m_bufferB)
            second.m_unusedBuffer = &second.m_bufferB;
        else if(first.m_unusedBuffer == &first.m_bufferC)
            second.m_unusedBuffer = &second.m_bufferC;
        else
            second.m_unusedBuffer = nullptr;

        // set values for first buffer
        first.m_unusedBuffer = firstUnused;
        first.m_readBuffer = firstRead;
        first.m_writeBuffer = firstWrite;
        first.m_renderAwaitBuffer = firstRenderAwait;
    }

    template <typename ...AttribRefs> class GridReference;

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

// include forward defined classes
#include "GridReference.h"

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
    assert_critical(numCells>0,"Grid","Number of cells must be at least one");
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
