/**
* \file spatial-precoding-matrix.cc
*
* \author Tobias Waurick
* \date 24.07.17
*
*/
#include "spatial-precoding-matrix.h"
#include "assert.h"

template <typename T>
TypeId SpatialPrecodingMatrix<T>::GetTypeId()
{
	std::string name = GetTypeParamName<T>();
	static TypeId tid = TypeId(("SpatialPrecodingMatrix<" + name + ">").c_str())
							.SetParent<Object>()
							.SetGroupName("CompressedSensing");

	return tid;
}

template <typename T>
SpatialPrecodingMatrix<T>::SpatialPrecodingMatrix() : m_n(0)
{
}

template <typename T>
SpatialPrecodingMatrix<T>::SpatialPrecodingMatrix(uint32_t n) : TOperator<T>(n, n),
																m_n(n), m_diag(n, true)
{
}

template <typename T>
void SpatialPrecodingMatrix<T>::SetSize(uint32_t n)
{
	TOperator<T>::resize(n, n);
	m_n = n;
	m_diag.resize(n, true);
}

template <typename T>
uint32_t SpatialPrecodingMatrix<T>::GetSize()
{
	return m_n;
}

template <typename T>
void SpatialPrecodingMatrix<T>::SetEntry(uint32_t idx, bool val)
{
	m_diag.at(idx) = val;
}

template <typename T>
void SpatialPrecodingMatrix<T>::SetDiag(std::vector<bool> vec)
{
	NS_ASSERT_MSG(vec.size() == m_diag.size(), "Size not matching!");

	m_diag = vec;
}

template <typename T>
void SpatialPrecodingMatrix<T>::apply(const arma::Col<T> &in, arma::Col<T> &out)
{
	ThrowTraceExceptionIf(KIncompatibleSizeOperatorException, in.n_rows != this->n());

	out.set_size(m_n);
	for (uint32_t i = 0; i < m_n; i++)
	{
		if (m_diag.at(i))
			out.at(i) = in.at(i);
		else
			out.at(i) = 0;
	}
}

template <typename T>
void SpatialPrecodingMatrix<T>::applyAdjoint(const arma::Col<T> &in, arma::Col<T> &out)
{
	apply(in, out);
}

template <typename T>
void SpatialPrecodingMatrix<T>::column(klab::UInt32 i, arma::Col<T> &out)
{
	ThrowTraceExceptionIf(KOutOfBoundOperatorException, i >= this->n());

	out.set_size(m_n);
	out.zeros();
	out.at(i) = m_diag.at(i);
}

template <typename T>
void SpatialPrecodingMatrix<T>::columnAdjoint(klab::UInt32 i, arma::Col<T> &out)
{
	column(i, out);
}

template <typename T>
void SpatialPrecodingMatrix<T>::toMatrix(arma::Mat<T> &out)
{

	out.set_size(m_n, m_n);
	out.fill(static_cast<T>(0));
	for (uint32_t i = 0; i < m_n; i++)
	{
		out(i, i) = m_diag.at(i);
	}
}

template <typename T>
void SpatialPrecodingMatrix<T>::toMatrixAdjoint(arma::Mat<T> &out)
{
	toMatrix(out);
}

OBJECT_TEMPLATE_CLASS_DEFINE(SpatialPrecodingMatrix, double);
OBJECT_TEMPLATE_CLASS_DEFINE(SpatialPrecodingMatrix, cx_double);
