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
TransMatrix::TransMatrix() : m_mat()
{
}

TransMatrix::TransMatrix(uint32_t n) : m_mat(n, n)
{
}

void TransMatrix::SetSize(uint32_t n)
{
	m_mat.set_size(n, n);
}

uint32_t TransMatrix::GetSize() const
{
	return m_mat.n_rows;
}

Mat<cx_double> operator*(const TransMatrix &lvl, const Mat<cx_double> &rvl)
{
	return lvl.m_mat * rvl;
}

Mat<cx_double> operator*(const Mat<cx_double> &lvl, const TransMatrix &rvl)
{
	return lvl * rvl.m_mat;
}

std::ostream &operator<<(std::ostream &os, const TransMatrix &obj)
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
