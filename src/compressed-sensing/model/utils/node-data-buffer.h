/**
* \file node-data-buffer-meta.h
*
* \author Tobias Waurick
* \date 04.06.17
*
*/

#ifndef NODE_DATABUFFER_H
#define NODE_DATABUFFER_H

#include "ns3/core-module.h"
#include <armadillo>
#include "assert.h"
using namespace arma;
/**
* \class NodeDataBuffer
*
* \brief a buffer template to save data(considered as a vector) from a node to a matrix
*
* A storage buffer template for data from a sensor node. 
* Data is assumed to be vectorized and so multiple data vectors are stored in matrix form (MxN).
* The buffer matrix can be written sequentially rowwise by arma::Col vectors or by common buffers.
* Rows can be written in several consequent writing operations(to make data splitting possible),
* BUT a row must be filled entirely before setting the next.
* Buffered data can be read columwise, as a whole matrix or into a buffer(column by column order)
* \tparam T		data type
*
* \author Tobias Waurick
* \date 04.06.17
*/
template <typename T>
class NodeDataBuffer : public ns3::Object
{
  public:
	/**
	* \brief creates an empty NodeDataBuffer
	*
	*/
	NodeDataBuffer();

	/**
	* \brief creates a NodeDataBuffer
	*
	* \param m 	NOF rows   (= maximum nof data samples)
	* \param n	NOF colums (=length of each data vector)		
	*
	*/
	NodeDataBuffer(uint32_t m, uint32_t n);

	/**
	* \brief writes data to this matrix buffer
	*
	* \param vect data vector
	* \param meta meta information vector
	*
	* \return remaining size of row to be filled
	*/
	uint32_t WriteData(const Row<T> &vect);

	/**
	* \brief writes data from a buffer to this matrix buffer (also over multiple colums)
	*
	* \param buffer pointer to data buffer
	* \param bufSize size of buffer
	* \param meta pointer to meta information buffer
	* \param metaSize size of buffer
	*
	* \return remaining size of internal buffer matrix
	*/
	uint32_t WriteData(const T *buffer, const uint32_t &bufSize);

	/**
	* \brief writes a single data value to this matrix buffer
	*
	* \param data data value
	* \param meta meta information
	*
	* \return remaining size of row to be filled
	*/
	uint32_t WriteData(const T &data);

	/**
	* \brief checks if storage is full
	*
	* \return true if it  is full
	*/
	bool IsFull() const;

	/**
	* \brief check how many rows were written
	*
	* \return NOF rows written
	*/
	uint32_t GetWrRow() const;

	/**
	* \brief check colums written for current row
	*
	* \return NOF cols written
	*/
	uint32_t GetWrCol() const;

	/**
	* \brief checks NOF of elements written (considering full rows only)
	*
	* \return NOF elements
	*/
	uint32_t GetWrElem() const;

	/**
	* \brief reads the buffer matrix to an external buffer(ordered column by column)
	*
	*/
	void ReadBuf(T *buffer, const uint32_t& bufSize);

	/**
	* \brief reads a column from the buffer
	*
	* \param colIdx column index
	*
	* \return column vector
	*/
	Col<T> ReadCol(uint32_t colIdx) const;

	/**
	* \brief reads all written data
	*
	* \return the buffer (sub)matrix
	*/
	Mat<T> ReadAll() const;

	/**
	* \brief write a entire matrix to buffer, which becomes the new buffer matrix
	*
	* \param mat input matrix
	*
	*/
	void WriteAll(const Mat<T> &mat);

	/**
	* \brief check if buffer was used before
	*
	* \return true if the buffer is empty
	*/
	bool IsEmpty() const;

	/**
	* \brief resets the buffer
	*
	*/
	void Reset();

	/**
	* \brief resets and resizes the buffer
	*
	*/
	void Resize(uint32_t m, uint32_t n);

	/**
	* \brief gets the dimensions of internal matrix buffer
	*
	* \return a Col-vector size 2 containing [nRows, nCols]
	*/
	Col<uint32_t> GetDimensions() const;

