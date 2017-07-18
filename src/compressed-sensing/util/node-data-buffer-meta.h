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
* \ingroup util
* \class NodeDataBufferMeta
*
* \brief a buffer template to save data(considered as a vector) from a node to a matrix
*
* A storage buffer template for data from a sensor node. 
* Data is assumed to be vectorized and so multiple data vectors are stored in matrix form (MxN).
* For each matrix row meta data is provided.
* The buffer matrix can be written sequentially rowwise by arma::Col vectors or by common buffers.
* Rows must be written entirely on each write operation.
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
	* \brief writes one row of this matrix buffer
	*
	* \param vect data vector
	* \param meta meta information
	*
	* \return remaining NOF rows to be filled
	*/
	uint32_t WriteData(const Row<T> &vect, TM meta);

	/**
	* \brief writes data from a buffer to this matrix buffer, to fill one row 
	*
	* \param buffer pointer to data buffer
	* \param bufSize size of buffer (must be N)
	* \param meta pointer to meta information buffer
	*
	* \return remaining NOF rows
	*/
	uint32_t WriteData(const T *buffer, uint32_t bufSize, TM meta);

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
	* \brief reads a column from the buffer
	*
	* \param colIdx column index
	*
	* \return column vector
	*/
	Col<T> ReadCol(uint32_t colIdx) const;

	/**
	* \brief reads meta of a certain row
	*
	* \param idx row index
	*
	* \return row meta data
	*/
	TM ReadMeta(uint32_t idx) const;

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
	Col<TM> ReadAllMeta() const;

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
	*
	*/
	uint32_t nCols() const;

	/**
	* \brief sort data matrix by given meta data (ascending)
	*/
	void SortByMeta();

  private:
	uint32_t m_nCol,	 /**< NOF columns*/
		m_nRow;			 /**< NOF rows*/
	Mat<T> m_dataMat;	/**< saved data in matrix form*/
	Col<TM> m_metaData;  /**< meta data for each row of m_dataMat*/
	uint32_t m_rowWrIdx; /**< index of current row to which is written*/
	bool m_isFull;		 /**< true when buffer is full*/
};

template <typename T, typename TM>
NodeDataBufferMeta<T, TM>::NodeDataBufferMeta() : m_nCol(0),
												  m_nRow(0),
												  m_dataMat(),
												  m_metaData(),
												  m_rowWrIdx(0),
												  m_isFull(false)
{
}

template <typename T, typename TM>
NodeDataBufferMeta<T, TM>::NodeDataBufferMeta(uint32_t m, uint32_t n) : m_nCol(n),
																		m_nRow(m),
																		m_dataMat(m, n),
																		m_metaData(m),
																		m_rowWrIdx(0),
																		m_isFull(false)
{
	m_dataMat.zeros();
	m_metaData.zeros();
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::WriteData(const Row<T> &vect, TM meta)
{
	NS_ASSERT_MSG(!m_isFull, "Buffer is already full.");
	uint32_t vectSize = vect.n_elem;
	NS_ASSERT_MSG(vectSize == m_nRow, " Data vector must be of size N!");

	m_dataMat.row(m_rowWrIdx) = vect;
	m_metaData(m_rowWrIdx) = meta;

	m_rowWrIdx++;
	if (m_rowWrIdx == m_nRow)
		m_isFull = true;
	return m_nRow - m_rowWrIdx;
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::WriteData(const T *buffer, uint32_t bufSize, TM meta)
{
	NS_ASSERT(buffer); //null pointer check
	NS_ASSERT_MSG(bufSize == m_nRow, " Buffer size  must equal N!");

	T *matMem_ptr = m_dataMat.memptr(); // here data is stored in a column by column order

	for (uint32_t i = 0; i < bufSize; i++)
	{
		uint32_t matMemStart = m_nRow * i + m_rowWrIdx;
		*(matMem_ptr + matMemStart) = *(buffer + i);
	}

	m_metaData(m_rowWrIdx) = meta;

	m_rowWrIdx++;
	if (m_rowWrIdx == m_nRow)
		m_isFull = true;
	return m_nRow - m_rowWrIdx;
}

template <typename T, typename TM>
bool NodeDataBufferMeta<T, TM>::IsFull() const
{
	return m_isFull;
}

template <typename T, typename TM>
uint32_t NodeDataBufferMeta<T, TM>::GetWrRow() const
{
	if (IsEmpty())
		return 0;
	return m_rowWrIdx - 1;
}

template <typename T, typename TM>
Col<T> NodeDataBufferMeta<T, TM>::ReadCol(uint32_t colIdx) const
{
	NS_ASSERT_MSG(colIdx < m_nCol, "Index exceeding NOF columns");
	return m_dataMat.col(colIdx);
}

template <typename T, typename TM>
TM NodeDataBufferMeta<T, TM>::ReadMeta(uint32_t idx) const
{
	NS_ASSERT_MSG(idx < m_nRow, "Index exceeding NOF rows");
	return m_metaData.at(idx);
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
Col<TM> NodeDataBufferMeta<T, TM>::ReadAllMeta() const
{
	if (IsEmpty()) //if no colums have been written yet, return empty matrix
	{
		return Col<TM>();
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
	m_rowWrIdx = 0;
	m_isFull = false;
}

template <typename T, typename TM>
void NodeDataBufferMeta<T, TM>::Resize(uint32_t m, uint32_t n)
{
	m_nCol = n;
	m_nRow = m;
	m_dataMat.set_size(m, n);
	m_metaData.set_size(m);
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

template <typename T, typename TM>
void NodeDataBufferMeta<T, TM>::SortByMeta()
{
	Col<TM> tmpMeta = ReadAllMeta();
	Mat<T> tmpData = ReadAll();

	uvec idx = sort_index(tmpMeta);
	for (uint32_t i = 0; i < m_rowWrIdx; i++)
	{
		m_dataMat.row(i) = tmpData.row(idx[i]);
		m_metaData[i] = tmpMeta.at(idx[i]);
	}
}

#endif //NODE_DATABUFFERMETA_H