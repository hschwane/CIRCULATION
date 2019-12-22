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

//-------------------------------------------------------------------
/**
 * @brief References a GridAttribute on the device.
 */
template <AT attributeType, typename T>
class GridAttributeReference
{
public:
    CUDAHOSTDEV explicit GridAttributeReference(const GridAttribute<attributeType,T>& attrib ) : m_data(attrib) {}

    CUDAHOSTDEV T read(int cellId) {return m_data[cellId];}
    template <typename Tin>
    CUDAHOSTDEV void write(int cellId, Tin&& data)
    {
        m_data[cellId] = std::move<Tin>(data);
    }

    static constexpr AT type = attributeType;
    using ReferencedType = GridAttribute<attributeType, T>;
    friend class RenderAttribute<attributeType,T>;

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
    explicit GridBufferReference(const GridBuffer<typename AttribRefs::ReferencedType...>& buffer )
        : AttribRefs(static_cast<typename AttribRefs::ReferencedType &>(buffer))... {};

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

    explicit GridReference(const Grid<typename AttribRefs::ReferencedType...>& grid)
        : m_readBuffer(*grid.m_readBuffer,*grid.m_writeBuffer) {}

    template <AT Param>
    CUDAHOSTDEV auto read(int cellId); //!< read data from grid cell cellId parameter Param
    template <AT Param, typename T>
    CUDAHOSTDEV void write(int cellId, T&& data); //!< read data from grid cell cellId parameter Param

    CUDAHOSTDEV int size(); //!< number of grid cells

private:
    int m_numGridcells;
    BufferType m_readBuffer;
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
template <AT Param, typename T>
void GridReference<AttribRefs...>::write(int cellId, T&& data)
{
    m_writeBuffer.write<Param>(cellId,data);
}

template <typename... AttribRefs>
int GridReference<AttribRefs...>::size()
{
    return m_numGridcells;
}


#endif //CIRCULATION_GRIDREFERENCE_H
