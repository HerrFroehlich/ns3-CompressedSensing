#include "cs-algorithm.h"
#include "assert.h"
#include "ns3/log.h"
#include <cmath> //log10

NS_LOG_COMPONENT_DEFINE("CsAlgorithm");

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm);
ns3::TypeId CsAlgorithm::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm")
							.SetParent<Object>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("Tolerance", "Tolerance of solution",
										  DoubleValue(1e-6),
										  MakeDoubleAccessor(&CsAlgorithm::m_tol),
										  MakeDoubleChecker<double>())
							.AddAttribute("MaxIter", "Maximum NOF Iterations",
										  UintegerValue(std::numeric_limits<uint32_t>::max()),
										  MakeUintegerAccessor(&CsAlgorithm::m_maxIter),
										  MakeUintegerChecker<uint32_t>())
							.AddTraceSource("RecComplete", "Callback when Reconstuction completed",
											MakeTraceSourceAccessor(&CsAlgorithm::m_completeCb),
											"Reconstructor::CompleteTracedCallback")
							.AddTraceSource("RecError", "Callback when Reconstuction failed with an error",
											MakeTraceSourceAccessor(&CsAlgorithm::m_errorCb),
											"Reconstructor::ErrorTracedCallback");
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Mat<double> CsAlgorithm::Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	NS_LOG_FUNCTION(this << &Y << &A);
	NS_ASSERT_MSG(!Y.is_empty(), "Y is empty, not able to reconstruct!");

	/*--------  Init  --------*/

	//SystemWallClockMs wallClock;
	klab::KTimer wallClock;
	int64_t time = 0;

	uint32_t vecLen = Y.n_cols,
			 n = A->n(),
			 iter = 0;

	Mat<double> X(n, vecLen);

	/*--------  Solve  --------*/

	SetMaxIter(m_maxIter);
	SetTolerance(m_tol);
	try
	{
		wallClock.start();
		for (uint32_t i = 0; i < vecLen; i++)
		{
			Col<double> xVec;
			iter += Solve(Y.col(i), xVec, A);
			X.col(i) = xVec;
		}
		wallClock.stop();
		time = wallClock.durationInMilliseconds();
		CallCompleteCb(time, iter);
	}
	catch (const klab::KException &e)
	{
		CallErrorCb(e);
	};
	return X;
}
/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_OMP);
ns3::TypeId CsAlgorithm_OMP::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_OMP")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("k", "sparsity of reconstructed signal, if 0 or > n -> will be set to k = m/log(n) < n$",
										  UintegerValue(0),
										  MakeUintegerAccessor(&CsAlgorithm_OMP::m_k),
										  MakeUintegerChecker<uint32_t>());
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_OMP::CsAlgorithm_OMP()
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_OMP::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_OMP::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_OMP::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	/*--------  Init  --------*/

	uint32_t k = m_k,
			 m = A->m(),
			 n = A->n();

	/*--------  Solve  --------*/

	if (k == 0)
		k = m / log10(n);
	NS_ASSERT_MSG(k <= n, "k must be <=n !");

	m_solver.solve(yin, A, k, xout);
	return m_solver.iterations();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_BP);
ns3::TypeId CsAlgorithm_BP::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_BP")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing");
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_BP::CsAlgorithm_BP()
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_BP::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_BP::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_BP::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	/*--------  Solve  --------*/
	m_solver.solve(yin, A, xout);
	return m_solver.iterations();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_AMP);
ns3::TypeId CsAlgorithm_AMP::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_AMP")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing");
	return tid;
}
/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_AMP::CsAlgorithm_AMP()
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_AMP::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_AMP::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_AMP::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{

	klab::TSmartPointer<kl1p::TOperator<double>> Anorm = new kl1p::TScalingOperator<double>(A, 1.0 / klab::Sqrt(A->m())); // Pseudo-normalization of the matrix
	//solve
	m_solver.solve(yin, Anorm, xout);
	xout = xout / klab::Sqrt(A->m()); //since we normalized before
	return m_solver.iterations();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_CoSaMP);
