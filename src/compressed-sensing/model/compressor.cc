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

NS_LOG_COMPONENT_DEFINE("Compressor");

/*--------  Compressor  --------*/

template <>
TypeId Compressor<double>::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::Compressor<double>")
							.SetParent<Object>()
							.AddConstructor<Compressor<double>>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  TypeId::ATTR_SET,
										  PointerValue(CreateObject<IdentRandomMatrix>()),
										  MakePointerAccessor(&Compressor::SetRanMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of a real transformation in which the solution is sparse",
										  TypeId::ATTR_SET,
										  PointerValue(),
										  MakePointerAccessor(&Compressor::SetTransMat),
										  MakePointerChecker<TransMatrix<double>>());
	return tid;
}

template <>
TypeId Compressor<cx_double>::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::Compressor<cx_double>")
							.SetParent<Object>()
							.AddConstructor<Compressor<cx_double>>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  TypeId::ATTR_SET,
										  PointerValue(CreateObject<IdentRandomMatrix>()),
										  MakePointerAccessor(&Compressor::SetRanMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of a real transformation in which the solution is sparse",
										  TypeId::ATTR_SET,
										  PointerValue(),
										  MakePointerAccessor(&Compressor::SetTransMat),
										  MakePointerChecker<TransMatrix<cx_double>>());
	return tid;
}

template <typename T>
Compressor<T>::Compressor() : m_seed(1), m_m(0),
							  m_n(0), m_vecLen(0),
							  m_bufLenIn(0), m_bufLenOut(0)
{
}

template <typename T>
Compressor<T>::Compressor(uint32_t n, uint32_t m, uint32_t vecLen) : m_seed(1), m_m(m),
																	 m_n(n), m_vecLen(vecLen),
																	 m_bufLenIn(n * vecLen), m_bufLenOut(m * vecLen)
{
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

	if (m_transMat.isValid())
		m_transMat->SetSize(n);

	if (m_ranMat.isValid())
		m_ranMat->SetSize(m, n, seed);

	if (norm)
		m_ranMat->NormalizeToM();
}

template <typename T>
void Compressor<T>::Compress(const T *bufferIn, uint32_t bufLenIn, T *bufferOut, uint32_t bufLenOut) const
{
	NS_LOG_FUNCTION(this << bufferIn << bufLenIn << bufferOut << bufLenOut);
	NS_ASSERT_MSG(bufLenIn == m_bufLenIn, "Incorrect input buffer size");
	NS_ASSERT_MSG(bufLenOut == m_bufLenOut, "Incorrect output buffer size");

	klab::TSmartPointer<kl1p::TOperator<T>> op_ptr = m_ranMat * m_transMat;
	const arma::Mat<T> x(const_cast<T *>(bufferIn), m_n, m_vecLen, false);
	arma::Mat<T> y(bufferOut, m_m, m_vecLen, false);

	for (uint32_t i = 0; i < m_vecLen; i++)
	{
		Col<T> yVec(m_m);
		op_ptr->apply(x.col(i), yVec);
		y.col(i) = yVec;
	}
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

template class Compressor<double>;
template class Compressor<cx_double>;

/*--------  CompressorTemp  --------*/

template <typename T>
CompressorTemp<T>::CompressorTemp() : Compressor<T>()
{
}

template <typename T>
CompressorTemp<T>::CompressorTemp(uint32_t n, uint32_t m) : Compressor<T>(m, n, VECLEN)
{
}

template <typename T>
void CompressorTemp<T>::Setup(uint32_t seed, uint32_t n, uint32_t m, bool norm)
{
	Compressor<T>::Setup(seed, m, n, VECLEN, norm);
}

template class CompressorTemp<double>;
template class CompressorTemp<cx_double>;