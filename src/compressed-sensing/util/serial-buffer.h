/**
* \file serial-buffer.h
*
* \author Tobias Waurick
* \date 30.05.17
*
*/
#ifndef SERIALBUFFER_H
#define SERIALBUFFER_H

#include <stdint.h>
#include <vector>
#include "ns3/simple-ref-count.h"
#include "assert.h"
#include <algorithm> //copy
/**
* \ingroup compsens
 * \defgroup util Utilities
 *
 * Various utility classes like buffers...
 */
/**
* \ingroup util
* \class SerialDataBuffer
*
* \brief a general buffer class to store and read data
*
* This buffer can write and read data serially.
* If copied or assigned from another SerialDataBuffer data and the write index are duplicated,
* but the read index is set to the beginning (0).		
*
* \tparam T	the type of stored data
*
* \author Tobias Waurick
* \date 30.05.17
*/
template <typename T>
class SerialDataBuffer : public ns3::Object
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
	* \brief copy constructor
	*
	* \param buf buffer to copy
	*
	*/
	SerialDataBuffer(const SerialDataBuffer &buf);

	/**
	* \brief reates a SerialDataBuffer and moves the memory from the buffer
	* The buffer pointer will be set to zero after memory has been moved, to avoid delete conflicts.
	*
	* \param buf buffer to copy
	*
	*/
	SerialDataBuffer(T *&buffer, uint32_t bufSize);

	/**
	* \brief destroy the Buffer
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
	T Read(uint32_t index) const;

	/**
	* \brief reads multiple values starting at at an index to a buffer 
	*
	* \param index of data to start reading from
	* \param buffer	a pointer to a  buffer  of the stored type
	* \param bufSize	size of that buffer
	*/
	void Read(uint32_t index, T *buffer, uint32_t bufSize) const;

	/**
	* \brief reads next value this SerialDataBuffer
	*
	* \return next value in SerialDataBuffer
	*/
	T ReadNext() const;

	/**
	* \brief reads multiple next values in a buffer given by a pointer
	*
	* \param buffer	a pointer to a  buffer  of the stored type
	* \param bufSize	size of that buffer
	*/
	void ReadNext(T *buffer, uint32_t bufSize) const;

	/**
	* \brief Moves the memory from a buffer to this serial buffer
	* BE SURE TO ALLOCATE THE BUFFER ON THE HEAP!
	* The buffer pointer will be set to zero after memory has been moved, to avoid delete conflicts.
	*
	* \param buffer 	a reference to a pointer to a  buffer  of the stored type
	* \param bufSize	size of that buffer
	*
	*/
	void MoveMem(T *&buffer, uint32_t bufSize);

	/**
	* \brief gets a constant pointer to underlying memory
	*
	* \return  constant pointer to the memory
	*/
	const T *GetMem() const;

	/**
	* \brief writes a single value into this SerialDataBuffer
	*
	* \param data data of underlaying type to store
	*/
	void WriteNext(T data);

	/**
	* \brief writes vector to this SerialDataBuffer
	*
	* \param vec vector containing the data
	*/
	void WriteNext(const std::vector<T> &vec);

	/**
	* \brief writes multiple values at the end of this SerialDataBuffer
	*
	* \param buffer	a SerialDataBuffer pointer of the stored type
	* \param bufSize	size of that SerialDataBuffer
	*/
	void WriteNext(const T *buffer, uint32_t bufSize);

	/**
	* \brief determine if the buffer is full
	*
	* \return true if the buffer is full
	*/
	bool IsFull() const;

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
	void Resize(uint32_t size);

	/**
	* \brief gets the space remaining in the buffer for writting
	* \return space
	*/
	uint32_t GetSpace() const;

	/**
	* \brief gets the remaining NOF elements for reading
	*
	* \return NOF remaining elements
	*/
	uint32_t GetRemaining() const;

	/**
	* \brief gets the total size of the buffer
	*
	* \return total size
	*/
	uint32_t GetSize() const;

	SerialDataBuffer<T> &operator=(const SerialDataBuffer<T> &other)
	{
		if (m_data_ptr)
			delete[] m_data_ptr;

		m_dataSize = other.m_dataSize;
		m_data_ptr = new T[m_dataSize];
		m_wrIdx = other.m_wrIdx;
		m_rdIdx = 0;
		std::copy(std::begin(other.m_data_ptr), std::end(other.m_data_ptr), std::begin(m_data_ptr));

		return *this;
	};

  private:
	T *m_data_ptr;			  /**< stored data*/
	uint32_t m_dataSize,	  /**< size of stored data*/
		m_wrIdx;			  /**< current write index*/
	mutable uint32_t m_rdIdx; /**< current read index*/
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
SerialDataBuffer<T>::SerialDataBuffer(const SerialDataBuffer &buf) : m_rdIdx(0)
{
	m_dataSize = buf.m_dataSize;
	m_data_ptr = new T[m_dataSize];
	m_wrIdx = buf.m_wrIdx;
	std::copy(std::begin(buf.m_data_ptr), std::end(buf.m_data_ptr), std::begin(m_data_ptr));
}

