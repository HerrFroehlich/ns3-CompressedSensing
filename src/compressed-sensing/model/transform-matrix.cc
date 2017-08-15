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

TransMatrix::TransMatrix() : TOperator<double>()
{
}


TransMatrix::TransMatrix(uint32_t n) : TOperator<double>(n)
{
}


void TransMatrix::SetSize(uint32_t n)
{
	TOperator<double>::resize(n);
}


uint32_t TransMatrix::GetSize() const
{
	return TOperator<double>::n();
}


NS_OBJECT_ENSURE_REGISTERED(TransMatrix);

/*--------  FourierTransMatrix  --------*/

FourierTransMatrix::FourierTransMatrix() : TransMatrix(0), TInverseFourier1DOperator<double>(0)//, m_inverse(0)
{
}


FourierTransMatrix::FourierTransMatrix(uint32_t n) : TransMatrix(n), TInverseFourier1DOperator<double>(n)//, m_inverse(n)
{
	SetSize(n);
}


void FourierTransMatrix::SetSize(uint32_t n)
{
	if (n != TransMatrix::GetSize())
	{
		TransMatrix::SetSize(n);
		TInverseFourier1DOperator<double>::resize(n);
		//m_inverse.resize(n);
	}
}


FourierTransMatrix*FourierTransMatrix::Clone() const
{
	return new FourierTransMatrix(*this);
}

// 
// FourierTransMatrix::ApplyInverse(const arma::Col<double> &in, arma::Col<double> &out)
// {
// 	return m_inverse.apply(in, out);
// }


void FourierTransMatrix::apply(const arma::Col<double> &in, arma::Col<double> &out)
{
	TInverseFourier1DOperator<double>::apply(in, out);
}


void FourierTransMatrix::applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out)
{
	TInverseFourier1DOperator<double>::applyAdjoint(in, out);
}

NS_OBJECT_ENSURE_REGISTERED(FourierTransMatrix);

/*--------  DcTransMatrix  --------*/

DcTransMatrix::DcTransMatrix() : TransMatrix(0), TInverseDCT1DOperator<double>(0)//, m_inverse(0)
{
}


DcTransMatrix::DcTransMatrix(uint32_t n) : TransMatrix(n), TInverseDCT1DOperator<double>(n)//, m_inverse(n)
{
	TransMatrix::SetSize(n);
}


void DcTransMatrix::SetSize(uint32_t n)
{
	if (n != TransMatrix::GetSize())
	{
		TransMatrix::SetSize(n);
		TInverseDCT1DOperator<double>::resize(n);
	//	m_inverse.resize(n);
	}
}


DcTransMatrix *DcTransMatrix::Clone() const
{
	return new DcTransMatrix(*this);
}

// 
// DcTransMatrix::ApplyInverse(const arma::Col<double> &in, arma::Col<double> &out)
// {
// 	return m_inverse.apply(in, out);
// }


void DcTransMatrix::apply(const arma::Col<double> &in, arma::Col<double> &out)
{
	TInverseDCT1DOperator<double>::apply(in, out);
}


void DcTransMatrix::applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out)
{
	TInverseDCT1DOperator<double>::applyAdjoint(in, out);
}

NS_OBJECT_ENSURE_REGISTERED(DcTransMatrix);
