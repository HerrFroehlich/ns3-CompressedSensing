/**
* \file mat-buffer.h
*
* \author Tobias Waurick
* \date 19.06.17
*
*/

#ifndef MAT_BUFFER_H
#define MAT_BUFFER_H

#include "ns3/core-module.h"
#include <armadillo>
#include "assert.h"
using namespace arma;
/**
* \class MatBuffer
*
* \brief a simple class template creating an API between NS3 and ARMADILLO's matrix class		
*
* Data is read/written either to/from simple buffers or by using arma::Mat matrices. In both cases dimensions are checked
* and the correct size of the buffer/ the matrix is asserted.
*
* \tparam (standard) data type beeing stored
*/
template <typename T>
class MatBuffer : public ns3::Object
{
  public:
	/**
	* \brief creates an empty matrix buffer
	*
	*/
	MatBuffer();

	/**
	* \brief creates a matrix buffer with fixed dimensions
	*
	* \param m 	NOF rows
	* \param n	NOF colums	
	*
	*/
	MatBuffer(uint32_t m, uint32_t n);

	/**
	* \brief resets the buffer
	*
	*/
	void Reset();

	/**
	* \brief resets & sets the dimensions of this matrix buffer
	*
	* \param m 	NOF rows
	* \param n	NOF colums	
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

	/**
	* \brief gets the NOF elements in buffer matrix
	*
	* \return NOF elements
	*/
	uint32_t nElem() const;

	/**
	* \brief writes data from a matrix
	*
	* \param mat input matrix
	*
	*/
	void Write(const Mat<T> &mat);

	/**
	* \brief writes data from a buffer by copying. Data in buffer has to be in a column by column order.
	*
	* \param buffer pointer to input buffer
	* \param bufSize size of buffer
	*
	*/
	void Write(const T *buffer, uint32_t bufSize);

	/**
	* \brief reads the buffer matrix
	*
	* \return a const reference to the buffered data as a matrix
	*/
	const Mat<T>& Read();

	/**
	* \brief reads the buffer matrix to an external buffer(ordered column by column)
	*
	*/
	void Read(T *buffer, uint32_t bufSize);

  private:
	Mat<T> m_dataMat; /**< saved data in matrix form*/
};

template <typename T>
MatBuffer<T>::MatBuffer() : m_dataMat()
{
}

template <typename T>
MatBuffer<T>::MatBuffer(uint32_t m, uint32_t n) : m_dataMat(m, n)
{
	m_dataMat.zeros();
}

template <typename T>
void MatBuffer<T>::Reset()
{
	m_dataMat.zeros();
}

template <typename T>
void MatBuffer<T>::Resize(uint32_t m, uint32_t n)
{
	m_dataMat.set_size(m, n);
	Reset();
}

template <typename T>
Col<uint32_t> MatBuffer<T>::GetDimensions() const
{
	Col<uint32_t> size(2);
	size(0) = m_dataMat.n_rows;
	size(1) = m_dataMat.n_cols;
	return size;
}

template <typename T>
uint32_t MatBuffer<T>::nRows() const
{
	return m_dataMat.n_rows;
}

template <typename T>
uint32_t MatBuffer<T>::nCols() const
{
	return m_dataMat.n_cols;
}

template <typename T>
uint32_t MatBuffer<T>::nElem() const
{
	return m_dataMat.n_elem;
}

template <typename T>
void MatBuffer<T>::Write(const Mat<T> &mat)
{
	NS_ASSERT_MSG(mat.n_rows == nRows(), "NOF rows not matching!");
	NS_ASSERT_MSG(mat.n_cols == nCols(), "NOF columns not matching!");

	m_dataMat = mat;
}

template <typename T>
void MatBuffer<T>::Write(const T *buffer, uint32_t bufSize)
{
	NS_ASSERT(buffer); // null pointer check
	NS_ASSERT_MSG(bufSize == nElem(), "NOF elements not matching!");
	m_dataMat = Mat<T>(buffer, nRows(), nCols());
}

// template <typename T>
// void MatBuffer<T>::WrMove(T *&buffer, uint32_t bufSize)
// {
// 	NS_ASSERT_MSG(bufSize == nElem(), "NOF elements not matching!");
// 	m_dataMat = Mat<T>(buffer, nRows(), nCols(), false, true);
// 	buffer = 0;
// }

template <typename T>
const Mat<T>& MatBuffer<T>::Read()
{
	return m_dataMat;
}

template <typename T>
void MatBuffer<T>::Read(T *buffer, uint32_t bufSize)
{
	NS_ASSERT(buffer); // null pointer check
	NS_ASSERT_MSG(bufSize == nElem(), "NOF elements not matching!");
	copy(buffer, buffer+bufSize, m_dataMat.m_memPtr());
}
#endif //MAT_BUFFER_H