template <typename T>
SerialDataBuffer<T>::SerialDataBuffer(T *&buffer, uint32_t bufSize)
{
	MoveMem(buffer, bufSize);
}

template <typename T>
SerialDataBuffer<T>::~SerialDataBuffer()
{
	delete[] m_data_ptr;
	m_data_ptr = 0;
	m_dataSize = 0;
}

template <typename T>
T SerialDataBuffer<T>::Read(uint32_t index) const
{
	NS_ASSERT_MSG(index <= m_wrIdx, "Read index > write index : Trying to read outside of written area!");

	return m_data_ptr[index];
}

template <typename T>
void SerialDataBuffer<T>::Read(uint32_t index, T *buffer, uint32_t bufSize) const
{
	NS_ASSERT_MSG((index + bufSize) <= m_wrIdx,
				  "Read index > write index : Trying to read outside of written area!");
	std::copy(m_data_ptr + index, m_data_ptr + index + bufSize, buffer);
}

template <typename T>
T SerialDataBuffer<T>::ReadNext() const
{
	return Read(m_rdIdx++);
}

template <typename T>
void SerialDataBuffer<T>::ReadNext(T *buffer, uint32_t bufSize) const
{
	Read(m_rdIdx, buffer, bufSize);
	m_rdIdx += bufSize;
}

template <typename T>
void SerialDataBuffer<T>::MoveMem(T *&buffer, uint32_t bufSize)
{
	delete[] m_data_ptr;
	m_dataSize = bufSize;
	m_wrIdx = bufSize;
	m_rdIdx = 0;

	m_data_ptr = buffer;
	buffer = 0;
}

template <typename T>
const T *SerialDataBuffer<T>::GetMem() const
{
	return const_cast<const T *>(m_data_ptr);
}

template <typename T>
void SerialDataBuffer<T>::WriteNext(T data)
{
	NS_ASSERT_MSG(m_wrIdx < m_dataSize, "Write index > data size : Trying to write outside of allocated area!");
	m_data_ptr[m_wrIdx++] = data;
}

template <typename T>
void SerialDataBuffer<T>::WriteNext(const std::vector<T> &vec)
{
	WriteNext(vec.data(), vec.size());
}

template <typename T>
void SerialDataBuffer<T>::WriteNext(const T *buffer, uint32_t bufSize)
{
	NS_ASSERT_MSG((m_wrIdx + bufSize - 1) < m_dataSize,
				  "Write index > data size : Trying to write outside of allocated area!");
	std::copy(buffer, buffer + bufSize, m_data_ptr + m_wrIdx);
	m_wrIdx += bufSize;
}

template <typename T>
bool SerialDataBuffer<T>::IsFull() const
{
	return (m_wrIdx == m_dataSize);
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

template <typename T>
uint32_t SerialDataBuffer<T>::GetSpace() const
{
	return m_dataSize - m_wrIdx;
}

template <typename T>
uint32_t SerialDataBuffer<T>::GetRemaining() const
{
	return m_wrIdx - m_rdIdx;
}

template <typename T>
uint32_t SerialDataBuffer<T>::GetSize() const
{
	return m_dataSize;
}

#endif //SERIALBUFFER_H
