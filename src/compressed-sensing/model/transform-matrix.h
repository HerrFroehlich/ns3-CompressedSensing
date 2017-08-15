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
	* \brief applies the inverse of the transformation
	*
	*
	* \param in input column vector
	* \param out output column vector
	*
	* \tparam T data type
	*/
	// virtual void ApplyInverse(const arma::Col<T> &in, arma::Col<T> &out) = 0;

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
* \tparam T data type
*/
template <typename T>
class FourierTransMatrix : public virtual TransMatrix<T>, public virtual TInverseFourier1DOperator<T>
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

	// virtual void ApplyInverse(const arma::Col<T> &in, arma::Col<T> &out);

	/*inherited from TOperator*/
	virtual void apply(const arma::Col<T> &in, arma::Col<T> &out);
	virtual void applyAdjoint(const arma::Col<T> &in, arma::Col<T> &out);

  private:
	// kl1p::TInverseFourier1DOperator<T> m_inverse;
};

/**
* \ingroup transmat
* \class DcTransMatrix
*
* \brief matrix inducing a real 1D discrete cosine transformatin
*
*/

template <typename T>
class DcTransMatrix : public virtual TransMatrix<T>, public virtual TInverseDCT1DOperator<T>
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

	// virtual void ApplyInverse(const arma::Col<T> &in, arma::Col<T> &out);

	/*inherited from TOperator*/
	virtual void apply(const arma::Col<T> &in, arma::Col<T> &out);
	virtual void applyAdjoint(const arma::Col<T> &in, arma::Col<T> &out);
  private:
	// kl1p::TInverseDCT1DOperator<T> m_inverse;
};
#endif //TRANSFORM_MATRIX_H
