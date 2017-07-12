/**
* \file random-matrix.h
*
* \author Tobias Waurick
* \date 21.06.17
*
*/

#ifndef RANDOM_MATRIX_H
#define RANDOM_MATRIX_H

#include "ns3/core-module.h"
#include <armadillo>
#include <KL1pInclude.h>
#include <KSciInclude.h>
#include <math.h> //sqrt
#include "transform-matrix.h"
using namespace ns3;
using namespace kl1p;
/**
* \ingroup compsens
 * \defgroup ranmat Random Sensing Matrices
 *
 * Various classes for generating random sub-sampling matrices
 */
/**
* \ingroup ranmat
* \class RandomMatrix
*
* \brief base clase to create matrices with random entries
*
*/
class RandomMatrix : public Object, public kl1p::TOperator<double>
{
public:
  static TypeId GetTypeId();

  /**
  * \brief create a empty matrix
  *
  */
  RandomMatrix();

  /**
  * \brief create a Random matrix of size MxN
  *
  * \param m NOF rows
  * \param n NOF columns
  *
  */
  RandomMatrix(uint32_t m, uint32_t n);

  /**
  * \brief sets the size of the matrix, may regenerate the entries
  *
  * \param m NOF rows
  * \param n NOF columns
  * \param regenerate  regenerate matrix?
  *
  */
  void SetSize(uint32_t m, uint32_t n, bool regenerate = true);

  /**
  * \brief sets the size of the matrix and regenerates with the given seed
  *
  * \param m NOF rows
  * \param n NOF columns
  * \param seed seed to use
  *
  */
  void SetSize(uint32_t m, uint32_t n, uint32_t seed);

  /**
  * \brief Generate random entries for a given seed (if it is different than the previous one, or if forced to)
  *
  * \param seed seed to use
  * \param force force generation
  *
  */
  virtual void Generate(uint32_t seed, bool force = false) = 0;

  /**
  * \brief gets the NOF rows
  *
  * \return NOF rows
  */
  uint32_t nRows() const;

  /**
  * \brief gets the NOF columns
  *
  * \return NOF columns
  */
  uint32_t nCols() const;

  /**
	* \brief gets the dimensions of internal matrix buffer
	*
	* \return a Col-vector size 2 containing [nRows, nCols]
	*/
  arma::Col<uint32_t> Dim() const;

  /**
  * \brief normalizes the matrix by 1/sqrt(m)
  */
  void NormalizeToM();

  /**
  * \brief clones the matrix
  *
  * \return pointer to a new matrix
  */
  virtual RandomMatrix *Clone() const = 0;

  /**
  * \brief cast to complex operator pointer
  *
  * \return complex operator pointe
  */ /**
  * \brief cast to complex operator pointer
  *
  * \return complex operator pointer
  */
  typedef std::complex<double> cx_double;
  operator klab::TSmartPointer<kl1p::TOperator<cx_double>>() const
  {
    return new kl1p::TMatrixOperator<cx_double>(arma::conv_to<arma::cx_mat>::from(m_mat));
  };
  /**
  * \brief cast to matrix
  *
  * \return matrix
  */
  operator arma::mat() const
  {
    return m_mat;
  };

  /*inherited from TOperator*/
  virtual void apply(const arma::Col<double> &in, arma::Col<double> &out);
  virtual void applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out);
  virtual void column(klab::UInt32 i, arma::Col<double> &out);
  virtual void columnAdjoint(klab::UInt32 i, arma::Col<double> &out);
  virtual void toMatrix(arma::Mat<double> &out);
  virtual void toMatrixAdjoint(arma::Mat<double> &out);

  friend arma::mat operator*(const RandomMatrix &, const arma::mat &);
  friend arma::mat operator*(const arma::mat &, const RandomMatrix &);

  friend std::ostream &operator<<(std::ostream &os, const RandomMatrix &obj);

protected:
  uint32_t m_prevSeed; /**< seed used previously*/
  arma::mat m_mat;     /**< underlying matrix*/
  int64_t m_stream;    /**< stream number*/
};

