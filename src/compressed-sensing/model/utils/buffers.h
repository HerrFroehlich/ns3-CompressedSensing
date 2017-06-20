/**
* \file SerialDataBuffers.h
*
* \author Tobias Waurick
* \date 30.05
*
* Includes several SerialDataBuffer templates
*/
#ifndef UTILBUFFERS_H
#define UTILBUFFERS_H

#include <stdint.h>
#include "ns3/object.h"
#include "assert.h"

/**
* \class SerialDataBuffer
*
* \brief a general buffer class to store and read data
*		
*
* \tparam T	the type of stored data
*
* \author Tobias Waurick
* \date 30.05.17
*/
template <typename T>
class SerialDataBuffer<T> : public Object
{
  public:
	/**
	* \brief creates an empty SerialDataBuffer
	*
	*/
	SerialDataBuffer();

	/**
	* \brief creates a SerialDataBuffer with a certain size
	*
	* \param size size of the buff
	*
	*/
	SerialDataBuffer(uint32_t size);

	/**
	* \brief destroy the Biffer
	*
	*/
	~SerialDataBuffer();

	/**
	* \brief reads a value at a certain index
	*
	* \param index of data in buffer to read from
	*
	* \return indexed value in buffer
	*/
	T Read(uint32_t index);

	/**
	* \brief reads multiple values starting at at an index to a buffer 
	*
	* \param index of data to start reading from
	* \param buffer	a pointer to a  buffer  of the stored type
	* \param bufSize	size of that buffer
	*/
	void Read(uint32_t index, *buffer, uint32_t bufSize);

	/**
	* \brief reads next value this SerialDataBuffer
	*
	* \return next value in SerialDataBuffer
	*/
	T ReadNext();

	/**
	* \brief reads multiple next values in a buffer given by a pointer
	*
	* \param buffer	a pointer to a  buffer  of the stored type
	* \param bufSize	size of that buffer
	*/
	void ReadNext(T *SerialDataBuffer, uint32_t bufSize);

	/**
	* \brief writes a single value into this SerialDataBuffer
	*
	* \param data data of underlaying type to store
	*/
	void WriteNext(T data);

	/**
	* \brief writes multiple values at the end of this SerialDataBuffer
	*
	* \param SerialDataBuffer	a SerialDataBuffer pointer of the stored type
	* \param bufSize	size of that SerialDataBuffer
	*/
	void WriteNext(T *SerialDataBuffer, uint32_t bufSize);

	/**
	* \brief determine if the buffer is full
	*
	* \return true if the buffer is full
	*/
	bool IsFull();

	/**
	* \brief clears the buffer by setting the write index to zero
	*
	*/
	void Clear();

	/**
	* \brief deletes and resizes the data buffer
	*
	* \param size size of new buffer
	*
	*/
	void
	Resize(uint32_t size);

  private:
	T *m_data_ptr;		 /**< stored data*/
	uint32_t m_dataSize, /**< size of stored data*/
		m_wrIdx,		 /**< current write index*/
		m_rdIdx;		 /**< current read index*/
};

template <typename T>
SerialDataBuffer<T>::SerialDataBuffer() : m_data_ptr(0),
										  m_dataSize(0),
										  m_wrIdx(0),
										  m_rdIdx(0)
{
}

template <typename T>
SerialDataBuffer<T>::SerialDataBuffer(uint32_t dataSize) : m_dataSize(dataSize),
														   m_wrIdx(0),
														   m_rdIdx(0)
{
	m_data_ptr = new T[dataSize];
}

template <typename T>
SerialDataBuffer<T>::~SerialDataBuffer()
{
	delete[] m_data_ptr;
	m_data_ptr = 0;
	m_dataSize = 0;
}

template <typename T>
T SerialDataBuffer<T>::Read(uint32_t index)
{
	NS_ASSERT_MSG(index <= m_wrIdx, "Read index > write index : Trying to read outside of written area!");

	return m_data_ptr[index];
}

template <typename T>
void SerialDataBuffer<T>::Read(uint32_t index, T *buffer, uint32_t bufSize)
{
	NS_ASSERT_MSG((index + bufSize) <= m_wrIdx,
				  "Read index > write index : Trying to read outside of written area!");

	memcpy(SerialDataBuffer, m_data_ptr + index, bufSize);
}

template <typename T>
T SerialDataBuffer<T>::ReadNext()
{
	return Read(m_rdIdx++);
}

template <typename T>
void SerialDataBuffer<T>::ReadNext(T *buffer, uint32_t bufSize)
{
	Read(m_rdIdx, buffer, bufSize)
		m_rdIdx += bufSize;
}

template <typename T>
void SerialDataBuffer<T>::WriteNext(T data)
{
	NS_ASSERT_MSG(m_wrIdx < m_dataSize, "Write index > data size : Trying to write outside of allocated area!");
	m_data_ptr[m_wrIdx++] = data;
}

template <typename T>
void SerialDataBuffer<T>::WriteNext(T *buffer, uint32_t bufSize)
{
	NS_ASSERT_MSG((m_wrIdx + bufSize - 1) < m_dataSize,
				  "Write index > data size : Trying to write outside of allocated area!");
	memcpy(m_data_ptr + m_wrIdx, buffer, bufSize);
	m_wrIdx += bufSize;
}

template <typename T>
bool SerialDataBuffer<T>::IsFull()
{
	return (m_wrIdx + 1 == m_dataSize);
}

template <typename T>
void SerialDataBuffer<T>::Clear()
{
	m_wrIdx = 0;
	m_rdIdx = 0;
}

template <typename T>
void SerialDataBuffer<T>::Resize(uint32_t size)
{
	delete[] m_data_ptr;
	m_dataSize = size;
	m_wrIdx = 0;
	m_rdIdx = 0;
	m_data_ptr = new T[m_dataSize];
}

#endif //UTILBUFFERS_H
