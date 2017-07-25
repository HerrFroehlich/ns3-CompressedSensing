/**
* \file transform-matrix.cc
*
* \author Tobias Waurick
* \date 23.06.17
*
*/

#include "transform-matrix.h"

/*--------  TransMatrix  --------*/
template <typename T>
TransMatrix<T>::TransMatrix() : TOperator<T>()
{
}

template <typename T>
TransMatrix<T>::TransMatrix(uint32_t n) : TOperator<T>(n)
{
}

template <typename T>
void TransMatrix<T>::SetSize(uint32_t n)
{
	TOperator<T>::resize(n);
}

template <typename T>
uint32_t TransMatrix<T>::GetSize() const
{
	return TOperator<T>::n();
}

template <typename T>
arma::Mat<T> TransMatrix<T>::GetMatrix()
{
	arma::Mat<T> out;
	TOperator<T>::toMatrix(out);
	return out;
}

template <typename T>
arma::Mat<T> operator*(const TransMatrix<T> &lvl, const arma::Mat<T> &rvl)
{
	return lvl.GetMatrix() * rvl;
}

template <typename T>
arma::Mat<T> operator*(const arma::Mat<T> &lvl, const TransMatrix<T> &rvl)
{
	return lvl * rvl.GetMatrix();
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const TransMatrix<T> &obj)
{
	// write obj to stream
	os << obj.GetMatrix();
	return os;
}

template class TransMatrix<double>;
template class TransMatrix<cx_double>;

/*--------  FourierTransMatrix  --------*/
FourierTransMatrix::FourierTransMatrix() : TransMatrix<cx_double>(0), TFourier1DOperator<cx_double>(0)
{
}

FourierTransMatrix::FourierTransMatrix(uint32_t n) : TransMatrix<cx_double>(n), TFourier1DOperator<cx_double>(n)
{
	SetSize(n);
}

void FourierTransMatrix::SetSize(uint32_t n)
{
	if (n != GetSize())
	{
		TransMatrix::SetSize(n);
	}
}

FourierTransMatrix *FourierTransMatrix::Clone() const
{
	return new FourierTransMatrix(*this);
}

void FourierTransMatrix::apply(const arma::Col<cx_double> &in, arma::Col<cx_double> &out)
{
	TFourier1DOperator<cx_double>::apply(in, out);
}

void FourierTransMatrix::applyAdjoint(const arma::Col<cx_double> &in, arma::Col<cx_double> &out)
{
	TFourier1DOperator<cx_double>::applyAdjoint(in, out);
}

/*--------  DcTransMatrix  --------*/
template <typename T>
DcTransMatrix<T>::DcTransMatrix() : TransMatrix<T>(0), TDCT1DOperator<T>(0)
{
}

template <typename T>
DcTransMatrix<T>::DcTransMatrix(uint32_t n) : TransMatrix<T>(n), TDCT1DOperator<T>(n)
{
	SetSize(n);
}

template <typename T>
void DcTransMatrix<T>::SetSize(uint32_t n)
{
	if (n != TransMatrix<T>::GetSize())
	{
		TransMatrix<T>::SetSize(n);
	}
}

template <typename T>
DcTransMatrix<T> *DcTransMatrix<T>::Clone() const
{
	return new DcTransMatrix<T>(*this);
}

template <typename T>
void DcTransMatrix<T>::apply(const arma::Col<T> &in, arma::Col<T> &out)
{
	TDCT1DOperator<T>::apply(in, out);
}

template <typename T>
void DcTransMatrix<T>::applyAdjoint(const arma::Col<T> &in, arma::Col<T> &out)
{
	TDCT1DOperator<T>::applyAdjoint(in, out);
}

template class DcTransMatrix<double>;
template class DcTransMatrix<cx_double>;
