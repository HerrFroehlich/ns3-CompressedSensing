/**
* \file cs-algorithm.h
*
* \author Tobias Waurick
* \date 03.08.17
*
*/
#ifndef CS_ALGORITHM_H
#define CS_ALGORITHM_H

#include <KL1pInclude.h>
#include <armadillo>
#include "ns3/core-module.h"
using namespace arma;
using namespace ns3;

/**
* \ingroup rec
* \class CsAlgorithm
*
* \brief base class for compressed sensing algorithms
*
* Serves as an abstract base class for various specialised compressed sensing algorithms like OMP, Basis pursuit, AMP, etc.
* Features a method to run the algorithms and traced callbacks when the algorithm succeeds (gives time + NOF iterations)
* , as well when it fails.
*
*/
class CsAlgorithm : public ns3::Object
{
  public:
	typedef void (*CompleteTracedCallback)(int64_t time, uint32_t iter); /**< callback signature when completed a reconstruction*/
	typedef void (*ErrorTracedCallback)(const klab::KException &e);		 /**< callback signature when reconstruction fails*/

	static ns3::TypeId GetTypeId()
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
	};

	CsAlgorithm(){};

	/**
	* \brief runs the underlying algorithm to solve Y = A*X
	*
	* Base function for sub classes.
	* The dimensions of X (n:NOF measurements, length of a measurement vector)
	* is deduced from the size of A and Y.
	*
	* \param Y  Matrix/Vector Y containing compressed measurments
	* \param A  sensing Matrix, which was used to compress X to Y
	*
	* \return the reconstructed X
	*/
	virtual Mat<double> Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<double>> A) = 0;
	virtual Mat<cx_double> Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<cx_double>> A) = 0;

  protected:
	/**
	* \brief gets the  configured tolerance
	*
	* \return tolerance
	*/
	double GetTolerance()
	{
		return m_tol;
	};

	/**
	* \brief gets the  configured maximum NOF iterations
	*
	* If this returns zero, the maximum NOF iterations should not be set in the solver.
	*
	* \return  maximum NOF iterations
	*/
	uint32_t GetMaxIter()
	{
		return m_maxIter;
	};

	/**
	* \brief calls the complete callback
	*
	* \param time time needed for solving
	* \param iter NOF iterations needed
	*
	*/
	void CallCompleteCb(int64_t time, uint32_t iter)
	{
		m_completeCb(time, iter);
	};

	/**
	* \brief calls the error callback
	*
	*
	* \param error occured klab::KException 
	*
	*/
	void CallErrorCb(const klab::KException &err)
	{
		m_errorCb(err);
	}

  private:
	double m_tol;		/**< tolerance of solution*/
	uint32_t m_maxIter; /**< maximum NOF iterations, if 0 -> no iteration limit*/
	//traces
	TracedCallback<int64_t, uint32_t> m_completeCb;		/**< callback when completed reconstruction, returning time+iterations needed*/
	TracedCallback<const klab::KException &> m_errorCb; /**< callback when reconstruction fails, returning KExeception type*/
};

/**
* \ingroup rec
* \class CsAlgorithm_OMP
*
* \brief OMP solver algorithm
*
*/
class CsAlgorithm_OMP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_OMP();

	/*inherited from CsAlgorithm*/
	virtual Mat<double> Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual Mat<cx_double> Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<cx_double>> A);

  private:
	uint32_t m_k; /**< sparsity of reconstructed signal, if 0 or > n -> will be set to \f$k = \frac{m}{\log(n)} < n\f$*/
};

/**
* \ingroup rec
* \class CsAlgorithm_BP
*
* \brief basis pursuit solver algorithm
*
*/
class CsAlgorithm_BP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_BP();

	/*inherited from CsAlgorithm*/
	virtual Mat<double> Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual Mat<cx_double> Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<cx_double>> A);
};
#endif //CS_ALGORITHM_H