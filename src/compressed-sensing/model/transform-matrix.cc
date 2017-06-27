/**
* \file transform-matrix.cc
*
* \author Tobias Waurick
* \date 23.06.17
*
*/

#include "transform-matrix.h"
#include <KL1pInclude.h>

/*--------  TransMatrix  --------*/
template<typename T>
TransMatrix<T>::TransMatrix() : m_mat()
{
}

template<typename T>
TransMatrix<T>::TransMatrix(uint32_t n) : m_mat(n, n)
{
}

template<typename T>
void TransMatrix<T>::SetSize(uint32_t n)
{
	m_mat.set_size(n, n);
}

template<typename T>
uint32_t TransMatrix<T>::GetSize() const
{
	return m_mat.n_rows;
}

template<typename T>
Mat<T> operator*(const TransMatrix<T> &lvl, const Mat<T> &rvl)
{
	return lvl.m_mat * rvl;
}

template<typename T>
Mat<T> operator*(const Mat<T> &lvl, const TransMatrix<T> &rvl)
{
	return lvl * rvl.m_mat;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const TransMatrix<T> &obj)
{
	// write obj to stream
	os << obj.m_mat;
	return os;
}

/*--------  FourierTransMatrix  --------*/
FourierTransMatrix::FourierTransMatrix()
{
}

FourierTransMatrix::FourierTransMatrix(uint32_t n) : TransMatrix(n)
{
	SetSize(n);
}

void FourierTransMatrix::SetSize(uint32_t n)
{
	if (n != GetSize())
	{
		TransMatrix::SetSize(n);
		kl1p::TFourier1DOperator<cx_double> op(n);
		op.toMatrix(m_mat);
	}
}
