/*
 * CIRCULATION
 * GridReference.h
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Implements the GridReference class
 *
 * Copyright (c) 2019 Hendrik Schwanekamp
 *
 */

#ifndef CIRCULATION_GRIDREFERENCE_H
#define CIRCULATION_GRIDREFERENCE_H

// includes
//--------------------
#include "Grid.h"
//--------------------

//#define ENABLE_BOUNDS_CHECKING

//-------------------------------------------------------------------
/**
 * @brief References a GridAttribute on the device.
 */
template <AT attributeType, typename T>
class GridAttributeReference
{
public:
    explicit GridAttributeReference( GridAttribute<attributeType,T>* attrib ) : m_data(attrib->m_data.getVectorReference()) {}

    CUDAHOSTDEV T read(int cellId)
    {
    #if defined(ENABLE_BOUNDS_CHECKING)
        return m_data.at(cellId);
    #else
        return m_data[cellId];
    #endif
    }

    template <typename Tin>
    CUDAHOSTDEV void write(int cellId, Tin&& data)
    {
    #if defined(ENABLE_BOUNDS_CHECKING)
        m_data.at(cellId) = std::move<Tin>(data);
    #else
        m_data[cellId] = std::move<Tin>(data);
    #endif

    }

    static constexpr AT type = attributeType;
    using ReferencedType = GridAttribute<attributeType, T>;

private:
    mpu::VectorReference<T> m_data;
};

//-------------------------------------------------------------------
/**
 * @brief references a GridBuffer on the device
 */
template <typename ...AttribRefs>
class GridBufferReference : AttribRefs...
{
public:
    explicit GridBufferReference( GridBuffer<typename AttribRefs::ReferencedType...>& buffer )
        : AttribRefs(static_cast<typename AttribRefs::ReferencedType *>(&buffer))... {};

    template <AT Param>
    CUDAHOSTDEV auto read(int cellId);
    template <AT Param, typename T>
    CUDAHOSTDEV void write(int cellId, T&& data);
};

// template function definitions of the GridBufferReference class
//-------------------------------------------------------------------
template <typename... Attributes>
template <AT Param>
CUDAHOSTDEV auto GridBufferReference<Attributes...>::read(int cellId)
{
    return GridAttributeSelector_t<Param,Attributes...>::read(cellId);
}

template <typename... Attributes>
template <AT Param, typename T>
CUDAHOSTDEV void GridBufferReference<Attributes...>::write(int cellId, T&& data)
{
    GridAttributeSelector_t<Param,Attributes...>::write(cellId, std::forward<T>(data));
}


//-------------------------------------------------------------------
/**
 * class GridReference
 *
 * usage:
 *
 */
template <typename ...AttribRefs>
class GridReference
{
public:
    using BufferType = GridBufferReference<AttribRefs...>;

    explicit GridReference( Grid<typename AttribRefs::ReferencedType...>& grid)
        : m_readBuffer(grid.m_buffers[grid.m_readBuffer]), m_writeBuffer(grid.m_buffers[grid.m_writeBuffer]),
          m_previousBuffer(grid.m_buffers[grid.m_previousBuffer]), m_numGridcells(grid.size()) {}

    template <AT Param>
    CUDAHOSTDEV auto read(int cellId); //!< read data from grid cell cellId parameter Param at time t
    template <AT Param>
    CUDAHOSTDEV auto readNext(int cellId); //!< read data from grid cell cellId parameter Param at time t +1. Beware of possible race conditions when also writing to the time t+1 buffer!
    template <AT Param>
    CUDAHOSTDEV auto readPrev(int cellId); //!< read data from grid cell cellId parameter Param at time t-1
    template <AT Param, typename T>
    CUDAHOSTDEV void write(int cellId, T&& data); //!< write data to grid cell cellId parameter Param at time t+1
    template <AT Param, typename T>
    CUDAHOSTDEV void writeCurrent(int cellId, T&& data); //!< write data to grid cell cellId parameter Param at time t. Beware of possible race conditions when also reading from the time t buffer!
    template <AT Param>
    CUDAHOSTDEV void copy(int cellId); //!< copy data from the read to the write grid


    CUDAHOSTDEV int size(); //!< number of grid cells

private:
    int m_numGridcells;
    BufferType m_readBuffer;
    BufferType m_previousBuffer;
    BufferType m_writeBuffer;
};

// template function definitions of the Grid reference class
//-------------------------------------------------------------------

template <typename... AttribRefs>
template <AT Param>
auto GridReference<AttribRefs...>::read(int cellId)
{
    return m_readBuffer.read<Param>(cellId);
}

template <typename... AttribRefs>
template <AT Param>
auto GridReference<AttribRefs...>::readNext(int cellId)
{
    return m_writeBuffer.read<Param>(cellId);
}

template <typename... AttribRefs>
template <AT Param>
auto GridReference<AttribRefs...>::readPrev(int cellId)
{
    return m_previousBuffer.read<Param>(cellId);
}

template <typename... AttribRefs>
template <AT Param, typename T>
void GridReference<AttribRefs...>::write(int cellId, T&& data)
{
    m_writeBuffer.write<Param>(cellId,data);
}

template <typename... AttribRefs>
template <AT Param>
void GridReference<AttribRefs...>::copy(int cellId)
{
    auto data = read<Param>(cellId);
    write<Param>(cellId,data);
}

template <typename... AttribRefs>
template <AT Param, typename T>
void GridReference<AttribRefs...>::writeCurrent(int cellId, T&& data)
{
    m_readBuffer.write<Param>(cellId,data);
}

template <typename... AttribRefs>
int GridReference<AttribRefs...>::size()
{
    return m_numGridcells;
}


#endif //CIRCULATION_GRIDREFERENCE_H
