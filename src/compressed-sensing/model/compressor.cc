/**
* \file compressor.cc
*
* \author Tobias Waurick
* \date 11.07.17
*
*/
#include "compressor.h"
#include <armadillo>
#include "assert.h"
#include "ns3/template-registration.h"

NS_LOG_COMPONENT_DEFINE("Compressor");

/*--------  Compressor  --------*/

template <>
TypeId Compressor<double>::GetTypeId(void)
{
	static TypeId tid = TypeId("Compressor<double>")
							.SetParent<Object>()
							.AddConstructor<Compressor<double>>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(CreateObject<GaussianRandomMatrix>()),
										  MakePointerAccessor(&Compressor::SetRanMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of a real transformation in which the solution is sparse",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(),
										  MakePointerAccessor(&Compressor::SetTransMat),
										  MakePointerChecker<TransMatrix<double>>())
							.AddTraceSource("Complete",
											"Trace source indicating that compression completed",
											MakeTraceSourceAccessor(&Compressor::m_completeCb),
											"Compressor::CompleteCallback");
	return tid;
}

template <>
TypeId Compressor<cx_double>::GetTypeId(void)
{
	static TypeId tid = TypeId("Compressor<cx_double>")
							.SetParent<Object>()
							.AddConstructor<Compressor<cx_double>>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(CreateObject<GaussianRandomMatrix>()),
										  MakePointerAccessor(&Compressor::SetRanMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of a real transformation in which the solution is sparse",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(),
										  MakePointerAccessor(&Compressor::SetTransMat),
										  MakePointerChecker<TransMatrix<cx_double>>())
							.AddTraceSource("Complete",
											"Trace source indicating that compression completed",
											MakeTraceSourceAccessor(&Compressor::m_completeCb),
											"Compressor::CompleteCallback");
	return tid;
}

template <typename T>
Compressor<T>::Compressor() : m_seed(1), m_m(0),
							  m_n(0), m_vecLen(0),
							  m_bufLenIn(0), m_bufLenOut(0),
							  m_normalize(false)
{
}

template <typename T>
Compressor<T>::Compressor(uint32_t n, uint32_t m, uint32_t vecLen) : m_seed(1), m_m(m),
																	 m_n(n), m_vecLen(vecLen),
																	 m_bufLenIn(n * vecLen), m_bufLenOut(m * vecLen),
																	 m_normalize(false)
																	 
{
	if (m_transMat.isValid())
		m_transMat->SetSize(n);

	if (m_ranMat.isValid())
		m_ranMat->SetSize(m, n, m_seed);
}

template <typename T>
void Compressor<T>::Setup(uint32_t seed, uint32_t n, uint32_t m, uint32_t vecLen, bool norm)
{
	NS_LOG_FUNCTION(this << seed << m << n << vecLen << norm);
	m_seed = seed;
	m_m = m;
	m_n = n;
	m_vecLen = vecLen;
	m_bufLenIn = vecLen * n;
	m_bufLenOut = vecLen * m;

}

template <typename T>
void Compressor<T>::Compress(const T *bufferIn, uint32_t bufLenIn, T *bufferOut, uint32_t bufLenOut) const
{
	NS_LOG_FUNCTION(this << bufferIn << bufLenIn << bufferOut << bufLenOut);
	NS_ASSERT_MSG(bufLenIn == m_bufLenIn, "Incorrect input buffer size!");
	NS_ASSERT(bufferIn); //null pointer check

	klab::TSmartPointer<kl1p::TOperator<T>> op_ptr = m_ranMat * m_transMat;
	const arma::Mat<T> x(const_cast<T *>(bufferIn), m_n, m_vecLen, false);
	Compress(x, bufferOut, bufLenOut);
}

template <typename T>
void Compressor<T>::Compress(const arma::Mat<T> &matIn, T *bufferOut, uint32_t bufLenOut) const
{
	NS_LOG_FUNCTION(this << matIn << bufferOut << bufLenOut);
	NS_ASSERT_MSG(bufLenOut == m_bufLenOut, "Incorrect output buffer size!");
	NS_ASSERT_MSG((matIn.n_rows == m_n) && (matIn.n_cols == m_vecLen), "Incorrect matrix size!");
	NS_ASSERT(bufferOut); //null pointer check

	if (m_transMat.isValid())
		m_transMat->SetSize(m_n);

	if (m_ranMat.isValid())
		m_ranMat->SetSize(m_m, m_n, m_seed);

	if (m_normalize)
		m_ranMat->NormalizeToM();

	klab::TSmartPointer<kl1p::TOperator<T>> op_ptr = m_ranMat * m_transMat;
	arma::Mat<T> y(bufferOut, m_m, m_vecLen, false, true);

	for (uint32_t i = 0; i < m_vecLen; i++)
	{
		Col<T> yVec(m_m);
		op_ptr->apply(matIn.col(i), yVec);
		y.col(i) = yVec;
	}
	m_completeCb(matIn, y);
}

