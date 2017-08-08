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
										  DoubleValue(1e-3),
										  MakeDoubleAccessor(&CsAlgorithm::m_tol),
										  MakeDoubleChecker<double>())
							.AddAttribute("MaxIter", "Maximum NOF Iterations, if 0 -> no iteration limit",
										  UintegerValue(0),
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

Mat<double> CsAlgorithm_OMP::Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	NS_LOG_FUNCTION(this << &Y << &A);
	NS_ASSERT_MSG(!Y.is_empty(), "Y is empty, not able to reconstruct!");

	/*--------  Init  --------*/

	//SystemWallClockMs wallClock;
	klab::KTimer wallClock;
	int64_t time = 0;

	uint32_t k = m_k,
			 m = Y.n_rows,
			 vecLen = Y.n_cols,
			 n = A->n(),
			 iter = GetMaxIter();

	kl1p::TOMPSolver<double> omp(GetTolerance());

	Mat<double> X(n, vecLen);

	/*--------  Solve  --------*/

	if (k == 0)
		k = m / log10(n);
	NS_ASSERT_MSG(k <= n, "k must be <=n !");

	if (iter)
		omp.setIterationLimit(iter);

	try
	{
		wallClock.start();
		for (uint32_t i = 0; i < vecLen; i++)
		{
			Col<double> xVec;
			omp.solve(Y.col(i), A, k, xVec);
			X.col(i) = xVec;
		}
		wallClock.stop();
		time = wallClock.durationInMilliseconds();
		CallCompleteCb(time, omp.iterations());
	}
	catch (const klab::KException &e)
	{
		CallErrorCb(e);
	};
	return X;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Mat<cx_double> CsAlgorithm_OMP::Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<cx_double>> A)
{
	NS_LOG_FUNCTION(this << &Y << &A);
	NS_ASSERT_MSG(!Y.is_empty(), "Y is empty, not able to reconstruct!");

	/*--------  Init  --------*/

	//SystemWallClockMs wallClock;
	klab::KTimer wallClock;
	int64_t time = 0;

	uint32_t k = m_k,
			 m = Y.n_rows,
			 vecLen = Y.n_cols,
			 n = A->n(),
			 iter = GetMaxIter();

	kl1p::TOMPSolver<double, cx_double> omp(GetTolerance());

	Mat<cx_double> X(n, vecLen);
	Mat<cx_double> Yc = conv_to<Mat<cx_double>>::from(Y); //complex version of Y

	/*--------  Solve  --------*/
	if (k == 0)
		k = m / log10(n);
	if (iter)
		omp.setIterationLimit(iter);

	try
	{
		wallClock.start();
		for (uint32_t i = 0; i < vecLen; i++)
		{
			Col<cx_double> xVec;
			omp.solve(Yc.col(i), A, k, xVec);
			X.col(i) = xVec;
		}
		wallClock.stop();
		time = wallClock.durationInMilliseconds();
		CallCompleteCb(time, omp.iterations());
	}
	catch (const klab::KException &e)
	{
		CallErrorCb(e);
	};
	return X;
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

Mat<double> CsAlgorithm_BP::Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<double>> A)
{
	NS_LOG_FUNCTION(this << &Y << &A);
	NS_ASSERT_MSG(!Y.is_empty(), "Y is empty, not able to reconstruct!");

	/*--------  Init  --------*/

	//SystemWallClockMs wallClock;
	klab::KTimer wallClock;
	int64_t time = 0;

	uint32_t vecLen = Y.n_cols,
			 n = A->n(),
			 iter = GetMaxIter();

	kl1p::TBasisPursuitSolver<double> bp(GetTolerance());

	Mat<double> X(n, vecLen);

	/*--------  Solve  --------*/

	if (iter)
		bp.setIterationLimit(iter);

	try
	{
		wallClock.start();
		for (uint32_t i = 0; i < vecLen; i++)
		{
			Col<double> xVec;
			bp.solve(Y.col(i), A, xVec);
			X.col(i) = xVec;
		}
		wallClock.stop();
		time = wallClock.durationInMilliseconds();
		CallCompleteCb(time, bp.iterations());
	}
	catch (const klab::KException &e)
	{
		CallErrorCb(e);
	};
	return X;
}

/*-----------------------------------------------------------------------------------------------------------------------*/

Mat<cx_double> CsAlgorithm_BP::Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<cx_double>> A)
{
	NS_LOG_FUNCTION(this << &Y << &A);
	NS_ASSERT_MSG(!Y.is_empty(), "Y is empty, not able to reconstruct!");

	/*--------  Init  --------*/

	//SystemWallClockMs wallClock;
	klab::KTimer wallClock;
	int64_t time = 0;

	uint32_t vecLen = Y.n_cols,
			 n = A->n(),
			 iter = GetMaxIter();

	kl1p::TBasisPursuitSolver<double, cx_double> bp(GetTolerance());

	Mat<cx_double> X(n, vecLen);
	Mat<cx_double> Yc = conv_to<Mat<cx_double>>::from(Y); //complex version of Y

	/*--------  Solve  --------*/

	if (iter)
		bp.setIterationLimit(iter);

	try
	{
		wallClock.start();
		for (uint32_t i = 0; i < vecLen; i++)
		{
			Col<cx_double> xVec;
			bp.solve(Yc.col(i), A, xVec);
			X.col(i) = xVec;
		}
		wallClock.stop();
		time = wallClock.durationInMilliseconds();
		CallCompleteCb(time, bp.iterations());
	}
	catch (const klab::KException &e)
	{
		CallErrorCb(e);
	};
	return X;
}