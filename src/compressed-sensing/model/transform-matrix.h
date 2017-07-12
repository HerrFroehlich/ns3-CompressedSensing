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
typedef std::complex<double> cx_double;
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
* \brief a base clase template to create transformation matrices
*
* \tparam T data type entries of underlying matrix
*/
template <typename T>
class TransMatrix : public ns3::Object, public virtual TOperator<T>
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
	* \brief returns the transformation matrix
	*
	*
	* \return transformation matrix
	*/
	arma::Mat<T> GetMatrix();

	/**
	* \brief clones the object
	*
	* \return pointer to a new TransMatrix
	*/
	virtual TransMatrix *Clone() const = 0;

	operator arma::Mat<T>()
	{
		return GetMatrix();
	};

	template <typename T2>
	friend arma::Mat<T2> operator*(const TransMatrix<T2> &, const arma::Mat<T2> &);

	template <typename T2>
	friend arma::Mat<T2> operator*(const arma::Mat<T2> &, const TransMatrix<T2> &);
	template <typename T2>
	friend std::ostream &operator<<(std::ostream &os, const TransMatrix<T2> &obj);
};

/**
* \ingroup transmat
* \class FourierTransMatrix
*
* \brief matrix inducing a 1D fourier transformation
*
*/
class FourierTransMatrix : public virtual TransMatrix<cx_double>, public virtual TFourier1DOperator<cx_double>
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

	/*inherited from TOperator*/
	virtual void apply(const arma::Col<cx_double> &in, arma::Col<cx_double> &out);
	virtual void applyAdjoint(const arma::Col<cx_double> &in, arma::Col<cx_double> &out);
};

#endif //TRANSFORM_MATRIX_H
