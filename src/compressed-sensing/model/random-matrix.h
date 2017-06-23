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
#include <math.h> //sqrt
using namespace ns3;

/**
* \class RandomMatrix
*
* \brief base clase to create matrices with random entries
*
*/
class RandomMatrix : public Object
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
  * \brief sets the size of the matrix
  *
  * \param m NOF rows
  * \param n NOF columns
  *
  */
  void SetSize(uint32_t m, uint32_t n);

  /**
  * \brief Generate random entries for a given seed
  *
  * \param seed seed to use
  *
  */
  virtual void Generate(uint32_t seed) = 0;

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
  * \brief normalizes the matrixby 1/sqrt(m)
  */
  void Normalize();

  operator arma::mat() const
  {
    return m_mat;
  };
  friend arma::mat operator*(const RandomMatrix &, const arma::mat &);
  friend arma::mat operator*(const arma::mat &, const RandomMatrix &);
  friend std::ostream &operator<<(std::ostream &os, const RandomMatrix &obj);

protected:
  uint32_t m_prevSeed;                  /**< seed used previously*/
  arma::mat m_mat;                    /**< underlying matrix*/
  int64_t m_stream; /**< stream number*/
};

/**
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
  * \brief Generate random entries for a given seed (current seed of RNG is presevered)
  *
  * \param seed seed to use
  *
  */
  virtual void Generate(uint32_t seed);
  
  private:
    typedef UniformRandomVariable T_RanVar; /**< random variable used*/
};

/**
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
  * \brief Generate random entries for a given seed (current seed of RNG is presevered)
  *
  * \param seed seed to use
  *
  */
  virtual void Generate(uint32_t seed);

  private:
    typedef NormalRandomVariable T_RanVar; /**< random variable used*/
    double m_mean, m_var; /**< mean& variance of distribution*/
};

/**
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
  * \brief Generate random entries for a given seed (current seed of RNG is presevered)
  *
  * \param seed seed to use
  *
  */
  virtual void Generate(uint32_t seed);

private:
  const double m_bernP = 0.5;
    typedef UniformRandomVariable T_RanVar; /**< random variable used*/
};

#endif //RANDOM_MATRIX_H