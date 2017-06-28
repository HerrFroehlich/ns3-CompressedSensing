/**
* \file omp-reconstructor.cc
*
* \author Tobias Waurick
* \date 20.06.17
*
*/
#include "omp-reconstructor.h"
#include "assert.h"

NS_LOG_COMPONENT_DEFINE("OMP_TempReconstructor");
NS_OBJECT_ENSURE_REGISTERED(OMP_TempReconstructor);

TypeId OMP_TempReconstructor::GetTypeId(void)
{
	static TypeId tid = TypeId("OMP_TempReconstructor")
							.SetParent<Reconstructor<double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<OMP_TempReconstructor>()
							.AddAttribute("nMeas", "Default NOF measurement vectors to reconstruct",
										  UintegerValue(0),
										  MakeUintegerAccessor(&OMP_TempReconstructor::m_nMeasDef),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("mMax", "Default maximum NOF measurement vectors used for reconstruction",
										  UintegerValue(0),
										  MakeUintegerAccessor(&OMP_TempReconstructor::m_mMaxDef),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("k", "Default sparsity of solution",
										  UintegerValue(0),
										  MakeUintegerAccessor(&OMP_TempReconstructor::m_kDef),
										  MakeUintegerChecker<uint32_t>())
							.AddAttribute("Tolerance", "Tolerance of solution",
										  DoubleValue(1e-3),
										  MakeDoubleAccessor(&OMP_TempReconstructor::m_tolerance),
										  MakeDoubleChecker<double>());

	return tid;
}

OMP_TempReconstructor::OMP_TempReconstructor()
{
	m_vecLenDef = 1;
}

void OMP_TempReconstructor::Setup(uint32_t nMeas, uint32_t mMax, uint32_t k, double tolerance)
{
	NS_LOG_FUNCTION(this << nMeas << mMax << k << tolerance);
	SetBufferDim(nMeas, mMax, 1);
	m_kDef = k;
	m_tolerance = tolerance;
}

void OMP_TempReconstructor::AddSrcNode(T_NodeIdTag nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax)
{
	Reconstructor::AddSrcNode(nodeId, seed, nMeas, mMax, 1);
}

uint32_t OMP_TempReconstructor::Write(T_NodeIdTag nodeId, double data)
{
	NS_LOG_FUNCTION(this << nodeId << data);
	return WriteData(nodeId, &data, 1);
}

int64_t OMP_TempReconstructor::Reconstruct(T_NodeIdTag nodeId, uint32_t kspars, uint32_t iter)
{
	NS_LOG_FUNCTION(this << nodeId << kspars << iter);
	typedef klab::TSmartPointer<kl1p::TOperator<double>> T_OpPtr;

	SystemWallClockMs wallClock;
	int64_t time = 0;
	uint32_t k = kspars;
	kl1p::TOMPSolver<double, double> omp(m_tolerance);
	Col<double> y, x;
	T_OpPtr A = new kl1p::TMatrixOperator<double>(GetMatOp(nodeId));

	y = GetBufMat(nodeId);
	// y.save("ry" + std::to_string(nodeId), arma_ascii);
	if (k == 0)
		k = GetNofMeas(nodeId) / 2;
	if (iter)
		omp.setIterationLimit(iter);

	wallClock.Start();
	omp.solve(y, A, k, x);
	time = wallClock.End();
	WriteRecBuf(nodeId, x);

	m_completeCb(time, omp.iterations());
	return time;
}

int64_t OMP_TempReconstructor::Reconstruct(T_NodeIdTag nodeId)
{
	return Reconstruct(nodeId, m_kDef, 0);
}

int64_t OMP_TempReconstructor::ReconstructAll()
{
	//TODO
	return 0;
}