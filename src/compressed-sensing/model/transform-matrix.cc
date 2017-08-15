/**
* \file transform-matrix.cc
*
* \author Tobias Waurick
* \date 23.06.17
*
*/

#include "transform-matrix.h"
#include "ns3/template-registration.h"

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

OBJECT_TEMPLATE_CLASS_DEFINE(TransMatrix, double);
OBJECT_TEMPLATE_CLASS_DEFINE(TransMatrix, cx_double);

/*--------  FourierTransMatrix  --------*/
template <typename T>
FourierTransMatrix<T>::FourierTransMatrix() : TransMatrix<T>(0), TInverseFourier1DOperator<T>(0)//, m_inverse(0)
{
}

template <typename T>
FourierTransMatrix<T>::FourierTransMatrix(uint32_t n) : TransMatrix<T>(n), TInverseFourier1DOperator<T>(n)//, m_inverse(n)
{
	SetSize(n);
}

template <typename T>
void FourierTransMatrix<T>::SetSize(uint32_t n)
{
	if (n != TransMatrix<T>::GetSize())
	{
		TransMatrix<T>::SetSize(n);
		TInverseFourier1DOperator<T>::resize(n);
		//m_inverse.resize(n);
	}
}

template <typename T>
FourierTransMatrix<T>*FourierTransMatrix<T>::Clone() const
{
	return new FourierTransMatrix<T>(*this);
}

// template <typename T>
// FourierTransMatrix::ApplyInverse(const arma::Col<T> &in, arma::Col<T> &out)
// {
// 	return m_inverse.apply(in, out);
// }

template <typename T>
void FourierTransMatrix<T>::apply(const arma::Col<T> &in, arma::Col<T> &out)
{
	TInverseFourier1DOperator<T>::apply(in, out);
}

template <typename T>
void FourierTransMatrix<T>::applyAdjoint(const arma::Col<T> &in, arma::Col<T> &out)
{
	TInverseFourier1DOperator<T>::applyAdjoint(in, out);
}

OBJECT_TEMPLATE_CLASS_DEFINE(FourierTransMatrix, double);
OBJECT_TEMPLATE_CLASS_DEFINE(FourierTransMatrix, cx_double);
/*--------  DcTransMatrix  --------*/
template <typename T>
DcTransMatrix<T>::DcTransMatrix() : TransMatrix<T>(0), TInverseDCT1DOperator<T>(0)//, m_inverse(0)
{
}

template <typename T>
DcTransMatrix<T>::DcTransMatrix(uint32_t n) : TransMatrix<T>(n), TInverseDCT1DOperator<T>(n)//, m_inverse(n)
{
	TransMatrix<T>::SetSize(n);
}

template <typename T>
void DcTransMatrix<T>::SetSize(uint32_t n)
{
	if (n != TransMatrix<T>::GetSize())
	{
		TransMatrix<T>::SetSize(n);
		TInverseDCT1DOperator<T>::resize(n);
	//	m_inverse.resize(n);
	}
}

template <typename T>
DcTransMatrix<T> *DcTransMatrix<T>::Clone() const
{
	return new DcTransMatrix<T>(*this);
}

// template <typename T>
// DcTransMatrix::ApplyInverse(const arma::Col<T> &in, arma::Col<T> &out)
// {
// 	return m_inverse.apply(in, out);
// }

template <typename T>
void DcTransMatrix<T>::apply(const arma::Col<T> &in, arma::Col<T> &out)
{
	TInverseDCT1DOperator<T>::apply(in, out);
}

template <typename T>
void DcTransMatrix<T>::applyAdjoint(const arma::Col<T> &in, arma::Col<T> &out)
{
	TInverseDCT1DOperator<T>::applyAdjoint(in, out);
}

OBJECT_TEMPLATE_CLASS_DEFINE(DcTransMatrix, double);
OBJECT_TEMPLATE_CLASS_DEFINE(DcTransMatrix, cx_double);
