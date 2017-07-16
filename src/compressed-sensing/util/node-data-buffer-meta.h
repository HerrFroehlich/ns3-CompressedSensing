/**
* \file node-data-buffer-meta.h
*
* \author Tobias Waurick
* \date 04.06.17
*
*/

#ifndef NODE_DATABUFFERMETA_H
#define NODE_DATABUFFERMETA_H

#include "ns3/core-module.h"
#include <armadillo>
#include "assert.h"
using namespace arma;
/**
* \class NodeDataBufferMeta
*
* \brief a buffer template to save data(considered as a vector) from a node to a matrix
*
* A storage buffer template for data from a sensor node. 
* Data is assumed to be vectorized and so multiple data vectors are stored in matrix form (MxN).
* For each matrix entry meta data is provided.
* The buffer matrix can be written sequentially rowwise by arma::Col vectors or by common buffers.
* Rows can be written in several consequent writing operations(to make data splitting possible),
* BUT a row must be filled entirely before setting the next.
* \tparam T		data type
* \tparam TM	type of meta data
*
* \author Tobias Waurick
* \date 04.06.17
*/
template <typename T, typename TM>
class NodeDataBufferMeta : public ns3::Object
{
  public:
	/**
	* \brief creates an empty NodeDataBufferMeta
	*
	*/
	NodeDataBufferMeta();

	/**
	* \brief creates a NodeDataBufferMeta
	*
	* \param m 	NOF rows   (= maximum nof data samples)
	* \param n	NOF colums (=length of each data vector)		
	*
	*/
	NodeDataBufferMeta(uint32_t m, uint32_t n);

	/**
	* \brief writes data to this matrix buffer
	*
	* \param vect data vector
	* \param meta meta information vector
	*
	* \return remaining size of row to be filled
	*/
	uint32_t WriteData(const Row<T> &vect, const Row<TM> &meta);

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
	uint32_t WriteData(T *buffer, TM *meta, uint32_t bufSize);

	/**
	* \brief writes a single data value to this matrix buffer
	*
	* \param data data value
	* \param meta meta information
	*
	* \return remaining size of row to be filled
	*/
	uint32_t WriteData(const T &data, const TM &meta);

	/**
	* \brief checks if storage is full
	*
	* \return true if it  is full
	*/
	bool IsFull() const;

	/**
	* \brief check how many rows in total were written
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
	* \brief reads a column from the buffer
	*
	* \param colIdx column index
	*
	* \return column vector
	*/
	Col<T> ReadCol(uint32_t colIdx) const;

	/**
	* \brief reads a column meta data
	*
	* \param colIdx column index
	*
	* \return column meta data
	*/
	Col<TM> ReadColMeta(uint32_t colIdx) const;

	/**
	* \brief reads all written data
	*
	* \return the buffer (sub)matrix
	*/
	Mat<T> ReadAll() const;

	/**
	* \brief reads all meta data
	*
	* \return the stored meta data
	*/
	Mat<TM> ReadAllMeta() const;

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
	* \brief gets the total dimensions of internal matrix buffer
	*
	* \return a Col-vector size 2 containing [nRows, nCols]
	*/
	Col<uint32_t> GetDimensions() const;


	/**
	* \brief gets the dimensions of written submatrix
	*
	* \return a Col-vector size 2 containing [nSubRows, nCols]
	*/
	Col<uint32_t> GetSubDimensions() const;

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
	Mat<TM> m_metaData;  /**< meta data for each entry of m_dataMat*/
	uint32_t m_colWrIdx, /**< write index for current column*/
		m_rowWrIdx;		 /**< index of current row to which is written*/
	bool m_isFull;		 /**< true when buffer is full*/
};

template <typename T, typename TM>
NodeDataBufferMeta<T, TM>::NodeDataBufferMeta() : m_nCol(0),
										  m_nRow(0),
										  m_dataMat(),
										  m_metaData(),
										  m_colWrIdx(0),
										  m_rowWrIdx(0)
{
}

