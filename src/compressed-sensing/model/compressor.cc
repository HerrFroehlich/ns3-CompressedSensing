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

NS_OBJECT_ENSURE_REGISTERED(Compressor);

TypeId Compressor::GetTypeId(void)
{
	static TypeId tid = TypeId("Compressor")
							.SetParent<Object>()
							.AddConstructor<Compressor>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("RanMatrix", "The underlying random matrix form to create the sensing matrix",
										  PointerValue(CreateObject<GaussianRandomMatrix>()),
										  MakePointerAccessor(&Compressor::SetRanMat, &Compressor::GetRanMat),
										  MakePointerChecker<RandomMatrix>())
							.AddAttribute("TransMatrix", "The underlying matrix of a real transformation in which the solution is sparse",
										  TypeId::ATTR_SET | TypeId::ATTR_CONSTRUCT,
										  PointerValue(),
										  MakePointerAccessor(&Compressor::SetTransMat),
										  MakePointerChecker<TransMatrix>())
							.AddTraceSource("Complete",
											"Trace source indicating that compression completed",
											MakeTraceSourceAccessor(&Compressor::m_completeCb),
											"Compressor::CompleteCallback");
	return tid;
}



Compressor::Compressor() : m_seed(1), m_m(0),
							  m_n(0), m_vecLen(0),
							  m_bufLenIn(0), m_bufLenOut(0)
{
}


Compressor::Compressor(uint32_t n, uint32_t m, uint32_t vecLen) : m_seed(1), m_m(m),
																	 m_n(n), m_vecLen(vecLen),
																	 m_bufLenIn(n * vecLen), m_bufLenOut(m * vecLen)
																	 
{
	if (m_transMat.isValid())
		m_transMat->SetSize(n);

	if (m_ranMat.isValid())
		m_ranMat->SetSize(m, n, m_seed);
}


void Compressor::Setup(uint32_t seed, uint32_t n, uint32_t m, uint32_t vecLen)
{
	NS_LOG_FUNCTION(this << seed << m << n << vecLen);
	m_seed = seed;
	m_m = m;
	m_n = n;
	m_vecLen = vecLen;
	m_bufLenIn = vecLen * n;
	m_bufLenOut = vecLen * m;

}


void Compressor::Compress(const double *bufferIn, uint32_t bufLenIn, double *bufferOut, uint32_t bufLenOut) const
{
	NS_LOG_FUNCTION(this << bufferIn << bufLenIn << bufferOut << bufLenOut);
	NS_ASSERT_MSG(bufLenIn == m_bufLenIn, "Incorrect input buffer size!");
	NS_ASSERT(bufferIn); //null pointer check

	klab::TSmartPointer<kl1p::TOperator<double>> op_ptr = m_ranMat * m_transMat;
	const arma::Mat<double> x(const_cast<double *>(bufferIn), m_n, m_vecLen, false);
	Compress(x, bufferOut, bufLenOut);
}


void Compressor::Compress(const arma::Mat<double> &matIn, double *bufferOut, uint32_t bufLenOut) const
{
	NS_LOG_FUNCTION(this << matIn << bufferOut << bufLenOut);
	NS_ASSERT_MSG(bufLenOut == m_bufLenOut, "Incorrect output buffer size!");
	NS_ASSERT_MSG((matIn.n_rows == m_n) && (matIn.n_cols == m_vecLen), "Incorrect matrix size!");
	NS_ASSERT(bufferOut); //null pointer check

	if (m_transMat.isValid())
		m_transMat->SetSize(m_n);

	if (m_ranMat.isValid())
		m_ranMat->SetSize(m_m, m_n, m_seed);

	klab::TSmartPointer<kl1p::TOperator<double>> op_ptr = m_ranMat * m_transMat;
	arma::Mat<double> y(bufferOut, m_m, m_vecLen, false, true);

	for (uint32_t i = 0; i < m_vecLen; i++)
	{
		Col<double> yVec(m_m);
		op_ptr->apply(matIn.col(i), yVec);
		y.col(i) = yVec;
	}
	m_completeCb(matIn, y);
}


template <typename TI>
void Compressor::CompressSparse(const arma::Mat<double> &data, const arma::Col<TI> &idx, double *bufferOut, uint32_t bufLenOut) const
{
	NS_LOG_FUNCTION(this << data << idx << bufferOut << bufLenOut);
	NS_ASSERT(data.n_rows == idx.n_rows);
	NS_ASSERT_MSG((data.n_rows <= m_n) && (data.n_cols == m_vecLen), "Size mismatch!");

	arma::Mat<double> sp = arma::zeros<arma::Mat<double>>(m_n, m_vecLen);
	//arma::SpMat<double> sp(m_m, m_n); // contains zeros by default
	for (uint32_t i = 0; i < idx.n_elem; i++)
	{
		uint32_t rowIdx = idx.at(i);
		sp.row(rowIdx) = data.row(i);
	}

	Compress(sp, bufferOut, bufLenOut);
}


void Compressor::SetSeed(uint32_t seed)
{
	m_seed = seed;
	if (m_ranMat.isValid())
	{
		m_ranMat->Generate(seed);
	}
}


void Compressor::SetRanMat(Ptr<RandomMatrix> ranMat_ptr)
{
	m_ranMat = ranMat_ptr->Clone();
	
	if (m_ranMat.isValid())
		m_ranMat->SetSize(m_m, m_n, m_seed);
}


Ptr<RandomMatrix>  Compressor::GetRanMat() const
{
	return Ptr<RandomMatrix>(m_ranMat.get());
}


void Compressor::SetTransMat(Ptr<TransMatrix> transMat_ptr)
{
	m_transMat = transMat_ptr->Clone();

	if (m_transMat.isValid())
		m_transMat->SetSize(m_n);
}


template void Compressor::CompressSparse<uint8_t>(const arma::Mat<double> &data, const arma::Col<uint8_t> &idx, double *bufferOut, uint32_t bufLenOut) const;
template void Compressor::CompressSparse<uint16_t>(const arma::Mat<double> &data, const arma::Col<uint16_t> &idx, double *bufferOut, uint32_t bufLenOut) const;
template void Compressor::CompressSparse<uint32_t>(const arma::Mat<double> &data, const arma::Col<uint32_t> &idx, double *bufferOut, uint32_t bufLenOut) const;

/*--------  CompressorTemp  --------*/

NS_OBJECT_ENSURE_REGISTERED(CompressorTemp);

TypeId CompressorTemp::GetTypeId(void)
{
	static TypeId tid = TypeId("CompressorTemp")
							.SetParent<Compressor>()
							.AddConstructor<CompressorTemp>()
							.SetGroupName("CompressedSensing");
	return tid;
}


CompressorTemp::CompressorTemp() : Compressor()
{
}


CompressorTemp::CompressorTemp(uint32_t n, uint32_t m) : Compressor(n, m, VECLEN)
{
}


void CompressorTemp::Setup(uint32_t seed, uint32_t n, uint32_t m)
{
	Compressor::Setup(seed, n, m, VECLEN);
}