	/**
	* \brief gets the NOF rows of internal matrix buffer
	*
	* \return NOF rows
	*/
	uint32_t nRows() const;

	/**
	* \brief gets the NOF columns of internal matrix buffer
	*
	* \return NOF colums
	*/
	uint32_t nCols() const;

  private:
	uint32_t m_nCol,	 /**< NOF columns*/
		m_nRow;			 /**< NOF rows*/
	Mat<T> m_dataMat;	/**< saved data in matrix form*/
	uint32_t m_colWrIdx, /**< write index for current column*/
		m_rowWrIdx;		 /**< index of current row to which is written*/
	bool m_isFull;		 /**< true when buffer is full*/
};

template <typename T>
NodeDataBuffer<T>::NodeDataBuffer() : m_nCol(0),
									  m_nRow(0),
									  m_dataMat(),
									  m_colWrIdx(0),
									  m_rowWrIdx(0),
									  m_isFull(false)
{
}

template <typename T>
NodeDataBuffer<T>::NodeDataBuffer(uint32_t m, uint32_t n) : m_nCol(n),
															m_nRow(m),
															m_dataMat(m, n),
															m_colWrIdx(0),
															m_rowWrIdx(0),
															m_isFull(false)
{
	m_dataMat.zeros();
}

template <typename T>
uint32_t NodeDataBuffer<T>::WriteData(const Row<T> &vect)
{
	NS_ASSERT_MSG(!m_isFull, "Buffer is already full.");
	uint32_t vectSize = vect.n_elem;

	uint32_t maxIdx = vectSize - 1 + m_colWrIdx,
			 minIdx = m_colWrIdx;
	NS_ASSERT_MSG(vectSize <= m_nCol, "Vector is greater than the maximum row size/nof columns!");
	NS_ASSERT_MSG(maxIdx < m_nCol, "Vector is greater than remaining space of current row!");
	m_dataMat(m_rowWrIdx, span(minIdx, maxIdx)) = vect;

	m_colWrIdx += vectSize;
	//check if row was written entirely
	if (m_nCol == m_colWrIdx)
	{
		m_rowWrIdx++;
		if (m_rowWrIdx == m_nRow)
			m_isFull = true;
		m_colWrIdx = 0;
		return 0;
	}

	return (m_nCol - m_colWrIdx);
}

template <typename T>
uint32_t NodeDataBuffer<T>::WriteData(const T &data)
{
	Row<T> dataVect(1);
	dataVect << data;
	return WriteData(dataVect);
}

template <typename T>
uint32_t NodeDataBuffer<T>::WriteData(const T *buffer, const uint32_t& bufSize)
{
	NS_ASSERT(buffer); //null pointer check
	//calculate remaining space
	uint32_t space = (m_nRow - m_rowWrIdx) * (m_nCol) + m_nCol - m_colWrIdx;
	NS_ASSERT_MSG(bufSize <= space, "Not enough space in buffer!");

	// uint32_t nElem;
	// if (m_rowWrIdx) //fill current row(if necessary)
	// {
	// 	nElem = m_nRow-m_rowWrIdx;

	// 	Row<T> dataVect(buffer, nElem, false, true); // speed up: use memory of buffer, since we copy only anyway
	// 	WriteData(dataVect);
	// 	bufSize -= nElem;
	// 	buffer += nElem;
	// 	meta += nElem;
	// }
	// if (bufSize)// TODO -> USE MATRIX instead of for
	// {
	// 	uint32_t nRow = bufSize / space; // NOF full(!) rows to write
	// 	nElem = m_nRow;
	// 	for (uint32_t row = 0; row < nRow; row++)
	// 	{
	// 		Row<T> dataVect(buffer, nElem, false, true); // speed up: use memory of buffer, since we copy only anyway
	// 		WriteData(dataVect);
	// 		bufSize -= nElem;
	// 		buffer += nElem;
	// 		meta += nElem;
	// 	}
	// 	 // start writing new incomplete
	// }
	// if (bufSize)
	// {
	// 	nElem = bufSize;
	// 	Row<T> dataVect(buffer, nElem, false, true); // speed up: use memory of buffer, since we copy only anyway
	// 	WriteData(dataVect);
	// 	bufSize -= nElem;
	// 	buffer += nElem;
	// 	meta += nElem;
	// }

	T *matMem_ptr = m_dataMat.memptr(); // here data is stored in a column by column order
										//	T *bufEnd_ptr = buffer + bufSize;
	for (uint32_t i = 0; i < bufSize; i++)
	{
		uint32_t matMemStart = m_nRow * m_colWrIdx + m_rowWrIdx;
		*(matMem_ptr + matMemStart) = *(buffer + i);

		m_colWrIdx++; //check if row was written entirely
		if (m_nCol == m_colWrIdx)
		{
			m_rowWrIdx++;
			if (m_rowWrIdx == m_nRow)
				m_isFull = true;
			m_colWrIdx = 0;
		}
	}
	space -= bufSize;
	return space;
}

