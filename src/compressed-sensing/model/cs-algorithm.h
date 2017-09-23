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

	static ns3::TypeId GetTypeId();

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
	virtual Mat<double> Run(const Mat<double> &Y, const klab::TSmartPointer<kl1p::TOperator<double>> A);

  protected:
	/**
	* \brief Starts solving
	*
	* Should be implemented by sub classes
	*
	* \param yin y vector as input
	* \param xout x vector to write output to
	* \param A  sensing Matrix, which was used to compress X to Y
	*
	* \return NOF iterations needed
	*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A) = 0;

	/**
	* \brief sets the maximum NOF iterations
	*
	* Should be implemented by sub classes
	*
	* \param maxIter maximum NOF iterations
	*
	*/
	virtual void SetMaxIter(uint32_t maxIter) = 0;

	/**
	* \brief sets the  configured tolerance
	*
	* Should be implemented by sub classes
	*
	* \param tol toleranceto set
	*/
	virtual void SetTolerance(double tol) = 0;

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

  private:
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
* Before using this algorithm the value of k should  be set via the Attribute System!
* As default \f$k = \frac{m}{\log(n)} < n\f$.
*/
class CsAlgorithm_OMP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_OMP();

	/*inherited from CsAlgorithm*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);

  private:
	kl1p::TOMPSolver<double> m_solver; /**< solver algorithm used*/
	uint32_t m_k;					   /**< sparsity of reconstructed signal, if 0 or > n -> will be set to \f$k = \frac{m}{\log(n)} < n\f$*/
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
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);

  private:
	kl1p::TBasisPursuitSolver<double> m_solver; /**< solver algorithm used*/
};

/**
* \ingroup rec
* \class CsAlgorithm_AMP
*
* \brief AMP solver algorithm
*
* When running the matrix operator A is normalized by 1/sqrt(m), as AMP requires this.
* The result of the algorithm is also scaled correctly to match the actual A.
* Consequently it is not nescessary to do the normalization on A before calling Run.
*/
class CsAlgorithm_AMP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_AMP();

	/*inherited from CsAlgorithm*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);

  private:
	kl1p::TAMPSolver<double> m_solver; /**< solver algorithm used*/
};

/**
* \ingroup rec
* \class CsAlgorithm_CoSaMP
*
* \brief Compressing  solver algorithm
*
* Before using this algorithm the value of k should be set via the Attribute System!
* As default \f$k = \frac{m}{\log(n)} < n\f$.
*/
class CsAlgorithm_CoSaMP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_CoSaMP();

	/*inherited from CsAlgorithm*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);

private:
	kl1p::TCoSaMPSolver<double> m_solver; /**< solver algorithm used*/
	uint32_t m_k; /**< sparsity of reconstructed signal, if 0 or > n -> will be set to \f$k = \frac{m}{\log(n)} < n\f$*/
};

/**
* \ingroup rec
* \class CsAlgorithm_ROMP
*
* \brief ROMP solver algorithm
*
* Before using this algorithm the value of k should be set via the Attribute System!
* As default \f$k = \frac{m}{\log^2(n)} < n\f$.
*/
class CsAlgorithm_ROMP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_ROMP();

	/*inherited from CsAlgorithm*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);

private:
	kl1p::TROMPSolver<double> m_solver; /**< solver algorithm used*/
	uint32_t m_k; /**< sparsity of reconstructed signal, if 0 or > n -> will be set to \f$k = \frac{m}{\log^2(n)} < n\f$*/
};

/**
* \ingroup rec
* \class CsAlgorithm_SP
*
* \brief Subspace Pursuit solver algorithm
*
* Before using this algorithm the value of k should be set via the Attribute System!
* As default \f$k = \frac{m}{\log(n)} < n\f$.
*
*/
class CsAlgorithm_SP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_SP();

	/*inherited from CsAlgorithm*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);

  private:
	kl1p::TSubspacePursuitSolver<double> m_solver; /**< solver algorithm used*/
	uint32_t m_k; /**< sparsity of reconstructed signal, if 0 or > n -> will be set to \f$k = \frac{m}{\log(n)} < n\f$*/
};

/**
* \ingroup rec
* \class CsAlgorithm_SL0
*
* \brief Smoothed L0 solver algorithm
*
*/
class CsAlgorithm_SL0 : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_SL0();

	/*inherited from CsAlgorithm*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);
private:
  kl1p::TSL0Solver<double> m_solver; /**< solver algorithm used*/
};

/**
* \ingroup rec
* \class CsAlgorithm_EMBP
*
* \brief EMBP solver algorithm
*
* Before using this algorithm the value of k should be set via the Attribute System!
* As default \f$k = \frac{m}{\log(n)} < n\f$.
*/
class CsAlgorithm_EMBP : public CsAlgorithm
{
  public:
	static ns3::TypeId GetTypeId();
	CsAlgorithm_EMBP();

	/*inherited from CsAlgorithm*/
	virtual uint32_t Solve(const Col<double> &yin, Col<double> &xout, const klab::TSmartPointer<kl1p::TOperator<double>> A);
	virtual void SetMaxIter(uint32_t maxIter);
	virtual void SetTolerance(double tol);
private:
  kl1p::TEMBPSolver<double> m_solver; /**< solver algorithm used*/
  uint32_t m_k; /**< sparsity of reconstructed signal, if 0 or > n -> will be set to \f$k = \frac{m}{\log(n)} < n\f$*/
};
#endif //CS_ALGORITHM_H