/**
* \file transform-matrix.h
*
* \author Tobias Waurick
* \date 23.06.17
*
*/

#ifndef TRANSFORM_MATRIX_H
#define TRANSFORM_MATRIX_H
#include "ns3/core-module.h"
#include <KL1pInclude.h>
#include <armadillo>
using namespace kl1p;
/**
* \ingroup compsens
 * \defgroup transmat Transform Matrices
 *
 * Various classes for transformations
 */
/**
* \ingroup transmat
* \class TransMatrix
*
* \brief a base clase  to create real transformation matrices
*
* With this base class it is possible to create NxN transformation matrices in the real domain.
* To use both with ns3 and kl1p it inherits from ns3::Object and as well from kl1p::TOperator<double>.
*
*
*/
class TransMatrix : public ns3::Object, public virtual TOperator<double>
{
  public:
	/**
	* \brief create empty matrix
	*/
	TransMatrix();

	/**
	* \brief create a transformation matrix of size MxN
	*
	* \param n NOF rows/columms
	*
	*/
	TransMatrix(uint32_t n);

	/**
	* \brief sets the size of the matrix (and thus changing its entries)
	*
	* \param n NOF rows/columms
	*
	*/
	virtual void SetSize(uint32_t n) = 0;

	/**
	* \brief gets the NOF rows/columms
	*
	* \return NOF rows/columms
	*/
	uint32_t GetSize() const;

	/**
	* \brief applies the inverse of the transformation
	*
	*
	* \param in input column vector
	* \param out output column vector
	*
	* \tparam T data type
	*/
	// virtual void ApplyInverse(const arma::Col<double> &in, arma::Col<double> &out) = 0;

	/**
	* \brief clones the object
	*
	* \return pointer to a new TransMatrix
	*/
	virtual TransMatrix *Clone() const = 0;

};

/**
* \ingroup transmat
* \class FourierTransMatrix
*
* \brief matrix inducing a 1D fourier transformation
*
* The inverse is used here, since with the compressed sensing approaches the transformation coefficients are
*
*/
class FourierTransMatrix : public virtual TransMatrix, public virtual TInverseFourier1DOperator<double>
{
  public:
	/**
	* \brief create empty matrix
	*/
	FourierTransMatrix();

	/**
	* \brief create a transformation matrix of size MxN
	*
	* \param n NOF rows/columms
	*
	*/
	FourierTransMatrix(uint32_t n);

	/**
	* \brief sets the size of the matrix (and thus changing its entries)
	*
	* \param n NOF rows/columms
	*
	*/
	virtual void SetSize(uint32_t n);

	/**
	* \brief clones the object
	*
	* \return pointer to a new TransMatrix
	*/
	virtual FourierTransMatrix *Clone() const;

	// virtual void ApplyInverse(const arma::Col<double> &in, arma::Col<double> &out);

	/*inherited from TOperator*/
	virtual void apply(const arma::Col<double> &in, arma::Col<double> &out);
	virtual void applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out);

  private:
	// kl1p::TInverseFourier1DOperator<double> m_inverse;
};

/**
* \ingroup transmat
* \class DcTransMatrix
*
* \brief matrix inducing a real 1D discrete cosine transformatin
*
*/

class DcTransMatrix : public virtual TransMatrix, public virtual TInverseDCT1DOperator<double>
{
  public:
	/**
	* \brief create empty matrix
	*/
	DcTransMatrix();

	/**
	* \brief create a transformation matrix of size MxN
	*
	* \param n NOF rows/columms
	*
	*/
	DcTransMatrix(uint32_t n);

	/**
	* \brief sets the size of the matrix (and thus changing its entries)
	*
	* \param n NOF rows/columms
	*
	*/
	virtual void SetSize(uint32_t n);

	/**
	* \brief clones the object
	*
	* \return pointer to a new TransMatrix
	*/
	virtual DcTransMatrix *Clone() const;

	// virtual void ApplyInverse(const arma::Col<double> &in, arma::Col<double> &out);

	/*inherited from TOperator*/
	virtual void apply(const arma::Col<double> &in, arma::Col<double> &out);
	virtual void applyAdjoint(const arma::Col<double> &in, arma::Col<double> &out);
  private:
	// kl1p::TInverseDCT1DOperator<double> m_inverse;
};
#endif //TRANSFORM_MATRIX_H