template <typename T, typename TM>
NodeDataBufferMeta<T, TM>::NodeDataBufferMeta(uint32_t m, uint32_t n) : m_nCol(n),
																m_nRow(m),
																m_dataMat(m, n),
																m_metaData(m, n),
																m_colWrIdx(0),
																m_rowWrIdx(0)
{
	m_dataMat.zeros();
	m_metaData.zeros();
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::WriteData(const Row<T> &vect, const Row<TM> &meta)
{
	NS_ASSERT_MSG(!m_isFull, "Buffer is already full.");
	uint32_t vectSize = vect.n_elem;
	NS_ASSERT_MSG(vectSize == meta.n_elem, " Data and meta are not matching size!");

	uint32_t maxIdx = vectSize - 1 + m_colWrIdx,
			 minIdx = m_colWrIdx;
	NS_ASSERT_MSG(vectSize <= m_nCol, "Vector is greater than the maximum row size/nof columns!");
	NS_ASSERT_MSG(maxIdx < m_nCol, "Vector is greater than remaining space of current row!");

	m_dataMat(m_rowWrIdx, span(minIdx, maxIdx)) = vect;
	m_metaData(m_rowWrIdx, span(minIdx, maxIdx)) = meta;

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

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::WriteData(const T &data, const TM &meta)
{
	Row<T> dataVect(1);
	dataVect << data;
	Row<TM> metaVect(1);
	metaVect << meta;
	return WriteData(dataVect, metaVect);
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::WriteData(T *buffer, TM *meta, uint32_t bufSize)
{
	NS_ASSERT((buffer) || (meta)); //null pointer check
	//calculate remaining space
	uint32_t space = (m_nRow - m_rowWrIdx) * (m_nCol) + m_nCol - m_colWrIdx;
	NS_ASSERT_MSG(bufSize <= space, "Not enough space in buffer!");

	uint32_t nElem;
	if (m_rowWrIdx) //fill current row(if necessary)
	{
		nElem = m_nRow-m_rowWrIdx;

		Row<T> dataVect(buffer, nElem, false, true); // speed up: use memory of buffer, since we copy only anyway
		Row<TM> metaVect(meta, nElem, false, true);
		WriteData(dataVect, metaVect);
		bufSize -= nElem;
		buffer += nElem;
		meta += nElem;
	}
	if (bufSize)// TODO -> USE MATRIX instead of for
	{
		uint32_t nRow = bufSize / space; // NOF full(!) rows to write
		nElem = m_nRow;
		for (uint32_t row = 0; row < nRow; row++)
		{
			Row<T> dataVect(buffer, nElem, false, true); // speed up: use memory of buffer, since we copy only anyway
			Row<TM> metaVect(meta, nElem, false, true);
			WriteData(dataVect, metaVect);
			bufSize -= nElem;
			buffer += nElem;
			meta += nElem;
		}
		 // start writing new incomplete
	}
	if (bufSize)
	{
		nElem = bufSize;	
		Row<T> dataVect(buffer, nElem, false, true); // speed up: use memory of buffer, since we copy only anyway
		Row<TM> metaVect(meta, nElem, false, true);
		WriteData(dataVect, metaVect);
		bufSize -= nElem;
		buffer += nElem;
		meta += nElem;
	}
	return space;
}

template <typename T, typename TM>
bool NodeDataBufferMeta<T, TM>::IsFull() const
{
	return m_isFull;
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::GetWrRow() const
{
	if(IsEmpty())
		return 0;
	return m_rowWrIdx - 1;
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::GetWrCol() const
{
	if(IsEmpty())
		return 0;
	return m_colWrIdx - 1;
}

template <typename T, typename TM>
Col<T> NodeDataBufferMeta<T, TM>::ReadCol(uint32_t colIdx) const
{
	NS_ASSERT_MSG(colIdx < m_nCol, "Index exceeding NOF columns");
	return m_dataMat.col(colIdx);
}

template <typename T, typename TM>
Col<TM> NodeDataBufferMeta<T, TM>::ReadColMeta(uint32_t colIdx) const
{
	NS_ASSERT_MSG(colIdx < m_nCol, "Index exceeding NOF columns");
	return m_metaData.col(colIdx);
}

template <typename T, typename TM>
Mat<T> NodeDataBufferMeta<T, TM>::ReadAll() const
{
	if (IsEmpty()) //if no colums have been written yet, return empty matrix
	{
		return Mat<T>();
	}
	return m_dataMat.rows(0, m_rowWrIdx - 1);
}

template <typename T, typename TM>
Mat<TM> NodeDataBufferMeta<T, TM>::ReadAllMeta() const
{
	if (IsEmpty()) //if no colums have been written yet, return empty matrix
	{
		return Mat<TM>();
	}
	return m_metaData.rows(0, m_rowWrIdx - 1);
}

template <typename T, typename TM>
bool NodeDataBufferMeta<T, TM>::IsEmpty() const
{
	return !(m_rowWrIdx);
}

template <typename T, typename TM>
void NodeDataBufferMeta<T, TM>::Reset()
{
	m_dataMat.zeros();
	m_metaData.zeros();
	m_colWrIdx = 0;
	m_rowWrIdx = 0;
}

template <typename T, typename TM>
void NodeDataBufferMeta<T, TM>::Resize(uint32_t m, uint32_t n)
{
	m_nCol = n;
	m_nRow = m;
	m_dataMat.set_size(m, n);
	m_metaData.set_size(m, n);
	Reset();
}

template <typename T, typename TM>
Col<uint32_t> NodeDataBufferMeta<T, TM>::GetSubDimensions() const
{
	Col<uint32_t> size(2);
	size(0) = m_rowWrIdx;
	size(1) = m_nCol;
	return size;
}

template <typename T, typename TM>
Col<uint32_t> NodeDataBufferMeta<T, TM>::GetDimensions() const
{
	Col<uint32_t> size(2);
	size(0) = m_nRow;
	size(1) = m_nCol;
	return size;
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::nRows() const
{
	return m_nRow;
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::nCols() const
{
	return m_nCol;
}
#endif //NODE_DATABUFFERMETA_H