klab::TSmartPointer<kl1p::TOperator<double>> operator*(const klab::TSmartPointer<RandomMatrix>, const klab::TSmartPointer<TransMatrix<double>>);
klab::TSmartPointer<kl1p::TOperator<cx_double>> operator*(const klab::TSmartPointer<RandomMatrix>, const klab::TSmartPointer<TransMatrix<cx_double>>);

/**
* \ingroup ranmat
* \class IdentRandomMatrix
*
* \brief a mxn matrix with rows chosen randomly from a nxn identity matrix
*
*
*/
class IdentRandomMatrix : public RandomMatrix
{
public:
  /**
  * \brief create a empty matrix
  *
  */
  IdentRandomMatrix();

  /**
  * \brief create a IdentRandomMatrix of size MxN
  *
  * \param m NOF rows
  * \param n NOF columns
  *
  */
  IdentRandomMatrix(uint32_t m, uint32_t n);

  /**
  * \brief Generate random entries for a given seed (if it is different than the previous one, or if forced to)
  *
  * \param seed seed to use
  * \param force force generation
  *
  */
  virtual void Generate(uint32_t seed, bool force = false);

  /**
  * \brief clones the matrix
  *
  * \return pointer to a new matrix
  */
  virtual IdentRandomMatrix *Clone() const;

private:
  typedef UniformRandomVariable T_RanVar; /**< random variable used*/
};

/**
* \ingroup ranmat
* \class GaussianRandomMatrix 
*
* \brief a random mxn matrix containing gaussian values
*
*/
class GaussianRandomMatrix : public RandomMatrix
{
public:
  /**
  * \brief create a empty matrix
  *
  */
  GaussianRandomMatrix();

  /**
  * \brief create a GaussianRandomMatrix of size MxN with mean = 0, var = 1
  *
  * \param m     NOF rows
  * \param n     NOF columns
  *
  */
  GaussianRandomMatrix(uint32_t m, uint32_t n);

  /**
  * \brief create a GaussianRandomMatrix of size MxN with parameterized mean and variance
  *
  * \param mean  mean value of distribution
  * \param var   variance value of distribution
  * \param m     NOF rows
  * \param n     NOF columns
  *
  */
  GaussianRandomMatrix(double mean, double var, uint32_t m, uint32_t n);

  /**
  * \brief Generate random entries for a given seed (if it is different than the previous one, or if forced to)
  *
  * \param seed seed to use
  * \param force force generation
  *
  */
  virtual void Generate(uint32_t seed, bool force = false);

  /**
  * \brief clones the matrix
  *
  * \return pointer to a new matrix
  */
  virtual GaussianRandomMatrix *Clone() const;

private:
  typedef NormalRandomVariable T_RanVar; /**< random variable used*/
  double m_mean, m_var;                  /**< mean& variance of distribution*/
};

/**
* \ingroup ranmat
* \class BernRandomMatrix
*
* \brief a mxn matrix with entries (1, -1) or normalized(1/sqrt(m), -1/sqrt(m)) chosen from a bernoulli distribution
*
* The entries e_ij are distributed as such:
* P(e_ij) = 0.5 if e_ij = 1   or 1/sqrt(m)
* P(e_ij) = 0.5 if e_ij = -1  or -1/sqrt(m)
* 
* The distrubution is created with a uniform distribution and by using the inverse transform method.
*/
class BernRandomMatrix : public RandomMatrix
{
public:
  /**
  * \brief create a empty matrix
  *
  */
  BernRandomMatrix();
  /**
  * \brief create a BernRandomMatrix of size MxN
  *
  * \param m NOF rows
  * \param n NOF columns
  *
  */
  BernRandomMatrix(uint32_t m, uint32_t n);

  /**
  * \brief Generate random entries for a given seed (if it is different than the previous one, or if forced to)
  *
  * \param seed seed to use
  * \param force force generation
  *
  */
  virtual void Generate(uint32_t seed, bool force = false);

  /**
  * \brief clones the matrix
  *
  * \return pointer to a new matrix
  */
  virtual BernRandomMatrix *Clone() const;

private:
  const double m_bernP = 0.5;
  typedef UniformRandomVariable T_RanVar; /**< random variable used*/
};

#endif //RANDOM_MATRIX_H