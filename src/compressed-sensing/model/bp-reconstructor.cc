/**
* \file bp-reconstructor.cc
*
* \author Tobias Waurick
* \date 26.07.17
*
*/
#include "bp-reconstructor.h"
#include "assert.h"

NS_LOG_COMPONENT_DEFINE("BP_Reconstructor");

template <>
TypeId BP_Reconstructor<double>::GetTypeId(void)
{
	static TypeId tid = TypeId("BP_Reconstructor<double>")
							.SetParent<Reconstructor<double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<BP_Reconstructor<double>>()
							.AddAttribute("Tolerance", "Tolerance of solution",
										  DoubleValue(1e-3),
										  MakeDoubleAccessor(&BP_Reconstructor::m_tolerance),
										  MakeDoubleChecker<double>());
	return tid;
}

template <>
TypeId BP_Reconstructor<cx_double>::GetTypeId(void)
{
	static TypeId tid = TypeId("BP_Reconstructor<cx_double>")
							.SetParent<Reconstructor<cx_double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<BP_Reconstructor<cx_double>>()
							.AddAttribute("Tolerance", "Tolerance of solution",
										  DoubleValue(1e-3),
										  MakeDoubleAccessor(&BP_Reconstructor::m_tolerance),
										  MakeDoubleChecker<double>());
	return tid;
}


template <typename T>
void BP_Reconstructor<T>::Setup(uint32_t nMeas, uint32_t mMax, uint32_t vecLen,  double tolerance)
{
	NS_LOG_FUNCTION(this << nMeas << mMax << vecLen <<  tolerance);
	Reconstructor<T>::SetBufferDim(nMeas, mMax, vecLen);
	m_tolerance = tolerance;
}

template <typename T>
int64_t BP_Reconstructor<T>::Reconstruct(T_NodeIdTag nodeId)
{
	NS_LOG_FUNCTION(this << nodeId);

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
		uint32_t vecLen = Reconstructor<T>::GetVecLen(nodeId),
				 n = Reconstructor<T>::GetN(nodeId);
		kl1p::TBasisPursuitSolver<double, T> bp(m_tolerance);
		klab::TSmartPointer<kl1p::TOperator<T>> A = Reconstructor<T>::GetOp(nodeId);
		Mat<T> x(n, vecLen);

		try
		{
			wallClock.start();
			for (uint32_t i = 0; i < vecLen; i++)
			{
				Col<T> xVec;
				bp.solve(y.col(i), A, xVec);
				x.col(i) = xVec;
				Col<T> yVec = y.col(i);
			}
			wallClock.stop();
			time = wallClock.durationInMilliseconds();
		}
		catch (const klab::KException &e)
		{
			Reconstructor<T>::m_errorCb(e);
		}

		y.save("./IOdata/y", arma::csv_ascii);
		x.save("./IOdata/x", arma::csv_ascii);

		Mat<T> mat;
		A->toMatrix(mat);
		mat.save("./IOdata/A", arma::csv_ascii);

		Reconstructor<T>::WriteRecBuf(nodeId, x);
		Reconstructor<T>::m_completeCb(time, bp.iterations());
		Reconstructor<T>::ResetFullBuf(nodeId);
	}

	return time;
}

template <typename T>
Ptr<Reconstructor<T>> BP_Reconstructor<T>::Clone()
{
	Ptr<BP_Reconstructor<T>> p(this);
	p = CopyObject(p);
	return Ptr<Reconstructor<T>>(dynamic_cast<Reconstructor<T> *>(PeekPointer(p)));
}

// template class BP_Reconstructor<double>;
// template class BP_Reconstructor<cx_double>;
OBJECT_TEMPLATE_CLASS_DEFINE(BP_Reconstructor, double);
OBJECT_TEMPLATE_CLASS_DEFINE(BP_Reconstructor, cx_double);

/*--------  BP_ReconstructorTemp  --------*/

template <>
TypeId BP_ReconstructorTemp<double>::GetTypeId(void)
{
	static TypeId tid = TypeId("BP_ReconstructorTemp<double>")
							.SetParent<BP_Reconstructor<double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<BP_ReconstructorTemp<double>>();
	return tid;
}

template <>
TypeId BP_ReconstructorTemp<cx_double>::GetTypeId(void)
{
	std::string name = TypeNameGet<BP_Reconstructor<cx_double>>();
	static TypeId tid = TypeId("BP_ReconstructorTemp<cx_double>")
							.SetParent<BP_Reconstructor<cx_double>>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<BP_ReconstructorTemp<cx_double>>();
	return tid;
}

template <typename T>
BP_ReconstructorTemp<T>::BP_ReconstructorTemp()
{
	Reconstructor<T>::m_vecLenDef = 1;
}

template <typename T>
void BP_ReconstructorTemp<T>::Setup(uint32_t nMeas, uint32_t mMax, double tolerance)
{
	NS_LOG_FUNCTION(this << nMeas << mMax <<tolerance);
	Reconstructor<T>::SetBufferDim(nMeas, mMax, 1);
	BP_Reconstructor<T>::m_tolerance = tolerance;
}

template <typename T>
void BP_ReconstructorTemp<T>::AddSrcNode(T_NodeIdTag nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax)
{
	Reconstructor<T>::AddSrcNode(nodeId, seed, nMeas, mMax, 1);
}

template <typename T>
uint32_t BP_ReconstructorTemp<T>::Write(T_NodeIdTag nodeId, T data)
{
	NS_LOG_FUNCTION(this << nodeId << data);
	return Reconstructor<T>::WriteData(nodeId, &data, 1);
}

template <typename T>
Ptr<Reconstructor<T>> BP_ReconstructorTemp<T>::Clone()
{
	Ptr<BP_Reconstructor<T>> p(this);
	p = CopyObject(p);
	return Ptr<Reconstructor<T>>(dynamic_cast<Reconstructor<T> *>(PeekPointer(p)));
}

// template class BP_ReconstructorTemp<double>;
// template class BP_ReconstructorTemp<cx_double>;
OBJECT_TEMPLATE_CLASS_DEFINE(BP_ReconstructorTemp, double);
OBJECT_TEMPLATE_CLASS_DEFINE(BP_ReconstructorTemp, cx_double);
