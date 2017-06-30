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
* \class TransMatrix
*
* \brief a base clase template to create transformation matrices
*
* \tparam T data type entries of underlying matrix
*/
template<typename T>
class TransMatrix : public ns3::Object, public TOperator<T>
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


// /**
// * \class FourierTransMatrix
// *
// * \brief matrix inducing a 1D fourier transformation
// *
// */
// class FourierTransMatrix : public virtual TFourier1DOperator<cx_double>, public TransMatrix<cx_double>
// {
// public:
// 	/**
// 	* \brief create empty matrix
// 	*/
// 	FourierTransMatrix();

// 	/**
// 	* \brief create a transformation matrix of size MxN
// 	*
// 	* \param n NOF rows/columms
// 	*
// 	*/
// 	FourierTransMatrix(uint32_t n);

// 	/**
// 	* \brief sets the size of the matrix (and thus changing its entries)
// 	*
// 	* \param n NOF rows/columms
// 	*
// 	*/
// 	virtual void SetSize(uint32_t n);
// };

#endif //TRANSFORM_MATRIX_H
