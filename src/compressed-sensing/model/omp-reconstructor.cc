/**
* \file omp-reconstructor.cc
*
* \author Tobias Waurick
* \date 20.06.17
*
*/
#include "omp-reconstructor.h"
#include "assert.h"

NS_LOG_COMPONENT_DEFINE("OMP_Reconstructor");

template <>
TypeId OMP_Reconstructor<double>::GetTypeId(void)
{
	std::string name = TypeNameGet<OMP_Reconstructor<double>>();
	static TypeId tid = TypeId(("ns3::OMP_Reconstructor<" + name + ">").c_str())
							.SetParent<Reconstructor<double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<OMP_Reconstructor<double>>()
							.AddAttribute("k", "Default sparsity of solution",
										  UintegerValue(0),
										  MakeUintegerAccessor(&OMP_Reconstructor::m_kDef),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("Tolerance", "Tolerance of solution",
										  DoubleValue(1e-3),
										  MakeDoubleAccessor(&OMP_Reconstructor::m_tolerance),
										  MakeDoubleChecker<double>());
	return tid;
}

template <>
TypeId OMP_Reconstructor<cx_double>::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::OMP_Reconstructor<cx_double>")
							.SetParent<Reconstructor<cx_double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<OMP_Reconstructor<cx_double>>()
							.AddAttribute("k", "Default sparsity of solution",
										  UintegerValue(0),
										  MakeUintegerAccessor(&OMP_Reconstructor::m_kDef),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("Tolerance", "Tolerance of solution",
										  DoubleValue(1e-3),
										  MakeDoubleAccessor(&OMP_Reconstructor::m_tolerance),
										  MakeDoubleChecker<double>());
	return tid;
}


template <typename T>
int64_t OMP_Reconstructor<T>::Reconstruct(T_NodeIdTag nodeId)
{
	return Reconstruct(nodeId, m_kDef, 0);
}

template <typename T>
void OMP_Reconstructor<T>::Setup(uint32_t nMeas, uint32_t mMax, uint32_t vecLen, uint32_t k, double tolerance)
{
	NS_LOG_FUNCTION(this << nMeas << mMax << vecLen << k << tolerance);
	Reconstructor<T>::SetBufferDim(nMeas, mMax, vecLen);
	m_kDef = k;
	m_tolerance = tolerance;
}

template <typename T>
int64_t OMP_Reconstructor<T>::Reconstruct(T_NodeIdTag nodeId, uint32_t kspars, uint32_t iter)
{
	NS_LOG_FUNCTION(this << nodeId << kspars << iter);

	//SystemWallClockMs wallClock;
	klab::KTimer wallClock;
	int64_t time = 0;
	Mat<T> y;

	y = Reconstructor<T>::GetBufMat(nodeId);
	if (y.is_empty())
	{
		NS_LOG_WARN("No data stored, not able to reconstruct!");
	}
	else
	{
		uint32_t k = kspars,
				 vecLen = Reconstructor<T>::GetVecLen(nodeId),
				 n = Reconstructor<T>::GetN(nodeId);
		kl1p::TOMPSolver<double, T> omp(m_tolerance);
		klab::TSmartPointer<kl1p::TOperator<T>> A = Reconstructor<T>::GetOp(nodeId);
		Mat<T> x(n, vecLen);

		if (k == 0)
			k = Reconstructor<T>::GetNofMeas(nodeId) / 2;
		if (iter)
			omp.setIterationLimit(iter);

		try
		{
			wallClock.start();
			for (uint32_t i = 0; i < vecLen; i++)
			{
				Col<T> xVec;
				omp.solve(y.col(i), A, k, xVec);
				x.col(i) = xVec;
			}
			wallClock.stop();
			time = wallClock.durationInMilliseconds();
		}
		catch (const klab::KException &e)
		{
			Reconstructor<T>::m_errorCb(e);
		}
		Reconstructor<T>::WriteRecBuf(nodeId, x);
		Reconstructor<T>::m_completeCb(time, omp.iterations());
		Reconstructor<T>::ResetFullBuf(nodeId);
	}

	return time;
}

template class OMP_Reconstructor<double>;
template class OMP_Reconstructor<cx_double>;

/*--------  OMP_ReconstructorTemp  --------*/

template <>
TypeId OMP_ReconstructorTemp<double>::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::OMP_ReconstructorTemp<double>")
							.SetParent<OMP_Reconstructor<double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<OMP_ReconstructorTemp<double>>();
	return tid;
}

template <>
TypeId OMP_ReconstructorTemp<cx_double>::GetTypeId(void)
{
	std::string name = TypeNameGet<OMP_Reconstructor<cx_double>>();
	static TypeId tid = TypeId("ns3::OMP_ReconstructorTemp<cx_double>")
							.SetParent<OMP_Reconstructor<cx_double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<OMP_ReconstructorTemp<cx_double>>();
	return tid;
}


template <typename T>
OMP_ReconstructorTemp<T>::OMP_ReconstructorTemp()
{
	Reconstructor<T>::m_vecLenDef = 1;
}


template <typename T>
void OMP_ReconstructorTemp<T>::Setup(uint32_t nMeas, uint32_t mMax, uint32_t k, double tolerance)
{
	NS_LOG_FUNCTION(this << nMeas << mMax << k << tolerance);
	Reconstructor<T>::SetBufferDim(nMeas, mMax, 1);
	OMP_Reconstructor<T>::m_kDef = k;
	OMP_Reconstructor<T>::m_tolerance = tolerance;
}

template <typename T>
void OMP_ReconstructorTemp<T>::AddSrcNode(T_NodeIdTag nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax)
{
	Reconstructor<T>::AddSrcNode(nodeId, seed, nMeas, mMax, 1);
}

template <typename T>
uint32_t OMP_ReconstructorTemp<T>::Write(T_NodeIdTag nodeId, T data)
{
	NS_LOG_FUNCTION(this << nodeId << data);
	return Reconstructor<T>::WriteData(nodeId, &data, 1);
}

template class OMP_ReconstructorTemp<double>;
template class OMP_ReconstructorTemp<cx_double>;

