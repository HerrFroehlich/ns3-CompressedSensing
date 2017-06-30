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

// /*--------  FourierTransMatrix  --------*/
// FourierTransMatrix::FourierTransMatrix() : TransMatrix(0), TFourier1DOperator<cx_double>(0)
// {
// }

// FourierTransMatrix::FourierTransMatrix(uint32_t n) : TransMatrix(n), TFourier1DOperator<cx_double>(n)
// {
// 	SetSize(n);
// }

// void FourierTransMatrix::SetSize(uint32_t n)
// {
// 	if (n != GetSize())
// 	{
// 		TransMatrix::SetSize(n);
// 	}
// }