template <typename T>
bool NodeDataBuffer<T>::IsFull() const
{
	return m_isFull;
}

template <typename T>
uint32_t NodeDataBuffer<T>::GetWrRow() const
{
	if (IsEmpty())
		return 0;
	return m_rowWrIdx - 1;
}

template <typename T>
uint32_t NodeDataBuffer<T>::GetWrCol() const
{
	if (IsEmpty())
		return 0;
	return m_colWrIdx - 1;
}

template <typename T>
uint32_t NodeDataBuffer<T>::GetWrElem() const
{
	if (IsEmpty())
		return 0;
	return m_rowWrIdx * m_nCol;
}

template <typename T>
Col<T> NodeDataBuffer<T>::ReadCol(uint32_t colIdx) const
{
	NS_ASSERT_MSG(colIdx < m_nCol, "Index exceeding NOF columns");
	return m_dataMat.col(colIdx);
}

template <typename T>
Mat<T> NodeDataBuffer<T>::ReadAll() const
{
	if (IsEmpty()) //if no colums have been written yet, return empty matrix
	{
		return Mat<T>();
	}
	return m_dataMat.rows(0, m_rowWrIdx - 1);
}

template <typename T>
void NodeDataBuffer<T>::ReadBuf(T *buffer, const uint32_t &bufSize)
{
	NS_ASSERT_MSG(bufSize == GetWrElem(), "NOF elements not matching!");

	Mat<T> subMat = ReadAll();
	std::copy(subMat.memptr(), subMat.memptr()+bufSize, buffer);
}

template <typename T>
void NodeDataBuffer<T>::WriteAll(const Mat<T> &mat)
{
	m_nCol = mat.n_cols;
	m_nRow = mat.n_rows;
	m_dataMat = mat;
	m_rowWrIdx = m_nRow;
	m_colWrIdx = m_nCol;
	m_isFull = true;
}

template <typename T>
bool NodeDataBuffer<T>::IsEmpty() const
{
	return !(m_rowWrIdx);
}

template <typename T>
void NodeDataBuffer<T>::Reset()
{
	m_dataMat.zeros();
	m_colWrIdx = 0;
	m_rowWrIdx = 0;
	m_isFull = false;
}

template <typename T>
void NodeDataBuffer<T>::Resize(uint32_t m, uint32_t n)
{
	m_nCol = n;
	m_nRow = m;
	m_dataMat.set_size(m, n);
	Reset();
}

template <typename T>
Col<uint32_t> NodeDataBuffer<T>::GetDimensions() const
{
	Col<uint32_t> size(2);
	size(0) = m_nRow;
	size(1) = m_nCol;
	return size;
}

template <typename T>
uint32_t NodeDataBuffer<T>::nRows() const
{
	return m_nRow;
}

template <typename T>
uint32_t NodeDataBuffer<T>::nCols() const
{
	return m_nCol;
}
#endif //NODE_DATABUFFER_H