template <typename T>
template <typename TI>
void Compressor<T>::CompressSparse(const arma::Mat<T> &data, const arma::Col<TI> &idx, T *bufferOut, uint32_t bufLenOut) const
{
	NS_LOG_FUNCTION(this << data << idx << bufferOut << bufLenOut);
	NS_ASSERT(data.n_rows == idx.n_rows);
	NS_ASSERT_MSG((data.n_rows <= m_n) && (data.n_cols == m_vecLen), "Size mismatch!");

	arma::Mat<T> sp = arma::zeros<arma::Mat<T>>(m_n, m_vecLen);
	//arma::SpMat<T> sp(m_m, m_n); // contains zeros by default
	for (uint32_t i = 0; i < idx.n_elem; i++)
	{
		uint32_t rowIdx = idx.at(i);
		sp.row(rowIdx) = data.row(i);
	}

	Compress(sp, bufferOut, bufLenOut);
}

template <typename T>
void Compressor<T>::SetSeed(uint32_t seed, bool norm)
{
	m_seed = seed;
	if (m_ranMat.isValid())
	{
		m_ranMat->Generate(seed);

		if (norm)
			m_ranMat->NormalizeToM();
	}
}

template <typename T>
void Compressor<T>::SetRanMat(Ptr<RandomMatrix> ranMat_ptr)
{
	m_ranMat = ranMat_ptr->Clone();
	
	if (m_ranMat.isValid())
		m_ranMat->SetSize(m_m, m_n, m_seed);
}

template <typename T>
void Compressor<T>::SetTransMat(Ptr<TransMatrix<T>> transMat_ptr)
{
	m_transMat = transMat_ptr->Clone();

	if (m_transMat.isValid())
		m_transMat->SetSize(m_n);
}

// template class Compressor<double>;
// template class Compressor<cx_double>;
OBJECT_TEMPLATE_CLASS_DEFINE(Compressor, double);
OBJECT_TEMPLATE_CLASS_DEFINE(Compressor, cx_double);

template void Compressor<double>::CompressSparse<uint8_t>(const arma::Mat<double> &data, const arma::Col<uint8_t> &idx, double *bufferOut, uint32_t bufLenOut) const;
template void Compressor<double>::CompressSparse<uint16_t>(const arma::Mat<double> &data, const arma::Col<uint16_t> &idx, double *bufferOut, uint32_t bufLenOut) const;
template void Compressor<double>::CompressSparse<uint32_t>(const arma::Mat<double> &data, const arma::Col<uint32_t> &idx, double *bufferOut, uint32_t bufLenOut) const;
template void Compressor<cx_double>::CompressSparse<uint8_t>(const arma::Mat<cx_double> &data, const arma::Col<uint8_t> &idx, cx_double *bufferOut, uint32_t bufLenOut) const;
template void Compressor<cx_double>::CompressSparse<uint16_t>(const arma::Mat<cx_double> &data, const arma::Col<uint16_t> &idx, cx_double *bufferOut, uint32_t bufLenOut) const;
template void Compressor<cx_double>::CompressSparse<uint32_t>(const arma::Mat<cx_double> &data, const arma::Col<uint32_t> &idx, cx_double *bufferOut, uint32_t bufLenOut) const;

/*--------  CompressorTemp  --------*/
template <>
TypeId CompressorTemp<double>::GetTypeId(void)
{
	static TypeId tid = TypeId("CompressorTemp<double>")
							.SetParent<Compressor<double>>()
							.AddConstructor<CompressorTemp<double>>()
							.SetGroupName("CompressedSensing");
	return tid;
}

template <>
TypeId CompressorTemp<cx_double>::GetTypeId(void)
{
	static TypeId tid = TypeId("CompressorTemp<cx_double>")
							.SetParent<Compressor<cx_double>>()
							.AddConstructor<Compressor<cx_double>>()
							.SetGroupName("CompressedSensing");
	return tid;
}
template <typename T>
CompressorTemp<T>::CompressorTemp() : Compressor<T>()
{
}

template <typename T>
CompressorTemp<T>::CompressorTemp(uint32_t n, uint32_t m) : Compressor<T>(n, m, VECLEN)
{
}

template <typename T>
void CompressorTemp<T>::Setup(uint32_t seed, uint32_t n, uint32_t m, bool norm)
{
	Compressor<T>::Setup(seed, n, m, VECLEN, norm);
}

// template class CompressorTemp<double>;
// template class CompressorTemp<cx_double>;
OBJECT_TEMPLATE_CLASS_DEFINE(CompressorTemp, double);
OBJECT_TEMPLATE_CLASS_DEFINE(CompressorTemp, cx_double);