ns3::TypeId CsAlgorithm_CoSaMP::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_CoSaMP")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("k", "sparsity of reconstructed signal, if 0 or > n -> will be set to k = m/log(n) < n$",
										  UintegerValue(0),
										  MakeUintegerAccessor(&CsAlgorithm_CoSaMP::m_k),
										  MakeUintegerChecker<uint32_t>());
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_CoSaMP::CsAlgorithm_CoSaMP()
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_CoSaMP::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_CoSaMP::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_CoSaMP::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	/*--------  Init  --------*/

	uint32_t k = m_k,
			 m = A->m(),
			 n = A->n();

	/*--------  Solve  --------*/

	if (k == 0)
		k = m / log10(n);
	NS_ASSERT_MSG(k <= n, "k must be <=n !");

	m_solver.solve(yin, A, k, xout);
	return m_solver.iterations();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_ROMP);
ns3::TypeId CsAlgorithm_ROMP::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_ROMP")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("k", "sparsity of reconstructed signal, if 0 or > n -> will be set to k = m/log^2(n) < n$",
										  UintegerValue(0),
										  MakeUintegerAccessor(&CsAlgorithm_ROMP::m_k),
										  MakeUintegerChecker<uint32_t>());
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_ROMP::CsAlgorithm_ROMP()
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_ROMP::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_ROMP::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_ROMP::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	/*--------  Init  --------*/

	uint32_t k = m_k,
			 m = A->m(),
			 n = A->n();

	/*--------  Solve  --------*/

	if (k == 0)
		k = m / (log10(n) * log10(n));
	NS_ASSERT_MSG(k <= n, "k must be <=n !");

	m_solver.solve(yin, A, k, xout);
	return m_solver.iterations();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_SP);
ns3::TypeId CsAlgorithm_SP::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_SP")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("k", "sparsity of reconstructed signal, if 0 or > n -> will be set to k = m/log^2(n) < n$",
										  UintegerValue(0),
										  MakeUintegerAccessor(&CsAlgorithm_SP::m_k),
										  MakeUintegerChecker<uint32_t>());
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_SP::CsAlgorithm_SP()
{
}

void CsAlgorithm_SP::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_SP::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_SP::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	/*--------  Init  --------*/

	uint32_t k = m_k,
			 m = A->m(),
			 n = A->n();

	/*--------  Solve  --------*/

	if (k == 0)
		k = m / (log10(n));
	NS_ASSERT_MSG(k <= n, "k must be <=n !");

	m_solver.solve(yin, A, k, xout);
	return m_solver.iterations();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_SL0);
ns3::TypeId CsAlgorithm_SL0::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_SL0")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing");
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_SL0::CsAlgorithm_SL0()
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/
void CsAlgorithm_SL0::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_SL0::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_SL0::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	/*--------  Init  --------*/
	klab::TSmartPointer<kl1p::TOperator<double>> Anorm = new kl1p::TScalingOperator<double>(A, 1.0 / klab::Sqrt(A->m())); // Pseudo-normalization of the matrix
	//solve
	m_solver.solve(yin, Anorm, xout);
	xout = xout / klab::Sqrt(A->m()); //since we normalized before
	return m_solver.iterations();
}

/*-----------------------------------------------------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(CsAlgorithm_EMBP);
ns3::TypeId CsAlgorithm_EMBP::GetTypeId()
{
	static TypeId tid = TypeId("CsAlgorithm_EMBP")
							.SetParent<CsAlgorithm>()
							.SetGroupName("CompressedSensing")
							.AddAttribute("k", "sparsity of reconstructed signal, if 0 or > n -> will be set to k = m/log(n) < n$",
										  UintegerValue(0),
										  MakeUintegerAccessor(&CsAlgorithm_EMBP::m_k),
										  MakeUintegerChecker<uint32_t>());
	return tid;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

CsAlgorithm_EMBP::CsAlgorithm_EMBP()
{
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_EMBP::SetMaxIter(uint32_t maxIter)
{
	m_solver.setIterationLimit(maxIter);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

void CsAlgorithm_EMBP::SetTolerance(double tol)
{
	m_solver.setTolerance(tol);
}

/*-----------------------------------------------------------------------------------------------------------------------*/

uint32_t CsAlgorithm_EMBP::Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	/*--------  Init  --------*/

	uint32_t k = m_k,
			 m = A->m(),
			 n = A->n();

	/*--------  Solve  --------*/

	if (k == 0)
		k = m / log10(n);
	NS_ASSERT_MSG(k <= n, "k must be <=n !");

	m_solver.solve(yin, A, k, xout);
	return m_solver.iterations();
}
